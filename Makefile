# Makefile -- Key_LED
# Author:	Corey Davyduke
# Created:	2012-12-07
# Modified:	2012-12-07

RM=rm -f
INSTALL=cp

# 68HC11 compiler and tools
DEVC_PREFIX=m6811-elf-
CC=$(DEVC_PREFIX)gcc
SIZE=$(DEVC_PREFIX)size
OBJCOPY=$(DEVC_PREFIX)objcopy

# Compile with debugging info (-g), with size optimization (-Os)
# and 16-bit integers (-mshort)
CFLAGS=-g -Os -mshort

# Must pass -mshort to the linker, use the m68hc11elfb linker script
# which includes 'memory.x'.
LDFLAGS=$(CFLAGS) -Wl,-m,m68hc11elfb

# Flags for objcopy tool
OBJCOPY_FLAGS=--only-section=.text \
              --only-section=.rodata \
	      --only-section=.vectors \
	      --only-section=.data

# List of C sources
CSRCS=Key_LED.c

# List of objects
OBJS=$(CSRCS:.c=.o)

PROGS=Key_LED.elf

all::	$(PROGS) Key_LED.s19

Key_LED.elf:	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

Key_LED.s19:    Key_LED.elf
	$(OBJCOPY) $(OBJCOPY_FLAGS) --output-target=srec $< $@

install::	$(PROGS)

clean::
	$(RM) *.elf *.o *.s19
