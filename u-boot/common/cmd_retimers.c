/******************************************************************************
 **
 ** SRC-MODULE   : cmd_retimers.c
 **
 ** Copyright(C) : VVDN Tech (C)
 **
 ** TARGET       : 
 **
 ** PROJECT      : FSLU_NVME
 **
 ** AUTHOR       : 
 **
 ** PURPOSE      : file contains U-boot command access retimers by using i2c
 **
 **
 ******************************************************************************/

#include <common.h>
#include <config.h>
#include <command.h>
#include <i2c.h>

/*-------------------- Macros---------------------------------------------------- */
#define MUX1_ADDR       	0x77    	/* Slave address of Mux1 */
#define RETIMERS1_TX_ADDR	0x18		/* Slave address of retimers1 tx */
#define RETIMERS1_RX_ADDR	0x19		/* Slave address of retimers1 rx */
#define RETIMERS2_TX_ADDR	0x1A		/* Slave address of retimers2 tx */
#define RETIMERS2_RX_ADDR	0x1B		/* Slave address of retimers2 rx */
#define MUX1_REG_ADDR		0x00            /* Mux reg address to select retimers */
#define MUX1_CHANNEL5		0x0d	       /* Encode value to select retimers */
#define FAILURE         	-1
#define SUCCESS         	0
/*---------------------------------------------------------------------------*/

/*--------------Extern Declarations--------------------------------------------*/
int  retimers_read  (unsigned retimers_addr,unsigned reg_addr);
extern int  mux_write (unsigned mux_addr,unsigned reg_addr, uint8_t data);
/*---------------------------------------------------------------------------*/

static int do_retimers(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong reg;
	char ret;
	if (argc !=5) {
		printf("Atleast 2 argument is needed\n");
		return FAILURE;
	}

	reg = simple_strtoul(argv[4], NULL, 0);  /* Getting the register Address */
	mux_write(MUX1_ADDR,MUX1_REG_ADDR,MUX1_CHANNEL5);

	if(strcmp (argv[1], "1") == 0) {

		if(strcmp (argv[2], "tx") == 0) {
			
			ret = i2c_probe(RETIMERS1_TX_ADDR);
	                if(ret) {
         	               printf(" %s %s is not detected\n",argv[0],argv[1]);
                	       return -1;
                	}

			if (strcmp (argv[3], "read") == 0) {
				int rcode;

				printf ("retimers %s %s %s %s = ",argv[1],argv[2],argv[3],argv[4]);
				rcode = retimers_read(RETIMERS1_TX_ADDR,reg);

				return rcode;
			} else
				printf("Unknown arguments \n");
		}else if(strcmp (argv[2], "rx") == 0) {
			
			ret = i2c_probe(RETIMERS1_RX_ADDR);
	                if(ret) {
                        	printf(" %s %s is not detected\n",argv[0],argv[1]);
                        	return -1;
                	}

			if (strcmp (argv[3], "read") == 0) {
				int rcode;

				printf ("retimers %s %s %s %s = ",argv[1],argv[2],argv[3],argv[4]);
				rcode = retimers_read(RETIMERS1_RX_ADDR,reg);

				return rcode;
			} else 
				printf("Unknown arguments \n");
		}
	} else if (strcmp (argv[1], "2") == 0) {

		if(strcmp (argv[2], "tx") == 0) {
			
			ret = i2c_probe(RETIMERS2_TX_ADDR);
	                if(ret) {
        	                printf(" %s %s is not detected\n",argv[0],argv[1]);
                	        return -1;
                	}

			if (strcmp (argv[3], "read") == 0) {
				int rcode;

				printf ("retimers %s %s %s %s = ",argv[1],argv[2],argv[3],argv[4]);
				rcode = retimers_read(RETIMERS2_TX_ADDR,reg);

				return rcode;	
			} else 
				printf("Unknown arguments \n");
		}else if(strcmp (argv[2], "rx") == 0) {
		
			ret = i2c_probe(RETIMERS2_RX_ADDR);
	                if(ret) {
                        	printf(" %s %s is not detected\n",argv[0],argv[1]);
                        	return -1;
                	}

			if (strcmp (argv[3], "read") == 0) {
				int rcode;

				printf ("retimers %s %s %s %s = ",argv[1],argv[2],argv[3],argv[4]);
				rcode = retimers_read(RETIMERS2_RX_ADDR,reg);

				return rcode;
			} else 
				printf("Unknown arguments \n"); 
		}
	}

	return CMD_RET_USAGE;
}


/*************************************************************************************
 ** function name       :       retimers_read
 ** arguments type      :       unsigned int and unsigned int
 ** return type         :       int
 ** Description         :       This function is used to read the 1 byte data 
 **                             from retimers register 
 **************************************************************************************/

int retimers_read(unsigned retimers_addr,unsigned reg_addr)
{
	uint8_t val;
	int rcode = 0;

	if(i2c_read(retimers_addr,reg_addr,1, &val,1)){
		printf("retimers read error\n");
		rcode = 1;
	}
	printf("%x\n",val);

	return rcode;
}

U_BOOT_CMD(retimers,5,1,do_retimers,"uboot retimers command "
		,"retimers <1> <tx/rx> <read> <reg>\n retimers <2> <tx/rx> <read> <reg>\n");
