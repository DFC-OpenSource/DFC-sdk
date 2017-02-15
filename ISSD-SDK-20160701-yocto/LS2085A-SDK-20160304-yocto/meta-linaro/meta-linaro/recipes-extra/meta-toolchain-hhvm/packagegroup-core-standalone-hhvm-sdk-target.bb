SUMMARY = "HipHop VM porting SDK (target tools)"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS_${PN} = "\
    packagegroup-core-standalone-sdk-target \
    binutils-dev \
    binutils-staticdev \
    bison \
    boost-dev \
    boost-staticdev \
    boost \
    bzip2-dev \
    cmake \
    curl-dev \
    elfutils-dev \
    elfutils-staticdev \
    expat \
    flex \
    gd-dev \
    gd-staticdev \
    glog-dev \
    icu-dev \
    libcap-dev \
    libcap-staticdev \
    uw-imap-dev \
    uw-imap-staticdev \
    libdwarf-dev \
    libdwarf-staticdev \
    libevent-fb-dev \
    libevent-fb-staticdev \
    libglade-dev \
    libmcrypt-dev \
    libmemcached-dev \
    libmemcached-staticdev \
    libmysqlclient-dev \
    libmysqlclient-staticdev \
    libmysqlclient-r-dev \
    libmysqlclient-r-staticdev \
    libpam-dev \
    libpcre-dev \
    libpcre-staticdev \
    libunwind-dev \
    libunwind-staticdev \
    libxml2-dev \
    libxml2-staticdev \
    ncurses-dev \
    ncurses-staticdev \
    onig-dev \
    openldap-dev \
    openssl-dev \
    openssl-staticdev \
    readline-dev \
    tbb-dev \
    zlib-dev \
    "
