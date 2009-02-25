include $(top_srcdir)/Makefile.decl

INCLUDES = \
    $(GDK_CFLAGS) \
    -I$(top_srcdir)/protocol \
    -DSAPWOOD_SERVER=\"$(daemondir)/sapwood-server\"

daemondir = $(libdir)/sapwood
daemon_PROGRAMS = sapwood-server

sapwood_server_SOURCES = \
	cache-node.c \
	cache-node.h \
	sapwood-server.c
sapwood_server_LDADD = $(GDK_LIBS) ../protocol/libprotocol.la
sapwood_server_CFLAGS = $(AM_CFLAGS)	# created both with libtool and without

