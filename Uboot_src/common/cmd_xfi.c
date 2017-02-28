/******************************************************************************
 **
 ** SRC-MODULE   : cmd_xfi.c
 **
 ** Copyright(C) : VVDN Tech (C)
 **
 ** TARGET       : 
 **
 ** PROJECT      : FSLU_NVME
 **
 ** AUTHOR       : 
 **
 ** PURPOSE      : file contains U-boot command access for xfi 
 **			by using i2c
 **
 **
 ******************************************************************************/

#include <common.h>
#include <config.h>
#include <command.h>
#include <i2c.h>

/*-------------------- Macros---------------------------------------------------- */
#define MUX1_ADDR 	0x77
#define MUX2_ADDR 	0x75
#define REG_ADDR 	0x00
#define CHANNEL0 	0x08
#define CHANNEL1 	0x09
#define CHANNEL2 	0x0a
#define CHANNEL3 	0x0b
#define CHANNEL6 	0x0e
#define XFI_ADDR 	0x50
#define XFI_MONI_ADDR 	0x51

#define LOSS_REG 	0x65
#define LOSS_MASK 	0x0A
#define REC_XFI 	0x6E
#define REC_MASK 	0x02


#define FAILURE -1
#define SUCCESS 0
/*---------------------------------------------------------------------------*/

/*--------------Extern Declarations--------------------------------------------*/
int  xfi_read  (unsigned reg_addr);
int  xfi_write (unsigned reg_addr, uint8_t val);
int  xfi_dump (void);
int  xfi_operations(char *arg2);
extern int  mux_write (unsigned mux_addr,unsigned reg_addr, uint8_t data);
/*---------------------------------------------------------------------------*/

ulong reg;
uint8_t data;

static int do_xfi(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if ( !((argc >= 3) && (argc <= 5)) ) {
        	printf("%s help\n", argv[0]);
        	return FAILURE;
    	}
	
	reg = simple_strtoul(argv[3], NULL, 0);  
	data = simple_strtoul(argv[4], NULL, 0);

	if((i2c_probe(MUX1_ADDR)) == 0){ 
		mux_write(MUX1_ADDR,REG_ADDR,CHANNEL6);
		if(strcmp(argv[1],"1") == 0) {
			mux_write(MUX2_ADDR,REG_ADDR,CHANNEL0);
			if(!((i2c_probe(XFI_ADDR)) == 0 )) {
				printf("SFP module 1 is not detected\n\n");
				return FAILURE;
			}
			printf("SFP module 1 is detected\n");		
			xfi_operations(argv[2]);
			return SUCCESS;	
		}
		else if(strcmp(argv[1],"2") == 0) {
			mux_write(MUX2_ADDR,REG_ADDR,CHANNEL1);
			if(!((i2c_probe(XFI_ADDR)) == 0 )) {
				printf("SFP module 2 is not detected\n\n");
				return FAILURE;
			}
			printf("SFP module 2 is detected\n");			
			xfi_operations(argv[2]);	
			return SUCCESS;	
		}
		else if(strcmp(argv[1],"3") == 0) {
			mux_write(MUX2_ADDR,REG_ADDR,CHANNEL2);
			if(!((i2c_probe(XFI_ADDR)) == 0 )) {
				printf("SFP module 3 is not detected\n\n");
				return FAILURE;
			}
			printf("SFP module 3 is detected\n");			
			xfi_operations(argv[2]);	
			return SUCCESS;	
		}
		else if(strcmp(argv[1],"4") == 0) {
			mux_write(MUX2_ADDR,REG_ADDR,CHANNEL3);
			if(!((i2c_probe(XFI_ADDR)) == 0 )) {
				printf("SFP module 4 is not detected\n\n");
				return FAILURE;
			}
			printf("SFP module 4 is detected\n");			
			xfi_operations(argv[2]);	
			return SUCCESS;	
		}
		else if(strcmp(argv[1],"all") == 0) {
			mux_write(MUX2_ADDR,REG_ADDR,CHANNEL0);
			if(!((i2c_probe(XFI_ADDR)) == 0 )) {
				printf("SFP module 1 is not detected\n\n");
			} else {
			printf("SFP module 1 is detected\n");			
			xfi_operations(argv[2]);
			}
			mux_write(MUX2_ADDR,REG_ADDR,CHANNEL1);
			if(!((i2c_probe(XFI_ADDR)) == 0 )) {
				printf("\nSFP module 2 is not detected\n\n");
			} else {
			printf("\nSFP module 2 is detected\n");			
			xfi_operations(argv[2]);
			}
			mux_write(MUX2_ADDR,REG_ADDR,CHANNEL2);
			if(!((i2c_probe(XFI_ADDR)) == 0 )) {
				printf("\nSFP module 3 is not detected\n\n");
			} else {
			printf("\nSFP module 3 is detected\n");			
			xfi_operations(argv[2]);
			}
			mux_write(MUX2_ADDR,REG_ADDR,CHANNEL3);
			if(!((i2c_probe(XFI_ADDR)) == 0 )) {
				printf("\nSFP module 4 is not detected\n\n");
			} else {
			printf("\nSFP module 4 is detected\n");			
			xfi_operations(argv[2]);
			}
			return SUCCESS;	
		}
	} else {
		printf("MUX1 is not detected\n");
		return FAILURE;
	}
	return CMD_RET_USAGE;	
}
/*************************************************************************************
 ** function name       :       xfi_operations
 ** arguments type      :   	unsigned int
 ** return type         :       int
 ** Description         :       This function is used to perform read, write & dump operations 
 **************************************************************************************/
int xfi_operations(char *arg2)
{
	if(strcmp(arg2,"dump") == 0) {
		int rcode;
		rcode=xfi_dump();
		return rcode;
	} else if (strcmp(arg2,"read") == 0) {
		int rcode;
		rcode=xfi_read(reg);
		return rcode;
	} else if (strcmp(arg2,"write") == 0) {
		int rcode;
		rcode=xfi_write(reg,data);
		return rcode;
	}
	return -1;
}

/*************************************************************************************
 ** function name       :       xfi_dump
 ** arguments type      :   	no
 ** return type         :       int
 ** Description         :       This function is used to dump the registers 
 **************************************************************************************/
int xfi_dump(void)
{
	uint8_t val1,val2;
	int rcode = 0;
	if(i2c_read(XFI_ADDR,LOSS_REG,1,&val1,1)) {
		printf("xfi dump error\n");
		rcode = 1;
	}
        ((val1 &= LOSS_MASK) == 0 ) ? printf("No loss occured (%02x=%02x)\n",LOSS_REG,val1):
					printf("Loss occurred (%02x=%02x)\n",LOSS_REG,val1);
	if(i2c_read(XFI_MONI_ADDR,REC_XFI,1,&val2,1)) {
		printf("xfi dump error\n");
		rcode = 1;
	}
        ((val2 &= REC_MASK) == 0 ) ? printf("Destination xfi is active (%02x=%02x)\n",REC_XFI,val2):
					printf("Destination xfi is not active (%02x=%02x)\n",REC_XFI,val2);
	return rcode;
}

/*************************************************************************************
 ** function name       :       xfi_read
 ** arguments type      :   	unsigned int
 ** return type         :       int
 ** Description         :       This function is used to read the value from register
 **************************************************************************************/
int xfi_read(unsigned reg_addr)
{
	uint8_t val;
	int rcode = 0;
	if(i2c_read(XFI_ADDR,reg_addr,1, &val,1)){
		printf("xfi read error\n");
		rcode = 1;
	}
	printf("xfi read : %02x \n",val);
	return rcode;
}
	
/*************************************************************************************
 ** function name       :       xfi_write
 ** arguments type      :   	unsigned int,unsigned int
 ** return type         :       int
 ** Description         :       This function is used to write the value to register 
 **************************************************************************************/
int xfi_write(unsigned reg_addr,uint8_t val)
{
        int rcode = 0;
        if(i2c_write(XFI_ADDR,reg_addr,1, &val,1)) {
		printf("xfi write error\n");
                rcode = 1;
        }
        return rcode;

}

U_BOOT_CMD(xfi,5,1,do_xfi,
	"xfi port access",
	"\nxfi <portnum> dump\n"
	"xfi <portnum> read <reg>\n"
	"xfi <portnum> write <reg> <data>\n"
);
