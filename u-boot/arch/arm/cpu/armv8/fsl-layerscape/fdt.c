/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <libfdt.h>
#include <fdt_support.h>
#include <phy.h>
#ifdef CONFIG_FSL_LSCH3
#include <asm/arch/fdt.h>
#endif
#ifdef CONFIG_FSL_ESDHC
#include <fsl_esdhc.h>
#endif
#ifdef CONFIG_SYS_DPAA_FMAN
#include <fsl_fman.h>
#endif
#ifdef CONFIG_MP
#include <asm/arch/mp.h>
#endif
#include <asm/io.h>
#include <asm/arch/speed.h>
#include <fsl_sec.h>
#include <asm/arch-fsl-layerscape/soc.h>

#if defined(CONFIG_TARGET_LS2080ARDB) || defined(CONFIG_TARGET_LS2080AQDS)
#include <asm/arch-fsl-layerscape/immap_lsch3.h>
#define SYS_CLK_FREQ_533MHZ    533
#endif
#ifdef CONFIG_FSL_LS_PPA
#include <asm/armv8/sec_firmware.h>
#endif

int fdt_fixup_phy_connection(void *blob, int offset, phy_interface_t phyc)
{
	return fdt_setprop_string(blob, offset, "phy-connection-type",
					 phy_string_for_interface(phyc));
}

#if defined(CONFIG_SYS_DPAA_QBMAN)

#define BMAN_IP_REV_1 0xBF8
#define BMAN_IP_REV_2 0xBFC
void fdt_fixup_bportals(void *blob)
{
	int off, err;
	unsigned int maj, min;
	unsigned int ip_cfg;

	u32 rev_1 = in_be32(CONFIG_SYS_FSL_BMAN_ADDR + BMAN_IP_REV_1);
	u32 rev_2 = in_be32(CONFIG_SYS_FSL_BMAN_ADDR + BMAN_IP_REV_2);
	char compat[64];
	int compat_len;

	maj = (rev_1 >> 8) & 0xff;
	min = rev_1 & 0xff;

	ip_cfg = rev_2 & 0xff;

	compat_len = sprintf(compat, "fsl,bman-portal-%u.%u.%u",
			     maj, min, ip_cfg) + 1;
	compat_len += sprintf(compat + compat_len, "fsl,bman-portal") + 1;

	off = fdt_node_offset_by_compatible(blob, -1, "fsl,bman-portal");
	while (off != -FDT_ERR_NOTFOUND) {
		err = fdt_setprop(blob, off, "compatible", compat, compat_len);
		if (err < 0) {
			printf("ERROR: unable to create props for %s: %s\n",
			       fdt_get_name(blob, off, NULL),
			       fdt_strerror(err));
			return;
		}

		off = fdt_node_offset_by_compatible(blob, off,
						    "fsl,bman-portal");
	}
}

#define QMAN_IP_REV_1 0xBF8
#define QMAN_IP_REV_2 0xBFC
void fdt_fixup_qportals(void *blob)
{
	int off, err;
	unsigned int maj, min;
	unsigned int ip_cfg;
	u32 rev_1 = in_be32(CONFIG_SYS_FSL_QMAN_ADDR + QMAN_IP_REV_1);
	u32 rev_2 = in_be32(CONFIG_SYS_FSL_QMAN_ADDR + QMAN_IP_REV_2);
	char compat[64];
	int compat_len;

	maj = (rev_1 >> 8) & 0xff;
	min = rev_1 & 0xff;
	ip_cfg = rev_2 & 0xff;

	compat_len = sprintf(compat, "fsl,qman-portal-%u.%u.%u",
			     maj, min, ip_cfg) + 1;
	compat_len += sprintf(compat + compat_len, "fsl,qman-portal") + 1;

	off = fdt_node_offset_by_compatible(blob, -1, "fsl,qman-portal");
	while (off != -FDT_ERR_NOTFOUND) {
		err = fdt_setprop(blob, off, "compatible", compat, compat_len);
		if (err < 0) {
			printf("ERROR: unable to create props for %s: %s\n",
			       fdt_get_name(blob, off, NULL),
			       fdt_strerror(err));
			return;
		}
		off = fdt_node_offset_by_compatible(blob, off,
						    "fsl,qman-portal");
	}
}
#endif

#ifdef CONFIG_MP
void ft_fixup_cpu(void *blob)
{
	int off;
	__maybe_unused u64 spin_tbl_addr = (u64)get_spin_tbl_addr();
	fdt32_t *reg;
	int addr_cells;
	u64 val, core_id;
	size_t *boot_code_size = &(__secondary_boot_code_size);
#if defined(CONFIG_FSL_LS_PPA) && defined(CONFIG_ARMV8_PSCI)
	int node;
#endif

#if defined(CONFIG_FSL_LS_PPA) && defined(CONFIG_ARMV8_PSCI)
	if (sec_firmware_validate()) {
		/* remove psci DT node */
		node = fdt_path_offset(blob, "/psci");
		if (node >= 0)
			goto remove_psci_node;

		node = fdt_node_offset_by_compatible(blob, -1, "arm,psci");
		if (node >= 0)
			goto remove_psci_node;

		node = fdt_node_offset_by_compatible(blob, -1, "arm,psci-0.2");
		if (node >= 0)
			goto remove_psci_node;

		node = fdt_node_offset_by_compatible(blob, -1, "arm,psci-1.0");
		if (node >= 0)
			goto remove_psci_node;

remove_psci_node:
		if (node >= 0)
			fdt_del_node(blob, node);
	} else {
		return;
	}
#endif
	off = fdt_path_offset(blob, "/cpus");
	if (off < 0) {
		puts("couldn't find /cpus node\n");
		return;
	}
	of_bus_default_count_cells(blob, off, &addr_cells, NULL);

	off = fdt_node_offset_by_prop_value(blob, -1, "device_type", "cpu", 4);
	while (off != -FDT_ERR_NOTFOUND) {
		reg = (fdt32_t *)fdt_getprop(blob, off, "reg", 0);
		if (reg) {
			core_id = of_read_number(reg, addr_cells);
			if (core_id  == 0 || (is_core_online(core_id))) {
				val = spin_tbl_addr;
				val += id_to_core(core_id) *
				       SPIN_TABLE_ELEM_SIZE;
				val = cpu_to_fdt64(val);
				fdt_setprop_string(blob, off, "enable-method",
						   "spin-table");
				fdt_setprop(blob, off, "cpu-release-addr",
					    &val, sizeof(val));
			} else {
				debug("skipping offline core\n");
			}
		} else {
			puts("Warning: found cpu node without reg property\n");
		}
		off = fdt_node_offset_by_prop_value(blob, off, "device_type",
						    "cpu", 4);
	}

	fdt_add_mem_rsv(blob, (uintptr_t)&secondary_boot_code,
			*boot_code_size);
}
#endif

/*
 * the burden is on the the caller to not request a count
 * exceeding the bounds of the stream_ids[] array
 */
void alloc_stream_ids(int start_id, int count, u32 *stream_ids, int max_cnt)
{
	int i;

	if (count > max_cnt) {
		printf("\n%s: ERROR: max per-device stream ID count exceed\n",
		       __func__);
		return;
	}

	for (i = 0; i < count; i++)
		stream_ids[i] = start_id++;
}

/*
 * This function updates the mmu-masters property on the SMMU
 * node as per the SMMU binding-- phandle and list of stream IDs
 * for each MMU master.
 */
void append_mmu_masters(void *blob, const char *smmu_path,
			const char *master_name, u32 *stream_ids, int count)
{
	u32 phandle;
	int smmu_nodeoffset;
	int master_nodeoffset;
	int i;

	/* get phandle of mmu master device */
	master_nodeoffset = fdt_path_offset(blob, master_name);
	if (master_nodeoffset < 0) {
		printf("\n%s: ERROR: master not found\n", __func__);
		return;
	}
	phandle = fdt_get_phandle(blob, master_nodeoffset);
	if (!phandle) { /* if master has no phandle, create one */
		phandle = fdt_create_phandle(blob, master_nodeoffset);
		if (!phandle) {
			printf("\n%s: ERROR: unable to create phandle\n",
			       __func__);
			return;
		}
	}

	/* append it to mmu-masters */
	smmu_nodeoffset = fdt_path_offset(blob, smmu_path);
	if (fdt_appendprop_u32(blob, smmu_nodeoffset, "mmu-masters",
			       phandle) < 0) {
		printf("\n%s: ERROR: unable to update SMMU node\n", __func__);
		return;
	}

	/* for each stream ID, append to mmu-masters */
	for (i = 0; i < count; i++) {
		fdt_appendprop_u32(blob, smmu_nodeoffset, "mmu-masters",
				   stream_ids[i]);
	}

	/* fix up #stream-id-cells with stream ID count */
	if (fdt_setprop_u32(blob, master_nodeoffset, "#stream-id-cells",
			    count) < 0)
		printf("\n%s: ERROR: unable to update #stream-id-cells\n",
		       __func__);
}


/*
 * The info below summarizes how streamID partitioning works
 * for ls2080a and how it is conveyed to the OS via the device tree.
 *
 *  -non-PCI legacy, platform devices (USB, SD/MMC, SATA, DMA)
 *     -all legacy devices get a unique ICID assigned and programmed in
 *      their AMQR registers by u-boot
 *     -u-boot updates the hardware device tree with streamID properties
 *      for each platform/legacy device (smmu-masters property)
 *
 *  -PCIe
 *     -for each PCI controller that is active (as per RCW settings),
 *      u-boot will allocate a range of ICID and convey that to Linux via
 *      the device tree (smmu-masters property)
 *
 *  -DPAA2
 *     -u-boot will allocate a range of ICIDs to be used by the Management
 *      Complex for containers and will set these values in the MC DPC image.
 *     -the MC is responsible for allocating and setting up ICIDs
 *      for all DPAA2 devices.
 *
 */
#ifdef CONFIG_FSL_LSCH3
static void fdt_fixup_smmu(void *blob)
{
	int nodeoffset;

	nodeoffset = fdt_path_offset(blob, "/iommu@5000000");
	if (nodeoffset < 0) {
		printf("\n%s: WARNING: no SMMU node found\n", __func__);
		return;
	}

	/* fixup for all PCI controllers */
#ifdef CONFIG_PCI
	fdt_fixup_smmu_pcie(blob);
#endif
}
#endif

void ft_cpu_setup(void *blob, bd_t *bd)
{
#if defined(CONFIG_SYS_DPAA_QBMAN)
	struct sys_info sysinfo;

	get_sys_info(&sysinfo);
#endif

#ifdef CONFIG_FSL_LSCH2
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	unsigned int svr = in_be32(&gur->svr);

	/* delete crypto node if not on an E-processor */
	if (!IS_E_PROCESSOR(svr))
		fdt_fixup_crypto_node(blob, 0);
#if CONFIG_SYS_FSL_SEC_COMPAT >= 4
	else {
		ccsr_sec_t __iomem *sec;

		sec = (void __iomem *)CONFIG_SYS_FSL_SEC_ADDR;
		fdt_fixup_crypto_node(blob, sec_in32(&sec->secvid_ms));
	}
#endif
#endif

#ifdef CONFIG_MP
	ft_fixup_cpu(blob);
#endif

#ifdef CONFIG_SYS_NS16550
	do_fixup_by_compat_u32(blob, "fsl,ns16550",
			       "clock-frequency", CONFIG_SYS_NS16550_CLK, 1);
#endif

	do_fixup_by_compat_u32(blob, "fixed-clock",
			       "clock-frequency", CONFIG_SYS_CLK_FREQ, 1);

#ifdef CONFIG_PCI
	ft_pci_setup(blob, bd);
#endif

#ifdef CONFIG_FSL_ESDHC
	fdt_fixup_esdhc(blob, bd);
#endif

#if defined(CONFIG_SYS_DPAA_QBMAN)
	fdt_fixup_bportals(blob);
	fdt_fixup_qportals(blob);
	do_fixup_by_compat_u32(blob, "fsl,qman",
			       "clock-frequency", sysinfo.freq_qman, 1);
#endif

#ifdef CONFIG_FSL_LSCH3
	fdt_fixup_smmu(blob);
#endif

#ifdef CONFIG_SYS_DPAA_FMAN
	fdt_fixup_fman_firmware(blob);
#endif
#if defined(CONFIG_TARGET_LS2080ARDB) || defined(CONFIG_TARGET_LS2080AQDS)
	fdt_fixup_usb(blob);
#endif

#ifdef CONFIG_LS1043A
#ifdef CONFIG_FSL_QSPI
	do_fixup_by_compat(blob, "fsl,ifc",
			   "status", "disabled", 8 + 1, 1);
#endif
#endif
}

#if defined(CONFIG_TARGET_LS2080ARDB) || defined(CONFIG_TARGET_LS2080AQDS)
void fdt_fixup_usb(void *blob)
{
	int off;
	uint sys_clk;
	struct sys_info sysinfo;

	get_sys_info(&sysinfo);
	sys_clk = sysinfo.freq_systembus / 1000000;
	if (SYS_CLK_FREQ_533MHZ == sys_clk) {
		off = fdt_node_offset_by_compatible(blob, -1, "snps,dwc3");
		while (off != -FDT_ERR_NOTFOUND) {
			fdt_status_disabled(blob, off);
			off = fdt_node_offset_by_compatible(blob, off,
							    "snps,dwc3");
		}
	}
}
#endif
