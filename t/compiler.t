/**								-*- c++ -*-
 *
 */

//#define BOOST_SPIRIT_DEBUG
//#define BOOST_SPIRIT_DEBUG_XML
#include "../src/context.hpp"
#include "../src/compiler.hpp"
#include "../src/string_table.hpp"
#include "../src/exceptions.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/operations.hpp>

#include <fstream>
#include <iostream>

#ifdef WIN32
#  include <windows.h> //!< for sleep()
#  define sleep(n) ::Sleep(1000*n)
#endif

namespace fs = boost::filesystem;

static std::string read_all( const std::string & fn )
{
  std::ifstream ifs( fn.c_str(), std::ios::binary );
  ifs.seekg( 0, ifs.end );
  int sz( ifs.tellg() );
  BOOST_CHECK( 0 < sz );
  if ( 0 < sz ) {
    std::string s( sz, '\0' );
    ifs.seekg( 0, ifs.beg );
    ifs.read( &s[0], sz );
    return s;
  }
  return std::string();
}

BOOST_AUTO_TEST_CASE( assignments )
{
  smart::context ctx;
  {
    std::string code
      ( "##############\n"
	"N = n\n"
	"N$(N)N = nnn\n"
	"##############\n"
	"L1 = i1\\\n"
	"   i2\\\n"
	"i3   \\\n"
	"i4\n"
	"L2 := i1\\\n"
	"   i2\\\n"
	"i3   \\\n"
	"i4\n"
	"##############\n"
	"" );

    smart::compiler sm( ctx );
    sm.compile( code );
    BOOST_CHECK( ctx.stable()->size() == 10 );
    //BOOST_CHECK( ctx.stable()->get("NnN").ptr != NULL );
    BOOST_CHECK( ctx.stable()->get("N").ptr != NULL );
    BOOST_CHECK( ctx.stable()->get("L1").ptr != NULL );
    BOOST_CHECK( ctx.stable()->get("L2").ptr != NULL );
    BOOST_CHECK( ctx.stable()->get("i1").ptr != NULL );
    BOOST_CHECK( ctx.stable()->get("i2").ptr != NULL );
    BOOST_CHECK( ctx.stable()->get("i3   ").ptr != NULL );
    BOOST_CHECK( ctx.stable()->get("i4").ptr != NULL );
    //BOOST_CHECK( *ctx.stable()->get("NnN").ptr == "NnN" );
    if ( ctx.stable()->get("N").ptr )  BOOST_CHECK( *ctx.stable()->get("N").ptr == "N" );
    if ( ctx.stable()->get("L1").ptr ) BOOST_CHECK( *ctx.stable()->get("L1").ptr == "L1" );
    if ( ctx.stable()->get("L2").ptr ) BOOST_CHECK( *ctx.stable()->get("L2").ptr == "L2" );
    if ( ctx.stable()->get("i1").ptr ) BOOST_CHECK( *ctx.stable()->get("i1").ptr == "i1" );
    if ( ctx.stable()->get("i2").ptr ) BOOST_CHECK( *ctx.stable()->get("i2").ptr == "i2" );
    if ( ctx.stable()->get("i3   ").ptr ) BOOST_CHECK( *ctx.stable()->get("i3   ").ptr == "i3   " );
    if ( ctx.stable()->get("i4").ptr ) BOOST_CHECK( *ctx.stable()->get("i4").ptr == "i4" );

    smart::builtin::macro N( ctx.macro("N") );
    smart::builtin::macro NnN( ctx.macro("NnN") );
    BOOST_CHECK( N.name() == "N" );
    BOOST_CHECK( N.value() == "n" );
    BOOST_CHECK( NnN.name() == "NnN" );
    BOOST_CHECK( NnN.value() == "nnn" );
    BOOST_CHECK( N.origin() == smart::builtin::macro::origin_file );
    BOOST_CHECK( NnN.origin() == smart::builtin::macro::origin_file );
    BOOST_CHECK( N.flavor() == smart::builtin::macro::flavor_recursive );
    BOOST_CHECK( NnN.flavor() == smart::builtin::macro::flavor_recursive );

    smart::builtin::macro L1( ctx.macro("L1") );
    smart::builtin::macro L2( ctx.macro("L2") );
    std::clog<<"L1: "<<L1.value()<<std::endl;
    std::clog<<"L2: "<<L2.value()<<std::endl;
    BOOST_CHECK( L1.name() == "L1" );
    BOOST_CHECK( L1.value() == "i1 i2 i3    i4" );
    BOOST_CHECK( L1.origin() == smart::builtin::macro::origin_file );
    BOOST_CHECK( L1.flavor() == smart::builtin::macro::flavor_recursive );
    BOOST_CHECK( L2.name() == "L2" );
    BOOST_CHECK( L2.value() == "i1 i2 i3    i4" );
    BOOST_CHECK( L2.origin() == smart::builtin::macro::origin_file );
    BOOST_CHECK( L2.flavor() == smart::builtin::macro::flavor_simple );
  }
  {
    {
      smart::builtin::macro N2( ctx.macro("N2") );
      BOOST_CHECK( N2.flavor() == smart::builtin::macro::flavor_undefined );
      BOOST_CHECK( N2.origin() == smart::builtin::macro::origin_undefined );
      BOOST_CHECK( N2.name() == "N2" );
      BOOST_CHECK( N2.value() == "" );
    }

    std::string code
      ( "##############\n"
	"N ?= xxx\n"
	"N2 ?= n2\n"
	"##############\n"
	"v = v\n"
	"V = $(v)\n"
	"V1 = $(V)$(V)\n"
	"V2 := $(V)$(V)\n"
	"" );
    smart::compiler sm( ctx );
    sm.compile( code );
    {
      smart::builtin::macro m1( ctx.macro("N") );
      smart::builtin::macro m2( ctx.macro("N2") );
      BOOST_CHECK( m1.name() == "N" );
      BOOST_CHECK( m1.value() == "n" );
      BOOST_CHECK( m1.origin() == smart::builtin::macro::origin_file );
      BOOST_CHECK( m1.flavor() == smart::builtin::macro::flavor_recursive );
      BOOST_CHECK( m2.name() == "N2" );
      BOOST_CHECK( m2.value() == "n2" );
      BOOST_CHECK( m2.origin() == smart::builtin::macro::origin_file );
      BOOST_CHECK( m2.flavor() == smart::builtin::macro::flavor_recursive );
    }
    {
      smart::builtin::macro v( ctx.macro("v") );
      smart::builtin::macro m1( ctx.macro("V1") );
      smart::builtin::macro m2( ctx.macro("V2") );
      //std::clog<<m2.value()<<std::endl;
      BOOST_CHECK( v.name() == "v" );
      BOOST_CHECK( v.value() == "v" );
      BOOST_CHECK( m1.flavor() == smart::builtin::macro::flavor_recursive );
      BOOST_CHECK( m1.value() == "$(V)$(V)" );
      BOOST_CHECK( m2.flavor() == smart::builtin::macro::flavor_simple );
      BOOST_CHECK( m2.value() == "vv" );
    }
  }
  {
    std::string code
      ( "##############\n"
	"V1 += $(V)\n"
	"V2 += $(V)\n"
	"" );
    smart::compiler sm( ctx );
    sm.compile( code );
    {
      smart::builtin::macro m1( ctx.macro("V1") );
      smart::builtin::macro m2( ctx.macro("V2") );
      BOOST_CHECK( m1.flavor() == smart::builtin::macro::flavor_recursive );
      BOOST_CHECK( m1.value() == "$(V)$(V)$(V)" );
      BOOST_CHECK( m2.flavor() == smart::builtin::macro::flavor_simple );
      BOOST_CHECK( m2.value() == "vvv" );
    }
  }
  {
    std::string code
      ( "##############\n"
	"VV = a b c	d 	 e    f\n"
	"fun = $1;$2\n"
	"CALL := $(call fun,abc,def)\n"
	"PAT1 := $(VV:%=%.o)\n"
	"PAT2 := $(patsubst %,%.o,$(VV))\n"
	"VVV = a.cpp b.cpp 	 c.cpp  d.cpp  e.cpp	 	 f.cpp\n"
	"PAT3 := $(VVV:.cpp=.o)\n"
	"" );
    smart::compiler sm( ctx );
    sm.compile( code );
    {
      smart::builtin::macro m0( ctx.macro("fun") );
      smart::builtin::macro m1( ctx.macro("CALL") );
      smart::builtin::macro m2( ctx.macro("PAT1") );
      smart::builtin::macro m3( ctx.macro("PAT2") );
      //std::clog<<"m2: "<<m2.value()<<std::endl;
      BOOST_CHECK( m0.flavor() == smart::builtin::macro::flavor_recursive );
      BOOST_CHECK( m0.value() == "$1;$2" );
      BOOST_CHECK( m1.flavor() == smart::builtin::macro::flavor_simple );
      BOOST_CHECK( m1.value() == "abc;def" );
      BOOST_CHECK( m2.flavor() == smart::builtin::macro::flavor_simple );
      BOOST_CHECK( m2.value() == "a.o b.o c.o d.o e.o f.o" );
      BOOST_CHECK( m3.flavor() == smart::builtin::macro::flavor_simple );
      BOOST_CHECK( m3.value() == "a.o b.o c.o d.o e.o f.o" );

      smart::builtin::macro PAT3( ctx.macro("PAT3") );
      BOOST_CHECK( PAT3.value() == "a.o b.o c.o d.o e.o f.o" );
    }
  }
}//assignments

BOOST_AUTO_TEST_CASE( function_call )
{
  smart::context ctx;
  {
    std::string code
      ( "##############\n"
	"$(info informative text)\n"
	"$(info informative text	 2,)\n"
	"$(warning warning text)\n"
	"$(error error text)\n"
	"" );
    smart::compiler sm( ctx );
    sm.compile( code );
  }
  {// if, or, and
    std::string code
      ( "##############\n"
	"VV1 := \n"
	"RES1 := $(if $(VV1),vv,empty)\n"
	"VV2 := vv\n"
	"RES2 := $(if $(VV2),vv,empty)\n"
	"RES3 := $(or $(VV1),$(VV2),xx)\n"
	"RES4 := $(and $(VV1),$(VV2),xx)\n"
	"RES5 := $(and $(VV2),xx,yy)\n"
	"" );
    smart::compiler sm( ctx );
    sm.compile( code );
    {
      smart::builtin::macro m1( ctx.macro("RES1") );
      smart::builtin::macro m2( ctx.macro("RES2") );
      smart::builtin::macro m3( ctx.macro("RES3") );
      smart::builtin::macro m4( ctx.macro("RES4") );
      smart::builtin::macro m5( ctx.macro("RES5") );
      //std::clog<<"m1: "<<m1.value()<<std::endl;
      //std::clog<<"m2: "<<m2.value()<<std::endl;
      //std::clog<<"m5: "<<m5.value()<<std::endl;
      BOOST_CHECK( m1.flavor() == smart::builtin::macro::flavor_simple );
      BOOST_CHECK( m1.value() == "empty" );
      BOOST_CHECK( m2.value() == "vv" );
      BOOST_CHECK( m3.value() == "vv" );
      BOOST_CHECK( m4.value() == "" );
      BOOST_CHECK( m5.value() == "yy" );
    }
  }
  {// foreach
    std::string code
      ( "##############\n"
	"VV = a b c	d 	 e    f\n"
	"RES := $(foreach v,$(VV),$v.o)\n"
	"" );
    smart::compiler sm( ctx );
    sm.compile( code );
    {
      smart::builtin::macro m0( ctx.macro("VV") );
      smart::builtin::macro m1( ctx.macro("RES") );
      //std::clog<<"m1: "<<m1.value()<<std::endl;
      BOOST_CHECK( m1.flavor() == smart::builtin::macro::flavor_simple );
      BOOST_CHECK( m1.value() == "a.o b.o c.o d.o e.o f.o" );
    }
  }
}//function_call

BOOST_AUTO_TEST_CASE( make_rules )
{
  smart::context ctx;
  smart::compiler sm( ctx );

  std::string code
    ( "##############\n"
      "foobar: foo bar\n"
      "\t@echo command line 1\n"
      "\t@echo command line 2\n"
      "xx yy: x\n"
      "\t@echo command 1\n"
      "\t@echo command 2\n"
      "xx: y\n"
      "xx:\r"
      "\t@echo command 3\n"
      "" );
  sm.compile( code );

  smart::vm::type_string name( "foobar" );
  smart::builtin::target foobar( ctx.target(name) );
  //std::clog<<foobar<<std::endl;
  //std::clog<<foobar.rules().size()<<std::endl;
  BOOST_CHECK( foobar.object() == "foobar" );
  BOOST_CHECK( foobar.refcount() == 3/*2*/ ); //!< default goal
  BOOST_CHECK( foobar.rule().prerequisites().size() == 2 );
  BOOST_CHECK( foobar.rule().prerequisites()[0].object() == "foo" );
  BOOST_CHECK( foobar.rule().prerequisites()[1].object() == "bar" );
  BOOST_CHECK( foobar.rule().commands().size() == 2 );
  BOOST_CHECK( foobar.rule().commands()[0] == "@echo command line 1" );
  BOOST_CHECK( foobar.rule().commands()[1] == "@echo command line 2" );

  name = "foo";
  smart::builtin::target foo( ctx.target(name) );
  //std::clog<<foo.refcount()<<std::endl;
  BOOST_CHECK( foo.object() == "foo" );
  BOOST_CHECK( foo.refcount() == 3 );
  BOOST_CHECK( foo.rule().prerequisites().size() == 0 );
  BOOST_CHECK( foo.rule().commands().size() == 0 );
  BOOST_CHECK( foo.rule().empty() );

  name = "bar";
  smart::builtin::target bar( ctx.target(name) );
  //std::clog<<bar.refcount()<<std::endl;
  BOOST_CHECK( bar.object() == "bar" );
  BOOST_CHECK( bar.refcount() == 3 );
  BOOST_CHECK( bar.rule().prerequisites().size() == 0 );
  BOOST_CHECK( bar.rule().empty() );

  name = "xx";
  smart::builtin::target xx( ctx.target(name) );
  //std::clog<<xx.rule().commands().size()<<std::endl;
  //std::clog<<xx.rule().prerequisites().size()<<std::endl;
  BOOST_CHECK( xx.object() == "xx" );
  BOOST_CHECK( xx.refcount() == 2 );
  BOOST_CHECK( xx.rule().prerequisites().size() == 2 );
  BOOST_CHECK( xx.rule().prerequisites()[0].object() == "x" );
  BOOST_CHECK( xx.rule().prerequisites()[1].object() == "y" );
  BOOST_CHECK( xx.rule().commands().size() == 1 );
  BOOST_CHECK( xx.rule().commands()[0] == "@echo command 3" );
  BOOST_CHECK( xx.rule().empty() == false );

  name = "yy";
  smart::builtin::target yy( ctx.target(name) );
  //std::clog<<yy.rule().commands().size()<<std::endl;
  //std::clog<<yy.rule().prerequisites().size()<<std::endl;
  BOOST_CHECK( yy.object() == "yy" );
  BOOST_CHECK( yy.refcount() == 2 );
  BOOST_CHECK( yy.rule().prerequisites().size() == 1 );
  BOOST_CHECK( yy.rule().prerequisites()[0].object() == "x" );
  BOOST_CHECK( yy.rule().commands().size() == 2 );
  BOOST_CHECK( yy.rule().commands()[0] == "@echo command 1" );
  BOOST_CHECK( yy.rule().commands()[1] == "@echo command 2" );
  BOOST_CHECK( yy.rule().empty() == false );

  code =
    "##############\n"
    "foo: \n"
    "\t@echo command 1\n"
    "\t@echo command 2\n"
    "bar: x\n"
    "\t@echo command 1\n"
    "\t@echo command 2\n"
    "\t@echo command 3\n"
    "x: \n"
    "";
  sm.compile( code );
  BOOST_CHECK( foo.rule().commands().size() == 2 );
  BOOST_CHECK( bar.rule().commands().size() == 3 );

  if ( fs::exists("foobar") ) fs::remove("foobar");
  if ( fs::exists("foo") ) fs::remove("foo");
  if ( fs::exists("bar") ) fs::remove("bar");
  try {
    smart::builtin::target::update_result uc( foobar.update( ctx ) );
    //std::clog<<"updated: "<<uc<<std::endl;
    BOOST_CHECK( uc.count_updated == 0 );
    BOOST_CHECK( uc.count_executed == 3 );
    BOOST_CHECK( uc.count_newer == 0 );
  }
  catch( const smart::make_error & e ) {
    std::clog<<e.what()<<std::endl;
  }
}//make_rules

BOOST_AUTO_TEST_CASE( update_targets )
{
  smart::context ctx;
  smart::compiler sm( ctx );

  std::string code
    ( "##############\n"
      "foobar: foo bar\n"
      "\tcat $< > $@\n"
      "\tcat bar >> foobar\n"
      "foo: \t\n"
      "\techo foo > $@\n"
      "bar:\n"
      "\techo $@ > bar\n"
      //"\ttyn"
      "" );
  sm.compile( code );

  if ( fs::exists("foobar") ) fs::remove("foobar");
  if ( fs::exists("foo") ) fs::remove("foo");
  if ( fs::exists("bar") ) fs::remove("bar");
  BOOST_CHECK( !fs::exists("foobar") );
  BOOST_CHECK( !fs::exists("foo") );
  BOOST_CHECK( !fs::exists("bar") );

  smart::vm::type_string name( "foobar" );
  smart::builtin::target foobar( ctx.target(name) );
  try {
    smart::builtin::target::update_result uc( foobar.update( ctx ) );
    BOOST_CHECK( uc.count_updated == 3 );
    BOOST_CHECK( uc.count_executed == 3 );
    BOOST_CHECK( uc.count_newer == 2 );
  }
  catch( const smart::make_error & e ) { std::clog<<e.what()<<std::endl; }
  //catch( const smart::runtime_error & e ) { std::clog<<e.what()<<std::endl; }

  name = "foo"; smart::builtin::target foo( ctx.target(name) );
  name = "bar"; smart::builtin::target bar( ctx.target(name) );
  BOOST_CHECK( foobar.last_write_time() != 0 );
  BOOST_CHECK( foo.last_write_time() != 0 );
  BOOST_CHECK( bar.last_write_time() != 0 );
  
  BOOST_CHECK( fs::exists("foobar") );
  BOOST_CHECK( fs::exists("foo") );
  BOOST_CHECK( fs::exists("bar") );

  {
    std::string s;
    std::ifstream ifs("foobar");
    std::istream_iterator<std::string::value_type> it( ifs ), end;
    std::copy( it, end, std::back_inserter(s) );
    BOOST_CHECK( s == "foobar" );
  }
  {
    std::string s;
    std::ifstream ifs("foo");
    std::istream_iterator<std::string::value_type> it( ifs ), end;
    std::copy( it, end, std::back_inserter(s) );
    BOOST_CHECK( s == "foo" );
  }
  {
    std::string s;
    std::ifstream ifs("bar");
    std::istream_iterator<std::string::value_type> it( ifs ), end;
    std::copy( it, end, std::back_inserter(s) );
    BOOST_CHECK( s == "bar" );
  }

  std::time_t t1( foobar.last_write_time() );
  std::time_t t2( foo.last_write_time() );
  std::time_t t3( bar.last_write_time() );
  sleep( 0.5/*1*/ );
  try {
    smart::builtin::target::update_result uc2( foobar.update( ctx ) );
    BOOST_CHECK( uc2.count_updated == 0 );
    BOOST_CHECK( uc2.count_executed == 0 );
    BOOST_CHECK( uc2.count_newer == 0 );
  }
  catch( const smart::make_error & e ) { std::clog<<e.what()<<std::endl; }
  BOOST_CHECK( t1 == foobar.last_write_time() );
  BOOST_CHECK( t2 == foo.last_write_time() );
  BOOST_CHECK( t3 == bar.last_write_time() );

  fs::remove("foobar");
  fs::remove("foo");
  fs::remove("bar");
}//update_targets

BOOST_AUTO_TEST_CASE( static_pattern_rule )
{
  smart::context sm;
  smart::compiler smc( sm );

  std::string code
    ( "############################\n"
      "foo.o bar.o foobar.o : %.o : %.cpp\n"
      "\t@echo $<, $@ > $@\n"
      "\n"
      "%.cpp:\n"
      "\t@echo source:$@ > $@\n"
      );
  //std::clog<<"static-pattern: "<<std::endl;
  smc.compile( code );
  
  smart::builtin::target foo_o( sm.target("foo.o") );
  smart::builtin::target bar_o( sm.target("bar.o") );
  smart::builtin::target foobar_o( sm.target("foobar.o") );
  BOOST_CHECK( foo_o.exists() == fs::exists("foo.o") );
  BOOST_CHECK( bar_o.exists() == fs::exists("bar.o") );
  BOOST_CHECK( foobar_o.exists() == fs::exists("foobar.o") );
  BOOST_CHECK( foo_o.rule().prerequisites().size() == 1 );
  BOOST_CHECK( bar_o.rule().prerequisites().size() == 1 );
  BOOST_CHECK( foobar_o.rule().prerequisites().size() == 1 );
  if ( foo_o.rule().prerequisites().size() == 1 )
    BOOST_CHECK( foo_o.rule().prerequisites()[0] == "foo.cpp" );
  if ( bar_o.rule().prerequisites().size() == 1 )
    BOOST_CHECK( bar_o.rule().prerequisites()[0] == "bar.cpp" );
  if ( foobar_o.rule().prerequisites().size() == 1 )
    BOOST_CHECK( foobar_o.rule().prerequisites()[0] == "foobar.cpp" );

  if ( fs::exists("foo.o") ) fs::remove("foo.o");
  if ( fs::exists("bar.o") ) fs::remove("bar.o");
  if ( fs::exists("foobar.o") ) fs::remove("foobar.o");
  if ( fs::exists("foo.cpp") ) fs::remove("foo.cpp");
  if ( fs::exists("bar.cpp") ) fs::remove("bar.cpp");
  if ( fs::exists("foobar.cpp") ) fs::remove("foobar.cpp");
  smart::builtin::target::update_result uc1 = foo_o.update( sm );
  smart::builtin::target::update_result uc2 = bar_o.update( sm );
  smart::builtin::target::update_result uc3 = foobar_o.update( sm );
  BOOST_CHECK( uc1.count_updated == 2 );
  BOOST_CHECK( uc1.count_executed == 2 );
  BOOST_CHECK( uc1.count_newer == 1 );
  BOOST_CHECK( uc2.count_updated == 2 );
  BOOST_CHECK( uc2.count_executed == 2 );
  BOOST_CHECK( uc2.count_newer == 1 );
  BOOST_CHECK( uc3.count_updated == 2 );
  BOOST_CHECK( uc3.count_executed == 2 );
  BOOST_CHECK( uc3.count_newer == 1 );
  BOOST_CHECK( fs::exists("foo.o") );
  BOOST_CHECK( fs::exists("bar.o") );
  BOOST_CHECK( fs::exists("foobar.o") );
  BOOST_CHECK( fs::exists("foo.cpp") );
  BOOST_CHECK( fs::exists("bar.cpp") );
  BOOST_CHECK( fs::exists("foobar.cpp") );
  fs::remove("foo.o");
  fs::remove("bar.o");
  fs::remove("foobar.o");
  fs::remove("foo.cpp");
  fs::remove("bar.cpp");
  fs::remove("foobar.cpp");
}

BOOST_AUTO_TEST_CASE( code_seg1 )
{
  smart::context sm;
  smart::compiler smc( sm );

  std::string code
    ( "############################\n"
      "CXX = g++\n"
      "CXXFLAGS = \n"
      "\n"
      "RM = rm\n"
      "RM_RF = $(RM) -rf\n"
      "V = debug\n"
      "\n"
      "OUT_DIR  = build/foo/$(V)\n"
      "OUT_BIN  = $(OUT_DIR)/bin\n"
      "OUT_OBJS = $(OUT_DIR)/objs\n"
      "\n"
      "#SOURCES = $(wildcard src/*.cpp)\n"
      "SOURCES = src/foo.cpp src/bar.cpp src/main.cpp\n"
      "OBJECTS = $(SOURCES:.cpp=.o)\n"
      "OBJECTS2 := $(SOURCES:%.cpp=%.o)\n"
      "OBJECTS3 := $(SOURCES:.cpp=.o)\n"
      "\n"
      "BUILT_OBJECT_PAT = $(OUT_OBJS)/%.o\n"
      "BUILT_OBJECT_PAT2 := $(OUT_OBJS)/%.o\n"
      "BUILT_OBJECTS = $(OBJECTS:%.o=$(BUILT_OBJECT_PAT))\n"
      "BUILT_OBJECTS2 := $(OBJECTS:%.o=$(BUILT_OBJECT_PAT))\n"
      "\n"
      "TARGET = foobar\n"
      "\n"
      "#.DEFAULT_GOAL := $(TARGET)\n"
      "\n"
      "MKDIR_IF_NON = @[ -d $(@D) ] || mkdir -p $(@D)\n"
      "\n"
      "all: $(TARGET:%=$(OUT_BIN)/%) foo\n"
      "\t@echo $(.DEFAULT_GOAL) done\n"
      "\n"
      "clean:\n"
      "\t$(RM_RF) $(TARGET:%=$(OUT_BIN)/%) $(BUILT_OBJECTS)\n"
      "\n"
      "$(TARGET:%=$(OUT_BIN)/%): $(BUILT_OBJECTS)\n"
      "#\t$(MKDIR_IF_NON)\n"
      "\tmkdir -p $(OUT_BIN)\n"
      "\t$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $() -o $@\n"
      "\n"
      "$(BUILT_OBJECTS):$(BUILT_OBJECT_PAT):%.cpp\n"
      "\t$(MKDIR_IF_NON)\n"
      "\t$(CXX) $(CXXFLAGS) -c $< -o $@\n"
      "\n"
      "" );

  smc.compile( code );

  smart::builtin::macro CXX( sm.macro("CXX") );
  smart::builtin::macro CXXFLAGS( sm.macro("CXXFLAGS") );
  smart::builtin::macro RM( sm.macro("RM") );
  smart::builtin::macro RM_RF( sm.macro("RM_RF") );
  smart::builtin::macro V( sm.macro("V") );
  smart::builtin::macro OUT_DIR( sm.macro("OUT_DIR") );
  smart::builtin::macro OUT_BIN( sm.macro("OUT_BIN") );
  smart::builtin::macro OUT_OBJS( sm.macro("OUT_OBJS") );
  smart::builtin::macro SOURCES( sm.macro("SOURCES") );
  smart::builtin::macro OBJECTS( sm.macro("OBJECTS") );
  smart::builtin::macro OBJECTS2( sm.macro("OBJECTS2") );
  smart::builtin::macro OBJECTS3( sm.macro("OBJECTS3") );
  smart::builtin::macro BUILT_OBJECT_PAT( sm.macro("BUILT_OBJECT_PAT") );
  smart::builtin::macro BUILT_OBJECT_PAT2( sm.macro("BUILT_OBJECT_PAT2") );
  smart::builtin::macro BUILT_OBJECTS( sm.macro("BUILT_OBJECTS") );
  smart::builtin::macro BUILT_OBJECTS2( sm.macro("BUILT_OBJECTS2") );
  smart::builtin::macro TARGET( sm.macro("TARGET") );
  smart::builtin::macro DEFAULT_GOAL( sm.macro(".DEFAULT_GOAL") );
  //std::clog<<"OBJECTS3: "<<OBJECTS3.value()<<std::endl;
  //std::clog<<"BUILT_OBJECTS2: "<<BUILT_OBJECTS2.value()<<std::endl;
  //std::clog<<"RM_RF: "<<RM_RF.value()<<std::endl;
  BOOST_CHECK( CXX == "g++" );
  BOOST_CHECK( CXXFLAGS == "" );
  BOOST_CHECK( RM == "rm" );
  BOOST_CHECK( RM_RF == "$(RM) -rf" );
  BOOST_CHECK( V == "debug" );
  BOOST_CHECK( OUT_DIR == "build/foo/$(V)" );
  BOOST_CHECK( OUT_BIN == "$(OUT_DIR)/bin" );
  BOOST_CHECK( OUT_OBJS == "$(OUT_DIR)/objs" );
  BOOST_CHECK( SOURCES == "src/foo.cpp src/bar.cpp src/main.cpp" );
  BOOST_CHECK( OBJECTS == "$(SOURCES:.cpp=.o)" );
  BOOST_CHECK( OBJECTS2 == "src/foo.o src/bar.o src/main.o" );
  BOOST_CHECK( OBJECTS3 == "src/foo.o src/bar.o src/main.o" );
  BOOST_CHECK( BUILT_OBJECT_PAT == "$(OUT_OBJS)/%.o" );
  BOOST_CHECK( BUILT_OBJECT_PAT2 == "build/foo/debug/objs/%.o" );
  BOOST_CHECK( BUILT_OBJECTS == "$(OBJECTS:%.o=$(BUILT_OBJECT_PAT))" );
  BOOST_CHECK( BUILT_OBJECTS2 == "build/foo/debug/objs/src/foo.o build/foo/debug/objs/src/bar.o build/foo/debug/objs/src/main.o" );
  BOOST_CHECK( TARGET == "foobar" );
  BOOST_CHECK( DEFAULT_GOAL == "all" );

  smart::builtin::target all( sm.target("all") );
  smart::builtin::target clean( sm.target("clean") );
  smart::builtin::target foobar( sm.target("build/foo/debug/bin/foobar") );
  smart::builtin::target obj1( sm.target("build/foo/debug/objs/src/foo.o") );
  smart::builtin::target obj2( sm.target("build/foo/debug/objs/src/bar.o") );
  smart::builtin::target obj3( sm.target("build/foo/debug/objs/src/main.o") );
  smart::builtin::target src1( "src/foo.cpp" /*sm.target("src/foo.cpp")*/ );
  smart::builtin::target src2( "src/bar.cpp" /*sm.target("src/bar.cpp")*/ );
  smart::builtin::target src3( "src/main.cpp" /*sm.target("src/main.cpp")*/ );
  //std::clog<<"foobar: "<<foobar<<std::endl;
  std::clog<<"obj1: "<<obj1<<std::endl;
  std::clog<<"obj1.prerequisites[0]: "<<obj1.rule().prerequisites()[0]<<std::endl;
  //std::clog<<"src1: "<<src1<<std::endl;
  BOOST_CHECK( all.object() == "all" );
  BOOST_CHECK( all.rule().prerequisites().size() == 2 );
  BOOST_CHECK( all.rule().prerequisites()[0].object() == "build/foo/debug/bin/foobar" );
  BOOST_CHECK( all.rule().prerequisites()[0] == foobar );
  BOOST_CHECK( all.rule().prerequisites()[1].object() == "foo" );
  BOOST_CHECK( all.rule().commands().size() == 1 );
  BOOST_CHECK( all.rule().commands()[0] == "@echo $(.DEFAULT_GOAL) done" );
  BOOST_CHECK( clean.object() == "clean" );
  BOOST_CHECK( clean.rule().prerequisites().size() == 0 );
  BOOST_CHECK( clean.rule().commands().size() == 1 );
  BOOST_CHECK( clean.rule().commands()[0] == "$(RM_RF) $(TARGET:%=$(OUT_BIN)/%) $(BUILT_OBJECTS)" );
  BOOST_CHECK( foobar.object() == "build/foo/debug/bin/foobar" );
  BOOST_CHECK( foobar.rule().prerequisites().size() == 3 );
  BOOST_CHECK( foobar.rule().prerequisites()[0].object() == "build/foo/debug/objs/src/foo.o" );
  BOOST_CHECK( foobar.rule().prerequisites()[1].object() == "build/foo/debug/objs/src/bar.o" );
  BOOST_CHECK( foobar.rule().prerequisites()[2].object() == "build/foo/debug/objs/src/main.o" );
  BOOST_CHECK( foobar.rule().prerequisites()[0] == obj1 );
  BOOST_CHECK( foobar.rule().prerequisites()[1] == obj2 );
  BOOST_CHECK( foobar.rule().prerequisites()[2] == obj3 );
  BOOST_CHECK( foobar.rule().commands().size() == 2 );
  BOOST_CHECK( foobar.rule().commands()[0] == "mkdir -p $(OUT_BIN)" );
  BOOST_CHECK( foobar.rule().commands()[1] == "$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $() -o $@" );
  BOOST_CHECK( obj1.object() == "build/foo/debug/objs/src/foo.o" );
  BOOST_CHECK( obj1.rule().prerequisites().size() == 1 );
  BOOST_CHECK( obj1.rule().prerequisites()[0].object() == "src/foo.cpp" );
  BOOST_CHECK( obj1.rule().prerequisites()[0] == src1 );
  BOOST_CHECK( obj2.object() == "build/foo/debug/objs/src/bar.o" );
  BOOST_CHECK( obj2.rule().prerequisites().size() == 1 );
  BOOST_CHECK( obj2.rule().prerequisites()[0].object() == "src/bar.cpp" );
  BOOST_CHECK( obj2.rule().prerequisites()[0] == src2 );
  BOOST_CHECK( obj3.object() == "build/foo/debug/objs/src/main.o" );
  BOOST_CHECK( obj3.rule().prerequisites().size() == 1 );
  BOOST_CHECK( obj3.rule().prerequisites()[0].object() == "src/main.cpp" );
  BOOST_CHECK( obj3.rule().prerequisites()[0] == src3 );
  BOOST_CHECK( src1 == "src/foo.cpp" );
  BOOST_CHECK( src1.object() == "src/foo.cpp" );
  BOOST_CHECK( src1.rule().prerequisites().size() == 0 );
  BOOST_CHECK( src1.exists() == fs::exists(std::string(src1.object())) );
  BOOST_CHECK( src2 == "src/bar.cpp" );
  BOOST_CHECK( src2.object() == "src/bar.cpp" );
  BOOST_CHECK( src2.rule().prerequisites().size() == 0 );
  BOOST_CHECK( src2.exists() == fs::exists(std::string(src2.object())) );
  BOOST_CHECK( src3 == "src/main.cpp" );
  BOOST_CHECK( src3.object() == "src/main.cpp" );
  BOOST_CHECK( src3.rule().prerequisites().size() == 0 );
  BOOST_CHECK( src3.exists() == fs::exists(std::string(src3.object())) );
}//code_seg1

BOOST_AUTO_TEST_CASE( code_seg2 )
{
  smart::context sm;
  smart::compiler smc( sm );

  std::string code
    ( "############################\n"
      "SOURCES := foo.hpp bar.hpp\n"
      "foo:\n"
      "\t@echo \"/* Generated by moc.mk */\" > $@\n"
      "\t@for F in $(SOURCES) ; do echo \"#include \\\"$$F\\\"\" >> $@ ; done\n"
      "\t@echo \\#!/bin/bash >> $@\n"
      "\t@echo D=~/$@$@  >> $@\n"
      "\t@echo LD_LIBRARY_PATH=\\$$D >> $@\n"
      "\t@echo export LD_LIBRARY_PATH >> $@\n"
      "\t@echo cd \\$$D >> $@\n"
      "\t@echo ./$@ >> $@\n"
      "\n"
      "foobar \\\n	 : \\\n"
      "  foo \\\n"
      "		 bar  \\\n \t \\\n"
      "\n"
      "" );
  //std::clog<<code<<std::endl;
  smc.compile( code );
  smart::builtin::target foo( sm.target("foo") );
  smart::builtin::target null( sm.target("nullxx") );
  smart::builtin::target foobar( sm.target("foobar") );
  BOOST_CHECK( null.is_null() );
  BOOST_CHECK( !foobar.is_null() );
  BOOST_CHECK( foobar.exists() == fs::exists("foobar") );
  BOOST_CHECK( foobar.rule().prerequisites().size() == 2 );
  BOOST_CHECK( foobar.rule().prerequisites()[0] == "foo" );
  BOOST_CHECK( foobar.rule().prerequisites()[1] == "bar" );
  BOOST_CHECK( !foo.is_null() );
  BOOST_CHECK( foo.exists() == fs::exists("foo") );
  if ( fs::exists( "foo" ) ) fs::remove( "foo" );
  smart::builtin::target::update_result uc( foo.update( sm ) );
  BOOST_CHECK( uc.count_updated == 1 );
  BOOST_CHECK( fs::exists("foo") );
  BOOST_CHECK( foo.exists() );
  {
    std::string cont
      (
       "/* Generated by moc.mk */\n"
       "#include \"foo.hpp\"\n"
       "#include \"bar.hpp\"\n"
       "#!/bin/bash\n"
       "D=~/foofoo\n"
       "LD_LIBRARY_PATH=$D\n"
       "export LD_LIBRARY_PATH\n"
       "cd $D\n"
       "./foo\n"
       );
    std::string s( read_all( "foo" ) );
    BOOST_CHECK( s == cont );
    if ( s != cont ) {
      std::clog<<"cont = '"<<cont<<"'"<<std::endl;
      std::clog<<"s = '"<<s<<"'"<<std::endl;
    }
  }
}//code_seg2
