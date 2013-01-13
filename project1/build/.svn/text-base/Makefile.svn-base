UNAMEALL=$(shell uname -a)

ifeq ($(findstring csic.umd.edu,$(UNAMEALL)),csic.umd.edu)
#linuxlab has old qemu, 0.10.5ish.
	BIOSLOCATION:=-L /usr/local/qemu-0.10.5/share/qemu
	QEMU=/usr/local/bin/qemu -no-kqemu
	QEMU_DEBUG_OPT=-S -s -p $(RANDOM_PORT)
	QEMU_VGA_OPT=-vga std 
	include Makefile.linux
else
# OSTYPE ?= linux
	OSTYPE ?= $(shell uname -s | tr A-Z a-z)
	MACHTYPE ?= $(shell uname -m)
	ifeq ($(MACHTYPE),x86_64)
		ifeq ($(wildcard Makefile.$(OSTYPE).$(MACHTYPE)),Makefile.linux.x86_64)
			include Makefile.$(OSTYPE).$(MACHTYPE)
        else
		    include Makefile.$(OSTYPE)
		endif
	else
	  	include Makefile.$(OSTYPE)
	endif
endif
