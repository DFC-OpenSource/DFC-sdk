#ifndef __DF_DM_H
#define __DF_DM_H
#include "bdbm_drv.h"
extern struct bdbm_dm_inf_t _dm_dragonfire_inf;

uint32_t dm_df_probe (struct bdbm_drv_info* bdi);
uint32_t dm_df_open (struct bdbm_drv_info* bdi);
void dm_df_close (struct bdbm_drv_info* bdi); 

/**
 * @Func    	: dm_df_make_req
 * @Brief   	: Act as the entry point to the DM layer for IO commands which will
 * 				  be accessed by IO thread.
 *  @input  	: bdi			- Structure which contains drive information and 
 *  			              	  major parametrs used in each layer.
 *  			  ptr_llm_req	- Pointer to the LLM request.
 *  @return     : Return zero on success and non zero on failure.
 */
uint32_t dm_df_make_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t **ptr_llm_req);


/**
 * @Func    	: dm_df_end_req
 * @Brief   	: Act as the exit point from the DM layer for IO commands which will
 * 			  	  be accessed by completion thread.
 *  @input  	: bdi			- Structure which contains drive information and 
 *  			              	  major parametrs used in each layer.
 *  			  ptr_llm_req	- Pointer to the LLM request.
 *  @return     : Return zero on success and non zero on failure.
 */
void dm_df_end_req (struct bdbm_drv_info* bdi, struct bdbm_llm_req_t* ptr_llm_req);

void dm_df_completion_cb (void *req_ptr, uint8_t ret);
uint32_t dm_df_load (struct bdbm_drv_info* bdi, void *pmt, void *abm);
uint32_t dm_df_store (struct bdbm_drv_info* bdi);
#endif
