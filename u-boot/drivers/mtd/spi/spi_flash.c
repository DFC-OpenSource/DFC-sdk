/*
 * SPI Flash Core
 *
 * Copyright (C) 2015 Jagan Teki <jteki@openedev.com>
 * Copyright (C) 2013 Jagannadha Sutradharudu Teki, Xilinx Inc.
 * Copyright (C) 2010 Reinhard Meyer, EMK Elektronik
 * Copyright (C) 2008 Atmel Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <mapmem.h>
#include <spi.h>
#include <spi_flash.h>
#include <linux/log2.h>

#include "sf_internal.h"

DECLARE_GLOBAL_DATA_PTR;

static void spi_flash_addr(u32 addr, u8 *cmd)
{
	if (addr >= SPI_FLASH_16MB_BOUN) {
		/* cmd[0] is actual command */
		cmd[1] = addr >> 24;
		cmd[2] = addr >> 16;
		cmd[3] = addr >> 8;
		cmd[4] = addr >> 0;
		cmd[5] = SPI_FLASH_ADDR_MAGIC;
	} else {
		cmd[1] = addr >> 16;
		cmd[2] = addr >> 8;
		cmd[3] = addr >> 0;
	}
}

/* Read commands array */
static u8 spi_read_cmds_array[] = {
	CMD_READ_ARRAY_SLOW,
	CMD_READ_ARRAY_FAST,
	CMD_READ_DUAL_OUTPUT_FAST,
	CMD_READ_DUAL_IO_FAST,
	CMD_READ_QUAD_OUTPUT_FAST,
	CMD_READ_QUAD_IO_FAST,
};

static int read_sr(struct spi_flash *flash, u8 *rs)
{
	int ret;
	u8 cmd;

	cmd = CMD_READ_STATUS;
	ret = spi_flash_read_common(flash, &cmd, 1, rs, 1);
	if (ret < 0) {
		debug("SF: fail to read status register\n");
		return ret;
	}

	return 0;
}

static int read_fsr(struct spi_flash *flash, u8 *fsr)
{
	int ret;
	const u8 cmd = CMD_FLAG_STATUS;

	ret = spi_flash_read_common(flash, &cmd, 1, fsr, 1);
	if (ret < 0) {
		debug("SF: fail to read flag status register\n");
		return ret;
	}

	return 0;
}

static int write_sr(struct spi_flash *flash, u8 ws)
{
	u8 cmd;
	int ret;

	cmd = CMD_WRITE_STATUS;
	ret = spi_flash_write_common(flash, &cmd, 1, &ws, 1);
	if (ret < 0) {
		debug("SF: fail to write status register\n");
		return ret;
	}

	return 0;
}

#if defined(CONFIG_SPI_FLASH_SPANSION) || defined(CONFIG_SPI_FLASH_WINBOND)
static int read_cr(struct spi_flash *flash, u8 *rc)
{
	int ret;
	u8 cmd;

	cmd = CMD_READ_CONFIG;
	ret = spi_flash_read_common(flash, &cmd, 1, rc, 1);
	if (ret < 0) {
		debug("SF: fail to read config register\n");
		return ret;
	}

	return 0;
}

static int write_cr(struct spi_flash *flash, u8 wc)
{
	u8 data[2];
	u8 cmd;
	int ret;

	ret = read_sr(flash, &data[0]);
	if (ret < 0)
		return ret;

	cmd = CMD_WRITE_STATUS;
	data[1] = wc;
	ret = spi_flash_write_common(flash, &cmd, 1, &data, 2);
	if (ret) {
		debug("SF: fail to write config register\n");
		return ret;
	}

	return 0;
}
#endif

#ifdef CONFIG_SPI_FLASH_BAR
static int spi_flash_write_bar(struct spi_flash *flash, u32 offset)
{
	u8 cmd, bank_sel;
	int ret;

	bank_sel = offset / (SPI_FLASH_16MB_BOUN << flash->shift);
	if (bank_sel == flash->bank_curr)
		goto bar_end;

	cmd = flash->bank_write_cmd;
	ret = spi_flash_write_common(flash, &cmd, 1, &bank_sel, 1);
	if (ret < 0) {
		debug("SF: fail to write bank register\n");
		return ret;
	}

bar_end:
	flash->bank_curr = bank_sel;
	return flash->bank_curr;
}

static int spi_flash_read_bar(struct spi_flash *flash, u8 idcode0)
{
	u8 curr_bank = 0;
	int ret;

	if (flash->size <= SPI_FLASH_16MB_BOUN)
		goto bank_end;

	switch (idcode0) {
	case SPI_FLASH_CFI_MFR_SPANSION:
		flash->bank_read_cmd = CMD_BANKADDR_BRRD;
		flash->bank_write_cmd = CMD_BANKADDR_BRWR;
		break;
	default:
		flash->bank_read_cmd = CMD_EXTNADDR_RDEAR;
		flash->bank_write_cmd = CMD_EXTNADDR_WREAR;
	}

	ret = spi_flash_read_common(flash, &flash->bank_read_cmd, 1,
				    &curr_bank, 1);
	if (ret) {
		debug("SF: fail to read bank addr register\n");
		return ret;
	}

bank_end:
	flash->bank_curr = curr_bank;
	return 0;
}
#endif

#ifdef CONFIG_SF_DUAL_FLASH
static void spi_flash_dual(struct spi_flash *flash, u32 *addr)
{
	switch (flash->dual_flash) {
	case SF_DUAL_STACKED_FLASH:
		if (*addr >= (flash->size >> 1)) {
			*addr -= flash->size >> 1;
			flash->spi->flags |= SPI_XFER_U_PAGE;
		} else {
			flash->spi->flags &= ~SPI_XFER_U_PAGE;
		}
		break;
	case SF_DUAL_PARALLEL_FLASH:
		*addr >>= flash->shift;
		break;
	default:
		debug("SF: Unsupported dual_flash=%d\n", flash->dual_flash);
		break;
	}
}
#endif

static int spi_flash_sr_ready(struct spi_flash *flash)
{
	u8 sr;
	int ret;

	ret = read_sr(flash, &sr);
	if (ret < 0)
		return ret;

	return !(sr & STATUS_WIP);
}

static int spi_flash_fsr_ready(struct spi_flash *flash)
{
	u8 fsr;
	int ret;

	ret = read_fsr(flash, &fsr);
	if (ret < 0)
		return ret;

	return fsr & STATUS_PEC;
}

static int spi_flash_ready(struct spi_flash *flash)
{
	int sr, fsr;

	sr = spi_flash_sr_ready(flash);
	if (sr < 0)
		return sr;

	fsr = 1;
	if (flash->flags & SNOR_F_USE_FSR) {
		fsr = spi_flash_fsr_ready(flash);
		if (fsr < 0)
			return fsr;
	}

	return sr && fsr;
}

static int spi_flash_cmd_wait_ready(struct spi_flash *flash,
					unsigned long timeout)
{
	int timebase, ret;

	timebase = get_timer(0);

	while (get_timer(timebase) < timeout) {
		ret = spi_flash_ready(flash);
		if (ret < 0)
			return ret;
		if (ret)
			return 0;
	}

	printf("SF: Timeout!\n");

	return -ETIMEDOUT;
}

int spi_flash_write_common(struct spi_flash *flash, const u8 *cmd,
		size_t cmd_len, const void *buf, size_t buf_len)
{
	struct spi_slave *spi = flash->spi;
	unsigned long timeout = SPI_FLASH_PROG_TIMEOUT;
	int ret;

	if (buf == NULL)
		timeout = SPI_FLASH_PAGE_ERASE_TIMEOUT;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: unable to claim SPI bus\n");
		return ret;
	}

	ret = spi_flash_cmd_write_enable(flash);
	if (ret < 0) {
		debug("SF: enabling write failed\n");
		return ret;
	}

	ret = spi_flash_cmd_write(spi, cmd, cmd_len, buf, buf_len);
	if (ret < 0) {
		debug("SF: write cmd failed\n");
		return ret;
	}

	ret = spi_flash_cmd_wait_ready(flash, timeout);
	if (ret < 0) {
		debug("SF: write %s timed out\n",
		      timeout == SPI_FLASH_PROG_TIMEOUT ?
			"program" : "page erase");
		return ret;
	}

	spi_release_bus(spi);

	return ret;
}

int spi_flash_cmd_erase_ops(struct spi_flash *flash, u32 offset, size_t len)
{
	u32 erase_size, erase_addr;
	u8 *cmd, cmdsz;
	int ret = -1;

	erase_size = flash->erase_size;
	if (offset % erase_size || len % erase_size) {
		debug("SF: Erase offset/length not multiple of erase size\n");
		return -1;
	}

	if (flash->flash_is_locked) {
		if (flash->flash_is_locked(flash, offset, len) > 0) {
			printf("offset 0x%x is protected and cannot be erased\n",
			       offset);
			return -EINVAL;
		}
	}
	if (flash->size > SPI_FLASH_16MB_BOUN)
		cmdsz = SPI_FLASH_CMD_LEN_EXT + flash->dummy_byte;
	else
		cmdsz = SPI_FLASH_CMD_LEN;

	cmd = calloc(1, cmdsz);
	if (!cmd) {
		debug("SF: Failed to allocate cmd\n");
		return -ENOMEM;
	}
	memset(cmd, 0x0, cmdsz);
	cmd[0] = flash->erase_cmd;

	while (len) {
		erase_addr = offset;

#ifdef CONFIG_SF_DUAL_FLASH
		if (flash->dual_flash > SF_SINGLE_FLASH)
			spi_flash_dual(flash, &erase_addr);
#endif
#ifdef CONFIG_SPI_FLASH_BAR
		ret = spi_flash_write_bar(flash, erase_addr);
		if (ret < 0)
			return ret;
#endif
		spi_flash_addr(erase_addr, cmd);

		debug("SF: erase %2x %2x %2x %2x (%x)\n", cmd[0], cmd[1],
		      cmd[2], cmd[3], erase_addr);

		ret = spi_flash_write_common(flash, cmd, cmdsz, NULL, 0);
		if (ret < 0) {
			debug("SF: erase failed\n");
			break;
		}

		offset += erase_size;
		len -= erase_size;
	}

	free(cmd);
	return ret;
}

int spi_flash_cmd_write_ops(struct spi_flash *flash, u32 offset,
		size_t len, const void *buf)
{
	unsigned long byte_addr, page_size;
	u32 write_addr;
	size_t chunk_len, actual;
	u8 *cmd, cmdsz;
	int ret = -1;

	page_size = flash->page_size;

	if (flash->flash_is_locked) {
		if (flash->flash_is_locked(flash, offset, len) > 0) {
			printf("offset 0x%x is protected and cannot be written\n",
			       offset);
			return -EINVAL;
		}
	}
	if (flash->size > SPI_FLASH_16MB_BOUN)
		cmdsz = SPI_FLASH_CMD_LEN_EXT + flash->dummy_byte;
	else
		cmdsz = SPI_FLASH_CMD_LEN;
	cmd = calloc(1, cmdsz);
	if (!cmd) {
		debug("SF: Failed to allocate cmd\n");
		return -ENOMEM;
	}
	memset(cmd, 0x0, cmdsz);
	cmd[0] = flash->write_cmd;
	for (actual = 0; actual < len; actual += chunk_len) {
		write_addr = offset;

#ifdef CONFIG_SF_DUAL_FLASH
		if (flash->dual_flash > SF_SINGLE_FLASH)
			spi_flash_dual(flash, &write_addr);
#endif
#ifdef CONFIG_SPI_FLASH_BAR
		ret = spi_flash_write_bar(flash, write_addr);
		if (ret < 0)
			return ret;
#endif
		byte_addr = offset % page_size;
		chunk_len = min(len - actual, (size_t)(page_size - byte_addr));

		if (flash->spi->max_write_size)
			chunk_len = min(chunk_len,
					(size_t)flash->spi->max_write_size);

		spi_flash_addr(write_addr, cmd);

		debug("SF: 0x%p => cmd = { 0x%02x 0x%02x%02x%02x } chunk_len = %zu\n",
		      buf + actual, cmd[0], cmd[1], cmd[2], cmd[3], chunk_len);

		ret = spi_flash_write_common(flash, cmd, cmdsz,
					buf + actual, chunk_len);
		if (ret < 0) {
			debug("SF: write failed\n");
			break;
		}

		offset += chunk_len;
	}

	free(cmd);
	return ret;
}

int spi_flash_read_common(struct spi_flash *flash, const u8 *cmd,
		size_t cmd_len, void *data, size_t data_len)
{
	struct spi_slave *spi = flash->spi;
	int ret;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: unable to claim SPI bus\n");
		return ret;
	}

	ret = spi_flash_cmd_read(spi, cmd, cmd_len, data, data_len);
	if (ret < 0) {
		debug("SF: read cmd failed\n");
		return ret;
	}

	spi_release_bus(spi);

	return ret;
}

void __weak spi_flash_copy_mmap(void *data, void *offset, size_t len)
{
	memcpy(data, offset, len);
}

int spi_flash_cmd_read_ops(struct spi_flash *flash, u32 offset,
		size_t len, void *data)
{
	u8 *cmd, cmdsz;
	u32 remain_len, read_len, read_addr;
	int bank_sel = 0;
	int ret = -1;

	/* Handle memory-mapped SPI */
	if (flash->memory_map) {
		ret = spi_claim_bus(flash->spi);
		if (ret) {
			debug("SF: unable to claim SPI bus\n");
			return ret;
		}
		spi_xfer(flash->spi, 0, NULL, NULL, SPI_XFER_MMAP);
		spi_flash_copy_mmap(data, flash->memory_map + offset, len);
		spi_xfer(flash->spi, 0, NULL, NULL, SPI_XFER_MMAP_END);
		spi_release_bus(flash->spi);
		return 0;
	}

	if (flash->size > SPI_FLASH_16MB_BOUN)
		cmdsz = SPI_FLASH_CMD_LEN_EXT + flash->dummy_byte;
	else
		cmdsz = SPI_FLASH_CMD_LEN + flash->dummy_byte;

	cmd = calloc(1, cmdsz);
	if (!cmd) {
		debug("SF: Failed to allocate cmd\n");
		return -ENOMEM;
	}
	memset(cmd, 0x0, cmdsz);
	cmd[0] = flash->read_cmd;

	while (len) {
		read_addr = offset;
#ifdef CONFIG_SF_DUAL_FLASH
		if (flash->dual_flash > SF_SINGLE_FLASH)
			spi_flash_dual(flash, &read_addr);
#endif
#ifdef CONFIG_SPI_FLASH_BAR
		ret = spi_flash_write_bar(flash, read_addr);
		if (ret < 0)
			return ret;
		bank_sel = flash->bank_curr;
#endif
		remain_len = ((flash->size << flash->shift) *
				(bank_sel + 1)) - offset;

		if (len < remain_len)
			read_len = len;
		else
			read_len = remain_len;

		spi_flash_addr(read_addr, cmd);

		ret = spi_flash_read_common(flash, cmd, cmdsz, data, read_len);
		if (ret < 0) {
			debug("SF: read failed\n");
			break;
		}

		offset += read_len;
		len -= read_len;
		data += read_len;
	}

	free(cmd);
	return ret;
}

int spi_flash_cmd_4B_addr_switch(struct spi_flash *flash,
				int enable, u8 idcode0)
{
	int ret;
	u8 cmd, bar;
	bool need_wren = false;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: unable to claim SPI bus\n");
		return ret;
	}

	switch (idcode0) {
	case SPI_FLASH_CFI_MFR_STMICRO:
		/* Some Micron need WREN command; all will accept it */
		need_wren = true;
	case SPI_FLASH_CFI_MFR_MACRONIX:
	case SPI_FLASH_CFI_MFR_WINBOND:
		if (need_wren)
			spi_flash_cmd_write_enable(flash);

		cmd = enable ? CMD_ENTER_4B_ADDR : CMD_EXIT_4B_ADDR;
		ret = spi_flash_cmd(flash->spi, cmd, NULL, 0);
		if (need_wren)
			spi_flash_cmd_write_disable(flash);

		break;
	default:
		/* Spansion style */
		bar = enable << 7;
		cmd = CMD_BANKADDR_BRWR;
		ret = spi_flash_cmd_write(flash->spi, &cmd, 1, &bar, 1);
		break;
	}

	spi_release_bus(flash->spi);

	return ret;
}

#ifdef CONFIG_SPI_FLASH_SST
static int sst_byte_write(struct spi_flash *flash, u32 offset, const void *buf)
{
	int ret;
	u8 cmd[4] = {
		CMD_SST_BP,
		offset >> 16,
		offset >> 8,
		offset,
	};

	debug("BP[%02x]: 0x%p => cmd = { 0x%02x 0x%06x }\n",
	      spi_w8r8(flash->spi, CMD_READ_STATUS), buf, cmd[0], offset);

	ret = spi_flash_cmd_write_enable(flash);
	if (ret)
		return ret;

	ret = spi_flash_cmd_write(flash->spi, cmd, sizeof(cmd), buf, 1);
	if (ret)
		return ret;

	return spi_flash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);
}

int sst_write_wp(struct spi_flash *flash, u32 offset, size_t len,
		const void *buf)
{
	size_t actual, cmd_len;
	int ret;
	u8 cmd[4];

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: Unable to claim SPI bus\n");
		return ret;
	}

	/* If the data is not word aligned, write out leading single byte */
	actual = offset % 2;
	if (actual) {
		ret = sst_byte_write(flash, offset, buf);
		if (ret)
			goto done;
	}
	offset += actual;

	ret = spi_flash_cmd_write_enable(flash);
	if (ret)
		goto done;

	cmd_len = 4;
	cmd[0] = CMD_SST_AAI_WP;
	cmd[1] = offset >> 16;
	cmd[2] = offset >> 8;
	cmd[3] = offset;

	for (; actual < len - 1; actual += 2) {
		debug("WP[%02x]: 0x%p => cmd = { 0x%02x 0x%06x }\n",
		      spi_w8r8(flash->spi, CMD_READ_STATUS), buf + actual,
		      cmd[0], offset);

		ret = spi_flash_cmd_write(flash->spi, cmd, cmd_len,
					buf + actual, 2);
		if (ret) {
			debug("SF: sst word program failed\n");
			break;
		}

		ret = spi_flash_cmd_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);
		if (ret)
			break;

		cmd_len = 1;
		offset += 2;
	}

	if (!ret)
		ret = spi_flash_cmd_write_disable(flash);

	/* If there is a single trailing byte, write it out */
	if (!ret && actual != len)
		ret = sst_byte_write(flash, offset, buf + actual);

 done:
	debug("SF: sst: program %s %zu bytes @ 0x%zx\n",
	      ret ? "failure" : "success", len, offset - actual);

	spi_release_bus(flash->spi);
	return ret;
}

int sst_write_bp(struct spi_flash *flash, u32 offset, size_t len,
		const void *buf)
{
	size_t actual;
	int ret;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: Unable to claim SPI bus\n");
		return ret;
	}

	for (actual = 0; actual < len; actual++) {
		ret = sst_byte_write(flash, offset, buf + actual);
		if (ret) {
			debug("SF: sst byte program failed\n");
			break;
		}
		offset++;
	}

	if (!ret)
		ret = spi_flash_cmd_write_disable(flash);

	debug("SF: sst: program %s %zu bytes @ 0x%zx\n",
	      ret ? "failure" : "success", len, offset - actual);

	spi_release_bus(flash->spi);
	return ret;
}
#endif

#if defined(CONFIG_SPI_FLASH_STMICRO) || defined(CONFIG_SPI_FLASH_SST)
static void stm_get_locked_range(struct spi_flash *flash, u8 sr, loff_t *ofs,
				 u32 *len)
{
	u8 mask = SR_BP2 | SR_BP1 | SR_BP0;
	int shift = ffs(mask) - 1;
	int pow;

	if (!(sr & mask)) {
		/* No protection */
		*ofs = 0;
		*len = 0;
	} else {
		pow = ((sr & mask) ^ mask) >> shift;
		*len = flash->size >> pow;
		*ofs = flash->size - *len;
	}
}

/*
 * Return 1 if the entire region is locked, 0 otherwise
 */
static int stm_is_locked_sr(struct spi_flash *flash, u32 ofs, u32 len,
			    u8 sr)
{
	loff_t lock_offs;
	u32 lock_len;

	stm_get_locked_range(flash, sr, &lock_offs, &lock_len);

	return (ofs + len <= lock_offs + lock_len) && (ofs >= lock_offs);
}

/*
 * Check if a region of the flash is (completely) locked. See stm_lock() for
 * more info.
 *
 * Returns 1 if entire region is locked, 0 if any portion is unlocked, and
 * negative on errors.
 */
int stm_is_locked(struct spi_flash *flash, u32 ofs, size_t len)
{
	int status;
	u8 sr;

	status = read_sr(flash, &sr);
	if (status < 0)
		return status;

	return stm_is_locked_sr(flash, ofs, len, sr);
}

/*
 * Lock a region of the flash. Compatible with ST Micro and similar flash.
 * Supports only the block protection bits BP{0,1,2} in the status register
 * (SR). Does not support these features found in newer SR bitfields:
 *   - TB: top/bottom protect - only handle TB=0 (top protect)
 *   - SEC: sector/block protect - only handle SEC=0 (block protect)
 *   - CMP: complement protect - only support CMP=0 (range is not complemented)
 *
 * Sample table portion for 8MB flash (Winbond w25q64fw):
 *
 *   SEC  |  TB   |  BP2  |  BP1  |  BP0  |  Prot Length  | Protected Portion
 *  --------------------------------------------------------------------------
 *    X   |   X   |   0   |   0   |   0   |  NONE         | NONE
 *    0   |   0   |   0   |   0   |   1   |  128 KB       | Upper 1/64
 *    0   |   0   |   0   |   1   |   0   |  256 KB       | Upper 1/32
 *    0   |   0   |   0   |   1   |   1   |  512 KB       | Upper 1/16
 *    0   |   0   |   1   |   0   |   0   |  1 MB         | Upper 1/8
 *    0   |   0   |   1   |   0   |   1   |  2 MB         | Upper 1/4
 *    0   |   0   |   1   |   1   |   0   |  4 MB         | Upper 1/2
 *    X   |   X   |   1   |   1   |   1   |  8 MB         | ALL
 *
 * Returns negative on errors, 0 on success.
 */
int stm_lock(struct spi_flash *flash, u32 ofs, size_t len)
{
	u8 status_old, status_new;
	u8 mask = SR_BP2 | SR_BP1 | SR_BP0;
	u8 shift = ffs(mask) - 1, pow, val;
	int ret;

	ret = read_sr(flash, &status_old);
	if (ret < 0)
		return ret;

	/* SPI NOR always locks to the end */
	if (ofs + len != flash->size) {
		/* Does combined region extend to end? */
		if (!stm_is_locked_sr(flash, ofs + len, flash->size - ofs - len,
				      status_old))
			return -EINVAL;
		len = flash->size - ofs;
	}

	/*
	 * Need smallest pow such that:
	 *
	 *   1 / (2^pow) <= (len / size)
	 *
	 * so (assuming power-of-2 size) we do:
	 *
	 *   pow = ceil(log2(size / len)) = log2(size) - floor(log2(len))
	 */
	pow = ilog2(flash->size) - ilog2(len);
	val = mask - (pow << shift);
	if (val & ~mask)
		return -EINVAL;

	/* Don't "lock" with no region! */
	if (!(val & mask))
		return -EINVAL;

	status_new = (status_old & ~mask) | val;

	/* Only modify protection if it will not unlock other areas */
	if ((status_new & mask) <= (status_old & mask))
		return -EINVAL;

	write_sr(flash, status_new);

	return 0;
}

/*
 * Unlock a region of the flash. See stm_lock() for more info
 *
 * Returns negative on errors, 0 on success.
 */
int stm_unlock(struct spi_flash *flash, u32 ofs, size_t len)
{
	uint8_t status_old, status_new;
	u8 mask = SR_BP2 | SR_BP1 | SR_BP0;
	u8 shift = ffs(mask) - 1, pow, val;
	int ret;

	ret = read_sr(flash, &status_old);
	if (ret < 0)
		return ret;

	/* Cannot unlock; would unlock larger region than requested */
	if (stm_is_locked_sr(flash, ofs - flash->erase_size, flash->erase_size,
			     status_old))
		return -EINVAL;
	/*
	 * Need largest pow such that:
	 *
	 *   1 / (2^pow) >= (len / size)
	 *
	 * so (assuming power-of-2 size) we do:
	 *
	 *   pow = floor(log2(size / len)) = log2(size) - ceil(log2(len))
	 */
	pow = ilog2(flash->size) - order_base_2(flash->size - (ofs + len));
	if (ofs + len == flash->size) {
		val = 0; /* fully unlocked */
	} else {
		val = mask - (pow << shift);
		/* Some power-of-two sizes are not supported */
		if (val & ~mask)
			return -EINVAL;
	}

	status_new = (status_old & ~mask) | val;

	/* Only modify protection if it will not lock other areas */
	if ((status_new & mask) >= (status_old & mask))
		return -EINVAL;

	write_sr(flash, status_new);

	return 0;
}
#endif


#ifdef CONFIG_SPI_FLASH_MACRONIX
static int spi_flash_set_qeb_mxic(struct spi_flash *flash)
{
	u8 qeb_status;
	int ret;

	ret = read_sr(flash, &qeb_status);
	if (ret < 0)
		return ret;

	if (qeb_status & STATUS_QEB_MXIC) {
		debug("SF: mxic: QEB is already set\n");
	} else {
		ret = write_sr(flash, STATUS_QEB_MXIC);
		if (ret < 0)
			return ret;
	}

	return ret;
}
#endif

#if defined(CONFIG_SPI_FLASH_SPANSION) || defined(CONFIG_SPI_FLASH_WINBOND)
static int spi_flash_set_qeb_winspan(struct spi_flash *flash)
{
	u8 qeb_status;
	int ret;

	ret = read_cr(flash, &qeb_status);
	if (ret < 0)
		return ret;

	if (qeb_status & STATUS_QEB_WINSPAN) {
		debug("SF: winspan: QEB is already set\n");
	} else {
		ret = write_cr(flash, STATUS_QEB_WINSPAN);
		if (ret < 0)
			return ret;
	}

	return ret;
}
#endif

static int spi_flash_set_qeb(struct spi_flash *flash, u8 idcode0)
{
	switch (idcode0) {
#ifdef CONFIG_SPI_FLASH_MACRONIX
	case SPI_FLASH_CFI_MFR_MACRONIX:
		return spi_flash_set_qeb_mxic(flash);
#endif
#if defined(CONFIG_SPI_FLASH_SPANSION) || defined(CONFIG_SPI_FLASH_WINBOND)
	case SPI_FLASH_CFI_MFR_SPANSION:
	case SPI_FLASH_CFI_MFR_WINBOND:
		return spi_flash_set_qeb_winspan(flash);
#endif
#ifdef CONFIG_SPI_FLASH_STMICRO
	case SPI_FLASH_CFI_MFR_STMICRO:
		debug("SF: QEB is volatile for %02x flash\n", idcode0);
		return 0;
#endif
	default:
		printf("SF: Need set QEB func for %02x flash\n", idcode0);
		return -1;
	}
}

#if CONFIG_IS_ENABLED(OF_CONTROL)
int spi_flash_decode_fdt(const void *blob, struct spi_flash *flash)
{
	fdt_addr_t addr;
	fdt_size_t size;
	int node;

	/* If there is no node, do nothing */
	node = fdtdec_next_compatible(blob, 0, COMPAT_GENERIC_SPI_FLASH);
	if (node < 0)
		return 0;

	addr = fdtdec_get_addr_size(blob, node, "memory-map", &size);
	if (addr == FDT_ADDR_T_NONE) {
		debug("%s: Cannot decode address\n", __func__);
		return 0;
	}

	if (flash->size != size) {
		debug("%s: Memory map must cover entire device\n", __func__);
		return -1;
	}
	flash->memory_map = map_sysmem(addr, size);

	return 0;
}
#endif /* CONFIG_IS_ENABLED(OF_CONTROL) */

#ifdef CONFIG_SPI_FLASH_SPANSION
static int spansion_s25fss_disable_4KB_erase(struct spi_slave *spi)
{
	u8 cmd[4];
	u32 offset = 0x000004; /* CR3NV register offset */
	u8 cr3v;
	int ret;

	cmd[0] = CMD_SPANSION_RDAR;
	cmd[1] = offset >> 16;
	cmd[2] = offset >> 8;
	cmd[3] = offset >> 0;

	ret = spi_flash_cmd_read(spi, cmd, 4, &cr3v, 1);
	if (ret)
		return -EIO;
	/* CR3V bit3: 4-KB Erase */
	if (cr3v & 0x8)
		return 0;

	cmd[0] = CMD_SPANSION_WRAR;
	cr3v |= 0x8;
	ret = spi_flash_cmd_write(spi, cmd, 4, &cr3v, 1);
	if (ret)
		return -EIO;

	cmd[0] = CMD_SPANSION_RDAR;
	ret = spi_flash_cmd_read(spi, cmd, 4, &cr3v, 1);
	if (ret)
		return -EIO;
	if (!(cr3v & 0x8))
		return -EFAULT;

	return 0;
}
#endif

int spi_flash_scan(struct spi_flash *flash)
{
	struct spi_slave *spi = flash->spi;
	const struct spi_flash_params *params;
	u16 jedec, ext_jedec;
	u8 idcode[5];
	u8 cmd;
	int ret;
#ifdef CONFIG_SPI_FLASH_SPANSION
	u8 id[6];
#endif

	/* Read the ID codes */
	ret = spi_flash_cmd(spi, CMD_READ_ID, idcode, sizeof(idcode));
	if (ret) {
		printf("SF: Failed to get idcodes\n");
		return -EINVAL;
	}

#ifdef DEBUG
	printf("SF: Got idcodes\n");
	print_buffer(0, idcode, 1, sizeof(idcode), 0);
#endif

	jedec = idcode[1] << 8 | idcode[2];
	ext_jedec = idcode[3] << 8 | idcode[4];

	/* Validate params from spi_flash_params table */
	params = spi_flash_params_table;
	for (; params->name != NULL; params++) {
		if ((params->jedec >> 16) == idcode[0]) {
			if ((params->jedec & 0xFFFF) == jedec) {
				if (params->ext_jedec == 0)
					break;
				else if (params->ext_jedec == ext_jedec)
					break;
			}
		}
	}

	if (!params->name) {
		printf("SF: Unsupported flash IDs: ");
		printf("manuf %02x, jedec %04x, ext_jedec %04x\n",
		       idcode[0], jedec, ext_jedec);
		return -EPROTONOSUPPORT;
	}

#ifdef CONFIG_SPI_FLASH_SPANSION
	/*
	 * The S25FS-S family physical sectors may be configured as a
	 * hybrid combination of eight 4-kB parameter sectors
	 * at the top or bottom of the address space with all
	 * but one of the remaining sectors being uniform size.
	 * The Parameter Sector Erase commands (20h or 21h) must
	 * be used to erase the 4-kB parameter sectors individually.
	 * The Sector (uniform sector) Erase commands (D8h or DCh)
	 * must be used to erase any of the remaining
	 * sectors, including the portion of highest or lowest address
	 * sector that is not overlaid by the parameter sectors.
	 * The uniform sector erase command has no effect on parameter sectors.
	 */
	if ((jedec == 0x0219 || (jedec == 0x0220)) &&
	    (ext_jedec & 0xff00) == 0x4d00) {
		int ret;

		/* Read the ID codes again, 6 bytes */
		ret = spi_flash_cmd(flash->spi, CMD_READ_ID, id, sizeof(id));
		if (ret)
			return -EIO;

		ret = memcmp(id, idcode, 5);
		if (ret)
			return -EIO;

		/* 0x81: S25FS-S family 0x80: S25FL-S family */
		if (id[5] == 0x81) {
			ret = spansion_s25fss_disable_4KB_erase(spi);
			if (ret)
				return ret;
		}
	}
#endif
	/* Flash powers up read-only, so clear BP# bits */
	if (idcode[0] == SPI_FLASH_CFI_MFR_ATMEL ||
	    idcode[0] == SPI_FLASH_CFI_MFR_MACRONIX ||
	    idcode[0] == SPI_FLASH_CFI_MFR_SST)
		write_sr(flash, 0);

	/* Assign spi data */
	flash->name = params->name;
	flash->memory_map = spi->memory_map;
	flash->dual_flash = flash->spi->option;

	/* Assign spi flash flags */
	if (params->flags & SST_WR)
		flash->flags |= SNOR_F_SST_WR;

	/* Assign spi_flash ops */
#ifndef CONFIG_DM_SPI_FLASH
	flash->write = spi_flash_cmd_write_ops;
#if defined(CONFIG_SPI_FLASH_SST)
	if (flash->flags & SNOR_F_SST_WR) {
		if (flash->spi->op_mode_tx & SPI_OPM_TX_BP)
			flash->write = sst_write_bp;
		else
			flash->write = sst_write_wp;
	}
#endif
	flash->erase = spi_flash_cmd_erase_ops;
	flash->read = spi_flash_cmd_read_ops;
#endif

	/* lock hooks are flash specific - assign them based on idcode0 */
	switch (idcode[0]) {
#if defined(CONFIG_SPI_FLASH_STMICRO) || defined(CONFIG_SPI_FLASH_SST)
	case SPI_FLASH_CFI_MFR_STMICRO:
	case SPI_FLASH_CFI_MFR_SST:
		flash->flash_lock = stm_lock;
		flash->flash_unlock = stm_unlock;
		flash->flash_is_locked = stm_is_locked;
#endif
		break;
	default:
		debug("SF: Lock ops not supported for %02x flash\n", idcode[0]);
	}

	/* Compute the flash size */
	flash->shift = (flash->dual_flash & SF_DUAL_PARALLEL_FLASH) ? 1 : 0;
	/*
	 * The Spansion S25FL032P and S25FL064P have 256b pages, yet use the
	 * 0x4d00 Extended JEDEC code. The rest of the Spansion flashes with
	 * the 0x4d00 Extended JEDEC code have 512b pages. All of the others
	 * have 256b pages.
	 */
	if (ext_jedec == 0x4d00) {
		if ((jedec == 0x0215) || (jedec == 0x216))
			flash->page_size = 256;
		else
			flash->page_size = 512;
	} else {
		flash->page_size = 256;
	}
	flash->page_size <<= flash->shift;
	flash->sector_size = params->sector_size << flash->shift;
	flash->size = flash->sector_size * params->nr_sectors << flash->shift;
#ifdef CONFIG_SF_DUAL_FLASH
	if (flash->dual_flash & SF_DUAL_STACKED_FLASH)
		flash->size <<= 1;
#endif

	/*
	 * So far, the 4-byte address mode haven't been supported in U-Boot,
	 * and make sure the chip (> 16MiB) in default 3-byte address mode,
	 * in case of warm bootup, the chip was set to 4-byte mode in kernel.
	 */
	if (flash->size >> (flash->dual_flash & SF_DUAL_STACKED_FLASH ? 1 : 0)
		> SPI_FLASH_16MB_BOUN) {
		if (spi_flash_cmd_4B_addr_switch(flash, false, idcode[0]) < 0)
			debug("SF: enter 3B address mode failed\n");
	}

	/* Compute erase sector and command */
	if (params->flags & SECT_4K) {
		flash->erase_cmd = CMD_ERASE_4K;
		flash->erase_size = 4096 << flash->shift;
	} else if (params->flags & SECT_32K) {
		flash->erase_cmd = CMD_ERASE_32K;
		flash->erase_size = 32768 << flash->shift;
	} else {
		flash->erase_cmd = CMD_ERASE_64K;
		flash->erase_size = flash->sector_size;
	}

	/* Now erase size becomes valid sector size */
	flash->sector_size = flash->erase_size;

	/* Look for the fastest read cmd */
	cmd = fls(params->e_rd_cmd & flash->spi->op_mode_rx);
	if (cmd) {
		cmd = spi_read_cmds_array[cmd - 1];
		flash->read_cmd = cmd;
	} else {
		/* Go for default supported read cmd */
		flash->read_cmd = CMD_READ_ARRAY_FAST;
	}

	/* Not require to look for fastest only two write cmds yet */
	if (params->flags & WR_QPP && flash->spi->op_mode_tx & SPI_OPM_TX_QPP)
		flash->write_cmd = CMD_QUAD_PAGE_PROGRAM;
	else
		/* Go for default supported write cmd */
		flash->write_cmd = CMD_PAGE_PROGRAM;

	/* Set the quad enable bit - only for quad commands */
	if ((flash->read_cmd == CMD_READ_QUAD_OUTPUT_FAST) ||
	    (flash->read_cmd == CMD_READ_QUAD_IO_FAST) ||
	    (flash->write_cmd == CMD_QUAD_PAGE_PROGRAM)) {
		ret = spi_flash_set_qeb(flash, idcode[0]);
		if (ret) {
			debug("SF: Fail to set QEB for %02x\n", idcode[0]);
			return -EINVAL;
		}
	}

	/* Read dummy_byte: dummy byte is determined based on the
	 * dummy cycles of a particular command.
	 * Fast commands - dummy_byte = dummy_cycles/8
	 * I/O commands- dummy_byte = (dummy_cycles * no.of lines)/8
	 * For I/O commands except cmd[0] everything goes on no.of lines
	 * based on particular command but incase of fast commands except
	 * data all go on single line irrespective of command.
	 */
	switch (flash->read_cmd) {
	case CMD_READ_QUAD_IO_FAST:
		flash->dummy_byte = 2;
		break;
	case CMD_READ_ARRAY_SLOW:
		flash->dummy_byte = 0;
		break;
	default:
		flash->dummy_byte = 1;
	}

#ifdef CONFIG_SPI_FLASH_STMICRO
	if (params->flags & E_FSR)
		flash->flags |= SNOR_F_USE_FSR;
#endif

	/* Configure the BAR - discover bank cmds and read current bank */
#ifdef CONFIG_SPI_FLASH_BAR
	ret = spi_flash_read_bar(flash, idcode[0]);
	if (ret < 0)
		return ret;
#endif

#if CONFIG_IS_ENABLED(OF_CONTROL)
	ret = spi_flash_decode_fdt(gd->fdt_blob, flash);
	if (ret) {
		debug("SF: FDT decode error\n");
		return -EINVAL;
	}
#endif

#ifndef CONFIG_SPL_BUILD
	printf("SF: Detected %s with page size ", flash->name);
	print_size(flash->page_size, ", erase size ");
	print_size(flash->erase_size, ", total ");
	print_size(flash->size, "");
	if (flash->memory_map)
		printf(", mapped at %p", flash->memory_map);
	puts("\n");
#endif

#ifdef CONFIG_SPI_FLASH_SPANSION
#ifndef CONFIG_SPI_FLASH_BAR
	if ((id[5] != 0x81) &&
	    /*
	     * Spansion FS-S family not support BAR ,
	     * Even if CONFIG_SPI_FLASH_BAR is unable,
	     * Need not the Warning prints */
	    ((((flash->dual_flash == SF_SINGLE_FLASH) &&
	    (flash->size > SPI_FLASH_16MB_BOUN))) ||
	    ((flash->dual_flash > SF_SINGLE_FLASH) &&
	    (flash->size > SPI_FLASH_16MB_BOUN << 1)))) {
		puts("SF: Warning - Only lower 16MiB accessible,");
		puts(" Full access #define CONFIG_SPI_FLASH_BAR\n");
	}
#endif
#endif

	return ret;
}
