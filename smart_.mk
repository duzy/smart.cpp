# -*- mode: Makefile:gnu -*-
#	Copyright(c) 2009, by Zhan Xin-ming, duzy@duzy.info
#	

INCLUDES = -I$(BOOST_DIR)

#CXXFLAGS = -std=gnu++0x -g -ggdb -DBOOST_SPIRIT_DEBUG -DBOOST_SPIRIT_DEBUG_XML $(INCLUDES)
#CXXFLAGS = -std=gnu++0x -g -ggdb -DBOOST_SPIRIT_DEBUG_XML $(INCLUDES)
CXXFLAGS = -std=gnu++0x -g -ggdb $(INCLUDES)
#CXXFLAGS = -std=gnu++0x -O3 -DNDEBUG $(INCLUDES)
#CXXFLAGS = $(INCLUDES)
#CXXFLAGS = -ftemplate-depth-128 -O3 -finline-functions -DNDEBUG

LOADLIBRES = -L$(SM_OUT_DIR)/lib -L$(BOOST_LIB_DIR)
#LDLIBS = -lsmart
LDLIBS = -lpthread \
  -l$(call BOOST_LIB,filesystem) \
  -l$(call BOOST_LIB,system) \
  -l$(call BOOST_LIB,program_options)
##################################################

SOURCES = $(wildcard src/*.cpp)
OBJECT_PAT = $(SM_OUT_DIR)/objs/%.o
OBJECTS = $(SOURCES:%.cpp=$(OBJECT_PAT))
DEPEND_PAT = $(SM_OUT_DIR)/deps/%.d
DEPENDS = $(SOURCES:%.cpp=$(DEPEND_PAT))

UNITS = $(wildcard t/*.t)
UNIT_PAT = $(SM_OUT_DIR)/objs/%.o
UNIT_OBJECTS = $(UNITS:%.t=$(UNIT_PAT))
TEST_PAT = $(SM_OUT_DIR)/%.test
TESTS = $(UNITS:%.t=$(TEST_PAT))
TEST_DEPEND_PAT = $(SM_OUT_DIR)/deps/%.d
TEST_DEPENDS = $(UNITS:%.t=$(TEST_DEPEND_PAT))

##################################################
PREPARE_OUTPUT_DIR = @[ -d `dirname $@` ] || mkdir -pv `dirname $@`

##################################################

SMART = smart
SMART.LIB = $(SM_OUT_DIR)/lib/libsmart.a

PHONY = all
all: $(SMART)

PHONY += test
test: $(TESTS)
	@#for T in $(TESTS); do echo "unit: $$T"; $$T; done
	@for T in $(TESTS); do $$T; done

$(TESTS): $(SMART.LIB)
test-%: $(SM_OUT_DIR)/t/%.test
	./$<

$(SMART):$(OBJECTS)
	$(PREPARE_OUTPUT_DIR)
	$(LINK.cc) -o $@ $^ $(LOADLIBRES) $(LDLIBS)
$(SMART.LIB):$(OBJECTS)
	$(PREPARE_OUTPUT_DIR)
	$(AR) crs $@ $^
$(OBJECTS):$(OBJECT_PAT):%.cpp
	$(PREPARE_OUTPUT_DIR)
	$(COMPILE.cc) -o $@ $<
$(DEPENDS):$(DEPEND_PAT):%.cpp
	$(PREPARE_OUTPUT_DIR)
	$(CXX) -MM -MT $(SM_OUT_DIR)/objs/$*.o -MF $@ $<
$(UNIT_OBJECTS):$(UNIT_PAT):%.t
	$(PREPARE_OUTPUT_DIR)
	$(COMPILE.cc) -xc++ -o $@ $<
$(TESTS):$(TEST_PAT):$(SM_OUT_DIR)/objs/%.o t/boost_test_stuff.o
	$(PREPARE_OUTPUT_DIR)
	$(LINK.cc) -o $@ $^ $(LOADLIBRES) $(LDLIBS) -lsmart
$(TEST_DEPENDS):$(TEST_DEPEND_PAT):%.t
	$(PREPARE_OUTPUT_DIR)
	$(CXX) -xc++ -MM -MT $(SM_OUT_DIR)/objs/$*.o -MF $@ $<

clean:
	$(RM) $(SMART) $(SMART.LIB) $(OBJECTS) $(DEPENDS)

include $(DEPENDS)
include $(TEST_DEPENDS)