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
#ifndef _FSL_DPDMUX_CMD_H
#define _FSL_DPDMUX_CMD_H

/* DPDMUX Version */
#define DPDMUX_VER_MAJOR				6
#define DPDMUX_VER_MINOR				0

#define DPDMUX_CMD_BASE_VER				1
#define DPDMUX_CMD_ID_OFF				4
#define DPDMUX_CMD_ID(id) (((id) << DPDMUX_CMD_ID_OFF) | DPDMUX_CMD_BASE_VER)

/* Command IDs */
#define DPDMUX_CMDID_CLOSE                   DPDMUX_CMD_ID(0x800)
#define DPDMUX_CMDID_OPEN                    DPDMUX_CMD_ID(0x806)
#define DPDMUX_CMDID_CREATE                  DPDMUX_CMD_ID(0x906)
#define DPDMUX_CMDID_DESTROY                 DPDMUX_CMD_ID(0x900)
#define DPDMUX_CMDID_GET_API_VERSION         DPDMUX_CMD_ID(0xa06)

#define DPDMUX_CMDID_ENABLE                  DPDMUX_CMD_ID(0x002)
#define DPDMUX_CMDID_DISABLE                 DPDMUX_CMD_ID(0x003)
#define DPDMUX_CMDID_GET_ATTR                DPDMUX_CMD_ID(0x004)
#define DPDMUX_CMDID_RESET                   DPDMUX_CMD_ID(0x005)
#define DPDMUX_CMDID_IS_ENABLED              DPDMUX_CMD_ID(0x006)

#define DPDMUX_CMDID_SET_IRQ                 DPDMUX_CMD_ID(0x010)
#define DPDMUX_CMDID_GET_IRQ                 DPDMUX_CMD_ID(0x011)
#define DPDMUX_CMDID_SET_IRQ_ENABLE          DPDMUX_CMD_ID(0x012)
#define DPDMUX_CMDID_GET_IRQ_ENABLE          DPDMUX_CMD_ID(0x013)
#define DPDMUX_CMDID_SET_IRQ_MASK            DPDMUX_CMD_ID(0x014)
#define DPDMUX_CMDID_GET_IRQ_MASK            DPDMUX_CMD_ID(0x015)
#define DPDMUX_CMDID_GET_IRQ_STATUS          DPDMUX_CMD_ID(0x016)
#define DPDMUX_CMDID_CLEAR_IRQ_STATUS        DPDMUX_CMD_ID(0x017)

#define DPDMUX_CMDID_SET_MAX_FRAME_LENGTH    DPDMUX_CMD_ID(0x0a1)

#define DPDMUX_CMDID_UL_RESET_COUNTERS       DPDMUX_CMD_ID(0x0a3)

#define DPDMUX_CMDID_IF_SET_ACCEPTED_FRAMES  DPDMUX_CMD_ID(0x0a7)
#define DPDMUX_CMDID_IF_GET_ATTR             DPDMUX_CMD_ID(0x0a8)

#define DPDMUX_CMDID_IF_ADD_L2_RULE          DPDMUX_CMD_ID(0x0b0)
#define DPDMUX_CMDID_IF_REMOVE_L2_RULE       DPDMUX_CMD_ID(0x0b1)
#define DPDMUX_CMDID_IF_GET_COUNTER          DPDMUX_CMD_ID(0x0b2)
#define DPDMUX_CMDID_IF_SET_LINK_CFG         DPDMUX_CMD_ID(0x0b3)
#define DPDMUX_CMDID_IF_GET_LINK_STATE       DPDMUX_CMD_ID(0x0b4)

#define DPDMUX_MASK(field)        \
	GENMASK(DPDMUX_##field##_SHIFT + DPDMUX_##field##_SIZE - 1, \
		DPDMUX_##field##_SHIFT)
#define dpdmux_set_field(var, field, val) \
	((var) |= (((val) << DPDMUX_##field##_SHIFT) & DPDMUX_MASK(field)))
#define dpdmux_get_field(var, field)      \
	(((var) & DPDMUX_MASK(field)) >> DPDMUX_##field##_SHIFT)

struct dpdmux_cmd_open {
	__le32 dpdmux_id;
};

struct dpdmux_cmd_create {
	u8 method;
	u8 manip;
	__le16 num_ifs;
	__le32 pad;

	__le16 adv_max_dmat_entries;
	__le16 adv_max_mc_groups;
	__le16 adv_max_vlan_ids;
	__le16 pad1;

	__le64 options;
};

struct dpdmux_cmd_destroy {
	__le32 dpdmux_id;
};

#define DPDMUX_ENABLE_SHIFT	0
#define DPDMUX_ENABLE_SIZE	1

struct dpdmux_rsp_is_enabled {
	u8 en;
};

struct dpdmux_cmd_set_irq {
	u8 irq_index;
	u8 pad[3];
	__le32 irq_val;
	__le64 irq_addr;
	__le32 irq_num;
};

struct dpdmux_cmd_get_irq {
	__le32 pad;
	u8 irq_index;
};

struct dpdmux_rsp_get_irq {
	__le32 irq_val;
	__le32 pad;
	__le64 irq_addr;
	__le32 irq_num;
	__le32 type;
};

struct dpdmux_cmd_set_irq_enable {
	u8 enable;
	u8 pad[3];
	u8 irq_index;
};

struct dpdmux_cmd_get_irq_enable {
	__le32 pad;
	u8 irq_index;
};

struct dpdmux_rsp_get_irq_enable {
	u8 enable;
};

struct dpdmux_cmd_set_irq_mask {
	__le32 mask;
	u8 irq_index;
};

struct dpdmux_cmd_get_irq_mask {
	__le32 pad;
	u8 irq_index;
};

struct dpdmux_rsp_get_irq_mask {
	__le32 mask;
};

struct dpdmux_cmd_get_irq_status {
	__le32 status;
	u8 irq_index;
};

struct dpdmux_rsp_get_irq_status {
	__le32 status;
};

struct dpdmux_cmd_clear_irq_status {
	__le32 status;
	u8 irq_index;
};

struct dpdmux_rsp_get_attr {
	u8 method;
	u8 manip;
	__le16 num_ifs;
	__le16 mem_size;
	__le16 pad;

	__le64 pad1;

	__le32 id;
	__le32 pad2;

	__le64 options;
};

struct dpdmux_cmd_set_max_frame_length {
	__le16 max_frame_length;
};

#define DPDMUX_ACCEPTED_FRAMES_TYPE_SHIFT	0
#define DPDMUX_ACCEPTED_FRAMES_TYPE_SIZE	4
#define DPDMUX_UNACCEPTED_FRAMES_ACTION_SHIFT	4
#define DPDMUX_UNACCEPTED_FRAMES_ACTION_SIZE	4

struct dpdmux_cmd_if_set_accepted_frames {
	__le16 if_id;
	u8 frames_options;
};

struct dpdmux_cmd_if_get_attr {
	__le16 if_id;
};

struct dpdmux_rsp_if_get_attr {
	u8 pad[3];
	u8 enabled;
	u8 pad1[3];
	u8 accepted_frames_type;
	__le32 rate;
};

struct dpdmux_cmd_if_l2_rule {
	__le16 if_id;
	u8 mac_addr5;
	u8 mac_addr4;
	u8 mac_addr3;
	u8 mac_addr2;
	u8 mac_addr1;
	u8 mac_addr0;

	__le32 pad;
	__le16 vlan_id;
};

struct dpdmux_cmd_if_get_counter {
	__le16 if_id;
	u8 counter_type;
};

struct dpdmux_rsp_if_get_counter {
	__le64 pad;
	__le64 counter;
};

struct dpdmux_cmd_if_set_link_cfg {
	__le16 if_id;
	__le16 pad[3];

	__le32 rate;
	__le32 pad1;

	__le64 options;
};

struct dpdmux_cmd_if_get_link_state {
	__le16 if_id;
};

struct dpdmux_rsp_if_get_link_state {
	__le32 pad;
	u8 up;
	u8 pad1[3];

	__le32 rate;
	__le32 pad2;

	__le64 options;
};

struct dpdmux_rsp_get_api_version {
	__le16 major;
	__le16 minor;
};

#endif /* _FSL_DPDMUX_CMD_H */
