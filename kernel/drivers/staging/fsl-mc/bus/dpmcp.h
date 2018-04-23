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
#ifndef __FSL_DPMCP_H
#define __FSL_DPMCP_H

/* Data Path Management Command Portal API
 * Contains initialization APIs and runtime control APIs for DPMCP
 */

struct fsl_mc_io;

/**
 * dpmcp_open() - Open a control session for the specified object.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @dpmcp_id:	DPMCP unique ID
 * @token:	Returned token; use in subsequent API calls
 *
 * This function can be used to open a control session for an
 * already created object; an object may have been declared in
 * the DPL or by calling the dpmcp_create function.
 * This function returns a unique authentication token,
 * associated with the specific object ID and the specific MC
 * portal; this token must be used in all subsequent commands for
 * this specific object
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmcp_open(struct fsl_mc_io	*mc_io,
	       uint32_t		cmd_flags,
	       int			dpmcp_id,
	       uint16_t		*token);

/* Get portal ID from pool */
#define DPMCP_GET_PORTAL_ID_FROM_POOL (-1)

/**
 * dpmcp_close() - Close the control session of the object
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMCP object
 *
 * After this function is called, no further operations are
 * allowed on the object without opening a new control session.
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmcp_close(struct fsl_mc_io	*mc_io,
		uint32_t		cmd_flags,
		uint16_t		token);

/**
 * struct dpmcp_cfg - Structure representing DPMCP configuration
 * @portal_id:	Portal ID; 'DPMCP_GET_PORTAL_ID_FROM_POOL' to get the portal ID
 *				from pool
 */
struct dpmcp_cfg {
	int portal_id;
};

/**
 * dpmcp_create() - Create the DPMCP object.
 *
 * @mc_io:	Pointer to MC portal's I/O object
 * @dprc_token: parent container token; '0' for default container
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @cfg:	Configuration structure
 * @obj_id: returns the object id or portal id.
 *
 * Create the DPMCP object, allocate required resources and
 * perform required initialization.
 *
 * The object can be created either by declaring it in the
 * DPL file, or by calling this function.
 *
 * The function accepts an authentication token of a parent
 * container that this object should be assigned to. The token
 * can be '0' so the object will be assigned to the default container.
 * The newly created object can be opened with the returned
 * object id and using the container's associated tokens and MC portals.
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmcp_create(struct fsl_mc_io	*mc_io,
		uint16_t	dprc_token,
		uint32_t	cmd_flags,
		const struct dpmcp_cfg	*cfg,
		uint32_t	*obj_id);

/**
 * dpmcp_destroy() - Destroy the DPMCP object and release all its resources.
 *
 * @mc_io:	Pointer to MC portal's I/O object
 * @dprc_token: MC v10.x - parent container token; '0' for default container
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @object_id:  Object id or portal id; it must be a valid id
 *              within the container that created this object.
 *
 * The function accepts the authentication token of the parent
 * container that created the object (not the one that currently owns
 * the object). The object is searched within parent using the provided
 * object_id. All tokens to the object must be closed before calling destroy.
 *
 * Return:	'0' on Success; error code otherwise.
 */
int dpmcp_destroy(struct fsl_mc_io	*mc_io,
		uint16_t	dprc_token,
		uint32_t	cmd_flags,
		uint32_t	object_id);

/**
 * dpmcp_reset() - Reset the DPMCP, returns the object to initial state.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMCP object
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmcp_reset(struct fsl_mc_io	*mc_io,
		uint32_t		cmd_flags,
		uint16_t			token);

/* IRQ */
/* IRQ Index */
#define DPMCP_IRQ_INDEX                             0
/* irq event - Indicates that the link state changed */
#define DPMCP_IRQ_EVENT_CMD_DONE                    0x00000001

/**
 * struct dpmcp_irq_cfg - IRQ configuration
 * @paddr:	Address that must be written to signal a message-based interrupt
 * @val:	Value to write into irq_addr address
 * @user_irq_id: A user defined number associated with this IRQ
 */
struct dpmcp_irq_cfg {
	     uint64_t		paddr;
	     uint32_t		val;
	     int		user_irq_id;
};

/**
 * dpmcp_set_irq() - Set IRQ information for the DPMCP to trigger an interrupt.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMCP object
 * @irq_index:	Identifies the interrupt index to configure
 * @irq_cfg:	IRQ configuration
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmcp_set_irq(struct fsl_mc_io	*mc_io,
		  uint32_t		cmd_flags,
		  uint16_t		token,
		  uint8_t		irq_index,
		  struct dpmcp_irq_cfg	*irq_cfg);

/**
 * dpmcp_get_irq() - Get IRQ information from the DPMCP.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMCP object
 * @irq_index:	The interrupt index to configure
 * @type:	Interrupt type: 0 represents message interrupt
 *		type (both irq_addr and irq_val are valid)
 * @irq_cfg:	IRQ attributes
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmcp_get_irq(struct fsl_mc_io	*mc_io,
		  uint32_t		cmd_flags,
		  uint16_t		token,
		  uint8_t		irq_index,
		  int			*type,
		  struct dpmcp_irq_cfg	*irq_cfg);

/**
 * dpmcp_set_irq_enable() - Set overall interrupt state.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMCP object
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
int dpmcp_set_irq_enable(struct fsl_mc_io	*mc_io,
			 uint32_t		cmd_flags,
			 uint16_t		token,
			 uint8_t		irq_index,
			 uint8_t		en);

/**
 * dpmcp_get_irq_enable() - Get overall interrupt state
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMCP object
 * @irq_index:	The interrupt index to configure
 * @en:		Returned interrupt state - enable = 1, disable = 0
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmcp_get_irq_enable(struct fsl_mc_io	*mc_io,
			 uint32_t		cmd_flags,
			 uint16_t		token,
			 uint8_t		irq_index,
			 uint8_t		*en);

/**
 * dpmcp_set_irq_mask() - Set interrupt mask.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMCP object
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
int dpmcp_set_irq_mask(struct fsl_mc_io *mc_io,
		       uint32_t	cmd_flags,
		       uint16_t		token,
		       uint8_t		irq_index,
		       uint32_t		mask);

/**
 * dpmcp_get_irq_mask() - Get interrupt mask.
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMCP object
 * @irq_index:	The interrupt index to configure
 * @mask:	Returned event mask to trigger interrupt
 *
 * Every interrupt can have up to 32 causes and the interrupt model supports
 * masking/unmasking each cause independently
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmcp_get_irq_mask(struct fsl_mc_io *mc_io,
		       uint32_t	cmd_flags,
		       uint16_t		token,
		       uint8_t		irq_index,
		       uint32_t		*mask);

/**
 * dpmcp_get_irq_status() - Get the current status of any pending interrupts.
 *
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMCP object
 * @irq_index:	The interrupt index to configure
 * @status:	Returned interrupts status - one bit per cause:
 *			0 = no interrupt pending
 *			1 = interrupt pending
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmcp_get_irq_status(struct fsl_mc_io	*mc_io,
			 uint32_t		cmd_flags,
			 uint16_t		token,
			 uint8_t		irq_index,
			 uint32_t		*status);

/**
 * struct dpmcp_attr - Structure representing DPMCP attributes
 * @id:		DPMCP object ID
 */
struct dpmcp_attr {
	int id;
};

/**
 * dpmcp_get_attributes - Retrieve DPMCP attributes.
 *
 * @mc_io:	Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @token:	Token of DPMCP object
 * @attr:	Returned object's attributes
 *
 * Return:	'0' on Success; Error code otherwise.
 */
int dpmcp_get_attributes(struct fsl_mc_io	*mc_io,
			 uint32_t		cmd_flags,
			 uint16_t		token,
			 struct dpmcp_attr	*attr);

/**
 * dpmcp_get_api_version() - Get Data Path Management Command Portal API version
 * @mc_io:  Pointer to MC portal's I/O object
 * @cmd_flags:	Command flags; one or more of 'MC_CMD_FLAG_'
 * @majorVer: 	Major version of data path management command portal API
 * @minorVer: 	Minor version of data path management command portal API
 *
 * Return:  '0' on Success; Error code otherwise.
 */
int dpmcp_get_api_version(struct fsl_mc_io *mc_io,
			   uint32_t cmd_flags,
			   uint16_t* major_ver,
			   uint16_t* minor_ver);
#endif /* __FSL_DPMCP_H */
