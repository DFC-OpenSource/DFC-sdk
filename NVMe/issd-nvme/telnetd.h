#ifndef _TELNETD_H
#define _TELNETD_H   1

#if defined REMOTE_CLI
int telnetd_task(void);
void close_sock(int fd);
//#endif

//#if defined uC && defined REMOTE_CLI

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdio.h>
#include "cli_parser.h"
//#include "vasprintf.h"
#include "iac.h"
//#include <rtl.h>

extern int cli_online;
extern int quit_cli (U32 argc, char **argv, contextInfo_t *c);

//__task void telnetd_task (void);
//void close_sock (int fd);

#define SEND_IAC_CMD(c,x,y) do {                                        \
	int ret = 0;                                                    \
	U32 cmd = htonl(IAC_CMD((x),(y)));                              \
	if ((c)->sessionFd) {                                           \
		ret = send ((c)->sessionFd, (char *)&cmd, sizeof(cmd), 0);  \
		if (ret < 0) {                                              \
			CLI_ERROR ("Send failed, fd=%d, ret=%d\r\n",            \
						c->sessionFd, ret);                          \
			cli_online = 0; \
			quit_cli (0, NULL, c);                                  \
		}                                                           \
	} else {                                                        \
		cli_online = 0; \
		return (contextInfo_t *)(intptr_t)(quit_cli (0, NULL, c));                             \
	}                                                               \
} while(0)

static int remote_cprintf (contextInfo_t *c, const char *fmt, ...)
{
	va_list argp;
	int ret = 0;
	char *buff;

	if (!(c->sessionFd)) {
		/* Clean up & exit */
		return (quit_cli (0, NULL, c));
	}

	va_start (argp, fmt);
	ret = vasprintf (&buff, fmt, argp);
	va_end (argp);
	ret = send (c->sessionFd,(void *) buff, strlen(buff), 0);
	// perror("send");
	if (ret < 0) {
		free (buff);
		CLI_ERROR ("Send failed, fd=%d, ret=%d\r\n",
				c->sessionFd, ret);
		/* Clean up & exit */
		cli_online = 0;
		quit_cli (0, NULL, c);
		return ret;
	}
	free (buff);
	return ret;
}

static int remote_cputchar (contextInfo_t *c, char ch)
{
	int ret = 0;
	if (!(c->sessionFd)) {
		/* Clean up & exit */
		cli_online = 0;
		return (quit_cli (0, NULL, c));
	}

	ret = send (c->sessionFd, &ch, 1, 0);
	//perror("send");
	if (ret < 0) {
		CLI_ERROR ("Send failed, fd=%d, ret=%d\r\n",
				c->sessionFd, ret);
		/* Clean up & exit */
		cli_online = 0;
		quit_cli (0, NULL, c);
		return ret;
	}
	return ((ret) ? EOF : ch);
}

static char remote_cgetchar (contextInfo_t *c)
{
	int ret = 0;
	char ch;

	if (!(c->sessionFd)) {
		/* Clean up & exit */
		cli_online = 0;
		return (quit_cli (0, NULL, c));
	}

	ret = recv (c->sessionFd, &ch, 1 , 0);
	// perror("recv");
	if (ret < 0) {
		CLI_ERROR ("Recv failed, fd=%d, ret=%d\r\n",
				c->sessionFd, ret);
		/* Clean up & exit */
		cli_online = 0;
		quit_cli (0, NULL, c);
		return ret;
	}
	return ch;
}

#endif
#endif
