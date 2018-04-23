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
#ifndef _FSL_DPBP_CMD_H
#define _FSL_DPBP_CMD_H

/* DPBP Version */
#define DPBP_VER_MAJOR				3
#define DPBP_VER_MINOR				2
#define DPBP_CMD_BASE_VERSION			1
#define DPBP_CMD_ID_OFFSET			4

/* Command IDs */
#define DPBP_CMDID_CLOSE                        ((0x800 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_OPEN                         ((0x804 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_CREATE                       ((0x904 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_DESTROY                      ((0x984 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_GET_API_VERSION              ((0xa04 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)

#define DPBP_CMDID_ENABLE                       ((0x002 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_DISABLE                      ((0x003 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_GET_ATTR                     ((0x004 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_RESET                        ((0x005 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_IS_ENABLED                   ((0x006 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)

#define DPBP_CMDID_SET_IRQ                      ((0x010 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_GET_IRQ                      ((0x011 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_SET_IRQ_ENABLE               ((0x012 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_GET_IRQ_ENABLE               ((0x013 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_SET_IRQ_MASK                 ((0x014 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_GET_IRQ_MASK                 ((0x015 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_GET_IRQ_STATUS               ((0x016 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_CLEAR_IRQ_STATUS             ((0x017 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)

#define DPBP_CMDID_SET_NOTIFICATIONS           ((0x01b0 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)
#define DPBP_CMDID_GET_NOTIFICATIONS           ((0x01b1 << DPBP_CMD_ID_OFFSET) | DPBP_CMD_BASE_VERSION)

#endif /* _FSL_DPBP_CMD_H */
