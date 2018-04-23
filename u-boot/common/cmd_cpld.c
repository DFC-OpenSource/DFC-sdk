/**
 * command for CPLD i2c access at u-boot
 */

#include <common.h>
#include <config.h>
#include <command.h>
#include <i2c.h>

#define MUX1_CHANNEL7   0x0F
#define MUX1_REG_ADDR   0x00
#define FAILURE         -1
#define SUCCESS          0

int cpld_read(unsigned chip_addr, unsigned reg_addr);
int cpld_write(unsigned chip_addr, unsigned reg_addr, uint8_t val);
extern int  mux_write (unsigned mux_addr,unsigned reg_addr, uint8_t val);

int do_cpld(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
 	ulong reg,data;
	int rcode;

	if ( (argc < 3) || (argc > 4) ) {
        	printf("USAGE: atleast three arguements required and shouldn't exceed 4\n");
        	return FAILURE;
    	}
	
	reg = simple_strtoul(argv[2], NULL, 0);  
	data = simple_strtoul(argv[3], NULL, 0);

	if((i2c_probe(CONFIG_SYS_I2C_MUX_ADDR)) == 0) { 
		printf(" MUX1 is detected\n");
		mux_write(CONFIG_SYS_I2C_MUX_ADDR, MUX1_REG_ADDR, MUX1_CHANNEL7);
		if(i2c_probe(CONFIG_SYS_I2C_CPLD_ADDR) == 0) {
			if ((strcmp(argv[1],"read")) == 0) {
				rcode=cpld_read(CONFIG_SYS_I2C_CPLD_ADDR, reg);
				printf("cpld read : 0x%02x \n", rcode);
				return rcode;		
			} else if ((strcmp(argv[1],"write")) == 0) {
				rcode=cpld_write(CONFIG_SYS_I2C_CPLD_ADDR, reg, data);
	          	       return rcode;			
			}
			 else 
				goto end;					
        	}
	} 
	else {
		printf("MUX1 is not detected\n");
		return FAILURE;
	}
end:	return CMD_RET_USAGE;	
}

int cpld_read(unsigned chip_addr, unsigned reg_addr)
{
	unsigned char val;

	if(i2c_read(chip_addr, reg_addr, 1, &val, 1)){
		printf("cpld read error\n");
		return FAILURE;
	}
	return val;
}
	
int cpld_write(unsigned chip_addr, unsigned reg_addr, uint8_t val)
{
        if(i2c_write( chip_addr, reg_addr, 1, &val, 1)) {
		printf("cpld write error\n");
                return FAILURE;
        }
        return val;

}

U_BOOT_CMD(cpld, 4, 1, do_cpld, "uboot command for CPLD I2C access",
	"\ncpld read <reg>\ncpld write <reg> <data>\n");

