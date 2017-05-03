#ifndef __DIMM_H
#define __DIMM_H

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "smbus.h"

#define I2C1_FILE_NAME "/dev/i2c-3"

enum dimm_module {
    ddr_0_nand_1 = 1,
    ddr_0_nand_2 = 2,
    ddr_1_nand_0 = 3,
    ddr_1_nand_1 = 4,
    ddr_1_nand_2 = 5,
    ddr_2_nand_0 = 6,
    ddr_2_nand_1 = 7,
    ddr_2_nand_2 = 8,
}; 

enum namespace_stat {
	DDR_ALONE = 1,
	NAND_ALONE =2,
	DDR_NAND  = 3,
	NO_NAMESPACE = 4,
};

typedef struct dimm_details {
    unsigned int dimm_slot_addr;
    unsigned int reg;
    unsigned int length;
    unsigned char spd_array[256];
    unsigned int valid;
}dimm_details;

/* ddr3 SPD structure */
typedef struct spd_details {
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
}spd;

#endif
