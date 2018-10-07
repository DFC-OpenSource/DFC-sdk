
#ifndef __NVME_REGS_H
#define __NVME_REGS_H

#include <stdint.h>

typedef struct vs_reg {
	uint8_t rsvd;
	uint8_t mnr;
	uint16_t mjr;
} __attribute__((packed)) vs_t;

typedef struct cc_reg {
	uint8_t en:1;
	uint8_t rsvd1:3;
	uint8_t iocss:3;
	uint8_t mps:4;
	uint8_t ams:3;
	uint8_t shn:2;
	uint8_t iosqes:4;
	uint8_t iocqes:4;
	uint8_t rsvd2:8;
} __attribute__((packed)) cconf_t;

typedef struct csts_reg {
	uint8_t rdy:1;
	uint8_t cfs:1;
	uint8_t shst:2;
	uint8_t nssro:1;
	uint8_t pp:1;
	uint32_t rsvd:26;
}__attribute__((packed)) csts_t;

typedef struct aqa_t {
	uint16_t asqs:12;
	uint8_t rsvd1:4;
	uint16_t acqs:12;
	uint8_t rsvd2:4;
}__attribute__((packed)) aqa_t;

typedef struct asq_reg {
	uint16_t rsvd:12;
	uint64_t asqb:52;
}__attribute__((packed)) asq_t;

typedef struct acq_reg {
	uint16_t rsvd:12;
	uint64_t acqb:52;
}__attribute__((packed)) acq_t;

typedef struct intms_reg {
	uint32_t ivms;
}__attribute__((packed)) intms_t;

typedef struct intmc_reg {
	uint32_t ivmc;
}__attribute__((packed)) intmc_t;

typedef struct nssr_reg {
	uint32_t nssrc;
}__attribute__((packed)) nssr_t;

typedef struct cap_reg {
	uint16_t    mqes:16;
	uint8_t     cqr:1;
	uint8_t     ams:2;
	uint8_t     rsvd1:5;
	uint8_t     to:8;
	uint8_t     dstrd:4;
	uint8_t     nssrs:1;
	uint8_t     Css:8;
	uint8_t     rsvd2:3;
	uint8_t     mpsmin:4;
	uint8_t     mpsmax:4;
	uint8_t     rsvd3:8;
} __attribute__((packed)) cap_t ;

typedef struct cmbloc_reg {
	uint8_t     bir:3;
	uint16_t	rsvd:9;
	uint32_t    ofst:20;
}__attribute__((packed)) cmbloc_t;

typedef struct cmbsz {
	uint8_t     sqs:1;
	uint8_t     cqs:1;
	uint8_t     lists:1;
	uint8_t     rds:1;
	uint8_t     wds:1;
	uint8_t		rsvd:3;
	uint8_t     szu:4;
	uint32_t    sz:20;
}__attribute__((packed)) cmbsz_t;

typedef struct NvmeRegs_bits {
	cap_t       cap;
	vs_t        vs;
	intms_t     intms;
	intmc_t     intmc;
	cconf_t     cc;
	uint32_t    rsvd1;
	csts_t      csts;
	nssr_t      nssrc;
	aqa_t       aqa;
	asq_t       asq;
	acq_t       acq;
	cmbloc_t    cmbloc;
	cmbsz_t     cmbsz;
} NvmeRegs_bits;

typedef struct NvmeRegs_vals {
	uint64_t    cap;
	uint32_t    vs;
	uint32_t    intms;
	uint32_t    intmc;
	uint32_t    cc;
	uint32_t    rsvd1;
	uint32_t    csts;
	uint32_t    nssrc;
	uint32_t    aqa;
	uint64_t    asq;
	uint64_t    acq;
	uint32_t    cmbloc;
	uint32_t    cmbsz;
} NvmeRegs_vals;

typedef union NvmeRegs {
	NvmeRegs_vals vBar;
	NvmeRegs_bits bBar;
} NvmeRegs;

enum NvmeCapShift {
	CAP_MQES_SHIFT     = 0,
	CAP_CQR_SHIFT      = 16,
	CAP_AMS_SHIFT      = 17,
	CAP_TO_SHIFT       = 24,
	CAP_DSTRD_SHIFT    = 32,
	CAP_NSSRS_SHIFT    = 33,
	CAP_CSS_SHIFT      = 37,
	CAP_MPSMIN_SHIFT   = 48,
	CAP_MPSMAX_SHIFT   = 52,
};

enum NvmeCapMask {
	CAP_MQES_MASK      = 0xffff,
	CAP_CQR_MASK       = 0x1,
	CAP_AMS_MASK       = 0x3,
	CAP_TO_MASK        = 0xff,
	CAP_DSTRD_MASK     = 0xf,
	CAP_NSSRS_MASK     = 0x1,
	CAP_CSS_MASK       = 0xff,
	CAP_MPSMIN_MASK    = 0xf,
	CAP_MPSMAX_MASK    = 0xf,
};

#define NVME_CAP_MQES(cap)  (((cap) >> CAP_MQES_SHIFT)   & CAP_MQES_MASK)
#define NVME_CAP_CQR(cap)   (((cap) >> CAP_CQR_SHIFT)    & CAP_CQR_MASK)
#define NVME_CAP_AMS(cap)   (((cap) >> CAP_AMS_SHIFT)    & CAP_AMS_MASK)
#define NVME_CAP_TO(cap)    (((cap) >> CAP_TO_SHIFT)     & CAP_TO_MASK)
#define NVME_CAP_DSTRD(cap) (((cap) >> CAP_DSTRD_SHIFT)  & CAP_DSTRD_MASK)
#define NVME_CAP_NSSRS(cap) (((cap) >> CAP_NSSRS_SHIFT)  & CAP_NSSRS_MASK)
#define NVME_CAP_CSS(cap)   (((cap) >> CAP_CSS_SHIFT)    & CAP_CSS_MASK)
#define NVME_CAP_MPSMIN(cap)(((cap) >> CAP_MPSMIN_SHIFT) & CAP_MPSMIN_MASK)
#define NVME_CAP_MPSMAX(cap)(((cap) >> CAP_MPSMAX_SHIFT) & CAP_MPSMAX_MASK)

#define NVME_CAP_SET_MQES(cap, val)   (cap |= (uint64_t)(val & CAP_MQES_MASK)  \
		<< CAP_MQES_SHIFT)
#define NVME_CAP_SET_CQR(cap, val)    (cap |= (uint64_t)(val & CAP_CQR_MASK)   \
		<< CAP_CQR_SHIFT)
#define NVME_CAP_SET_AMS(cap, val)    (cap |= (uint64_t)(val & CAP_AMS_MASK)   \
		<< CAP_AMS_SHIFT)
#define NVME_CAP_SET_TO(cap, val)     (cap |= (uint64_t)(val & CAP_TO_MASK)    \
		<< CAP_TO_SHIFT)
#define NVME_CAP_SET_DSTRD(cap, val)  (cap |= (uint64_t)(val & CAP_DSTRD_MASK) \
		<< CAP_DSTRD_SHIFT)
#define NVME_CAP_SET_NSSRS(cap, val)  (cap |= (uint64_t)(val & CAP_NSSRS_MASK) \
		<< CAP_NSSRS_SHIFT)
#define NVME_CAP_SET_CSS(cap, val)    (cap |= (uint64_t)(val & CAP_CSS_MASK)   \
		<< CAP_CSS_SHIFT)
#define NVME_CAP_SET_MPSMIN(cap, val) (cap |= (uint64_t)(val & CAP_MPSMIN_MASK)\
		<< CAP_MPSMIN_SHIFT)
#define NVME_CAP_SET_MPSMAX(cap, val) (cap |= (uint64_t)(val & CAP_MPSMAX_MASK)\
		<< CAP_MPSMAX_SHIFT)

enum NvmeCcShift {
	CC_EN_SHIFT     = 0,
	CC_CSS_SHIFT    = 4,
	CC_MPS_SHIFT    = 7,
	CC_AMS_SHIFT    = 11,
	CC_SHN_SHIFT    = 14,
	CC_IOSQES_SHIFT = 16,
	CC_IOCQES_SHIFT = 20,
};

enum NvmeCcMask {
	CC_EN_MASK      = 0x1,
	CC_CSS_MASK     = 0x7,
	CC_MPS_MASK     = 0xf,
	CC_AMS_MASK     = 0x7,
	CC_SHN_MASK     = 0x3,
	CC_IOSQES_MASK  = 0xf,
	CC_IOCQES_MASK  = 0xf,
};

#define NVME_CC_EN(cc)     ((cc >> CC_EN_SHIFT)     & CC_EN_MASK)
#define NVME_CC_CSS(cc)    ((cc >> CC_CSS_SHIFT)    & CC_CSS_MASK)
#define NVME_CC_MPS(cc)    ((cc >> CC_MPS_SHIFT)    & CC_MPS_MASK)
#define NVME_CC_AMS(cc)    ((cc >> CC_AMS_SHIFT)    & CC_AMS_MASK)
#define NVME_CC_SHN(cc)    ((cc >> CC_SHN_SHIFT)    & CC_SHN_MASK)
#define NVME_CC_IOSQES(cc) ((cc >> CC_IOSQES_SHIFT) & CC_IOSQES_MASK)
#define NVME_CC_IOCQES(cc) ((cc >> CC_IOCQES_SHIFT) & CC_IOCQES_MASK)

enum NvmeCstsShift {
	CSTS_RDY_SHIFT      = 0,
	CSTS_CFS_SHIFT      = 1,
	CSTS_SHST_SHIFT     = 2,
	CSTS_NSSRO_SHIFT    = 4,
};

enum NvmeCstsMask {
	CSTS_RDY_MASK   = 0x1,
	CSTS_CFS_MASK   = 0x1,
	CSTS_SHST_MASK  = 0x3,
	CSTS_NSSRO_MASK = 0x1,
};

enum NvmeCsts {
	NVME_CSTS_READY         = 1 << CSTS_RDY_SHIFT,
	NVME_CSTS_FAILED        = 1 << CSTS_CFS_SHIFT,
	NVME_CSTS_SHST_NORMAL   = 0 << CSTS_SHST_SHIFT,
	NVME_CSTS_SHST_PROGRESS = 1 << CSTS_SHST_SHIFT,
	NVME_CSTS_SHST_COMPLETE = 2 << CSTS_SHST_SHIFT,
	NVME_CSTS_NSSRO         = 1 << CSTS_NSSRO_SHIFT,
};

#define NVME_CSTS_RDY(csts)     ((csts >> CSTS_RDY_SHIFT)   & CSTS_RDY_MASK)
#define NVME_CSTS_CFS(csts)     ((csts >> CSTS_CFS_SHIFT)   & CSTS_CFS_MASK)
#define NVME_CSTS_SHST(csts)    ((csts >> CSTS_SHST_SHIFT)  & CSTS_SHST_MASK)
#define NVME_CSTS_NSSRO(csts)   ((csts >> CSTS_NSSRO_SHIFT) & CSTS_NSSRO_MASK)

enum NvmeAqaShift {
	AQA_ASQS_SHIFT  = 0,
	AQA_ACQS_SHIFT  = 16,
};

enum NvmeAqaMask {
	AQA_ASQS_MASK   = 0xfff,
	AQA_ACQS_MASK   = 0xfff,
};

#define NVME_AQA_ASQS(aqa) ((aqa >> AQA_ASQS_SHIFT) & AQA_ASQS_MASK)
#define NVME_AQA_ACQS(aqa) ((aqa >> AQA_ACQS_SHIFT) & AQA_ACQS_MASK)

enum NvmeCmblocShift {
	CMBLOC_BIR_SHIFT  = 0,
	CMBLOC_OFST_SHIFT = 12,
};

enum NvmeCmblocMask {
	CMBLOC_BIR_MASK  = 0x7,
	CMBLOC_OFST_MASK = 0xfffff,
};

#define NVME_CMBLOC_BIR(cmbloc) ((cmbloc >> CMBLOC_BIR_SHIFT)  & CMBLOC_BIR_MASK)
#define NVME_CMBLOC_OFST(cmbloc)((cmbloc >> CMBLOC_OFST_SHIFT) & CMBLOC_OFST_MASK)

#define NVME_CMBLOC_SET_BIR(cmbloc, val)   (cmbloc |= (uint64_t)(val & CMBLOC_BIR_MASK)  \
		<< CMBLOC_BIR_SHIFT)
#define NVME_CMBLOC_SET_OFST(cmbloc, val)  (cmbloc |= (uint64_t)(val & CMBLOC_OFST_MASK)  \
		<< CMBLOC_OFST_SHIFT)
enum NvmeCmbszShift {
	CMBSZ_SQS_SHIFT   = 0,
	CMBSZ_CQS_SHIFT   = 1,
	CMBSZ_LISTS_SHIFT = 2,
	CMBSZ_RDS_SHIFT   = 3,
	CMBSZ_WDS_SHIFT   = 4,
	CMBSZ_SZU_SHIFT   = 8,
	CMBSZ_SZ_SHIFT    = 12,
};

enum NvmeCmbszMask {
	CMBSZ_SQS_MASK   = 0x1,
	CMBSZ_CQS_MASK   = 0x1,
	CMBSZ_LISTS_MASK = 0x1,
	CMBSZ_RDS_MASK   = 0x1,
	CMBSZ_WDS_MASK   = 0x1,
	CMBSZ_SZU_MASK   = 0xf,
	CMBSZ_SZ_MASK    = 0xfffff,
};

#define NVME_CMBSZ_SQS(cmbsz)  ((cmbsz >> CMBSZ_SQS_SHIFT)   & CMBSZ_SQS_MASK)
#define NVME_CMBSZ_CQS(cmbsz)  ((cmbsz >> CMBSZ_CQS_SHIFT)   & CMBSZ_CQS_MASK)
#define NVME_CMBSZ_LISTS(cmbsz)((cmbsz >> CMBSZ_LISTS_SHIFT) & CMBSZ_LISTS_MASK)
#define NVME_CMBSZ_RDS(cmbsz)  ((cmbsz >> CMBSZ_RDS_SHIFT)   & CMBSZ_RDS_MASK)
#define NVME_CMBSZ_WDS(cmbsz)  ((cmbsz >> CMBSZ_WDS_SHIFT)   & CMBSZ_WDS_MASK)
#define NVME_CMBSZ_SZU(cmbsz)  ((cmbsz >> CMBSZ_SZU_SHIFT)   & CMBSZ_SZU_MASK)
#define NVME_CMBSZ_SZ(cmbsz)   ((cmbsz >> CMBSZ_SZ_SHIFT)    & CMBSZ_SZ_MASK)

#define NVME_CMBSZ_SET_SQS(cmbsz, val)   (cmbsz |= (uint64_t)(val & CMBSZ_SQS_MASK)  \
		<< CMBSZ_SQS_SHIFT)
#define NVME_CMBSZ_SET_CQS(cmbsz, val)   (cmbsz |= (uint64_t)(val & CMBSZ_CQS_MASK)  \
		<< CMBSZ_CQS_SHIFT)
#define NVME_CMBSZ_SET_LISTS(cmbsz, val) (cmbsz |= (uint64_t)(val & CMBSZ_LISTS_MASK)  \
		<< CMBSZ_LISTS_SHIFT)
#define NVME_CMBSZ_SET_RDS(cmbsz, val)   (cmbsz |= (uint64_t)(val & CMBSZ_RDS_MASK)  \
		<< CMBSZ_RDS_SHIFT)
#define NVME_CMBSZ_SET_WDS(cmbsz, val)   (cmbsz |= (uint64_t)(val & CMBSZ_WDS_MASK)  \
		<< CMBSZ_WDS_SHIFT)
#define NVME_CMBSZ_SET_SZU(cmbsz, val)   (cmbsz |= (uint64_t)(val & CMBSZ_SZU_MASK)  \
		<< CMBSZ_SZU_SHIFT)
#define NVME_CMBSZ_SET_SZ(cmbsz, val)    (cmbsz |= (uint64_t)(val & CMBSZ_SZ_MASK)  \
		<< CMBSZ_SZ_SHIFT)

#define NVME_CMBSZ_GETSIZE(cmbsz) (NVME_CMBSZ_SZ(cmbsz) * (1<<(12+4*NVME_CMBSZ_SZU(cmbsz))))

#endif /*#ifndef __NVME_REGS_H*/

