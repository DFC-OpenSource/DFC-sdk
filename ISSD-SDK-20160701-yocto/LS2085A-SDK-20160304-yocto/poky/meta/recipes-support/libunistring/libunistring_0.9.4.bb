SUMMARY = "Library for manipulating C and Unicode strings"

DESCRIPTION = "Text files are nowadays usually encoded in Unicode, and may\
 consist of very different scripts from Latin letters to Chinese Hanzi\
 with many kinds of special characters accents, right-to-left writing\
 marks, hyphens, Roman numbers, and much more. But the POSIX platform\
 APIs for text do not contain adequate functions for dealing with\
 particular properties of many Unicode characters. In fact, the POSIX\
 APIs for text have several assumptions at their base which don't hold\
 for Unicode text.  This library provides functions for manipulating\
 Unicode strings and for manipulating C strings according to the Unicode\
 standard.  This package contains documentation."

HOMEPAGE = "http://www.gnu.org/software/libunistring/"
SECTION = "devel"
LICENSE = "GPLv3&LGPLv3"
LIC_FILES_CHKSUM = "file://COPYING;md5=d32239bcb673463ab874e80d47fae504 \
                    file://COPYING.LIB;md5=6a6a8e020838b23406c81b19c1d46df6" 

SRC_URI = "${GNU_MIRROR}/libunistring/libunistring-${PV}.tar.gz \
           file://iconv-m4-remove-the-test-to-convert-euc-jp.patch \
"

SRC_URI[md5sum] = "c24a6a3838d9ad4a41a62549312c4226"
SRC_URI[sha256sum] = "f5246d63286a42902dc096d6d44541fbe4204b6c02d6d5d28b457c9882ddd8a6"

inherit autotools texinfo
BBCLASSEXTEND = "native nativesdk"
