SPIDERQ_MAJOR=0
SPIDERQ_MINOR=1
DYLIBSUFFIX=so
DYLIBMAJORSUFFIX=$(SPIDERQ_MAJOR)#动态库的前缀
DYLIBMINORSUFFIX=$(SPIDERQ_MAJOR).$(SPIDERQ_MINOR)

SOURCES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SOURCES))
DYLIBNAMES=$(patsubst %.cpp,%.(DYLIBSUFFIX),$(SOURCES))#动态库的名字xxx.so


CXX:=g++
OPTIMIZATION?=-O2 -DNDEBUG
WARNINGS=-Wall
CXXFLAGS=-I../src 
REAL_CXXFLAGS=$(OPTIMIZATION) fPIC $(CXXFLAGS) $(WARNINGS) $(DEBUG) $(PROF)


all:$(DYLIBNAMES)

%.so:%.o 
	$(CXX) -shared - W1,-soname,S@.$(DYLIBMINORSUFFIX) $(LDFLAGS) $< -o $@

.cpp.o:
	$(CXX) -c $(REAL_CXXFLAGS) $<

clean:
	rm -rf $(DYLIBNAMES) $(OBJS)

#install, 就是把动态库复制到/etc/spider目录里面
PREFIX:=/etc/spider
MODULEDIR=$(PREFIX)/modules
ifeq ($(uname_S), SunOS)
	INSTALL?= cp -r
endif

INSTALL?= cp -af

install: $(DYLIBNAMES)
	@mkdir -p $(MODULEDIR)
	-$(INSTALL) $^ $(MODULEDIR)

debug:
	$(MAKE) OPTIMIZATION="" DEBUG="-g -ggdb -g3"

