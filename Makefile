#
# Makefile for the binary module of the GraphLCD Kodi addon
#

PRGNAME = resources/lib/graphlcd.so

PYTHONVERSION=2
OBJS = graphlcd.o
INCLUDES = $(shell pkg-config --cflags python$(PYTHONVERSION))
LIBS = -lglcdgraphics -lglcddrivers -lglcdskin -lstdc++
CXXFLAGS ?= -Wall
ADDONDIR = /usr/share/kodi/addons/script.service.graphlcd

.PHONY: all
all: $(PRGNAME)

%.o: %.c
	$(CXX) $(CXXEXTRA) $(CXXFLAGS) -c -fPIC $(DEFINES) $(INCLUDES) $<

$(PRGNAME): $(OBJS)
	@mkdir -p $(dir $(PRGNAME))
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared -rdynamic $(OBJS) $(LIBS) $(LIBDIRS) -o $(PRGNAME)

install: clean-python $(PRGNAME)
	install -d $(DESTDIR)$(ADDONDIR)
	install -m 644 addon.py $(DESTDIR)$(ADDONDIR)
	install -m 644 addon.xml $(DESTDIR)$(ADDONDIR)
	install -m 644 icon.png $(DESTDIR)$(ADDONDIR)
	cp -vr resources $(DESTDIR)$(ADDONDIR)

clean-python:
	@rm -f resources/lib/*.pyo
	@rm -rf resources/lib/__pycache__

clean: clean-python
	@rm -f $(OBJS) $(PRGNAME) *~
