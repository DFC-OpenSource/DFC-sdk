require linaro-image-common.inc

DESCRIPTION = "A small image for Linaro KVM validation."

IMAGE_INSTALL += "qemu kernel-image kernel-bootwrapper"

