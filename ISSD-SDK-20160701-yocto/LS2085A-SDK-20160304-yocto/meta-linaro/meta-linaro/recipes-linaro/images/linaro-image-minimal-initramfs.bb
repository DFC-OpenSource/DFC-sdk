SUMMARY = "Initramfs image for kernel boot testing"
DESCRIPTION = "Small image capable of booting a device."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

BAD_RECOMMENDATIONS += "busybox-syslog"

export IMAGE_BASENAME = "linaro-image-minimal-initramfs"

IMAGE_FSTYPES = "${INITRAMFS_FSTYPES} ${INITRAMFS_FSTYPES}.u-boot"

# Do not pollute the initrd image with rootfs features
IMAGE_FEATURES = ""

# List of packages to install
IMAGE_INSTALL = "\
  base-passwd \
  bash \
  busybox \
  bzip2 \
  dhcp-client \
  dosfstools \
  e2fsprogs \
  e2fsprogs-mke2fs \
  gzip \
  initramfs-boot-linaro \
  net-tools \
  parted \
  tar \
  u-boot-mkimage \
  wget \
  "

# Keep extra language files from being installed
IMAGE_LINGUAS = ""

IMAGE_ROOTFS_SIZE = "8192"

inherit core-image image_types_uboot
