/**
 * @file      : cli_parser.c
 * @brief     : This file is the main file for the PCLI (Predictive CLI) parser
 * @author    : Sakthi Kannan R (sakthikannan.r@vvdntech.com)
 * @copyright : (c) 2012-2014 , VVDN Technologies Pvt. Ltd.
 *              Permission is hereby granted to everyone in VVDN Technologies
 *              to use the Software without restriction, including without
 *              limitation the rights to use, copy, modify, merge, publish,
 *              distribute, distribute with modifications.
 */

#ifndef DONT_USE_STDIO
#include <stdio.h>
#endif
#include <string.h>
#ifndef DONT_USE_STDLIB
#include <stdlib.h>
#endif
#include <stdarg.h>
#include "cli_parser.h"
#include "cmd_struct.h"
#include "cmd_defn.h"
#ifdef REMOTE_CLI
#include "telnetd.h"
#include "iac.h"
#endif

#ifdef uC
#include <uart.h>
#ifdef REMOTE_CLI
#include "telnetd.h"
#include "iac.h"
#endif
#endif
#include <pthread.h>
#include "common.h"
//static contextInfo_t context_local;

static U32 getcode (contextInfo_t* c);
static int exec_cmd (contextInfo_t *c);
static status_t final_ambiguity_check (char *needle, U32 n, contextInfo_t *c);
static status_t find_needle (char *needle, U32 n, U8 need_help,
							U8 help_word, contextInfo_t *c);
static status_t helper (char *needle, U8 word, status_t status,
						contextInfo_t *c);
static status_t parse_cmd (char *help_on_word, contextInfo_t *c);
static size_t squeeze_spaces (char *string);
static size_t insert_char (char *dst, char src);
static size_t insert_word (char *dst, char *src);
static size_t remove_word (char *start, char *end);
static size_t remove_char (char *ptr);

#if defined CHARACTER_MODE && __linux__
#include <stdio_ext.h>
#include <unistd.h>
#include <ctype.h>
#include <termios.h>

static struct termios stored_settings;

/**
 * @function : set_keypress
 * @brief    : Change the linux terminal from buffered mode to
 *             character mode.
 * @caller   : cli_main
 */
static void set_keypress (void)
{
	struct termios new_settings;
	tcgetattr(0,&stored_settings);

	new_settings = stored_settings;

	/* Disable canonical mode, and set buffer size to 1 byte */
	new_settings.c_lflag &= ~(ICANON | ECHO);
	/* new_settings.c_lflag &= (~ECHO); */
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
	/* tcsetattr(0,TCSANOW, &new_settings); */
	tcsetattr(0,TCSAFLUSH, &new_settings);
	return;
}

/**
 * @function : reset_keypress
 * @brief    : Change the linux terminal backed to buffered mode
 *             from character mode.
 * @caller   : quit_cli
 */
static void reset_keypress (void)
{
	tcsetattr(0, TCSANOW, &stored_settings);
	memset(&stored_settings, 0, sizeof(struct termios));
	return;
}
#endif /* CHARACTER_MODE && __linux__ */

/**
 * @function : clean_exit
 * @param1   : c - Context Info
 * @return   : EOT - End of Transmission
 * @brief    : Clean upon exit.
 * @caller   : quit_cli
 */
int clean_exit (contextInfo_t *c)
{
	//#if defined uC && defined REMOTE_CLI
	if (!c) return EOT;
#ifdef REMOTE_CLI
	if (c->isRemote) {
		if (c->sessionFd)
			close_sock (c->sessionFd);
		c->sessionFd = 0;
		/* Clear history */
		free (c);
		c = NULL;
		//os_tsk_delete_self();
	}
#endif
	return EOT;
}

/**
 * @function : show_cli
 * @param1   : c - Context Info
 * @brief    : Print the CLI prompt.
 * @caller   : cli_main
 */
void show_cli (contextInfo_t *c)
{
	cprintf (c, "\r%s %s ",
			c->isDebug ? CLI_PROMPT DEBUG_PROMPT : CLI_PROMPT,
			c->isAdmin ? ROOT_PROMPT : USER_PROMPT);
#if !(defined CHARACTER_MODE) && (defined __linux__)
	fflush (stdout);
#endif /* #if !(defined CHARACTER_MODE) && (defined __linux__) */
}

/**
 * @function : help_cli
 * @param1   : argc - No. of cmd line args
 * @param2   : argv - Cmd line args
 * @param3   : c - Context Info
 * @brief    : Command handler for the command "help".
 * @caller   : exec_cmd
 */
int help_cli (U32 argc, char **argv, contextInfo_t *c)
{
	/* Clear of the command string so as to get help on all
	 * commands */
	c->cmd_str[0] = '\0';
	c->cmd_handler = NULL;
	c->need_help = REQUEST_HELP;
	parse_cmd ("", c);
	return 0;
}

/**
 * @function : quit_cli
 * @param1   : argc - No. of cmd line args
 * @param2   : argv - Cmd line args
 * @param3   : c - Context Info
 * @return   : EOT
 * @brief    : Quit the CLI
 * @caller   : exec_cmd
 */
int quit_cli (U32 argc, char **argv, contextInfo_t *c)
{
#if defined CHARACTER_MODE && __linux__
	reset_keypress();
#endif
	return (clean_exit(c));
}

/**
 * @function : debug_show_cmd
 * @param1   : argc - No. of cmd line args
 * @param2   : argv - Cmd line args
 * @param3   : c - Context Info
 * @return   : EOT - Upon calloc failure, 0 - Success
 * @brief    : List all the commands from the haystack
 * @caller   : exec_cmd
 */
int debug_show_cmd (U32 argc, char **argv, contextInfo_t *c)
{
	U32 i, j;

	/* CLI_DEBUG ("Done - start = %d\r\n", start); */
	for (i = 0; i < SIZEOF(haystack); i++) {
		cprintf (c, "CMD: ");
		for (j = 0; j < hs_argc(i); j++) {
			cprintf (c, "%s ", hs_argv(i,j));
		}
		cprintf (c, "\r\n");
		for (j = 0; j < hs_argc(i); j++) {
			cprintf (c, "  %-20s -   %-20s\r\n",
					hs_argv(i,j),
					hs_help(i,j));
		}
		cprintf (c, "\r\n");
	}
	return 0;
}

/**
 * @function : final_ambiguity_check
 * @param1   : needle - The word under process
 * @param2   : n - The index of the word under process
 * @param3   : c - Context Info
 * @return   : status_t
 * @brief    : Final check for ambiguity, to validate the needle by looking
 *             deeper into the haystack.
 * @caller   : find_needle
 */
static status_t final_ambiguity_check (char *needle, U32 n, contextInfo_t *c)
{
	U32 len_str1, len_str2;
	ARGTYPE_T type1 = hs_type (c->first_match, n);
	ARGTYPE_T type2 = hs_type (c->last_match, n);
	status_t rc = NOT_FOUND;

	/* This is the tricky bit, this procedure is written based on a
	 * certain assumption made on the order in which the haystack is
	 * arranged, refer cmd_struct_t structure */
	CLI_DEBUG ("AMBIGUOUS: first_match = %d, last_match = %d\r\n",
			c->first_match, c->last_match);
	if ((type1 == ARGUMENT) && (type2 == ARGUMENT)) {
		CLI_DEBUG ("BOTH ARE ARGS. HENCE, AMBIGUOUS_YET_UNIQUE\r\n");
		rc = AMBIGUOUS_YET_UNIQUE;
	} else if ((type1 == ARGUMENT) || (type2 == ARGUMENT)) {
		/* ONE IS AN ARG AND THE OTHER ISN'T, REDUCING */
		while (c->first_match != c->last_match) {
			if (hs_type(c->first_match,n) == ARGUMENT)
				c->first_match++;
			else if (hs_type(c->last_match,n) == ARGUMENT)
				c->last_match--;
			else
				break;
		}
		CLI_DEBUG("Skiped on the args, reduced first=%d, last=%d\r\n",
				c->first_match, c->last_match);
		/* Have we found our match yet? */
		if (c->first_match == c->last_match) {
			return FOUND;
		}
		/* The "FINAL" final check for ambiguity */
		rc = find_needle (needle, n, FALSE, 0, c);
		CLI_DEBUG ("SECOND rc=%d\r\n", rc);
		return rc;
	}
	len_str1 =  strlen (hs_argv(c->first_match,n));
	len_str2 =  strlen (hs_argv(c->last_match,n));
	CLI_DEBUG ("%s <==> %s : %d\r\n",
			hs_argv(c->first_match,n), hs_argv(c->last_match,n), len_str1);
	if (strncasecmp (hs_argv(c->first_match,n),
				hs_argv(c->last_match,n),
				len_str1) == 0) {
		if (len_str1 == len_str2) {
			/* Ambiguous, yet unique */
			CLI_DEBUG ("AMBIGUOUS_YET_UNIQUE\r\n");
			rc = AMBIGUOUS_YET_UNIQUE;
			/* If AMBIGUOUS, in the next iteration look into this
			 * sub-set alone from here. Hence reducing the number
			 * of iterations */
		} else {
			/* Ambiguous, not unique */
			CLI_DEBUG ("AMBIGUOUS_MAYBE_UNIQUE\r\n");
			rc = AMBIGUOUS_MAYBE_UNIQUE;
		}
	} else {
		CLI_DEBUG ("TOO AMBIGUOUS\r\n");
		rc = TOO_AMBIGUOUS;
	}
	return rc;
}

/**
 * @function : find_needle
 * @param1   : needle - The word to be processed
 * @param2   : n - Index of the needle
 * @param3   : need_help - Is help requested?
 * @param4   : help_word - Index of the word on which help is requested
 * @param5   : c - Context Info
 * @return   : status_t
 * @brief    : Look for the needle in the haystack.
 * @caller   : final_ambiguity_check, helper, parse_cmd
 */
static status_t find_needle (char *needle, U32 n, U8 need_help, U8 help_word, contextInfo_t *c)
{
	/* Sanity */
	U32 found = 0, length = 0;
	U16 i = 0, last_match = c->last_match;
	U32 match = FALSE;
	status_t rc = NOT_FOUND;
	char *ptr = NULL;

	/* CLI_DEBUG ("START\r\n"); */

	for (i = c->first_match; i <= c->last_match; i++, match = FALSE) {
		ptr = index (needle, ' ');
		/* If the command is not accessible in this context then ignore
		 * it */
		if (!isCmdAccessible(c,i))
			continue;

		/* If the number of words on the needle string is greater than
		 * the haystack string then ignore it */
		if (n >= hs_argc(i))
			continue;

		/* Length of the needle word */
		if (ptr) {
			length = (U32)(ptr - needle);
		} else { /* Last word */
			length = strlen (needle);
		}

		switch (hs_type(i,n)) {
			case KEYWORD:
				if ((length > strlen (hs_argv(i,n))) || (n >= hs_argc(i))) {
					CLI_DEBUG("%d:needle=\"%s\" in haystack=\"%s\":%d(skipped)\r\n",
							i, needle, hs_argv(i,n), length);
					continue;
				}
				/* In case of hidden commands, turn off preemptiveness
				 * Match if only the whole command is typed */
				if (!isCmdVisible(c,i)) {
					length = strlen (hs_argv(i,n));
				}
				match = !(strncasecmp (needle, hs_argv(i,n), length));
				break;
			case ARGUMENT:
				match = TRUE;
				break;
			default:
				match = FALSE;
		}
		if (match) {
			CLI_DEBUG ("%d: needle=\"%s\" in haystack=\"%s\" : %d <- *\r\n",
					i, needle, hs_argv(i,n), length);

			/* Make note of the first match */
			if ((found++) == 0) {
				c->first_match = i;
				if (need_help && isCmdVisible(c,i)) {
					CLI_DEBUG ("help_word = %d\r\n", help_word);
					if (help_word < hs_argc(i))
						cprintf (c, "\r\n  %-20s -   %-20s",
								hs_argv(i,help_word), hs_help(i,help_word));
					else
						cprintf (c, "\r\n  %-20s -   %-20s",
								"<cr>", "Execute the command");
				}
			} else if (need_help && isCmdVisible(c,i) &&
					/* (help_word <= hs_argc(last_match) && */
				(help_word <= hs_argc(i)))/* ) */ {
					CLI_DEBUG ("help_word=%d\r\n", help_word);
					if ((help_word < hs_argc(last_match)) &&
							(strcasecmp (hs_argv(last_match,help_word),
								     hs_argv(i,help_word)) == 0)) {
						continue;
					}
					if (help_word < hs_argc(i))
						cprintf (c, "\r\n  %-20s -   %-20s",
								hs_argv(i,help_word), hs_help(i,help_word));
					else
						cprintf (c, "\r\n  %-20s -   %-20s",
								"<cr>", "Execute the command");
				}
			last_match = i;
		} else {
			CLI_DEBUG ("%d: needle=\"%s\" in haystack=\"%s\" : %d\r\n",
						i, needle, hs_argv(i,n), length);
		}
#ifdef uC /* Keil */
		os_tsk_pass();
#endif
	}
	if (need_help) {
		cprintf (c, "\r\n");
		return HELPED;
	}
	if (found == 0) {
		/* CLI_DEBUG ("NOT_FOUND\r\n"); */
		rc = NOT_FOUND;
	} else {
		if (found == 1) {
			CLI_DEBUG ("FOUND: found = %d, opcode = %d\r\n",
					found, c->first_match);
			rc = FOUND;
			/* If FOUND, in the next iteration look into this entry
			 * only. Hence reducing the number of iterations */
			c->last_match = c->first_match;
		} else { /* Ambiguous, test for uniqueness */
			c->last_match = last_match;
			rc = final_ambiguity_check (needle, n, c);
		}
		CLI_DEBUG ("n = %d\r\n", n);
	}
	return rc;
}

/**
 * @function : helper
 * @param1   : needle - The word to be processed
 * @param2   : word - Index of the needle
 * @param3   : status - Status of the attempt to find the needle in the haystack
 * @param4   : c - Context Info
 * @return   : status_t
 * @brief    : Displays the help messages of the commands or completes the
 *             requested command.
 * @caller   : parse_cmd
 */
static status_t helper (char *needle, U8 word, status_t status, contextInfo_t *c)
{
	size_t length;
	char *remaining_letters = NULL, *ptr = NULL;
	status_t rc = status;

	CLI_DEBUG ("Enter - HELPER\r\n");
	CLI_DEBUG ("command = \"%s\"\r\n", needle);
	ptr = index (needle, ' ');
	if (ptr) {
		length = ptr - needle;
	} else {
		length = strlen (needle);
	}
	remaining_letters = (char *) (hs_argv(c->first_match, word) + length);
	if ((c->need_help == REQUEST_HELP) &&
			(*remaining_letters != '\0') &&
			(hs_type(c->first_match, word) != ARGUMENT)) {
		switch (status) {
			case FOUND:
				if (isCmdVisible(c,c->first_match)) {
					cprintf (c, "\r\n  %-20s -   %-20s\r\n",
							hs_argv(c->first_match,word),
							hs_help(c->first_match,word));
					rc = HELPED;
					break;
				} else {
					return NOT_FOUND;
				}
			case AMBIGUOUS_YET_UNIQUE:
			case AMBIGUOUS_MAYBE_UNIQUE:
			case TOO_AMBIGUOUS:
				rc = find_needle (needle, word, TRUE, word, c);
				break;
			default:
				cputchar (c, '\a');
				break;
		}
	} else {
		if ((*remaining_letters != '\0') &&
				(hs_type(c->first_match, word) != ARGUMENT)) {
			switch (status) {
				case FOUND:
				case AMBIGUOUS_YET_UNIQUE:
					if (isCmdVisible(c,c->first_match)) {
						length = strlen(hs_argv(c->first_match, word));
						if (ptr == NULL) {
							strncpy (needle, hs_argv(c->first_match, word), length);
							needle[length] = ' ';
							needle[length+1] = '\0';
						} else {
							insert_word (ptr, remaining_letters);
							insert_char (needle+length, ' ');
						}
						rc = COMPLETED;
						break;
					} else {
						return NOT_FOUND;
					}
				case AMBIGUOUS_MAYBE_UNIQUE:
					if (isCmdVisible(c,c->first_match)) {
						length = strlen(hs_argv(c->first_match, word));
						if (ptr == NULL) {
							CLI_DEBUG("start=%d, haystack[start,word]=\"%s\"\r\n",
									c->first_match,
									hs_argv(c->first_match, word));
							strncpy (needle, hs_argv(c->first_match, word), length);
							needle[length] = '\0';
						} else {
							insert_word (ptr, remaining_letters);
						}
						rc = HELPED;
					} else {
						return NOT_FOUND;
					}
					/* Fall through */
				case TOO_AMBIGUOUS:
					rc = find_needle (needle, word, TRUE, word, c);
				default:
					cputchar (c, '\a');
					return rc;
			}
		} else {
			switch (status) {
				case FOUND:
					if (isCmdVisible(c,c->first_match)) {
						if (word+1 >= hs_argc(c->first_match)) {
							cprintf (c, "\r\n  %-20s -   %-20s\r\n",
									"<cr>", "Execute the command");
						} else {
							cprintf (c, "\r\n  %-20s -   %-20s\r\n",
									hs_argv(c->first_match,word+1),
									hs_help(c->first_match,word+1));
						}
						rc = HELPED;
						break;
					} else {
						return NOT_FOUND;
					}
				case AMBIGUOUS_YET_UNIQUE:
					rc = find_needle (needle, word, TRUE, word+1, c);
					break;
				case AMBIGUOUS_MAYBE_UNIQUE:
				case TOO_AMBIGUOUS:
					rc = find_needle (needle, word, TRUE, word, c);
					break;
				default:
					cputchar (c, '\a');
					return rc;
			}
			if (status == AMBIGUOUS_MAYBE_UNIQUE) {
				needle[length] = '\0';
			} else {
				needle[length] = ' ';
				needle[length+1] = '\0';
			}
		}
	}
	return rc;
}

/**
 * @function : parse_cmd
 * @param1   : help_on_word - Word on which help needed
 * @param2   : c - Context Info
 * @return   : status_t
 * @brief    : Parse the command in c->cmd_str and also call helper if
 *             help_on_word is set.
 * @caller   : cli_main, help_cli
 */
static status_t parse_cmd (char *help_on_word, contextInfo_t *c)
{
	U8 word = 0;
	char *command, *prev_word;
	status_t rc = NOT_FOUND;

	command = c->cmd_str;

	if (c->need_help && help_on_word) {
		while (help_on_word != c->cmd_str) {
			help_on_word--;
			if (*help_on_word == ' ')
				break;
		}
		if (help_on_word != c->cmd_str) {
			help_on_word++;
		}
	}

	CLI_DEBUG ("Got command \"%s\", help_on_word=\"%s\"\r\n",
			c->cmd_str, help_on_word);

	c->first_match = 0;
	c->last_match = SIZEOF(haystack) - 1;

	/* Take a backup */
	prev_word = command;
	do {
		CLI_DEBUG ("ENTER LOOP\r\n");
		/* Search for the needle within the selected haystack */
		rc = find_needle (command, word, FALSE, 0, c);
		if (rc == NOT_FOUND) {
			/* CLI_DEBUG ("From here %s:%d\r\n", __func__, __LINE__); */
			cputchar (c, '\a');
			break;
		}

		CLI_DEBUG ("%s:%d, command = %s\r\n", __func__, __LINE__, command);

		/* Move on to the next word */
		prev_word = command;
		command = index (command, (int) ' ');
		if (command) {
			if (rc == TOO_AMBIGUOUS) {
				break;
			}
			if (c->need_help && help_on_word &&
					(prev_word == help_on_word)) {
				rc = helper (prev_word, word, rc, c);
				break;
			}
			command++;
			word++;
		} else { /* End of the input string */
			/* Check for the end in the cmd_string also */
			/* CLI_DEBUG ("start = %d, hs_argc(start) = %d, word = %d, hs_type(start, word+1) = %d\r\n",
			   c->first_match, hs_argc(c->first_match), word,
			   hs_type(c->first_match, word+1)); */
			if (hs_argc(c->first_match) == (word+1) && c->need_help == FALSE) {
				/* The cmd_string has also been terminated, this means
				 * we have found our command */
				CLI_DEBUG ("Reached the end\r\n");
				if ((rc == AMBIGUOUS_YET_UNIQUE) ||
						(rc == AMBIGUOUS_MAYBE_UNIQUE)) {
					rc = FOUND;
				} else if (rc == TOO_AMBIGUOUS) {
					break;
				}
			} else {
				/* The cmd_string has more, while the command entered
				 * by the user has abrubtly ended */
				CLI_DEBUG ("Reached an ambiguous end\r\n");
				if (c->need_help) {
					rc = helper (prev_word, word, rc, c);
				} else {
					if ((rc == AMBIGUOUS_YET_UNIQUE) ||
							(rc == AMBIGUOUS_MAYBE_UNIQUE) ||
							(rc == FOUND)) {
						rc = INCOMPLETE;
					} else if (rc == TOO_AMBIGUOUS) {
						break;
					} else {
						rc = AMBIGUOUS;
					}
				}
			}
			/* CLI_DEBUG ("From here %s:%d\r\n", __func__, __LINE__); */
			break;
		}
		CLI_DEBUG ("%s:%d, command = \"%s\"\r\n", __func__, __LINE__, command);
#ifdef uC /* Keil */
		os_tsk_pass();
#endif
	} while (*command != '\0');

	if (rc == FOUND) {
#ifdef VERBOSE
		char *str = NULL;
		U32 i;

		str = (char *) CALLOC (MAX_CLI_BUF_LEN, sizeof(char));
		if (str == NULL) {
			CLI_ERROR ("%s:%d, calloc error\r\n", __func__, __LINE__);
			return EOT;
		}

		/* CLI_DEBUG ("Done - start = %d\r\n", c->first_match); */
		for (i = 0; i < hs_argc(c->first_match); i++) {
			sprintf (str, "%s %s", str, hs_argv(c->first_match, i));
		}
		cprintf (c, "FOUND: \"%s\" = \"%s\"\r\n", c->cmd_str, &str[1]);
		free (str);
#endif
		c->cmd_handler = hs_cmd_handler(c->first_match);
		return rc;
	}
	if (c->need_help)
		return rc;
	switch (rc) {
		case TOO_AMBIGUOUS:
		case AMBIGUOUS:
			cprintf (c, "Error: \"%s\" Ambiguous command\r\n", c->cmd_str);
			break;
		case INCOMPLETE:
			cprintf (c, "Error: \"%s\" Incomplete command\r\n", c->cmd_str);
			break;
		case NOT_FOUND:
			cprintf (c, "Error: \"%s\" Command not found\r\n", c->cmd_str);
			break;
		default:
			cprintf (c, "Error: \"%s\" Command not found, rc = %d\r\n",
					c->cmd_str, rc);
			break;
	}
	return rc;
}

/**
 * @function : squeeze_spaces
 * @param1   : string - The raw command string entered by the user
 * @return   : Resultant size of the squeezed string
 * @brief    : Squeeze out the spaces in front / between / end of the string
 * @caller   : cli_main
 */
static size_t squeeze_spaces (char *string)
{
	char *ptr;
	U32 offset = 0;
	size_t length = strlen (string);

	/* Replace double white spaces with a single white space */
	while ((ptr = strstr (string, "  ")) && (offset <= length)) {
		offset = ptr - string;
		memmove (ptr, ptr+1, length - offset);
	}
	length = strlen (string);

	/* If the input string begins with a ' ' */
	if (string[0] == ' ') {
		memmove (string, string+1, length);
	}
	length = strlen (string);

	/* Remove the trailing ' ' */
	if (string[length-1] == ' ') {
		string[length-1] = '\0';
	}
	return length;
}

/**
 * @function : insert_char
 * @param1   : dst - Destination pointer, location to insert
 * @param2   : src - Source character to be inserted
 * @return   : Size of the resultant string, after insertion
 * @brief    : Insert a character into the dst string.
 * @caller   : cli_main, helper
 */
static size_t insert_char (char *dst, char src)
{
	size_t length = strlen (dst);
	memmove (dst+1, dst, length+1);
	*dst = src;
	return (length + 1);
}

/**
 * @function : insert_word
 * @param1   : dst - Destination pointer, location to insert
 * @param2   : src - Source string to be inserted
 * @return   : Size of the resultant string, after insertion
 * @brief    : Insert a word into the dst string.
 * @caller   : helper
 */
static size_t insert_word (char *dst, char *src)
{
	size_t dst_length = strlen (dst);
	size_t src_length = strlen (src);
	memmove (dst+src_length, dst, dst_length+1);
	strncpy (dst, src, src_length);
	return (dst_length + src_length);
}

/**
 * @function : remove_word
 * @param1   : start - Pointer to the start of the word to be removed
 * @param2   : end - Pointer to the end of the word to be removed
 * @return   : Number of characters removed
 * @brief    : Remove a word in the string.
 * @caller   : cli_main
 */
static size_t remove_word (char *start, char *end)
{
	U32 length;
	if (end > start)
		length = (U32) (end - start);
	else
		return 0;
	memmove (start, end, strlen(end)+1);
	return (length);
}

/**
 * @function : remove_char
 * @param1   : ptr -  Pointer to the character to be removed
 * @return   : 1 (character removed)
 * @brief    : Remove a single character from the string.
 * @caller   : cli_main
 */
static size_t remove_char (char *ptr)
{
	size_t length = strlen(ptr);
	memmove (ptr, ptr+1, length);
	return (1);
}

/**
 * @function : clear_line
 * @param1   : lenght - Lenght of the raw command string typed by the user
 * @param2   : c - Context Info
 * @brief    : Clears the line, upto the number of characters as specified by
 *             the length parameter.
 * @caller   : cli_main
 */
void clear_line (U32 length, contextInfo_t *c)
{
	cprintf (c, "\033[%dD\033[K", length);
}

#ifdef CHARACTER_MODE
/**
 * @function : getcode
 * @param1   : c - Context Info
 * @return   : Returns the code read from the user
 * @brief    : This is a wrapper for the getchar function, to handle key
 *             combinations and also IAC command when running on a telnet server.
 * @caller   : cli_main
 */
static U32 getcode (contextInfo_t* c)
{
	U32 code = 0;
	code = cgetchar(c);
	if (code == '\033') {
		code = cgetchar(c);
		switch (code) {
			case '[':
				code = cgetchar(c);
				switch (code) {
					case 'A': /* Up arrow key */
						code = CTRL('P');
						break;
					case 'B': /* Down arrow key */
						code = CTRL('N');
						break;
					case 'C': /* Right arrow key */
						code = CTRL('F');
						break;
					case 'D': /* Left arrow key */
						code = CTRL('B');
						break;
					case '3':
						code = cgetchar(c);
						if (code == '~') {
							code = CTRL('D'); /* Del key */
						}
						break;
					default: /* Unsupported sequences */
						/* return code; */
						code = CTRL('G');
						break;
				}
				break;
			case 'O':
				code = cgetchar(c);
				switch (code) {
					case 'H': /* Home key */
						code = CTRL('A');
						break;
					case 'F': /* End key */
						code = CTRL('E');
						break;
				}
				break;
			default: /* Unsupported sequences */
				/* return code; */
				code = CTRL('G');
				break;
		}
	}
#ifdef uC
	else if (code == 0) {
		return 0;
	}
#ifdef REMOTE_CLI
	else if ((c->isRemote) && (code == IAC)) {
		/* Telnet's IAC commands, ignore them */
		code = cgetchar(c);
		if ((code >= SUB_NEGO_END) &&
				(code <= DONT)) {
			if (code == SUB_NEGO_START) {
				U32 i = 0;
				do {
					code = cgetchar(c);
					i++;
				} while ((code != SUB_NEGO_END) && (i < 10));
			} else {
				cgetchar(c);
			}
			code = '\0';
		}
	}
#endif
#endif
	return code;
}
#endif

/**
 * @function : exec_cmd
 * @param1   : c - Context Info
 * @return   : Return value of the command handler executed
 * @brief    : This function transforms the raw command string into argv, argc
 *             format, calls the corresponding command handler.
 * @caller   : cli_main
 */
static int exec_cmd (contextInfo_t *c)
{
	char **argv, *ptr;
	U32 i, argc, rc = 0;

	for (argc = 0, ptr = c->cmd_str; ptr != NULL; argc++) {
		ptr++;
		ptr = index (ptr, ' ');
	}

	argv = (char **) CALLOC (argc, sizeof (char *));
	if (argv == NULL) {
		CLI_ERROR ("%s:%d, calloc error\r\n", __func__, __LINE__);
		return EOT;
	}

	for (i = 0, ptr = c->cmd_str; i < argc; i++) {
		argv[i] = ptr;
		ptr = index (ptr, ' ');
		if (ptr) {
			*(ptr++) = '\0';
		} else {
			break;
		}
	}

	rc = (int) c->cmd_handler (argc, (char **) argv, c);
	free (argv);
#ifdef VERBOSE
	cprintf (c, "Command Execution - %s\r\n", rc ? "FAILED" : "SUCCESS");
#endif
	return rc;
}

/**
 * @function : cli_init
 * @param1   : sessionFd - File descriptor of the current session, 0 for local
 *             sessions in uC
 * @return   : Filled in contextInfo_t structure, NULL - Upon failure
 * @brief    : Initialize the contextInfo_t structure accordingly.
 * @caller   : main
 */
contextInfo_t *cli_init (U8 sessionFd)
{
	contextInfo_t *context = NULL;
	//#if defined uC && defined REMOTE_CLI
#if defined REMOTE_CLI && defined __linux__
	if (sessionFd) {
		context = (contextInfo_t *) CALLOC (1, sizeof(contextInfo_t));
		if (context == NULL) {
			CLI_ERROR ("%s:%d, calloc error\r\n", __func__, __LINE__);
			return NULL;
		}

		context->isRemote = 1;
		context->sessionFd = sessionFd;
		CLI_DEBUG ("sessionFd = %d\r\n", sessionFd);
		context->_cprintf = remote_cprintf;
		context->_cputchar = (void*)remote_cputchar;
		context->_cgetchar = remote_cgetchar;

		/* Send out the IAC commands to enable char mode */
		SEND_IAC_CMD (context, WILL, ECHO);
		SEND_IAC_CMD (context, DO, SUPPRESS_GO_AHEAD);
		SEND_IAC_CMD (context, WILL, SUPPRESS_GO_AHEAD);
	}
	else
#else /* #if defined uC && defined REMOTE_CLI */
	{
		context = &context_local;
		context->_cprintf = local_cprintf;
		context->_cputchar = local_cputchar;
		context->_cgetchar = local_cgetchar;
#ifdef __linux__
		context->sessionFd = sessionFd;
#endif
	}
#endif
	context->isAdmin = FALSE;
	context->isDebug = FALSE;

	return context;
}

int cli_online;

/**
 * @function : cli_main
 * @param1   : c - Context Info
 * @return   : 0 upon clean exit
 * @brief    : This is the main CLI loop.
 * @caller   : main
 */
int cli_main (contextInfo_t *c)
{
	char *cursor = NULL, *base_ptr = c->cmd_str;
	status_t rc = NOT_FOUND;
	size_t length = 0, raw_cmd_len = 0;
#ifdef CHARACTER_MODE
	U32 loop = TRUE, input = 0;
#endif

#ifdef uC /* Keil */
	os_itv_set (50);
#endif

	base_ptr[0] = '\0';
#if defined CHARACTER_MODE &&  defined __linux__
	set_keypress();
#endif
	while (cli_online) {
#ifdef uC /* Keil */
		os_itv_wait();
#endif
		c->cmd_handler = NULL;
		cursor = base_ptr;
		loop = TRUE;

		if (c->need_help) {
			c->need_help = FALSE;
			cputchar (c, '\r');
			show_cli (c);
			cursor += cprintf (c, "%s", base_ptr);
			*cursor = '\0'; /* Just in case */
			raw_cmd_len = strlen (base_ptr);
		} else {
			raw_cmd_len = 0;
			show_cli (c);
			*base_ptr = '\0';
		}

#ifdef CHARACTER_MODE
		while ((loop == TRUE) &&
				(raw_cmd_len < MAX_CLI_BUF_LEN)) {
			input = getcode(c);
			CLI_DEBUG ("GOTCHAR \"%c\", 0x%x\r\n", input, input);
			switch (input) {
				case '\0': /* Ignore NULL characters */
					loop = TRUE;
					*cursor = '\0';
#ifdef uC /* Keil */
					os_tsk_pass();
#endif
					break;
				case '\n': /* Enter key event */
				case '\r':
					cprintf (c, "\r\n");
					loop = FALSE;
					/* If cursor is not on the end of the string */
					if (*cursor != '\0') {
						cursor += strlen(cursor);
					}
					break;
				case '\t': /* C-i = Request completions (TAB) */
					c->need_help = REQUEST_COMPLETIONS;
					loop = FALSE;
					/* If cursor is not on the end of the string */
					if (*cursor != '\0') {
						/* Insert the '?' char at the position of the
						 * cursor, to mark the word on which the help was
						 * requested */
						insert_char (cursor++, '?');
					}
					break;
				case '?': /* Request help - '?' */
					cputchar (c, '?');
					c->need_help = REQUEST_HELP;
					loop = FALSE;
					/* If cursor is not on the end of the string */
					if (*cursor != '\0') {
						insert_char (cursor++, '?');
					}
					break;
				case CTRL('A'): /* C-a = Move cursor to the beginning of ths
						 * string */
					/* If cursor is not on the beginning of the string */
					if (cursor != base_ptr) {
						cprintf (c, "\033[%dD", (U32) (cursor - base_ptr));
						cursor = base_ptr;
					} else {
						cputchar (c, '\a');
					}
					break;
				case CTRL('B'): /* C-b = Move cursor left */
					/* If cursor is not on the beginning of the string */
					if (cursor != base_ptr) {
						cprintf (c, "\033[D");
						cursor--;
					} else {
						cputchar (c, '\a');
					}
					break;
				case CTRL('C'): /* C-c = Cancel input string */
					/* If cursor is not on the beginning of the string */
					cprintf (c, "^C\r\n");
					cursor = base_ptr;
					*cursor = '\0';
					show_cli (c);
					break;
				case CTRL('D'): /* C-d = Delete */
					/* If the cursor is at the begining of the string and
					 * no character has been entered, then consider it as
					 * an EOT */
					if ((cursor == base_ptr) && (*cursor == '\0')) {
						return (quit_cli (0, NULL, c));
					}
					/* If cursor is not on the end of the string */
					else if (*cursor != '\0') {
						cprintf (c, "\033[P");
						remove_char (cursor);
						raw_cmd_len--;
					} else {
						cputchar (c, '\a');
					}
					break;
				case CTRL('E'): /* C-e = Move cursor to the end of the string */
					/* If cursor is not on the end of the string */
					if (*cursor != '\0') {
						length = strlen(cursor);
						cprintf (c, "\033[%dC", (U32) length);
						cursor += length;
					} else {
						cputchar (c, '\a');
					}
					break;
				case CTRL('F'): /* C-f = Move cursor right */
					/* If cursor is not on the end of the string */
					if (*cursor != '\0') {
						cprintf (c, "\033[C");
						cursor++;
					} else {
						cputchar (c, '\a');
					}
					break;
#ifndef CMD_HISTORY
				case CTRL('P'): /* C-p = Previous command */
				case CTRL('N'): /* C-n = Next command */
#endif
				case CTRL('O'):
				case CTRL('R'):
				case CTRL('S'):
				case CTRL('T'):
				case CTRL('V'):
				case CTRL('W'):
				case CTRL('X'):
				case CTRL('Y'):
				case CTRL('Z'):
				case CTRL('G'): /* C-g = Audiable bell */
					cputchar (c, '\a');
					break;
				case CTRL('H'): /* C-h = Backspace */
				case 0x7f:
					/* If cursor is not on the beginning of the string */
					if (cursor != base_ptr) {
						cursor -= remove_char (cursor-1);
						cprintf (c, "\033[D\033[P");
						raw_cmd_len--;
					}
					break;
				case CTRL('K'): /* C-k = Del all char to the right of the
						 * cursor */
					/* If cursor is not on the end of the string */
					if (*cursor != '\0') {
						length = strlen(cursor);
						cprintf (c, "\033[%dP\033[K", length);
						*cursor = '\0';
						raw_cmd_len -= length;
					} else {
						cputchar (c, '\a');
					}
					break;
				case CTRL('L'): /* C-l = Clear the screen */
					cprintf (c, "\033[H\033[J");
					show_cli (c);
					cprintf (c, "%s", base_ptr);
					length = strlen (base_ptr);
					if (length) {
						U32 offset = base_ptr + length - cursor;
						if (offset) {
							cprintf (c, "\033[%dD", offset);
						}
					}
					break;
				case CTRL('U'): /* C-u = Del all char to the left of the
						 * cursor */
					/* If cursor is not on the beginning of the string */
					if (cursor != base_ptr) {
						/* Clear the line from beginning to current
						 * position of the cursor */
						cprintf (c, "\033[%dD\033[K", (U32) (cursor - base_ptr));
						remove_word (base_ptr, cursor);
						cursor = base_ptr;
						length = strlen (cursor);
						cprintf (c, "%s", cursor);
						if (length) {
							/* Move the cursor back to the start */
							cprintf (c, "\033[%dD", (U32) length);
							raw_cmd_len -= length;
						}
					} else {
						cputchar (c, '\a');
					}
					break;
				default:
					if (*cursor == '\0') {
						if(!c->sessionFd) {
							cputchar (c, input);
						}
						*cursor = input;
						*(cursor+1) = '\0';
					} else {
						insert_char (cursor, input);
						length = cprintf (c, "%s", cursor);
						cprintf (c, "\033[%dD", (U32) (length-1));
					}
					cursor++;
					raw_cmd_len++;
					loop = TRUE;
			}
		}

		if (raw_cmd_len >= MAX_CLI_BUF_LEN) {
			cprintf (c, "\r\nError: Too many charecters\a\r\n");
			continue;
		}

		/* Replace the trailing '\r\n' with a '\0' */
		if (c->need_help == FALSE) {
			*cursor = '\0';
		}  else if (c->need_help == REQUEST_COMPLETIONS) {
			raw_cmd_len = strlen (base_ptr);
		}
		length = squeeze_spaces (base_ptr);
		if ((cursor == base_ptr || (length == 0)) && (c->need_help == FALSE))
			continue;
#else /* CHARACTER_MODE */
		/* Buffred Mode */
		if (fgets (cursor, MAX_CLI_BUF_LEN, stdin) == NULL) {
			continue;
		}
		/* Replace the trailing '\r\n' with a '\0' */
		cursor[strlen(cursor)-1] = '\0';
		/* Squeeze out the spaces between words */
		length = squeeze_spaces (base_ptr);
		if (length == 0)
			continue;
		cursor = index (base_ptr, (int) '?');
		if (cursor) {
			c->need_help = REQUEST_HELP;
			*cursor = '\0';
		}
#endif /* CHARACTER_MODE */
		CLI_DEBUG ("squeezed = \"%s\"\r\n", base_ptr);
		if (c->need_help) {
			cursor = index (base_ptr, '?');
			if (cursor) {
				remove_char (cursor);
			} else {
				cursor = base_ptr + length;
			}
		}

#ifdef CMD_HISTORY
		/* History command implementation */
		else
		{
			Add_History(base_ptr);
		}
#endif

		/* Parse the command */
		rc = parse_cmd (cursor, c);
		if (rc == COMPLETED) {
			clear_line (raw_cmd_len, c);
		}
		if (rc == FOUND && c->need_help == FALSE) {
#ifdef VERBOSE
			cprintf (c, "SUCCESS\r\n");
#endif
			if (c->cmd_handler) {
				rc = (status_t) exec_cmd (c);
				if (rc == EOT)
					return 0;
			}
		}
#ifdef uC /* Keil */
		os_tsk_pass();
#endif
	}
	return (quit_cli(0, NULL, c));
}

/**
 * @function : cgetstr
 * @param1   : c - Context Info
 * @param2   : str - Pointer to the dst string
 * @param3   : echo - The character to print
 0 - Print the char read
 1 to 127 - Replace with ASCII char
 255 - Hidden
 * @param4   : max_len - Max. length of the string
 * @return   : Lenght of characters read
 * @brief    : Read a string from the user
 * @caller   : main
 */
U32 cgetstr (contextInfo_t *c, char *str, U8 echo, U32 max_len)
{
	char ch;
	U32 i = 0;
	U8 skip = FALSE;

	if (max_len > MAX_CLI_BUF_LEN) {
		CLI_ERROR ("Max Length(%d) is greater than MAX_CLI_BUF_LEN(%d)\r\n",
				max_len, MAX_CLI_BUF_LEN);
		return 0;
	}

	max_len = max_len ? max_len : MAX_CLI_BUF_LEN;
	do {
		while (0 == (ch = cgetchar (c))) {
#ifdef uC /* Keil */
			os_task_pass();
#endif
		}
		if (skip == FALSE) {
			str[i] = ch;
		}
		if (echo != HIDDEN_CHAR) {
			cprintf (c, "%c", (((echo < 128) && (echo > 0)) ? echo : ch));
		}
		if ((i >= max_len) && (skip == FALSE)) {
			str[i-1] = '\0';
			skip = TRUE;
		}
		if (i >= MAX_CLI_BUF_LEN) {
			break;
		}
		i++;
	} while ((ch != '\r') && (ch != '\n') && (ch != CTRL('C'))
			&& (ch != CTRL('G')) && (ch != CTRL('D')));
	str[i-1] = '\0';
	return i;
}

pthread_t cli_tid;

void *cli(void *arg) 
{
	void *ret = NULL;

	set_thread_affinity (3, pthread_self ());
	telnetd_task();
	pthread_join (cli_tid, NULL);

	return ret;
}

int init_cli() {

	int ret = 0;

	if((ret = pthread_create(&cli_tid, NULL, cli, NULL) != 0)) {
		perror("pthread_create");
		return -1;
	}
	return 0;
}
#if 0 /* nvme integration changes */
#ifdef __linux__
/**
 * @function : main
 * @param1   : argc
 * @param2   : argv
 * @return   : Return code
 * @brief    : The main function.
 */
int main (int argc, char *argv[])
{
	contextInfo_t *context = cli_init(0);
	if (context) {
		return cli_main (context);
	} else {
		return 1;
	}
}
#endif
#endif

#ifdef CMD_HISTORY
void Add_History(char *string)
{
	static char max_his =0;
	add_his_st  *new= NULL;

	if(max_his < MAX_HISTORY) {
		max_his++;
		if((new = malloc(sizeof(add_his_st))) == 0) {
			printf("error in dynamic memory allocation...\n");
			exit(1);
		}
		strcpy(new->commd_ptr,string);

		if(head == 0) { /* if the list is empty initially */
			head = new;
			new->pre = new;
			new->post = new;
		} else { /* to insert at the start of the list */
			new->post = head;
			new->pre = new->post->pre;
			head = new;
			new->pre->post = new->post->pre = new;
		}
	} else { /* to get the last entered at 1st and remove 1st entry
		    when no. of commands > 10 */
		head = head->pre;
		strcpy(head->commd_ptr,string);

	}
}

int print_history(U32 argc, char **argv, contextInfo_t *c )
{
	int i=0;
	add_his_st *temp;
	temp = head;
	temp = temp->pre;

	do{
		printf("%d----------- %s\n",i,temp->commd_ptr);
		i++;
		temp = temp->pre;
	}while(temp != head->pre);
	return 0;
}
#endif /* CMD_HISTORY */

