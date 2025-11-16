// sources/aml_dvb/aml_ts_core.c
// TS input handling - serial/parallel mode

#include "aml_dvb.h"

static void aml_ts_set_mode(struct aml_dvb *dvb, int serial)
{
    u32 val = readl(dvb->reg_base + TS_CTRL_REG);

    if (serial) {
        val |= TS_SERIAL_MODE;
        val &= ~TS_PARALLEL_MASK;
    } else {
        val &= ~TS_SERIAL_MODE;
        val |= TS_PARALLEL_8BIT;
    }

    writel(val, dvb->reg_base + TS_CTRL_REG);
    dev_info(&dvb->pdev->dev, "TS mode: %s\n", serial ? "SERIAL" : "PARALLEL");
}

int aml_ts_init(struct aml_dvb *dvb)
{
    bool serial = of_property_read_bool(dvb->pdev->dev.of_node, "ts-serial");

    aml_ts_set_mode(dvb, serial);

    // Enable TS input
    writel(TS_ENABLE, dvb->reg_base + TS_INPUT_CTRL);

    return 0;
}
EXPORT_SYMBOL(aml_ts_init);
