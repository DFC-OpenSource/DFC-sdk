#ifndef __INIC_CONFIG_H
#define __INIC_CONFIG_H

#define FSLU_NVME_INIC 0

#if FSLU_NVME_INIC

#define FSLU_NVME_INIC_DPAA2 1
#define FSLU_NVME_INIC_QDMA 0/*enable qdma driver before macro*/

#if FSLU_NVME_INIC_DPAA2
#define FSLU_NVME_INIC_QDMA_IRQ 1 /*qdma irq for doorbell*/
#define FSLU_NVME_INIC_FPGA_IRQ 0 /*fpga irq for doorbell*/
#endif/*FSLU_NVME_INIC_DPAA2*/

#define FSLU_NVME_INIC_POLL 1 /*polling mode for tx path*/
#define FSLU_NVME_INIC_SG 0 /*scatter gather disabled*/

#endif /* FSLU_NVME_INIC */

#endif
