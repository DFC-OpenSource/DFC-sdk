/******************************************************************************
 **
 ** SRC-MODULE   : i2capp.c
 **
 ** Copyright(C) : VVDN Tech (C)
 **
 ** TARGET       : 
 **
 ** PROJECT      : FSLU_NVME
 **
 ** AUTHOR       : 
 **
 ** PURPOSE      : file contains application to access the i2c device
 **
 ******************************************************************************/

#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

/*-------------------------MACROS--------------------------------------------------*/
#define I2C0_FILE_NAME "/dev/i2c-0"
#define MUX1_ADDR		0x77
#define MUX2_ADDR		0x75
#define EEPROM_SYSID_ADDR	0x57
#define EEPROM_RCW_ADDR		0x50
#define TEMP_ADDR		0x4C
#define RETIMERS1_TX_ADDR	0x18
#define RETIMERS1_RX_ADDR	0x19

#define CPLD_NIC_ADDR           0x35            
#define CODE_VER                0x02            

#define XFI_ADDR		0x50
#define XFI_MONI_ADDR		0x51
#define LOSS_REG 		0x65
#define LOSS_MASK 		0x0A
#define REC_XFI 		0x6E
#define REC_MASK 		0x02
#define SFP_REG			0x00
#define SFP_VALUE		0x03

#define MUX_REG			0x00
#define CHANNEL0		0x08
#define CHANNEL1		0x09
#define CHANNEL2		0x0A
#define CHANNEL3		0x0B
#define CHANNEL4		0x0C
#define CHANNEL5		0x0D
#define CHANNEL6		0x0E
#define CHANNEL7		0x0F
#define DISP_LINE_LEN		16

/* For accessing i2c bus 3 */
#define I2C1_FILE_NAME "/dev/i2c-3"
#define CPLD_SSD_ADDR		0x36

#define DIMM_SLOT_1		0x51
#define DIMM_SLOT_2		0x52
#define DIMM_SLOT_3		0x53
#define DIMM_SLOT_4		0x54

#define USAGE_MESSAGE \
	"Usage:\n" \
"  %s <device> r [register]   " \
"to read value from [register]\n" \
"  %s <device> w [register] [value]   " \
"to write a value [value] to register [register]\n" \
"  %s <device> d [register] [length]   " \
"to dump the register upto [length]\n" \
"  %s cpld_nic version  " \
"to find the present NIC card cpld version\n" \
"  %s cpld_ssd version  " \
"to find the present SSD card cpld version\n" \
"  %s ssd_dimm dump " \
"to find the present SSD card DIMM slots details\n" \
""
/*------------------------------------------------------------------------------------*/

int i2c0_file;
int i2c1_file;

/**********Structure for DDR3 SPD details********************************************/

typedef struct ddr3_spd_details {
 /* General Section: Bytes 0-59 */
        unsigned char info_size_crc;   /*  0 # bytes written into serial memory,
                                             CRC coverage */
        unsigned char spd_rev;         /*  1 Total # bytes of SPD mem device */
        unsigned char mem_type;        /*  2 Key Byte / Fundamental mem type */
        unsigned char module_type;     /*  3 Key Byte / Module Type */
        unsigned char density_banks;   /*  4 SDRAM Density and Banks */
        unsigned char addressing;      /*  5 SDRAM Addressing */
        unsigned char module_vdd;      /*  6 Module nominal voltage, VDD */
        unsigned char organization;    /*  7 Module Organization */
        unsigned char bus_width;       /*  8 Module Memory Bus Width */
        unsigned char ftb_div;         /*  9 Fine Timebase (FTB)
                                             Dividend / Divisor */
        unsigned char mtb_dividend;    /* 10 Medium Timebase (MTB) Dividend */
        unsigned char mtb_divisor;     /* 11 Medium Timebase (MTB) Divisor */
        unsigned char tck_min;         /* 12 SDRAM Minimum Cycle Time */
        unsigned char res_13;          /* 13 Reserved */
        unsigned char caslat_lsb;      /* 14 CAS Latencies Supported,
                                             Least Significant Byte */
        unsigned char caslat_msb;      /* 15 CAS Latencies Supported,
                                             Most Significant Byte */
        unsigned char taa_min;         /* 16 Min CAS Latency Time */
        unsigned char twr_min;         /* 17 Min Write REcovery Time */
        unsigned char trcd_min;        /* 18 Min RAS# to CAS# Delay Time */
        unsigned char trrd_min;        /* 19 Min Row Active to
                                             Row Active Delay Time */
        unsigned char trp_min;         /* 20 Min Row Precharge Delay Time */
        unsigned char tras_trc_ext;    /* 21 Upper Nibbles for tRAS and tRC */
        unsigned char tras_min_lsb;    /* 22 Min Active to Precharge
                                             Delay Time */
        unsigned char trc_min_lsb;     /* 23 Min Active to Active/Refresh
                                             Delay Time, LSB */
        unsigned char trfc_min_lsb;    /* 24 Min Refresh Recovery Delay Time */
        unsigned char trfc_min_msb;    /* 25 Min Refresh Recovery Delay Time */
        unsigned char twtr_min;        /* 26 Min Internal Write to
                                      	     Read Command Delay Time */
        unsigned char trtp_min;        /* 27 Min Internal Read to Precharge
                                             Command Delay Time */
        unsigned char tfaw_msb;        /* 28 Upper Nibble for tFAW */
        unsigned char tfaw_min;        /* 29 Min Four Activate Window
                                             Delay Time*/
        unsigned char opt_features;    /* 30 SDRAM Optional Features */
        unsigned char therm_ref_opt;   /* 31 SDRAM Thermal and Refresh Opts */
        unsigned char therm_sensor;    /* 32 Module Thermal Sensor */
        unsigned char device_type;     /* 33 SDRAM device type */
        int8_t fine_tck_min;           /* 34 Fine offset for tCKmin */
        int8_t fine_taa_min;           /* 35 Fine offset for tAAmin */
        int8_t fine_trcd_min;          /* 36 Fine offset for tRCDmin */
        int8_t fine_trp_min;           /* 37 Fine offset for tRPmin */
        int8_t fine_trc_min;           /* 38 Fine offset for tRCmin */
        unsigned char res_39_59[21];   /* 39-59 Reserved, General Section */

        /* Module-Specific Section: Bytes 60-116 */
        union {
                struct {
                        /* 60 (Unbuffered) Module Nominal Height */
                        unsigned char mod_height;
                        /* 61 (Unbuffered) Module Maximum Thickness */
                        unsigned char mod_thickness;
                        /* 62 (Unbuffered) Reference Raw Card Used */
                        unsigned char ref_raw_card;
                        /* 63 (Unbuffered) Address Mapping from
                              Edge Connector to DRAM */
                        unsigned char addr_mapping;
                        /* 64-116 (Unbuffered) Reserved */
                        unsigned char res_64_116[53];
                } unbuffered;
                struct {
                        /* 60 (Registered) Module Nominal Height */
                        unsigned char mod_height;
                        /* 61 (Registered) Module Maximum Thickness */
                        unsigned char mod_thickness;
                        /* 62 (Registered) Reference Raw Card Used */
 			unsigned char ref_raw_card;
                        /* 63 DIMM Module Attributes */
                        unsigned char modu_attr;
                        /* 64 RDIMM Thermal Heat Spreader Solution */
                        unsigned char thermal;
                        /* 65 Register Manufacturer ID Code, Least Significant Byte */
                        unsigned char reg_id_lo;
                        /* 66 Register Manufacturer ID Code, Most Significant Byte */
                        unsigned char reg_id_hi;
                        /* 67 Register Revision Number */
                        unsigned char reg_rev;
                        /* 68 Register Type */
                        unsigned char reg_type;
                        /* 69-76 RC1,3,5...15 (MS Nibble) / RC0,2,4...14 (LS Nibble) */
                        unsigned char rcw[8];
                } registered;
                unsigned char uc[57]; /* 60-116 Module-Specific Section */
        } mod_section;

        /* Unique Module ID: Bytes 117-125 */
        unsigned char mmid_lsb;        /* 117 Module MfgID Code LSB - JEP-106 */
        unsigned char mmid_msb;        /* 118 Module MfgID Code MSB - JEP-106 */
        unsigned char mloc;            /* 119 Mfg Location */
        unsigned char mdate[2];        /* 120-121 Mfg Date */
        unsigned char sernum[4];       /* 122-125 Module Serial Number */

        /* CRC: Bytes 126-127 */
        unsigned char crc[2];          /* 126-127 SPD CRC */

        /* Other Manufacturer Fields and User Space: Bytes 128-255 */
        unsigned char mpart[18];       /* 128-145 Mfg's Module Part Number */
        unsigned char mrev[2];         /* 146-147 Module Revision Code */
        unsigned char dmid_lsb;        /* 148 DRAM MfgID Code LSB - JEP-106 */
        unsigned char dmid_msb;        /* 149 DRAM MfgID Code MSB - JEP-106 */
        unsigned char msd[26];         /* 150-175 Mfg's Specific Data */
        unsigned char cust[80];        /* 176-255 Open for Customer Use */
} spd;

/*************************************************************************************
 ** function name       :       i2c_write
 ** arguments type      :       unsigned char,unsigned char,unsigned char
 ** return type         :       int
 ** Description         :       This function is used to write 1 byte data 
 **                             to i2c devices
 **************************************************************************************/

static int i2c_write(unsigned char addr,
		unsigned char reg,
		unsigned char value) {

	unsigned char outbuf[2];
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[1];

	messages[0].addr  = addr;
	messages[0].flags = 0;
	messages[0].len   = sizeof(outbuf);
	messages[0].buf   = outbuf;

	/* The first byte indicates which register we'll write */
	outbuf[0] = reg;

	/* 
	 * The second byte indicates the value to write.  Note that for many
	 * devices, we can write multiple, sequential registers at once by
	 * simply making outbuf bigger.
	 */
	outbuf[1] = value;

	/* Transfer the i2c packets to the kernel and verify it worked */
	packets.msgs  = messages;
	packets.nmsgs = 1;
	if(ioctl(i2c0_file, I2C_RDWR, &packets) < 0) {
		perror("write : Unable to send data");
		return -1;
	}

	return 0;
}

/*************************************************************************************
 ** function name       :       i2c_read
 ** arguments type      :       unsigned char,unsigned char
 ** return type         :       int
 ** Description         :       This function is used to read 1 byte data 
 **                             from i2c devices
 **************************************************************************************/

static int i2c_read(unsigned char addr,
		unsigned char reg,char **argv) 
{
	unsigned char inbuf, outbuf;
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];

	/*
	 * In order to read a register, we first do a "dummy write" by writing
	 * 0 bytes to the register.
	 */
	outbuf = reg;
	messages[0].addr  = addr;
	messages[0].flags = 0;
	messages[0].len   = sizeof(outbuf);
	messages[0].buf   = &outbuf;

	/* The data will get returned in this structure */
	messages[1].addr  = addr;
	messages[1].flags = I2C_M_RD;
	messages[1].len   = sizeof(inbuf);
	messages[1].buf   = &inbuf;

	/* Send the request to the kernel and get the result back */
	packets.msgs      = messages;
	packets.nmsgs     = 2;
	if(strcmp(argv[1],"cpld_ssd") == 0) {
		if(ioctl(i2c1_file, I2C_RDWR, &packets) < 0) 
			return -1;
	} else if(strcmp(argv[1],"ssd_dimm") == 0) {
		if(ioctl(i2c1_file, I2C_RDWR, &packets) < 0) 
			return -1;
	} else {
	if(ioctl(i2c0_file, I2C_RDWR, &packets) < 0)
		return -1;
	}
	return inbuf;
}

/*************************************************************************************
 ** function name       :       i2c_read_n_bytes
 ** arguments type      :       unsigned char,unsigned char,unsigned char,unsigned *char
				unsigned int
 ** return type         :       char *
 ** Description         :       This function is used to read n bytes of data 
 **                             from i2c devices
 **************************************************************************************/

char * i2c_read_n_bytes(unsigned char addr,unsigned char reg_addr,unsigned char *buf,unsigned int len,char **argv)
{
	int i;
	for(i=0;i<len;i++) {
		if((buf[i]=i2c_read(addr,reg_addr,argv)) < 0)
			return "error";
		reg_addr++;
	}
	return buf;
}

/*************************************************************************************
 ** function name       :       i2c_dump
 ** arguments type      :       unsigned char,unsigned char,unsigned char
 ** return type         :       int
 ** Description         :       This function is used to dump n bytes of data 
 **                             from i2c devices
 **************************************************************************************/

static int i2c_dump(unsigned char addr,unsigned char reg_addr,unsigned char len,char **argv) 
{
	unsigned int nbytes,linebytes,j;
        nbytes = len;
        do {
                unsigned char   linebuf[DISP_LINE_LEN];
                unsigned char   *cp;

                linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;
                i2c_read_n_bytes(addr,reg_addr,linebuf,linebytes,argv);
		if(strcmp(linebuf,"error") == 0) 
			return -1;
                else {
                        printf("%04x:", reg_addr);      /*Displaying the data byte line by line */
                        cp = linebuf;
                        for (j=0; j<linebytes; j++) {
                                printf(" %02x", *cp++);
                                reg_addr++;
                        }
                        printf ("    ");
                        cp = linebuf;
                        for (j=0; j<linebytes; j++) {
                                if ((*cp < 0x20) || (*cp > 0x7e))
                                        printf (".");
                                else
                                        printf("%c", *cp);
                                cp++;
                        }
			printf("\n");
                }
                nbytes -= linebytes;
        } while (nbytes > 0);
	
	return 0;
}

/*************************************************************************************
 ** function name       :       i2c_open
 ** arguments type      :       void
 ** return type         :       int
 ** Description         :       This function is used to open the i2c device file
 **************************************************************************************/

int i2c_open()
{
	if((i2c0_file = open(I2C0_FILE_NAME, O_RDWR)) < 0) 
		return -1;
	if((i2c1_file = open(I2C1_FILE_NAME, O_RDWR)) < 0) 
		return -1;
	return 0;
}

/*************************************************************************************
 ** function name       :       i2c_close
 ** arguments type      :       void
 ** return type         :       void
 ** Description         :       This function is used to close the i2c device file
 **************************************************************************************/

void i2c_close()
{
	close(i2c0_file);
	close(i2c1_file);
}

/*************************************************************************************
 ** function name       :       mux
 ** arguments type      :       int,char **
 ** return type         :       int
 ** Description         :       This function is used access the i2c mux registers 
 **                            
 **************************************************************************************/

int mux(int slave_addr,char **argv)
{
	int reg,data,ret = 0 ;
	if(strcmp(argv[3],"r") == 0) {
		reg = strtol(argv[4], NULL, 0);
		if((ret = i2c_read(slave_addr,reg,argv)) < 0) 
			return -1;
		printf("reg = %02x  value = %02x\n",reg,ret);
	} else if (strcmp(argv[3],"w") == 0) {
		reg = strtol(argv[4], NULL, 0);
		data = strtol(argv[5], NULL, 0);
		if((i2c_write(slave_addr,reg,data)) < 0)
			return -1;
	}else {
		printf("error\n");
		return -1;
	}
	return 0;
}

/*************************************************************************************
 ** function name       :       eeprom
 ** arguments type      :       int,char **
 ** return type         :       int
 ** Description         :       This function is used access the eeprom registers 
 **                            
 **************************************************************************************/

int eeprom(int slave_addr,char **argv)
{
	int reg,len,data,ret;
	i2c_write(MUX1_ADDR,MUX_REG,CHANNEL0);
	if(strcmp(argv[3],"r") == 0) {
		reg = strtol(argv[4], NULL, 0);
		if((ret=i2c_read(slave_addr,reg,argv)) < 0)
			return -1;
		printf("reg = %02x  value = %02x\n",reg,ret);
	} else if (strcmp(argv[3],"w") == 0) {
		reg = strtol(argv[4], NULL, 0);
		data = strtol(argv[5], NULL, 0);
		if((i2c_write(slave_addr,reg,data)) < 0)
			return -1;
	} else if(strcmp(argv[3],"d") == 0) {
		reg = strtol(argv[4], NULL, 0);
		len = strtol(argv[5], NULL, 0);
		if((i2c_dump(slave_addr,reg,len,argv)) < 0)
			return -1;
	}else {
		printf("error\n");
		return -1;
	}
	return 0;
}

/*************************************************************************************
 ** function name       :       temp
 ** arguments type      :       int,char **
 ** return type         :       int
 ** Description         :       This function is used access the temp registers 
 **                            
 **************************************************************************************/

int temp(int slave_addr,char **argv)
{
	int reg,data,ret;
	if(strcmp(argv[3],"r") == 0) {
		reg = strtol(argv[4], NULL, 0);
		if((ret=i2c_read(slave_addr,reg,argv)) < 0)
			return -1;
		printf("reg = %02x  value = %02x\n",reg,ret);
	} else if (strcmp(argv[3],"w") == 0) {
		reg = strtol(argv[4], NULL, 0);
		data = strtol(argv[5], NULL, 0);
		if((i2c_write(slave_addr,reg,data)) < 0)
			return -1;
	}else {
		printf("error\n");
		return -1;
	}
	return 0;
}

/*************************************************************************************
 ** function name       :       retimers
 ** arguments type      :       int,char **
 ** return type         :       int
 ** Description         :       This function is used access the retimers registers 
 **                            
 **************************************************************************************/

int retimers(int slave_addr,char **argv)
{
	int reg,ret;
	 if(strcmp(argv[4],"r") == 0) {
                reg = strtol(argv[5], NULL, 0);
                if((ret = i2c_read(slave_addr,reg,argv)) < 0)
                        return -1;
		printf("reg = %02x  value = %02x\n",reg,ret);
	} else 
		return -1;
}

/*************************************************************************************
 ** function name       :       xfi
 ** arguments type      :       int,char **
 ** return type         :       int
 ** Description         :       This function is used access the xfi registers 
 **                            
 **************************************************************************************/

int xfi(int slave_addr,char **argv)
{
	int reg,ret;
	if(strcmp(argv[2],"1") == 0) {
		i2c_write(MUX2_ADDR,MUX_REG,CHANNEL0);
	} else if(strcmp(argv[2],"2") == 0) {
		i2c_write(MUX2_ADDR,MUX_REG,CHANNEL1);
	} else if (strcmp(argv[2],"3") == 0) {
		i2c_write(MUX2_ADDR,MUX_REG,CHANNEL2);
	} else if (strcmp(argv[2],"4") == 0) {
		i2c_write(MUX2_ADDR,MUX_REG,CHANNEL3);
	} else if (strcmp(argv[2],"all") == 0) {
		if(strcmp(argv[3],"d") == 0) {
			i2c_write(MUX2_ADDR,MUX_REG,CHANNEL0);
			xfi_dump(slave_addr,1,argv);
			i2c_write(MUX2_ADDR,MUX_REG,CHANNEL1);
			xfi_dump(slave_addr,2,argv);
			i2c_write(MUX2_ADDR,MUX_REG,CHANNEL2);
			xfi_dump(slave_addr,3,argv);
			i2c_write(MUX2_ADDR,MUX_REG,CHANNEL3);
			xfi_dump(slave_addr,4,argv);
			return 0;
		} else {
			printf("Usage : <app> xfi all d\n");
			return -1;
		}
	 } else 
		return -1;
	if(strcmp(argv[3],"r") == 0) {
		reg = strtol(argv[4], NULL, 0);
		if((ret = i2c_read(slave_addr,reg,argv)) < 0)
                	return -1;
		printf("reg = %02x  value = %02x\n",reg,ret);
	} else if(strcmp(argv[3],"d") == 0) {
		xfi_dump(slave_addr,atol(argv[2]),argv);
	}else {
		return -1;
	}
	return 0;
}

/*************************************************************************************
 ** function name       :       xfi_dump
 ** arguments type      :       int
 ** return type         :       int
 ** Description         :       This function is used to dump the regiters 
 **                            
 **************************************************************************************/
int xfi_dump(int slave_addr,int num,char **argv)
{
	unsigned int val1,val2,val3;
	if((val1 = i2c_read(slave_addr,SFP_REG,argv)) < 0)
		return -1;
        if (val1 == SFP_VALUE) {
		 printf("\n\tSFP %d module detected (%02x=%02x)\n",num,SFP_REG,val1);
	} else {
		printf("\n\tSFP %d module not detected (%02x=%02x)\n",num,SFP_REG,val1);
		return -1;
	}
	if((val2 = i2c_read(slave_addr,LOSS_REG,argv)) < 0)
		return -1;
        ((val2 &= LOSS_MASK) == 0) ? printf("\tNo Loss occured (%02x=%02x)\n",LOSS_REG,val2):
				printf("\tLoss occured (%02x=%02x)\n",LOSS_REG,val2);
	if((val3 = i2c_read(XFI_MONI_ADDR,REC_XFI,argv)) < 0)
                return -1;
        ((val3 &= REC_MASK) == 0 ) ? printf("\tDestination xfi is active (%02x=%02x)\n",REC_XFI,val3):
				printf("\tDestination xfi is not active (%02x=%02x)\n",REC_XFI,val3);
        return 0;
}

/*************************************************************************************
 ** function name       :       cpld_nic
 ** arguments type      :       int,char **
 ** return type         :       int
 ** Description         :       This function is used to find the cpld_nic version. 
 **                            
 **************************************************************************************/

int cpld_nic(int slave_addr,char **argv)
{
	int ret=0;
	if(strcmp(argv[2],"version") == 0) {
		if((ret = i2c_read(slave_addr,CODE_VER,argv)) < 0)
                	return -1;
		printf("%c%02u \n",((ret >> 5) + 'A'),(ret & 0x1f));
	}
	return 0;
}

/*************************************************************************************
 ** function name       :       cpld_ssd
 ** arguments type      :       int,char **
 ** return type         :       int
 ** Description         :       This function is used to find the cpld_ssd version. 
 **                            
 **************************************************************************************/

int cpld_ssd(int slave_addr,char **argv)
{
	int ret=0;
	if(strcmp(argv[2],"version") == 0) {
		if((ret = i2c_read(slave_addr,CODE_VER,argv)) < 0)
                	return -1;
		printf("%c%02u \n",((ret >> 5) + 'A'),(ret & 0x1f));
	}
	return 0;
}

/*************************************************************************************
 ** function name       :       ssd_dimm
 ** arguments type      :       int,char **
 ** return type         :       int
 ** Description         :       This function is used to find the ssd card dimm slots details. 
 **                            
 **************************************************************************************/

int ssd_dimm(int slave_addr_1,int slave_addr_2,int slave_addr_3,int slave_addr_4,char **argv)
{
	int length=256,check=0;
	unsigned char   spd_array_1[256];
	unsigned char   spd_array_2[256];
	unsigned char   spd_array_3[256];
	unsigned char   spd_array_4[256];
	spd dimm1,dimm2,dimm3,dimm4;

	if(strcmp(argv[2],"dump") == 0) {
		i2c_read_n_bytes(slave_addr_1,0x00,spd_array_1,length,argv);
		if(strcmp(spd_array_1,"error") == 0) {
			printf("DIMM slot 1 is empty \n");
		} else {
			memcpy(&dimm1,spd_array_1,sizeof(spd));
			check=dimm1.spd_rev;
			if((check & 255) != 255) {
				printf("DIMM slot 1 contains DDR \n");
			} else {
				printf("DIMM slot 1 is Empty \n");
			}

		}
		check=0;
		i2c_read_n_bytes(slave_addr_2,0x00,spd_array_2,length,argv);
		if(strcmp(spd_array_2,"error") == 0) {
			printf("DIMM slot 2 is empty \n");
		} else {
			memcpy(&dimm2,spd_array_2,sizeof(spd));
			check=dimm2.spd_rev;
			if((check & 255) != 255) {
				printf("DIMM slot 2 contains DDR \n");
			} else {
				printf("DIMM slot 2 is Empty \n");
			}
		}
		check=0;
		i2c_read_n_bytes(slave_addr_3,0x00,spd_array_3,length,argv);
		if(strcmp(spd_array_3,"error") == 0) {
			printf("DIMM slot 3 is empty \n");
		} else {
			memcpy(&dimm3,spd_array_3,sizeof(spd));
			check=dimm3.spd_rev;
			if((check & 255) != 255) {
				printf("DIMM slot 3 contains DDR \n");
			} else {
				printf("DIMM slot 3 is Empty \n");
			}
		}
		check=0;
		i2c_read_n_bytes(slave_addr_4,0x00,spd_array_4,length,argv);
		if(strcmp(spd_array_4,"error") == 0) {
			printf("DIMM slot 4 is empty \n");
		} else {
			memcpy(&dimm4,spd_array_4,sizeof(spd));
			check=dimm4.spd_rev;
			if((check & 255) != 255) {
				printf("DIMM slot 4 contains DDR \n");
			} else {
				printf("DIMM slot 4 is Empty \n");
			}
		}
        }
	return 0;
}

/***************** MAIN FUNCTION *******************************/

int main(int argc, char **argv) 
{

	if (argc <=2  || argc >=7 ) {
		printf("insufficient argument\n");
                goto ERR;
        }
	if((i2c_open()) < 0) {
		perror("Unable to open i2c device");
		exit(0);
	}

	/* selecting the i2c device to access */
	if(strcmp (argv[1], "mux") == 0) { 
		if(strcmp(argv[2],"1") == 0) {
			if((mux(MUX1_ADDR,argv)) < 0) 
				return -1;
		}else if(strcmp(argv[2],"2") == 0) {
			i2c_write(MUX1_ADDR,MUX_REG,CHANNEL6);
			if((mux(MUX2_ADDR,argv)) < 0) 
				return -1;
		}else
			goto ERR;

	}else if(strcmp (argv[1], "eeprom") == 0) { 
		if(strcmp(argv[2],"sysid") == 0){ 
			if(eeprom(EEPROM_SYSID_ADDR,argv) < 0)
				return -1;
		} else if(strcmp(argv[2],"rcw") == 0) {
			if(eeprom(EEPROM_RCW_ADDR,argv) < 0) 
				return -1;
		} else
			goto ERR;
	}else if(strcmp (argv[1], "retimers") == 0) {
		i2c_write(MUX1_ADDR,MUX_REG,CHANNEL5); 
		if(strcmp(argv[2],"1") == 0) {
			if(strcmp(argv[3],"tx") == 0) {
				if((retimers(RETIMERS1_TX_ADDR,argv)) < 0)
					return -1;
		       } else if(strcmp(argv[3],"rx") == 0) {
				if((retimers(RETIMERS1_RX_ADDR,argv)) < 0)
					return -1;
		       } else 
				goto ERR;
		} else
			goto ERR;
	}else if (strcmp (argv[1], "temp") == 0) {
		if(strcmp(argv[2],"nic1") == 0) {
			i2c_write(MUX1_ADDR,MUX_REG,CHANNEL3);
			if((temp(TEMP_ADDR,argv)) < 0)
				return -1;
		} else if(strcmp(argv[2],"nic2") == 0) {
			i2c_write(MUX1_ADDR,MUX_REG,CHANNEL4);
			if((temp(TEMP_ADDR,argv)) < 0)
				return -1;
		} else
			goto ERR;
	} else if (strcmp (argv[1],"xfi") == 0) {
		i2c_write(MUX1_ADDR,MUX_REG,CHANNEL6);
		if((xfi(XFI_ADDR,argv)) < 0)
			return -1;
	} else if (strcmp (argv[1],"cpld_nic") == 0) {
		i2c_write(MUX1_ADDR,MUX_REG,CHANNEL7);
		if((cpld_nic(CPLD_NIC_ADDR,argv)) < 0)
			return -1;
	} else if (strcmp (argv[1],"cpld_ssd") == 0) {
		if((cpld_ssd(CPLD_SSD_ADDR,argv)) < 0)
			return -1;
	} else if (strcmp (argv[1],"ssd_dimm") == 0) {
		if((ssd_dimm(DIMM_SLOT_1,DIMM_SLOT_2,DIMM_SLOT_3,DIMM_SLOT_4,argv)) < 0)
			return -1;
	} else 
		goto ERR;

	i2c_close();
	return 0;
ERR:
	fprintf(stderr, USAGE_MESSAGE,argv[0],argv[0],argv[0],argv[0],argv[0],argv[0]);
	i2c_close();
	return -1;
}
