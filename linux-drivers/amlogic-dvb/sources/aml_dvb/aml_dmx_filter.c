// sources/aml_dvb/aml_dmx_filter.c
// PID and section filtering for demux

#include "aml_dvb.h"

int aml_dmx_filter_set(struct dvb_demux_feed *feed, const struct dmx_section_filter *filter)
{
    struct aml_dvb *dvb = feed->demux->priv;
    // Call hardware filter add
    aml_dmx_hw_add_filter(dvb, feed->pid, filter->filter_value, filter->filter_mask, filter->filter_length);
    return 0;
}

EXPORT_SYMBOL(aml_dmx_filter_set);

MODULE_DESCRIPTION("Amlogic DVB Filter Module");
MODULE_LICENSE("GPL");
