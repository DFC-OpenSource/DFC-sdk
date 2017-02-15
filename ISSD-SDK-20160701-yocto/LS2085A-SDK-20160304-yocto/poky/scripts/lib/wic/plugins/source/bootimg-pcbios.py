# ex:ts=4:sw=4:sts=4:et
# -*- tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
#
# Copyright (c) 2014, Intel Corporation.
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# DESCRIPTION
# This implements the 'bootimg-pcbios' source plugin class for 'wic'
#
# AUTHORS
# Tom Zanussi <tom.zanussi (at] linux.intel.com>
#

import os
import shutil
import re
import tempfile

from wic import kickstart, msger
from wic.utils import misc, fs_related, errors, runner, cmdln
from wic.conf import configmgr
from wic.plugin import pluginmgr
import wic.imager.direct as direct
from wic.pluginbase import SourcePlugin
from wic.utils.oe.misc import *
from wic.imager.direct import DirectImageCreator

class BootimgPcbiosPlugin(SourcePlugin):
    name = 'bootimg-pcbios'

    @classmethod
    def do_install_disk(self, disk, disk_name, cr, workdir, oe_builddir,
                        bootimg_dir, kernel_dir, native_sysroot):
        """
        Called after all partitions have been prepared and assembled into a
        disk image.  In this case, we install the MBR.
        """
        mbrfile = "%s/syslinux/" % bootimg_dir
        if cr._ptable_format == 'msdos':
            mbrfile += "mbr.bin"

        if not os.path.exists(mbrfile):
            msger.error("Couldn't find %s.  If using the -e option, do you have the right MACHINE set in local.conf?  If not, is the bootimg_dir path correct?" % mbrfile)

        full_path = cr._full_path(workdir, disk_name, "direct")
        msger.debug("Installing MBR on disk %s as %s with size %s bytes" \
                    % (disk_name, full_path, disk['min_size']))

        rc = runner.show(['dd', 'if=%s' % mbrfile,
                          'of=%s' % full_path, 'conv=notrunc'])
        if rc != 0:
            raise ImageError("Unable to set MBR to %s" % full_path)

    @classmethod
    def do_configure_partition(self, part, source_params, cr, cr_workdir,
                               oe_builddir, bootimg_dir, kernel_dir,
                               native_sysroot):
        """
        Called before do_prepare_partition(), creates syslinux config
        """
        hdddir = "%s/hdd/boot" % cr_workdir
        rm_cmd = "rm -rf " + cr_workdir
        exec_cmd(rm_cmd)

        install_cmd = "install -d %s" % hdddir
        exec_cmd(install_cmd)

        splash = os.path.join(cr_workdir, "/hdd/boot/splash.jpg")
        if os.path.exists(splash):
            splashline = "menu background splash.jpg"
        else:
            splashline = ""

        (rootdev, root_part_uuid) = cr._get_boot_config()
        options = cr.ks.handler.bootloader.appendLine

        syslinux_conf = ""
        syslinux_conf += "PROMPT 0\n"
        timeout = kickstart.get_timeout(cr.ks)
        if not timeout:
            timeout = 0
        syslinux_conf += "TIMEOUT " + str(timeout) + "\n"
        syslinux_conf += "\n"
        syslinux_conf += "ALLOWOPTIONS 1\n"
        syslinux_conf += "SERIAL 0 115200\n"
        syslinux_conf += "\n"
        if splashline:
            syslinux_conf += "%s\n" % splashline
        syslinux_conf += "DEFAULT boot\n"
        syslinux_conf += "LABEL boot\n"

        kernel = "/vmlinuz"
        syslinux_conf += "KERNEL " + kernel + "\n"

        if cr._ptable_format == 'msdos':
            rootstr = rootdev
        else:
            raise ImageError("Unsupported partition table format found")

        syslinux_conf += "APPEND label=boot root=%s %s\n" % (rootstr, options)

        msger.debug("Writing syslinux config %s/hdd/boot/syslinux.cfg" \
                    % cr_workdir)
        cfg = open("%s/hdd/boot/syslinux.cfg" % cr_workdir, "w")
        cfg.write(syslinux_conf)
        cfg.close()

    @classmethod
    def do_prepare_partition(self, part, source_params, cr, cr_workdir,
                             oe_builddir, bootimg_dir, kernel_dir,
                             rootfs_dir, native_sysroot):
        """
        Called to do the actual content population for a partition i.e. it
        'prepares' the partition to be incorporated into the image.
        In this case, prepare content for legacy bios boot partition.
        """
        def _has_syslinux(dir):
            if dir:
                syslinux = "%s/syslinux" % dir
                if os.path.exists(syslinux):
                    return True
            return False

        if not _has_syslinux(bootimg_dir):
            bootimg_dir = get_bitbake_var("STAGING_DATADIR")
            if not bootimg_dir:
                msger.error("Couldn't find STAGING_DATADIR, exiting\n")
            if not _has_syslinux(bootimg_dir):
                msger.error("Please build syslinux first\n")
            # just so the result notes display it
            cr.set_bootimg_dir(bootimg_dir)

        staging_kernel_dir = kernel_dir

        hdddir = "%s/hdd/boot" % cr_workdir

        install_cmd = "install -m 0644 %s/bzImage %s/vmlinuz" \
            % (staging_kernel_dir, hdddir)
        exec_cmd(install_cmd)

        install_cmd = "install -m 444 %s/syslinux/ldlinux.sys %s/ldlinux.sys" \
            % (bootimg_dir, hdddir)
        exec_cmd(install_cmd)

        du_cmd = "du -bks %s" % hdddir
        out = exec_cmd(du_cmd)
        blocks = int(out.split()[0])

        extra_blocks = part.get_extra_block_count(blocks)

        if extra_blocks < BOOTDD_EXTRA_SPACE:
            extra_blocks = BOOTDD_EXTRA_SPACE

        blocks += extra_blocks

        msger.debug("Added %d extra blocks to %s to get to %d total blocks" % \
                    (extra_blocks, part.mountpoint, blocks))

        # Ensure total sectors is an integral number of sectors per
        # track or mcopy will complain. Sectors are 512 bytes, and we
        # generate images with 32 sectors per track. This calculation is
        # done in blocks, thus the mod by 16 instead of 32.
        blocks += (16 - (blocks % 16))

        # dosfs image, created by mkdosfs
        bootimg = "%s/boot.img" % cr_workdir

        dosfs_cmd = "mkdosfs -n boot -S 512 -C %s %d" % (bootimg, blocks)
        exec_native_cmd(dosfs_cmd, native_sysroot)

        mcopy_cmd = "mcopy -i %s -s %s/* ::/" % (bootimg, hdddir)
        exec_native_cmd(mcopy_cmd, native_sysroot)

        syslinux_cmd = "syslinux %s" % bootimg
        exec_native_cmd(syslinux_cmd, native_sysroot)

        chmod_cmd = "chmod 644 %s" % bootimg
        exec_cmd(chmod_cmd)

        du_cmd = "du -Lbks %s" % bootimg
        out = exec_cmd(du_cmd)
        bootimg_size = out.split()[0]

        part.set_size(bootimg_size)
        part.set_source_file(bootimg)


