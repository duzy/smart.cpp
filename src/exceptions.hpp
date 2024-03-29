/**
 *    Copyright 2009-08-29 DuzySoft.com, by Zhan Xin-Ming (Duzy Chan)
 *    All rights reserved by Zhan Xin-Ming (Duzy Chan)
 *    Email: <duzy@duzy.info, duzy.chan@gmail.com>
 *
 *    $Id$
 *
 **/

#include <exception>
#include <string>

namespace smart
{
  struct exception : std::exception
  {
    explicit exception( const std::string & s ) throw();
    virtual ~exception() throw();

    const char * what() const throw();

  private:
    std::string _what;
  };//struct exception

  struct parser_error : exception
  {
    parser_error( long line, long column, const std::string & w ) throw();
    virtual ~parser_error() throw();

    template<typename TIter>
    parser_error( const TIter & it, const std::string & w ) throw()
      : exception(w), _line(get_line(it)), _column(get_column(it))
    {}

    long line() const throw();
    long column() const throw();

  private:
    template<typename TIter> static inline long get_line( const TIter & it ) { return it.get_position().line; }
    template<typename TIter> static inline long get_column( const TIter & it ) { return it.get_position().column; }

  private:
    long _line;
    long _column;
  };//struct parser_error

  struct compile_error : exception
  {
    compile_error( const std::string & f, long line, long column, const std::string & w ) throw();
    virtual ~compile_error() throw();

    template<typename TIter>
    compile_error( const std::string & f, const TIter & it, const std::string & w ) throw()
      : exception(w), _file(f), _line(get_line(it)), _column(get_column(it))
    {}

    const std::string & file() const throw();
    long line() const throw();
    long column() const throw();

  private:
    template<typename TIter> static inline long get_line( const TIter & it ) { return it->value.begin().get_position().line; }
    template<typename TIter> static inline long get_column( const TIter & it ) { return it->value.begin().get_position().column; }

  private:
    std::string _file;
    long _line;
    long _column;
  };//struct compile_error

  struct make_error : exception
  {
    explicit make_error( const std::string & ) throw();
    virtual ~make_error() throw();
  };//struct make_error

  struct runtime_error : exception
  {
    explicit runtime_error( const std::string & ) throw();
    virtual ~runtime_error() throw();
  };//struct make_error
}//namespace smart
