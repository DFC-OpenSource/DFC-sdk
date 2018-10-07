#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "cli_parser.h"
#include "telnetd.h"
#include <syslog.h>
U32 gClients = 0;
//extern __task void cli_task (void *);
extern uint8_t nvmeKilled;

int telnetd_task (void)
{
	struct sockaddr_in addr; 
	char sendBuff[1025];
	int socke, ret;
	static int sd = 0;

	socke = socket (AF_INET, SOCK_STREAM, 0);
	if (socke > 0) {
		CLI_DEBUG ("Socket created\r\n");
		syslog(LOG_DEBUG,"Socket created\n");
	} else {
		CLI_ERROR ("socket not created\r\n");
		syslog(LOG_ERR,"socket creation failed\n");
		return -1;
	}
	memset(&addr, '0', sizeof(addr));
	memset(sendBuff, '0', sizeof(sendBuff));

	addr.sin_port        = htons(5000);
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind (socke, (struct sockaddr *)&addr, sizeof(addr));
	//perror("bind");
	if (ret == 0) {
		syslog(LOG_DEBUG,"CLI: Bound successfully\n");	
		CLI_DEBUG ("BOUND\r\n");
	} else {
		CLI_ERROR ("Bind failed: %d\r\n",ret);
		syslog(LOG_ERR,"CLI: Bind failed\n");
		return -1;
	}

	CLI_DEBUG ("listening........\r\n");
	syslog(LOG_DEBUG,"Listening\n");
	ret = listen (socke, MAX_REMOTE_CLIENTS);
	//perror("listen");
	syslog(LOG_DEBUG,"Listen done successfully\n");
	if (ret < 0) {
		CLI_ERROR ("Listen failed\r\n");
		syslog(LOG_ERR,"CLI: Listen failed\n");
		return -1;
	}

	while (!nvmeKilled) {
		/* Take mutex */
#if 0
		if (gClients == MAX_REMOTE_CLIENTS) {
			/* Release mutex */
			break;
		}
#endif
		sd = accept (socke, NULL, NULL);
		perror("accept");
		syslog(LOG_DEBUG,"CLI: Accepted connection\n");
		if ((sd < 0)) {
			CLI_ERROR ("Error accepting connection.\r\n");
			syslog(LOG_ERR,"CLI: Accept failed\n");
			return -1;
		}
#if 0
		gClients++;
#endif
		CLI_DEBUG ("ACCEPTED. Clients = %d, fd = %d\r\n", gClients, sd);
		contextInfo_t *context = cli_init(sd);
		cli_online = 1;
		if (context) {
			cli_main (context);
		} 

#if 0
		os_tsk_create_ex (cli_task, CLI_TASK_PRIO, (void *) sd);
		CLI_DEBUG ("\r\nSpawned task\r\n");
#endif
		/* Release mutex */
	}
	return 0;
}

void close_sock (int fd)
{
	/* Take mutex */
	close(fd);
#if 0
	gClients--;
#endif
	/* Release mutex */
}
