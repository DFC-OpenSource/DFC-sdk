require conf/machine/include/soc-family.inc
require conf/machine/include/arm/arch-arm64.inc

# Freescale BSP default providers
PREFERRED_PROVIDER_u-boot ?= "u-boot-ls2"
PREFERRED_PROVIDER_virtual/kernel ?= "linux-ls2-sdk"
PREFERRED_PROVIDER_libevent ?= "libevent"

# Define the default image type
IMAGE_CLASSES ?= "image_types_uboot"
KERNEL_IMAGETYPE ?= "Image"

MACHINE_FEATURES = "pci ext2 ext3 serial"
MACHINE_EXTRA_RRECOMMENDS = "udev-extraconf"

EXTRA_IMAGEDEPENDS += "cst-native"
