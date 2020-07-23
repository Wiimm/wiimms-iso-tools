
#------------------------------------------------------------------------------
# standard object files

DCLIB_O = dclib-basics.o dclib-color.o dclib-numeric.o \
		dclib-file.o dclib-option.o \
		dclib-utf8.o dclib-vector.o dclib-tables.o dclib-ui.o \
		dclib-xdump.o dclib-debug.o
DCLIB_O1 := lib-dol.o lib-bmg.o
DCLIB_O2 := lib-wiimmfi.o lib-gamespy.o lib-natneg.o

DCLIB_PRIVATE := dclib-terminal* dclib-command* lib-wiimmfi* lib-gamespy* lib-natneg*

#------------------------------------------------------------------------------
# if DCLIB_MYSQL not set: enable it if file 'mysql.h' is found

ifeq ($(origin DCLIB_MYSQL), undefined)
  DCLIB_MYSQL := $(shell [[ -f /usr/include/mysql/mysql.h ]]; echo $$((!$$?)))
endif

#------------------------------------------------------------------------------
# if DCLIB_TERMINAL not set: disable it

ifeq ($(origin DCLIB_TERMINAL), undefined)
  DCLIB_TERMINAL := 0
endif

#------------------------------------------------------------------------------
# if DCLIB_NETWORK not set: enable it if DCLIB_MYSQL is also set

ifeq ($(origin DCLIB_NETWORK), undefined)
  DCLIB_NETWORK := $(DCLIB_MYSQL)
endif

#------------------------------------------------------------------------------
# some more setting

ifeq ($(origin DCLIB_HAVE_USABLE_SIZE), undefined)
    DCLIB_HAVE_USABLE_SIZE := $(shell grep -sqw malloc_usable_size /usr/include/malloc.h; echo $$((!$$?)))
endif

#------------------------------------------------------------------------------
# add libraries and guardian macros

ifeq ($(DCLIB_NETWORK),1)
    DCLIB_O += dclib-network.o
    DCLIB_FLAGS += -DDCLIB_NETWORK=1
else
    DCLIB_MYSQL := 0
endif

ifeq ($(DCLIB_MYSQL),1)
    DCLIB_O += dclib-mysql.o
    DCLIB_FLAGS += -DDCLIB_MYSQL=1
endif

ifeq ($(DCLIB_TERMINAL),1)
    DCLIB_O += dclib-terminal.o dclib-command.o
    DCLIB_FLAGS += -DDCLIB_TERMINAL=1
endif

ifeq ($(DCLIB_HAVE_USABLE_SIZE),1)
    DCLIB_FLAGS += -DDCLIB_HAVE_USABLE_SIZE=1
endif

#------------------------------------------------------------------------------
