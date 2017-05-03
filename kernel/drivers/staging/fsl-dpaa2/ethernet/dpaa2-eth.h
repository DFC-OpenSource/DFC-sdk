/* Copyright 2014-2015 Freescale Semiconductor Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __DPAA2_ETH_H
#define __DPAA2_ETH_H

#include<inic_config.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include "../../fsl-mc/include/fsl_dpaa2_io.h"
#include "../../fsl-mc/include/fsl_dpaa2_fd.h"
#include "../../fsl-mc/include/dpbp.h"
#include "../../fsl-mc/include/dpbp-cmd.h"
#include "../../fsl-mc/include/dpcon.h"
#include "../../fsl-mc/include/dpcon-cmd.h"
#include "dpni.h"
#include "dpni-cmd.h"

#include "dpaa2-eth-trace.h"
#include "dpaa2-eth-debugfs.h"

#define DPAA2_ETH_STORE_SIZE		16

/* We set a max threshold for how many Tx confirmations we should process
 * on a NAPI poll call, they take less processing time.
 */
#define TX_CONF_PER_NAPI_POLL		256

/* Maximum number of scatter-gather entries in an ingress frame,
 * considering the maximum receive frame size is 64K
 */
#define DPAA2_ETH_MAX_SG_ENTRIES	((64 * 1024) / DPAA2_ETH_RX_BUF_SIZE)

/* Maximum acceptable MTU value. It is in direct relation with the hardware
 * enforced Max Frame Length (currently 10k).
 */
#define DPAA2_ETH_MFL			(10 * 1024)
#define DPAA2_ETH_MAX_MTU		(DPAA2_ETH_MFL - VLAN_ETH_HLEN)
/* Convert L3 MTU to L2 MFL */
#define DPAA2_ETH_L2_MAX_FRM(mtu)	(mtu + VLAN_ETH_HLEN)

/* Set the taildrop threshold (in bytes) to allow the enqueue of several jumbo
 * frames in the Rx queues (length of the current frame is not
 * taken into account when making the taildrop decision)
 */
#define DPAA2_ETH_TAILDROP_THRESH	(64 * 1024)

/* Buffer quota per queue. Must be large enough such that for minimum sized
 * frames taildrop kicks in before the bpool gets depleted, so we compute
 * how many 64B frames fit inside the taildrop threshold and add a margin
 * to accommodate the buffer refill delay.
 */
#define DPAA2_ETH_MAX_FRAMES_PER_QUEUE	(DPAA2_ETH_TAILDROP_THRESH / 64)
#if FSLU_NVME_INIC
#if FSLU_NVME_INIC_DPAA2
#define DPAA2_ETH_NUM_BUFS		(300 * 7 * 8)
#endif
#if FSLU_NVME_INIC_QDMA
#define DPAA2_ETH_NUM_BUFS		(DPAA2_ETH_MAX_FRAMES_PER_QUEUE + 256)
#define DPAA2_ETH_NUM_TX_BUFS		(300 * 7 * 8)
#endif
#define DPAA2_ETH_RX_BUFFER_SIZE	9216

#else
#define DPAA2_ETH_RX_BUFFER_SIZE	2048
#define DPAA2_ETH_NUM_BUFS		(DPAA2_ETH_MAX_FRAMES_PER_QUEUE + 256)
#endif

#define DPAA2_ETH_REFILL_THRESH		DPAA2_ETH_MAX_FRAMES_PER_QUEUE

/* Maximum number of buffers that can be acquired/released through a single
 * QBMan command
 */
#define DPAA2_ETH_BUFS_PER_CMD		7

/* Hardware requires alignment for ingress/egress buffer addresses
 * and ingress buffer lengths.
 */
#define DPAA2_ETH_RX_BUF_SIZE		2048
#define DPAA2_ETH_TX_BUF_ALIGN		64
#define DPAA2_ETH_RX_BUF_ALIGN		256
#define DPAA2_ETH_NEEDED_HEADROOM(p_priv) \
	((p_priv)->tx_data_offset + DPAA2_ETH_TX_BUF_ALIGN)

/* rx_extra_head prevents reallocations in L3 processing. */
#define DPAA2_ETH_SKB_SIZE(p_priv) \
	(DPAA2_ETH_RX_BUF_SIZE + \
	 (p_priv)->rx_extra_head + \
	 SKB_DATA_ALIGN(sizeof(struct skb_shared_info)))

/* Hardware only sees DPAA2_ETH_RX_BUF_SIZE, but we need to allocate ingress
 * buffers large enough to allow building an skb around them and also account
 * for alignment restrictions.
 */
#define DPAA2_ETH_BUF_RAW_SIZE(p_priv) \
	(DPAA2_ETH_SKB_SIZE(p_priv) + \
	DPAA2_ETH_RX_BUF_ALIGN)

/* PTP nominal frequency 1GHz */
#define DPAA2_PTP_NOMINAL_FREQ_PERIOD_NS 1

/* We are accommodating a skb backpointer and some S/G info
 * in the frame's software annotation. The hardware
 * options are either 0 or 64, so we choose the latter.
 */
#define DPAA2_ETH_SWA_SIZE		64

/* Size of hardware annotation area based on the current buffer layout
 * configuration
 */
#define DPAA2_ETH_HWA_SIZE		128

/* Must keep this struct smaller than DPAA2_ETH_SWA_SIZE */
struct dpaa2_eth_swa {
	struct sk_buff *skb;
	struct scatterlist *scl;
	int num_sg;
	int num_dma_bufs;
};

/* Annotation valid bits in FD FRC */
#define DPAA2_FD_FRC_FASV		0x8000
#define DPAA2_FD_FRC_FAEADV		0x4000
#define DPAA2_FD_FRC_FAPRV		0x2000
#define DPAA2_FD_FRC_FAIADV		0x1000
#define DPAA2_FD_FRC_FASWOV		0x0800
#define DPAA2_FD_FRC_FAICFDV		0x0400

/* Error bits in FD CTRL */
#define DPAA2_FD_CTRL_UFD		0x00000004
#define DPAA2_FD_CTRL_SBE		0x00000008
#define DPAA2_FD_CTRL_FSE		0x00000010
#define DPAA2_FD_CTRL_FAERR		0x00000020

#define DPAA2_FD_RX_ERR_MASK	(DPAA2_FD_CTRL_SBE		| \
					DPAA2_FD_CTRL_FAERR)

#define DPAA2_FD_TX_ERR_MASK	(DPAA2_FD_CTRL_UFD		| \
					DPAA2_FD_CTRL_SBE		| \
					DPAA2_FD_CTRL_FSE		| \
					DPAA2_FD_CTRL_FAERR)

/* Annotation bits in FD CTRL */
#define DPAA2_FD_CTRL_ASAL		0x00020000	/* ASAL = 128 */
#define DPAA2_FD_CTRL_PTA		0x00800000
#define DPAA2_FD_CTRL_PTV1		0x00400000

/* Frame annotation status */
struct dpaa2_fas {
	u8 reserved;
	u8 ppid;
	__le16 ifpid;
	__le32 status;
} __packed;

/* Frame annotation egress action descriptor */
#define DPAA2_FAEAD_OFFSET		0x58

struct dpaa2_faead {
	__le32 conf_fqid;
	__le32 ctrl;
};

#define DPAA2_FAEAD_A2V			0x20000000
#define DPAA2_FAEAD_UPDV		0x00001000
#define DPAA2_FAEAD_UPD			0x00000010

/* Error and status bits in the frame annotation status word */
/* Debug frame, otherwise supposed to be discarded */
#define DPAA2_FAS_DISC			0x80000000
/* MACSEC frame */
#define DPAA2_FAS_MS			0x40000000
#define DPAA2_FAS_PTP			0x08000000
/* Ethernet multicast frame */
#define DPAA2_FAS_MC			0x04000000
/* Ethernet broadcast frame */
#define DPAA2_FAS_BC			0x02000000
#define DPAA2_FAS_KSE			0x00040000
#define DPAA2_FAS_EOFHE			0x00020000
#define DPAA2_FAS_MNLE			0x00010000
#define DPAA2_FAS_TIDE			0x00008000
#define DPAA2_FAS_PIEE			0x00004000
/* Frame length error */
#define DPAA2_FAS_FLE			0x00002000
/* Frame physical error */
#define DPAA2_FAS_FPE			0x00001000
#define DPAA2_FAS_PTE			0x00000080
#define DPAA2_FAS_ISP			0x00000040
#define DPAA2_FAS_PHE			0x00000020
#define DPAA2_FAS_BLE			0x00000010
/* L3 csum validation performed */
#define DPAA2_FAS_L3CV			0x00000008
/* L3 csum error */
#define DPAA2_FAS_L3CE			0x00000004
/* L4 csum validation performed */
#define DPAA2_FAS_L4CV			0x00000002
/* L4 csum error */
#define DPAA2_FAS_L4CE			0x00000001
/* Possible errors on the ingress path */
#define DPAA2_FAS_RX_ERR_MASK		(DPAA2_FAS_KSE		| \
					 DPAA2_FAS_EOFHE	| \
					 DPAA2_FAS_MNLE		| \
					 DPAA2_FAS_TIDE		| \
					 DPAA2_FAS_PIEE		| \
					 DPAA2_FAS_FLE		| \
					 DPAA2_FAS_FPE		| \
					 DPAA2_FAS_PTE		| \
					 DPAA2_FAS_ISP		| \
					 DPAA2_FAS_PHE		| \
					 DPAA2_FAS_BLE		| \
					 DPAA2_FAS_L3CE		| \
					 DPAA2_FAS_L4CE)
/* Tx errors */
#define DPAA2_FAS_TX_ERR_MASK	(DPAA2_FAS_KSE		| \
					 DPAA2_FAS_EOFHE	| \
					 DPAA2_FAS_MNLE		| \
					 DPAA2_FAS_TIDE)

/* Time in milliseconds between link state updates */
#define DPAA2_ETH_LINK_STATE_REFRESH	1000

/* Driver statistics, other than those in struct rtnl_link_stats64.
 * These are usually collected per-CPU and aggregated by ethtool.
 */
struct dpaa2_eth_drv_stats {
	__u64	tx_conf_frames;
	__u64	tx_conf_bytes;
	__u64	tx_sg_frames;
	__u64	tx_sg_bytes;
	__u64	rx_sg_frames;
	__u64	rx_sg_bytes;
	/* Enqueues retried due to portal busy */
	__u64	tx_portal_busy;
};

/* Per-FQ statistics */
struct dpaa2_eth_fq_stats {
	/* Number of frames received on this queue */
	__u64 frames;
};

/* Per-channel statistics */
struct dpaa2_eth_ch_stats {
	/* Volatile dequeues retried due to portal busy */
	__u64 dequeue_portal_busy;
	/* Number of CDANs; useful to estimate avg NAPI len */
	__u64 cdan;
	/* Number of frames received on queues from this channel */
	__u64 frames;
	/* Pull errors */
	__u64 pull_err;
};

/* Maximum number of queues associated with a DPNI */
#define DPAA2_ETH_MAX_RX_QUEUES		16
#define DPAA2_ETH_MAX_TX_QUEUES		NR_CPUS
#define DPAA2_ETH_MAX_RX_ERR_QUEUES	1
#define DPAA2_ETH_MAX_QUEUES		(DPAA2_ETH_MAX_RX_QUEUES + \
					DPAA2_ETH_MAX_TX_QUEUES + \
					DPAA2_ETH_MAX_RX_ERR_QUEUES)

#define DPAA2_ETH_MAX_DPCONS		NR_CPUS

#if FSLU_NVME_INIC
struct circ_buf_desc {
    __le32 len_offset_flag;
    __le32 addr_lower;
    __le32 addr_higher;
    __le32 fd_err_frc;
};
#if FSLU_NVME_INIC_QDMA
struct tx_buf_desc {
uint64_t addr;
uint64_t flag;
};
#endif

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
    __le32 tx_desc_base;
    __le16 bpid;
    __le16 obj_id;
//#if FSLU_NVME_INIC_DPAA2
    __le16 pull_rx_buffers;
//#endif
};

struct refill_mem_pool {
    __le32 pool_addr;
    __le32 pool_flag;
};
struct x86mem_flags{
__le32 tx_irq_flag;
__le32 link_state_update;
__le32 link_state;
__le32 padding;
};
#endif

enum dpaa2_eth_fq_type {
	DPAA2_RX_FQ = 0,
	DPAA2_TX_CONF_FQ,
	DPAA2_RX_ERR_FQ
};

struct dpaa2_eth_priv;

struct dpaa2_eth_fq {
	u32 fqid;
	u32 tx_qdbin;
	u16 flowid;
	int target_cpu;
	struct dpaa2_eth_channel *channel;
	enum dpaa2_eth_fq_type type;

	void (*consume)(struct dpaa2_eth_priv *,
			struct dpaa2_eth_channel *,
			const struct dpaa2_fd *,
			struct napi_struct *,
			u16 queue_id);
	struct dpaa2_eth_fq_stats stats;
};

struct dpaa2_eth_channel {
	struct dpaa2_io_notification_ctx nctx;
	struct fsl_mc_device *dpcon;
	int dpcon_id;
	int ch_id;
	int dpio_id;
	struct napi_struct napi;
	struct dpaa2_io_store *store;
	struct dpaa2_eth_priv *priv;
	int buf_count;
	struct dpaa2_eth_ch_stats stats;
};

struct dpaa2_eth_cls_rule {
	struct ethtool_rx_flow_spec fs;
	bool in_use;
};

struct dpaa2_eth_hash_fields {
	u64 rxnfc_field;
	enum net_prot cls_prot;
	int cls_field;
	int offset;
	int size;
};

/* Driver private data */
struct dpaa2_eth_priv {
	struct net_device *net_dev;

	u8 num_fqs;
	/* Tx queues are at the beginning of the array */
	struct dpaa2_eth_fq fq[DPAA2_ETH_MAX_QUEUES];

	u8 num_channels;
	struct dpaa2_eth_channel *channel[DPAA2_ETH_MAX_DPCONS];

	int dpni_id;
	struct dpni_attr dpni_attrs;
	/* Insofar as the MC is concerned, we're using one layout on all 3 types
	 * of buffers (Rx, Tx, Tx-Conf).
	 */
	struct dpni_buffer_layout buf_layout;
	u16 tx_data_offset;
	/* Rx extra headroom space */
	u16 rx_extra_head;

	struct fsl_mc_device *dpbp_dev;
	struct dpbp_attr dpbp_attrs;

	u16 tx_qdid;
	struct fsl_mc_io *mc_io;
	/* SysFS-controlled affinity mask for TxConf FQs */
	struct cpumask txconf_cpumask;
	/* Cores which have an affine DPIO/DPCON.
	 * This is the cpu set on which Rx frames are processed;
	 * Tx confirmation frames are processed on a subset of this,
	 * depending on user settings.
	 */
	struct cpumask dpio_cpumask;

	/* Standard statistics */
	struct rtnl_link_stats64 __percpu *percpu_stats;
	/* Extra stats, in addition to the ones known by the kernel */
	struct dpaa2_eth_drv_stats __percpu *percpu_extras;

	u16 mc_token;

	struct dpni_link_state link_state;
	bool do_link_poll;
	struct task_struct *poll_thread;

	struct dpaa2_eth_hash_fields *hash_fields;
	u8 num_hash_fields;
	/* enabled ethtool hashing bits */
	u64 rx_flow_hash;

#ifdef CONFIG_FSL_DPAA2_ETH_DEBUGFS
	struct dpaa2_debugfs dbg;
#endif

	/* array of classification rules */
	struct dpaa2_eth_cls_rule *cls_rule;

	struct dpni_tx_shaping_cfg shaping_cfg;

	bool ts_tx_en; /* Tx timestamping enabled */
	bool ts_rx_en; /* Rx timestamping enabled */
#if FSLU_NVME_INIC
	spinlock_t tx_desc_lock;
	spinlock_t tx_share_lock;
	spinlock_t rx_desc_lock;
#if FSLU_NVME_INIC_QDMA
	spinlock_t rx_dma_lock;
#endif
	spinlock_t tx_conf_lock;
	struct circ_buf_desc *rx_base;
	struct circ_buf_desc *tx_base;
	struct circ_buf_desc *conf_base;
	struct circ_buf_desc *cur_rx;
	struct circ_buf_desc *cur_tx;
#if FSLU_NVME_INIC_DPAA2
	struct circ_buf_desc *cur_conf;
#endif
#if FSLU_NVME_INIC_QDMA
    struct circ_buf_desc *cur_tx_dma;
#endif
	struct circ_buf_desc *dirty_tx;
	struct buf_pool_desc *bman_buf;
	struct refill_mem_pool *mem_pool_base;
	struct refill_mem_pool *mem_pool_cur;
	struct x86mem_flags  *flags_ptr;
	int tx_free;
	int rx_free;
	int net_state;
	struct napi_struct napi;
	struct work_struct net_start_work;
	struct work_struct net_stop_work;
	void *netregs;
	void *g_outaddr;
	void *g_ccsr;
	void *fpga_reg;
	uint64_t msix_addr;
	uint32_t msix_value;
	unsigned long fpga_irq;
	char tx_name[32];
	int interface_id;
	uint64_t rx_count;
	uint16_t tx_error;
	uint64_t qbman_pool;
	struct task_struct *poll_host;/*inic_thread*/
#if FSLU_NVME_INIC_QDMA
    struct tx_buf_desc *tx_buf[DPAA2_ETH_NUM_TX_BUFS];
    uint64_t tx_buf_count;
    uint64_t tx_buf_ready;
    uint64_t tx_buf_dma;
    uint64_t tx_buf_free;
    uint64_t rx_buf_count;
    uint64_t rx_buf[700];
/*qdma related*/
	struct dma_chan *dma_chan;
	struct dma_device *dma_device;
    volatile int qdma_flag;


#endif
#endif


};

#define dpaa2_eth_hash_enabled(priv)	\
	((priv)->dpni_attrs.num_queues > 1)

#define dpaa2_eth_fs_enabled(priv)	\
	(!((priv)->dpni_attrs.options & DPNI_OPT_NO_FS))

#define dpaa2_eth_fs_mask_enabled(priv)	\
	((priv)->dpni_attrs.options & DPNI_OPT_HAS_KEY_MASKING)

#define dpaa2_eth_fs_count(priv)	\
	((priv)->dpni_attrs.fs_entries)

/* size of DMA memory used to pass configuration to classifier, in bytes */
#define DPAA2_CLASSIFIER_DMA_SIZE 256

extern const struct ethtool_ops dpaa2_ethtool_ops;

static int dpaa2_eth_queue_count(struct dpaa2_eth_priv *priv)
{
	return priv->dpni_attrs.num_queues;
}

static inline int dpaa2_eth_max_channels(struct dpaa2_eth_priv *priv)
{
	/* Ideally, we want a number of channels large enough
	 * to accommodate both the Rx distribution size
	 * and the max number of Tx confirmation queues
	 */
	return dpaa2_eth_queue_count(priv);
}

void check_cls_support(struct dpaa2_eth_priv *priv);

#endif	/* __DPAA2_H */
