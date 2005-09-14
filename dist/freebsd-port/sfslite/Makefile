# New ports collection makefile for:   oneko
# Date created:        5 December 1994
# Whom:                asami
#
# $FreeBSD$
#

PORTNAME=	sfslite
PORTVERSION=	0.8pre
CATEGORIES=	devel
MASTER_SITES=	http://dist.okws.org/dist/

MAINTAINER=	ports@okws.org
COMMENT= 	Standard version of the sfslite library

GNU_CONFIGURE=	yes

CONFIGURE_ARGS= --with-mode=shopt

post-extract:
	/bin/sh ${WRKSRC}/contrib/mk-sfsuser-freebsd.sh

.include <bsd.port.mk>
