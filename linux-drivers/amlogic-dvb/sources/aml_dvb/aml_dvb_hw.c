// sources/aml_dvb/aml_dvb_hw.c
// Hardware initialization for Amlogic DVB (GXL family - S905D)
// Based on CoreELEC 22 + chewitt/dvb-sucks-more + mainline 6.x API

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/dma-mapping.h>
#include "aml_dvb.h"

static int aml_dvb_hw_init(struct aml_dvb *dvb)
{
    struct device *dev = &dvb->pdev->dev;
    int ret;

    // Clock setup
    dvb->clk_demux = devm_clk_get(dev, "demux");
    if (IS_ERR(dvb->clk_demux)) {
        dev_err(dev, "cannot get demux clock\n");
        return PTR_ERR(dvb->clk_demux);
    }

    dvb->clk_ts = devm_clk_get(dev, "ts");
    if (IS_ERR(dvb->clk_ts)) {
        dev_warn(dev, "ts clock not defined\n");
        dvb->clk_ts = NULL;
    }

    ret = clk_prepare_enable(dvb->clk_demux);
    if (ret)
        return ret;

    if (dvb->clk_ts) {
        ret = clk_prepare_enable(dvb->clk_ts);
        if (ret)
            goto err_demux;
    }

    // Reset controller
    dvb->rst_demux = devm_reset_control_get_shared(dev, "demux");
    if (IS_ERR(dvb->rst_demux)) {
        dev_err(dev, "cannot get demux reset\n");
        ret = PTR_ERR(dvb->rst_demux);
        goto err_ts;
    }

    reset_control_deassert(dvb->rst_demux);

    // Memory mapped I/O
    dvb->reg_base = devm_platform_ioremap_resource(dvb->pdev, 0);
    if (IS_ERR(dvb->reg_base)) {
        ret = PTR_ERR(dvb->reg_base);
        goto err_reset;
    }

    dev_info(dev, "Amlogic DVB hardware initialized\n");
    return 0;

err_reset:
    reset_control_assert(dvb->rst_demux);
err_ts:
    if (dvb->clk_ts)
        clk_disable_unprepare(dvb->clk_ts);
err_demux:
    clk_disable_unprepare(dvb->clk_demux);
    return ret;
}

static void aml_dvb_hw_exit(struct aml_dvb *dvb)
{
    if (dvb->clk_ts)
        clk_disable_unprepare(dvb->clk_ts);
    clk_disable_unprepare(dvb->clk_demux);
    reset_control_assert(dvb->rst_demux);
}

int aml_dvb_hw_probe(struct aml_dvb *dvb)
{
    return aml_dvb_hw_init(dvb);
}
EXPORT_SYMBOL(aml_dvb_hw_probe);

void aml_dvb_hw_remove(struct aml_dvb *dvb)
{
    aml_dvb_hw_exit(dvb);
}
EXPORT_SYMBOL(aml_dvb_hw_remove);

MODULE_DESCRIPTION("Amlogic DVB Hardware Layer");
MODULE_AUTHOR("AI-Ported from CoreELEC 22");
MODULE_LICENSE("GPL");
