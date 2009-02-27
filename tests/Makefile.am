include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS=-I$(top_srcdir)/protocol $(GTK_CFLAGS)
LDADD=$(top_builddir)/engine/libsapwood-client.la

TESTS_ENVIRONMENT=$(srcdir)/sapwood-wrapper $(top_srcdir)/demos/run-demo

noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS+=double-free
double_free_SOURCES=double-free.c
double_free_CPPFLAGS=$(AM_CPPFLAGS) $(GIO_CFLAGS) -I$(top_srcdir)/engine
double_free_LDADD=$(LDADD) $(GIO_LIBS)

TEST_PROGS+=large-window
large_window_SOURCES=large-window.c
large_window_CPPFLAGS=$(AM_CPPFLAGS) -I$(top_srcdir)/engine -DTOP_SRCDIR=\""$(top_srcdir)"\"
large_window_LDADD=$(LDADD)

EXTRA_DIST+=\
	sapwood-wrapper \
	$(NULL)
