#
# Makefile
#

BIN = memo-panel
DESTDIR = /usr
PREFIX = /local

LVGL_DIR = lvgl

CC = gcc
CXX = g++
CFLAGS = -Wall -Wshadow -Wundef
CFLAGS += -O3 -g3 -I./ -I./deps -I$(LVGL_DIR)

# directory for local libs
LDFLAGS = -L$(DESTDIR)$(PREFIX)/lib
LIBS += -lstdc++ -lm -lcurl

ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
	CFLAGS += -Wuninitialized
else
	CFLAGS += -Wmaybe-uninitialized -Wno-format
endif

ifeq ($(OS),Windows_NT)
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		CFLAGS += -I/opt/local/include
		LIBS += -L/opt/local/lib -lSDL2 -lpthread
	else
		LIBS += -lbsd -lpthread
	endif
endif

VPATH =

$(info LDFLAGS ="$(LDFLAGS)")

include $(LVGL_DIR)/lvgl.mk
ifeq ($(LVGL),7)
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk
include $(LVGL_DIR)/lv_lib_png/lv_lib_png.mk
endif

CFLAGS += -DLVGL=$(LVGL)

# folder for object files
OBJDIR = ./obj

CSRCS += $(wildcard *.c) $(wildcard assets/*.c) $(wildcard deps/*.c)
CXXSRCS += $(wildcard *.cpp) $(wildcard deps/*.cpp)

COBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(CSRCS)) $(patsubst %.cpp,$(OBJDIR)/%.o,$(CXXSRCS))

SRCS = $(CSRCS) $(CXXSRCS)
OBJS = $(COBJS)

#.PHONY: clean

all: default

$(OBJDIR)/%.o: %.c memo-panel-sup.h lv_conf.h
	@mkdir -p $(dir $@)
	@echo "CC $< -> $@"
	@$(CC)  $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp memo-panel-sup.h lv_conf.h
	@mkdir -p $(dir $@)
	@echo "CXX $< -> $@"
	@$(CXX) -std=c++14 $(CFLAGS) -c $< -o $@

default: $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LDFLAGS) $(LIBS)

# nothing to do but will print info
nothing:
	$(info OBJS ="$(OBJS)")
	$(info SRCS ="$(SRCS)")
	$(info DONE)


clean:
	rm -f $(OBJS)

