# Optional include of site specific configuration
-include $(ROOTDIR)/rules.site

# Optional include of site specific configuration that is propagated from main module
ifdef TOPDIR
-include ${TOPDIR}/rules.site
endif

VERSION=$(shell git describe --abbrev=7 --tags --dirty --always)

ifndef RELEASE
CFLAGS+=-O0 -ggdb -DDEBUG=1
else
CFLAGS+=-O3 -march=native -fno-strict-aliasing -DRELEASE=1 -DNDEBUG=1
endif

CFLAGS+=-Wall -std=gnu99 -static
CPPFLAGS+=-I $(ROOTDIR)/include
LDLIBS+=-Wall

ifdef USE_COVERAGE
CFLAGS+=-coverage
LDLIBS+=-coverage
endif

.PHONY: clean all subdirs ${CLEANSUBDIRS} ${SUBDIRS}

# OpenSSL

OPENSSLINCPATH?=/usr/local/include
OPENSSLLIBPATH?=/usr/local/lib

CPPFLAGS+=-I${OPENSSLINCPATH}

ifdef OPENSSLDYNAMIC
LDLIBS+=-L${OPENSSLLIBPATH} -lssl -lcrypto
else
LDLIBS+=${OPENSSLLIBPATH}/libssl.a ${OPENSSLLIBPATH}/libcrypto.a
endif


# LibEv

EVINCPATH?=/usr/local/include
EVLIBPATH?=/usr/local/lib

CPPFLAGS+=-I${EVINCPATH}

ifdef EVDYNAMIC
LDLIBS+=-L${EVLIBPATH} -lev
else
LDLIBS+=${EVLIBPATH}/libev.a
endif


LDLIBS+=-lm


# Basic commands

subdirs:
	@$(foreach dir, $(SUBDIRS), echo " [CD]" $(dir) && $(MAKE) -C $(dir);)

clean:
	@echo " [RM] in" $(CURDIR)
	@$(RM) $(BIN) $(LIB) $(CLEAN) $(EXTRACLEAN) ${OBJS}
	@$(foreach dir, $(SUBDIRS) $(CLEANSUBDIRS), $(MAKE) -C $(dir) clean;)
	@find . -name "*.gcov" -or -name "*.gcda" -or -name "*.gcno" | xargs $(RM) -f

# Compile command

.c.o:
	@echo " [CC]" $@
	@$(COMPILE.c) $(OUTPUT_OPTION) $<


# Link commands

${BIN}: ${OBJS}
	@echo " [LD]" $@
	@$(LINK.o) $^ ${UTOBJS} $(LOADLIBES) -ldl $(LDLIBS) -ldl -o $@
ifdef RELEASE
	@echo " [ST]" $@
	@strip $@
	@bash -c "if [[ `nm $@ | grep _ev_ | wc -l` -gt 0 ]] ; then \
		 echo \"Produced binary contains unwanted symbols!\"; \
		 exit 1; \
	fi"
endif

${LIB}: ${OBJS}
	@echo " [AR]" $@
	@$(AR) -cr $@ $^

# Utility commands
rebuild:
	(cd $(ROOTDIR) && make clean all) && make clean all
