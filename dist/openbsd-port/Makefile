# $OpenBSD$

#
# To update this port:
#   - Set snapshot in DISTNAME and release in WRKSRC
#   - rm pkg/PLIST pkg/PFRAG.shared files/md5
#   - make addsum
#   - make plist
#

COMMENT=	"self-certifying file system"

BASENAME=	sfs
DISTNAME=	${BASENAME}-0.5k
WRKSRC=		${WRKDIR}/${BASENAME}-0.5
CATEGORIES=	security

HOMEPAGE=	http://www.fs.net/

MAINTAINER=	David Mazieres <dm@openbsd.org>

PERMIT_PACKAGE_CDROM=	Yes
PERMIT_PACKAGE_FTP=	Yes
PERMIT_DISTFILES_CDROM=	Yes
PERMIT_DISTFILES_FTP=	Yes

MASTER_SITES=	http://www.fs.net/download/

SEPARATE_BUILD=	simple
CONFIGURE_STYLE= gnu
.if !defined(NO_SHARED_LIBS)
CONFIGURE_ARGS+= --enable-shlib
.endif
CONFIGURE_ARGS+= --with-confdir=/usr/local/etc/sfs
CONFIGURE_ARGS+= --with-gmp=/usr

.include <bsd.port.mk>
