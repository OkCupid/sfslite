# New ports collection makefile for:  sfslite
# Date created:        5 December 1994
# Whom:                max
#
# $FreeBSD$
#

PORTNAME=	sfslite
PORTVERSION=	0.8.1
CATEGORIES=	devel
MASTER_SITES=	http://dist.okws.org/dist/

MAINTAINER=	ports@okws.org
COMMENT= 	Standard version of the sfslite library

GNU_CONFIGURE=	yes

#
# Map X.Y.Z.Q => X.Y
#
SHORTVERSION!=	${ECHO} ${PORTVERSION} | sed -Ee 's/([^.]+\.[^.]+).*/\1/'

#
# if being called from one of the slave distributions (like
# sfslite-dbg), then the build-mode will be set, and we should
# rename the port accordingly so that more than one of them can
# be built.
#
.if defined(BUILD_MODE)
PKGNAMESUFFIX+= "-${BUILD_MODE}"
RPCC_BIN?=      lib/${PORTNAME}-${SHORTVERSION}/${BUILD_MODE}
.else
BUILD_MODE?=    shopt
RPCC_BIN?=      bin
.endif # define(BUILD_MODE)

#
# rpcc does not work with byacc, so just require BISON
#
USE_BISON=	yes

LIB_DEPENDS+=   gmp:${PORTSDIR}/math/libgmp4


#
# Do not add LDCONFIG dirs, since we want SFS to link to its own
# libraries, and not our (lite) ones...
#
#INSTALLS_SHLIB= yes
#LDCONFIG_DIRS=  %%PREFIX%%/lib/${PORTNAME}/${BUILD_MODE}
#

CONFIGURE_TARGET=--build=${MACHINE_ARCH}-portbld-freebsd${OSREL}
CONFIGURE_ARGS=  --with-mode=${BUILD_MODE}

PLIST_SUB+=      VERSION=${SHORTVERSION}
PLIST_SUB+=      TAG=${BUILD_MODE}
PLIST_SUB+=      RPCC_BIN=${RPCC_BIN}

#
# need to run this pre-configure
#
post-extract:
	${SH} ${WRKSRC}/contrib/mk-sfsuser-freebsd.sh

.include <bsd.port.mk>
