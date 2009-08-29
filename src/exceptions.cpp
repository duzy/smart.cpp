/**
 *    Copyright 2009-08-29 DuzySoft.com, by Duzy Chan��ղ������
 *    All rights reserved by Duzy Chan��ղ������
 *    Email: <duzy@duzy.info, duzy.chan@gmail.com>
 *
 *    $Id$
 *
 **/

#include "exceptions.hpp"

namespace smart
{

  exception::exception( const std::string & s ) throw()
    : _what(s)
  {
  }

  exception::~exception() throw()
  {
  }

  const char * exception::what() const throw()
  {
    return _what.c_str();
  }

  //======================================================================

  compile_error::compile_error( const std::string & f, long line, long column, const std::string & w ) throw()
    : exception(w), _file(f), _line(line), _column(column)
  {
  }

  compile_error::~compile_error() throw()
  {
  }

  const std::string & compile_error::file() const throw()
  {
    return _file;
  }

  long compile_error::line() const throw()
  {
    return _line;
  }

  long compile_error::column() const throw()
  {
    return _column;
  }

  //======================================================================

  make_error::make_error( const std::string & err ) throw()
    : exception( err )
  {
  }

  make_error::~make_error() throw()
  {
  }

}//namespace smart
