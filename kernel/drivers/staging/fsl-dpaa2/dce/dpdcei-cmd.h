/* Copyright 2013-2016 Freescale Semiconductor Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * * Neither the name of the above-listed copyright holders nor the
 * names of any contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FSL_DPDCEI_CMD_H
#define _FSL_DPDCEI_CMD_H

/* DPDCEI Version */
#define DPDCEI_VER_MAJOR				1
#define DPDCEI_VER_MINOR				2

#define DPDCEI_CMD_BASE_VER				0
#define DPDCEI_CMD_ID_OFF				4
#define DPDCEI_CMD_ID(id) (((id) << DPDCEI_CMD_ID_OFF) | DPDCEI_CMD_BASE_VER)

/* Command IDs */
#define DPDCEI_CMDID_CLOSE				DPDCEI_CMD_ID(0x800)
#define DPDCEI_CMDID_OPEN				DPDCEI_CMD_ID(0x80D)
#define DPDCEI_CMDID_CREATE				DPDCEI_CMD_ID(0x90D)
#define DPDCEI_CMDID_DESTROY			DPDCEI_CMD_ID(0x900)

#define DPDCEI_CMDID_ENABLE				DPDCEI_CMD_ID(0x002)
#define DPDCEI_CMDID_DISABLE			DPDCEI_CMD_ID(0x003)
#define DPDCEI_CMDID_GET_ATTR			DPDCEI_CMD_ID(0x004)
#define DPDCEI_CMDID_RESET				DPDCEI_CMD_ID(0x005)
#define DPDCEI_CMDID_IS_ENABLED			DPDCEI_CMD_ID(0x006)

#define DPDCEI_CMDID_SET_IRQ			DPDCEI_CMD_ID(0x010)
#define DPDCEI_CMDID_GET_IRQ			DPDCEI_CMD_ID(0x011)
#define DPDCEI_CMDID_SET_IRQ_ENABLE		DPDCEI_CMD_ID(0x012)
#define DPDCEI_CMDID_GET_IRQ_ENABLE		DPDCEI_CMD_ID(0x013)
#define DPDCEI_CMDID_SET_IRQ_MASK		DPDCEI_CMD_ID(0x014)
#define DPDCEI_CMDID_GET_IRQ_MASK		DPDCEI_CMD_ID(0x015)
#define DPDCEI_CMDID_GET_IRQ_STATUS		DPDCEI_CMD_ID(0x016)
#define DPDCEI_CMDID_CLEAR_IRQ_STATUS	DPDCEI_CMD_ID(0x017)

#define DPDCEI_CMDID_SET_RX_QUEUE		DPDCEI_CMD_ID(0x1B0)
#define DPDCEI_CMDID_GET_RX_QUEUE		DPDCEI_CMD_ID(0x1B1)
#define DPDCEI_CMDID_GET_TX_QUEUE		DPDCEI_CMD_ID(0x1B2)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_OPEN(cmd, dpdcei_id) \
	MC_CMD_OP(cmd, 0, 0,  32, int,      dpdcei_id)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_CREATE(cmd, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 8,  8,  enum dpdcei_engine,  cfg->engine);\
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  cfg->priority);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_RSP_IS_ENABLED(cmd, en) \
	MC_RSP_OP(cmd, 0, 0,  1,  int,	    en)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_SET_IRQ(cmd, irq_index, irq_addr, irq_val, user_irq_id) \
do { \
	MC_CMD_OP(cmd, 0, 0,  8,  uint8_t,  irq_index);\
	MC_CMD_OP(cmd, 0, 32, 32, u32, irq_val);\
	MC_CMD_OP(cmd, 1, 0,  64, uint64_t, irq_addr);\
	MC_CMD_OP(cmd, 2, 0,  32, int,	    user_irq_id); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_GET_IRQ(cmd, irq_index) \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_RSP_GET_IRQ(cmd, type, irq_addr, irq_val, user_irq_id) \
do { \
	MC_RSP_OP(cmd, 0, 0,  32, u32, irq_val); \
	MC_RSP_OP(cmd, 1, 0,  64, uint64_t, irq_addr);\
	MC_RSP_OP(cmd, 2, 0,  32, int,	    user_irq_id); \
	MC_RSP_OP(cmd, 2, 32, 32, int,	    type); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_SET_IRQ_ENABLE(cmd, irq_index, enable_state) \
do { \
	MC_CMD_OP(cmd, 0, 0,  8,  uint8_t,  enable_state); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_GET_IRQ_ENABLE(cmd, irq_index) \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_RSP_GET_IRQ_ENABLE(cmd, enable_state) \
	MC_RSP_OP(cmd, 0, 0,  8,  uint8_t,  enable_state)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_SET_IRQ_MASK(cmd, irq_index, mask) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, u32, mask); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_GET_IRQ_MASK(cmd, irq_index) \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_RSP_GET_IRQ_MASK(cmd, mask) \
	MC_RSP_OP(cmd, 0, 0,  32, u32, mask)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_GET_IRQ_STATUS(cmd, irq_index) \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_RSP_GET_IRQ_STATUS(cmd, status) \
	MC_RSP_OP(cmd, 0, 0,  32, u32,  status)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_CLEAR_IRQ_STATUS(cmd, irq_index, status) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, u32, status); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_RSP_GET_ATTR(cmd, attr) \
do { \
	MC_RSP_OP(cmd, 0, 0,  32, int,	    attr->id); \
	MC_RSP_OP(cmd, 0, 32,  8,  enum dpdcei_engine,  attr->engine); \
	MC_RSP_OP(cmd, 1, 0,  16, uint16_t, attr->version.major);\
	MC_RSP_OP(cmd, 1, 16, 16, uint16_t, attr->version.minor);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_CMD_SET_RX_QUEUE(cmd, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, int,      cfg->dest_cfg.dest_id); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  cfg->dest_cfg.priority); \
	MC_CMD_OP(cmd, 0, 48, 4,  enum dpdcei_dest, cfg->dest_cfg.dest_type); \
	MC_CMD_OP(cmd, 1, 0,  64, uint64_t, cfg->user_ctx); \
	MC_CMD_OP(cmd, 2, 0,  32, u32, cfg->options);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_RSP_GET_RX_QUEUE(cmd, attr) \
do { \
	MC_RSP_OP(cmd, 0, 0,  32, int,      attr->dest_cfg.dest_id);\
	MC_RSP_OP(cmd, 0, 32, 8,  uint8_t,  attr->dest_cfg.priority);\
	MC_RSP_OP(cmd, 0, 48, 4,  enum dpdcei_dest, attr->dest_cfg.dest_type);\
	MC_RSP_OP(cmd, 1, 0,  64, uint64_t,  attr->user_ctx);\
	MC_RSP_OP(cmd, 2, 0,  32, u32,  attr->fqid);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPDCEI_RSP_GET_TX_QUEUE(cmd, attr) \
	MC_RSP_OP(cmd, 0, 32, 32, u32,  attr->fqid)

#endif /* _FSL_DPDCEI_CMD_H */
