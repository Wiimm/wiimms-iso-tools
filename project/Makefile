
#####################################################################
##                 __            __ _ ___________                  ##
##                 \ \          / /| |____   ____|                 ##
##                  \ \        / / | |    | |                      ##
##                   \ \  /\  / /  | |    | |                      ##
##                    \ \/  \/ /   | |    | |                      ##
##                     \  /\  /    | |    | |                      ##
##                      \/  \/     |_|    |_|                      ##
##                                                                 ##
##                        Wiimms ISO Tools                         ##
##                      https://wit.wiimm.de/                      ##
##                                                                 ##
#####################################################################
##                                                                 ##
##   This file is part of the WIT project.                         ##
##   Visit https://wit.wiimm.de/ for project details and sources.  ##
##                                                                 ##
##   Copyright (c) 2009-2021 by Dirk Clemens <wiimm@wiimm.de>      ##
##                                                                 ##
#####################################################################

#----------------------------------------------------------------
# make manual: http://www.gnu.org/software/make/manual/make.html
#----------------------------------------------------------------

#-------------------------------------------------------------------------------
# global settings

MYMAKE			:= $(lastword $(MAKEFILE_LIST))

#SHELL			= /bin/bash
SHELL			= /usr/bin/env bash

AUTHOR			= Dirk Clemens
TOOLSET_SHORT		= WIT
TOOLSET_LONG		= Wiimms ISO Tools

WIT_SHORT		= wit
WIT_LONG		= Wiimms ISO Tool
WWT_SHORT		= wwt
WWT_LONG		= Wiimms WBFS Tool
WDF_SHORT		= wdf
WDF_LONG		= Wiimms WDF Tool
WFUSE_SHORT		= wfuse
WFUSE_LONG		= Wiimms FUSE Tool

VERSION_NUM		= 3.05a
BETA_VERSION		= 0
			# 0:off  -1:"beta"  >0:"beta#"

URI_HOME		= https://wit.wiimm.de/
URI_DOWNLOAD		= https://wit.wiimm.de/download
URI_FILE		= https://wit.wiimm.de/file

ifeq ($(BETA_VERSION),0)
  URI_REPOS		= http://opensvn.wiimm.de/wii/trunk/wiimms-iso-tools/
  URI_VIEWVC		= https://wit.wiimm.de/r/viewvc
else
  URI_REPOS		= http://opensvn.wiimm.de/wii/branches/public/wiimms-iso-tools/
  URI_VIEWVC		= https://wit.wiimm.de/r/viewvc-beta
endif

URI_WDF			= https://wit.wiimm.de/r/wdf
URI_CISO		= https://wit.wiimm.de/r/ciso
URI_QTWITGUI		= https://wit.wiimm.de/r/qtwitgui
URI_WIIBAFU		= https://wit.wiimm.de/r/wiibafu
#URI_WCDWM		= https://wit.wiimm.de/r/wcdwm
#URI_WIIJMANAGER	= https://wit.wiimm.de/r/wiijman
URI_MACWITGUI		= https://wit.wiimm.de/r/mac-witgui
URI_GBATEMP		= http://gbatemp.net/index.php?showtopic=182236\#entry2286365
URI_DOWNLOAD_I386	= $(URI_DOWNLOAD)/$(DISTRIB_I386)
URI_DOWNLOAD_X86_64	= $(URI_DOWNLOAD)/$(DISTRIB_X86_64)
URI_DOWNLOAD_MAC	= $(URI_DOWNLOAD)/$(DISTRIB_MAC)
URI_DOWNLOAD_CYGWIN	= $(URI_DOWNLOAD)/$(DISTRIB_CYGWIN)
URI_TITLES		= http://gametdb.com/titles.txt

DOWNLOAD_DIR		= /n/www/wit.wiimm.de/download
EXPORT_DIR		= /cygdrive/n/wit/wiimms-iso-tools/bin

DUMMY			:= $(shell $(SHELL) ./setup.sh)
include Makefile.setup

#------------------------------------------------------------------------------
# format for logging messages, $1=job, $2=object, $3=more info

LOGFORMAT		:= \33[36;1m*** %7s %-17s %s\33[0m\n
LOGFORMAT_CC		:= \33[33;1m*** %7s %-17s %s\33[0m\n
LOGFORMAT_TOOL		:= \33[35;1m*** %7s %-17s %s\33[0m\n
LOGFORMAT_INST		:= \33[32;1m*** %7s %-17s %s\33[0m\n

#------------------------------------------------------------------------------
# include source library definitions

DCLIB_MYSQL := 0
include ./dclib/Makefile.inc

#-------------------------------------------------------------------------------
# version+beta settings

ifeq ($(BETA_VERSION),0)
  BETA_SUFFIX	:=
else ifeq ($(BETA_VERSION),-1)
  BETA_SUFFIX	:= .beta
else
  BETA_SUFFIX	:= .beta$(BETA_VERSION)
endif

VERSION		:= $(VERSION_NUM)$(BETA_SUFFIX)

#-------------------------------------------------------------------------------
# compiler settings

PRE		?= 
CC		= $(PRE)gcc
CPP		= $(PRE)g++
STRIP		= $(PRE)strip

#-------------------------------------------------------------------------------
# files

DIR_LIST	=

RM_FILES	= *.{a,o,d,tmp,bak,exe} */*.{tmp,bak} */*/*.{tmp,bak}
RM_FILES2	= *.{iso,ciso,wdf,wbfs} templates.sed Makefile.setup

MODE_FILE	= ./_mode.flag
MODE		= $(shell test -s $(MODE_FILE) && cat $(MODE_FILE))
RM_FILES	+= $(MODE_FILE)

# wbfs: files / size in GiB of WBFS partition / number of ISO files to copy
WBFS_FILE	?= a.wbfs
WBFS_FILES	?= $(WBFS_FILE) b.wbfs c.wbfs d.wbfs
WBFS_SIZE	?= 20
WBFS_COUNT	?= 4

#-------------------------------------------------------------------------------
# tools

MAIN_TOOLS	:= wit wwt wdf
TEST_TOOLS	:= wtest
EXTRA_TOOLS	:=

ifeq ($(HAVE_FUSE),1)
  MAIN_TOOLS	+= wfuse
else
  EXTRA_TOOLS	:= wfuse
endif

ALL_TOOLS	:= $(sort $(MAIN_TOOLS) $(TEST_TOOLS))
ALL_TOOLS_X	:= $(sort $(MAIN_TOOLS) $(TEST_TOOLS) $(EXTRA_TOOLS))

HELPER_TOOLS	:= gen-ui

WDF_TEST_LINKS	:= WdfCat UnWdf WdfCmp WdfDump Ciso CisoCat UnCiso Wbi
WDF_LINKS	:= wdf-cat wdf-dump

RM_FILES	+= $(ALL_TOOLS_X) $(HELPER_TOOLS) $(WDF_LINKS) $(WDF_TEST_LINKS)

#-------------------------------------------------------------------------------
# tool dependent options and objects

ifeq ($(STATIC),1)
    OPT_STATIC	:= -static -DDCLIB_STATIC=1
else
    OPT_STATIC	:=
endif

#---------------

TOPT_wit	:= $(OPT_STATIC)
TOPT_wwt	:= $(OPT_STATIC)
TOPT_wdf	:= $(OPT_STATIC)
TOPT_wfuse	:= -lfuse -lpthread -ldl

#TOPT_ALL	:= $(TOPT_wit) $(TOPT_wwt) $(TOPT_wdf) $(TOPT_wfuse)

#---------------

TOBJ_wit	:= wit-mix.o
TOBJ_wwt	:=
TOBJ_wdf	:=
TOBJ_wfuse	:=

TOBJ_ALL	:= $(TOBJ_wit) $(TOBJ_wwt) $(TOBJ_wdf) $(TOBJ_wfuse)

#-------------------------------------------------------------------------------
# sub libs

# libbz2 & lzma
LIBBZ2_SRC	= $(shell echo src/libbz2/*.c)
LIBBZ2_OBJ	= $(patsubst %.c,%.o,$(LIBBZ2_SRC))

LZMA_SRC	= $(shell echo src/lzma/*.c)
LZMA_OBJ	= $(patsubst %.c,%.o,$(LZMA_SRC))

ifeq ($(SYSTEM),cygwin)
  LZMA_FLAGS	= -Wno-unused-but-set-variable
  BZIP2_FLAGS	= -Wno-maybe-uninitialized
else ifeq ($(SYSTEM),mac)
  LZMA_FLAGS	= 
  BZIP2_FLAGS	= 
else
  LZMA_FLAGS	= -Wno-unused-but-set-variable
  BZIP2_FLAGS	= -Wno-maybe-uninitialized
endif

# lib summary
LIB_LIST	+= libbz2 lzma
LIB_OBJECTS	+= $(LIBBZ2_OBJ) $(LZMA_OBJ)
RM_FILES	+= $(foreach l,$(LIB_LIST),src/$(l)/*.{d,o})


#-------------------------------------------------------------------------------
# source files

UI_FILES	= ui.def
UI_FILES	+= $(patsubst %,ui-%.c,$(MAIN_TOOLS) $(EXTRA_TOOLS))
UI_FILES	+= $(patsubst %,ui-%.h,$(MAIN_TOOLS) $(EXTRA_TOOLS))

UI_TABS		= $(patsubst %,tab-%.inc,$(MAIN_TOOLS) $(EXTRA_TOOLS))

SETUP_DIR	=  ./setup
SETUP_INFO	=  INSTALL.txt
SETUP_FILES	=  version.h install.sh cygwin-copy.sh wit.def \
		   $(SETUP_INFO) $(CYGWIN_SCRIPTS)
DIR_LIST	+= $(SETUP_DIR)
RM_FILES2	+= $(SETUP_FILES)

TEXT_DIR	=  ./text-files
TEXT_FILES	=  logo.inc ui-head.inc config-paths.inc
DIR_LIST	+= $(TEXT_DIR)
RM_FILES2	+= $(TEXT_FILES)

#-------------------------------------------------------------------------------
# object files

# objects of tools
MAIN_TOOLS_OBJ	:= $(patsubst %,%.o,$(MAIN_TOOLS) $(EXTRA_TOOLS))
OTHER_TOOLS_OBJ	:= $(patsubst %,%.o,$(TEST_TOOLS) $(HELPER_TOOLS))

# dclib
DCLIB_DIR	:= ./dclib
DIR_LIST	+= $(DCLIB_DIR)
VPATH		+= $(DCLIB_DIR)
CFLAGS		+= -I$(DCLIB_DIR)

# other objects
WIT_O		:= lib-std.o lib-file.o lib-sf.o \
		   lib-bzip2.o lib-lzma.o lib-dol.o \
		   lib-wdf.o lib-wia.o lib-ciso.o lib-gcz.o \
		   iso-interface.o wbfs-interface.o patch.o \
		   titles.o match-pattern.o dclib-utf8.o \
		   sha1dgst.o sha1_one.o \
		   $(DCLIB_O)

LIBWBFS_O	:= tools.o file-formats.o libwbfs.o wiidisc.o cert.o rijndael.o

ifeq ($(SYSTEM),cygwin)
WIT_O		+= winapi.o
endif

# object groups
UI_OBJECTS	:= $(sort $(MAIN_TOOLS_OBJ))
#C_OBJECTS	:= $(sort $(OTHER_TOOLS_OBJ) $(WIT_O) $(LIBWBFS_O) $(LZMA_O) $(TOBJ_ALL))
C_OBJECTS	:= $(sort $(OTHER_TOOLS_OBJ) $(WIT_O) $(LIBWBFS_O) $(TOBJ_ALL))
ASM_OBJECTS	:= ssl-asm.o

# all objects + sources
#ALL_OBJECTS	= $(sort $(WIT_O) $(LIBWBFS_O) $(LZMA_O) $(ASM_OBJECTS) $(LIB_OBJECTS))
ALL_OBJECTS	= $(sort $(WIT_O) $(LIBWBFS_O) $(ASM_OBJECTS) $(LIB_OBJECTS))
ALL_SOURCES	= $(patsubst %.o,%.c,$(UI_OBJECTS) $(C_OBJECTS) $(ASM_OBJECTS))

#-------------------------------------------------------------------------------

SHARE_PATH	= $(INSTALL_PATH)/share/wit
INSTALL_SCRIPTS	= install.sh load-titles.sh
RM_FILES	+= $(INSTALL_SCRIPTS)
SCRIPTS		= ./scripts
TEMPLATES	= ./templates
MODULES		= $(TEMPLATES)/module
GEN_TEMPLATE	= ./gen-template.sh
GEN_TEXT_FILE	= ./gen-text-file.sh
UI		= ./src/ui
DIR_LIST	+= $(SCRIPTS) $(TEMPLATES) $(MODULES)

#VPATH		+= src src/libwbfs src/lzma src/crypto $(UI) work
#DIR_LIST	+= src src/libwbfs src/lzma src/crypto $(UI) work
VPATH		+= src src/libwbfs src/crypto $(UI) work
DIR_LIST	+= src src/libwbfs src/crypto $(UI) work

DEFINES1	+= -DLARGE_FILES -D_FILE_OFFSET_BITS=64
DEFINES1	+= -DWIT		# compile wit tools (for shared sources (e.g. libwbfs))
DEFINES1	+= -DDEBUG_ASSERT	# enable ASSERTions in release version too
DEFINES1	+= -DEXTENDED_ERRORS=1	# enable extended error messages (function,line,file)
DEFINES1	+= -D_7ZIP_ST=1		# disable 7zip multi threading
DEFINES1	+= -D_LZMA_PROB32=1	# LZMA option
DEFINES		=  $(strip $(DEFINES1) $(MODE) $(XDEF))

CFLAGS		+= -std=gnu99 -fomit-frame-pointer -fno-strict-aliasing -funroll-loops
CFLAGS		+= -Wall -Wno-parentheses -Wno-unused-function
#CFLAGS		+= -O3 -Isrc/libwbfs -Isrc/lzma -Isrc -I$(UI) -I. -Iwork
CFLAGS		+= -O3 -Isrc/libwbfs -Isrc -I$(UI) -I. -Iwork

ifeq ($(SYSTEM_LINUX),1)
  LIBS		+= -ltinfo
  CFLAGS	+= -Wno-format-truncation
endif

ifeq ($(SYSTEM),cygwin)
  CFLAGS	+= -Wno-format-truncation
endif

ifeq ($(SYSTEM),mac)
  CFLAGS	+= -I/usr/local/include
else
  LDFLAGS	+= -static-libgcc
endif

ifeq ($(shell expr $(GCC_VERSION) '>=' 7.0 ),1)
  CFLAGS	+= -Wno-format-truncation
endif

CFLAGS		+= $(XFLAGS)
CFLAGS		:= $(strip $(CFLAGS))

DEPFLAGS	+= -MMD


#--- system depedent flags & libs

LDFLAGS		:= $(strip $(LDFLAGS))

ifeq ($(HAVE_ZLIB),1)
 LIBS		+= -lz
endif
LIBS		+= -lm -lncurses $(XLIBS)

ifeq ($(HAVE_SHA),1)
 ifeq ($(SYSTEM_LINUX),1)
  ifneq ($(STATIC),1)
    DEFINES1	+= -DUSE_SHA_LIB
    LIBS	+= -lcrypto
  endif
 else ifeq ($(SYSTEM),cygwin)
    DEFINES1	+= -DUSE_SHA_LIB
    LIBS	+= -lcrypto
 endif
endif

#-------------------------------------------------------------------------------

DISTRIB_RM	= ./wit-v$(VERSION)-r
DISTRIB_BASE	= wit-v$(VERSION)-r$(REVISION_NEXT)
DISTRIB_PATH	= ./$(DISTRIB_BASE)-$(SYSTEM2)
DISTRIB_I386	= $(DISTRIB_BASE)-i386.tar.gz
DISTRIB_X86_64	= $(DISTRIB_BASE)-x86_64.tar.gz
DISTRIB_MAC	= $(DISTRIB_BASE)-mac.tar.gz
DISTRIB_CYGWIN32 = $(DISTRIB_BASE)-cygwin32.zip
DISTRIB_CYGWIN64 = $(DISTRIB_BASE)-cygwin64.zip
DISTRIB_FILES	= gpl-2.0.txt $(INSTALL_SCRIPTS) $(SETUP_INFO)

DOC_FILES	= doc/*.txt
IGNORE_DOC_FILES= HISTORY-v*.txt
TITLE_FILES	= titles.txt $(patsubst %,titles-%.txt,$(LANGUAGES))
LANGUAGES	= de es fr it ja ko nl pt ru zhcn zhtw

BIN_FILES	= $(MAIN_TOOLS) $(EXTRA_TOOLS)
SHARE_FILES	= $(TITLE_FILES) system-menu.txt magic.txt

CYGWIN_DIR	= /usr/bin
CYGWIN_TOOLS	= bash cp diff env grep mkdir mv realpath regtool rm stat tr wget
CYGWIN_SCRIPTS	= load-titles.sh load-titles.bat \
		  windows-install.sh windows-uninstall.sh
CYGWIN_INSTALLER= $(SETUP_DIR)/windows-install.exe

WIN_INSTALL_PATH= Wiimm/WIT

DIR_LIST_BIN	= $(SCRIPTS) bin
DIR_LIST	+= $(DIR_LIST_BIN)
DIR_LIST	+= share work pool makefiles-local edit-list


#-------------------------------------------------------------------------------
# sub projects

SUB_PROJECTS	+= test-libwbfs
RM_FILES	+= $(foreach p,$(SUB_PROJECTS),$(p)/*.{d,o} $(p)/$(p))

#
###############################################################################
# default rule

default_rule: all
	@echo "HINT: try 'make help'"

# include this behind the default rule
-include $(ALL_SOURCES:.c=.d)

#
###############################################################################
# general rules

$(ALL_TOOLS_X): %: %.o $(ALL_OBJECTS) $(TOBJ_ALL) Makefile | $(HELPER_TOOLS)
	@printf "$(LOGFORMAT_TOOL)" tool "$@" "$(MODE) $(TOPT_$@) $(TOBJ_$@)"
	@$(CC) $(CFLAGS) $(DEFINES) $(LDFLAGS) $@.o \
		$(ALL_OBJECTS) $(TOBJ_$@) $(LIBS) $(TOPT_$@) -o $@
	@if test -f $@.exe; then $(STRIP) $@.exe; else $(STRIP) $@; fi

	@mkdir -p bin/$(SYSTEM2) bin/$(SYSTEM2)/debug
	@if test -s $(MODE_FILE) && grep -Fq -e -DDEBUG $(MODE_FILE); \
		then cp -p $@ bin/$(SYSTEM2)/debug/; \
		else cp -p $@ bin/; cp -p $@ bin/$(SYSTEM2)/; fi

#--------------------------

$(HELPER_TOOLS): %: %.o $(ALL_OBJECTS) $(UI_TABS) Makefile
	@printf "$(LOGFORMAT_TOOL)" helper "$@ $(TOBJ_$@)" "$(MODE)"
	@$(CC) $(CFLAGS) $(DEFINES) $(LDFLAGS) $@.o \
		$(ALL_OBJECTS) $(TOBJ_$@) $(LIBS) -o $@

#--------------------------

$(WDF_LINKS): wdf
	@printf "$(LOGFORMAT_INST)" "link" "wdf -> $@" ""
	@ln -f wdf "$@"

#--------------------------

$(UI_OBJECTS): %.o: %.c ui-%.c ui-%.h version.h Makefile
	@printf "$(LOGFORMAT_CC)" +object "$@" "$(MODE)"
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(DEFINES) -c $< -o $@

#--------------------------

$(C_OBJECTS): %.o: %.c version.h Makefile $(TEXT_FILES)
	@printf "$(LOGFORMAT_CC)" object "$@" "$(MODE)"
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(DEFINES) -c $< -o $@

#--------------------------

$(ASM_OBJECTS): %.o: %.S Makefile
	@printf "$(LOGFORMAT_CC)" asm "$@" "$(MODE)"
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(DEFINES) -c $< -o $@

#--------------------------

$(SETUP_FILES): templates.sed $(SETUP_DIR)/$@
	@printf "$(LOGFORMAT)" create "$@" ""
	@chmod 775 $(GEN_TEMPLATE)
	@$(GEN_TEMPLATE) $@

#--------------------------

$(TEXT_FILES): $(GEN_TEXT_FILE) $(TEXT_DIR)/$@
	@printf "$(LOGFORMAT)" text "$@" ""
	@chmod 775 $(GEN_TEXT_FILE)
	@$(GEN_TEXT_FILE) $(TEXT_DIR) $@

#--------------------------

$(UI_FILES): gen-ui.c ui.h $(UI_TABS) | gen-ui
	@printf "$(LOGFORMAT)" run gen-ui ""
	@./gen-ui

.PHONY : ui
ui : gen-ui
	@printf "$(LOGFORMAT)" run gen-ui ""
	@./gen-ui

#
###############################################################################
# lib specific rules

$(LIBBZ2_OBJ): %.o: %.c Makefile
	@printf "$(LOGFORMAT_CC)" object "$(subst src/libbz2/,,$@)" "$(MODE) [libbz2]"
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(BZIP2_FLAGS) $(DEFINES) -c $< -o $@

$(LZMA_OBJ): %.o: %.c Makefile
	@printf "$(LOGFORMAT_CC)" object "$(subst src/lzma/,,$@)" "$(MODE) [lzma]"
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(LZMA_FLAGS) $(DEFINES) -c $< -o $@

#
###############################################################################
# specific rules in alphabetic order

.PHONY : all
all: $(HELPER_TOOLS) $(ALL_TOOLS) $(WDF_LINKS) $(INSTALL_SCRIPTS) $(SETUP_INFO)

.PHONY : all+
all+: clean+ all distrib

.PHONY : all++
all++: clean+ all titles distrib

#
#--------------------------

.PHONY : ch+
ch+:	chmod chown chgrp

#
#--------------------------

.PHONY : chmod
chmod:
	@printf "$(LOGFORMAT)" chmod 775/664 ""
	@for d in . $(DIR_LIST); do test -d "$$d" && chmod ug+rw "$$d"/*; done
	@for d in $(DIR_LIST); do test -d "$$d" && chmod 775 "$$d"; done
	@find . -name '*.sh' -exec chmod 775 {} +
	@for t in $(ALL_TOOLS_X); do test -f "$$t" && chmod 775 "$$t"; done || true
	@chmod 664 src/*.[hc] src/*/*.[hc] dclib/*.[hc]

#
#--------------------------

.PHONY : chown
chown:
	@printf "$(LOGFORMAT)" chown "-R $$( stat -c%u . 2>/dev/null || stat -f%u . ) ." ""
	@chown -R "$$( stat -c%u . 2>/dev/null || stat -f%u . )" .

#
#--------------------------

.PHONY : chgrp
chgrp:
	@printf "$(LOGFORMAT)" chgrp "-R $$( stat -c%g . 2>/dev/null || stat -f%g . ) ." ""
	@chgrp -R "$$( stat -c%g . 2>/dev/null || stat -f%g . )" .

#
#--------------------------

.PHONY : clean
clean:
	@printf "$(LOGFORMAT)" rm "output files + distrib" ""
	@rm -f $(RM_FILES)
	@rm -fr $(DISTRIB_RM)*
	@cd dclib && rm -f $(DCLIB_PRIVATE)

.PHONY : c+ clean+
c+ clean+: clean
	@printf "$(LOGFORMAT)" rm "test files + template output" ""
	@rm -f $(RM_FILES2)
	-@rm -fr doc

.PHONY : c++ clean++
c++ clean++: clean+
	@test -d .svn && svn st | sort -k2 || true

#
#--------------------------

.PHONY : debug
debug:
	@printf "$(LOGFORMAT)" enable debug "-> define -DDEBUG"
	@rm -f *.o $(ALL_TOOLS_X)
	@echo "-DDEBUG" >>$(MODE_FILE)
	@sort $(MODE_FILE) | uniq > $(MODE_FILE).tmp
# 2 steps to bypass a cygwin mv failure
	@cp $(MODE_FILE).tmp $(MODE_FILE)
	@rm -f $(MODE_FILE).tmp

#
#--------------------------

.PHONY : distrib2
distrib2:
ifeq ($(SYSTEM_LINUX),1)

	@printf "\n---------- BUILDING LINUX/I386 ----------\n\n"
	@for t in $(ALL_TOOLS_X); do rm -f bin/$$t; done
	@M32=1 $(MAKE) --no-print-directory clean+ distrib
	@mv "$(DISTRIB_I386)" "save-$(DISTRIB_I386)"

	@printf "\n---------- BUILDING LINUX/X86_64 ----------\n\n"
	@for t in $(ALL_TOOLS_X); do rm -f bin/$$t; done
	@$(MAKE) --no-print-directory clean+ distrib
	@mv "save-$(DISTRIB_I386)" "$(DISTRIB_I386)"

else
	@$(MAKE) --no-print-directory clean+ distrib
endif

#-----

.PHONY : distrib
distrib:
ifeq ($(SYSTEM),mac)
	@$(MAKE) --no-print-directory mac-distrib
else ifeq ($(SYSTEM),cygwin)
	$(MAKE) --no-print-directory all doc cygwin-copy.sh \
		$(CYGWIN_SCRIPTS) gen-distrib wit.def
else
	@STATIC=1 $(MAKE) --no-print-directory all doc gen-distrib wit.def
endif

#-----

.PHONY : gen-distrib
gen-distrib:
	@printf "$(LOGFORMAT)" create "$(DISTRIB_PATH)" ""

ifeq ($(SYSTEM),cygwin)

	@rm -rf $(DISTRIB_PATH)/* 2>/dev/null || true
	@rm -rf $(DISTRIB_PATH) 2>/dev/null || true
	@mkdir -p $(DISTRIB_PATH)/bin $(DISTRIB_PATH)/doc
	@./cygwin-copy.sh
	@printf '@cmd\r\n' >$(DISTRIB_PATH)/bin/wit-console.bat
	@cp -p gpl-2.0.txt $(SETUP_INFO) $(DISTRIB_PATH)
	@ln -f $(MAIN_TOOLS) $(WDF_LINKS) $(CYGWIN_SCRIPTS) $(DISTRIB_PATH)/bin
	@ln -f $(CYGWIN_INSTALLER) $(DISTRIB_PATH)/windows-install.exe
	@ln -f $(CYGWIN_INSTALLER) $(DISTRIB_PATH)/windows-uninstall.exe
	@ln -f $(CYGWIN_INSTALLER) $(DISTRIB_PATH)/bin/windows-install.exe
	@ln -f $(CYGWIN_INSTALLER) $(DISTRIB_PATH)/bin/windows-uninstall.exe
	@( cd share; cp $(SHARE_FILES) ../$(DISTRIB_PATH)/bin )
	@cp -p $(DOC_FILES) $(DISTRIB_PATH)/doc
	@rm -f $(DISTRIB_PATH)/doc/$(IGNORE_DOC_FILES)

	@zip -roq $(DISTRIB_PATH).zip $(DISTRIB_PATH)
	@chmod 664 $(DISTRIB_PATH).zip

else

	@rm -rf $(DISTRIB_PATH)
	@mkdir -p $(DISTRIB_PATH)/bin $(DISTRIB_PATH)/scripts
	@mkdir -p $(DISTRIB_PATH)/share $(DISTRIB_PATH)/doc

	@cp -p $(DISTRIB_FILES) $(DISTRIB_PATH)
	@ln -f $(MAIN_TOOLS) $(WDF_LINKS) $(DISTRIB_PATH)/bin
	@for t in $(EXTRA_TOOLS); do [[ -f bin/$(SYSTEM)/$$t ]] \
		&& ln -f bin/$(SYSTEM)/$$t $(DISTRIB_PATH)/bin; done || true
	@cp -p share/*.txt $(DISTRIB_PATH)/share
	@cp -p $(DOC_FILES) $(DISTRIB_PATH)/doc
	@rm -f $(DISTRIB_PATH)/doc/$(IGNORE_DOC_FILES)
	@cp -p $(SCRIPTS)/*.{sh,txt} $(DISTRIB_PATH)/scripts

	@chmod -R 664 $(DISTRIB_PATH)
	@chmod a+x $(DISTRIB_PATH)/*.sh $(DISTRIB_PATH)/scripts/*.sh $(DISTRIB_PATH)/bin*/*
	@chmod -R a+X $(DISTRIB_PATH)

	@tar -czf $(DISTRIB_PATH).tar.gz $(DISTRIB_PATH)
	@chmod 664 $(DISTRIB_PATH).tar.gz

endif

#-----

.PHONY : copy-distrib
copy-distrib: distrib2
	@printf "$(LOGFORMAT)" copy "$(DISTRIB_BASE).*" "-> $(DOWNLOAD_DIR)"
	@[[ -s "$(DISTRIB_I386)" ]] \
		&& cp --preserve=time "$(DISTRIB_I386)" "$(DOWNLOAD_DIR)" \
		|| true
	@[[ -s "$(DISTRIB_X86_64)" ]] \
		&& cp --preserve=time "$(DISTRIB_X86_64)" "$(DOWNLOAD_DIR)" \
		|| true
	@[[ -s "$(DISTRIB_CYGWIN32)" ]] \
		&& cp --preserve=time "$(DISTRIB_CYGWIN32)" "$(DOWNLOAD_DIR)" \
		|| true
	@[[ -s "$(DISTRIB_CYGWIN64)" ]] \
		&& cp --preserve=time "$(DISTRIB_CYGWIN64)" "$(DOWNLOAD_DIR)" \
		|| true

#
#--------------------------

.PHONY : doc
doc: $(MAIN_TOOLS) templates.sed gen-doc

.PHONY : gen-doc
gen-doc:
	@printf "$(LOGFORMAT)" create documentation ""
	@chmod ug+x $(GEN_TEMPLATE)
	@$(GEN_TEMPLATE)

#
#--------------------------

.PHONY : flags
flags:
	@echo  ""
	@echo  "DEFINES: $(DEFINES)"
	@echo  ""
	@echo  "CFLAGS:  $(CFLAGS)"
	@echo  ""
	@echo  "LDFLAGS: $(LDFLAGS)"
	@echo  ""
	@echo  "LIBS: $(LIBS)"
	@echo  ""
	@echo  "LIBBZ2_OBJ: $(LIBBZ2_OBJ)"
	@echo  ""
	@echo  "LZMA_OBJ: $(LZMA_OBJ)"
	@echo  ""
	@echo  "C_OBJECTS: $(C_OBJECTS)"
	@echo  ""

#
#--------------------------

.PHONY : install
install: all
ifeq ($(SYSTEM),cygwin)
	@[[ -d $(DISTRIB_PATH)/bin ]] || make distrib
	@( cd "$(DISTRIB_PATH)/bin" && ./windows-install.sh --cygwin )
else
	@chmod a+x install.sh
	@./install.sh --make
endif

.PHONY : install+
install+: clean+ all
	@chmod a+x install.sh
	@./install.sh --make

#--------------------------

.PHONY : install2
install2:
ifeq ($(SYSTEM_LINUX),1)

	@printf "\n---------- BUILDING LINUX/I386 ----------\n\n"
	@for t in $(ALL_TOOLS_X); do rm -f bin/$$t; done
	@$(MAKE) --no-print-directory clean+
	@M32=1 $(MAKE) -j --no-print-directory install

ifeq ($(HAVE_INSTBIN_32),1)
	@printf "$(LOGFORMAT_INST)" copy "$(INSTBIN)/* to $(INSTBIN_32)"
	@for f in $(BIN_FILES); do [[ -f $(INSTBIN)/$$f ]] \
		&& cp -p $(INSTBIN)/$$f $(INSTBIN_32); done; true
	@for f in $(WDF_LINKS); do ln -f $(INSTBIN_32)/wdf $(INSTBIN_32)/$$f; done
endif

	@printf "\n---------- BUILDING LINUX/X86_64 ----------\n\n"
	@for t in $(ALL_TOOLS_X); do rm -f bin/$$t; done
	@$(MAKE) --no-print-directory clean+
	@$(MAKE) -j --no-print-directory install

ifeq ($(HAVE_INSTBIN_64),1)
	@printf "$(LOGFORMAT_INST)" copy "$(INSTBIN)/* to $(INSTBIN_64)"
	@for f in $(BIN_FILES); do [[ -f $(INSTBIN)/$$f ]] \
		&& cp -p $(INSTBIN)/$$f $(INSTBIN_64); done; true
	@for f in $(WDF_LINKS); do ln -f $(INSTBIN_64)/wdf $(INSTBIN_64)/$$f; done
endif

else
	@$(MAKE) --no-print-directory clean+ install
endif

#
#--------------------------

.PHONY : new
new:
	@printf "$(LOGFORMAT)" enable new "-> define -DNEW_FEATURES"
	@rm -f *.o $(ALL_TOOLS_X)
	@echo "-DNEW_FEATURES" >>$(MODE_FILE)
	@sort $(MODE_FILE) | uniq > $(MODE_FILE).tmp
# 2 steps to bypass a cygwin mv failure
	@cp $(MODE_FILE).tmp $(MODE_FILE)
	@rm -f $(MODE_FILE).tmp

#
#--------------------------

.PHONY : old
old:
	@printf "$(LOGFORMAT)" enable old "-> define -DOLD_FEATURES"
	@rm -f *.o $(ALL_TOOLS_X)
	@echo "-DOLD_FEATURES" >>$(MODE_FILE)
	@sort $(MODE_FILE) | uniq > $(MODE_FILE).tmp
# 2 steps to bypass a cygwin mv failure
	@cp $(MODE_FILE).tmp $(MODE_FILE)
	@rm -f $(MODE_FILE).tmp

#
#--------------------------

.PHONY : predef
predef:
	@gcc -E -dM none.c | sort

#
#--------------------------

.PHONY : $(SUB_PROJECTS)
$(SUB_PROJECTS):
	@printf "$(LOGFORMAT)" make "$@" ""
	@$(MAKE) -C "$@"

#
#--------------------------

templates.sed: Makefile
	@printf "$(LOGFORMAT)" create templates.sed ""
	@printf '%s\n' \
		'/^~/ d;' \
		's|@.@@@|$(VERSION_NUM)|g;' \
		's|@@@@-@@-@@|$(DATE)|g;' \
		's|@@:@@:@@|$(TIME)|g;' \
		's|@@AUTHOR@@|$(AUTHOR)|g;' \
		's|@@SYSTEM@@|$(SYSTEM)|g;' \
		's|@@SYSTEM2@@|$(SYSTEM2)|g;' \
		's|@@TOOLSET-SHORT@@|$(TOOLSET_SHORT)|g;' \
		's|@@TOOLSET-LONG@@|$(TOOLSET_LONG)|g;' \
		's|@@WIT-SHORT@@|$(WIT_SHORT)|g;' \
		's|@@WIT-LONG@@|$(WIT_LONG)|g;' \
		's|@@WWT-SHORT@@|$(WWT_SHORT)|g;' \
		's|@@WWT-LONG@@|$(WWT_LONG)|g;' \
		's|@@WDF-SHORT@@|$(WDF_SHORT)|g;' \
		's|@@WDF-LONG@@|$(WDF_LONG)|g;' \
		's|@@WFUSE-SHORT@@|$(WFUSE_SHORT)|g;' \
		's|@@WFUSE-LONG@@|$(WFUSE_LONG)|g;' \
		's|@@VERSION@@|$(VERSION)|g;' \
		's|@@VERSION-NUM@@|$(VERSION_NUM)|g;' \
		's|@@BETA-VERSION@@|$(BETA_VERSION)|g;' \
		's|@@BETA-SUFFIX@@|$(BETA_SUFFIX)|g;' \
		's|@@REV@@|$(REVISION)|g;' \
		's|@@REV-NUM@@|$(REVISION_NUM)|g;' \
		's|@@REV-NEXT@@|$(REVISION_NEXT)|g;' \
		's|@@BINTIME@@|$(BINTIME)|g;' \
		's|@@DATE@@|$(DATE)|g;' \
		's|@@TIME@@|$(TIME)|g;' \
		's|@@INSTALL-PATH@@|$(INSTALL_PATH)|g;' \
		's|@@SHARE-PATH@@|$(SHARE_PATH)|g;' \
		's|@@BIN-FILES@@|$(BIN_FILES)|g;' \
		's|@@SHARE-FILES@@|$(SHARE_FILES)|g;' \
		's|@@WDF-LINKS@@|$(WDF_LINKS)|g;' \
		's|@@CYGWIN-DIR@@|$(CYGWIN_DIR)|g;' \
		's|@@CYGWIN-TOOLS@@|$(CYGWIN_TOOLS)|g;' \
		's|@@WIN-INSTALL-PATH@@|$(WIN_INSTALL_PATH)|g;' \
		's|@@LANGUAGES@@|$(LANGUAGES)|g;' \
		's|@@DISTRIB-PATH@@|$(DISTRIB_PATH)|g;' \
		's|@@DISTRIB-I386@@|$(DISTRIB_I386)|g;' \
		's|@@DISTRIB-X86_64@@|$(DISTRIB_X86_64)|g;' \
		's|@@DISTRIB-MAC@@|$(DISTRIB_MAC)|g;' \
		's|@@DISTRIB-CYGWIN32@@|$(DISTRIB_CYGWIN64)|g;' \
		's|@@DISTRIB-CYGWIN64@@|$(DISTRIB_CYGWIN32)|g;' \
		's|@@URI-FILE@@|$(URI_FILE)|g;' \
		's|@@URI-REPOS@@|$(URI_REPOS)|g;' \
		's|@@URI-VIEWVC@@|$(URI_VIEWVC)|g;' \
		's|@@URI-HOME@@|$(URI_HOME)|g;' \
		's|@@URI-DOWNLOAD@@|$(URI_DOWNLOAD)|g;' \
		's|@@URI-WDF@@|$(URI_WDF)|g;' \
		's|@@URI-CISO@@|$(URI_CISO)|g;' \
		's|@@URI-QTWITGUI@@|$(URI_QTWITGUI)|g;' \
		's|@@URI-WIIBAFU@@|$(URI_WIIBAFU)|g;' \
		's|@@URI-MACWITGUI@@|$(URI_MACWITGUI)|g;' \
		's|@@URI-GBATEMP@@|$(URI_GBATEMP)|g;' \
		's|@@URI-DOWNLOAD-I386@@|$(URI_DOWNLOAD_I386)|g;' \
		's|@@URI-DOWNLOAD-X86_64@@|$(URI_DOWNLOAD_X86_64)|g;' \
		's|@@URI-DOWNLOAD-MAC@@|$(URI_DOWNLOAD_MAC)|g;' \
		's|@@URI-DOWNLOAD-CYGWIN@@|$(URI_DOWNLOAD_CYGWIN)|g;' \
		's|@@URI-TITLES@@|$(URI_TITLES)|g;' \
		>templates.sed

#
#--------------------------

.PHONY : test
test:
	@printf "$(LOGFORMAT)" enable test "-> define -DTEST"
	@rm -f *.o $(ALL_TOOLS_X)
	@echo "-DTEST" >>$(MODE_FILE)
	@sort $(MODE_FILE) | uniq > $(MODE_FILE).tmp
# 2 steps to bypass a cygwin mv failure
	@cp $(MODE_FILE).tmp $(MODE_FILE)
	@rm -f $(MODE_FILE).tmp

#
#--------------------------

.PHONY : test-trace
test-trace:
	@printf "$(LOGFORMAT)" enable testtrace "-> define -DTESTTRACE"
	@rm -f *.o $(ALL_TOOLS_X)
	@echo "-DTESTTRACE" >>$(MODE_FILE)
	@sort $(MODE_FILE) | uniq > $(MODE_FILE).tmp
# 2 steps to bypass a cygwin mv failure
	@cp $(MODE_FILE).tmp $(MODE_FILE)
	@rm -f $(MODE_FILE).tmp

#
#--------------------------

.PHONY : titles
titles: wit load-titles.sh gen-titles

.PHONY : gen-titles
gen-titles:
	@chmod a+x load-titles.sh
	@./load-titles.sh --make

#
#--------------------------

.PHONY : tools
tools: $(ALL_TOOLS)

#
#--------------------------

.PHONY : up
up: clean++ do_up ch+

.PHONY : do_up
do_up: clean
	@printf "$(LOGFORMAT)" svn update
	@svn update

#
#--------------------------

.PHONY : wait
wait:
	@printf "$(LOGFORMAT)" enable wait "-> define -DWAIT_ENABLED"
	@rm -f *.o $(ALL_TOOLS_X)
	@echo "-DWAIT_ENABLED" >>$(MODE_FILE)
	@sort $(MODE_FILE) | uniq > $(MODE_FILE).tmp
# 2 steps to bypass a cygwin mv failure
	@cp $(MODE_FILE).tmp $(MODE_FILE)
	@rm -f $(MODE_FILE).tmp

#
#--------------------------

%.wbfs: wwt
	@printf "$(LOGFORMAT)" create "$@" "$(WBFS_SIZE)G, add smallest $(WBFS_COUNT) ISOs"
	@rm -f $@
	@./wwt format --force --inode --size $(WBFS_SIZE)- "$@"
	@stat --format="%b|%n" pool/wdf/*.wdf \
		| sort -n \
		| awk '-F|' '{print $$2}' \
		| head -n$(WBFS_COUNT) \
		| ./wwt -A -p "$@" add @- -v

#
#--------------------------

.PHONY : format-wbfs
format-wbfs:
	@printf "$(LOGFORMAT)" create "$(WBFS_FILES)," "size=$(WBFS_SIZE)G"
	@rm -f $(WBFS_FILES)
	@s=512; \
	    for w in $(WBFS_FILES); \
		do ./wwt format -qfs $(WBFS_SIZE)- --sector-size=$$s --inode "$$w"; \
		let s*=2; \
	    done

#
#--------------------------

.PHONY : wbfs
wbfs: wwt format-wbfs gen-wbfs

.PHONY : gen-wbfs
gen-wbfs: format-wbfs
	@printf "$(LOGFORMAT)" charge "$(WBFS_FILE)" ""
	@stat --format="%b|%n" pool/wdf/*.wdf \
		| sort -n \
		| awk '-F|' '{print $$2}' \
		| head -n$(WBFS_COUNT) \
		| ./wwt -A -p @<(ls $(WBFS_FILE)) add @-
	@echo
	@./wwt f -l $(WBFS_FILES)
	@./wwt lf $(WBFS_FILE) --mtime

#
#--------------------------

.PHONY : wbfs+
wbfs+: wwt format-wbfs gen-wbfs+

.PHONY : gen-wbfs+
gen-wbfs+: format-wbfs
	@printf "$(LOGFORMAT)" charge "$(WBFS_FILES)" ""
	@stat --format="%b|%n" pool/iso/*.iso \
		| sort -n \
		| awk '-F|' '{print $$2}' \
		| head -n$(WBFS_COUNT) \
		| ./wwt -A -p @<(ls $(WBFS_FILES)) add @-
	@echo
	@./wwt f -l $(WBFS_FILES)
	@./wwt lf $(WBFS_FILES)
	@echo "WBFS: this is not a wbfs file" >no.wbfs

#
#--------------------------

.PHONY : xwbfs
xwbfs: wwt
	@printf "$(LOGFORMAT)" create x.wbfs "1TB, sec-size=2048 and then with sec-size=512"
	@./wwt init -qfs1t x.wbfs --inode --sector-size=2048
	@sleep 2
	@./wwt init -qfs1t x.wbfs --inode --sector-size=512

#--------------------------

.PHONY : bad-wbfs
bad-wbfs: wwt a.wbfs
	@printf "$(LOGFORMAT)" edit "a.wbfs" "(create errors)"
#	@./wwt edit  -p a.wbfs -f rm=10 act=0
	@./wwt edit  -p a.wbfs -f free=4 act=0-1 R64P01=10:1
#	@./wwt check -p a.wbfs -vv

#
#--------------------------

.PHONY : wdf-links
wdf-links:
	@printf "$(LOGFORMAT)" link "$(WDF_TEST_LINKS) -> wdf" ""
	@for l in $(WDF_TEST_LINKS); do rm -f $$l; ln -s wdf $$l; done

#
###############################################################################
# help rule

.PHONY : help
help:
	@echo  ""
	@echo  "$(DATE) $(TIME) - $(VERSION) - svn r$(REVISION):$(REVISION_NEXT)"
	@echo  ""
	@echo  " make		:= make all"
	@echo  " make all	make all tools and install scripts"
	@echo  " make all+	:= make clean+ all distrib"
	@echo  " make all++	:= make clean+ all titles distrib"
	@echo  " make _tool_	compile only the named '_tool_' (wit,wwt,...)"
	@echo  " make tools	make all tools"
	@echo  ""
	@echo  " make clean	remove all output files"
	@echo  " make clean+	make clean & rm test_files & rm template_output"
	@echo  ""
	@echo  " make debug	enable '-DDEBUG'"
	@echo  " make test	enable '-DTEST'"
	@echo  " make new	enable '-DNEW_FEATURES'"
	@echo  " make old	enable '-DOLD_FEATURES'"
	@echo  " make wait	enable '-DWAIT'"
	@echo  " make testtrace	enable '-DTESTTRACE'"
	@echo  " make flags	print DEFINES, CFLAGS and LDFLAGS"
	@echo  ""
	@echo  " make doc	generate doc files from their templates"
	@echo  " make distrib	make all & build $(DISTRIB_PATH)"
	@echo  " make titles	get titles from $(URI_TITLES)"
	@echo  " make install	make all & copy tools to $(INSTALL_PATH)"
	@echo  " make install+	:= make clean+ install"
	@echo  ""
	@echo  " make chmod	change mode 775/644 for known dirs and files"
	@echo  " make chown	change owner of all dirs+files to owner of ."
	@echo  " make chgrp	change group of all dirs+files to group of ."
	@echo  " make ch+	:= make chmod chown chgrp"
	@echo  ""
	@echo  " make %.wbfs	gen %.wbfs, $(WBFS_SIZE)G, add smallest $(WBFS_COUNT) ISOs"
	@echo  " make wbfs	gen $(WBFS_FILE), $(WBFS_SIZE)G, add smallest $(WBFS_COUNT) ISOs"
	@echo  " make wbfs+	gen $(WBFS_FILES), $(WBFS_SIZE)G, add smallest $(WBFS_COUNT) ISOs"
	@echo  ""
	@echo  " make help	print this help"
	@echo  ""
	@echo  "-------------------------------------------------------------------------------"
	@awk -F: '/^.PHONY/ { gsub(/[[:blank:]]*/,"",$$2); print $$2}' $(MYMAKE) \
		| sort | pr -5T -w80
	@echo  "-------------------------------------------------------------------------------"

#
###############################################################################
# local definitions

-include makefiles-local/Makefile.local.$(SYSTEM)
-include Makefile.user

