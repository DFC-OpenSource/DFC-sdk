/*
 * Shared Definitions for the PCINet / PCISerial drivers
 * 
 * Copyright (c) 2008 Ira W. Snyder <[hidden email]>
 * 
 * Heavily inspired by the drivers/net/fs_enet driver
 * 
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef PCINET_H
#define PCINET_H
#include "config.h"
#include <linux/kernel.h>
#include <linux/if_ether.h>

/* Ring and Frame size -- these must match between the drivers */
#if FSLU_NVME_INIC_DPAA2
#define PH_RING_SIZE	(168000)
#define RX_BD_RING_SIZE	(84000)
#endif
#if FSLU_NVME_INIC_QDMA
#define PH_RING_SIZE	(16800)
#define RX_BD_RING_SIZE	(16800)
#endif
#define REFILL_POOL_SIZE (350)
#define REFILL_RING_SIZE  (30)
#define RX_NAPI_WEIGHT	    64 
#define TX_FREE_NAPI_WEIGHT	256 
#define PH_MAX_FRSIZE	(64*1024)
#define PH_MAX_MTU	(PH_MAX_FRSIZE - ETH_HLEN)
#define PH_MTU            4000
#define QBMAN_POOL        0x11800
#define LINK_STATUS 0x2
#define LINK_STATE_UP 0x8
#define LINK_STATE_DOWN 0x4
#if FSLU_NVME_INIC_DPAA2
#define BP_PULL_SET 0x1
#define BP_PULL_DONE 0x2
#define BP_PULL_CLEAR 0x3
#endif

struct circ_buf_desc {
	__le32 len_offset_flag;
	__le32 addr_lower;
	__le32 addr_higher;
	__le32 fd_err_frc; 
};
struct refill_mem_pool {
	__le32 pool_addr; 
	__le32 pool_flag;
};

struct x86mem_flags {
	__le32 tx_irq_flag; 
        __le32 link_state_update;
        __le32 link_state;
        __le32 padding;


};


struct buf_pool_desc {
	__le32 pool_set;
	__le32 pool_addr;
	__le32 pool_len;
	__le32 pool_refill;
	__le32 napi_msi;
	__le32 tx_data_offset;
	__le32 tx_irq;
	__le32 conf_irq;
	int8_t iface_status;
	int8_t mac_id[7];
	__le32 rx_desc_base; 
	__le32 tx_conf_desc_base; 
	__le16 bpid;
	__le16 obj_id;
#if FSLU_NVME_INIC_DPAA2 
    __le16 pull_rx_buffers;
#endif
};

/* Buffer Descriptor Registers */
#define INTERFACE_REG           0x300000
#define PCINET_TXBD_BASE        0x800 //leaving 1 k for flag
#define PCINET_RXBD_BASE        0x200000  //0x7fd00 //adding 21k*24 with 64 bytes aligned
#define PCINET_TXCONF_BASE      0x7fd00 //no sep mem for conform desc
//Interupt
#define PCIDMA_INT_MODE_NONE            0
#define PCIDMA_INT_MODE_LEGACY          1
#define PCIDMA_INT_MODE_MSI             2
#define PCIDMA_INT_MODE_MSIX            3
#define FLAG_MSI_ENABLED 1


/* Buffer Descriptor Status */
#define BD_MEM_READY     0x01
#define BD_MEM_DIRTY     0x02
#define BD_MEM_FREE      0x04
#define BD_MEM_CONSUME	 0x08
#define TX_CONF_ENABLE	 0x05
#define TX_CONF_DISABLE	 0x06
/*IRQ SET */
#define BD_IRQ_SET       0x4
#define BD_IRQ_MASK      0x8
#define BD_IRQ_CLEAR     0x2

/* Status Register Bits */
#define PCINET_UART_RX_ENABLED	 (1<<0)
#define PCINET_NET_STATUS_RUNNING	(1<<1)
#define PCINET_NET_RXINT_OFF	 (1<<2)
#define PCINET_NET_REGISTERS_VALID	(1<<3)

/* Driver State Bits */ 
#define NET_STATE_STOPPED	 0
#define NET_STATE_RUNNING	 1

#endif /* PCINET_H */
