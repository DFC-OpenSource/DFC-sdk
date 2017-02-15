# Copyright (C) 2015 Freescale Semiconductor Inc.
#
# deploy pkgs for all fsl images

DEPLOY_PKGS = " \
    dpl-examples \
    management-complex \
    rcw \
    u-boot \
"

DEPLOY_PKGS_append_ls2085ardb = " ls2-phy"
DEPLOY_PKGS_append_ls2085ardb-be = " ls2-phy"
DEPLOY_PKGS_append_ls2080ardb = " ls2-phy"

DEPLOY_PKGS_remove_ls2085aqds-be = "u-boot"
DEPLOY_PKGS_remove_ls2085ardb-be = "u-boot"

EXTRA_IMAGEDEPENDS_append = " ${DEPLOY_PKGS}"
