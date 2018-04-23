/******************************************************************************
 **
 ** SRC-MODULE   : cmd_i2c_scan.c
 **
 ** Copyright(C) : VVDN Tech (C)
 **
 ** TARGET       : 
 **
 ** PROJECT      : FSLU_NVME
 **
 ** AUTHOR       : 
 **
 ** PURPOSE      : file contains U-boot command to scan i2c devices
 **
 **
 ******************************************************************************/

#include <common.h>
#include <config.h>
#include <command.h>
#include <i2c.h>

/*-------------------- Macros---------------------------------------------------- */
#define MUX1_ADDR       	0x77    /* 	Slave address of Mux1 		*/
#define MUX2_ADDR       	0x75    /* 	Slave address of Mux2 		*/
#define MUX_REG			0x00	/*	MUX register	 		*/
#define CHANNEL0		0x08
#define CHANNEL1		0x09
#define CHANNEL2		0x0a
#define CHANNEL3		0x0b
#define CHANNEL4		0x0c
#define CHANNEL5		0x0d
#define CHANNEL6		0x0e
#define CHANNEL7        	0x0f
#define DDR1_ADDR		0x51	/*	DDR1 Slave address		*/
#define DDR2_ADDR		0x52	/*	DDR2 Slave address		*/
#define EEPROM_SYSID_ADDR	0x57	/*	EEPROM SYSID Slave address	*/
#define EEPROM_RCW_ADDR		0x50	/*	EEPROM RCW Slave address 	*/
#define CLK_GENER_ADDR		0x60	/*	CLK Generator Slave address	*/
#define POWERCORE_ADDR		0x38	/*	Power core Slave address	*/
#define TEMP_SENSOR_ADDR	0x4c	/*	Temp sensor Slave address	*/
#define RETIMERS1_TX_ADDR	0x18	/*	retimers 1 tx Slave address	*/
#define RETIMERS1_RX_ADDR	0x19	/*	retimers 1 rx Slave address	*/
#if 0
#define RETIMERS2_TX_ADDR	0x1A	/*	retimers 2 tx Slave address	*/
#define RETIMERS2_RX_ADDR	0x1B	/*	retimers 2 rx Slave address	*/
#endif
#define CPLD_ADDR	        0x35		
#define XFI_ADDR		0xA0	/* 	XFI Slave address		*/	
#define FAILURE         	-1
#define SUCCESS         	0
#define VALID_CHIP		1
#define UNVALID_CHIP		0

int chip_validate(int slave_addr);
extern int  mux_write (unsigned mux_addr,unsigned reg_addr, uint8_t data);

static int do_i2c_scan(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if(argc !=1) {
		printf("1 argument is enough\n");
		return FAILURE;
	}
	if(chip_validate(MUX1_ADDR)) {
		printf("Mux 1 detected = %02x\n",MUX1_ADDR);

		mux_write(MUX1_ADDR,MUX_REG,CHANNEL0);

		/*--------- Channel0 devices ---------------*/		
		if(chip_validate(DDR1_ADDR)) 
			printf("DDR_DIMM_1 is detected = %02x \n",DDR1_ADDR);
		else 
			printf("DDR_DIMM_1 is not detected\n");
		if(chip_validate(DDR2_ADDR)) 
                        printf("DDR_DIMM_2 is detected = %02x \n",DDR2_ADDR);
                else
                        printf("DDR_DIMM_2 is not detected\n");
		if(chip_validate(EEPROM_SYSID_ADDR))
			printf("eeprom sysid is detected = %02x \n",EEPROM_SYSID_ADDR);
		else
			printf("eeprom sysid is not detected \n");
		if(chip_validate(EEPROM_RCW_ADDR))
                        printf("eeprom RCW is detected = %02x \n",EEPROM_RCW_ADDR);
                else
                        printf("eeprom RCW is not detected \n");

		/*---------------------------------------------*/

		/*-----------------Channel1 devices------------*/
		 mux_write(MUX1_ADDR,MUX_REG,CHANNEL1);
		
		if(chip_validate(CLK_GENER_ADDR))
                        printf("clock generator is detected = %02x \n",CLK_GENER_ADDR);
                else
                        printf("clock generator is not detected\n");
		/*-------------------------------------------------*/

		/*--------------- Channel2 devices-------------*/
		mux_write(MUX1_ADDR,MUX_REG,CHANNEL2);
		
		if(chip_validate(POWERCORE_ADDR))
                        printf("powercore is detected = %02x \n",POWERCORE_ADDR);
                else
                        printf("powercore is not detected\n");
		/*-----------------------------------------------*/
		
		/*----------------Channel3 devices----------------*/
		mux_write(MUX1_ADDR,MUX_REG,CHANNEL3);
		
		if(chip_validate(TEMP_SENSOR_ADDR))
                        printf("temp sensor is detected = %02x \n",TEMP_SENSOR_ADDR);
                else
                        printf("temp sensor is not detected\n");
		/*--------------------------------------------------*/

		/*-----------------Channel4 devices------------------*/
		 mux_write(MUX1_ADDR,MUX_REG,CHANNEL4);

                if(chip_validate(TEMP_SENSOR_ADDR))
                        printf("temp sensor is detected = %02x \n",TEMP_SENSOR_ADDR);
                else
                        printf("temp sensor is not detected\n");
		/*-------------------------------------------------------*/
		
		/*-----------------Channel5 devices-----------------------*/
		mux_write(MUX1_ADDR,MUX_REG,CHANNEL5);
		
		if(chip_validate(RETIMERS1_TX_ADDR))
                        printf("retimer tx  is detected = %02x \n",RETIMERS1_TX_ADDR);
                else
                        printf("retimer tx is not detected\n");
		if(chip_validate(RETIMERS1_RX_ADDR))
                        printf("retimer rx  is detected = %02x \n",RETIMERS1_RX_ADDR);
                else
                        printf("retimer rx is not detected\n");
#if 0
		if(chip_validate(RETIMERS2_TX_ADDR))
                        printf("retimers2 tx  is detected = %02x \n",RETIMERS2_TX_ADDR);
                else
                        printf("retimers2 tx is not detected\n");
                if(chip_validate(RETIMERS2_RX_ADDR))
                        printf("retimers2 rx  is detected = %02x \n",RETIMERS2_RX_ADDR);
                else
                        printf("retimers2 rx is not detected\n");
#endif
		/*------------------------------------------------------------------*/
		
		/*-----------------Channel6 devices ---------------------------------*/
		mux_write(MUX1_ADDR,MUX_REG,CHANNEL6);
		if(chip_validate(MUX2_ADDR)) {
                	printf("Mux 2 detected = %02x\n",MUX2_ADDR);		
		
			mux_write(MUX2_ADDR,MUX_REG,CHANNEL0);
		
			if(chip_validate(XFI_ADDR))
				printf("XFI 1  is detected = %02x \n",XFI_ADDR);
			else
				printf("XFI 1  is not detected\n");

			mux_write(MUX2_ADDR,MUX_REG,CHANNEL1);

			if(chip_validate(XFI_ADDR))
				printf("XFI 2  is detected = %02x \n",XFI_ADDR);
			else
				printf("XFI 2  is not detected\n");

			mux_write(MUX2_ADDR,MUX_REG,CHANNEL2);

			if(chip_validate(XFI_ADDR))
				printf("XFI 3  is detected = %02x \n",XFI_ADDR);
			else
				printf("XFI 3  is not detected\n");

			mux_write(MUX2_ADDR,MUX_REG,CHANNEL3);

			if(chip_validate(XFI_ADDR))
				printf("XFI 4  is detected = %02x \n",XFI_ADDR);
			else
				printf("XFI 4  is not detected\n");
		} else {
			printf("MUX2 is not detected \n");
			return FAILURE;
		}
		/*-------------------------------------------------------------------------------*/

		/*---------------------------Channel7 devices------------------------------------*/
		mux_write(MUX1_ADDR,MUX_REG,CHANNEL7);
		
	        if(chip_validate(CPLD_ADDR))
                        printf("CPLD is detected = %02x \n",CPLD_ADDR);
                else
                        printf("CPLD is not detected\n");

	}else {
		 printf("MUX1 is not detected \n");
                 return FAILURE;
	}
	return SUCCESS;
}

/*****************************************************************************************
 ** function name       :       chip_validate
 ** arguments type      :       unsigned int 
 ** return type         :       int
 ** Description         :       This function is used check the i2c device is vaild or not
 *****************************************************************************************/

int chip_validate(int slave_addr)
{
  	int ret = 0;
	ret = i2c_probe(slave_addr);
	if(ret) 
		return UNVALID_CHIP;
	else
		return VALID_CHIP;
		
}

U_BOOT_CMD(i2c_scan,1,1,do_i2c_scan,"uboot i2c_scan command "
                ,"i2c_scan\n");
