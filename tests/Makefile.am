include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS=-I$(top_srcdir)/protocol $(GTK_CFLAGS)
LDADD=$(top_builddir)/engine/libsapwood-client.la

TESTS_ENVIRONMENT=$(srcdir)/sapwood-wrapper $(top_srcdir)/demos/run-demo

noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS+=double-free
double_free_SOURCES=double-free.c
double_free_CPPFLAGS=$(AM_CPPFLAGS) $(GIO_CFLAGS) -I$(top_srcdir)/engine
double_free_LDADD=$(LDADD) $(GIO_LIBS)

EXTRA_DIST+=\
	sapwood-wrapper \
	$(NULL)
