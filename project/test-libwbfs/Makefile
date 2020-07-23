#----------------------------------------------------------------
# make manual: http://www.gnu.org/software/make/manual/make.html
#----------------------------------------------------------------

#-------------------------------------------------------------------------------
# global settings

SHELL			= /bin/bash

PRE		?= 
CC		= $(PRE)gcc
CPP		= $(PRE)g++
STRIP		= $(PRE)strip

#-------------------------------------------------------------------------------
# files

RM_FILES	= *.{o,d,tmp,bak,exe}

#-------------------------------------------------------------------------------
# tools

TOOLS		:= test-libwbfs
RM_FILES	+= $(TOOLS)

#-------------------------------------------------------------------------------
# object files

# objects of tools
TOOLS_O	:= $(patsubst %,%.o,$(TOOLS))

# other objects
LIBWBFS_O	:= file-formats.o libwbfs.o wiidisc.o rijndael.o

# object groups
C_OBJECTS	:= $(sort $(TOOLS_O) $(LIBWBFS_O))

# all objects + sources
ALL_OBJECTS	:= $(sort $(LIBWBFS_O))
ALL_SOURCES	:= $(patsubst %.o,%.c,$(ALL_OBJECTS))

#-------------------------------------------------------------------------------

VPATH		+= ../src/libwbfs

DEFINES		=  -DLARGE_FILES -D_FILE_OFFSET_BITS=64 $(XDEF)
DEFINES		:= $(strip $(DEFINES))

CFLAGS		=  -fomit-frame-pointer -fno-strict-aliasing
CFLAGS		+= -Wall -Wno-parentheses -Wno-unused-function
CFLAGS		+= -O3 -I../src/libwbfs -I.
CFLAGS		+= $(XFLAGS)
CFLAGS		:= $(strip $(CFLAGS))

DEPFLAGS	+= -MMD

LDFLAGS		+= -static-libgcc
#LDFLAGS	+= -static
LDFLAGS		:= $(strip $(LDFLAGS))

LIBS		+= $(XLIBS)

#
###############################################################################
# default rule

default_rule: all
#	@echo "HINT: try 'make help'"

# include this behind the default rule
-include $(ALL_SOURCES:.c=.d)

#
###############################################################################
# general rules

$(TOOLS): %: %.o $(ALL_OBJECTS) Makefile
	@echo "***    tool $@ $(MODE)"
	@$(CC) $(CFLAGS) $(DEFINES) $(LDFLAGS) $@.o $(ALL_OBJECTS) $(LIBS) -o $@
	@if test -f $@.exe; then $(STRIP) $@.exe; else $(STRIP) $@; fi

#--------------------------

$(C_OBJECTS): %.o: %.c Makefile
	@echo "***  object $@ $(MODE)"
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(DEFINES) -c $< -o $@

#--------------------------

$(ASM_OBJECTS): %.o: %.S Makefile
	@echo "***     asm $@ $(MODE)"
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(DEFINES) -c $< -o $@

#
###############################################################################
# specific rules in alphabetic order

.PHONY : all
all: $(TOOLS)

.PHONY : all+
all+: clean all

#
#--------------------------

.PHONY : clean
clean:
	@echo "***      rm output files"
	@rm -f $(RM_FILES)

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

#
#--------------------------

.PHONY : tools
tools: $(ALL_TOOLS)

