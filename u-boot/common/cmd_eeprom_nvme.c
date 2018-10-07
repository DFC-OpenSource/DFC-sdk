/******************************************************************************
 **
 ** SRC-MODULE   : cmd_eeprom_nvme.c
 **
 ** Copyright(C) : VVDN Tech (C)
 **
 ** TARGET       : 
 **
 ** PROJECT      : FSLU_NVME
 **
 ** AUTHOR       : 
 **
 ** PURPOSE      : file contains U-boot command access for eeprom (sysid/rcw) 
 **			by using i2c
 **
 **
 ******************************************************************************/

#include <common.h>
#include <config.h>
#include <command.h>
#include <i2c.h>

/*-------------------- Macros---------------------------------------------------- */
#define MUX1_ADDR       	0x77	/* Slave address of Mux1 */
#define MUX1_CHENNAL0_ADDR   	0x00	
#define MUX1_CHANNEL0_SEL	0x08    /* Selecting Channel 0 Mux1 */
#define EEPROM_SYSID		0x57	/* Slave address of eeprom sysid */
#define EEPROM_RCW		0x50  	/* Slave address of eeprom rcw */ 
#define DISP_LINE_LEN   	16
#define FAILURE         	-1
#define SUCCESS         	0
/*---------------------------------------------------------------------------*/

/*--------------Extern Declarations--------------------------------------------*/
int  eeprom_dump(unsigned eeprom_addr,unsigned reg_addr,unsigned len);
extern int  mux_write (unsigned mux_addr,unsigned reg_addr, uint8_t data);
/*---------------------------------------------------------------------------*/

static int do_eeprom_nvme(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong reg_addr,len;
	char ret;
	if (argc != 5 ) {
		printf("Atleast 2 argument is needed\n");
		return FAILURE;
	}
	mux_write(MUX1_ADDR,MUX1_CHENNAL0_ADDR,MUX1_CHANNEL0_SEL);
	reg_addr = simple_strtoul(argv[3], NULL, 0);  /* Getting the register Address */
	len 	 = simple_strtoul(argv[4], NULL, 0); /* Getting the length */
	if(strcmp (argv[1], "sysid") == 0) {
		
		ret = i2c_probe(EEPROM_SYSID);
                if(ret) {
                        printf(" %s %s is not detected\n",argv[0],argv[1]);
                        return -1;
                }

		if (strcmp (argv[2], "dump") == 0) {
			int rcode;

			printf ("eeprom_nvme %s %s %s %s\n ",argv[1],argv[2],argv[3],argv[4]);
			rcode = eeprom_dump(EEPROM_SYSID,reg_addr,len);

			return rcode;
		} else if (strcmp (argv[2], "prog") == 0) {
			int rcode = 0;

			printf ("TBD\n");

			return rcode;
		} else
			printf("Unknown arguments \n"); 
	}else if (strcmp (argv[1], "rcw") == 0) {
		
		ret = i2c_probe(EEPROM_RCW);
                if(ret) {
                        printf(" %s %s is not detected\n",argv[0],argv[1]);
                        return -1;
                }

		if (strcmp (argv[2], "dump") == 0) {
			int rcode;

			printf ("eeprom_nvme %s %s %s %s\n ",argv[1],argv[2],argv[3],argv[4]);
			rcode = eeprom_dump(EEPROM_RCW,reg_addr,len);

			return rcode;
		} else if (strcmp (argv[2], "prog") == 0) {
			int rcode = 0;

			printf ("TBD\n");

			return rcode;	
		} else 
			printf("Unknown arguments \n");

	}

	return CMD_RET_USAGE;
}

/*************************************************************************************
 ** function name       :       eeprom_dump
 ** arguments type      :       unsigned int,unsigned int,unsigned int
 ** return type         :       int
 ** Description         :       This function is used to read the n bytes data 
 **                             from eeprom register 
 **************************************************************************************/

int eeprom_dump(unsigned eeprom_addr,unsigned reg_addr,unsigned len)
{
	int rcode = 0;
	unsigned int nbytes,linebytes,ret,j;
	nbytes = len;
	do {
		unsigned char   linebuf[DISP_LINE_LEN];
		unsigned char   *cp;

		linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes; 
		ret = i2c_read(eeprom_addr, reg_addr,1, linebuf, linebytes); 
		if (ret) {
			printf("eeprom i2c read error\n");
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

U_BOOT_CMD(eeprom_nvme,5,1,do_eeprom_nvme,"eeprom_nvme uboot command "
		,"eeprom <sysid/rcw> <dump> <addr> <len> ");  	
