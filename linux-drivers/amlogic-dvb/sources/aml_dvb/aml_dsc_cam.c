// sources/aml_dvb/aml_dsc_cam.c
// CAM (Conditional Access Module) interface

#include "aml_dvb.h"

int aml_dsc_cam_set_key(struct aml_dvb *dvb, u8 *key, int key_len)
{
    // TODO: Program hardware key for descrambling
    dev_dbg(dvb->dev, "CAM key set (len=%d)\n", key_len);
    return 0;
}

EXPORT_SYMBOL(aml_dsc_cam_set_key);

MODULE_DESCRIPTION("Amlogic DVB CAM Module");
MODULE_LICENSE("GPL");
