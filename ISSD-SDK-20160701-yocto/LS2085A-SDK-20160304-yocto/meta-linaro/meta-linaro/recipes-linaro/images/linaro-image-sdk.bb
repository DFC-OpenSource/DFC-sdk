require linaro-image-common.inc

DESCRIPTION = "A small SDK based image for Linaro development work."

IMAGE_INSTALL += "${SDK_IMAGE_INSTALL}"

IMAGE_FEATURES += "\
	dev-pkgs \
	staticdev-pkgs \
	tools-debug \
	tools-sdk \
	"
