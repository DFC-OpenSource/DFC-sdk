#ifndef __NVME_CTRL_H
#define __NVME_CTRL_H

#include <unistd.h>

#include "platform.h"

#include "nvme_bio.h"
#include "nvme_regs.h"
#include "common.h"
#include "dlcl.h"

#include "pcie.h"
#include "bdbm_ftl/host_block.h"
#include "i2c_dimm.h"
#include "ls2_cache.h"
#include "unaligned_cache.h"
#include "qdma.h"

#define NVME_MAX_QS             2047 //PCI_MSIX_FLAGS_QSIZE -TODO
#define NVME_MAX_QUEUE_ENTRIES  0xffff
#define NVME_MAX_STRIDE         12
#define NVME_MAX_NUM_NAMESPACES 256
#define NVME_MAX_QUEUE_ES       0xf
#define NVME_MIN_CQUEUE_ES      0x4
#define NVME_MIN_SQUEUE_ES      0x6
#define NVME_SPARE_THRESHOLD    20
#define NVME_TEMPERATURE        0x143
#define NVME_OP_ABORTED         0xff

#define PCI_VENDOR_ID_INTEL     0x8086
/*#define ADMIN_MSI*/

enum nvme_rw_reqs {
	NVME_REQ_WRITE = 0,
	NVME_REQ_READ = 1,
};

typedef struct NvmeCmd {
	uint8_t     opcode;
	uint8_t     fuse;
	uint16_t    cid;
	uint32_t    nsid;
	uint64_t    res1;
	uint64_t    mptr;
	uint64_t    prp1;
	uint64_t    prp2;
	uint32_t    cdw10;
	uint32_t    cdw11;
	uint32_t    cdw12;
	uint32_t    cdw13;
	uint32_t    cdw14;
	uint32_t    cdw15;
} NvmeCmd;

enum NvmeAdminCommands {
	NVME_ADM_CMD_DELETE_SQ      = 0x00,
	NVME_ADM_CMD_CREATE_SQ      = 0x01,
	NVME_ADM_CMD_GET_LOG_PAGE   = 0x02,
	NVME_ADM_CMD_DELETE_CQ      = 0x04,
	NVME_ADM_CMD_CREATE_CQ      = 0x05,
	NVME_ADM_CMD_IDENTIFY       = 0x06,
	NVME_ADM_CMD_ABORT          = 0x08,
	NVME_ADM_CMD_SET_FEATURES   = 0x09,
	NVME_ADM_CMD_GET_FEATURES   = 0x0a,
	NVME_ADM_CMD_ASYNC_EV_REQ   = 0x0c,
	NVME_ADM_CMD_ACTIVATE_FW    = 0x10,
	NVME_ADM_CMD_DOWNLOAD_FW    = 0x11,
	NVME_ADM_CMD_FORMAT_NVM     = 0x80,
	NVME_ADM_CMD_SECURITY_SEND  = 0x81,
	NVME_ADM_CMD_SECURITY_RECV  = 0x82,
	/*Vendor specific start here*/
};

enum NvmeIoCommands {
	NVME_CMD_FLUSH              = 0x00,
	NVME_CMD_WRITE              = 0x01,
	NVME_CMD_READ               = 0x02,
	NVME_CMD_WRITE_UNCOR        = 0x04,
	NVME_CMD_COMPARE            = 0x05,
	NVME_CMD_WRITE_ZEROS        = 0x08,
	NVME_CMD_DSM                = 0x09,
};

typedef struct NvmeDeleteQ {
	uint8_t     opcode;
	uint8_t     flags;
	uint16_t    cid;
	uint32_t    rsvd1[9];
	uint16_t    qid;
	uint16_t    rsvd10;
	uint32_t    rsvd11[5];
} NvmeDeleteQ;

typedef struct NvmeCreateCq {
	uint8_t     opcode;
	uint8_t     flags;
	uint16_t    cid;
	uint32_t    rsvd1[5];
	uint64_t    prp1;
	uint64_t    rsvd8;
	uint16_t    cqid;
	uint16_t    qsize;
	uint16_t    cq_flags;
	uint16_t    irq_vector;
	uint32_t    rsvd12[4];
} NvmeCreateCq;

#define NVME_CQ_FLAGS_PC(cq_flags)  (cq_flags & 0x1)
#define NVME_CQ_FLAGS_IEN(cq_flags) ((cq_flags >> 1) & 0x1)

typedef struct NvmeCreateSq {
	uint8_t     opcode;
	uint8_t     flags;
	uint16_t    cid;
	uint32_t    rsvd1[5];
	uint64_t    prp1;
	uint64_t    rsvd8;
	uint16_t    sqid;
	uint16_t    qsize;
	uint16_t    sq_flags;
	uint16_t    cqid;
	uint32_t    rsvd12[4];
} NvmeCreateSq;

#define NVME_SQ_FLAGS_PC(sq_flags)      (sq_flags & 0x1)
#define NVME_SQ_FLAGS_QPRIO(sq_flags)   ((sq_flags >> 1) & 0x3)

enum NvmeQFlags {
	NVME_Q_PC           = 1,
	NVME_Q_PRIO_URGENT  = 0,
	NVME_Q_PRIO_HIGH    = 1,
	NVME_Q_PRIO_NORMAL  = 2,
	NVME_Q_PRIO_LOW     = 3,
};

typedef struct NvmeIdentify {
	uint8_t     opcode;
	uint8_t     flags;
	uint16_t    cid;
	uint32_t    nsid;
	uint64_t    rsvd2[2];
	uint64_t    prp1;
	uint64_t    prp2;
	uint32_t    cns;
	uint32_t    rsvd11[5];
} NvmeIdentify;

typedef struct NvmeRwCmd {
	uint8_t     opcode;
	uint8_t     flags;
	uint16_t    cid;
	uint32_t    nsid;
	uint64_t    rsvd2;
	uint64_t    mptr;
	uint64_t    prp1;
	uint64_t    prp2;
	uint64_t    slba;
	uint16_t    nlb;
	uint16_t    control;
	uint32_t    dsmgmt;
	uint32_t    reftag;
	uint16_t    apptag;
	uint16_t    appmask;
} NvmeRwCmd;

enum {
	NVME_RW_LR                  = 1 << 15,
	NVME_RW_FUA                 = 1 << 14,
	NVME_RW_DSM_FREQ_UNSPEC     = 0,
	NVME_RW_DSM_FREQ_TYPICAL    = 1,
	NVME_RW_DSM_FREQ_RARE       = 2,
	NVME_RW_DSM_FREQ_READS      = 3,
	NVME_RW_DSM_FREQ_WRITES     = 4,
	NVME_RW_DSM_FREQ_RW         = 5,
	NVME_RW_DSM_FREQ_ONCE       = 6,
	NVME_RW_DSM_FREQ_PREFETCH   = 7,
	NVME_RW_DSM_FREQ_TEMP       = 8,
	NVME_RW_DSM_LATENCY_NONE    = 0 << 4,
	NVME_RW_DSM_LATENCY_IDLE    = 1 << 4,
	NVME_RW_DSM_LATENCY_NORM    = 2 << 4,
	NVME_RW_DSM_LATENCY_LOW     = 3 << 4,
	NVME_RW_DSM_SEQ_REQ         = 1 << 6,
	NVME_RW_DSM_COMPRESSED      = 1 << 7,
	NVME_RW_PRINFO_PRACT        = 1 << 13,
	NVME_RW_PRINFO_PRCHK_GUARD  = 1 << 12,
	NVME_RW_PRINFO_PRCHK_APP    = 1 << 11,
	NVME_RW_PRINFO_PRCHK_REF    = 1 << 10,
};

typedef struct NvmeDsmCmd {
	uint8_t     opcode;
	uint8_t     flags;
	uint16_t    cid;
	uint32_t    nsid;
	uint64_t    rsvd2[2];
	uint64_t    prp1;
	uint64_t    prp2;
	uint32_t    nr;
	uint32_t    attributes;
	uint32_t    rsvd12[4];
} NvmeDsmCmd;

enum {
	NVME_DSMGMT_IDR = 1 << 0,
	NVME_DSMGMT_IDW = 1 << 1,
	NVME_DSMGMT_AD  = 1 << 2,
};

typedef struct NvmeDsmRange {
	uint32_t    cattr;
	uint32_t    nlb;
	uint64_t    slba;
} NvmeDsmRange;

enum NvmeAsyncEventRequest {
	NVME_AER_TYPE_ERROR                     = 0,
	NVME_AER_TYPE_SMART                     = 1,
	NVME_AER_TYPE_IO_SPECIFIC               = 6,
	NVME_AER_TYPE_VENDOR_SPECIFIC           = 7,
	NVME_AER_INFO_ERR_INVALID_SQ            = 0,
	NVME_AER_INFO_ERR_INVALID_DB            = 1,
	NVME_AER_INFO_ERR_DIAG_FAIL             = 2,
	NVME_AER_INFO_ERR_PERS_INTERNAL_ERR     = 3,
	NVME_AER_INFO_ERR_TRANS_INTERNAL_ERR    = 4,
	NVME_AER_INFO_ERR_FW_IMG_LOAD_ERR       = 5,
	NVME_AER_INFO_SMART_RELIABILITY         = 0,
	NVME_AER_INFO_SMART_TEMP_THRESH         = 1,
	NVME_AER_INFO_SMART_SPARE_THRESH        = 2,
};

typedef struct NvmeAerResult {
	uint8_t event_type;
	uint8_t event_info;
	uint8_t log_page;
	uint8_t resv;
} NvmeAerResult;

typedef struct NvmeCqe {
	uint32_t    result;
	uint32_t    rsvd;
	uint16_t    sq_head;
	uint16_t    sq_id;
	uint16_t    cid;
	uint16_t    status;
} NvmeCqe;

enum NvmeStatusCodes {
	NVME_SUCCESS                = 0x0000,
	NVME_INVALID_OPCODE         = 0x0001,
	NVME_INVALID_FIELD          = 0x0002,
	NVME_CID_CONFLICT           = 0x0003,
	NVME_DATA_TRAS_ERROR        = 0x0004,
	NVME_POWER_LOSS_ABORT       = 0x0005,
	NVME_INTERNAL_DEV_ERROR     = 0x0006,
	NVME_CMD_ABORT_REQ          = 0x0007,
	NVME_CMD_ABORT_SQ_DEL       = 0x0008,
	NVME_CMD_ABORT_FAILED_FUSE  = 0x0009,
	NVME_CMD_ABORT_MISSING_FUSE = 0x000a,
	NVME_INVALID_NSID           = 0x000b,
	NVME_CMD_SEQ_ERROR          = 0x000c,
	NVME_LBA_RANGE              = 0x0080,
	NVME_CAP_EXCEEDED           = 0x0081,
	NVME_NS_NOT_READY           = 0x0082,
	NVME_NS_RESV_CONFLICT       = 0x0083,
	NVME_INVALID_CQID           = 0x0100,
	NVME_INVALID_QID            = 0x0101,
	NVME_MAX_QSIZE_EXCEEDED     = 0x0102,
	NVME_ACL_EXCEEDED           = 0x0103,
	NVME_RESERVED               = 0x0104,
	NVME_AER_LIMIT_EXCEEDED     = 0x0105,
	NVME_INVALID_FW_SLOT        = 0x0106,
	NVME_INVALID_FW_IMAGE       = 0x0107,
	NVME_INVALID_IRQ_VECTOR     = 0x0108,
	NVME_INVALID_LOG_ID         = 0x0109,
	NVME_INVALID_FORMAT         = 0x010a,
	NVME_FW_REQ_RESET           = 0x010b,
	NVME_INVALID_QUEUE_DEL      = 0x010c,
	NVME_FID_NOT_SAVEABLE       = 0x010d,
	NVME_FID_NOT_NSID_SPEC      = 0x010f,
	NVME_FW_REQ_SUSYSTEM_RESET  = 0x0110,
	NVME_CONFLICTING_ATTRS      = 0x0180,
	NVME_INVALID_PROT_INFO      = 0x0181,
	NVME_WRITE_TO_RO            = 0x0182,
	NVME_INVALID_MEMORY_ADDRESS = 0x01C0,  /* Vendor extension. */
	NVME_WRITE_FAULT            = 0x0280,
	NVME_UNRECOVERED_READ       = 0x0281,
	NVME_E2E_GUARD_ERROR        = 0x0282,
	NVME_E2E_APP_ERROR          = 0x0283,
	NVME_E2E_REF_ERROR          = 0x0284,
	NVME_CMP_FAILURE            = 0x0285,
	NVME_ACCESS_DENIED          = 0x0286,
	NVME_MORE                   = 0x2000,
	NVME_DNR                    = 0x4000,
	NVME_NO_COMPLETE            = 0xffff,
};

typedef struct NvmeFwSlotInfoLog {
	uint8_t     afi;
	uint8_t     reserved1[7];
	uint8_t     frs1[8];
	uint8_t     frs2[8];
	uint8_t     frs3[8];
	uint8_t     frs4[8];
	uint8_t     frs5[8];
	uint8_t     frs6[8];
	uint8_t     frs7[8];
	uint8_t     reserved2[448];
} NvmeFwSlotInfoLog;

typedef struct NvmeErrorLog {
	uint64_t    error_count;
	uint16_t    sqid;
	uint16_t    cid;
	uint16_t    status_field;
	uint16_t    param_error_location;
	uint64_t    lba;
	uint32_t    nsid;
	uint8_t     vs;
	uint8_t     resv[35];
} NvmeErrorLog;

typedef struct NvmeSmartLog {
	uint8_t     critical_warning;
	uint8_t     temperature[2];
	uint8_t     available_spare;
	uint8_t     available_spare_threshold;
	uint8_t     percentage_used;
	uint8_t     reserved1[26];
	uint64_t    data_units_read[2];
	uint64_t    data_units_written[2];
	uint64_t    host_read_commands[2];
	uint64_t    host_write_commands[2];
	uint64_t    controller_busy_time[2];
	uint64_t    power_cycles[2];
	uint64_t    power_on_hours[2];
	uint64_t    unsafe_shutdowns[2];
	uint64_t    media_errors[2];
	uint64_t    number_of_error_log_entries[2];
	uint8_t     reserved2[320];
} NvmeSmartLog;

enum NvmeSmartWarn {
	NVME_SMART_SPARE                  = 1 << 0,
	NVME_SMART_TEMPERATURE            = 1 << 1,
	NVME_SMART_RELIABILITY            = 1 << 2,
	NVME_SMART_MEDIA_READ_ONLY        = 1 << 3,
	NVME_SMART_FAILED_VOLATILE_MEDIA  = 1 << 4,
};

enum LogIdentifier {
	NVME_LOG_ERROR_INFO     = 0x01,
	NVME_LOG_SMART_INFO     = 0x02,
	NVME_LOG_FW_SLOT_INFO   = 0x03,
};

typedef struct NvmePSD {
	uint16_t    mp;
	uint16_t    reserved;
	uint32_t    enlat;
	uint32_t    exlat;
	uint8_t     rrt;
	uint8_t     rrl;
	uint8_t     rwt;
	uint8_t     rwl;
	uint8_t     resv[16];
} NvmePSD;

typedef struct NvmeIdCtrl {
	uint16_t    vid;
	uint16_t    ssvid;
	uint8_t     sn[20];
	uint8_t     mn[40];
	uint8_t     fr[8];
	uint8_t     rab;
	uint8_t     ieee[3];
	uint8_t     cmic;
	uint8_t     mdts;
	uint8_t     rsvd255[178];
	uint16_t    oacs;
	uint8_t     acl;
	uint8_t     aerl;
	uint8_t     frmw;
	uint8_t     lpa;
	uint8_t     elpe;
	uint8_t     npss;
	uint8_t     rsvd511[248];
	uint8_t     sqes;
	uint8_t     cqes;
	uint16_t    rsvd515;
	uint32_t    nn;
	uint16_t    oncs;
	uint16_t    fuses;
	uint8_t     fna;
	uint8_t     vwc;
	uint16_t    awun;
	uint16_t    awupf;
	uint8_t     rsvd703[174];
	uint8_t     rsvd2047[1344];
	NvmePSD     psd[32];
	uint8_t     vs[1024];
} NvmeIdCtrl;

enum NvmeIdCtrlOacs {
	NVME_OACS_SECURITY  = 1 << 0,
	NVME_OACS_FORMAT    = 1 << 1,
	NVME_OACS_FW        = 1 << 2,
};

enum NvmeIdCtrlOncs {
	NVME_ONCS_COMPARE       = 1 << 0,
	NVME_ONCS_WRITE_UNCORR  = 1 << 1,
	NVME_ONCS_DSM           = 1 << 2,
	NVME_ONCS_WRITE_ZEROS   = 1 << 3,
	NVME_ONCS_FEATURES      = 1 << 4,
	NVME_ONCS_RESRVATIONS   = 1 << 5,
};

#define NVME_CTRL_SQES_MIN(sqes) ((sqes) & 0xf)
#define NVME_CTRL_SQES_MAX(sqes) (((sqes) >> 4) & 0xf)
#define NVME_CTRL_CQES_MIN(cqes) ((cqes) & 0xf)
#define NVME_CTRL_CQES_MAX(cqes) (((cqes) >> 4) & 0xf)

typedef struct NvmeFeatureVal {
	uint32_t    arbitration;
	uint32_t    power_mgmt;
	uint32_t    temp_thresh;
	uint32_t    err_rec;
	uint32_t    volatile_wc;
	uint32_t    num_queues;
	uint32_t    int_coalescing;
	uint32_t    *int_vector_config;
	uint32_t    write_atomicity;
	uint32_t    async_config;
	uint32_t    sw_prog_marker;
} NvmeFeatureVal;

#define NVME_ARB_AB(arb)    (arb & 0x7)
#define NVME_ARB_LPW(arb)   ((arb >> 8) & 0xff)
#define NVME_ARB_MPW(arb)   ((arb >> 16) & 0xff)
#define NVME_ARB_HPW(arb)   ((arb >> 24) & 0xff)

#define NVME_INTC_THR(intc)     (intc & 0xff)
#define NVME_INTC_TIME(intc)    ((intc >> 8) & 0xff)

enum NvmeFeatureIds {
	NVME_ARBITRATION                = 0x1,
	NVME_POWER_MANAGEMENT           = 0x2,
	NVME_LBA_RANGE_TYPE             = 0x3,
	NVME_TEMPERATURE_THRESHOLD      = 0x4,
	NVME_ERROR_RECOVERY             = 0x5,
	NVME_VOLATILE_WRITE_CACHE       = 0x6,
	NVME_NUMBER_OF_QUEUES           = 0x7,
	NVME_INTERRUPT_COALESCING       = 0x8,
	NVME_INTERRUPT_VECTOR_CONF      = 0x9,
	NVME_WRITE_ATOMICITY            = 0xa,
	NVME_ASYNCHRONOUS_EVENT_CONF    = 0xb,
	NVME_SOFTWARE_PROGRESS_MARKER   = 0x80
};

typedef struct NvmeRangeType {
	uint8_t     type;
	uint8_t     attributes;
	uint8_t     rsvd2[14];
	uint64_t    slba;
	uint64_t    nlb;
	uint8_t     guid[16];
	uint8_t     rsvd48[16];
} NvmeRangeType;

typedef struct NvmeLBAF {
	uint16_t    ms;
	uint8_t     ds;
	uint8_t     rp;
} NvmeLBAF;

typedef struct NvmeIdNs {
	uint64_t    nsze;
	uint64_t    ncap;
	uint64_t    nuse;
	uint8_t     nsfeat;
	uint8_t     nlbaf;
	uint8_t     flbas;
	uint8_t     mc;
	uint8_t     dpc;
	uint8_t     dps;
	uint8_t     res30[98];
	NvmeLBAF    lbaf[16];
	uint8_t     res192[192];
	uint8_t     vs[3712];
} NvmeIdNs;

#define NVME_ID_NS_NSFEAT_THIN(nsfeat)      ((nsfeat & 0x1))
#define NVME_ID_NS_FLBAS_EXTENDED(flbas)    ((flbas >> 4) & 0x1)
#define NVME_ID_NS_FLBAS_INDEX(flbas)       ((flbas & 0xf))
#define NVME_ID_NS_MC_SEPARATE(mc)          ((mc >> 1) & 0x1)
#define NVME_ID_NS_MC_EXTENDED(mc)          ((mc & 0x1))
#define NVME_ID_NS_DPC_LAST_EIGHT(dpc)      ((dpc >> 4) & 0x1)
#define NVME_ID_NS_DPC_FIRST_EIGHT(dpc)     ((dpc >> 3) & 0x1)
#define NVME_ID_NS_DPC_TYPE_3(dpc)          ((dpc >> 2) & 0x1)
#define NVME_ID_NS_DPC_TYPE_2(dpc)          ((dpc >> 1) & 0x1)
#define NVME_ID_NS_DPC_TYPE_1(dpc)          ((dpc & 0x1))
#define NVME_ID_NS_DPC_TYPE_MASK            0x7

enum NvmeIdNsDps {
	DPS_TYPE_NONE   = 0,
	DPS_TYPE_1      = 1,
	DPS_TYPE_2      = 2,
	DPS_TYPE_3      = 3,
	DPS_TYPE_MASK   = 0x7,
	DPS_FIRST_EIGHT = 8,
};

typedef struct NvmeAsyncEvent {
	dlcl_list_t   entry;
	NvmeAerResult result;
} NvmeAsyncEvent;

typedef void  (*BlockAIOCB)(void);

typedef struct NvmeRequest {
	dlcl_list_t entry;
	struct NvmeSQ           *sq;
	struct NvmeNamespace    *ns;
	BlockAIOCB              *aiocb;
	uint16_t                status;
	uint64_t                slba;
	uint16_t                is_write;
	uint16_t                nlb;
	uint16_t                ctrl;
	uint64_t                meta_size;
	uint64_t                mptr;
	void                    *meta_buf;
	NvmeCqe                 cqe;
	uint8_t					lba_index;
	// 	BlockAcctCookie         acct;
	nvme_bio                bio;
	struct NvmeRequest      *next;
	struct NvmeRequest      *prev;
} NvmeRequest;

typedef struct NvmeSQ {
	dlcl_list_t entry;
	struct NvmeCtrl *ctrl;
	uint8_t     phys_contig;
	uint8_t     arb_burst;
	uint16_t    sqid;
	uint16_t    cqid;
	uint32_t    head;
	uint32_t    tail;
	uint32_t    size;
	uint64_t    dma_addr;
	uint64_t    completed;
	uint64_t    *prp_list;
	NvmeRequest *io_req;
	NvmeRequest *req_list;
	NvmeRequest *out_req_list;
	/* Mapped memory location where the tail pointer is stored by the guest
	 * without triggering MMIO exits. */
	uint64_t    db_addr;
	/* virtio-like eventidx pointer, guest updates to the tail pointer that
	 * do not go over this value will not result in MMIO writes (but will
	 * still write the tail pointer to the "db_addr" location above). */
	uint64_t    eventidx_addr;
	int         fd_qmem;
	enum NvmeQFlags  prio;
	uint64_t    posted;
} NvmeSQ;

typedef struct NvmeCQ {
	struct NvmeCtrl *ctrl;
	uint8_t     phys_contig;
	uint8_t     phase;
	uint16_t    cqid;
	uint16_t    irq_enabled;
	uint32_t    head;
	uint32_t    tail;
	uint32_t    vector;
	uint32_t    size;
	uint64_t    dma_addr;
	uint64_t    *prp_list;
	NvmeTimer   *timer;
	NvmeSQ  *sq_list;
	NvmeRequest *req_list;
	/* Mapped memory location where the head pointer is stored by the guest
	 * without triggering MMIO exits. */
	uint64_t    db_addr;
	/* virtio-like eventidx pointer, guest updates to the head pointer that
	 * do not go over this value will not result in MMIO writes (but will
	 * still write the head pointer to the "db_addr" location above). */
	uint64_t    eventidx_addr;
	int         fd_qmem;
	volatile uint8_t hold_sqs;
} NvmeCQ;

typedef struct NvmeNamespace {
	struct NvmeCtrl *ctrl;
	NvmeIdNs        id_ns;
	NvmeRangeType   lba_range[64];
	unsigned long   *util;
	unsigned long   *uncorrectable;
	uint32_t        id;
	uint64_t        start_block;
	uint64_t        meta_start_offset;
} NvmeNamespace;
#if 0
/*Is it here? -TODO*/
enum BlockAcctType {
	BLOCK_ACCT_READ,
	BLOCK_ACCT_WRITE,
	BLOCK_ACCT_FLUSH,
	BLOCK_MAX_IOTYPE,
};

typedef struct BlockAcctStats {
	uint64_t nr_bytes[BLOCK_MAX_IOTYPE];
	uint64_t nr_ops[BLOCK_MAX_IOTYPE];
	uint64_t total_time_ns[BLOCK_MAX_IOTYPE];
	uint64_t wr_highest_sector;
} BlockAcctStats;
#endif
typedef struct BlockAcctStats {
	uint64_t    nr_bytes_read[2];
	uint64_t    nr_bytes_written[2];
	uint64_t    nr_read_ops[2];
	uint64_t    nr_write_ops[2];
	uint64_t    total_time_ns;
	uint64_t    wr_highest_sector;
	uint8_t     num_active_queues;
	uint64_t    tot_num_AdminCmd;
	uint64_t    tot_num_IOCmd;
	uint64_t    tot_num_ReadCmd;
	uint64_t    tot_num_WriteCmd;
} BlockAcctStats;


#define blk_get_stats(x) NULL
#define NVME_NUM_QUEUES  64

#define BDRV_SECTOR_BITS 9
/*1 => system v msg Qs; 0 => posix msg Qs*/
#define USING_MSGQ_FROM_SYSV 0

#define NVME_MQ_MSGSIZE  8 /*Messages are integers indicating SQIDs*/
#define NVME_MQ_MAXMSG   NVME_NUM_QUEUES
#define NVME_LS2_DDR_MQ_NAME     "/nvme_ls2_ddr_mq"
#define NVME_FPGA_DDR_MQ_NAME     "/nvme_fpga_ddr_mq"
#define NVME_NAND_MQ_NAME     "/nvme_nand_mq"
#define NVME_UNALIGN_MQ_NAME     "/nvme_unalign_mq"
#define NVME_TRIM_MQ_NAME     "/nvme_trim_mq"
#define NVME_QDMA_MQ_NAME     "/nvme_qdma_mq"

#define SHADOW_REG_SZ 64
#define NVME_MAX_PRIORITY 4

typedef struct NvmeQSched {
#if 0
	struct timespec time;
	int          mq_txid;
	int          mq_rxid;
#endif
	uint16_t     n_active_iosqs;
	uint64_t     shadow_regs[NVME_MAX_PRIORITY][2];
	uint64_t     mask_regs[NVME_MAX_PRIORITY][2];
	uint32_t     *iodbst_reg;
	uint32_t     SQID[NVME_MAX_PRIORITY];
	unsigned int prio_avail[NVME_MAX_PRIORITY];
	unsigned int prio_lvl_next_q[NVME_MAX_PRIORITY];
	uint32_t     round_robin_status_regs[4];
	int	     WRR;
} NvmeQSched;

typedef struct NvmeCtrl {
	int fpga_version;
	uint8_t		running;
	int		fd_uio[2];
	FpgaCtrl	fpga;
	DmaCtrl		dma;
	HostCtrl	host;
	FpgaIrqs	in_irqs;
	NvmeBioTarget	target;
	MemoryRegion	iomem;
	MemoryRegion	ctrl_mem;
	NvmeRegs    	nvme_regs;
	NvmeQSched  	qsched;
	// 	BlockConf	conf;
	time_t		start_time;
	uint16_t	temperature;
	uint16_t    page_size;
	uint16_t    page_bits;
	uint16_t    max_prp_ents;
	uint16_t    cqe_size;
	uint16_t    sqe_size;
	uint16_t    oacs;
	uint16_t    oncs;
	uint32_t    num_namespaces;
	uint32_t    num_queues;
	uint32_t    max_q_ents;
	uint64_t    ns_size[3];
	uint8_t     ns_type[3];
	uint8_t     db_stride;
	uint8_t     aerl;
	uint8_t     acl;
	uint8_t     elpe;
	uint8_t     elp_index;
	//	uint8_t     error_count;
	uint8_t     mdts;
	uint8_t     cqr;
	uint8_t     max_sqes;
	uint8_t     max_cqes;
	uint8_t     meta;
	uint8_t     vwc;
	uint8_t     mc;
	uint8_t     dpc;
	uint8_t     dps;
	uint8_t     nlbaf;
	uint8_t     extended;
	uint8_t     lba_index;
	uint8_t     mpsmin;
	uint8_t     mpsmax;
	uint8_t     intc;
	uint8_t     intc_thresh;
	uint8_t     intc_time;
	uint8_t     outstanding_aers;
	uint8_t     temp_warn_issued;
	uint8_t     num_errors;
	uint8_t     cqes_pending;
	uint16_t    vid;
	uint16_t    did;
	uint16_t    cmb;
	uint8_t     *cmbuf;
	nvme_drv_params *drv_params;
	struct timespec time;
	int         ls2_ddr_mq_txid;
	int         ls2_ddr_mq_rxid;
	int         fpga_ddr_mq_txid;
	int         fpga_ddr_mq_rxid;
	int         nand_mq_txid;
	int         nand_mq_rxid;
	int         unalign_mq_txid;
	int         trim_mq_txid;

	char            *serial;
	NvmeErrorLog    *elpes;
	NvmeRequest     **aer_reqs;
	NvmeNamespace   *namespaces;
	NvmeSQ          **sq;
	NvmeCQ          **cq;
	NvmeSQ          admin_sq;
	NvmeCQ          admin_cq;
	NvmeFeatureVal  features;
	NvmeIdCtrl      id_ctrl;
	NvmeAsyncEvent  *aer_queue;
	NvmeTimer       *aer_timer;
	uint8_t		aer_mask;
	pthread_mutex_t	req_mutex;
	pthread_t	io_completer_tid_ddr;
	pthread_t	io_completer_tid_nand;
	pthread_t	io_processor_tid;
	pthread_t	unalign_bio_tid;
	pthread_t	qdma_completer_tid;
	pthread_t	qdma_handler_tid;
	pthread_spinlock_t qs_req_spin;
	pthread_spinlock_t aer_req_spin;
	BlockAcctStats	stat;
	uint64_t	namespace_size[3];
	uint64_t	ns_check;
	uint8_t 	pex_count;
	uint8_t 	nbe_flag;
	int			dimm_module_type;
	dimm_details	**dm;
	LS2_Cache	ls2_cache;
	nvme_cache	cache;
	QdmaCtrl	qdmactrl;
} NvmeCtrl;

typedef struct NvmeDifTuple {
	uint16_t guard_tag;
	uint16_t app_tag;
	uint32_t ref_tag;
} NvmeDifTuple;

#ifndef container_of
#define container_of(ptr, type, member) ( {                      \
		const typeof(((type *) 0)->member) *__mptr = (ptr);     \
		(type *) ((char *) __mptr - offsetof(type, member));})
#endif

#define THREAD_CANCEL(id) { \
	pthread_cancel (id); \
	pthread_join (id, NULL); }

typedef struct fifo_data {
	uint64_t new_val;
	uint64_t offset;
} fifo_data;

typedef struct BioMsg_Buf {
	long type;
	uint64_t req_addr;
} BioMsg_Buf;

pthread_t thread_fifoId;
void fifo_read_poll(char *fifo_mem); /* Poll FIFO Count register to  */
void nvmeRegInit(NvmeRegs *nvmebar);

static inline void reset_iosqdb_bits (FpgaCtrl *fpga)
{
	/*Clear IOSQDB_STATUS REGISTER*/
	int i = 0;

	for(;i<4;i++) memset((void *)(fpga->iosqdb_bits + (i * 0x01)), 0, sizeof(uint32_t));
}

static inline void reset_fifo (FpgaCtrl *fpga)
{
	/*Clear FIFO*/
	*(fpga->fifo_reset) = 0x0;
	*(fpga->fifo_reset) = 0x1;
	*(fpga->fifo_reset) = 0x0;
}

static inline void reset_nvmeregs (FpgaCtrl *fpga)
{
	/*Clear NVME registers*/
	*(fpga->fifo_reset) = 0x0;
	*(fpga->fifo_reset) = 0x2;
	*(fpga->fifo_reset) = 0x0;
}

static inline int mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	return pthread_mutex_init(mutex, mutexattr);
}

static inline int mutex_destroy(pthread_mutex_t *mutex)
{
	return pthread_mutex_destroy(mutex);
}

static inline int mutex_lock(pthread_mutex_t *mutex)
{
	return pthread_mutex_lock(mutex);
}

static inline int mutex_trylock(pthread_mutex_t *mutex)
{
	return pthread_mutex_trylock(mutex);
}

static inline int mutex_unlock(pthread_mutex_t *mutex)
{
	return pthread_mutex_unlock(mutex);
}

static inline int spin_init(pthread_spinlock_t *req_spin, int pshared)
{
    return pthread_spin_init(req_spin,pshared);
}

static inline int spin_destroy(pthread_spinlock_t *req_spin)
{
    return pthread_spin_destroy(req_spin);
}

static inline int spin_lock(pthread_spinlock_t *req_spin)
{
	return pthread_spin_lock(req_spin);
}

static inline int spin_unlock(pthread_spinlock_t *req_spin)
{
	return pthread_spin_unlock(req_spin);
}

extern int init_cli();
extern int get_dimm_info(NvmeCtrl *n);
extern int get_new_dimm_info(NvmeCtrl *n);
extern int ns_check_valid(NvmeCtrl *n,int slot[]);
int nvme_bio_init (NvmeCtrl *n);
inline void nvme_bio_deinit (NvmeCtrl *n);
void nvme_rw_cb (void *opaque, int ret);
void *io_processor (void *arg);

#endif	/*__NVME_CTRL_H*/

