// sources/aml_dvb/aml_ts_dma.c
// DMA ring buffer for TS data

#include <linux/dmaengine.h>
#include "aml_dvb.h"

int aml_ts_dma_init(struct aml_dvb *dvb)
{
    dvb->dma_buf = dma_alloc_coherent(&dvb->pdev->dev, TS_DMA_SIZE,
                                     &dvb->dma_handle, GFP_KERNEL);
    if (!dvb->dma_buf)
        return -ENOMEM;

    // TODO: Setup DMA descriptor ring
    dev_info(&dvb->pdev->dev, "DMA buffer allocated: %zu bytes\n", TS_DMA_SIZE);
    return 0;
}
EXPORT_SYMBOL(aml_ts_dma_init);

void aml_ts_dma_exit(struct aml_dvb *dvb)
{
    if (dvb->dma_buf)
        dma_free_coherent(&dvb->pdev->dev, TS_DMA_SIZE,
                          dvb->dma_buf, dvb->dma_handle);
}
EXPORT_SYMBOL(aml_ts_dma_exit);
