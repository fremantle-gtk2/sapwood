include $(top_srcdir)/Makefile.decl

AM_CPPFLAGS=-I$(top_srcdir)/protocol $(GTK_CFLAGS)
LDADD=$(top_builddir)/protocol/libprotocol.la

TESTS_ENVIRONMENT=./sapwood-wrapper

noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS+=double-free
double_free_SOURCES=double-free.c
double_free_CPPFLAGS=$(AM_CPPFLAGS) $(GIO_CFLAGS) -I$(top_srcdir)/engine
double_free_LDADD=$(LDADD) $(GIO_LIBS)

