# Plugin name
PLUGIN_NAME ?= httpproxytest

# You will need kismet git-master sources if not compiling in the main tree
KIS_SRC_DIR ?= /usr/src/kismet
KIS_INC_DIR ?= $(KIS_SRC_DIR)

-include $(KIS_SRC_DIR)/Makefile.inc

BLDHOME	= .
top_builddir = $(BLDHOME)

# Set sane values if we don't have the base config
INSTALL ?= /usr/bin/install
plugindir ?= $(shell pkg-config --variable=plugindir kismet)
bindir ?= $(shell pkg-config --variable=exec_prefix kismet)

ifeq ("$(plugindir)", "")
	plugindir := "/usr/local/lib/kismet/"
	plugingeneric := "1"
endif

ifeq ("$(bindir)", "")
	bindir := "/usr/local/bin/"
endif

INSTUSR ?= root
INSTGRP ?= root

all:
	@-echo "Run 'make install' to install the plugin and helper."

install:
ifeq ("$(plugingeneric)", "1")
	echo "No kismet install found in pkgconfig, assuming /usr/local"
endif

	mkdir -p $(DESTDIR)/$(plugindir)/$(PLUGIN_NAME)
	$(INSTALL) -o $(INSTUSR) -g $(INSTGRP) -m 444 manifest.conf $(DESTDIR)/$(plugindir)/$(PLUGIN_NAME)/manifest.conf
	$(INSTALL) -o $(INSTUSR) -g $(INSTGRP) -m 555 kismet_proxytest $(bindir)/kismet_proxytest;


userinstall:
	mkdir -p ${HOME}/.kismet/plugins/$(PLUGIN_NAME)
	$(INSTALL) manifest.conf ${HOME}/.kismet/plugins/$(PLUGIN_NAME)/manifest.conf
	$(INSTALL) -o $(INSTUSR) -g $(INSTGRP) -m 555 kismet_proxytest $(bindir)/kismet_proxytest;

