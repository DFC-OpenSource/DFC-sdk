/******************************************************************************
 **
 ** SRC-MODULE   : cmd_power.c
 **
 ** Copyright(C) : VVDN Tech (C)
 **
 ** TARGET       : 
 **
 ** PROJECT      : FSLU_NVME
 **
 ** AUTHOR       : 
 **
 ** PURPOSE      : file contains U-boot command access for power core,GVDD
 **                     by using i2c
 **
 **
 ******************************************************************************/

#include <common.h>
#include <config.h>
#include <command.h>
#include <i2c.h>

/*-------------------- Macros---------------------------------------------------- */
#define MUX1_ADDR               0x77    /* Slave address of Mux1 */
#define REG_ADDR		0x00    
#define CHANNEL2	       	0x0a    /* Selecting Channel 2 Mux1 */
#define POWERCORE_ADDR          0x38    /* Slave address of power core GVDD */
#define DISP_LINE_LEN           16
#define FAILURE                 -1
#define SUCCESS                 0
/*---------------------------------------------------------------------------*/

/*--------------Extern Declarations--------------------------------------------*/
int  power_dump(unsigned power_addr,unsigned reg_addr,unsigned len);
extern int  mux_write (unsigned mux_addr,unsigned reg_addr, uint8_t data);
/*---------------------------------------------------------------------------*/

static int do_power(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
        ulong reg_addr,len;
        char ret;
        if (argc != 4 ) {
                printf("Atleast 2 argument is needed\n");
                return FAILURE;
        }
        reg_addr = simple_strtoul(argv[2], NULL, 0);  /* Getting the register Address */
        len      = simple_strtoul(argv[4], NULL, 0); /* Getting the length */
        mux_write(MUX1_ADDR,REG_ADDR,CHANNEL2);
        ret = i2c_probe(POWERCORE_ADDR);
        if(ret) {
                printf(" %s %s is not detected\n",argv[0],argv[1]);
                return -1;
        }

        if (strcmp (argv[1], "dump") == 0) {
                    int rcode;
                    rcode = power_dump(POWERCORE_ADDR,reg_addr,len);
                    return rcode;
        } else {
                    printf("Unknown arguments \n");
		    return -1;
        }
        return CMD_RET_USAGE;
}

/*************************************************************************************
 ** function name       :       power_dump
 ** arguments type      :       unsigned int,unsigned int,unsigned int
 ** return type         :       int
 ** Description         :       This function is used to read the n bytes data 
 **                             from power core, GVDD register 
 **************************************************************************************/

int power_dump(unsigned power_addr,unsigned reg_addr,unsigned len)
{
        int rcode = 0;
        unsigned int nbytes,linebytes,ret,j;
        nbytes = len;
        do {
                unsigned char   linebuf[DISP_LINE_LEN];
                unsigned char   *cp;

                linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;
                ret = i2c_read(power_addr, reg_addr,1, linebuf, linebytes);
                if (ret) {
                        printf("power i2c read error\n");
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

U_BOOT_CMD(power,4,1,do_power,"power uboot command"
                ,"power <dump> <addr> <len> \n");
