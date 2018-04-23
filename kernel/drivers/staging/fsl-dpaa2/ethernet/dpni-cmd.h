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
#ifndef _FSL_DPNI_CMD_H
#define _FSL_DPNI_CMD_H

/* DPNI Version */
#define DPNI_VER_MAJOR				7
#define DPNI_VER_MINOR				0
#define DPNI_CMD_BASE_VERSION			1
#define DPNI_CMD_ID_OFFSET			4

/* Command IDs */
#define DPNI_CMDID_OPEN                              ((0x801 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_CLOSE                             ((0x800 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_CREATE                            ((0x901 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_DESTROY                           ((0x981 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_API_VERSION                   ((0xa01 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_ENABLE                            ((0x002 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_DISABLE                           ((0x003 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_ATTR                          ((0x004 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_RESET                             ((0x005 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_IS_ENABLED                        ((0x006 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_SET_IRQ                           ((0x010 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_IRQ                           ((0x011 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_IRQ_ENABLE                    ((0x012 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_IRQ_ENABLE                    ((0x013 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_IRQ_MASK                      ((0x014 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_IRQ_MASK                      ((0x015 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_IRQ_STATUS                    ((0x016 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_CLEAR_IRQ_STATUS                  ((0x017 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_SET_POOLS                         ((0x200 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_ERRORS_BEHAVIOR               ((0x20B << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_GET_QDID                          ((0x210 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_SP_INFO                       ((0x211 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_TX_DATA_OFFSET                ((0x212 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_LINK_STATE                    ((0x215 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_MAX_FRAME_LENGTH              ((0x216 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_MAX_FRAME_LENGTH              ((0x217 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_LINK_CFG                      ((0x21A << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_TX_SHAPING                    ((0x21B << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_SET_MCAST_PROMISC                 ((0x220 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_MCAST_PROMISC                 ((0x221 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_UNICAST_PROMISC               ((0x222 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_UNICAST_PROMISC               ((0x223 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_PRIM_MAC                      ((0x224 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_PRIM_MAC                      ((0x225 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_ADD_MAC_ADDR                      ((0x226 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_REMOVE_MAC_ADDR                   ((0x227 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_CLR_MAC_FILTERS                   ((0x228 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_ENABLE_VLAN_FILTER                ((0x230 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_ADD_VLAN_ID                       ((0x231 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_REMOVE_VLAN_ID                    ((0x232 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_CLR_VLAN_FILTERS                  ((0x233 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_SET_RX_TC_DIST                    ((0x235 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_SET_RX_TC_POLICING                ((0x23E << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_SET_QOS_TBL                       ((0x240 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_ADD_QOS_ENT                       ((0x241 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_REMOVE_QOS_ENT                    ((0x242 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_CLR_QOS_TBL                       ((0x243 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_ADD_FS_ENT                        ((0x244 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_REMOVE_FS_ENT                     ((0x245 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_CLR_FS_ENT                        ((0x246 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_SET_TX_PRIORITIES                 ((0x250 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_RX_TC_POLICING                ((0x251 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_GET_STATISTICS                    ((0x25D << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_RESET_STATISTICS                  ((0x25E << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_QUEUE                         ((0x25F << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_QUEUE                         ((0x260 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_TAILDROP                      ((0x261 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_TAILDROP                      ((0x262 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_GET_PORT_MAC_ADDR                 ((0x263 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_GET_BUFFER_LAYOUT                 ((0x264 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_BUFFER_LAYOUT                 ((0x265 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

#define DPNI_CMDID_SET_TX_CONFIRMATION_MODE          ((0x266 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_CONGESTION_NOTIFICATION       ((0x267 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_CONGESTION_NOTIFICATION       ((0x268 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_EARLY_DROP                    ((0x269 << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_EARLY_DROP                    ((0x26A << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_GET_OFFLOAD                       ((0x26B << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)
#define DPNI_CMDID_SET_OFFLOAD                       ((0x26C << DPNI_CMD_ID_OFFSET) | DPNI_CMD_BASE_VERSION)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_OPEN(cmd, dpni_id) \
	MC_CMD_OP(cmd,	 0,	0,	32,	int,	dpni_id)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_CREATE(cmd, cfg) \
do { \
	MC_CMD_OP(cmd, 0,  0, 32, uint32_t,  (cfg)->options); \
	MC_CMD_OP(cmd, 0, 32,  8,  uint8_t,  (cfg)->num_queues); \
	MC_CMD_OP(cmd, 0, 40,  8,  uint8_t,  (cfg)->num_tcs); \
	MC_CMD_OP(cmd, 0, 48,  8,  uint8_t,  (cfg)->mac_filter_entries); \
	MC_CMD_OP(cmd, 1,  0,  8,  uint8_t,  (cfg)->vlan_filter_entries); \
	MC_CMD_OP(cmd, 1, 16,  8,  uint8_t,  (cfg)->qos_entries); \
	MC_CMD_OP(cmd, 1, 32, 16, uint16_t,  (cfg)->fs_entries); \
} while (0)


/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_POOLS(cmd, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 0,  8,  uint8_t,  cfg->num_dpbp); \
	MC_CMD_OP(cmd, 0, 8,  1,  int,      cfg->pools[0].backup_pool); \
	MC_CMD_OP(cmd, 0, 9,  1,  int,      cfg->pools[1].backup_pool); \
	MC_CMD_OP(cmd, 0, 10, 1,  int,      cfg->pools[2].backup_pool); \
	MC_CMD_OP(cmd, 0, 11, 1,  int,      cfg->pools[3].backup_pool); \
	MC_CMD_OP(cmd, 0, 12, 1,  int,      cfg->pools[4].backup_pool); \
	MC_CMD_OP(cmd, 0, 13, 1,  int,      cfg->pools[5].backup_pool); \
	MC_CMD_OP(cmd, 0, 14, 1,  int,      cfg->pools[6].backup_pool); \
	MC_CMD_OP(cmd, 0, 15, 1,  int,      cfg->pools[7].backup_pool); \
	MC_CMD_OP(cmd, 0, 32, 32, int,      cfg->pools[0].dpbp_id); \
	MC_CMD_OP(cmd, 4, 32, 16, uint16_t, cfg->pools[0].buffer_size);\
	MC_CMD_OP(cmd, 1, 0,  32, int,      cfg->pools[1].dpbp_id); \
	MC_CMD_OP(cmd, 4, 48, 16, uint16_t, cfg->pools[1].buffer_size);\
	MC_CMD_OP(cmd, 1, 32, 32, int,      cfg->pools[2].dpbp_id); \
	MC_CMD_OP(cmd, 5, 0,  16, uint16_t, cfg->pools[2].buffer_size);\
	MC_CMD_OP(cmd, 2, 0,  32, int,      cfg->pools[3].dpbp_id); \
	MC_CMD_OP(cmd, 5, 16, 16, uint16_t, cfg->pools[3].buffer_size);\
	MC_CMD_OP(cmd, 2, 32, 32, int,      cfg->pools[4].dpbp_id); \
	MC_CMD_OP(cmd, 5, 32, 16, uint16_t, cfg->pools[4].buffer_size);\
	MC_CMD_OP(cmd, 3, 0,  32, int,      cfg->pools[5].dpbp_id); \
	MC_CMD_OP(cmd, 5, 48, 16, uint16_t, cfg->pools[5].buffer_size);\
	MC_CMD_OP(cmd, 3, 32, 32, int,      cfg->pools[6].dpbp_id); \
	MC_CMD_OP(cmd, 6, 0,  16, uint16_t, cfg->pools[6].buffer_size);\
	MC_CMD_OP(cmd, 4, 0,  32, int,      cfg->pools[7].dpbp_id); \
	MC_CMD_OP(cmd, 6, 16, 16, uint16_t, cfg->pools[7].buffer_size);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_IS_ENABLED(cmd, en) \
	MC_RSP_OP(cmd, 0, 0,  1,  int,	    en)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_IRQ(cmd, irq_index, irq_cfg) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, uint32_t, irq_cfg->val); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index); \
	MC_CMD_OP(cmd, 1, 0,  64, uint64_t, irq_cfg->addr); \
	MC_CMD_OP(cmd, 2, 0,  32, int,	    irq_cfg->irq_num); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_GET_IRQ(cmd, irq_index) \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_IRQ(cmd, type, irq_cfg) \
do { \
	MC_RSP_OP(cmd, 0, 0,  32, uint32_t, irq_cfg->val); \
	MC_RSP_OP(cmd, 1, 0,  64, uint64_t, irq_cfg->addr); \
	MC_RSP_OP(cmd, 2, 0,  32, int,      irq_cfg->irq_num); \
	MC_RSP_OP(cmd, 2, 32, 32, int,	    type); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_IRQ_ENABLE(cmd, irq_index, en) \
do { \
	MC_CMD_OP(cmd, 0, 0,  8,  uint8_t,  en); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_GET_IRQ_ENABLE(cmd, irq_index) \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_IRQ_ENABLE(cmd, en) \
	MC_RSP_OP(cmd, 0, 0,  8,  uint8_t,  en)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_IRQ_MASK(cmd, irq_index, mask) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, uint32_t, mask); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_GET_IRQ_MASK(cmd, irq_index) \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_IRQ_MASK(cmd, mask) \
	MC_RSP_OP(cmd, 0, 0,  32, uint32_t,  mask)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_GET_IRQ_STATUS(cmd, irq_index, status) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, uint32_t, status);\
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_IRQ_STATUS(cmd, status) \
	MC_RSP_OP(cmd, 0, 0,  32, uint32_t,  status)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_CLEAR_IRQ_STATUS(cmd, irq_index, status) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, uint32_t, status); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  irq_index);\
} while (0)

/* DPNI_CMD_GET_ATTR is not used, no input parameters */

#define DPNI_RSP_GET_ATTR(cmd, attr) \
do { \
	MC_RSP_OP(cmd, 0,  0, 32, uint32_t, (attr)->options); \
	MC_RSP_OP(cmd, 0, 32,  8, uint8_t,  (attr)->num_queues); \
	MC_RSP_OP(cmd, 0, 40,  8, uint8_t,  (attr)->num_tcs); \
	MC_RSP_OP(cmd, 0, 48,  8, uint8_t,  (attr)->mac_filter_entries); \
	MC_RSP_OP(cmd, 1,  0,  8, uint8_t, (attr)->vlan_filter_entries); \
	MC_RSP_OP(cmd, 1, 16,  8, uint8_t,  (attr)->qos_entries); \
	MC_RSP_OP(cmd, 1, 32, 16, uint16_t, (attr)->fs_entries); \
	MC_RSP_OP(cmd, 2,  0,  8, uint8_t,  (attr)->qos_key_size); \
	MC_RSP_OP(cmd, 2,  8,  8, uint8_t,  (attr)->fs_key_size); \
	MC_RSP_OP(cmd, 2, 16, 16, uint16_t, (attr)->wriop_version); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_ERRORS_BEHAVIOR(cmd, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 0,  32, uint32_t, cfg->errors); \
	MC_CMD_OP(cmd, 0, 32, 4,  enum dpni_error_action, cfg->error_action); \
	MC_CMD_OP(cmd, 0, 36, 1,  int,      cfg->set_frame_annotation); \
} while (0)

#define DPNI_CMD_GET_BUFFER_LAYOUT(cmd, qtype) \
do { \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_queue_type, qtype); \
} while (0)

#define DPNI_RSP_GET_BUFFER_LAYOUT(cmd, layout) \
do { \
	MC_RSP_OP(cmd, 0, 48,  1, char, (layout)->pass_timestamp); \
	MC_RSP_OP(cmd, 0, 49,  1, char, (layout)->pass_parser_result); \
	MC_RSP_OP(cmd, 0, 50,  1, char, (layout)->pass_frame_status); \
	MC_RSP_OP(cmd, 1,  0, 16, uint16_t, (layout)->private_data_size); \
	MC_RSP_OP(cmd, 1, 16, 16, uint16_t, (layout)->data_align); \
	MC_RSP_OP(cmd, 1, 32, 16, uint16_t, (layout)->data_head_room); \
	MC_RSP_OP(cmd, 1, 48, 16, uint16_t, (layout)->data_tail_room); \
} while (0)

#define DPNI_CMD_SET_BUFFER_LAYOUT(cmd, qtype, layout) \
do { \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_queue_type, qtype); \
	MC_CMD_OP(cmd, 0, 32, 16, uint16_t, (layout)->options); \
	MC_CMD_OP(cmd, 0, 48,  1, char, (layout)->pass_timestamp); \
	MC_CMD_OP(cmd, 0, 49,  1, char, (layout)->pass_parser_result); \
	MC_CMD_OP(cmd, 0, 50,  1, char, (layout)->pass_frame_status); \
	MC_CMD_OP(cmd, 1,  0, 16, uint16_t, (layout)->private_data_size); \
	MC_CMD_OP(cmd, 1, 16, 16, uint16_t, (layout)->data_align); \
	MC_CMD_OP(cmd, 1, 32, 16, uint16_t, (layout)->data_head_room); \
	MC_CMD_OP(cmd, 1, 48, 16, uint16_t, (layout)->data_tail_room); \
} while (0)

#define DPNI_CMD_SET_OFFLOAD(cmd, type, config) \
do { \
	MC_CMD_OP(cmd, 0, 24,  8, enum dpni_offload, type); \
	MC_CMD_OP(cmd, 0, 32, 32, uint32_t, config); \
} while (0)

#define DPNI_CMD_GET_OFFLOAD(cmd, type) \
	MC_CMD_OP(cmd, 0, 24,  8, enum dpni_offload, type)

#define DPNI_RSP_GET_OFFLOAD(cmd, config) \
	MC_RSP_OP(cmd, 0, 32, 32, uint32_t, config)

#define DPNI_CMD_GET_QDID(cmd, qtype) \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_queue_type, qtype)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_QDID(cmd, qdid) \
	MC_RSP_OP(cmd, 0, 0,  16, uint16_t, qdid)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_SP_INFO(cmd, sp_info) \
do { \
	MC_RSP_OP(cmd, 0, 0,  16, uint16_t, sp_info->spids[0]); \
	MC_RSP_OP(cmd, 0, 16, 16, uint16_t, sp_info->spids[1]); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_TX_DATA_OFFSET(cmd, data_offset) \
	MC_RSP_OP(cmd, 0, 0,  16, uint16_t, data_offset)

#define DPNI_CMD_GET_STATISTICS(cmd, page) \
do { \
	MC_CMD_OP(cmd, 0, 0, 8, uint8_t, page); \
} while (0)

#define DPNI_RSP_GET_STATISTICS(cmd, stat) \
do { \
	MC_RSP_OP(cmd, 0, 0, 64, uint64_t, (stat)->raw.counter[0]); \
	MC_RSP_OP(cmd, 1, 0, 64, uint64_t, (stat)->raw.counter[1]); \
	MC_RSP_OP(cmd, 2, 0, 64, uint64_t, (stat)->raw.counter[2]); \
	MC_RSP_OP(cmd, 3, 0, 64, uint64_t, (stat)->raw.counter[3]); \
	MC_RSP_OP(cmd, 4, 0, 64, uint64_t, (stat)->raw.counter[4]); \
	MC_RSP_OP(cmd, 5, 0, 64, uint64_t, (stat)->raw.counter[5]); \
	MC_RSP_OP(cmd, 6, 0, 64, uint64_t, (stat)->raw.counter[6]); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_LINK_CFG(cmd, cfg) \
do { \
	MC_CMD_OP(cmd, 1, 0,  32, uint32_t, cfg->rate);\
	MC_CMD_OP(cmd, 2, 0,  64, uint64_t, cfg->options);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_LINK_STATE(cmd, state) \
do { \
	MC_RSP_OP(cmd, 0, 32,  1, int,      state->up);\
	MC_RSP_OP(cmd, 1, 0,  32, uint32_t, state->rate);\
	MC_RSP_OP(cmd, 2, 0,  64, uint64_t, state->options);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_TX_SHAPING(cmd, tx_shaper) \
do { \
	MC_CMD_OP(cmd, 0, 0,  16, uint16_t, tx_shaper->max_burst_size);\
	MC_CMD_OP(cmd, 1, 0,  32, uint32_t, tx_shaper->rate_limit);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_MAX_FRAME_LENGTH(cmd, max_frame_length) \
	MC_CMD_OP(cmd, 0, 0,  16, uint16_t, max_frame_length)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_MAX_FRAME_LENGTH(cmd, max_frame_length) \
	MC_RSP_OP(cmd, 0, 0,  16, uint16_t, max_frame_length)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_MULTICAST_PROMISC(cmd, en) \
	MC_CMD_OP(cmd, 0, 0,  1,  int,      en)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_MULTICAST_PROMISC(cmd, en) \
	MC_RSP_OP(cmd, 0, 0,  1,  int,	    en)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_UNICAST_PROMISC(cmd, en) \
	MC_CMD_OP(cmd, 0, 0,  1,  int,      en)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_UNICAST_PROMISC(cmd, en) \
	MC_RSP_OP(cmd, 0, 0,  1,  int,	    en)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_PRIMARY_MAC_ADDR(cmd, mac_addr) \
do { \
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  mac_addr[5]); \
	MC_CMD_OP(cmd, 0, 24, 8,  uint8_t,  mac_addr[4]); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  mac_addr[3]); \
	MC_CMD_OP(cmd, 0, 40, 8,  uint8_t,  mac_addr[2]); \
	MC_CMD_OP(cmd, 0, 48, 8,  uint8_t,  mac_addr[1]); \
	MC_CMD_OP(cmd, 0, 56, 8,  uint8_t,  mac_addr[0]); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_PRIMARY_MAC_ADDR(cmd, mac_addr) \
do { \
	MC_RSP_OP(cmd, 0, 16, 8,  uint8_t,  mac_addr[5]); \
	MC_RSP_OP(cmd, 0, 24, 8,  uint8_t,  mac_addr[4]); \
	MC_RSP_OP(cmd, 0, 32, 8,  uint8_t,  mac_addr[3]); \
	MC_RSP_OP(cmd, 0, 40, 8,  uint8_t,  mac_addr[2]); \
	MC_RSP_OP(cmd, 0, 48, 8,  uint8_t,  mac_addr[1]); \
	MC_RSP_OP(cmd, 0, 56, 8,  uint8_t,  mac_addr[0]); \
} while (0)

#define DPNI_RSP_GET_PORT_MAC_ADDR(cmd, mac_addr) \
do { \
	MC_RSP_OP(cmd, 0, 16, 8,  uint8_t,  mac_addr[5]); \
	MC_RSP_OP(cmd, 0, 24, 8,  uint8_t,  mac_addr[4]); \
	MC_RSP_OP(cmd, 0, 32, 8,  uint8_t,  mac_addr[3]); \
	MC_RSP_OP(cmd, 0, 40, 8,  uint8_t,  mac_addr[2]); \
	MC_RSP_OP(cmd, 0, 48, 8,  uint8_t,  mac_addr[1]); \
	MC_RSP_OP(cmd, 0, 56, 8,  uint8_t,  mac_addr[0]); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_ADD_MAC_ADDR(cmd, mac_addr) \
do { \
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  mac_addr[5]); \
	MC_CMD_OP(cmd, 0, 24, 8,  uint8_t,  mac_addr[4]); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  mac_addr[3]); \
	MC_CMD_OP(cmd, 0, 40, 8,  uint8_t,  mac_addr[2]); \
	MC_CMD_OP(cmd, 0, 48, 8,  uint8_t,  mac_addr[1]); \
	MC_CMD_OP(cmd, 0, 56, 8,  uint8_t,  mac_addr[0]); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_REMOVE_MAC_ADDR(cmd, mac_addr) \
do { \
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  mac_addr[5]); \
	MC_CMD_OP(cmd, 0, 24, 8,  uint8_t,  mac_addr[4]); \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  mac_addr[3]); \
	MC_CMD_OP(cmd, 0, 40, 8,  uint8_t,  mac_addr[2]); \
	MC_CMD_OP(cmd, 0, 48, 8,  uint8_t,  mac_addr[1]); \
	MC_CMD_OP(cmd, 0, 56, 8,  uint8_t,  mac_addr[0]); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_CLEAR_MAC_FILTERS(cmd, unicast, multicast) \
do { \
	MC_CMD_OP(cmd, 0, 0,  1,  int,      unicast); \
	MC_CMD_OP(cmd, 0, 1,  1,  int,      multicast); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_ENABLE_VLAN_FILTER(cmd, en) \
	MC_CMD_OP(cmd, 0, 0,  1,  int,	    en)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_ADD_VLAN_ID(cmd, vlan_id) \
	MC_CMD_OP(cmd, 0, 32, 16, uint16_t, vlan_id)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_REMOVE_VLAN_ID(cmd, vlan_id) \
	MC_CMD_OP(cmd, 0, 32, 16, uint16_t, vlan_id)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_TX_PRIORITIES(cmd, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 0,  16,  uint16_t, (cfg)->tc_sched[0].delta_bandwidth);\
	MC_CMD_OP(cmd, 0, 16,  4,  enum dpni_tx_schedule_mode, \
				(cfg)->tc_sched[0].mode); \
	MC_CMD_OP(cmd, 0, 32, 16,  uint16_t, (cfg)->tc_sched[1].delta_bandwidth);\
	MC_CMD_OP(cmd, 0, 48, 4,  enum dpni_tx_schedule_mode, \
				(cfg)->tc_sched[1].mode); \
	MC_CMD_OP(cmd, 1, 0,  16,  uint16_t, (cfg)->tc_sched[2].delta_bandwidth);\
	MC_CMD_OP(cmd, 1, 16,  4,  enum dpni_tx_schedule_mode, \
				(cfg)->tc_sched[2].mode); \
	MC_CMD_OP(cmd, 1, 32, 16,  uint16_t, (cfg)->tc_sched[3].delta_bandwidth);\
	MC_CMD_OP(cmd, 1, 48, 4,  enum dpni_tx_schedule_mode, \
				(cfg)->tc_sched[3].mode); \
	MC_CMD_OP(cmd, 2, 0,  16,  uint16_t, (cfg)->tc_sched[4].delta_bandwidth);\
	MC_CMD_OP(cmd, 2, 16,  4,  enum dpni_tx_schedule_mode, \
				(cfg)->tc_sched[4].mode); \
	MC_CMD_OP(cmd, 2, 32, 16,  uint16_t, (cfg)->tc_sched[5].delta_bandwidth);\
	MC_CMD_OP(cmd, 2, 48, 4,  enum dpni_tx_schedule_mode, \
				(cfg)->tc_sched[5].mode); \
	MC_CMD_OP(cmd, 3, 0,  16,  uint16_t, (cfg)->tc_sched[6].delta_bandwidth);\
	MC_CMD_OP(cmd, 3, 16,  4,  enum dpni_tx_schedule_mode, \
				(cfg)->tc_sched[6].mode); \
	MC_CMD_OP(cmd, 3, 32, 16,  uint16_t, (cfg)->tc_sched[7].delta_bandwidth);\
	MC_CMD_OP(cmd, 3, 48, 4,  enum dpni_tx_schedule_mode, \
				(cfg)->tc_sched[7].mode); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_RX_TC_DIST(cmd, tc_id, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 0,  16, uint16_t,  cfg->dist_size); \
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  tc_id); \
	MC_CMD_OP(cmd, 0, 24, 4,  enum dpni_dist_mode, cfg->dist_mode); \
	MC_CMD_OP(cmd, 0, 28, 4,  enum dpni_fs_miss_action, \
						  cfg->fs_cfg.miss_action); \
	MC_CMD_OP(cmd, 0, 48, 16, uint16_t, cfg->fs_cfg.default_flow_id); \
	MC_CMD_OP(cmd, 6, 0,  64, uint64_t, cfg->key_cfg_iova); \
} while (0)

#define DPNI_CMD_GET_QUEUE(cmd, qtype, tc, index) \
do { \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_queue_type, qtype); \
	MC_CMD_OP(cmd, 0,  8,  8,  uint8_t, tc); \
	MC_CMD_OP(cmd, 0, 16,  8,  uint8_t, index); \
} while (0)

#define DPNI_RSP_GET_QUEUE(cmd, queue, queue_id) \
do { \
	MC_RSP_OP(cmd, 1,  0, 32, uint32_t, (queue)->destination.id); \
	MC_RSP_OP(cmd, 1, 48,  8, uint8_t, (queue)->destination.priority); \
	MC_RSP_OP(cmd, 1, 56,  4, enum dpni_dest, (queue)->destination.type); \
	MC_RSP_OP(cmd, 1, 62,  1, char, (queue)->flc.stash_control); \
	MC_RSP_OP(cmd, 1, 63,  1, char, (queue)->destination.hold_active); \
	MC_RSP_OP(cmd, 2,  0, 64, uint64_t, (queue)->flc.value); \
	MC_RSP_OP(cmd, 3,  0, 64, uint64_t, (queue)->user_context); \
	MC_RSP_OP(cmd, 4,  0, 32, uint32_t, (queue_id)->fqid); \
	MC_RSP_OP(cmd, 4, 32, 16, uint16_t, (queue_id)->qdbin); \
} while (0)

#define DPNI_CMD_SET_QUEUE(cmd, qtype, tc, index, options, queue) \
do { \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_queue_type, qtype); \
	MC_CMD_OP(cmd, 0,  8,  8,  uint8_t, tc); \
	MC_CMD_OP(cmd, 0, 16,  8,  uint8_t, index); \
	MC_CMD_OP(cmd, 0, 24,  8,  uint8_t, options); \
	MC_CMD_OP(cmd, 1,  0, 32, uint32_t, (queue)->destination.id); \
	MC_CMD_OP(cmd, 1, 48,  8, uint8_t, (queue)->destination.priority); \
	MC_CMD_OP(cmd, 1, 56,  4, enum dpni_dest, (queue)->destination.type); \
	MC_CMD_OP(cmd, 1, 62,  1, char, (queue)->flc.stash_control); \
	MC_CMD_OP(cmd, 1, 63,  1, char, (queue)->destination.hold_active); \
	MC_CMD_OP(cmd, 2,  0, 64, uint64_t, (queue)->flc.value); \
	MC_CMD_OP(cmd, 3,  0, 64, uint64_t, (queue)->user_context); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_QOS_TABLE(cmd, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 32, 8,  uint8_t,  cfg->default_tc); \
	MC_CMD_OP(cmd, 0, 40, 1,  int,	    cfg->discard_on_miss); \
	MC_CMD_OP(cmd, 6, 0,  64, uint64_t, cfg->key_cfg_iova); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_ADD_QOS_ENTRY(cmd, cfg, tc_id, index) \
do { \
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  tc_id); \
	MC_CMD_OP(cmd, 0, 24, 8,  uint8_t,  cfg->key_size); \
	MC_CMD_OP(cmd, 0, 32, 16, uint16_t, index); \
	MC_CMD_OP(cmd, 1, 0,  64, uint64_t, cfg->key_iova); \
	MC_CMD_OP(cmd, 2, 0,  64, uint64_t, cfg->mask_iova); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_REMOVE_QOS_ENTRY(cmd, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 24, 8,  uint8_t,  cfg->key_size); \
	MC_CMD_OP(cmd, 1, 0,  64, uint64_t, cfg->key_iova); \
	MC_CMD_OP(cmd, 2, 0,  64, uint64_t, cfg->mask_iova); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_ADD_FS_ENTRY(cmd, cfg, tc_id, index, action) \
do { \
	MC_CMD_OP(cmd, 0,  0, 16, uint16_t, (action)->options); \
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  tc_id); \
	MC_CMD_OP(cmd, 0, 48, 16, uint16_t, (action)->flow_id); \
	MC_CMD_OP(cmd, 0, 24, 8,  uint8_t,  (cfg)->key_size); \
	MC_CMD_OP(cmd, 0, 32, 16, uint16_t, index); \
	MC_CMD_OP(cmd, 1, 0,  64, uint64_t, (cfg)->key_iova); \
	MC_CMD_OP(cmd, 2, 0,  64, uint64_t, (cfg)->mask_iova); \
	MC_CMD_OP(cmd, 3, 0, 64, uint64_t, (action)->flc); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_REMOVE_FS_ENTRY(cmd, tc_id, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  tc_id); \
	MC_CMD_OP(cmd, 0, 24, 8,  uint8_t,  cfg->key_size); \
	MC_CMD_OP(cmd, 1, 0,  64, uint64_t, cfg->key_iova); \
	MC_CMD_OP(cmd, 2, 0,  64, uint64_t, cfg->mask_iova); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_CLEAR_FS_ENTRIES(cmd, tc_id) \
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  tc_id)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_SET_RX_TC_POLICING(cmd, tc_id, cfg) \
do { \
	MC_CMD_OP(cmd, 0, 0,  4, enum dpni_policer_mode, cfg->mode); \
	MC_CMD_OP(cmd, 0, 4,  4, enum dpni_policer_color, cfg->default_color); \
	MC_CMD_OP(cmd, 0, 8,  4, enum dpni_policer_unit, cfg->units); \
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  tc_id); \
	MC_CMD_OP(cmd, 0, 32, 32, uint32_t, cfg->options); \
	MC_CMD_OP(cmd, 1, 0,  32, uint32_t, cfg->cir); \
	MC_CMD_OP(cmd, 1, 32, 32, uint32_t, cfg->cbs); \
	MC_CMD_OP(cmd, 2, 0,  32, uint32_t, cfg->eir); \
	MC_CMD_OP(cmd, 2, 32, 32, uint32_t, cfg->ebs);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_GET_RX_TC_POLICING(cmd, tc_id) \
	MC_CMD_OP(cmd, 0, 16, 8,  uint8_t,  tc_id)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_RSP_GET_RX_TC_POLICING(cmd, cfg) \
do { \
	MC_RSP_OP(cmd, 0, 0,  4, enum dpni_policer_mode, cfg->mode); \
	MC_RSP_OP(cmd, 0, 4,  4, enum dpni_policer_color, cfg->default_color); \
	MC_RSP_OP(cmd, 0, 8,  4, enum dpni_policer_unit, cfg->units); \
	MC_RSP_OP(cmd, 0, 32, 32, uint32_t, cfg->options); \
	MC_RSP_OP(cmd, 1, 0,  32, uint32_t, cfg->cir); \
	MC_RSP_OP(cmd, 1, 32, 32, uint32_t, cfg->cbs); \
	MC_RSP_OP(cmd, 2, 0,  32, uint32_t, cfg->eir); \
	MC_RSP_OP(cmd, 2, 32, 32, uint32_t, cfg->ebs);\
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_PREP_EARLY_DROP(ext, cfg) \
do { \
	MC_PREP_OP(ext, 0, 0,  2, enum dpni_early_drop_mode, (cfg)->mode); \
	MC_PREP_OP(ext, 0, 2,  2, \
		  enum dpni_congestion_unit, (cfg)->units); \
	MC_PREP_OP(ext, 0, 32, 32, uint32_t, (cfg)->tail_drop_threshold); \
	MC_PREP_OP(ext, 1, 0,  8,  uint8_t,  (cfg)->green.drop_probability); \
	MC_PREP_OP(ext, 2, 0,  64, uint64_t, (cfg)->green.max_threshold); \
	MC_PREP_OP(ext, 3, 0,  64, uint64_t, (cfg)->green.min_threshold); \
	MC_PREP_OP(ext, 5, 0,  8,  uint8_t,  (cfg)->yellow.drop_probability);\
	MC_PREP_OP(ext, 6, 0,  64, uint64_t, (cfg)->yellow.max_threshold); \
	MC_PREP_OP(ext, 7, 0,  64, uint64_t, (cfg)->yellow.min_threshold); \
	MC_PREP_OP(ext, 9, 0,  8,  uint8_t,  (cfg)->red.drop_probability); \
	MC_PREP_OP(ext, 10, 0,  64, uint64_t, (cfg)->red.max_threshold); \
	MC_PREP_OP(ext, 11, 0,  64, uint64_t, (cfg)->red.min_threshold); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_EXT_EARLY_DROP(ext, cfg) \
do { \
	MC_EXT_OP(ext, 0, 0,  2, enum dpni_early_drop_mode, (cfg)->mode); \
	MC_EXT_OP(ext, 0, 2,  2, \
		  enum dpni_congestion_unit, (cfg)->units); \
	MC_EXT_OP(ext, 0, 32, 32, uint32_t, (cfg)->tail_drop_threshold); \
	MC_EXT_OP(ext, 1, 0,  8,  uint8_t,  (cfg)->green.drop_probability); \
	MC_EXT_OP(ext, 2, 0,  64, uint64_t, (cfg)->green.max_threshold); \
	MC_EXT_OP(ext, 3, 0,  64, uint64_t, (cfg)->green.min_threshold); \
	MC_EXT_OP(ext, 5, 0,  8,  uint8_t,  (cfg)->yellow.drop_probability);\
	MC_EXT_OP(ext, 6, 0,  64, uint64_t, (cfg)->yellow.max_threshold); \
	MC_EXT_OP(ext, 7, 0,  64, uint64_t, (cfg)->yellow.min_threshold); \
	MC_EXT_OP(ext, 9, 0,  8,  uint8_t,  (cfg)->red.drop_probability); \
	MC_EXT_OP(ext, 10, 0,  64, uint64_t, (cfg)->red.max_threshold); \
	MC_EXT_OP(ext, 11, 0,  64, uint64_t, (cfg)->red.min_threshold); \
} while (0)

#define DPNI_CMD_SET_EARLY_DROP(cmd, qtype, tc, early_drop_iova) \
do { \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_queue_type, qtype); \
	MC_CMD_OP(cmd, 0,  8,  8, uint8_t, tc); \
	MC_CMD_OP(cmd, 1,  0, 64, uint64_t, early_drop_iova); \
} while (0)

/*                cmd, param, offset, width, type, arg_name */
#define DPNI_CMD_GET_EARLY_DROP(cmd, qtype, tc, early_drop_iova) \
do { \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_queue_type, qtype); \
	MC_CMD_OP(cmd, 0,  8,  8, uint8_t, tc); \
	MC_CMD_OP(cmd, 1,  0, 64, uint64_t, early_drop_iova); \
} while (0)

#define DPNI_CMD_GET_TAILDROP(cmd, cp, q_type, tc, q_index) \
do { \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_congestion_point, cp); \
	MC_CMD_OP(cmd, 0,  8,  8, enum dpni_queue_type, q_type); \
	MC_CMD_OP(cmd, 0, 16,  8, uint8_t, tc); \
	MC_CMD_OP(cmd, 0, 24,  8, uint8_t, q_index); \
} while (0)

#define DPNI_RSP_GET_TAILDROP(cmd, taildrop) \
do { \
	MC_RSP_OP(cmd, 1,  0,  1, char, (taildrop)->enable); \
	MC_RSP_OP(cmd, 1, 16,  8, enum dpni_congestion_unit, (taildrop)->units); \
	MC_RSP_OP(cmd, 1, 32, 32, uint32_t, (taildrop)->threshold); \
} while (0)

#define DPNI_CMD_SET_TAILDROP(cmd, cp, q_type, tc, q_index, taildrop) \
do { \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_congestion_point, cp); \
	MC_CMD_OP(cmd, 0,  8,  8, enum dpni_queue_type, q_type); \
	MC_CMD_OP(cmd, 0, 16,  8, uint8_t, tc); \
	MC_CMD_OP(cmd, 0, 24,  8, uint8_t, q_index); \
	MC_CMD_OP(cmd, 1,  0,  1, char, (taildrop)->enable); \
	MC_CMD_OP(cmd, 1, 16,  8, enum dpni_congestion_unit, (taildrop)->units); \
	MC_CMD_OP(cmd, 1, 32, 32, uint32_t, (taildrop)->threshold); \
} while (0)

#define DPNI_CMD_SET_TX_CONFIRMATION_MODE(cmd, mode) \
	MC_CMD_OP(cmd, 0, 32, 8, enum dpni_confirmation_mode, mode)

#define DPNI_CMD_SET_CONGESTION_NOTIFICATION(cmd, qtype, tc, cfg) \
do { \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_queue_type, qtype); \
	MC_CMD_OP(cmd, 0,  8,  8, uint8_t, tc); \
	MC_CMD_OP(cmd, 1,  0, 32, uint32_t, (cfg)->dest_cfg.dest_id); \
	MC_CMD_OP(cmd, 1,  0, 16, uint16_t, (cfg)->notification_mode); \
	MC_CMD_OP(cmd, 1, 48,  8, uint8_t, (cfg)->dest_cfg.priority); \
	MC_CMD_OP(cmd, 1, 56,  4, enum dpni_dest, (cfg)->dest_cfg.dest_type); \
	MC_CMD_OP(cmd, 1, 60,  2, enum dpni_congestion_unit, (cfg)->units); \
	MC_CMD_OP(cmd, 2,  0, 64, uint64_t, (cfg)->message_iova); \
	MC_CMD_OP(cmd, 3,  0, 64, uint64_t, (cfg)->message_ctx); \
	MC_CMD_OP(cmd, 4,  0, 32, uint32_t, (cfg)->threshold_entry); \
	MC_CMD_OP(cmd, 4, 32, 32, uint32_t, (cfg)->threshold_exit); \
} while (0)

#define DPNI_CMD_GET_CONGESTION_NOTIFICATION(cmd, qtype, tc) \
do { \
	MC_CMD_OP(cmd, 0,  0,  8, enum dpni_queue_type, qtype); \
	MC_CMD_OP(cmd, 0,  8,  8, uint8_t, tc); \
} while (0)

#define DPNI_RSP_GET_CONGESTION_NOTIFICATION(cmd, cfg) \
do { \
	MC_RSP_OP(cmd, 1,  0, 32, uint32_t, (cfg)->dest_cfg.dest_id); \
	MC_RSP_OP(cmd, 1,  0, 16, uint16_t, (cfg)->notification_mode); \
	MC_RSP_OP(cmd, 1, 48,  8, uint8_t, (cfg)->dest_cfg.priority); \
	MC_RSP_OP(cmd, 1, 56,  4, enum dpni_dest, (cfg)->dest_cfg.dest_type); \
	MC_RSP_OP(cmd, 1, 60,  2, enum dpni_congestion_unit, (cfg)->units); \
	MC_RSP_OP(cmd, 2,  0, 64, uint64_t, (cfg)->message_iova); \
	MC_RSP_OP(cmd, 3,  0, 64, uint64_t, (cfg)->message_ctx); \
	MC_RSP_OP(cmd, 4,  0, 32, uint32_t, (cfg)->threshold_entry); \
	MC_RSP_OP(cmd, 4, 32, 32, uint32_t, (cfg)->threshold_exit); \
} while (0)

#endif /* _FSL_DPNI_CMD_H */
