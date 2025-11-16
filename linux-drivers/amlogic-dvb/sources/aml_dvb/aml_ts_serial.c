// sources/aml_dvb/aml_ts_serial.c
// Serial TS input (single lane) - common in Mecool M8S

#include "aml_dvb.h"

int aml_ts_serial_init(struct aml_dvb *dvb)
{
    u32 val;

    // Configure serial clock polarity
    val = readl(dvb->reg_base + TS_SERIAL_CTRL);
    val |= TS_CLK_INVERT | TS_VALID_ON_FALLING;
    writel(val, dvb->reg_base + TS_SERIAL_CTRL);

    dev_info(&dvb->pdev->dev, "Serial TS initialized\n");
    return 0;
}
EXPORT_SYMBOL(aml_ts_serial_init);
