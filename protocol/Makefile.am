INCLUDES = \
    $(GTK_CFLAGS) \
    -DSAPWOOD_SERVER=\"$(daemondir)/sapwood-server\"

noinst_LTLIBRARIES = \
    libprotocol.la

libprotocol_la_LIBADD = \
    $(GTK_LIBS)

libprotocol_la_SOURCES = \
    sapwood-proto.c    \
    sapwood-proto.h
