
#ifndef __COMMON_H
#define __COMMON_H

#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>

/**
 * @Structure  : NvmeTimer
 * @Brief      : Contains the parameters used for timer-callback operations in NVMe
 * @Members
 *  @timerID : timer ID
 *  @sev : signal event info
 *  @its : timeout info
 *  @sa : action for the signal
 */
typedef struct NvmeTimer {
	timer_t timerID;
	struct sigevent sev;
	struct itimerspec its;
	struct sigaction sa;
} NvmeTimer;

#ifndef MIN
#define MIN(a,b) (a<b)?a:b
#endif

/**
 * @func   : create_nvme_timer
 * @brief  : create new timer and start it
 * @input  : void *timerCB - callback for timeout handling
 *			 void *arg - transparent pointer to any valid data in memory
 *			 uint16_t expiry_ns - timeout duration 
 * @output : none
 * @return : if success, pointer to new timer info;
 * 			 if failure, NULL.
 */
NvmeTimer *create_nvme_timer (void *timerCB, void *arg, uint16_t expiry_ns);

/**
 * @func   : nvme_timer_expired
 * @brief  : verify if a timer is expired
 * @input  : NvmeTimer *timer - timer whose status is needed
 * @output : none
 * @return : 1 if timer's expired, else 0
 */
int nvme_timer_expired (NvmeTimer *timer);

/**
 * @func   : start_nvme_timer
 * @brief  : start timer; called when it has already timed-out
 * @input  : NvmeTimer *timer - timer info
 * @output : none
 * @return : 0 if timer starts, else < 0
 */
int start_nvme_timer (NvmeTimer *timer);

/**
 * @func   : stop_nvme_timer
 * @brief  : stop timer
 * @input  : NvmeTimer *timer - timer info
 * @output : none
 * @return : void
 */
void stop_nvme_timer (NvmeTimer *timer);

/**
 * @func   : _delete_nvme_timer
 * @brief  : delete timer
 * @input  : NvmeTimer **timer - timer info
 * @output : none
 * @return : void
 */
void _delete_nvme_timer (NvmeTimer **timer);

#define nvme_timer_pending(timer) !nvme_timer_expired (timer)
#define delete_nvme_timer(timer) _delete_nvme_timer (&timer)

#endif /*#ifndef __COMMON_H*/

