/******************************************************************************
 **
 ** SRC-MODULE   : cmd_temp.c
 **
 ** Copyright(C) : VVDN Tech (C)
 **
 ** TARGET       : 
 **
 ** PROJECT      : FSLU_NVME
 **
 ** AUTHOR       : 
 **
 ** PURPOSE      : file contains U-boot command to access temperature
 **			sensor by using i2c.
 **
 ******************************************************************************/

#include <common.h>
#include <config.h>
#include <command.h>
#include <i2c.h>

/*-------------------- Macros---------------------------------------------------- */
#define MUX1_ADDR       	0x77	/* Slave address of Mux1 */
#define MUX2_ADDR       	0x75	/* Slave address of Mux2 */
#define REG_ADDR	   	0x00    
#define CHANNEL6	   	0x0e   	/*Encode data to select Mux2 from Mux1 */
#define CHANNEL3		0x0b  	/* Encode data to select temp sensor 1 from Mux2 */	   
#define CHANNEL4 		0x0c  	/* Encode data to select temp sensor 2 from Mux2 */
#define TEMP_ADDR		0x4c   	/* Slave address of temp sensor */
#define FAILURE         	-1
#define SUCCESS         	0
/*---------------------------------------------------------------------------*/

/*--------------Extern Declarations--------------------------------------------*/
int  temp_read(unsigned mux_addr,unsigned reg_addr);
int  temp_write(unsigned mux_addr,unsigned reg_addr, uint8_t val);
extern int  mux_write (unsigned mux_addr,unsigned reg_addr, uint8_t data);
/*---------------------------------------------------------------------------*/


static int do_temp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong reg_addr;
	uint8_t data;
	char ret;
	if (argc <= 3 || argc >=6 ) {
		printf("Atleast 2 argument is needed\n");
		return FAILURE;
	}
	reg_addr = simple_strtoul(argv[3], NULL, 0);  /* Getting the register Address */
	data 	 = simple_strtoul(argv[4], NULL, 0); /* Getting the data */
	if(strcmp (argv[1], "nic1") == 0) {

		mux_write(MUX1_ADDR,REG_ADDR,CHANNEL3); /* Selecting the temp sensor 1 from Mux2 */
		
		 ret = i2c_probe(TEMP_ADDR);
                if(ret) {
                        printf(" %s %s is not detected\n",argv[0],argv[1]);
                        return FAILURE;
                }
		
		if (strcmp (argv[2], "read") == 0) {
			int rcode;
			rcode = temp_read(TEMP_ADDR,reg_addr);
			return rcode;
		} else if (strcmp (argv[2], "write") == 0) {
			int rcode;
			rcode = temp_write (TEMP_ADDR,reg_addr,data);
			return rcode;
		} else
			printf("Unknown arguments \n"); 
	} else if (strcmp (argv[1], "nic2") == 0) {

		mux_write(MUX1_ADDR,REG_ADDR,CHANNEL4); /* Selecting the temp sensor 2 from Mux2 */
		
		  ret = i2c_probe(TEMP_ADDR);
                if(ret) {
                        printf(" %s %s is not detected\n",argv[0],argv[1]);
                        return FAILURE;
                }

		if (strcmp (argv[2], "read") == 0) {
			int rcode;
			rcode = temp_read (TEMP_ADDR,reg_addr);
			return rcode;
		} else if (strcmp (argv[2], "write") == 0) {
			int rcode;
			rcode = temp_write (TEMP_ADDR,reg_addr,data);
			return rcode;
		} else
			printf("Unknown arguments \n");
	} 
	return CMD_RET_USAGE;
}

/*************************************************************************************
 ** function name       :       temp_read
 ** arguments type      :       unsigned int and unsigned int
 ** return type         :       int
 ** Description         :       This function is used to read the 1 byte data 
 **                             from temp sensor register 
 **************************************************************************************/

int temp_read(unsigned temp_addr,unsigned reg_addr)
{
	uint8_t val;
	int rcode = 0;

	if(i2c_read(temp_addr,reg_addr,1, &val,1)){
		printf("Mux read error\n");
		rcode = 1;
	}
	printf("Read Value : %02x\n",val);

	return rcode;
}

/**************************************************************************************
 ** function name       :       temp_write
 ** arguments type      :       unsigned int and unsigned int
 ** return type         :       int
 ** Description         :       This function is used to write 1 byte data 
 **                             to temp sensor register
 **********************************************************************************/

int temp_write(unsigned temp_addr,unsigned reg_addr,uint8_t val)
{
	int rcode = 0;

	if(i2c_write(temp_addr,reg_addr,1, &val,1)) {
		printf("Mux write error\n");
		rcode = 1;
	}

	return rcode;
}

U_BOOT_CMD(temp,5,1,do_temp,"temp sensor access by using i2c"
		,"temp <nic1/2> <read> <reg>\ntemp <nic1/2> <write> <reg> ");
