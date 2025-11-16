// SPDX-License-Identifier: GPL-2.0+
/*
 * Amlogic DVB - Hardware Register Definitions and Access
 * File: aml_dvb_reg.c
 */

#include <linux/io.h>
#include "aml_dvb.h"

/* GXL (S905D/S905X) Hardware Register Map */
#define DVB_REG_BASE            0xc8006000

/* Core control registers */
#define TS_TOP_CONFIG           0x00
#define TS_TOP_STATUS           0x04
#define TS_FILE_CONFIG          0x08

/* TS input registers */
#define TS_PL_PID_INDEX         0x10
#define TS_PL_PID_DATA          0x14
#define TS_PL_CHAN_PTR          0x18
#define TS_PL_STATUS            0x1c

/* DMA registers */
#define TS_DMA_CONTROL          0x20
#define TS_DMA_WR_PTR           0x24
#define TS_DMA_RD_PTR           0x28
#define TS_DMA_BUFF_SIZE        0x2c
#define TS_DMA_START_ADDR       0x30
#define TS_DMA_END_ADDR         0x34

/* Interrupt registers */
#define TS_INT_CONTROL          0x40
#define TS_INT_STATUS           0x44
#define TS_INT_MASK             0x48

/* PID filter registers */
#define TS_PID_FILTER_BASE      0x100
#define TS_PID_FILTER_SIZE      256

/* Register bit definitions */

/* TS_TOP_CONFIG bits */
#define TS_TOP_CONFIG_ENABLE            BIT(0)
#define TS_TOP_CONFIG_SERIAL            BIT(1)
#define TS_TOP_CONFIG_PARALLEL          BIT(2)
#define TS_TOP_CONFIG_CLK_POL           BIT(3)
#define TS_TOP_CONFIG_SYNC_POL          BIT(4)
#define TS_TOP_CONFIG_VALID_POL         BIT(5)
#define TS_TOP_CONFIG_BIT_ENDIAN        BIT(6)
#define TS_TOP_CONFIG_BYTE_ENDIAN       BIT(7)

/* TS_DMA_CONTROL bits */
#define TS_DMA_CONTROL_ENABLE           BIT(0)
#define TS_DMA_CONTROL_RESET            BIT(1)
#define TS_DMA_CONTROL_IRQ_ENABLE       BIT(2)
#define TS_DMA_CONTROL_SG_MODE          BIT(3)

/* TS_INT_STATUS bits */
#define TS_INT_STATUS_DMA_DONE          BIT(0)
#define TS_INT_STATUS_OVERFLOW          BIT(1)
#define TS_INT_STATUS_TIMEOUT           BIT(2)
#define TS_INT_STATUS_ERROR             BIT(3)

/* Register access functions */

u32 aml_dvb_reg_read(struct aml_dvb *dvb, u32 reg)
{
    return readl(dvb->base + reg);
}

void aml_dvb_reg_write(struct aml_dvb *dvb, u32 reg, u32 val)
{
    writel(val, dvb->base + reg);
}

void aml_dvb_reg_set_bits(struct aml_dvb *dvb, u32 reg, u32 bits)
{
    u32 val = aml_dvb_reg_read(dvb, reg);
    aml_dvb_reg_write(dvb, reg, val | bits);
}

void aml_dvb_reg_clear_bits(struct aml_dvb *dvb, u32 reg, u32 bits)
{
    u32 val = aml_dvb_reg_read(dvb, reg);
    aml_dvb_reg_write(dvb, reg, val & ~bits);
}

/* Hardware initialization */
int aml_dvb_reg_init(struct aml_dvb *dvb)
{
    u32 config = 0;
    
    /* Reset all registers */
    aml_dvb_reg_write(dvb, TS_TOP_CONFIG, 0);
    aml_dvb_reg_write(dvb, TS_DMA_CONTROL, TS_DMA_CONTROL_RESET);
    usleep_range(100, 200);
    aml_dvb_reg_write(dvb, TS_DMA_CONTROL, 0);
    
    /* Configure TS mode */
    if (dvb->ts_mode == 1) {
        /* Serial mode */
        config = TS_TOP_CONFIG_ENABLE | TS_TOP_CONFIG_SERIAL;
        dvb_info(dvb, "Configuring Serial TS mode\n");
    } else if (dvb->ts_mode == 2) {
        /* Parallel mode */
        config = TS_TOP_CONFIG_ENABLE | TS_TOP_CONFIG_PARALLEL;
        dvb_info(dvb, "Configuring Parallel TS mode\n");
    } else {
        /* Auto-detect: default to serial for GXL */
        config = TS_TOP_CONFIG_ENABLE | TS_TOP_CONFIG_SERIAL;
        dvb_info(dvb, "Auto-detect: using Serial TS mode\n");
    }
    
    /* Apply clock polarity if configured */
    if (dvb->ts_clk_pol)
        config |= TS_TOP_CONFIG_CLK_POL;
    
    aml_dvb_reg_write(dvb, TS_TOP_CONFIG, config);
    
    /* Clear all interrupts */
    aml_dvb_reg_write(dvb, TS_INT_STATUS, 0xFFFFFFFF);
    
    /* Enable required interrupts */
    aml_dvb_reg_write(dvb, TS_INT_MASK,
                     TS_INT_STATUS_DMA_DONE |
                     TS_INT_STATUS_OVERFLOW |
                     TS_INT_STATUS_ERROR);
    
    return 0;
}

/* PID filter management */
int aml_dvb_reg_add_pid(struct aml_dvb *dvb, u16 pid, int index)
{
    if (index >= TS_PID_FILTER_SIZE) {
        dvb_err(dvb, "PID index out of range: %d\n", index);
        return -EINVAL;
    }
    
    /* Write PID to hardware filter */
    aml_dvb_reg_write(dvb, TS_PL_PID_INDEX, index);
    aml_dvb_reg_write(dvb, TS_PL_PID_DATA, pid & 0x1FFF);
    
    dvb_dbg(dvb, "Added PID 0x%04x at index %d\n", pid, index);
    
    return 0;
}

int aml_dvb_reg_remove_pid(struct aml_dvb *dvb, int index)
{
    if (index >= TS_PID_FILTER_SIZE) {
        dvb_err(dvb, "PID index out of range: %d\n", index);
        return -EINVAL;
    }
    
    /* Write 0x1FFF (invalid PID) to clear filter */
    aml_dvb_reg_write(dvb, TS_PL_PID_INDEX, index);
    aml_dvb_reg_write(dvb, TS_PL_PID_DATA, 0x1FFF);
    
    dvb_dbg(dvb, "Removed PID at index %d\n", index);
    
    return 0;
}

/* DMA configuration */
int aml_dvb_reg_setup_dma(struct aml_dvb *dvb, dma_addr_t addr, size_t size)
{
    /* Set DMA buffer addresses */
    aml_dvb_reg_write(dvb, TS_DMA_START_ADDR, addr);
    aml_dvb_reg_write(dvb, TS_DMA_END_ADDR, addr + size);
    aml_dvb_reg_write(dvb, TS_DMA_BUFF_SIZE, size);
    
    /* Reset pointers */
    aml_dvb_reg_write(dvb, TS_DMA_WR_PTR, addr);
    aml_dvb_reg_write(dvb, TS_DMA_RD_PTR, addr);
    
    dvb_info(dvb, "DMA configured: addr=0x%pad size=%zu\n", &addr, size);
    
    return 0;
}

void aml_dvb_reg_start_dma(struct aml_dvb *dvb)
{
    u32 control = TS_DMA_CONTROL_ENABLE | TS_DMA_CONTROL_IRQ_ENABLE;
    
    /* Enable scatter-gather if configured */
    if (dvb->dma_sg)
        control |= TS_DMA_CONTROL_SG_MODE;
    
    aml_dvb_reg_write(dvb, TS_DMA_CONTROL, control);
    
    dvb_dbg(dvb, "DMA started\n");
}

void aml_dvb_reg_stop_dma(struct aml_dvb *dvb)
{
    aml_dvb_reg_write(dvb, TS_DMA_CONTROL, 0);
    dvb_dbg(dvb, "DMA stopped\n");
}

/* Status reporting */
void aml_dvb_reg_dump(struct aml_dvb *dvb)
{
    dev_info(dvb->dev, "=== DVB Register Dump ===\n");
    dev_info(dvb->dev, "TS_TOP_CONFIG:   0x%08x\n",
            aml_dvb_reg_read(dvb, TS_TOP_CONFIG));
    dev_info(dvb->dev, "TS_TOP_STATUS:   0x%08x\n",
            aml_dvb_reg_read(dvb, TS_TOP_STATUS));
    dev_info(dvb->dev, "TS_DMA_CONTROL:  0x%08x\n",
            aml_dvb_reg_read(dvb, TS_DMA_CONTROL));
    dev_info(dvb->dev, "TS_DMA_WR_PTR:   0x%08x\n",
            aml_dvb_reg_read(dvb, TS_DMA_WR_PTR));
    dev_info(dvb->dev, "TS_DMA_RD_PTR:   0x%08x\n",
            aml_dvb_reg_read(dvb, TS_DMA_RD_PTR));
    dev_info(dvb->dev, "TS_INT_STATUS:   0x%08x\n",
            aml_dvb_reg_read(dvb, TS_INT_STATUS));
    dev_info(dvb->dev, "=========================\n");
}
