/**                                                             -*- c++ -*-
 *    Copyright 2009-08-25 DuzySoft.com, by Zhan Xin-Ming (Duzy Chan)
 *    All rights reserved by Zhan Xin-Ming (Duzy Chan)
 *    Email: <duzy@duzy.info, duzy.chan@gmail.com>
 *
 *    $Id$
 *
 **/

#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_parse_tree.hpp>
#include <boost/spirit/include/classic_parse_tree_utils.hpp>
#include <boost/spirit/include/classic_grammar_def.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>
//#include <boost/spirit/include/classic_attribute.hpp>
//#include <boost/spirit/include/classic_parametric.hpp>
#include <boost/spirit/include/classic_exceptions.hpp>

namespace smart
{
  using namespace boost::spirit::classic;

  struct grammar : boost::spirit::classic::grammar<grammar>
  {
    enum parser_id_e {
      id_noop,
      id_statements,
      id_statement,
      id_assignment,
      id_macro_name,
      id_macro_ref,
      id_macro_ref_name,
      id_macro_ref_args,
      id_macro_ref_pattern,
      id_macro_value,
      id_expandable,
      id_make_rule,
      id_make_rule_targets,
      id_make_rule_commands,
      id_make_rule_command,
      id_include_directive,
    };//enum parser_id_e

    enum error_e {
      e_expect_eol, e_expect_eq, e_expect_rparen,
    };
    typedef boost::spirit::classic::assertion<int> assertion_t;
    typedef guard<int> guard_t;
    struct parse_error_handler_t
    {
      template<typename TScan, typename TError>
      error_status<> operator()( const TScan &, const TError & e ) const
      {
        std::ostringstream err;
        err<<"Syntax error: "<<*e.where;
        throw smart::parser_error( e.where, err.str() );
      }
    };//struct parse_error_handler_t
    struct parse_expandable_error_handler_t
    {
      template<typename TScan, typename TError>
      error_status<> operator()( const TScan &, const TError & e ) const
      {
        std::ostringstream err;
        err<<"Expandable syntax error at '"<<*e.where<<"'";
        throw smart::parser_error( e.where, err.str() );
      }
    };//struct parse_expandable_error_handler_t

    template<typename TScan>
    struct definition
      : boost::spirit::classic::grammar_def<
      rule<TScan, parser_tag<id_statements> >,
      rule<TScan, parser_tag<id_expandable> >
      >
    {
      assertion_t expect_eol;
      assertion_t expect_eq;
      assertion_t expect_rparen;

      guard_t sm_guard;
      parse_error_handler_t sm_error_handler;
      parse_expandable_error_handler_t sm_expandable_error_handler;

      rule<TScan, parser_tag<id_statements> > statements;
      rule<TScan, parser_tag<id_statement> > statement;
      rule<TScan, parser_tag<id_assignment> > assignment;
      rule<TScan, parser_tag<id_macro_name> > macro_name;
      rule<TScan, parser_tag<id_macro_ref> > macro_ref;
      rule<TScan, parser_tag<id_macro_ref_name> > macro_ref_name;
      rule<TScan, parser_tag<id_macro_ref_args> > macro_ref_args;
      rule<TScan, parser_tag<id_macro_ref_pattern> > macro_ref_pattern;
      rule<TScan, parser_tag<id_macro_value> > macro_value;
      rule<TScan, parser_tag<id_expandable> > expandable;
      rule<TScan, parser_tag<id_make_rule> > make_rule;
      rule<TScan, parser_tag<id_make_rule_targets> > make_rule_targets;
      rule<TScan, parser_tag<id_make_rule_commands> > make_rule_commands;
      rule<TScan, parser_tag<id_make_rule_command> > make_rule_command;
      rule<TScan, parser_tag<id_include_directive> > include_directive;

      definition( const smart::grammar & self )
        : expect_eol( e_expect_eol )
        , expect_eq( e_expect_eq )
        , expect_rparen( e_expect_rparen )
        , push_paren( _parens )
        , pop_paren( _parens )
        , rparen( _parens )
      {
        statements
          =  sm_guard( *statement >> end_p )[ sm_error_handler ]
          ;

        statement  /* looks spirit can't eat spaces between two rules */
          =  no_node_d[ *space_p ] >> assignment
          |  no_node_d[ *space_p ] >> make_rule
	  |  no_node_d[ *space_p ] >> macro_ref
          |  no_node_d[ *space_p ] >> include_directive
          ;

        assignment
          =  lexeme_d
             [
                 macro_name
                 >> no_node_d[ *(space_p - eol_p) ]
                 >> root_node_d
                    [  ch_p('=')
                    |  str_p("+=")
                    |  str_p(":=")
                    |  str_p("?=")
                    ]
                 >> no_node_d[ *(space_p - eol_p) ]
                 >> (  no_node_d[ (eol_p | end_p) ]
                    |  macro_value
                       >> no_node_d[ !(eol_p | end_p) ]
                    )
             ]
          ;

        macro_name
          =  lexeme_d
             [
                +(  token_node_d[ +(graph_p - chset_p("$:=+?"))]
                 |  ~eps_p( space_p ) >> macro_ref
                 )
             ]
	     >> ( eps_p( ch_p('=' ) | "+=" | ":=" | "?=" ) )
          ;

        macro_ref
          =  lexeme_d
             [
                eps_p('$')
                >> (  no_node_d[ str_p("$()") ]
                   |  no_node_d[ str_p("${}") ]
                   |  ( str_p("$(") | str_p("${") )[ push_paren ]
                      >> ~eps_p( space_p )
                      >> macro_ref_name
                      >> !( ( no_node_d[ space_p ] >> macro_ref_args )
                          | ( eps_p(':') >> macro_ref_pattern )
                          )
                      >> expect_rparen( f_ch_p(rparen)[ pop_paren ] )

                   |  ch_p('$') >> graph_p
                   )
             ]
          ;

        macro_ref_name
          =  lexeme_d
             [
                +(  token_node_d
                    [
                       +(  (graph_p - (chset_p("$:") | f_ch_p(rparen)))
                        //|  ~eps_p( space_p ) >> macro_ref
                        )
                    ]
                 |  ~eps_p( space_p ) >> macro_ref
                 )
             ]
          ;

        macro_ref_args
          =  lexeme_d
             [
                token_node_d
                [ +(  ( anychar_p - (chset_p("$,")|f_ch_p(rparen)) )
                   |  macro_ref
                   )
                ]
                >> *(  no_node_d[ ch_p(',') ]
                       >> !token_node_d
                          [ +(  (anychar_p - (chset_p("$,") | f_ch_p(rparen) ))
                             |  macro_ref
                             )
                          ]
                    )
             ]
          ;

        macro_ref_pattern
          =  lexeme_d
             [
                no_node_d[ ch_p(':') ]
                >> token_node_d
                   [
                      +(  (anychar_p - (chset_p("$=")|f_ch_p(rparen)))
                       |  ~eps_p( space_p ) >> macro_ref
                       )
                   ]
                >> expect_eq( root_node_d[ ch_p('=') ] )
                >> token_node_d
                   [
                      +(  (anychar_p - (chset_p("$")|f_ch_p(rparen)))
                       |  ~eps_p( space_p ) >> macro_ref
                       )
                   ]
             ]
          ;

        //!< list
        macro_value
          =  lexeme_d
             [
               //!< discard the heading spaces
                no_node_d[ *(space_p - eol_p) ]
                >> *(  !token_node_d[ ch_p('\\') >> eol_p ]
                       >> no_node_d[ *(space_p - eol_p) ]
                       >> +(  token_node_d
                              [
                                 +( anychar_p - (  chset_p("$\r\n")
                                                |  ch_p('\\') >> eps_p(eol_p)
                                                )
                                  )
                              ]
                           |  ~eps_p(eol_p) >> macro_ref
                           //|  token_node_d[ ch_p('\\') >> graph_p ]
                           )
                    )
             ]
          ;

        expandable
          = sm_guard( macro_value )[ sm_expandable_error_handler ]
          ;

        make_rule
          =  lexeme_d
             [  //eps_p( e >>
                make_rule_targets
                >> no_node_d[ *(space_p - eol_p) ]
                >> token_node_d[ ch_p(':') ]
                >> no_node_d[ *(space_p - eol_p) ]
                >> !(  ~eps_p( eol_p )
                       >> make_rule_targets
                       >> no_node_d[ *(space_p - eol_p) ]
                       >> !(  token_node_d[ ch_p(':') ]
                              >> no_node_d[ *(space_p - eol_p) ]
                              >> ~eps_p(eol_p)
                              >> make_rule_targets
                           )
                    )
                >> no_node_d[ *(space_p - eol_p) ]
                >> (  token_node_d[ eol_p | end_p ]
                      >> no_node_d[ *comment_p( "#" ) ]
                      >> !(  no_node_d[ ch_p('\t') ]
                             >> make_rule_commands
                          )
                   |  token_node_d[ ch_p(';') ]
                      >> make_rule_commands
                   )
             ]
          ;

        make_rule_targets
          =  lexeme_d
             [
                +(  token_node_d
                    [
                       +( (anychar_p - chset_p("$:|; \t\r\n\\"))
                        | ~eps_p( space_p ) >> macro_ref
                        )
                    ]
                 |  no_node_d[ +(space_p - eol_p) ] >> !macro_ref
                 |  no_node_d[ ch_p('\\') >> eol_p ]
                 )
             ]
          ;

        make_rule_commands
          =  lexeme_d
             [
                make_rule_command
                >> *(  no_node_d[ eol_p >> ch_p('\t') ]
                       >> make_rule_command
                    )
             ]
          ;

        make_rule_command
          =  lexeme_d
             [
                *(  token_node_d
                    [
                       +(anychar_p - ( eol_p | ch_p('\\') >> eps_p(eol_p) ) )
                    ]
                 //|  token_node_d[ ch_p('\\') >> graph_p ]
                 |  no_node_d[ ch_p('\\') >> expect_eol( eol_p ) ]
                 )
             ]
          ;

        include_directive
          =  root_node_d[ !ch_p('-') >> str_p("include") ]
          >> make_rule_targets
          ;

	this->start_parsers( statements, expandable );

        debug();
      }//definition()

      void debug()
      {
#       ifdef BOOST_SPIRIT_DEBUG
        BOOST_SPIRIT_DEBUG_RULE(statements);
        BOOST_SPIRIT_DEBUG_RULE(statement);
        BOOST_SPIRIT_DEBUG_RULE(assignment);
        BOOST_SPIRIT_DEBUG_RULE(macro_name);
        BOOST_SPIRIT_DEBUG_RULE(macro_ref);
        BOOST_SPIRIT_DEBUG_RULE(macro_ref_name);
        BOOST_SPIRIT_DEBUG_RULE(macro_ref_args);
        BOOST_SPIRIT_DEBUG_RULE(macro_ref_pattern);
        BOOST_SPIRIT_DEBUG_RULE(macro_value);
        BOOST_SPIRIT_DEBUG_RULE(expandable);
        BOOST_SPIRIT_DEBUG_RULE(make_rule);
        BOOST_SPIRIT_DEBUG_RULE(make_rule_targets);
        BOOST_SPIRIT_DEBUG_RULE(make_rule_commands);
        BOOST_SPIRIT_DEBUG_RULE(make_rule_command);
        BOOST_SPIRIT_DEBUG_RULE(include_directive);
#       endif
      }//debug()

      const rule<TScan, parser_tag<id_statements> > & start()
      {
        return statements;
      }

    protected:
      std::vector<char> _parens;
      struct push_paren_t
      {
        std::vector<char> & parens;
        explicit push_paren_t( std::vector<char> & ref ) : parens(ref) {}
        template<typename TIter> void operator()( TIter beg, TIter end ) const
        {
          std::string s( beg, end );
          if ( s == "$(" ) parens.push_back( ')' );
          else if ( s == "${" ) parens.push_back( '}' );
          else throw smart::runtime_error( "unmatched paren: " + s);
        }
      } push_paren;
      struct pop_paren_t
      {
        std::vector<char> & parens;
        explicit pop_paren_t( std::vector<char> & ref ) : parens(ref) {}
        template<typename TChar> void operator()( TChar ch ) const
        {
          if ( parens.empty() ) throw smart::runtime_error("empty parens");
          else if ( parens.back() != ch )
            throw smart::runtime_error(std::string("unmatched paren: ")+ch);
          parens.pop_back();
        }
      } pop_paren;
      struct right_paren_t
      {
        std::vector<char> & parens;
        explicit right_paren_t( std::vector<char> & ref ) : parens(ref) {}
        char operator()() const
        {
          if ( parens.empty() ) return '\0';
          return parens.back();
        }
      } rparen;
    };//struct definition
  };//struct grammar

  //============================================================

  struct grammar_partial : boost::spirit::classic::grammar<grammar_partial>
  {
    template<typename TScan>
    struct definition
    {
      definition( const smart::grammar_partial & self )
      {
      }
    };//struct definition
  };//struct grammar_partial

  //============================================================

  struct grammar_skip : boost::spirit::classic::grammar<grammar_skip>
  {
    template<typename TScan>
    struct definition
    {
      definition( const grammar_skip & self )
      {
        skip
          =   +space_p
          |   comment_p("#")
          ;

#       ifdef BOOST_SPIRIT_DEBUG
        BOOST_SPIRIT_DEBUG_RULE(skip);
#       endif
      }

      rule<TScan> skip;

      const rule<TScan> & start() { return skip; }
    };//struct definition
  };//struct grammar_skip
  
}//namespace smart
