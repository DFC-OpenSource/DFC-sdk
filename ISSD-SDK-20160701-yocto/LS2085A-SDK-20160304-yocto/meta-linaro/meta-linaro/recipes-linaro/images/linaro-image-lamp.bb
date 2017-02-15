require linaro-image-common.inc

IMAGE_INSTALL += " \
    apache2 \
    grub \
    mysql5-server \
    mysql5-client \
    php-fpm \
    php-fpm-apache2 \
    packagegroup-core-buildessential \
    wget \
    ${SDK_IMAGE_INSTALL}"

IMAGE_FEATURES += "\
	dev-pkgs \
	staticdev-pkgs \
	tools-debug \
	tools-sdk \
	"
