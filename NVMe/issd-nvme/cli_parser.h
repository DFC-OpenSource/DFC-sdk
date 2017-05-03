/**
 * @file      : cli_parser.h
 * @brief     : This file contains the declarations for the functioins and
 *              structures used in cli_parser.c
 * @author    : Sakthi Kannan R (sakthikannan.r@vvdntech.com)
 * @copyright : (c) 2012-2014 , VVDN Technologies Pvt. Ltd.
 *              Permission is hereby granted to everyone in VVDN Technologies
 *              to use the Software without restriction, including without
 *              limitation the rights to use, copy, modify, merge, publish,
 *              distribute, distribute with modifications.
 */

#ifndef _CLI_PARSER_H
#define _CLI_PARSER_H    1

#ifndef DONT_USE_STDIO
#include <stdio.h>
#endif
#include <string.h>
#ifndef DONT_USE_STDLIB
#include <stdlib.h>
#endif
#include <stdarg.h>
#include "cli_config.h"
#ifdef PROJECT
#define CLI_PROMPT PROJECT
#else
#define CLI_PROMPT "CLI"
#endif

#if !(defined __linux__) && !(defined uC)
#error "Platform not supported"
#endif

#define FALSE 0
#define TRUE  1

#define REQUEST_COMPLETIONS 0x01
#define REQUEST_HELP        0x02

#define HIDDEN_CHAR 255

#if (defined CHARACTER_MODE) && !(defined CTRL)
#define CTRL(x) (U32) ((x) - '@')
#endif

/* End of Transmission */
#define EOT CTRL('D') /* EOT's ASCII is 0x04 */

#define DEBUG_PRINTF(module, format, args...) printf ("["#module"]" " - " format, ##args)

#ifdef DEBUG
#define CLI_DEBUG(format, args...) DEBUG_PRINTF (CLI, format, ##args)
#else
#define CLI_DEBUG(format, args...) {}
#endif

#define CLI_ERROR(format, args...) printf (format, ##args)

#define cprintf(c, fmt, args...)   ((c)->_cprintf) (c, fmt, ##args)
#define cputchar(c, ch)            ((c)->_cputchar) (c, ch)

#define cgetchar(c)                ((c)->_cgetchar) (c)

typedef enum argtype {
	KEYWORD,
	ARGUMENT,
	/* CHOICE_KEYWORD, */
	/* OPTIONAL_KEYWORD, */
} ARGTYPE_T;

#define NORMAL_COMMAND 0x0
#define ADMIN_COMMAND  0x1
#define HIDDEN_COMMAND 0x2
#define DEBUG_COMMAND  0x4

/**
 * @structure : contextInfo_t
 * @brief     : This is used to store the context info of the CLI session
 *              level mask
 * @members
 *     @isAdmin     : Is the logged in user super-user?
 *     @isDebug     : Debug mode enabled?
 *     @need_help   : Help requested from the user?
 *     @isRemote    : Is the CLI session over telnet?
 *     @pad         : Padding flag bits
 *     @sessionFd   : Session file descriptor
 *     @first_match : Haystack index for the first match
 *     @last_match  : Haystack index for the last match
 *     @_cprintf    : Function pointer for the printf function
 *     @_cgetchar   : Function pointer for the getchar function
 *     @cmd_handler : Function pointer for the command handler found by the
 *                    parser function
 *     @cmd_str     : Raw command string
 */
typedef struct contextInfo {
	U8   isAdmin:1;
	U8   isDebug:1;
	U8   need_help:2;
#if defined uC && defined REMOTE_CLI
	U8   isRemote:1;
	U8   pad:3;
	int  sessionFd;
#else
#if defined REMOTE_CLI && defined __linux__
	U8   pad:3;
	U8   isRemote:1;
#else 
	U8   pad:4;
#endif
#ifdef __linux__
	int  sessionFd;
#endif
#endif
	U16  first_match;
	U16  last_match;
	int  (*_cprintf)    (struct contextInfo *c, const char *fmt, ...);
	int  (*_cputchar)   (struct contextInfo *c, int ch);
	char (*_cgetchar)   (struct contextInfo *c);
	int  (*cmd_handler) (U32 argc, char **argv, struct contextInfo *c);
	char cmd_str[MAX_CLI_BUF_LEN+1];
} contextInfo_t;

extern U32 cgetstr (contextInfo_t *c, char *str, U8 echo, U32 max_len);

/**
 * @structure : argv_t
 * @brief     : This is used to store each word of the command and its
 *              corresponding help string.
 * @members
 *     @argv        : A word of the command
 *     @type        : Type of the command (KEYWORD | ARGUMENT)
 *     @help_string : Help string for the word
 */
typedef struct argv {
	const char *argv;
	ARGTYPE_T  type;
	const char *help_string;
} argv_t;

/**
 * @structure : cmd_struct_t
 * @brief     : This is used to store the details of each command.
 * @members
 *     @cmd_handler : Function pointer to the command handler
 *     @argc        : Number of words on the command string
 *     @argv        : Argv structure for the command
 *     @flags       : Flags for the command (NORMAL_COMMAND | ADMIN_COMMAND |
 *                    HIDDEN_COMMAND | DEBUG_COMMAND)
 */
typedef struct cmd_struct {
	int     (*cmd_handler) (U32 argc, char **argv, contextInfo_t *c);
	size_t  argc;
	argv_t  *argv;
	U8      flags;
} cmd_struct_t;

#define SIZEOF(x) (sizeof(x) / sizeof(x[0]))

typedef enum status {
	NOT_FOUND = 0,
	FOUND,
	AMBIGUOUS,
	AMBIGUOUS_YET_UNIQUE,
	AMBIGUOUS_MAYBE_UNIQUE,
	TOO_AMBIGUOUS,
	INCOMPLETE,
	HELPED,
	COMPLETED,
} status_t;

#define hs_argv(x,y)      haystack[(x)].argv[(y)].argv
#define hs_argc(x)        haystack[(x)].argc
#define hs_cmd_handler(x) haystack[(x)].cmd_handler
#define hs_type(x,y)      haystack[(x)].argv[(y)].type
#define hs_help(x,y)      haystack[(x)].argv[(y)].help_string
#define hs_flags(x)       haystack[(x)].flags

#define isCmdVisible(c,x) ((c)->isDebug || !(hs_flags(x) & HIDDEN_COMMAND))

#define isCmdAccessible(c,x)                             \
	(((c)->isDebug || !(hs_flags(x) & DEBUG_COMMAND)) && \
	 ((c)->isAdmin || !(hs_flags(x) & ADMIN_COMMAND)))

void clear_line (U32 length, contextInfo_t *c);
void show_cli   (contextInfo_t *c);
contextInfo_t *cli_init (U8 sessionFd);

/**
 * @function : local_cprintf
 * @param1   : c - Context Info
 * @param2   : fmt - printf like format arguments
 * @return   : Number of characters printed
 * @brief    : Print the string on the console, using printf.
 */
#ifdef __linux__
static inline int local_cprintf (contextInfo_t *c, const char *fmt, ...)
#else
static int local_cprintf (contextInfo_t *c, const char *fmt, ...)
#endif
{
	va_list argp;
	U32 ret = 0;
	va_start (argp, fmt);
	ret = vprintf (fmt, argp);
	va_end (argp);
	return ret;
}

/**
 * @function : local_cputchar
 * @param1   : c - Context Info
 * @param2   : ch - Character to be printed
 * @return   : The character printed as a char cast to an int
 * @brief    : Print the string on the console, using printf.
 */
#ifdef __linux__
static inline int local_cputchar (contextInfo_t *c, int ch)
#else
static int local_cputchar (contextInfo_t *c, int ch)
#endif
{
	return putchar(ch);
}

/**
 * @function : local_cgetchar
 * @param1   : c - Context Info
 * @return   : The character read
 * @brief    : Read a character from the console, using getchar
 */
#ifdef __linux__
static inline char local_cgetchar (contextInfo_t *c)
#else
#include <uart.h>
static char local_cgetchar (contextInfo_t *c)
#endif
{
#ifdef uC
	return (uart0_getch_nonblock());
#else
	return (getchar());
#endif
}
#if 0
#ifdef __linux__
#define remote_cprintf  local_cprintf
#define remote_cgetchar(c) local_cgetchar(c)
#endif
#endif

#ifdef CMD_HISTORY
#define MAX_HISTORY 10
typedef struct dlcl{
	struct dlcl *pre;
	char commd_ptr[50];
	struct dlcl *post;
}add_his_st;

add_his_st *head=NULL;
void Add_History(char *);
int print_history(U32 argc,char **argv, contextInfo_t *c);
#endif
#ifdef __linux__
int cli_main (contextInfo_t *c);
#endif
#endif
