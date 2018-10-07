#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <linux/types.h>
#include <time.h>

#define VENDOR_ID	"1172"
#define DEVICE_ID	"e003"

/*-----BAR Register offsets for different Perpose---*/
#define CTRL_REG		0x00
#define EPCQ_ID_REG		0x01
#define EPCQ_ST_REG		0x02
#define EPCQ_WR_DATA_REG	0X03
#define EPCQ_RD_DATA_REG	0X04
#define EPCQ_ADDR_REG		0X05
#define NO_OF_BYT_REG		0x06

/*--- Control & Status Register Bit Field----*/
#define ASMI_RST		0
#define BLK_ERS			1
#define EPCQ_ST			2
#define EPCQ_ID			3
#define EPCQ_RD			4
#define EPCQ_WR			5
#define EN_ADDR_MODE	6
#define	RD_DUMMY		7

#define PREDEFINE_ID	0x19
