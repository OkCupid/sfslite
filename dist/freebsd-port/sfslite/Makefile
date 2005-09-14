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
# This flag might be set by slave configuration directories that
# call us up.
#
BUILD_MODE?= shopt

INSTALLS_SHLIB= yes
LDCONFIG_DIRS=  %%PREFIX%%/lib/${PORTNAME}/${BUILD_MODE}

CONFIGURE_TARGET=--build=${MACHINE_ARCH}-portbld-freebsd${OSREL}
CONFIGURE_ARGS=  --with-mode=${BUILD_MODE}

#
# need to run this pre-configure
#
post-extract:
	sh ${WRKSRC}/contrib/mk-sfsuser-freebsd.sh

.include <bsd.port.mk>
