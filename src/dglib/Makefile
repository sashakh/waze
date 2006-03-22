MODULE_TOPDIR = ../../..

LIB_NAME = $(GRAPH_LIBNAME)

LIB_OBJS = \
	avl.o \
	tavl.o \
	graph.o \
	heap.o \
	tree.o \
	helpers.o \
	graph_v1.o \
	graph_v2.o

include $(MODULE_TOPDIR)/include/Make/Lib.make

DGLINC = $(ARCH_INCDIR)/dgl

default: $(DGLINC)/avl.h $(DGLINC)/tavl.h $(DGLINC)/graph.h $(DGLINC)/heap.h \
	 $(DGLINC)/tree.h $(DGLINC)/type.h $(DGLINC)/helpers.h $(DGLINC)/graph_v1.h $(DGLINC)/graph_v2.h \
	 $(ARCH_INCDIR)/dgl.h lib

$(DGLINC)/avl.h: avl.h
	$(MKDIR) $(DGLINC)
	$(INSTALL) -m 644 avl.h $(DGLINC)/avl.h

$(DGLINC)/tavl.h: tavl.h
	$(INSTALL) -m 644 tavl.h $(DGLINC)/tavl.h

$(DGLINC)/heap.h: heap.h
	$(INSTALL) -m 644 heap.h $(DGLINC)/heap.h

$(DGLINC)/graph.h: graph.h
	$(INSTALL) -m 644 graph.h $(DGLINC)/graph.h

$(DGLINC)/tree.h: tree.h
	$(INSTALL) -m 644 tree.h $(DGLINC)/tree.h

$(DGLINC)/type.h: type.h
	$(INSTALL) -m 644 type.h $(DGLINC)/type.h

$(DGLINC)/graph_v1.h: graph_v1.h
	$(INSTALL) -m 644 graph_v1.h $(DGLINC)/graph_v1.h

$(DGLINC)/graph_v2.h: graph_v2.h
	$(INSTALL) -m 644 graph_v2.h $(DGLINC)/graph_v2.h

$(DGLINC)/helpers.h: helpers.h
	$(INSTALL) -m 644 helpers.h $(DGLINC)/helpers.h

$(ARCH_INCDIR)/dgl.h: dgl.h
	$(INSTALL) -m 644 dgl.h $(ARCH_INCDIR)/dgl.h

DOXNAME=dg
#check for program
checkdoxygen:
	@(type doxygen > /dev/null || (echo "ERROR: Install 'doxygen' software first (get from http://www.doxygen.org)" && exit 1))

htmldocs: checkdoxygen
# hack needed to get main page at beginning:
	@mv $(DOXNAME)lib.dox $(DOXNAME)lib.dox.org
	@cat $(DOXNAME)lib.dox.org | sed 's+/\*! \\page +/\*! \\mainpage +g' > $(DOXNAME)lib.dox
	doxygen ../Doxyfile_arch_html
	@mv $(DOXNAME)lib.dox.org $(DOXNAME)lib.dox
	@echo "HTML reference in directory ./html/index.html"

# NOTE: dglib, diglib and rtree are excluded in ./Doxyfile_arch_latex
# currently only Vect_*() functions in document:
latexdocs: checkdoxygen
	test ! -d latex || (cd ./latex && $(MAKE) clean)
# hack needed to get main page at beginning:
	@mv $(DOXNAME)lib.dox $(DOXNAME)lib.dox.org
	@cat $(DOXNAME)lib.dox.org | sed 's+/\*! \\page +/\*! \\mainpage +g' > $(DOXNAME)lib.dox
	doxygen ../Doxyfile_arch_latex
#this hack is needed to make Acroread's search engine happy:
	(cd ./latex ; echo "\usepackage[T1]{fontenc}" >> doxygen.sty)
	(cd ./latex && $(MAKE) )
	@mv $(DOXNAME)lib.dox.org $(DOXNAME)lib.dox
	@echo "Latex reference in directory ./latex/refman.dvi"

pdfdocs: checkdoxygen
	test ! -d latex || (cd ./latex && $(MAKE) clean)
# hack needed to get main page at beginning:
	@mv $(DOXNAME)lib.dox $(DOXNAME)lib.dox.org
	@cat $(DOXNAME)lib.dox.org | sed 's+/\*! \\page +/\*! \\mainpage +g' > $(DOXNAME)lib.dox
	doxygen ../Doxyfile_arch_latex
#this hack is needed to make Acroread's search engine happy:
	(cd ./latex ; echo "\usepackage[T1]{fontenc}" >> doxygen.sty)
	(cd ./latex && $(MAKE) refman.pdf && mv refman.pdf grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}$(DOXNAME)_`date '+%Y_%m_%d'`_refman.pdf)
	@mv $(DOXNAME)lib.dox.org $(DOXNAME)lib.dox
	@echo "PDF reference in directory ./latex/grass${GRASS_VERSION_MAJOR}${GRASS_VERSION_MINOR}$(DOXNAME)_`date '+%Y_%m_%d'`_refman.pdf"

cleandocs:
	rm -rf ./latex ./html
