
#
# Compile without optimization and with dmalloc memory debugging
#
BUILD_MODE=      shdbg
MASTERDIR=       ${.CURDIR}/../sfslite

LIB_DEPENDS+=	 dmalloc:${PORTSDIR}/devel/dmalloc
PKG_INSTALL=     ${MASTERDIR}/pkg-install

.include "${MASTERDIR}/Makefile"
