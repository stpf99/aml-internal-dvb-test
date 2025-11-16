// sources/aml_dvb/aml_dmx_core.c
// Hardware demultiplexer core - adapted for mainline 6.x

#include <linux/dvb/dmx.h>
#include <media/dvbdev.h>
#include "aml_dvb.h"

static int aml_dmx_start_feed(struct dvb_demux_feed *feed)
{
    struct aml_dvb *dvb = feed->demux->priv;
    // TODO: Program PID into hardware filter
    dev_info(&dvb->pdev->dev, "Starting feed PID=%d\n", feed->pid);
    return 0;
}

static int aml_dmx_stop_feed(struct dvb_demux_feed *feed)
{
    struct aml_dvb *dvb = feed->demux->priv;
    dev_info(&dvb->pdev->dev, "Stopping feed PID=%d\n", feed->pid);
    return 0;
}

static int aml_dmx_write_to_decoder(struct dvb_demux_feed *feed,
                                    const u8 *buf, size_t len)
{
    // Bypass - data goes to userspace
    return feed->cb.ts(buf, len, NULL, 0, &feed->feed.ts, DMX_OK);
}

int aml_dmx_init(struct aml_dvb *dvb)
{
    struct dvb_demux *demux = &dvb->demux;

    demux->priv = dvb;
    demux->filternum = 32;
    demux->feednum = 32;
    demux->start_feed = aml_dmx_start_feed;
    demux->stop_feed = aml_dmx_stop_feed;
    demux->write_to_decoder = aml_dmx_write_to_decoder;
    demux->dmx.capabilities = DMX_TS_FILTERING | DMX_SECTION_FILTERING;

    return dvb_dmx_init(demux);
}
EXPORT_SYMBOL(aml_dmx_init);

void aml_dmx_release(struct aml_dvb *dvb)
{
    dvb_dmx_release(&dvb->demux);
}
EXPORT_SYMBOL(aml_dmx_release);
