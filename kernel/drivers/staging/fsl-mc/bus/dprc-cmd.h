/* Copyright 2013-2016 Freescale Semiconductor Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the above-listed copyright holders nor the
 *       names of any contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
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

/*************************************************************************//*
 dprc-cmd.h

 defines dprc portal commands

 *//**************************************************************************/

#ifndef _FSL_DPRC_CMD_H
#define _FSL_DPRC_CMD_H

/* Minimal supported DPRC Version */
#define DPRC_MIN_VER_MAJOR			6
#define DPRC_MIN_VER_MINOR			0
#define DPRC_CMD_BASE_VERSION			1
#define DPRC_CMD_ID_OFFSET			4

/* Command IDs */
#define DPRC_CMDID_CLOSE                        ((0x800 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_OPEN                         ((0x805 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_CREATE                       ((0x905 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_VERSION                  ((0xa05 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)

#define DPRC_CMDID_GET_ATTR                     ((0x004 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_RESET_CONT                   ((0x005 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)

#define DPRC_CMDID_SET_IRQ                      ((0x010 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_IRQ                      ((0x011 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_SET_IRQ_ENABLE               ((0x012 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_IRQ_ENABLE               ((0x013 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_SET_IRQ_MASK                 ((0x014 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_IRQ_MASK                 ((0x015 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_IRQ_STATUS               ((0x016 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_CLEAR_IRQ_STATUS             ((0x017 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)

#define DPRC_CMDID_CREATE_CONT                  ((0x151 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_DESTROY_CONT                 ((0x152 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_CONT_ID                  ((0x830 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_SET_RES_QUOTA                ((0x155 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_RES_QUOTA                ((0x156 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_ASSIGN                       ((0x157 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_UNASSIGN                     ((0x158 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_OBJ_COUNT                ((0x159 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_OBJ                      ((0x15A << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_RES_COUNT                ((0x15B << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_RES_IDS                  ((0x15C << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_OBJ_REG                  ((0x15E << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_SET_OBJ_IRQ                  ((0x15F << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_OBJ_IRQ                  ((0x160 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_SET_OBJ_LABEL                ((0x161 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_OBJ_DESC                 ((0x162 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)

#define DPRC_CMDID_CONNECT                      ((0x167 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_DISCONNECT                   ((0x168 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_POOL                     ((0x169 << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)
#define DPRC_CMDID_GET_POOL_COUNT               ((0x16A << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)

#define DPRC_CMDID_GET_CONNECTION               ((0x16C << DPRC_CMD_ID_OFFSET) | DPRC_CMD_BASE_VERSION)

#endif /* _FSL_DPRC_CMD_H */
