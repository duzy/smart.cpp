/**
 *    Copyright 2009-08-25 DuzySoft.com, by Zhan Xin-Ming (Duzy Chan)
 *    All rights reserved by Zhan Xin-Ming (Duzy Chan)
 *    Email: <duzy@duzy.info, duzy.chan@gmail.com>
 *
 *    $Id$
 *
 **/

#ifndef __SMART_CONTEXT__HPP____by_Duzy_Chan__
#define __SMART_CONTEXT__HPP____by_Duzy_Chan__ 1
#	include "vm/vm_fwd.hpp"
#	include "builtin/builtin_macro.hpp"
#	include "builtin/builtin_make_rule.hpp"
#	include "frame.hpp"
#	include <boost/unordered_map.hpp>
#	include <string>
#	include <list>
#	include <map>
#	include <set>

namespace smart
{
  struct string_table;
  struct real_table;
  struct macro_table;
  struct function_table;

  /**
   *  @brief Smart script running context
   */
  struct context
  {
    context();
    virtual ~context();

    string_table * stable() const;

    const std::vector<std::string> files(); //!< files compiling
    std::string file() const; //!< current compiling file

    vm::type_string const_string( const std::string & c );
    //vm::type_real const_number( double c );

    macro_table *mtable() const;
    builtin::macro macro( const vm::type_string & v );
    builtin::macro macro( const std::string & v );

    builtin::target target( const std::string & v ) { return this->target(vm::type_string(v)); }
    builtin::target target( const vm::type_string & v );
    builtin::target map_target( const vm::type_string & v );
    builtin::target map_pattern( const vm::type_string & v );
    builtin::target match_patterns( const vm::type_string & v ) const;

    builtin::make_rule find_rule( const builtin::target & );

    /**
     * setup $1, $2, $3, $4, ....
     */
    void setup_macro_args( const std::vector<vm::type_string> & args );
    void clear_macro_args();

    function_table *ftable() const;
    vm::type_string invoke( const vm::type_string&, const std::vector<vm::type_string>& );

    frame & current_frame();

    bool is_phony( const builtin::target & ) const;

    builtin::target default_goal() const;
    void set_default_goal_if_null( const builtin::target & tar );

    bool add_include( const vm::type_string & );

  private:
    void include_files();

  private:
    string_table *_string_table; //!< for string constants
    //real_table *_number_table; //!< for real number constants
    macro_table *_macro_table;
    function_table *_function_table;

    typedef boost::unordered_map<vm::type_string, builtin::target> target_table;
    target_table _targets;
    std::set< builtin::target > _phony_targets;
    std::vector< builtin::target > _patterns;
    builtin::target _default_goal;

    typedef std::vector<builtin::make_rule> rules_t;
    rules_t _rules;

    std::list<vm::type_string> _includes;
    std::set<vm::type_string> _included;

    typedef std::vector<vm::type_string> args_t;
    std::vector<args_t> _macroArgs;

    std::vector<frame> _frames;

    friend struct compiler;
    std::vector<std::string> _files;
  };//struct context

  void info( const vm::type_string & );
  void warning( const vm::type_string & );
  void error( const vm::type_string & );

}//namespace smart

/**
 *	Macro	- Expandable makefile variable
 *	Rule	- Target building rule
 *	Target	- A file to be built according a defined rule
 *	Prerequisite - Targets required by the currently building target
 *	Variable - Smart script variable: string, integer, float, array
 */

#endif//__SMART_CONTEXT__HPP____by_Duzy_Chan__
