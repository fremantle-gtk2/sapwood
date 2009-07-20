include $(top_srcdir)/Makefile.decl

INCLUDES = $(GTK_CFLAGS)
LDADD = $(GTK_LIBS)

TEST_PROGS+=buttonbox
TESTS_ENVIRONMENT=SAPWOOD_TESTING=true $(top_srcdir)/tests/sapwood-wrapper
noinst_PROGRAMS = \
	$(TEST_PROGS) \
	gradient \
	treeview

CLEANFILES = engines
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST+=\
	buttonbox.c \
	buttonbox.gtkrc \
	images/hbbox-brt.png \
	images/hbbox-tb.png \
	images/hbbox-tlb.png \
	images/hbbox-tlbr.png \
	gradient.c \
	gradient.gtkrc \
	images/gradient.png \
	treeview.c \
	treeview.gtkrc \
	images/treeview-active.png \
	images/treeview-active-alone.png \
	images/treeview-active-left.png \
	images/treeview-active-middle.png \
	images/treeview-active-right.png \
	images/treeview-normal.png \
	images/treeview-passive.png \
	images/treeview-selected.png \
	run-demo
