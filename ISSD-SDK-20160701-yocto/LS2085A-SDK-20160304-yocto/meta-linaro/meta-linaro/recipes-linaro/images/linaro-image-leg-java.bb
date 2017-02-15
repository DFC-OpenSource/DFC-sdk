require linaro-image-lamp.bb

DESCRIPTION = "A Lamp-based image for Linaro Enterprise Java validation."

IMAGE_INSTALL += " \
    alsa-conf \
    alsa-lib-dev \
    alsa-lib \
    alsa-oss \
    alsa-utils-alsaconf \
    alsa-utils-alsamixer \
    cups-dev \
    ganglia \
    git \
    htop \
    links \
    openjdk-8-doc \
    openjdk-8-jdk \
    openjdk-8-jre \
    openjdk-8-jtreg \
    openjdk-8-source \
    sed \
    tmux \
    vim \
    x11vnc \
    xauth \
    xserver-xorg-xvfb \
    zip \
    "
