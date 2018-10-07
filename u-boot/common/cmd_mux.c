/******************************************************************************
 **
 ** SRC-MODULE   : cmd_mux.c
 **
 ** Copyright(C) : VVDN Tech (C)
 **
 ** TARGET       : 
 **
 ** PROJECT      : FSLU_NVME
 **
 ** AUTHOR       : 
 **
 ** PURPOSE      : file contains U-boot command access for mux by using i2c
 **
 **
 ******************************************************************************/

#include <common.h>
#include <config.h>
#include <command.h>
#include <i2c.h>

/*-------------------- Macros---------------------------------------------------- */
#define MUX1_ADDR 	0x77  	/* Slave address of Mux1 */
#define MUX2_ADDR 	0x75	/* Slave address of Mux2 */
#define MUX2_SEL_ADDR 	0x00	
#define MUX2_SEL_DATA	0x0e   /* Encode value to select mux2 */
#define FAILURE 	-1
#define SUCCESS 	0
/*---------------------------------------------------------------------------*/

/*--------------Extern Declarations--------------------------------------------*/
int  mux_read  (unsigned mux_addr,unsigned reg_addr);
int  mux_write (unsigned mux_addr,unsigned reg_addr, uint8_t val);
/*---------------------------------------------------------------------------*/


static int do_mux(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong reg;
	uint8_t data;
	char ret;
	if (argc <= 3 || argc >=6 ) {
		printf("Atleast 2 argument is needed\n");
		return FAILURE;
	}

	reg = simple_strtoul(argv[3], NULL, 0);  /* Getting the register Address */
	data = simple_strtoul(argv[4], NULL, 0); /* Getting the data */

	if(strcmp (argv[1], "1") == 0) {

		ret = i2c_probe(MUX1_ADDR);
                if(ret) {
                        printf(" %s %s is not detected\n",argv[0],argv[1]);
                        return -1;
                }
	
		if (strcmp (argv[2], "read") == 0) {
			int rcode;

			printf ("mux %s %s %s = ",argv[1],argv[2],argv[3]);
			rcode = mux_read(MUX1_ADDR,reg);   

			return rcode;
		} else if (strcmp (argv[2], "write") == 0) {
			int rcode;

			printf ("mux %s %s %s %s\n",argv[1],argv[2],argv[3],argv[4]);
			rcode = mux_write (MUX1_ADDR,reg,data);

			return rcode;
		} else 
			printf("Unknown arguments \n");	
	} else if (strcmp (argv[1], "2") == 0) {

		mux_write(MUX1_ADDR,MUX2_SEL_ADDR,MUX2_SEL_DATA); /* Selecting Mux2 from Mux1 */
		
		ret = i2c_probe(MUX2_ADDR);
                if(ret) {
                        printf(" %s %s is not detected\n",argv[0],argv[1]);
                        return -1;
                }

		if (strcmp (argv[2], "read") == 0) {
			int rcode;

			printf ("mux %s %s %s = ",argv[1],argv[2],argv[3]);
			rcode = mux_read (MUX2_ADDR,reg);

			return rcode;
		} else if (strcmp (argv[2], "write") == 0) {
			int rcode;

			printf ("mux %s %s %s %s\n",argv[1],argv[2],argv[3],argv[4]);
			rcode = mux_write (MUX2_ADDR,reg,data);

			return rcode;
		} else
			printf("Unknown arguments \n");
	}

	return CMD_RET_USAGE;
}

/*************************************************************************************
 ** function name	:       mux_read
 ** arguments type   	:       unsigned int and unsigned int
 ** return type  	:       int
 ** Description  	:       This function is used to read the 1 byte data 
 **				from mux register 
 **************************************************************************************/

int mux_read(unsigned mux_addr,unsigned reg_addr)
{
	uint8_t val;
	int rcode = 0;

	if(i2c_read(mux_addr,reg_addr,1, &val,1)){
		printf("Mux read error\n");
		rcode = 1;
	}
	printf("%x\n",val);

	return rcode;
}

/**************************************************************************************
 ** function name	:       mux_write
 ** arguments type   	:       unsigned int and unsigned int
 ** return type  	:       int
 ** Description  	:       This function is used to write 1 byte data 
 **				to mux register
 **********************************************************************************/

int mux_write(unsigned mux_addr,unsigned reg_addr,uint8_t val)
{
	int rcode = 0;

	if(i2c_write(mux_addr,reg_addr,1, &val,1)) {
		printf("Mux write error\n");
		rcode = 1;
	}

	return rcode;
}

U_BOOT_CMD(mux,5,1,do_mux,"uboot mux command "
		,"mux <num> <read> <reg>\n mux <num> <write> <reg> <data> ");
