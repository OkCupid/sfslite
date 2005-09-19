
#
# Compile without optimization and with dmalloc memory debugging
#
BUILD_MODE=      shdbg
MASTERDIR=       ${.CURDIR}/../sfslite
COMMENT=	"sfslite with dmalloc and without optimization"

LIB_DEPENDS+=	 dmalloc:${PORTSDIR}/devel/dmalloc
PKG_INSTALL=     ${MASTERDIR}/pkg-install

.include "${MASTERDIR}/Makefile"
