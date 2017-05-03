/*
 * Copyright 2015 Freescale Semiconductor
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <fsl_ifc.h>
#include <fsl_ddr.h>
#include <asm/io.h>
#include <hwconfig.h>
#include <fdt_support.h>
#include <libfdt.h>
#include <fsl_debug_server.h>
#include <fsl-mc/fsl_mc.h>
#include <environment.h>
#include <i2c.h>
#include <asm/arch-fsl-lsch3/soc.h>
#include <fsl_sec.h>

#include "../common/qixis.h"
#include "ls2080ardb_qixis.h"

#define PIN_MUX_SEL_SDHC	0x00
#define PIN_MUX_SEL_DSPI	0x0a

#define SET_SDHC_MUX_SEL(reg, value)	((reg & 0xf0) | value)
DECLARE_GLOBAL_DATA_PTR;

enum {
	MUX_TYPE_SDHC,
	MUX_TYPE_DSPI,
};

unsigned long long get_qixis_addr(void)
{
	unsigned long long addr;

	if (gd->flags & GD_FLG_RELOC)
		addr = QIXIS_BASE_PHYS;
	else
		addr = QIXIS_BASE_PHYS_EARLY;

	/*
	 * IFC address under 256MB is mapped to 0x30000000, any address above
	 * is mapped to 0x5_10000000 up to 4GB.
	 */
	addr = addr  > 0x10000000 ? addr + 0x500000000ULL : addr + 0x30000000;

	return addr;
}

int checkboard(void)
{
	u8 sw;
	char buf[15];

	cpu_name(buf);
	printf("Board: %s-RDB, ", buf);

	sw = QIXIS_READ(arch);
	printf("Board Arch: V%d, ", sw >> 4);
	printf("Board version: %c, boot from ", (sw & 0xf) + 'A');

	sw = QIXIS_READ(brdcfg[0]);
	sw = (sw & QIXIS_LBMAP_MASK) >> QIXIS_LBMAP_SHIFT;

	if (sw < 0x8)
		printf("vBank: %d\n", sw);
	else if (sw == 0x9)
		puts("NAND\n");
	else
		printf("invalid setting of SW%u\n", QIXIS_LBMAP_SWITCH);

	printf("FPGA: v%d.%d\n", QIXIS_READ(scver), QIXIS_READ(tagdata));

	puts("SERDES1 Reference : ");
	printf("Clock1 = 156.25MHz ");
	printf("Clock2 = 156.25MHz");

	puts("\nSERDES2 Reference : ");
	printf("Clock1 = 100MHz ");
	printf("Clock2 = 100MHz\n");

	return 0;
}

unsigned long get_board_sys_clk(void)
{
	u8 sysclk_conf = QIXIS_READ(brdcfg[1]);

	switch (sysclk_conf & 0x0F) {
	case QIXIS_SYSCLK_83:
		return 83333333;
	case QIXIS_SYSCLK_100:
		return 100000000;
	case QIXIS_SYSCLK_125:
		return 125000000;
	case QIXIS_SYSCLK_133:
		return 133333333;
	case QIXIS_SYSCLK_150:
		return 150000000;
	case QIXIS_SYSCLK_160:
		return 160000000;
	case QIXIS_SYSCLK_166:
		return 166666666;
	}
	return 66666666;
}

int select_i2c_ch_pca9547(u8 ch)
{
	int ret;

	ret = i2c_write(I2C_MUX_PCA_ADDR_PRI, 0, 1, &ch, 1);
	if (ret) {
		puts("PCA: failed to select proper channel\n");
		return ret;
	}

	return 0;
}

int config_board_mux(int ctrl_type)
{
	u8 reg5;

	reg5 = QIXIS_READ(brdcfg[5]);

	switch (ctrl_type) {
	case MUX_TYPE_SDHC:
		reg5 = SET_SDHC_MUX_SEL(reg5, PIN_MUX_SEL_SDHC);
		break;
	case MUX_TYPE_DSPI:
		reg5 = SET_SDHC_MUX_SEL(reg5, PIN_MUX_SEL_DSPI);
		break;
	default:
		printf("Wrong mux interface type\n");
		return -1;
	}

	QIXIS_WRITE(brdcfg[5], reg5);

	return 0;
}

int board_init(void)
{
	char *env_hwconfig;
	u32 __iomem *dcfg_ccsr = (u32 __iomem *)DCFG_BASE;
	u32 __iomem *irq_ccsr = (u32 __iomem *)ISC_BASE;
	u32 val;

	init_final_memctl_regs();

	val = in_le32(dcfg_ccsr + DCFG_RCWSR13 / 4);

	env_hwconfig = getenv("hwconfig");

	if (hwconfig_f("dspi", env_hwconfig) &&
	    DCFG_RCWSR13_DSPI == (val & (u32)(0xf << 8)))
		config_board_mux(MUX_TYPE_DSPI);
	else
		config_board_mux(MUX_TYPE_SDHC);

#ifdef CONFIG_ENV_IS_NOWHERE
	gd->env_addr = (ulong)&default_environment[0];
#endif
	select_i2c_ch_pca9547(I2C_MUX_CH_DEFAULT);

	QIXIS_WRITE(rst_ctl, QIXIS_RST_CTL_RESET_EN);

	/* invert AQR405 IRQ pins polarity */
	out_le32(irq_ccsr + IRQCR_OFFSET / 4, AQR405_IRQ_MASK);

	return 0;
}

int board_early_init_f(void)
{
	fsl_lsch3_early_init_f();
	return 0;
}

#ifdef CONFIG_SYS_FSL_HAS_DP_DDR
void detail_board_ddr_info(void)
{
	puts("\nDDR    ");
	print_size(gd->bd->bi_dram[0].size + gd->bd->bi_dram[1].size, "");
	print_ddr_info(0);
	if (gd->bd->bi_dram[2].size) {
		puts("\nDP-DDR ");
		print_size(gd->bd->bi_dram[2].size, "");
		print_ddr_info(CONFIG_DP_DDR_CTRL);
	}
}
#endif
int dram_init(void)
{
	gd->ram_size = initdram(0);

	return 0;
}

#if defined(CONFIG_ARCH_MISC_INIT)
int arch_misc_init(void)
{
#ifdef CONFIG_FSL_DEBUG_SERVER
	debug_server_init();
#endif
#ifdef CONFIG_FSL_CAAM
	sec_init();
#endif
	return 0;
}
#endif

#ifdef CONFIG_FSL_MC_ENET
void fdt_fixup_board_enet(void *fdt)
{
	int offset;

	offset = fdt_path_offset(fdt, "/fsl-mc");

	if (offset < 0)
		offset = fdt_path_offset(fdt, "/fsl,dprc@0");

	if (offset < 0) {
		printf("%s: ERROR: fsl-mc node not found in device tree (error %d)\n",
		       __func__, offset);
		return;
	}

	if (get_mc_boot_status() == 0)
		fdt_status_okay(fdt, offset);
	else
		fdt_status_fail(fdt, offset);
}
#endif

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	int err;
	u64 base[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	ft_cpu_setup(blob, bd);

	/* fixup DT for the two GPP DDR banks */
	base[0] = gd->bd->bi_dram[0].start;
	size[0] = gd->bd->bi_dram[0].size;
	base[1] = gd->bd->bi_dram[1].start;
	size[1] = gd->bd->bi_dram[1].size;

	fdt_fixup_memory_banks(blob, base, size, 2);

#ifdef CONFIG_FSL_MC_ENET
	fdt_fixup_board_enet(blob);
	err = fsl_mc_ldpaa_exit(bd);
	if (err)
		return err;
#endif

	return 0;
}
#endif

void qixis_dump_switch(void)
{
	int i, nr_of_cfgsw;

	QIXIS_WRITE(cms[0], 0x00);
	nr_of_cfgsw = QIXIS_READ(cms[1]);

	puts("DIP switch settings dump:\n");
	for (i = 1; i <= nr_of_cfgsw; i++) {
		QIXIS_WRITE(cms[0], i);
		printf("SW%d = (0x%02x)\n", i, QIXIS_READ(cms[1]));
	}
}

/*
 * Board rev C and earlier has duplicated I2C addresses for 2nd controller.
 * Both slots has 0x54, resulting 2nd slot unusable.
 */
void update_spd_address(unsigned int ctrl_num,
			unsigned int slot,
			unsigned int *addr)
{
	u8 sw;

	sw = QIXIS_READ(arch);
	if ((sw & 0xf) < 0x3) {
		if (ctrl_num == 1 && slot == 0)
			*addr = SPD_EEPROM_ADDRESS4;
		else if (ctrl_num == 1 && slot == 1)
			*addr = SPD_EEPROM_ADDRESS3;
	}
}

/*
 * Get the i2c address configuration for the IR regulator chip
 *
 * There are some variance in the RDB HW regarding the I2C address configuration
 * for the IR regulator chip, which is likely a problem of external resistor
 * accuracy. So we just check each address in a hopefully non-intrusive mode
 * and use the first one that seems to work
 *
 * The IR chip can show up under the following addresses:
 * 0x08 (Verified on T1040RDB-PA,T4240RDB-PB,X-T4240RDB-16GPA)
 * 0x09 (Verified on T1040RDB-PA)
 * 0x38 (Verified on T2080QDS, T2081QDS)
 */
static int find_ir_chip_on_i2c(void)
{
	int i2caddress;
	int ret;
	u8 byte;
	int i;
	const int ir_i2c_addr[] = {0x38, 0x08, 0x09};

	/* Check all the address */
	for (i = 0; i < (sizeof(ir_i2c_addr) / sizeof(ir_i2c_addr[0])); i++) {
		i2caddress = ir_i2c_addr[i];
		ret = i2c_read(i2caddress,
			       IR3565B_MFR_ID_OFFSET, 1, (void *)&byte,
			       sizeof(byte));
		if ((ret >= 0) && (byte == IR3565B_MFR_ID))
			return i2caddress;
	}
	return -1;
}

/* Maximum loop count waiting for new voltage to take effect */
#define MAX_LOOP_WAIT_NEW_VOL		100
/* Maximum loop count waiting for the voltage to be stable */
#define MAX_LOOP_WAIT_VOL_STABLE	100
/*
 * read_voltage from sensor on I2C bus
 * We use average of 4 readings, waiting for WAIT_FOR_ADC before
 * another reading
 */
#define NUM_READINGS    4       /* prefer to be power of 2 for efficiency */

#define WAIT_FOR_ADC	138	/* wait for 138 microseconds for ADC */
#define ADC_MIN_ACCURACY	4

/* read voltage from IR */
#ifdef CONFIG_VOL_MONITOR_IR3565B_READ
static int read_voltage_from_IR(int i2caddress)
{
	int i, ret, voltage_read = 0;
	u16 vol_mon;
	u8 buf;

	for (i = 0; i < NUM_READINGS; i++) {
		ret = i2c_read(I2C_VOL_MONITOR_ADDR,
			       IR3565B_LOOP1_VOUT_OFFSET,
			       1, (void *)&buf, 1);
		if (ret) {
			printf("VID: failed to read vcpu\n");
			return ret;
		}
		vol_mon = buf;
		if (!vol_mon) {
			printf("VID: Core voltage sensor error\n");
			return -1;
		}
		debug("VID: bus voltage reads 0x%02x\n", vol_mon);
		/* Resolution is 1/128V. We scale up here to get 1/128mV
		 * and divide at the end
		 */
		voltage_read += vol_mon * 1000;
		udelay(WAIT_FOR_ADC);
	}
	/* Scale down to the real mV as IR resolution is 1/128V, rounding up */
	voltage_read = DIV_ROUND_UP(voltage_read, 128);

	/* calculate the average */
	voltage_read /= NUM_READINGS;

	return voltage_read;
}
#endif

static int read_voltage(int i2caddress)
{
	int voltage_read;
#ifdef CONFIG_VOL_MONITOR_IR3565B_READ
	voltage_read = read_voltage_from_IR(i2caddress);
#else
	return -1;
#endif
	return voltage_read;
}

/*
 * We need to calculate how long before the voltage stops to drop
 * or increase. It returns with the loop count. Each loop takes
 * several readings (WAIT_FOR_ADC)
 */
static int wait_for_new_voltage(int vdd, int i2caddress)
{
	int timeout, vdd_current;

	vdd_current = read_voltage(i2caddress);
	/* wait until voltage starts to reach the target. Voltage slew
	 * rates by typical regulators will always lead to stable readings
	 * within each fairly long ADC interval in comparison to the
	 * intended voltage delta change until the target voltage is
	 * reached. The fairly small voltage delta change to any target
	 * VID voltage also means that this function will always complete
	 * within few iterations. If the timeout was ever reached, it would
	 * point to a serious failure in the regulator system.
	 */
	for (timeout = 0;
	     abs(vdd - vdd_current) > (IR_VDD_STEP_UP + IR_VDD_STEP_DOWN) &&
	     timeout < MAX_LOOP_WAIT_NEW_VOL; timeout++) {
		vdd_current = read_voltage(i2caddress);
	}
	if (timeout >= MAX_LOOP_WAIT_NEW_VOL) {
		printf("VID: Voltage adjustment timeout\n");
		return -1;
	}
	return timeout;
}

/*
 * this function keeps reading the voltage until it is stable or until the
 * timeout expires
 */
static int wait_for_voltage_stable(int i2caddress)
{
	int timeout, vdd_current, vdd;

	vdd = read_voltage(i2caddress);
	udelay(NUM_READINGS * WAIT_FOR_ADC);

	/* wait until voltage is stable */
	vdd_current = read_voltage(i2caddress);
	/* The maximum timeout is
	 * MAX_LOOP_WAIT_VOL_STABLE * NUM_READINGS * WAIT_FOR_ADC
	 */
	for (timeout = MAX_LOOP_WAIT_VOL_STABLE;
	     abs(vdd - vdd_current) > ADC_MIN_ACCURACY &&
	     timeout > 0; timeout--) {
		vdd = vdd_current;
		udelay(NUM_READINGS * WAIT_FOR_ADC);
		vdd_current = read_voltage(i2caddress);
	}
	if (timeout == 0)
		return -1;

	return vdd_current;
}

#ifdef CONFIG_VOL_MONITOR_IR3565B_SET
/* Set the voltage to the IR chip */
static int set_voltage_to_IR(int i2caddress, int vdd)
{
	int wait, vdd_last;
	int ret;
	u8 vid;

	vid = DIV_ROUND_UP(vdd - 245, 5);

	ret = i2c_write(i2caddress, IR3565B_LOOP1_MANUAL_ID_OFFSET,
			1, (void *)&vid, sizeof(vid));
	if (ret) {
		printf("VID: failed to write VID\n");
		return -1;
	}
	wait = wait_for_new_voltage(vdd, i2caddress);
	if (wait < 0)
		return -1;
	debug("VID: Waited %d us\n", wait * NUM_READINGS * WAIT_FOR_ADC);

	vdd_last = wait_for_voltage_stable(i2caddress);
	if (vdd_last < 0)
		return -1;
	debug("VID: Current voltage is %d mV\n", vdd_last);

	return vdd_last;
}
#endif

static int set_voltage(int i2caddress, int vdd)
{
	int vdd_last = -1;

#ifdef CONFIG_VOL_MONITOR_IR3565B_SET
	vdd_last = set_voltage_to_IR(i2caddress, vdd);
#else
	printf(" Error: Specific voltage monitor must be defined\n");
#endif
	return vdd_last;
}

int adjust_vdd(ulong vdd_override)
{
	int re_enable = disable_interrupts();
	u32 fusesr;
	u8 vid;
	int vdd_target, vdd_current, vdd_last;
	int ret, i2caddress;
	unsigned long vdd_string_override;
	char *vdd_string;
	static const uint16_t vdd[32] = {
		0,      /* unused */
		9875,   /* 0.9875V */
		9750,
		9625,
		9500,
		9375,
		9250,
		9125,
		9000,
		8875,
		8750,
		8625,
		8500,
		8375,
		8250,
		8125,
		10000,  /* 1.0000V */
		10125,
		10250,
		10375,
		10500,
		10625,
		10750,
		10875,
		11000,
		0,      /* reserved */
	};
	struct vdd_drive {
		u8 vid;
		unsigned voltage;
	};

	ret = select_i2c_ch_pca9547(I2C_MUX_CH_VOL_MONITOR);
	if (ret) {
		printf("VID: I2C failed to switch channel\n");
		ret = -1;
		goto exit;
	}
	ret = find_ir_chip_on_i2c();
	if (ret < 0) {
		printf("VID: Could not find voltage regulator on I2C.\n");
		ret = -1;
		goto exit;
	} else {
		i2caddress = ret;
		debug("VID: IR Chip found on I2C address 0x%02x\n", i2caddress);
	}

	fusesr = in_le32(CONFIG_SYS_DCFG_FUSESR);
	/*
	 * VID is used according to the table below
	 *                ---------------------------------------
	 *                |                DA_V                 |
	 *                |-------------------------------------|
	 *                | 5b00000 | 5b00001-5b11110 | 5b11111 |
	 * ---------------+---------+-----------------+---------|
	 * | D | 5b00000  | NO VID  | VID = DA_V      | NO VID  |
	 * | A |----------+---------+-----------------+---------|
	 * | _ | 5b00001  |VID =    | VID =           |VID =    |
	 * | V |   ~      | DA_V_ALT|   DA_V_ALT      | DA_A_VLT|
	 * | _ | 5b11110  |         |                 |         |
	 * | A |----------+---------+-----------------+---------|
	 * | L | 5b11111  | No VID  | VID = DA_V      | NO VID  |
	 * | T |          |         |                 |         |
	 * ------------------------------------------------------
	 */
	vid = (fusesr >> FSL_DCFG_FUSESR_ALTVID_SHIFT) &
		FSL_DCFG_FUSESR_ALTVID_MASK;
	if ((vid == 0) || (vid == FSL_DCFG_FUSESR_ALTVID_MASK)) {
		vid = (fusesr >> FSL_DCFG_FUSESR_VID_SHIFT) &
			FSL_DCFG_FUSESR_VID_MASK;
	}
	vdd_target = vdd[vid];

	/* check override variable for overriding VDD */
	vdd_string = getenv(CONFIG_VID_FLS_ENV);
	if (vdd_override == 0 && vdd_string &&
	    !strict_strtoul(vdd_string, 10, &vdd_string_override))
		vdd_override = vdd_string_override;
	if (vdd_override >= VDD_MV_MIN && vdd_override <= VDD_MV_MAX) {
		vdd_target = vdd_override * 10; /* convert to 1/10 mV */
		printf("VDD override is %lu\n", vdd_override);
	} else if (vdd_override != 0) {
		printf("Invalid value.\n");
	}
	if (vdd_target == 0) {
		debug("VID: VID not used\n");
		ret = 0;
		goto exit;
	} else {
		/* divide and round up by 10 to get a value in mV */
		vdd_target = DIV_ROUND_UP(vdd_target, 10);
		debug("VID: vid = %d mV\n", vdd_target);
	}

	/*
	 * Read voltage monitor to check real voltage.
	 */
	vdd_last = read_voltage(i2caddress);
	if (vdd_last < 0) {
		printf("VID: Couldn't read sensor abort VID adjustment\n");
		ret = -1;
		goto exit;
	}
	vdd_current = vdd_last;
	debug("VID: Core voltage is currently at %d mV\n", vdd_last);
	/*
	  * Adjust voltage to at or one step above target.
	  * As measurements are less precise than setting the values
	  * we may run through dummy steps that cancel each other
	  * when stepping up and then down.
	  */
	while (vdd_last > 0 &&
	       vdd_last < vdd_target) {
		vdd_current += IR_VDD_STEP_UP;
		vdd_last = set_voltage(i2caddress, vdd_current);
	}
	while (vdd_last > 0 &&
	       vdd_last > vdd_target + (IR_VDD_STEP_DOWN - 1)) {
		vdd_current -= IR_VDD_STEP_DOWN;
		vdd_last = set_voltage(i2caddress, vdd_current);
	}

	if (vdd_last > 0)
		printf("VID: Core voltage after adjustment is at %d mV\n",
		       vdd_last);
	else
		ret = -1;
exit:
	if (re_enable)
		enable_interrupts();
	return ret;
}

static int print_vdd(void)
{
	int vdd_last, ret, i2caddress;

	ret = select_i2c_ch_pca9547(I2C_MUX_CH_VOL_MONITOR);
	if (ret) {
		debug("VID : I2c failed to switch channel\n");
		return -1;
	}
	ret = find_ir_chip_on_i2c();
	if (ret < 0) {
		debug("VID: Could not find voltage regulator on I2C.\n");
		return -1;
	} else {
		i2caddress = ret;
		debug("VID: IR Chip found on I2C address 0x%02x\n", i2caddress);
	}

	/*
	 * Read voltage monitor to check real voltage.
	 */
	vdd_last = read_voltage(i2caddress);
	if (vdd_last < 0) {
		printf("VID: Couldn't read sensor abort VID adjustment\n");
		return -1;
	}
	printf("VID: Core voltage is at %d mV\n", vdd_last);

	return 0;
}

static int do_vdd_override(cmd_tbl_t *cmdtp, int flag, int argc,
				char * const argv[])
{
	ulong override;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strict_strtoul(argv[1], 10, &override))
		adjust_vdd(override);	/* the value is checked by callee */
	else
		return CMD_RET_USAGE;
	return 0;
}

static int do_vdd_read(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	if (argc < 1)
		return CMD_RET_USAGE;
	print_vdd();

	return 0;
}

int misc_init_r(void)
{
	if (hwconfig("sdhc"))
		config_board_mux(MUX_TYPE_SDHC);

	print_vdd();

	return 0;
}

U_BOOT_CMD(
	vdd_override, 2, 0, do_vdd_override,
	"override VDD",
	" - override with the voltage specified in mV, eg. 1050"
);

U_BOOT_CMD(
	vdd_read, 1, 0, do_vdd_read,
	"read VDD",
	" - Read the voltage specified in mV"
)
