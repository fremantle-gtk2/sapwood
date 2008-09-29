include $(top_srcdir)/Makefile.decl

INCLUDES = \
    $(GDK_CFLAGS) \
    -I../protocol \
    -DSAPWOOD_SERVER=\"$(daemondir)/sapwood-server\"

daemondir = $(libdir)/sapwood
daemon_PROGRAMS = sapwood-server

sapwood_server_SOURCES = sapwood-server.c
sapwood_server_LDADD = $(GDK_LIBS) ../protocol/libprotocol.la
sapwood_server_CFLAGS = $(AM_CFLAGS)	# created both with libtool and without

MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = actual-abi actual-abi2 expected-abi expected-abi2 optional-abi

EXTRA_DIST = abicheck.sh

TESTS = abicheck.sh
