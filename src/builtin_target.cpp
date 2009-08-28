/**
 *    Copyright 2009-08-25 DuzySoft.com, by Duzy Chan��ղ������
 *    All rights reserved by Duzy Chan��ղ������
 *    Email: <duzy@duzy.info, duzy.chan@gmail.com>
 *
 *    $Id$
 *
 **/

#include "builtin_target.hpp"
#include "builtin_make_rule.hpp"
#include "builtin_target_imp.hpp"
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <algorithm>

namespace smart
{
  namespace builtin
  {

    void intrusive_ptr_add_ref( target::imp *p )
    {
      target::imp::inref( p );
    }

    void intrusive_ptr_release( target::imp * & p )
    {
      target::imp::deref( p );
    }
    
    target::target()
      : vm::type_ext()
      , _i( new imp )
    {
    }
    
    target::target( const vm::type_string & str )
      : vm::type_ext()
      , _i( new imp(str) )
    {
    }

    long target::refcount() const
    {
      return _i->_usage;
    }
    
    vm::type_string target::object() const
    {
      return _i->_object;
    }

    const make_rule & target::rule() const
    {
      return _i->_rule;
    }

    /**
     *  * Each target binds to one make-rule
     *  * New binding will overrides the previous commands
     *  * New binding combines prerequisites with the previous binding
     */
    void target::bind( const make_rule & r )
    {
      if ( _i->_rule.empty() ) {
        _i->_rule = r;
        return;
      }
      else {
	//!< If some other targets are referenced to the rule, we should make
	//!< a cloning of it.
	if ( 1 < _i->_rule.refcount() ) {
	  _i->_rule = _i->_rule.clone();
	}

        const std::vector<builtin::target> & ps( r.prerequisites() );
        std::for_each( ps.begin(), ps.end(), boost::bind(&make_rule::add_prerequisite, &_i->_rule, _1) );
        if ( !r.commands().empty() ) _i->_rule.set_commands( r.commands() );
      }
    }
    
  }//namespace builtin
}//namespace smart
