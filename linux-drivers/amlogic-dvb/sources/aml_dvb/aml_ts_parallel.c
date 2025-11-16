// sources/aml_dvb/aml_ts_parallel.c
// Parallel TS input (multi-lane) - alternative to serial

#include "aml_dvb.h"

int aml_ts_parallel_init(struct aml_dvb *dvb)
{
    u32 val;

    // Configure parallel mode
    val = readl(dvb->reg_base + TS_SERIAL_CTRL);  // Reuse CTRL for parallel
    val &= ~TS_CLK_INVERT;  // Default polarity for parallel
    writel(val, dvb->reg_base + TS_SERIAL_CTRL);

    dev_info(&dvb->pdev->dev, "Parallel TS initialized\n");
    return 0;
}
EXPORT_SYMBOL(aml_ts_parallel_init);

MODULE_DESCRIPTION("Amlogic Parallel TS Module");
MODULE_LICENSE("GPL");
