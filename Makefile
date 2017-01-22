#
# Makefile for the binary module of the GraphLCD Kodi addon
#

PRGNAME = resources/lib/graphlcd.so

OBJS = graphlcd.o
INCLUDES = $(shell pkg-config --cflags python2)
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

install: $(PRGNAME)
	install -d $(DESTDIR)$(ADDONDIR)
	install -m 644 addon.py $(DESTDIR)$(ADDONDIR)
	install -m 644 addon.xml $(DESTDIR)$(ADDONDIR)
	install -m 644 icon.png $(DESTDIR)$(ADDONDIR)
	rm -f resources/lib/*.pyo
	cp -vr resources $(DESTDIR)$(ADDONDIR)

clean:
	@-rm -f $(OBJS) $(PRGNAME) resources/lib/*.pyo *~
