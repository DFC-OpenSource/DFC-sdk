/**
 * @file      : cli_config.h
 * @brief     : This file contains the configurations for the CLI parser
 * @author    : Sakthi Kannan R (sakthikannan.r@vvdntech.com)
 * @copyright : (c) 2012-2014 , VVDN Technologies Pvt. Ltd.
 *              Permission is hereby granted to everyone in VVDN Technologies
 *              to use the Software without restriction, including without
 *              limitation the rights to use, copy, modify, merge, publish,
 *              distribute, distribute with modifications.
 */

#ifndef _CLI_CONFIG_H
#define _CLI_CONFIG_H    1

/* #define uC */ /* Define this if this is to be ported on a uC */

// #define PROJECT "RPSC" /* CLI prompt display */

/* #define DEBUG */
/* #define VERBOSE */

#define USER_PROMPT ">"
#define ROOT_PROMPT "#"
#define DEBUG_PROMPT " (debug-mode)"
#define CHARACTER_MODE
/* #define CMD_HISTORY */
#define REMOTE_CLI 
#ifdef uC
// #define REMOTE_CLI /* For CLI sessions over telnet */
/* #define DONT_USE_STDLIB */
/* #define DONT_USE_STDIO */
#endif /* uC */

#define MAX_CLI_BUF_LEN    200
#define CLI_TASK_PRIO      5

#if defined uC && defined REMOTE_CLI
#define MAX_REMOTE_CLIENTS 5
#define TELNETD_TASK_PRIO  5
#endif /* uC && REMOTE_CLI */

#if defined REMOTE_CLI
#define MAX_REMOTE_CLIENTS 5
#define TELNETD_TASK_PRIO  5
#endif /* REMOTE_CLI */

#ifdef uC

#define index   strchr

#ifdef DONT_USE_STDIO
/* #define getchar */
/* #define printf */
#ifndef getchar
#error "Please define handler for getchar"
#endif
#ifndef printf
#error "Please define handle for printf"
#endif
#endif /* DONT_USE_STDIO */

#endif /* uC */

#ifdef __GNUC__ /* gcc compiler */
/* #define uint8_t  unsigned char */
/* #define uint16_t unsigned short */
/* #define uint32_t unsigned int */
/* #define uint64_t unsigned long long */
#include <stdint.h>
#define U32      uint32_t
#define U16      uint16_t
#define U8       uint8_t

#else /* __GNUC__ */
#ifdef uC

/* Include your uC related files, which carries the definies the int
 * size */
/* #include <rtl.h> // For Keil, for definitions of U32, U16, U8 */
/* or */
/* #include <lpc_types.h> // For Keil, as an alternative for stdint.h */
/* #define U32      uint32_t */
/* #define U16      uint16_t */
/* #define U8       uint8_t */

#else /* uC */

#error "INT size not defined"

#endif /* uC */
#endif /* __GNUC__ */

/* If the target uC has another definition of dynamic memory related
 * functions, enable DONT_USE_STDLIB and define them here */
#ifdef DONT_USE_STDLIB
/* #define malloc */
/* #define calloc */
/* #define memset */
/* #define free */

/* Certain uCs don't support calloc, then use malloc and memset
 * combination */
#ifdef calloc
#define CALLOC(x, y) calloc((x), (y));
#else /* calloc */
#define CALLOC(x, y) ({                   \
		void *p = NULL;                   \
		p = malloc((x) * (y));            \
		if ((p) != NULL) {                \
		memset ((p), 0, (x) * (y));       \
		}                                 \
		p;                                \
		})
#endif /* calloc */

#else /* DONT_USE_STDLIB */

#define CALLOC(x, y) calloc((x), (y))

#endif /* DONT_USE_STDLIB */

#endif /* _CLI_CONFIG_H */
