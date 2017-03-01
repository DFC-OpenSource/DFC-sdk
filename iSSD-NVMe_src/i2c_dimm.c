#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <syslog.h>
#include "i2c_dimm.h"
#include "nvme.h"

static int i2c_read(dimm_details dm,int i2c1_file)
{
    unsigned char inbuf, outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    /*
     * In order to read a register, we first do a "dummy write" by writing
     * 0 bytes to the register.
     */
    outbuf = dm.reg;
    messages[0].addr  = dm.dimm_slot_addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = &outbuf;

    /* The data will get returned in this structure */
    messages[1].addr  = dm.dimm_slot_addr;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = sizeof(inbuf);
    messages[1].buf   = &inbuf;

    /* Send the request to the kernel and get the result back */
    packets.msgs      = messages;
    packets.nmsgs     = 2;
    if(ioctl(i2c1_file, I2C_RDWR, &packets) < 0) {
        return -1;
    }
    return inbuf;
}

char* i2c_read_n_bytes(dimm_details dm,int i2c1_file)
{
    int i;
    char *spd_buf = (char *)dm.spd_array;

    for(i=0;i<dm.length;i++) {
        if((spd_buf[i]=i2c_read(dm,i2c1_file)) < 0)
            return "error";
        dm.reg++;
    }
    return spd_buf;
}

static unsigned long long int memory_calc(spd dimm)
{
    unsigned long long int capacity=0;
    unsigned int bus_width=0, dev_width=0, no_ranks=0;
    unsigned long long int memory_capacity=0;

    capacity  = (1UL << ((dimm.density_banks & 0x0f)+28));
    bus_width = (8 << (dimm.bus_width & 7));
    dev_width = (4 << (dimm.organization & 7));
    no_ranks  = (((dimm.organization >> 3) & 7)+1);

    //    printf("cac:%llx, busW:%llx, devW:%llx, ranks:%llx\n",capacity,bus_width,dev_width,no_ranks);
    memory_capacity = (capacity >> 3) * bus_width / dev_width * no_ranks;
    //    printf("memCap:%llx\n",memory_capacity);
    syslog(LOG_INFO,"memory Capacity:%llx\n",memory_capacity);

    if(memory_capacity > 0x200000000){		/*FPGA Supports only 8GB ddr address per dimm, so restrict DDR slot memory to 8GB*/
	    memory_capacity = 0x200000000;
    }

    return memory_capacity;
}

static int scan_i2c_bus(int i2c1_file, int addr)
{
    int res;
    /* Set slave address */
    if (ioctl(i2c1_file, I2C_SLAVE, addr) < 0) {
        if (errno == EBUSY) {
            return 0;
        } else {
            printf("Error: Could not set "
                    "address to 0x%02x", addr,
                    strerror(errno));
            return -1;
        }
    }
    res = i2c_smbus_read_byte(i2c1_file);
    if (!(res < 0)){
        return 0;
    }

    return 1;
}

static int is_i2cdevice_present(int i2c1_file, int dimm_addr)
{
    int res;
    unsigned long funcs;


    if (ioctl(i2c1_file, I2C_FUNCS, &funcs) < 0) {
        fprintf(stderr, "Error: Could not get the adapter functionality matrix: %s\n", strerror(errno));
        close(i2c1_file);
        return -1;
    }

    res = scan_i2c_bus(i2c1_file, dimm_addr);
    if(!res)
        return 0;
    else
        return 1;
}

int get_dimm_info(NvmeCtrl *n)
{
    dimm_details *dm;
    int dimm_slot[4] = {0x51,0x52,0x53,0x54}; /*I2C ADDR: 52,54-->PEX2 DDR; 51,53-->PEX4 NAND*/
    int check = 0;
    unsigned device_count = 0;
    int i2c1_file = 0, no_of_dimm = 4;
    spd dimm;
    char *spd_buf;
    uint64_t ddr_size = 0;
    uint64_t nand_card = 0;

    if((i2c1_file = open(I2C1_FILE_NAME, O_RDWR)) < 0) {
        return -1;
    }
    n->dm = calloc(4,sizeof(uint64_t));
    for(device_count = 0; device_count < no_of_dimm; device_count++) {
        dm = malloc(sizeof(dimm_details));
        spd_buf = dm->spd_array;
        if(!is_i2cdevice_present(i2c1_file,dimm_slot[device_count])) {

            check=0;

            memset(dm,0,sizeof(dimm_details));
            memset(&dimm,0,sizeof(spd));

            dm->length=256;
            dm->reg=0x00;
            dm->dimm_slot_addr=dimm_slot[device_count];

            spd_buf = i2c_read_n_bytes(*dm,i2c1_file);

            if(strcmp(spd_buf,"error") == 0) {
                dm->valid = 0;
                syslog(LOG_INFO,"DIMM slot %d is empty \n",device_count+1);
            } 
            else {
                memcpy(&dimm,spd_buf,sizeof(spd));
                check=dimm.spd_rev;
                if((check & 255) != 255) {  /*TODO Check with DDR/NAND device id*/
                    syslog(LOG_INFO,"DIMM slot %d contains DDR \n",device_count+1);
                    dm->valid = 1;
                    // if(device_count == 1 || device_count == 3) 
                    if(device_count & 1) {
                        //n->namespace_size[0] += memory_calc(dimm);
                        ddr_size += memory_calc(dimm);
                    }
                    else {
                        syslog(LOG_ERR,"DDR should be in PEX2\n");
                        printf("ERROR: DDRs should be in M4 & M2.\nPlease shutdown and Put DDRs on M4 & M2  and Put NANDs on M3 & M1 then Power it again.\n");
                        return -1;
                    }
                } 
                else {
                    dm->valid = 1;
                    syslog(LOG_INFO,"DIMM slot %d contains NAND \n",device_count+1);
                    if(device_count & 1) {
                        printf("ERROR: NAND should be in M3 & M1.\nPlease shutdown and Put NANDs on M3 & M1  and Put DDRs on M4 & M2 then Power it again.\n");
                        return -1;
                    }

                }
            }
        }
        else {
            dm->valid = 0; 
            syslog(LOG_INFO,"I2C device is NOT present, DIMM slot %d is empty \n",device_count+1);
        }
        n->dm[device_count] = dm;
    }
    if(!(n->dm[0]->valid || n->dm[1]->valid || n->dm[2]->valid || n->dm[3]->valid)) {
        syslog(LOG_ERR,"\nERROR: No dimm modules...\n\n");
        return -1;
    }
    if(n->dm[3]->valid && n->dm[2]->valid) {
        if(n->dm[1]->valid ) {
            if (n->dm[0]->valid) {
                n->dimm_module_type = ddr_2_nand_2;
				n->ns_check = DDR_NAND;
			} else {
				n->dimm_module_type = ddr_2_nand_1;
				n->ns_check = DDR_NAND;
			}
		} else if(n->dm[0]->valid) {
			n->dimm_module_type = ddr_1_nand_2;
			n->ns_check = DDR_NAND;
		}else { /*DONT SUPPORT ERROR MSG*/
			n->dimm_module_type = ddr_1_nand_1;
			n->ns_check = DDR_NAND;
		}
	} else if (n->dm[3]->valid ) {
		if(n->dm[1]->valid) {
			n->dimm_module_type = ddr_2_nand_0;
			n->ns_check = DDR_ALONE;
		} else {
			n->dimm_module_type = ddr_1_nand_0;
			n->ns_check = DDR_ALONE;
		}
	} else if( n->dm[2]->valid) {
		if(n->dm[0]->valid) {
			n->dimm_module_type = ddr_0_nand_2;
			n->ns_check = NAND_ALONE;
		} else {
			n->dimm_module_type = ddr_0_nand_1;
			n->ns_check = NAND_ALONE;
		}
	} else {
		n->ns_check = NO_NAMESPACE;
		printf (" No Namespaces. Change DIMM Arrangements\n");
		return -1;
	}
#if 0
	if( (n->ns_check == NAND_ALONE) || (n->ns_check == DDR_NAND)){
		if((n->dimm_module_type == 7) || (n->dimm_module_type == 4)) {
			n->ns_check = DDR_ALONE;
		}
		else if(n->dimm_module_type == 1) {
			n->ns_check = NO_NAMESPACE;
			printf("No Namespace allowed. Change DIMM arrangments\n");
		}
	}
#endif
	if(ddr_size){
		n->namespace_size[0] = ddr_size;
	}
	syslog(LOG_INFO,"namespace_size: %lx\n",n->namespace_size[0]);
	syslog(LOG_INFO,"dimm_module_type: %d\n",n->dimm_module_type);
    close(i2c1_file);

    return 0;

}
