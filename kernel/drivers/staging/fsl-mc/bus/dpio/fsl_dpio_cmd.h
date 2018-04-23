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
#ifndef _FSL_DPIO_CMD_H
#define _FSL_DPIO_CMD_H

/* DPIO Version */
#define DPIO_VER_MAJOR				4
#define DPIO_VER_MINOR				2
#define DPIO_CMD_BASE_VERSION			1
#define DPIO_CMD_ID_OFFSET			4

/* Command IDs */
#define DPIO_CMDID_CLOSE                         ((0x800 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_OPEN                          ((0x803 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_CREATE                        ((0x903 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_DESTROY                       ((0x983 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_GET_API_VERSION               ((0xa03 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)

#define DPIO_CMDID_ENABLE                        ((0x002 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_DISABLE                       ((0x003 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_GET_ATTR                      ((0x004 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_RESET                         ((0x005 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_IS_ENABLED                    ((0x006 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)

#define DPIO_CMDID_SET_IRQ                       ((0x010 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_GET_IRQ                       ((0x011 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_SET_IRQ_ENABLE                ((0x012 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_GET_IRQ_ENABLE                ((0x013 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_SET_IRQ_MASK                  ((0x014 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_GET_IRQ_MASK                  ((0x015 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_GET_IRQ_STATUS                ((0x016 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_CLEAR_IRQ_STATUS              ((0x017 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)

#define DPIO_CMDID_SET_STASHING_DEST             ((0x120 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_GET_STASHING_DEST             ((0x121 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_ADD_STATIC_DEQUEUE_CHANNEL    ((0x122 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)
#define DPIO_CMDID_REMOVE_STATIC_DEQUEUE_CHANNEL ((0x123 << DPIO_CMD_ID_OFFSET) | DPIO_CMD_BASE_VERSION)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_OPEN(cmd, dpio_id) \
	MC_CMD_OP(cmd, 0, 0,  32, uint32_t,     dpio_id)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_CREATE(cmd, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 16, 2,  enum dpio_channel_mode,	\
					   cfg->channel_mode);\
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t, cfg->num_priorities);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_RSP_IS_ENABLED(cmd, en) \
	MC_RSP_OP(cmd, 0, 0,  1,  int,	    en)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_SET_IRQ(cmd, irq_index, irq_cfg) \
do { \
	MC_CMD_OP(cmd, 0, 0,  8,  uint8_t,  irq_index);\
	MC_CMD_OP(cmd, 0, 32, 32, uint32_t, irq_cfg->val);\
	MC_CMD_OP(cmd, 1, 0,  64, uint64_t, irq_cfg->addr);\
	MC_CMD_OP(cmd, 2, 0,  32, int,	    irq_cfg->irq_num); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_GET_IRQ(cmd, irq_index) \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_RSP_GET_IRQ(cmd, type, irq_cfg) \
do { \
	MC_RSP_OP(cmd, 0, 0,  32, uint32_t, irq_cfg->val); \
	MC_RSP_OP(cmd, 1, 0,  64, uint64_t, irq_cfg->addr); \
	MC_RSP_OP(cmd, 2, 0,  32, int,	    irq_cfg->irq_num); \
	MC_RSP_OP(cmd, 2, 32, 32, int,	    type); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_SET_IRQ_ENABLE(cmd, irq_index, en) \
do { \
	MC_CMD_OP(cmd, 0, 0,  8,  uint8_t, en); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t, irq_index);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_GET_IRQ_ENABLE(cmd, irq_index) \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_RSP_GET_IRQ_ENABLE(cmd, en) \
	MC_RSP_OP(cmd, 0, 0,  8,  uint8_t,  en)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_SET_IRQ_MASK(cmd, irq_index, mask) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, uint32_t, mask); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_GET_IRQ_MASK(cmd, irq_index) \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_RSP_GET_IRQ_MASK(cmd, mask) \
	MC_RSP_OP(cmd, 0, 0,  32, uint32_t, mask)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_GET_IRQ_STATUS(cmd, irq_index, status) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, uint32_t, status);\
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_RSP_GET_IRQ_STATUS(cmd, status) \
	MC_RSP_OP(cmd, 0, 0,  32, uint32_t, status)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_CLEAR_IRQ_STATUS(cmd, irq_index, status) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, uint32_t, status); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_RSP_GET_ATTR(cmd, attr) \
do { \
	MC_RSP_OP(cmd, 0, 0,  32, int,	    attr->id);\
	MC_RSP_OP(cmd, 0, 32, 16, uint16_t, attr->qbman_portal_id);\
	MC_RSP_OP(cmd, 0, 48, 8,  uint8_t,  attr->num_priorities);\
	MC_RSP_OP(cmd, 0, 56, 4,  enum dpio_channel_mode, attr->channel_mode);\
	MC_RSP_OP(cmd, 1, 0,  64, uint64_t, attr->qbman_portal_ce_offset);\
	MC_RSP_OP(cmd, 2, 0,  64, uint64_t, attr->qbman_portal_ci_offset);\
	MC_RSP_OP(cmd, 3, 0, 32, uint32_t, attr->qbman_version);\
	MC_RSP_OP(cmd, 4, 0,  32, uint32_t, attr->clk);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_SET_STASHING_DEST(cmd, sdest) \
	MC_CMD_OP(cmd, 0, 0,  8,  uint8_t,  sdest)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_RSP_GET_STASHING_DEST(cmd, sdest) \
	MC_RSP_OP(cmd, 0, 0,  8,  uint8_t,  sdest)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_ADD_STATIC_DEQUEUE_CHANNEL(cmd, dpcon_id) \
	MC_CMD_OP(cmd, 0, 0,  32, int,      dpcon_id)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_RSP_ADD_STATIC_DEQUEUE_CHANNEL(cmd, channel_index) \
	MC_RSP_OP(cmd, 0, 0,  8,  uint8_t,  channel_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPIO_CMD_REMOVE_STATIC_DEQUEUE_CHANNEL(cmd, dpcon_id) \
	MC_CMD_OP(cmd, 0, 0,  32, int,      dpcon_id)

#endif /* _FSL_DPIO_CMD_H */
