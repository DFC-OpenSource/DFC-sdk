/*
 * Copyright 2014-2015 Freescale Semiconductor
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fsl_immap.h>
#include <fsl_ifc.h>
#include <ahci.h>
#include <scsi.h>
#include <asm/arch/soc.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <asm/arch-fsl-layerscape/config.h>
#include <fsl_ddr_sdram.h>
#include <fsl_ddr.h>
#ifdef CONFIG_CHAIN_OF_TRUST
#include <fsl_validate.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static void erratum_a009008(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A009008
#if defined(CONFIG_LS1043A)
	u32 __iomem *scfg = (u32 __iomem *)SCFG_BASE;
	u32 val = scfg_in32(scfg + SCFG_USB3PRM1CR_USB1 / 4);
	val &= ~(0xF << 6);
	scfg_out32(scfg + SCFG_USB3PRM1CR_USB1 / 4, val|(USB_TXVREFTUNE << 6));
	val = scfg_in32(scfg + SCFG_USB3PRM1CR_USB2 / 4);
	val &= ~(0xF << 6);
	scfg_out32(scfg + SCFG_USB3PRM1CR_USB2 / 4, val|(USB_TXVREFTUNE << 6));
	val = scfg_in32(scfg + SCFG_USB3PRM1CR_USB3 / 4);
	val &= ~(0xF << 6);
	scfg_out32(scfg + SCFG_USB3PRM1CR_USB3 / 4, val|(USB_TXVREFTUNE << 6));
#elif defined(CONFIG_LS2080A) || defined(CONFIG_LS2085A)
	u32 __iomem *scfg = (u32 __iomem *)SCFG_BASE;
	u32 val = scfg_in32(scfg + SCFG_USB3PRM1CR / 4);
	val &= ~(0xF << 6);
	scfg_out32(scfg + SCFG_USB3PRM1CR / 4, val|(USB_TXVREFTUNE << 6));
#endif
#endif /* CONFIG_SYS_FSL_ERRATUM_A009008 */
}

static void erratum_a009798(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A009798
#if defined(CONFIG_LS1043A)
	u32 __iomem *scfg = (u32 __iomem *)SCFG_BASE;
	u32 val = scfg_in32(scfg + SCFG_USB3PRM1CR_USB1 / 4);
	scfg_out32(scfg + SCFG_USB3PRM1CR_USB1 / 4 , val & USB_SQRXTUNE);
	val = gur_in32(scfg + SCFG_USB3PRM1CR_USB2 / 4);
	scfg_out32(scfg + SCFG_USB3PRM1CR_USB2 / 4 , val & USB_SQRXTUNE);
	val = scfg_in32(scfg + SCFG_USB3PRM1CR_USB3 / 4);
	scfg_out32(scfg + SCFG_USB3PRM1CR_USB3 / 4 , val & USB_SQRXTUNE);
#elif defined(CONFIG_LS2080A) || defined(CONFIG_LS2085A)
	u32 __iomem *scfg = (u32 __iomem *)SCFG_BASE;
	u32 val = scfg_in32(scfg + SCFG_USB3PRM1CR / 4);
	scfg_out32(scfg + SCFG_USB3PRM1CR / 4, val & USB_SQRXTUNE);
#endif
#endif /* CONFIG_SYS_FSL_ERRATUM_A009798 */
}

static void erratum_a008997(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A008997
#if defined(CONFIG_LS1043A)
	u32 __iomem *scfg = (u32 __iomem *)SCFG_BASE;
	u32 val = scfg_in32(scfg + SCFG_USB3PRM2CR_USB1 / 4);
	val &= ~(0x7F << 9);
	scfg_out32(scfg + SCFG_USB3PRM2CR_USB1 / 4,
		   val | (USB_PCSTXSWINGFULL << 9));
	val = scfg_in32(scfg + SCFG_USB3PRM2CR_USB2 / 4);
	val &= ~(0x7F << 9);
	scfg_out32(scfg + SCFG_USB3PRM2CR_USB2 / 4,
		   val | (USB_PCSTXSWINGFULL << 9));
	val = scfg_in32(scfg + SCFG_USB3PRM2CR_USB3 / 4);
	val &= ~(0x7F << 9);
	scfg_out32(scfg + SCFG_USB3PRM2CR_USB3 / 4,
		   val | (USB_PCSTXSWINGFULL << 9));
#elif defined(CONFIG_LS2080A) || defined(CONFIG_LS2085A)
	u32 __iomem *scfg = (u32 __iomem *)SCFG_BASE;
	u32 val = scfg_in32(scfg + SCFG_USB3PRM2CR / 4);
	val &= ~(0x7F << 9);
	scfg_out32(scfg + SCFG_USB3PRM2CR / 4,
		   val | (USB_PCSTXSWINGFULL << 9));
#endif
#endif /* CONFIG_SYS_FSL_ERRATUM_A008997 */
}

static void erratum_a009007(void)
{
/* TODO:implement the out_be16 instead of writew which is taking
little endian style */
#if defined(CONFIG_LS1043A)
	u32 __iomem *usb_phy = (u32 __iomem *)USB_PHY1;
	writew(USB_PHY_RX_EQ_VAL_1, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_2, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_3, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_4, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	usb_phy = (u32 __iomem *)USB_PHY2;
	writew(USB_PHY_RX_EQ_VAL_1, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_2, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_3, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_4, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	usb_phy = (u32 __iomem *)USB_PHY3;
	writew(USB_PHY_RX_EQ_VAL_1, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_2, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_3, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_4, (u8 *)(usb_phy) + USB_PHY_RX_OVRD_IN_HI);
#elif defined(CONFIG_LS2080A) || defined(CONFIG_LS2085A)
	u32 __iomem *dcsr = (u32 __iomem *)DCSR_BASE;
	writew(USB_PHY_RX_EQ_VAL_1,
	       (u8 *)(dcsr) + DCSR_USB_PHY1 + DCSR_USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_2,
	       (u8 *)(dcsr) + DCSR_USB_PHY1 + DCSR_USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_3,
	       (u8 *)(dcsr) + DCSR_USB_PHY1 + DCSR_USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_4,
	       (u8 *)(dcsr) + DCSR_USB_PHY1 + DCSR_USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_1,
	       (u8 *)(dcsr) + DCSR_USB_PHY2 + DCSR_USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_2,
	       (u8 *)(dcsr) + DCSR_USB_PHY2 + DCSR_USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_3,
	       (u8 *)(dcsr) + DCSR_USB_PHY2 + DCSR_USB_PHY_RX_OVRD_IN_HI);
	writew(USB_PHY_RX_EQ_VAL_4,
	       (u8 *)(dcsr) + DCSR_USB_PHY2 + DCSR_USB_PHY_RX_OVRD_IN_HI);
#endif /* CONFIG_SYS_FSL_ERRATUM_A009007 */
}

#ifdef CONFIG_FSL_LSCH3
/*
 * This erratum requires setting a value to eddrtqcr1 to
 * optimal the DDR performance.
 */
static void erratum_a008336(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A008336
	u32 *eddrtqcr1;

#ifdef CONFIG_SYS_FSL_DCSR_DDR_ADDR
	eddrtqcr1 = (void *)CONFIG_SYS_FSL_DCSR_DDR_ADDR + 0x800;
	if (fsl_ddr_get_version(0) == 0x50200)
		out_le32(eddrtqcr1, 0x63b30002);
#endif
#ifdef CONFIG_SYS_FSL_DCSR_DDR2_ADDR
	eddrtqcr1 = (void *)CONFIG_SYS_FSL_DCSR_DDR2_ADDR + 0x800;
	if (fsl_ddr_get_version(0) == 0x50200)
		out_le32(eddrtqcr1, 0x63b30002);
#endif
#endif
}

/*
 * This erratum requires a register write before being Memory
 * controller 3 being enabled.
 */
static void erratum_a008514(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A008514
	u32 *eddrtqcr1;

#ifdef CONFIG_SYS_FSL_DCSR_DDR3_ADDR
	eddrtqcr1 = (void *)CONFIG_SYS_FSL_DCSR_DDR3_ADDR + 0x800;
	out_le32(eddrtqcr1, 0x63b20002);
#endif
#endif
}
#ifdef CONFIG_SYS_FSL_ERRATUM_A009635
#define PLATFORM_CYCLE_ENV_VAR	"a009635_interval_val"

static unsigned long get_internval_val_mhz(void)
{
	char *interval = getenv(PLATFORM_CYCLE_ENV_VAR);
	/*
	 *  interval is the number of platform cycles(MHz) between
	 *  wake up events generated by EPU.
	 */
	ulong interval_mhz = get_bus_freq(0) / (1000 * 1000);

	if (interval)
		interval_mhz = simple_strtoul(interval, NULL, 10);

	return interval_mhz;
}

void erratum_a009635(void)
{
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	unsigned int svr, ver;
	u32 val;
	unsigned long interval_mhz = get_internval_val_mhz();

	svr = gur_in32(&gur->svr);
	ver = SVR_SOC_VER(svr);
	switch (ver) {
	case SVR_LS2080:
	case SVR_LS2085:
	case SVR_LS2040:
	case SVR_LS2045:
		if (!interval_mhz)
			return;

		val = in_le32(DCSR_CGACRE5);
		writel(val | 0x00000200, DCSR_CGACRE5);

		val = in_le32(EPU_EPCMPR5);
		writel(interval_mhz, EPU_EPCMPR5);
		val = in_le32(EPU_EPCCR5);
		writel(val | 0x82820000, EPU_EPCCR5);
		val = in_le32(EPU_EPSMCR5);
		writel(val | 0x002f0000, EPU_EPSMCR5);
		val = in_le32(EPU_EPECR5);
		writel(val | 0x20000000, EPU_EPECR5);
		val = in_le32(EPU_EPGCR);
		writel(val | 0x80000000, EPU_EPGCR);
		break;
	default:
		break;
	}
}
#endif	/* CONFIG_SYS_FSL_ERRATUM_A009635 */

static void erratum_a008751(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A008751
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	unsigned int svr, ver;
	u32 __iomem *scfg = (u32 __iomem *)SCFG_BASE;
	svr = gur_in32(&gur->svr);
	ver = SVR_SOC_VER(svr);
	switch (ver) {
	case SVR_LS2080:
	case SVR_LS2085:
	case SVR_LS2040:
	case SVR_LS2045:
		writel(0x27672b2a, scfg + SCFG_USB3PRM1CR / 4);
		break;
	default:
		break;
	}
#endif
}

static void erratum_rcw_src(void)
{
#if defined(CONFIG_SPL) && defined(CONFIG_NAND_BOOT)
	u32 __iomem *dcfg_ccsr = (u32 __iomem *)DCFG_BASE;
	u32 __iomem *dcfg_dcsr = (u32 __iomem *)DCFG_DCSR_BASE;
	u32 val;

	val = in_le32(dcfg_ccsr + DCFG_PORSR1 / 4);
	val &= ~DCFG_PORSR1_RCW_SRC;
	val |= DCFG_PORSR1_RCW_SRC_NOR;
	out_le32(dcfg_dcsr + DCFG_DCSR_PORCR1 / 4, val);
#endif
}

#define I2C_DEBUG_REG 0x6
#define I2C_GLITCH_EN 0x8
/*
 * This erratum requires setting glitch_en bit to enable
 * digital glitch filter to improve clock stability.
 */
static void erratum_a009203(void)
{
	u8 __iomem *ptr;
#ifdef CONFIG_SYS_I2C
#ifdef I2C1_BASE_ADDR
	ptr = (u8 __iomem *)(I2C1_BASE_ADDR + I2C_DEBUG_REG);

	writeb(I2C_GLITCH_EN, ptr);
#endif
#ifdef I2C2_BASE_ADDR
	ptr = (u8 __iomem *)(I2C2_BASE_ADDR + I2C_DEBUG_REG);

	writeb(I2C_GLITCH_EN, ptr);
#endif
#ifdef I2C3_BASE_ADDR
	ptr = (u8 __iomem *)(I2C3_BASE_ADDR + I2C_DEBUG_REG);

	writeb(I2C_GLITCH_EN, ptr);
#endif
#ifdef I2C4_BASE_ADDR
	ptr = (u8 __iomem *)(I2C4_BASE_ADDR + I2C_DEBUG_REG);

	writeb(I2C_GLITCH_EN, ptr);
#endif
#endif
}
void bypass_smmu(void)
{
	u32 val;
	val = (in_le32(SMMU_SCR0) | SCR0_CLIENTPD_MASK) & ~(SCR0_USFCFG_MASK);
	out_le32(SMMU_SCR0, val);
	val = (in_le32(SMMU_NSCR0) | SCR0_CLIENTPD_MASK) & ~(SCR0_USFCFG_MASK);
	out_le32(SMMU_NSCR0, val);
}
void fsl_lsch3_early_init_f(void)
{
#ifdef CONFIG_SYS_FSL_HAS_CCI400
	struct ccsr_cci400 *cci = (struct ccsr_cci400 *)CONFIG_SYS_CCI400_ADDR;

	out_le32(&cci->slave[3].snoop_ctrl, CCI400_SNOOP_REQ_EN);
	out_le32(&cci->slave[4].snoop_ctrl, CCI400_SNOOP_REQ_EN);

	/*
	 * Workaround for the issue that DDR could not respond to
	 * barrier transaction which is generated by executing DSB/ISB
	 * instruction. Set CCI-400 control override register to
	 * terminate the barrier transaction. After DDR is initialized,
	 * allow barrier transaction to DDR again
	 */
	out_le32(&cci->ctrl_ord, CCI400_CTRLORD_TERM_BARRIER);
#endif

	erratum_a008751();
	erratum_a009008();
	erratum_a009798();
	erratum_a008997();
	erratum_a009007();
	erratum_rcw_src();
	init_early_memctl_regs();	/* tighten IFC timing */
	erratum_a009203();
	erratum_a008514();
	erratum_a008336();
#ifdef CONFIG_CHAIN_OF_TRUST
	/* In case of Secure Boot, the IBR configures the SMMU
	* to allow only Secure transactions.
	* SMMU must be reset in bypass mode.
	* Set the ClientPD bit and Clear the USFCFG Bit
	*/
	if (fsl_check_boot_mode_secure() == 1)
		bypass_smmu();
#endif
}

#ifdef CONFIG_SCSI_AHCI_PLAT
int sata_init(void)
{
	struct ccsr_ahci __iomem *ccsr_ahci;

#ifdef CONFIG_SYS_SATA2
	ccsr_ahci  = (void *)CONFIG_SYS_SATA2;
	out_le32(&ccsr_ahci->ppcfg, AHCI_PORT_PHY_1_CFG);
	out_le32(&ccsr_ahci->ptc, AHCI_PORT_TRANS_CFG);
#endif

#ifdef CONFIG_SYS_SATA1
	ccsr_ahci  = (void *)CONFIG_SYS_SATA1;
	out_le32(&ccsr_ahci->ppcfg, AHCI_PORT_PHY_1_CFG);
	out_le32(&ccsr_ahci->ptc, AHCI_PORT_TRANS_CFG);

	ahci_init((void __iomem *)CONFIG_SYS_SATA1);
	scsi_scan(0);

#endif
	return 0;
}
#endif

#elif defined(CONFIG_LS1043A)
#ifdef CONFIG_SCSI_AHCI_PLAT
int sata_init(void)
{
	struct ccsr_ahci __iomem *ccsr_ahci = (void *)CONFIG_SYS_SATA;

	out_le32(&ccsr_ahci->ppcfg, AHCI_PORT_PHY_1_CFG);
	out_le32(&ccsr_ahci->pp2c, AHCI_PORT_PHY_2_CFG);
	out_le32(&ccsr_ahci->pp3c, AHCI_PORT_PHY_3_CFG);
	out_le32(&ccsr_ahci->ptc, AHCI_PORT_TRANS_CFG);

	ahci_init((void __iomem *)CONFIG_SYS_SATA);
	scsi_scan(0);

	return 0;
}
#endif

static void erratum_a009929(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A009929
	struct ccsr_gur *gur = (void *)CONFIG_SYS_FSL_GUTS_ADDR;
	u32 __iomem *dcsr_cop_ccp = (void *)CONFIG_SYS_DCSR_COP_CCP_ADDR;
	u32 rstrqmr1 = gur_in32(&gur->rstrqmr1);

	rstrqmr1 |= 0x00000400;
	gur_out32(&gur->rstrqmr1, rstrqmr1);
	writel(0x01000000, dcsr_cop_ccp);
#endif
}

/*
 * This erratum requires setting a value to eddrtqcr1 to
 * optimal the DDR performance.
 */
static void erratum_a009660(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A009660
	u32 *eddrtqcr1 = (void *)CONFIG_SYS_FSL_SCFG_ADDR + 0x20c;
	out_be32(eddrtqcr1, 0x63b20042);
#endif
}

static void erratum_a008850_early(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A008850
	/* part 1 of 2 */
	struct ccsr_cci400 __iomem *cci = (void *)CONFIG_SYS_CCI400_ADDR;
	struct ccsr_ddr __iomem *ddr = (void *)CONFIG_SYS_FSL_DDR_ADDR;

	/* disables propagation of barrier transactions to DDRC from CCI400 */
	out_le32(&cci->ctrl_ord, CCI400_CTRLORD_TERM_BARRIER);

	/* disable the re-ordering in DDRC */
	ddr_out32(&ddr->eor, DDR_EOR_RD_REOD_DIS | DDR_EOR_WD_REOD_DIS);
#endif
}

void erratum_a008850_post(void)
{
#ifdef CONFIG_SYS_FSL_ERRATUM_A008850
	/* part 2 of 2 */
	struct ccsr_cci400 __iomem *cci = (void *)CONFIG_SYS_CCI400_ADDR;
	struct ccsr_ddr __iomem *ddr = (void *)CONFIG_SYS_FSL_DDR_ADDR;
	u32 tmp;

	/* enable propagation of barrier transactions to DDRC from CCI400 */
	out_le32(&cci->ctrl_ord, CCI400_CTRLORD_EN_BARRIER);

	/* enable the re-ordering in DDRC */
	tmp = ddr_in32(&ddr->eor);
	tmp &= ~(DDR_EOR_RD_REOD_DIS | DDR_EOR_WD_REOD_DIS);
	ddr_out32(&ddr->eor, tmp);
#endif
}

void fsl_lsch2_early_init_f(void)
{
	struct ccsr_cci400 *cci = (struct ccsr_cci400 *)CONFIG_SYS_CCI400_ADDR;
	struct ccsr_scfg *scfg = (struct ccsr_scfg *)CONFIG_SYS_FSL_SCFG_ADDR;

#ifdef CONFIG_FSL_IFC
	init_early_memctl_regs();	/* tighten IFC timing */
#endif

#if defined(CONFIG_FSL_QSPI) && !defined(CONFIG_QSPI_BOOT)
	out_be32(&scfg->qspi_cfg, SCFG_QSPI_CLKSEL);
#endif
	/* Make SEC reads and writes snoopable */
	setbits_be32(&scfg->snpcnfgcr, SCFG_SNPCNFGCR_SECRDSNP |
		     SCFG_SNPCNFGCR_SECWRSNP);

	/*
	 * Enable snoop requests and DVM message requests for
	 * Slave insterface S4 (A53 core cluster)
	 */
	out_le32(&cci->slave[4].snoop_ctrl,
		 CCI400_DVM_MESSAGE_REQ_EN | CCI400_SNOOP_REQ_EN);

	/* Erratum */
	erratum_a008850_early(); /* part 1 of 2 */
	erratum_a009929();
	erratum_a009660();
	erratum_a009008();
	erratum_a009798();
	erratum_a008997();
	erratum_a009007();
}
#endif

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#ifdef CONFIG_SCSI_AHCI_PLAT
	sata_init();
#endif
#ifdef CONFIG_CHAIN_OF_TRUST
	fsl_setenv_chain_of_trust();
#endif

	return 0;
}
#endif
