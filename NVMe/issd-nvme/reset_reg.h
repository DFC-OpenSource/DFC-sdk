#ifndef __RESET_REG_H__
#define __RESET_REG_H__

#include "nvme.h"

static const NvmeRegs_bits NvmeRegsReset = {
	.cap = {
		.mqes   =   0x7FF,
		.cqr    =   0x1,
		.ams    =   0x1,
		.rsvd1  =   0,
		.to     =   0xff,
		.dstrd  =   0,
		.nssrs  =   0x1,
		.Css    =   0x01,
		.rsvd2  =   0,
		.mpsmin =   0,
		.mpsmax =   0xF,
		.rsvd3  =   0,
	},

	.vs = {
		.rsvd   =   0,
		.mnr    =   0x2,
		.mjr    =   0x1,
	},

	.intms = {
		.ivms   =   0,
	},

	.intmc = {
		.ivmc   =   0,
	},

	.cc = {
		.en     =   0,
		.rsvd1  =   0,
		.iocss  =   0,
		.mps    =   0,
		.ams    =   0,
		.shn    =   0,
		.iosqes =   0,
		.iocqes =   0,
		.rsvd2  =   0,
	},

	.csts = {
		.rdy    =   0,
		.cfs    =   0,
		.shst   =   0,
		.nssro  =   0,
		.pp     =   0,
		.rsvd   =   0,
	},

	.nssrc = {
		.nssrc  =   0,
	},

	.aqa = {
		.asqs   =   0,
		.rsvd1  =   0,
		.acqs   =   0,
		.rsvd2  =   0,
	},

	.asq = {
		.rsvd   =   0,
		.asqb   =   0,
	},

	.acq = {
		.rsvd   =   0,
		.acqb   =   0,
	},

	.cmbloc = {
		.bir    =   0,
		.rsvd   =   0,
		.ofst   =   0,
	},

	.cmbsz = {
		.sqs    =   0,
		.cqs    =   0,
		.lists  =   0,
		.rds    =   0,
		.wds    =   0,
		.rsvd   =   0,
		.szu    =   0,
		.sz     =   0,
	},
};

#endif /*__RESET_REG_H__*/
