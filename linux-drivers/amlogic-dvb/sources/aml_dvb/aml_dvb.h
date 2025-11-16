// SPDX-License-Identifier: GPL-2.0+
/*
 * Amlogic DVB Driver Header
 * Definitions and structures for Amlogic DVB subsystem
 */

#ifndef __AML_DVB_H__
#define __AML_DVB_H__

#include <linux/types.h>
#include <linux/device.h>
#include <media/dvb_demux.h>
#include <media/dmxdev.h>
#include <media/dvb_frontend.h>
#include <media/dvb_net.h>

/* Hardware variants */
enum aml_dvb_hw_type {
    AML_DVB_HW_GXBB = 0,  /* S905 */
    AML_DVB_HW_GXL  = 1,  /* S905D, S905X, S905W */
    AML_DVB_HW_GXM  = 2,  /* S912 */
    AML_DVB_HW_G12A = 3,  /* S905X2, S905D2 */
    AML_DVB_HW_G12B = 4,  /* S922X */
    AML_DVB_HW_SM1  = 5,  /* S905X3, S905D3 */
    AML_DVB_HW_SC2  = 6,  /* S905X4 */
};

/* TS mode configuration */
#define TS_MODE_AUTO     0
#define TS_MODE_SERIAL   1
#define TS_MODE_PARALLEL 2

/* Hardware register bits */
#define TS_SERIAL_MODE      BIT(0)
#define TS_PARALLEL_MODE    BIT(1)
#define TS_SERIAL_CLK_POL   BIT(2)
#define TS_IRQ_ENABLE       BIT(8)
#define TS_IRQ_PENDING      BIT(9)
#define TS_IRQ_CLEAR        BIT(10)

/* TS packet size */
#define TS_PACKET_SIZE      188
#define TS_BUFFER_SIZE      (TS_PACKET_SIZE * 1024)

/* Maximum number of PIDs */
#define AML_DVB_MAX_PIDS    256

/* DMA register offsets */
#define TS_DMA_ADDR         0x20
#define TS_DMA_SIZE         0x24
#define TS_DMA_CONTROL      0x28

/* Device structure forward declaration */
struct aml_dvb;

/* TS interface configuration */
struct aml_ts_config {
    int mode;           /* Serial or parallel */
    int clk_pol;        /* Clock polarity */
    int sync_pol;       /* Sync polarity */
    int valid_pol;      /* Valid signal polarity */
    int bit_endian;     /* Bit endianness */
    int byte_endian;    /* Byte endianness */
};

/* Frontend configuration */
struct aml_frontend_config {
    int i2c_bus;        /* I2C bus number */
    int i2c_addr;       /* Frontend I2C address */
    int tuner_i2c_addr; /* Tuner I2C address */
    int reset_gpio;     /* Reset GPIO */
    int power_gpio;     /* Power GPIO */
};

/* DVB adapter private data */
struct aml_dvb_adapter {
    struct dvb_adapter adapter;
    struct dvb_demux demux;
    struct dmxdev dmxdev;
    struct dvb_net net;
    struct dvb_frontend *frontend;
    
    int adapter_num;
    int hw_type;
};

/* Function prototypes - Hardware control */
int aml_dvb_hw_init(struct aml_dvb *dvb);
void aml_dvb_hw_exit(struct aml_dvb *dvb);
int aml_dvb_hw_set_ts_mode(struct aml_dvb *dvb, int mode);
u32 aml_dvb_hw_get_status(struct aml_dvb *dvb);

/* Function prototypes - DMA */
int aml_dvb_dma_init(struct aml_dvb *dvb);
void aml_dvb_dma_exit(struct aml_dvb *dvb);
void aml_dvb_dma_start(struct aml_dvb *dvb);
void aml_dvb_dma_stop(struct aml_dvb *dvb);

/* Function prototypes - Frontend */
int aml_dvb_register_frontend(struct aml_dvb *dvb, struct dvb_frontend *fe);
void aml_dvb_unregister_frontend(struct aml_dvb *dvb);

/* Function prototypes - Demux */
int aml_dvb_demux_init(struct aml_dvb *dvb);
void aml_dvb_demux_exit(struct aml_dvb *dvb);

/* Debug macros */
#ifdef DEBUG
#define dvb_dbg(dvb, fmt, ...) \
    dev_dbg((dvb)->dev, "%s: " fmt, __func__, ##__VA_ARGS__)
#else
#define dvb_dbg(dvb, fmt, ...) do {} while (0)
#endif

#define dvb_info(dvb, fmt, ...) \
    dev_info((dvb)->dev, fmt, ##__VA_ARGS__)

#define dvb_warn(dvb, fmt, ...) \
    dev_warn((dvb)->dev, fmt, ##__VA_ARGS__)

#define dvb_err(dvb, fmt, ...) \
    dev_err((dvb)->dev, fmt, ##__VA_ARGS__)

/* Hardware register read/write helpers */
static inline u32 aml_dvb_read(struct aml_dvb *dvb, u32 reg)
{
    return readl(dvb->base + reg);
}

static inline void aml_dvb_write(struct aml_dvb *dvb, u32 reg, u32 val)
{
    writel(val, dvb->base + reg);
}

static inline void aml_dvb_set_bits(struct aml_dvb *dvb, u32 reg, u32 bits)
{
    u32 val = aml_dvb_read(dvb, reg);
    aml_dvb_write(dvb, reg, val | bits);
}

static inline void aml_dvb_clear_bits(struct aml_dvb *dvb, u32 reg, u32 bits)
{
    u32 val = aml_dvb_read(dvb, reg);
    aml_dvb_write(dvb, reg, val & ~bits);
}

#endif /* __AML_DVB_H__ */
