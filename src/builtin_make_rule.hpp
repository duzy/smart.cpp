/**
 *    Copyright 2009-08-25 DuzySoft.com, by Duzy Chan��ղ������
 *    All rights reserved by Duzy Chan��ղ������
 *    Email: <duzy@duzy.info, duzy.chan@gmail.com>
 *
 *    $Id$
 *
 **/

#ifndef __SMART_BUILTIN_MAKE_RULE__HPP____by_Duzy_Chan__
#define __SMART_BUILTIN_MAKE_RULE__HPP____by_Duzy_Chan__ 1
#	include "vm_types.hpp"
#	include "builtin_target.hpp"
#	include <vector>

namespace smart
{
  struct context;

  namespace builtin
  {
    /**
     *  @brief A make rule decides what to make, when to make and how to make.
     */
    struct make_rule : vm::type_ext
    {
      make_rule();

      /**
       *  @brief Update prerequisites that needs updating.
       */
      int update_prerequisites( context & ) const;

    private:
      //std::vector<vm::type_string> _targets;
      std::vector<target> _prerequisites;
      std::vector<vm::type_string> _orderonly; //!< order-only prerequisites
      std::vector<vm::type_string> _commands;
    };//struct make_rule
    
  }//namespace builtin
}//namespace smart

#endif//__SMART_BUILTIN_MAKE_RULE__HPP____by_Duzy_Chan__
