PACKAGECONFIG_append = " sctp"
PACKAGECONFIG[sctp] = "--enable-sctp ac_cv_header_netinet_sctp_h=yes,--disable-sctp,lksctp-tools"
