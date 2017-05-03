/* 
 * LS2085 SDK required MACRO's and structures
 */

#ifndef __DPAA2_ETH_H
#define __DPAA2_ETH_H
#include "config.h"
/* IMMR Accessor Helpers */
#define IMMR_R32(_off) ioread32(priv->immr+(_off))
#define IMMR_W32(_off, _val) iowrite32((_val), priv->immr+(_off))
#define IMMR_R32BE(_off) ioread32be(priv->immr+(_off))
#define IMMR_W32BE(_off, _val) iowrite32be((_val), priv->immr+(_off))

#define DPAA2_ETH_RX_BUFFER_SIZE       9216 
#define DPAA2_ETH_SWA_SIZE              64

#define SKB_FREE_FLAG_REFRESH   1000

/* Hardware requires alignment for ingress/egress buffer addresses
 * and ingress buffer lengths.
 */
#define DPAA2_ETH_RX_BUF_ALIGN             256
#define DPAA2_ETH_TX_BUF_ALIGN             64
#define DPAA2_ETH_NEEDED_HEADROOM(X)  (X + DPAA2_ETH_TX_BUF_ALIGN)
#define DPAA2_ETH_BUF_RAW_SIZE \
        (DPAA2_ETH_RX_BUFFER_SIZE + \
        SKB_DATA_ALIGN(sizeof(struct skb_shared_info)) + \
        DPAA2_ETH_RX_BUF_ALIGN)


/* BUFF POOL */
#define BUFF_POOL_SIZE 16800
//#define BUFF_POOL_SIZE 21000

/* Buff Pool Refill */
#define BUFF_POOL_REFILL_NOT_REQ 0x01
#define BUFF_POOL_REFILL_REQ     0x02
#define DPAA2_ETH_REFILL_THRESH  7000 

/* MSI */
#define MSI_ENABLE 0x01
#define MSI_DISABLE 0x02

/*LS2 Interrupt*/
#define DMA_ENABLE 0x01
#define DMA_DISABLE 0x02

/*ERROR CHECKING */
#define DPAA2_ETH_FAS_KSE               0x00040000
#define DPAA2_ETH_FAS_EOFHE             0x00020000
#define DPAA2_ETH_FAS_MNLE              0x00010000
#define DPAA2_ETH_FAS_TIDE              0x00008000
#define DPAA2_ETH_FAS_PIEE              0x00004000
#define DPAA2_ETH_FAS_L3CV              0x00000008

/* L3 csum error */
#define DPAA2_ETH_FAS_L3CE              0x00000004
#define DPAA2_ETH_FAS_L4CV              0x00000002

/* Debug frame, otherwise supposed to be discarded */
#define DPAA2_ETH_FAS_DISC              0x80000000

/* MACSEC frame */
#define DPAA2_ETH_FAS_MS                0x40000000
#define DPAA2_ETH_FAS_PTP               0x08000000

/* Ethernet multicast frame */
#define DPAA2_ETH_FAS_MC                0x04000000

/* Ethernet broadcast frame */
#define DPAA2_ETH_FAS_BC                0x02000000
#define DPAA2_ETH_FAS_KSE               0x00040000
#define DPAA2_ETH_FAS_EOFHE             0x00020000
#define DPAA2_ETH_FAS_MNLE              0x00010000
#define DPAA2_ETH_FAS_TIDE              0x00008000
#define DPAA2_ETH_FAS_PIEE              0x00004000

/* Frame length error */
#define DPAA2_ETH_FAS_FLE               0x00002000

/* Frame physical error; our favourite pastime */
#define DPAA2_ETH_FAS_FPE               0x00001000
#define DPAA2_ETH_FAS_PTE               0x00000080
#define DPAA2_ETH_FAS_ISP               0x00000040
#define DPAA2_ETH_FAS_PHE               0x00000020
#define DPAA2_ETH_FAS_BLE               0x00000010

/* These bits always signal errors */
#define DPAA2_ETH_RX_ERR_MASK		(DPAA2_ETH_FAS_KSE	| \
					 DPAA2_ETH_FAS_EOFHE	| \
					 DPAA2_ETH_FAS_MNLE	| \
					 DPAA2_ETH_FAS_TIDE	| \
					 DPAA2_ETH_FAS_PIEE	| \
					 DPAA2_ETH_FAS_FLE	| \
					 DPAA2_ETH_FAS_FPE	| \
					 DPAA2_ETH_FAS_PTE	| \
					 DPAA2_ETH_FAS_ISP	| \
					 DPAA2_ETH_FAS_PHE	| \
					 DPAA2_ETH_FAS_BLE	| \
					 DPAA2_ETH_FAS_L3CE	| \
					 DPAA2_ETH_FAS_L4CE)

/* Unsupported features in the ingress */
#define DPAA2_ETH_RX_UNSUPP_MASK        DPAA2_ETH_FAS_MS
#define DPAA2_ETH_FAS_MS                0x40000000
#define DPAA2_ETH_FAS_L4CE              0x00000001

/* Annotation valid bits in FD FRC */
#define DPAA2_FD_FRC_FASV               0x8000
#define DPAA2_FD_FRC_FAEADV             0x4000
#define DPAA2_FD_FRC_FAPRV              0x2000
#define DPAA2_FD_FRC_FAIADV             0x1000
#define DPAA2_FD_FRC_FASWOV             0x0800
#define DPAA2_FD_FRC_FAICFDV            0x0400

/* Annotation bits in FD CTRL */
#define DPAA2_FD_CTRL_ASAL              0x00020000      /* ASAL = 128 */
#define DPAA2_FD_CTRL_PTA               0x00800000
#define DPAA2_FD_CTRL_PTV1              0x00400000

/* TODO trim down the bitmask; not all of them apply to Tx-confirm */
#define DPAA2_ETH_TXCONF_ERR_MASK       (DPAA2_ETH_FAS_KSE      | \
        DPAA2_ETH_FAS_EOFHE    | \
        DPAA2_ETH_FAS_MNLE     | \
        DPAA2_ETH_FAS_TIDE)

/* So far we're only accomodating a skb backpointer in the frame's
 * software annotation, but the hardware options are either 0 or 64.
 */
#define DPAA2_ETH_SWA_SIZE              64

/*Interface UP */
#define IFACE_IDLE           0x7
#define IFACE_UP             0x8
#define IFACE_READY          0x9
#define IFACE_DOWN           0xa
#define IFACE_STOP           0xb

/* MAC Addr settings */
#define MAC_NOTSET 0x00
#define MAC_SET    0x02

/* Frame annotation status */
struct dpaa2_fas {
    u8 reserved; 
    u8 ppid;
    __le16 ifpid;
    __le32 status;
} __packed;

#endif /* LS2_ETH_H */
