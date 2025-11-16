// sources/aml_dvb/aml_dvb_core.c
// Core DVB subsystem for Amlogic - integrates demux, frontend, TS
// Ported for kernel 6.x, based on CoreELEC 22 patterns

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <media/dvb_demux.h>
#include <media/dmxdev.h>
#include <media/dvb_frontend.h>
#include <media/dvb_net.h>
#include "aml_dvb.h"

static int aml_dvb_core_start_feed(struct dvb_demux_feed *feed)
{
    struct aml_dvb *dvb = feed->demux->priv;
    // Call hardware PID add
    aml_dvb_reg_add_pid(dvb, feed->pid, feed->index);
    return 0;
}

static int aml_dvb_core_stop_feed(struct dvb_demux_feed *feed)
{
    struct aml_dvb *dvb = feed->demux->priv;
    // Call hardware PID remove
    aml_dvb_reg_remove_pid(dvb, feed->index);
    return 0;
}

int aml_dvb_core_init(struct aml_dvb *dvb)
{
    struct dvb_demux *demux = &dvb->demux;

    demux->priv = dvb;
    demux->filternum = AML_DVB_MAX_PIDS;
    demux->feednum = AML_DVB_MAX_PIDS;
    demux->start_feed = aml_dvb_core_start_feed;
    demux->stop_feed = aml_dvb_core_stop_feed;
    demux->write_to_decoder = NULL;  // Bypass to userspace
    demux->dmx.capabilities = DMX_TS_FILTERING | DMX_SECTION_FILTERING | DMX_PCR_EXTRACTION | DMX_MEMORY_BASED_FILTERING;

    return dvb_dmx_init(demux);
}
EXPORT_SYMBOL(aml_dvb_core_init);

void aml_dvb_core_release(struct aml_dvb *dvb)
{
    dvb_dmx_release(&dvb->demux);
}
EXPORT_SYMBOL(aml_dvb_core_release);

MODULE_DESCRIPTION("Amlogic DVB Core Module");
MODULE_AUTHOR("Ported from CoreELEC 22");
MODULE_LICENSE("GPL");
