#ifndef _IAC_H
#define _IAC_H    1

#define SUPPRESS_GO_AHEAD   0x03
#define STATUS              0x05
#define ECHO                0x01
#define TIMING_MARK         0x06
#define TERMINAL_TYPE       0x18
#define WINDOW_SIZE         0x1F
#define TERMINAL_SPEED      0x20
#define REMOTE_FLOW_CONTROL 0x21
#define LINE_MODE           0x22
#define ENV_VARIABLES       0x24

#define SUB_NEGO_END        0xF0
#define NOP                 0xF1
#define DATA_MARK           0xF2
#define BREAK               0xF3
#define INTERRUPT           0xF4
#define ABORT_OUTPUT        0xF5
#define ARE_YOU_THERE       0xF6
#define ERASE_CHARACTER     0xF7
#define ERASE_LINE          0xF8
#define GO_AHEAD            0xF9
#define SUB_NEGO_START      0xFA

#define WILL                0xFB
#define WONT                0xFC
#define DO                  0xFD
#define DONT                0xFE

#define IAC                 0xFF

#define IAC_CMD(x,y) ((IAC<<16)+((x)<<8)+(y))

#define TCP_SEND_IAC_CMD(soc,x,y) do {             \
	U8 *sendbuf;                               \
	U32 cmd = htonl (IAC_CMD((x),(y)));        \
	U32 maxlen = sizeof(cmd);                  \
	if (soc) {                                 \
		sendbuf = tcp_get_buf (maxlen);        \
		memcpy (sendbuf, (U8 *)&cmd, maxlen);  \
		tcp_send ((soc), sendbuf, maxlen);     \
	}                                          \
} while(0)

/* Usage: TCP_SEND_IAC_CMD (tcp_soc, WILL, ECHO) */

#endif
