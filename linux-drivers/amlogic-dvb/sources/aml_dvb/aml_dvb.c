// SPDX-License-Identifier: GPL-2.0+
/*
 * Amlogic DVB Core Driver
 * Ported from CoreELEC 22 (kernel 5.15.170) to LibreELEC (kernel 6.x)
 * Original author: mczerski
 * Adaptation: AI-assisted portingfor S905D (GXL) support
 *
 * This driver provides DVB adapter support for Amlogic SoCs with
 * built-in Transport Stream interface (serial/parallel).
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/reset.h>

#include <media/dvb_demux.h>
#include <media/dmxdev.h>
#include <media/dvb_frontend.h>
#include <media/dvb_net.h>

#include "aml_dvb.h"

#define DRIVER_NAME "aml_dvb"
#define DRIVER_VERSION "6.0-gxl"

/* Hardware register offsets for GXL (S905D/S905X) */
#define DVB_REG_BASE        0xc8006000
#define TS_PL_PID_INDEX     0x00
#define TS_PL_PID_DATA      0x04
#define TS_PL_CHAN_PTR      0x08
#define TS_CONTROL          0x10
#define TS_CONFIG           0x14
#define TS_STATUS           0x18

/* Device structure */
struct aml_dvb {
    struct device *dev;
    struct platform_device *pdev;
    void __iomem *base;
    struct clk *clk;
    struct reset_control *reset;
    
    /* DVB adapter */
    struct dvb_adapter adapter;
    struct dvb_demux demux;
    struct dmxdev dmxdev;
    struct dvb_net net;
    struct dvb_frontend *frontend;
    
    /* DMA buffer */
    void *dma_buf;
    dma_addr_t dma_addr;
    size_t dma_size;
    
    /* TS mode: 0=auto, 1=serial, 2=parallel */
    int ts_mode;
    
    /* IRQ */
    int irq;
    
    /* Reference count */
    struct kref refcount;
};

/* Forward declarations */
static int aml_dvb_probe(struct platform_device *pdev);
static void aml_dvb_remove_new(struct platform_device *pdev); /* kernel 6.x uses remove_new */
static int aml_dvb_start_feed(struct dvb_demux_feed *feed);
static int aml_dvb_stop_feed(struct dvb_demux_feed *feed);

/* Device tree match table */
static const struct of_device_id aml_dvb_dt_match[] = {
    {
        .compatible = "amlogic,dvb",
        .data = (void *)AML_DVB_HW_GXL,  /* S905D/S905X */
    },
    {
        .compatible = "amlogic,dvb-gxl",
        .data = (void *)AML_DVB_HW_GXL,
    },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, aml_dvb_dt_match);

/* Platform driver structure */
static struct platform_driver aml_dvb_driver = {
    .probe = aml_dvb_probe,
    .remove_new = aml_dvb_remove_new,  /* Kernel 6.x API change */
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = aml_dvb_dt_match,
        .owner = THIS_MODULE,
    },
};

/* IRQ handler */
static irqreturn_t aml_dvb_irq_handler(int irq, void *dev_id)
{
    struct aml_dvb *dvb = dev_id;
    u32 status;
    
    status = readl(dvb->base + TS_STATUS);
    
    if (status & TS_IRQ_PENDING) {
        /* Handle TS data */
        dvb_dmx_swfilter(&dvb->demux, dvb->dma_buf, dvb->dma_size);
        
        /* Clear IRQ */
        writel(status | TS_IRQ_CLEAR, dvb->base + TS_STATUS);
        
        return IRQ_HANDLED;
    }
    
    return IRQ_NONE;
}

/* Initialize hardware */
static int aml_dvb_hw_init(struct aml_dvb *dvb)
{
    int ret;
    u32 config = 0;
    
    /* Enable clock */
    ret = clk_prepare_enable(dvb->clk);
    if (ret) {
        dev_err(dvb->dev, "Failed to enable clock: %d\n", ret);
        return ret;
    }
    
    /* Reset hardware */
    reset_control_assert(dvb->reset);
    usleep_range(100, 200);
    reset_control_deassert(dvb->reset);
    usleep_range(100, 200);
    
    /* Configure TS mode */
    switch (dvb->ts_mode) {
    case 1: /* Serial */
        config = TS_SERIAL_MODE | TS_SERIAL_CLK_POL;
        dev_info(dvb->dev, "Using Serial TS mode\n");
        break;
    case 2: /* Parallel */
        config = TS_PARALLEL_MODE;
        dev_info(dvb->dev, "Using Parallel TS mode\n");
        break;
    default: /* Auto-detect */
        config = TS_SERIAL_MODE; /* Default to serial for GXL */
        dev_info(dvb->dev, "Auto-detect: using Serial TS mode\n");
        break;
    }
    
    writel(config, dvb->base + TS_CONFIG);
    
    /* Allocate DMA buffer - kernel 6.x compatible */
    dvb->dma_size = 188 * 1024; /* 188KB for TS packets */
    dvb->dma_buf = dma_alloc_coherent(dvb->dev, dvb->dma_size,
                                      &dvb->dma_addr, GFP_KERNEL);
    if (!dvb->dma_buf) {
        dev_err(dvb->dev, "Failed to allocate DMA buffer\n");
        clk_disable_unprepare(dvb->clk);
        return -ENOMEM;
    }
    
    /* Configure DMA */
    writel(dvb->dma_addr, dvb->base + TS_DMA_ADDR);
    writel(dvb->dma_size, dvb->base + TS_DMA_SIZE);
    
    /* Enable interrupts */
    writel(TS_IRQ_ENABLE, dvb->base + TS_CONTROL);
    
    return 0;
}

/* Cleanup hardware */
static void aml_dvb_hw_exit(struct aml_dvb *dvb)
{
    /* Disable interrupts */
    writel(0, dvb->base + TS_CONTROL);
    
    /* Free DMA buffer */
    if (dvb->dma_buf) {
        dma_free_coherent(dvb->dev, dvb->dma_size,
                         dvb->dma_buf, dvb->dma_addr);
        dvb->dma_buf = NULL;
    }
    
    /* Disable clock */
    clk_disable_unprepare(dvb->clk);
}

/* Demux start feed callback */
static int aml_dvb_start_feed(struct dvb_demux_feed *feed)
{
    struct aml_dvb *dvb = feed->demux->priv;
    
    dev_dbg(dvb->dev, "Start feed: PID=0x%04x\n", feed->pid);
    
    /* Hardware PID filtering can be added here */
    
    return 0;
}

/* Demux stop feed callback */
static int aml_dvb_stop_feed(struct dvb_demux_feed *feed)
{
    struct aml_dvb *dvb = feed->demux->priv;
    
    dev_dbg(dvb->dev, "Stop feed: PID=0x%04x\n", feed->pid);
    
    return 0;
}

/* Probe function */
static int aml_dvb_probe(struct platform_device *pdev)
{
    struct aml_dvb *dvb;
    struct resource *res;
    int ret;
    
    dev_info(&pdev->dev, "Amlogic DVB driver probe (kernel 6.x)\n");
    
    /* Allocate driver data */
    dvb = devm_kzalloc(&pdev->dev, sizeof(*dvb), GFP_KERNEL);
    if (!dvb)
        return -ENOMEM;
    
    dvb->dev = &pdev->dev;
    dvb->pdev = pdev;
    platform_set_drvdata(pdev, dvb);
    
    /* Get TS mode from device tree */
    of_property_read_u32(pdev->dev.of_node, "ts-mode", &dvb->ts_mode);
    
    /* Get memory resource */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    dvb->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(dvb->base))
        return PTR_ERR(dvb->base);
    
    /* Get clock */
    dvb->clk = devm_clk_get(&pdev->dev, "dvb");
    if (IS_ERR(dvb->clk)) {
        dev_err(&pdev->dev, "Failed to get clock\n");
        return PTR_ERR(dvb->clk);
    }
    
    /* Get reset control */
    dvb->reset = devm_reset_control_get(&pdev->dev, "dvb");
    if (IS_ERR(dvb->reset)) {
        dev_err(&pdev->dev, "Failed to get reset control\n");
        return PTR_ERR(dvb->reset);
    }
    
    /* Get IRQ */
    dvb->irq = platform_get_irq(pdev, 0);
    if (dvb->irq < 0) {
        dev_err(&pdev->dev, "Failed to get IRQ\n");
        return dvb->irq;
    }
    
    /* Request IRQ */
    ret = devm_request_irq(&pdev->dev, dvb->irq, aml_dvb_irq_handler,
                          0, DRIVER_NAME, dvb);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ: %d\n", ret);
        return ret;
    }
    
    /* Initialize hardware */
    ret = aml_dvb_hw_init(dvb);
    if (ret)
        return ret;
    
    /* Register DVB adapter */
    ret = dvb_register_adapter(&dvb->adapter, "Amlogic DVB",
                              THIS_MODULE, &pdev->dev,
                              adapter_nr);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to register DVB adapter: %d\n", ret);
        goto err_hw_exit;
    }
    
    /* Initialize demux */
    dvb->demux.dmx.capabilities = DMX_TS_FILTERING | DMX_SECTION_FILTERING;
    dvb->demux.priv = dvb;
    dvb->demux.filternum = 256;
    dvb->demux.feednum = 256;
    dvb->demux.start_feed = aml_dvb_start_feed;
    dvb->demux.stop_feed = aml_dvb_stop_feed;
    
    ret = dvb_dmx_init(&dvb->demux);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to init demux: %d\n", ret);
        goto err_unregister_adapter;
    }
    
    /* Register dmxdev */
    dvb->dmxdev.filternum = 256;
    dvb->dmxdev.demux = &dvb->demux.dmx;
    dvb->dmxdev.capabilities = 0;
    
    ret = dvb_dmxdev_init(&dvb->dmxdev, &dvb->adapter);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to init dmxdev: %d\n", ret);
        goto err_dmx_release;
    }
    
    /* Initialize DVB net */
    ret = dvb_net_init(&dvb->adapter, &dvb->net, &dvb->demux.dmx);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to init DVB net: %d\n", ret);
        goto err_dmxdev_release;
    }
    
    dev_info(&pdev->dev, "Amlogic DVB adapter registered successfully\n");
    dev_info(&pdev->dev, "Device: /dev/dvb/adapter%d/\n", dvb->adapter.num);
    
    return 0;

err_dmxdev_release:
    dvb_dmxdev_release(&dvb->dmxdev);
err_dmx_release:
    dvb_dmx_release(&dvb->demux);
err_unregister_adapter:
    dvb_unregister_adapter(&dvb->adapter);
err_hw_exit:
    aml_dvb_hw_exit(dvb);
    return ret;
}

/* Remove function - kernel 6.x uses remove_new */
static void aml_dvb_remove_new(struct platform_device *pdev)
{
    struct aml_dvb *dvb = platform_get_drvdata(pdev);
    
    dev_info(&pdev->dev, "Removing Amlogic DVB driver\n");
    
    /* Unregister DVB components */
    dvb_net_release(&dvb->net);
    dvb_dmxdev_release(&dvb->dmxdev);
    dvb_dmx_release(&dvb->demux);
    dvb_unregister_adapter(&dvb->adapter);
    
    /* Cleanup hardware */
    aml_dvb_hw_exit(dvb);
    
    /* Note: kernel 6.x remove_new returns void, not int */
}

/* Module initialization */
static int __init aml_dvb_init(void)
{
    pr_info("Amlogic DVB driver " DRIVER_VERSION " (kernel 6.x)\n");
    return platform_driver_register(&aml_dvb_driver);
}

/* Module cleanup */
static void __exit aml_dvb_exit(void)
{
    platform_driver_unregister(&aml_dvb_driver);
    pr_info("Amlogic DVB driver unloaded\n");
}

module_init(aml_dvb_init);
module_exit(aml_dvb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mczerski, AI-assisted porting");
MODULE_DESCRIPTION("Amlogic DVB driver for S905D/S905X (GXL)");
MODULE_VERSION(DRIVER_VERSION);
MODULE_ALIAS("platform:" DRIVER_NAME);

/* Module parameters */
static int adapter_nr[] = {[0 ... (DVB_MAX_ADAPTERS - 1)] = -1};
module_param_array(adapter_nr, int, NULL, 0444);
MODULE_PARM_DESC(adapter_nr, "DVB adapter numbers");

static int debug;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Debug level (0-5)");
