PACKAGECONFIG[spice] = "--enable-spice,--disable-spice,spice,"

PACKAGECONFIG ?= "fdt spice virtfs attr cap-ng"
PACKAGECONFIG_class-native = "fdt"
PACKAGECONFIG_class-nativesdk = "fdt"
