DESCRIPTION = "Linaro image with basic X11 and firefox"

inherit core-image

IMAGE_FEATURES += "splash package-management x11-base x11-sato ssh-server-openssh"

LICENSE = "MIT"

IMAGE_INSTALL += "firefox gator openssh-sftp-server"
