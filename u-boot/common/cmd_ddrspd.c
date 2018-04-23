/******************************************************************************
 **
 ** SRC-MODULE   : cmd_ddrspd.c
 **
 ** Copyright(C) : VVDN Tech (C)
 **
 ** TARGET       : 
 **
 ** PROJECT      : FSLU_NVME
 **
 ** AUTHOR       : 
 **
 ** PURPOSE      : file contains U-boot command access for DDR-SODIMM1/2
 **                     by using i2c
 **
 ******************************************************************************/

#include <common.h>
#include <config.h>
#include <command.h>
#include <i2c.h>

/*-------------------- Macros---------------------------------------------------- */
#define MUX1_ADDR               0x77    /* Slave address of Mux1 */
#define MUX1_CHENNEL0_ADDR      0x00    
#define MUX1_CHANNEL0_SEL       0x08    /* Selecting Channel 0 Mux1 */
#define DDR_SODIMMI1            0x51    /* Slave address of DDR-DIMMI1 */ 
#define DDR_SODIMMI2            0x52    /* Slave address of DDR-DIMMI2 */ 
#define DISP_LINE_LEN           16
#define FAILURE                 -1
#define SUCCESS                 0
/*---------------------------------------------------------------------------*/

/*--------------Extern Declarations--------------------------------------------*/
int  ddrspd_dump(unsigned ddrspd_addr,unsigned reg_addr,unsigned len);
extern int  mux_write (unsigned mux_addr,unsigned reg_addr, uint8_t data);
/*---------------------------------------------------------------------------*/

static int do_ddrspd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong reg_addr,len;
	char ret;
	if (argc !=5 ) {
		printf("Atleast 2 argument is needed\n");
		return FAILURE;
	}
	mux_write(MUX1_ADDR,MUX1_CHENNEL0_ADDR,MUX1_CHANNEL0_SEL);
	reg_addr = simple_strtoul(argv[3], NULL, 0);  /* Getting the register Address */
	len      = simple_strtoul(argv[4], NULL, 0); /* Getting the length */
	if(strcmp (argv[1], "1") == 0) {
		ret = i2c_probe(DDR_SODIMMI1);
		if(ret) {
			printf(" %s %s is not detected\n",argv[0],argv[1]);
			return -1;
		}

		if (strcmp (argv[2], "dump") == 0) {
			int rcode;

			printf ("ddrspd %s %s %s %s\n",argv[1],argv[2],argv[3],argv[4]);
			rcode = ddrspd_dump(DDR_SODIMMI1,reg_addr,len);

			return rcode;
		} else 
			printf("Unknow arguments \n");	
	} else if (strcmp (argv[1], "2") == 0) {
		
		ret = i2c_probe(DDR_SODIMMI2);
                if(ret) {
                        printf(" %s %s is not detected\n",argv[0],argv[1]);
                        return -1;
                }

		if (strcmp (argv[2], "dump") == 0) {
			int rcode;

			printf ("ddrspd %s %s %s %s\n ",argv[1],argv[2],argv[3],argv[4]);
			rcode = ddrspd_dump(DDR_SODIMMI2,reg_addr,len);

			return rcode;

		} else
			printf("Unknow arguments \n");		
	}

	return CMD_RET_USAGE;
}

/*************************************************************************************
 ** function name       :       ddrspd_dump
 ** arguments type      :       unsigned int,unsigned int,unsigned int
 ** return type         :       int
 ** Description         :       This function is used to read the n bytes data 
 **                             from ddrspd register 
 **************************************************************************************/

int ddrspd_dump(unsigned ddrspd_addr,unsigned reg_addr,unsigned len)
{
	int rcode = 0;
	unsigned int nbytes,linebytes,ret,j;
	nbytes = len;
	do {
		unsigned char   linebuf[DISP_LINE_LEN];
		unsigned char   *cp;

		linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;
		ret = i2c_read(ddrspd_addr, reg_addr,1, linebuf, linebytes);
		if (ret) {
			printf("ddrspd i2c read error\n");
			rcode = 1;
		}  else {
			printf("%04x:", reg_addr);      /*Displaying the data byte line by line */
			cp = linebuf;
			for (j=0; j<linebytes; j++) {
				printf(" %02x", *cp++);
				reg_addr++;
			}
			puts ("    ");
			cp = linebuf;
			for (j=0; j<linebytes; j++) {
				if ((*cp < 0x20) || (*cp > 0x7e))
					puts (".");
				else
					printf("%c", *cp);
				cp++;
			}
			putc ('\n');
		}
		nbytes -= linebytes;
	} while (nbytes > 0);
	return rcode;
}

U_BOOT_CMD(ddrspd,5,1,do_ddrspd,"ddrspd uboot command "
		,"ddrspd <slot_num> <dump> <addr> <len> \n");
