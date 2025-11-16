// sources/aml_dvb/aml_dmx_section.c
// Section demux handling

#include "aml_dvb.h"

static void aml_dmx_section_cb(const u8 *buffer, size_t len, void *priv)
{
    struct dvb_demux_feed *feed = priv;
    // Pass to userspace
    feed->cb.section(buffer, len, NULL, 0, &feed->filter);
}

int aml_dmx_section_init(struct aml_dvb *dvb)
{
    // Setup section callback
    dvb->demux.dmx.section_cb = aml_dmx_section_cb;
    dev_info(dvb->dev, "Section demux initialized\n");
    return 0;
}

EXPORT_SYMBOL(aml_dmx_section_init);

MODULE_DESCRIPTION("Amlogic DVB Section Module");
MODULE_LICENSE("GPL");
