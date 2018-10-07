/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <config.h>
#include <errno.h>
#include <malloc.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/types.h>
#include <asm/macro.h>
#include <asm/arch/soc.h>
#ifdef CONFIG_FSL_LSCH3
#include <asm/arch/immap_lsch3.h>
#elif defined(CONFIG_FSL_LSCH2)
#include <asm/arch/immap_lsch2.h>
#endif
#include <asm/armv8/sec_firmware.h>
#ifdef CONFIG_CHAIN_OF_TRUST
#include <fsl_validate.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

extern void c_runtime_cpu_setup(void);

#define LS_PPA_FIT_FIRMWARE_IMAGE	"firmware"
#define LS_PPA_FIT_CNF_NAME		"config@1"
#define PPA_MEM_SIZE_ENV_VAR		"ppamemsize"

/*
 * Return the actual size of the PPA private DRAM block.
 */
unsigned long ppa_get_dram_block_size(void)
{
	unsigned long dram_block_size = CONFIG_SYS_LS_PPA_DRAM_BLOCK_MIN_SIZE;

	char *dram_block_size_env_var = getenv(PPA_MEM_SIZE_ENV_VAR);

	if (dram_block_size_env_var) {
		dram_block_size = simple_strtoul(dram_block_size_env_var, NULL,
						 10);

		if (dram_block_size < CONFIG_SYS_LS_PPA_DRAM_BLOCK_MIN_SIZE) {
			printf("fsl-ppa: WARNING: Invalid value for \'"
			       PPA_MEM_SIZE_ENV_VAR
			       "\' environment variable: %lu\n",
			       dram_block_size);

			dram_block_size = CONFIG_SYS_LS_PPA_DRAM_BLOCK_MIN_SIZE;
		}
	}

	return dram_block_size;
}

#ifdef CONFIG_CHAIN_OF_TRUST
static void secboot_validate_ppa(void *ppa_addr)
{
	int ret;
	uintptr_t ppa_esbc_hdr = CONFIG_SYS_LS_PPA_ESBC_ADDR;
	uintptr_t ppa_img_addr = 0;

	ppa_img_addr = (uintptr_t)ppa_addr;

	if (fsl_check_boot_mode_secure() != 0) {
		printf("PPA validation started");
		/* fsl_secboot_validate() will return 0 in case of
		 * successful validation.
		 * If validation fails, reset would be issued for
		 * Production Environment (ITS = 1).
		 * For development environment (ITS = 0, SB_EN = 1),
		 * execution will be allowed to continue, so function
		 * will rturn a non-zero value
		 */
		ret = fsl_secboot_validate(ppa_esbc_hdr,
					   CONFIG_PPA_KEY_HASH,
					   &ppa_img_addr);
		if (ret != 0)
			printf("PPA validation failed\n");
		else
			printf("PPA validation Successful\n");
	}
}
#endif

static int ppa_firmware_validate(void *ppa_addr)
{
	void *fit_hdr;

	fit_hdr = ppa_addr;

	if (fdt_check_header(fit_hdr)) {
		printf("fsl-ppa: Bad firmware image (not a FIT image)\n");
		return -EINVAL;
	}

	if (!fit_check_format(fit_hdr)) {
		printf("fsl-ppa: Bad firmware image (bad FIT header)\n");
		return -EINVAL;
	}

	return 0;
}

static int ppa_firmware_get_data(void *ppa_addr,
				 const void **data, size_t *size)
{
	void *fit_hdr;
	int conf_node_off, fw_node_off;
	char *conf_node_name = NULL;
	char *desc;
	int ret;

	fit_hdr = ppa_addr;
	conf_node_name = LS_PPA_FIT_CNF_NAME;

	conf_node_off = fit_conf_get_node(fit_hdr, conf_node_name);
	if (conf_node_off < 0) {
		printf("fsl-ppa: %s: no such config\n", conf_node_name);
		return -ENOENT;
	}

	fw_node_off = fit_conf_get_prop_node(fit_hdr, conf_node_off,
			LS_PPA_FIT_FIRMWARE_IMAGE);
	if (fw_node_off < 0) {
		printf("fsl-ppa: No '%s' in config\n",
				LS_PPA_FIT_FIRMWARE_IMAGE);
		return -ENOLINK;
	}

	/* Verify PPA firmware image */
	if (!(fit_image_verify(fit_hdr, fw_node_off))) {
		printf("fsl-ppa: Bad firmware image (bad CRC)\n");
		return -EINVAL;
	}

	if (fit_image_get_data(fit_hdr, fw_node_off, data, size)) {
		printf("fsl-ppa: Can't get %s subimage data/size",
				LS_PPA_FIT_FIRMWARE_IMAGE);
		return -ENOENT;
	}

	ret = fit_get_desc(fit_hdr, fw_node_off, &desc);
	if (ret)
		printf("PPA Firmware unavailable\n");
	else
		printf("%s\n", desc);

	return 0;
}

/*
 * PPA firmware FIT image parser checks if the image is in FIT
 * format, verifies integrity of the image and calculates raw
 * image address and size values.
 *
 * Returns 0 on success and a negative errno on error task fail.
 */
static int ppa_parse_firmware_fit_image(const void **raw_image_addr,
				size_t *raw_image_size)
{
	void *ppa_addr;
	int ret;

#ifdef CONFIG_SYS_LS_PPA_FW_IN_XIP
	ppa_addr = (void *)CONFIG_SYS_LS_PPA_FW_ADDR;
#else
#error "No CONFIG_SYS_LS_PPA_FW_IN_xxx defined"
#endif

	ret = ppa_firmware_validate(ppa_addr);
	if (ret)
		return ret;

#ifdef CONFIG_CHAIN_OF_TRUST
	secboot_validate_ppa(ppa_addr);
#endif

	ret = ppa_firmware_get_data(ppa_addr, raw_image_addr, raw_image_size);
	if (ret)
		return ret;

	debug("fsl-ppa: raw_image_addr = 0x%p, raw_image_size = 0x%lx\n",
			*raw_image_addr, *raw_image_size);

	return 0;
}

static int ppa_copy_image(const char *title,
			 u64 image_addr, u32 image_size, u64 ppa_ram_addr)
{
	debug("%s copied to address 0x%p\n", title, (void *)ppa_ram_addr);
	memcpy((void *)ppa_ram_addr, (void *)image_addr, image_size);
	flush_dcache_range(ppa_ram_addr, ppa_ram_addr + image_size);

	return 0;
}

int sec_firmware_validate(void)
{
	void *ppa_addr;

#ifdef CONFIG_SYS_LS_PPA_FW_IN_XIP
	ppa_addr = (void *)CONFIG_SYS_LS_PPA_FW_ADDR;
#else
#error "No CONFIG_SYS_LS_PPA_FW_IN_xxx defined"
#endif

	return ppa_firmware_validate(ppa_addr);
}

#ifdef CONFIG_ARMV8_PSCI
unsigned int sec_firmware_support_psci_version(void)
{
	unsigned int psci_ver = 0;

	if (!sec_firmware_validate())
		psci_ver = ppa_support_psci_version();

	return psci_ver;
}
#endif

int ppa_init_pre(u64 *entry)
{
	u64 ppa_ram_addr;
	const void *raw_image_addr;
	size_t raw_image_size = 0;
	size_t ppa_ram_size = ppa_get_dram_block_size();
	int ret;

	debug("fsl-ppa: ppa size(0x%lx)\n", ppa_ram_size);

	/*
	 * The PPA must be stored in secure memory.
	 * Append PPA to secure mmu table.
	 */
	ppa_ram_addr = (gd->secure_ram & MEM_RESERVE_SECURE_ADDR_MASK) +
			gd->arch.tlb_size;

	/* Align PPA base address to 4K */
	ppa_ram_addr = (ppa_ram_addr + 0xfff) & ~0xfff;
	debug("fsl-ppa: PPA load address (0x%llx)\n", ppa_ram_addr);

	ret = ppa_parse_firmware_fit_image(&raw_image_addr, &raw_image_size);
	if (ret < 0)
		goto out;

	if (ppa_ram_size < raw_image_size) {
		ret = -ENOSPC;
		goto out;
	}

	ppa_copy_image("PPA firmware", (u64)raw_image_addr,
			raw_image_size, ppa_ram_addr);

	debug("fsl-ppa: PPA entry: 0x%llx\n", ppa_ram_addr);
	*entry = ppa_ram_addr;

	return 0;

out:
	printf("fsl-ppa: error (%d)\n", ret);
	*entry = 0;

	return ret;
}

int ppa_init_entry(void *ppa_entry)
{
	int ret;
	u32 *boot_loc_ptr_l, *boot_loc_ptr_h;

#ifdef CONFIG_FSL_LSCH3
	struct ccsr_gur __iomem *gur = (void *)(CONFIG_SYS_FSL_GUTS_ADDR);
	boot_loc_ptr_l = &gur->bootlocptrl;
	boot_loc_ptr_h = &gur->bootlocptrh;
#elif defined(CONFIG_FSL_LSCH2)
	struct ccsr_scfg __iomem *scfg = (void *)(CONFIG_SYS_FSL_SCFG_ADDR);
	boot_loc_ptr_l = &scfg->scratchrw[1];
	boot_loc_ptr_h = &scfg->scratchrw[0];
#endif

	debug("fsl-ppa: boot_loc_ptr_l = 0x%p, boot_loc_ptr_h =0x%p\n",
			boot_loc_ptr_l, boot_loc_ptr_h);
	ret = ppa_init(ppa_entry, boot_loc_ptr_l, boot_loc_ptr_h);
	if (ret < 0)
		return ret;

	debug("fsl-ppa: Return from PPA: current_el = %d\n", current_el());

	/*
	 * The PE will be turned into EL2 when run out of PPA.
	 * First, set vector for EL2.
	 */
	c_runtime_cpu_setup();

	/* Enable caches and MMU for EL2. */
	enable_caches();

	return 0;
}
