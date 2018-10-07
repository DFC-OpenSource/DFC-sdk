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
#ifndef __FSL_DPMAC_H
#define __FSL_DPMAC_H

/* Data Path MAC API
 * Contains initialization APIs and runtime control APIs for DPMAC
 */

struct fsl_mc_io;

/**
 * dpmac_open() - Open a control session for the specified object.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @dpmac_id:	DPMAC unique ID
 * @token:	Returned token; use in subsequent API calls
 *
 * This function can be used to open a control session for an
 * already created object; an object may have been declared in
 * the DPL or by calling the dpmac_create function.
 * This function returns a unique authentication token,
 * associated with the specific object ID and the specific MC
 * portal; this token must be used in all subsequent commands for
 * this specific object
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_open(struct fsl_mc_io	*mc_io,
	       uint32_t		cmd_flags,
	       int			dpmac_id,
	       uint16_t		*token);

/**
 * dpmac_close() - Close the control session of the object
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 *
 * After this function is called, no further operations are
 * allowed on the object without opening a new control session.
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_close(struct fsl_mc_io	*mc_io,
		uint32_t		cmd_flags,
		uint16_t		token);

/**
 * enum dpmac_link_type -  DPMAC link type
 * @DPMAC_LINK_TYPE_NONE: No link
 * @DPMAC_LINK_TYPE_FIXED: Link is fixed type
 * @DPMAC_LINK_TYPE_PHY: Link by PHY ID
 * @DPMAC_LINK_TYPE_BACKPLANE: Backplane link type
 */
enum dpmac_link_type {
	DPMAC_LINK_TYPE_NONE,
	DPMAC_LINK_TYPE_FIXED,
	DPMAC_LINK_TYPE_PHY,
	DPMAC_LINK_TYPE_BACKPLANE
};

/**
 * enum dpmac_eth_if - DPMAC Ethrnet interface
 * @DPMAC_ETH_IF_MII: MII interface
 * @DPMAC_ETH_IF_RMII: RMII interface
 * @DPMAC_ETH_IF_SMII: SMII interface
 * @DPMAC_ETH_IF_GMII: GMII interface
 * @DPMAC_ETH_IF_RGMII: RGMII interface
 * @DPMAC_ETH_IF_SGMII: SGMII interface
 * @DPMAC_ETH_IF_QSGMII: QSGMII interface
 * @DPMAC_ETH_IF_XAUI: XAUI interface
 * @DPMAC_ETH_IF_XFI: XFI interface
 */
enum dpmac_eth_if {
	DPMAC_ETH_IF_MII,
	DPMAC_ETH_IF_RMII,
	DPMAC_ETH_IF_SMII,
	DPMAC_ETH_IF_GMII,
	DPMAC_ETH_IF_RGMII,
	DPMAC_ETH_IF_SGMII,
	DPMAC_ETH_IF_QSGMII,
	DPMAC_ETH_IF_XAUI,
	DPMAC_ETH_IF_XFI
};

/**
 * struct dpmac_cfg - Structure representing DPMAC configuration
 * @mac_id:	Represents the Hardware MAC ID; in case of multiple WRIOP,
 *		the MAC IDs are continuous.
 *		For example:  2 WRIOPs, 16 MACs in each:
 *				MAC IDs for the 1st WRIOP: 1-16,
 *				MAC IDs for the 2nd WRIOP: 17-32.
 */
struct dpmac_cfg {
	uint16_t mac_id;
};

/**
 * dpmac_create() - Create the DPMAC object.
 * @mc_io:	Pointer to MC portal's I/O object
 * @dprc_token: Parent container token; '0' for default container
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @cfg:	Configuration structure
 * @obj_id: returned object id
 *
 * Create the DPMAC object, allocate required resources and
 * perform required initialization.
 *
 * The function accepts an authentication token of a parent
 * container that this object should be assigned to. The token
 * can be '0' so the object will be assigned to the default container.
 * The newly created object can be opened with the returned
 * object id and using the container's associated tokens and MC portals.
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_create(struct fsl_mc_io	*mc_io,
		uint16_t	dprc_token,
		uint32_t	cmd_flags,
		const struct dpmac_cfg	*cfg,
		uint32_t	*obj_id);

/**
 * dpmac_destroy() - Destroy the DPMAC object and release all its resources.
 * @mc_io:	Pointer to MC portal's I/O object
 * @dprc_token: Parent container token; '0' for default container
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @object_id:	The object id; it must be a valid id within the container that
 * created this object;
 *
 * The function accepts the authentication token of the parent container that
 * created the object (not the one that currently owns the object). The object
 * is searched within parent using the provided 'object_id'.
 * All tokens to the object must be closed before calling destroy.
 *
 * Return:	'0' on Success; error code otherwise.
 */
int dpmac_destroy(struct fsl_mc_io	*mc_io,
		uint16_t	dprc_token,
		uint32_t	cmd_flags,
		uint32_t	object_id);

/**
 * DPMAC IRQ Index and Events
 */

/**
 * IRQ index
 */
#define DPMAC_IRQ_INDEX						0
/**
 * IRQ event - indicates a change in link state
 */
#define DPMAC_IRQ_EVENT_LINK_CFG_REQ		0x00000001
/**
 * IRQ event - Indicates that the link state changed
 */
#define DPMAC_IRQ_EVENT_LINK_CHANGED		0x00000002

/**
 * struct dpmac_irq_cfg - IRQ configuration
 * @addr:	Address that must be written to signal a message-based interrupt
 * @val:	Value to write into irq_addr address
 * @irq_num: A user defined number associated with this IRQ
 */
struct dpmac_irq_cfg {
	     uint64_t		addr;
	     uint32_t		val;
	     int		irq_num;
};

/**
 * dpmac_set_irq() - Set IRQ information for the DPMAC to trigger an interrupt.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @irq_index:	Identifies the interrupt index to configure
 * @irq_cfg:	IRQ configuration
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_set_irq(struct fsl_mc_io	*mc_io,
		  uint32_t		cmd_flags,
		  uint16_t		token,
		  uint8_t		irq_index,
		  struct dpmac_irq_cfg	*irq_cfg);

/**
 * dpmac_get_irq() - Get IRQ information from the DPMAC.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @irq_index:	The interrupt index to configure
 * @type:	Interrupt type: 0 represents message interrupt
 *		type (both irq_addr and irq_val are valid)
 * @irq_cfg:	IRQ attributes
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_get_irq(struct fsl_mc_io	*mc_io,
		  uint32_t		cmd_flags,
		  uint16_t		token,
		  uint8_t		irq_index,
		  int			*type,
		  struct dpmac_irq_cfg	*irq_cfg);

/**
 * dpmac_set_irq_enable() - Set overall interrupt state.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @irq_index:	The interrupt index to configure
 * @en:	Interrupt state - enable = 1, disable = 0
 *
 * Allows GPP software to control when interrupts are generated.
 * Each interrupt can have up to 32 causes.  The enable/disable control's the
 * overall interrupt state. if the interrupt is disabled no causes will cause
 * an interrupt.
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_set_irq_enable(struct fsl_mc_io	*mc_io,
			 uint32_t		cmd_flags,
			 uint16_t		token,
			 uint8_t		irq_index,
			 uint8_t		en);

/**
 * dpmac_get_irq_enable() - Get overall interrupt state
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @irq_index:	The interrupt index to configure
 * @en:		Returned interrupt state - enable = 1, disable = 0
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_get_irq_enable(struct fsl_mc_io	*mc_io,
			 uint32_t		cmd_flags,
			 uint16_t		token,
			 uint8_t		irq_index,
			 uint8_t		*en);

/**
 * dpmac_set_irq_mask() - Set interrupt mask.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @irq_index:	The interrupt index to configure
 * @mask:	Event mask to trigger interrupt;
 *			each bit:
 *				0 = ignore event
 *				1 = consider event for asserting IRQ
 *
 * Every interrupt can have up to 32 causes and the interrupt model supports
 * masking/unmasking each cause independently
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_set_irq_mask(struct fsl_mc_io *mc_io,
		       uint32_t	cmd_flags,
		       uint16_t		token,
		       uint8_t		irq_index,
		       uint32_t		mask);

/**
 * dpmac_get_irq_mask() - Get interrupt mask.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @irq_index:	The interrupt index to configure
 * @mask:	Returned event mask to trigger interrupt
 *
 * Every interrupt can have up to 32 causes and the interrupt model supports
 * masking/unmasking each cause independently
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_get_irq_mask(struct fsl_mc_io *mc_io,
		       uint32_t	cmd_flags,
		       uint16_t		token,
		       uint8_t		irq_index,
		       uint32_t		*mask);

/**
 * dpmac_get_irq_status() - Get the current status of any pending interrupts.
 *
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @irq_index:	The interrupt index to configure
 * @status:	Returned interrupts status - one bit per cause:
 *			0 = no interrupt pending
 *			1 = interrupt pending
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_get_irq_status(struct fsl_mc_io	*mc_io,
			 uint32_t		cmd_flags,
			 uint16_t		token,
			 uint8_t		irq_index,
			 uint32_t		*status);

/**
 * dpmac_clear_irq_status() - Clear a pending interrupt's status
 *
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @irq_index:	The interrupt index to configure
 * @status:	Bits to clear (W1C) - one bit per cause:
 *					0 = don't change
 *					1 = clear status bit
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_clear_irq_status(struct fsl_mc_io	*mc_io,
			   uint32_t		cmd_flags,
			   uint16_t		token,
			   uint8_t		irq_index,
			   uint32_t		status);

/**
 * struct dpmac_attr - Structure representing DPMAC attributes
 * @id: DPMAC object ID
 * @max_rate: Maximum supported rate - in Mbps
 * @eth_if: Ethernet interface
 * @link_type: link type
 */
struct dpmac_attr {
	uint16_t				id;
	uint32_t				max_rate;
	enum dpmac_eth_if		eth_if;
	enum dpmac_link_type	link_type;
};

/**
 * dpmac_get_attributes - Retrieve DPMAC attributes.
 *
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @attr:	Returned object's attributes
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_get_attributes(struct fsl_mc_io	*mc_io,
			 uint32_t		cmd_flags,
			 uint16_t		token,
			 struct dpmac_attr	*attr);

/**
 * DPMAC link configuration/state options
 */

/**
 * Enable auto-negotiation
 */
#define DPMAC_LINK_OPT_AUTONEG		0x0000000000000001ULL
/**
 * Enable half-duplex mode
 */
#define DPMAC_LINK_OPT_HALF_DUPLEX	0x0000000000000002ULL
/**
 * Enable pause frames
 */
#define DPMAC_LINK_OPT_PAUSE		0x0000000000000004ULL
/**
 * Enable a-symmetric pause frames
 */
#define DPMAC_LINK_OPT_ASYM_PAUSE	0x0000000000000008ULL

/**
 * struct dpmac_link_cfg - Structure representing DPMAC link configuration
 * @rate: Link's rate - in Mbps
 * @options: Enable/Disable DPMAC link cfg features (bitmap)
 */
struct dpmac_link_cfg {
	uint32_t rate;
	uint64_t options;
};

/**
 * dpmac_get_link_cfg() - Get Ethernet link configuration
 * @mc_io:	Pointer to opaque I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @cfg:	Returned structure with the link configuration
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_get_link_cfg(struct fsl_mc_io	*mc_io,
		       uint32_t		cmd_flags,
		       uint16_t		token,
		       struct dpmac_link_cfg	*cfg);

/**
 * struct dpmac_link_state - DPMAC link configuration request
 * @rate: Rate in Mbps
 * @options: Enable/Disable DPMAC link cfg features (bitmap)
 * @up: Link state
 */
struct dpmac_link_state {
	uint32_t	rate;
	uint64_t	options;
	int		up;
};

/**
 * dpmac_set_link_state() - Set the Ethernet link status
 * @mc_io:	Pointer to opaque I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @link_state:	Link state configuration
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmac_set_link_state(struct fsl_mc_io		*mc_io,
			 uint32_t			cmd_flags,
			 uint16_t			token,
			 struct dpmac_link_state	*link_state);

/**
 * enum dpmac_counter - DPMAC counter types
 * @DPMAC_CNT_ING_FRAME_64: counts 64-bytes frames, good or bad.
 * @DPMAC_CNT_ING_FRAME_127: counts 65- to 127-bytes frames, good or bad.
 * @DPMAC_CNT_ING_FRAME_255: counts 128- to 255-bytes frames, good or bad.
 * @DPMAC_CNT_ING_FRAME_511: counts 256- to 511-bytes frames, good or bad.
 * @DPMAC_CNT_ING_FRAME_1023: counts 512- to 1023-bytes frames, good or bad.
 * @DPMAC_CNT_ING_FRAME_1518: counts 1024- to 1518-bytes frames, good or bad.
 * @DPMAC_CNT_ING_FRAME_1519_MAX: counts 1519-bytes frames and larger
 *				  (up to max frame length specified),
 *				  good or bad.
 * @DPMAC_CNT_ING_FRAG: counts frames which are shorter than 64 bytes received
 *			with a wrong CRC
 * @DPMAC_CNT_ING_JABBER: counts frames longer than the maximum frame length
 *			  specified, with a bad frame check sequence.
 * @DPMAC_CNT_ING_FRAME_DISCARD: counts dropped frames due to internal errors.
 *				 Occurs when a receive FIFO overflows.
 *				 Includes also frames truncated as a result of
 *				 the receive FIFO overflow.
 * @DPMAC_CNT_ING_ALIGN_ERR: counts frames with an alignment error
 *			     (optional used for wrong SFD).
 * @DPMAC_CNT_EGR_UNDERSIZED: counts frames transmitted that was less than 64
 *			      bytes long with a good CRC.
 * @DPMAC_CNT_ING_OVERSIZED: counts frames longer than the maximum frame length
 *			     specified, with a good frame check sequence.
 * @DPMAC_CNT_ING_VALID_PAUSE_FRAME: counts valid pause frames (regular and PFC).
 * @DPMAC_CNT_EGR_VALID_PAUSE_FRAME: counts valid pause frames transmitted
 *				     (regular and PFC).
 * @DPMAC_CNT_ING_BYTE: counts bytes received except preamble for all valid
 *			frames and valid pause frames.
 * @DPMAC_CNT_ING_MCAST_FRAME: counts received multicast frames.
 * @DPMAC_CNT_ING_BCAST_FRAME: counts received broadcast frames.
 * @DPMAC_CNT_ING_ALL_FRAME: counts each good or bad frames received.
 * @DPMAC_CNT_ING_UCAST_FRAME: counts received unicast frames.
 * @DPMAC_CNT_ING_ERR_FRAME: counts frames received with an error
 *			     (except for undersized/fragment frame).
 * @DPMAC_CNT_EGR_BYTE: counts bytes transmitted except preamble for all valid
 *			frames and valid pause frames transmitted.
 * @DPMAC_CNT_EGR_MCAST_FRAME: counts transmitted multicast frames.
 * @DPMAC_CNT_EGR_BCAST_FRAME: counts transmitted broadcast frames.
 * @DPMAC_CNT_EGR_UCAST_FRAME: counts transmitted unicast frames.
 * @DPMAC_CNT_EGR_ERR_FRAME: counts frames transmitted with an error.
 * @DPMAC_CNT_ING_GOOD_FRAME: counts frames received without error, including
 *			      pause frames.
 * @DPMAC_CNT_ENG_GOOD_FRAME: counts frames transmitted without error, including
 *			      pause frames.
 */
enum dpmac_counter {
	DPMAC_CNT_ING_FRAME_64,
	DPMAC_CNT_ING_FRAME_127,
	DPMAC_CNT_ING_FRAME_255,
	DPMAC_CNT_ING_FRAME_511,
	DPMAC_CNT_ING_FRAME_1023,
	DPMAC_CNT_ING_FRAME_1518,
	DPMAC_CNT_ING_FRAME_1519_MAX,
	DPMAC_CNT_ING_FRAG,
	DPMAC_CNT_ING_JABBER,
	DPMAC_CNT_ING_FRAME_DISCARD,
	DPMAC_CNT_ING_ALIGN_ERR,
	DPMAC_CNT_EGR_UNDERSIZED,
	DPMAC_CNT_ING_OVERSIZED,
	DPMAC_CNT_ING_VALID_PAUSE_FRAME,
	DPMAC_CNT_EGR_VALID_PAUSE_FRAME,
	DPMAC_CNT_ING_BYTE,
	DPMAC_CNT_ING_MCAST_FRAME,
	DPMAC_CNT_ING_BCAST_FRAME,
	DPMAC_CNT_ING_ALL_FRAME,
	DPMAC_CNT_ING_UCAST_FRAME,
	DPMAC_CNT_ING_ERR_FRAME,
	DPMAC_CNT_EGR_BYTE,
	DPMAC_CNT_EGR_MCAST_FRAME,
	DPMAC_CNT_EGR_BCAST_FRAME,
	DPMAC_CNT_EGR_UCAST_FRAME,
	DPMAC_CNT_EGR_ERR_FRAME,
	DPMAC_CNT_ING_GOOD_FRAME,
	DPMAC_CNT_ENG_GOOD_FRAME
};

/**
 * dpmac_get_counter() - Read a specific DPMAC counter
 * @mc_io:	Pointer to opaque I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @type:	The requested counter
 * @counter:	Returned counter value
 *
 * Return:	The requested counter; '0' otherwise.
 */
int dpmac_get_counter(struct fsl_mc_io		*mc_io,
		      uint32_t			cmd_flags,
		      uint16_t			token,
		      enum dpmac_counter	 type,
		      uint64_t			*counter);

/**
 * dpmac_set_port_mac_addr() - Set a MAC address associated with the physical
 *              port.  This is not used for filtering, MAC is always in
 *              promiscuous mode, it is passed to DPNIs through DPNI API for
 *              application used.
 * @mc_io:	Pointer to opaque I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMAC object
 * @addr:	MAC address to set
 *
 * Return:	The requested counter; '0' otherwise.
 */
int dpmac_set_port_mac_addr(struct fsl_mc_io *mc_io,
		      uint32_t cmd_flags,
		      uint16_t token,
		      const uint8_t addr[6]);

/**
 * dpmac_get_api_version() - Get Data Path MAC version
 * @mc_io:  Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @major_ver:	Major version of data path mac API
 * @minor_ver:	Minor version of data path mac API
 *
 * Return:  '0' on Success; Error code otherwise.
 */
int dpmac_get_api_version(struct fsl_mc_io *mc_io,
			   uint32_t cmd_flags,
			   uint16_t *major_ver,
			   uint16_t *minor_ver);

#endif /* __FSL_DPMAC_H */
