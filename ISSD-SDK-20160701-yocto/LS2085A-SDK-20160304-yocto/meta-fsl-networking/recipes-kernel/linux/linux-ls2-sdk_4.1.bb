require recipes-kernel/linux/linux-ls2-sdk.inc

#SRC_URI = "git://sw-stash.freescale.net/scm/dnnpi/ls2-linux.git;branch=linux-v4.1;protocol=http"
#SRCREV = "b282f9a00b101fcae3bcb2831e076a84492b005b"

SRC_URI = "git://192.168.100.20:8080/fslu_nvme_kernel_ear6.git;protocol=http;nobranch=1 \
            file://roce_v2.patch"
SRCREV = "67e9b491401691cb1498dd5d191129d2a5db36c9"
#SRCREV = "${AUTOREV}"
