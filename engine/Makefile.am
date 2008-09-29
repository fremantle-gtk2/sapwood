include $(top_srcdir)/Makefile.decl

INCLUDES = $(GTK_CFLAGS) \
	   -DSAPWOOD_SERVER=\"$(daemondir)/sapwood-server\" \
	   -I$(top_srcdir)/protocol

enginedir = $(libdir)/gtk-2.0/$(GTK_VERSION)/engines

engine_LTLIBRARIES=libsapwood.la
noinst_LTLIBRARIES=libsapwood-client.la

libsapwood_la_SOURCES=\
	sapwood-main.c \
	sapwood-rc-style.c	\
	sapwood-rc-style.h	\
	sapwood-style.c		\
	sapwood-style.h		\
	theme-pixbuf.c		\
	theme-pixbuf.h		\
	sapwood-pixmap.c \
	sapwood-pixmap.h \
	sapwood-pixmap-priv.h \
	$(NULL)

libsapwood_client_la_SOURCES=\
	sapwood-client.c \
	sapwood-client.h \
	$(NULL)

libsapwood_la_LDFLAGS = -avoid-version -module -Wl,-z,defs
libsapwood_la_LIBADD=$(GTK_LIBS) \
	libsapwood-client.la \
	$(NULL)
libsapwood_la_CFLAGS = -DG_LOG_DOMAIN=\"sapwood\" $(AM_CFLAGS)

libsapwood_client_la_LIBADD=\
	$(top_builddir)/protocol/libprotocol.la \
	$(NULL)

MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = actual-abi actual-abi2 expected-abi expected-abi2 optional-abi

EXTRA_DIST+=abicheck.sh

TESTS = abicheck.sh
