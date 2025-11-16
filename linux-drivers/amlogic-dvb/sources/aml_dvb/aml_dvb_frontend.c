// sources/aml_dvb/aml_dvb_frontend.c
// DVB Frontend attachment (M88RS6060)

#include <media/dvb_frontend.h>
#include "aml_dvb.h"

int aml_dvb_attach_frontend(struct aml_dvb *dvb)
{
    struct dvb_frontend *fe;
    void *frontend_priv;

    // M88RS6060 is already in mainline
    fe = dvb_attach(m88rs6060_attach, &m88rs6060_config);
    if (!fe) {
        dev_err(&dvb->pdev->dev, "Failed to attach M88RS6060\n");
        return -ENODEV;
    }

    if (dvb_attach(dvb_create_simple_tuner, fe)) {
        dev_info(&dvb->pdev->dev, "Simple tuner attached\n");
    }

    dvb->fe = fe;
    return 0;
}
EXPORT_SYMBOL(aml_dvb_attach_frontend);
