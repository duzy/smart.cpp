/**
 *    Copyright 2009-08-25 DuzySoft.com, by Duzy Chan��ղ������
 *    All rights reserved by Duzy Chan��ղ������
 *    Email: <duzy@duzy.info, duzy.chan@gmail.com>
 *
 *    $Id$
 *
 **/

#include "compiler.hpp"
#include "context.hpp"
#include "string_table.hpp"
#include "macro_table.hpp"
#include "builtins.hpp"
#include "vm_types.hpp"
#include "expand.hpp"
#include "grammar.ipp"
#include "exceptions.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/bind.hpp>

# ifdef BOOST_SPIRIT_DEBUG
#   include <iostream>
# endif
# ifdef BOOST_SPIRIT_DEBUG_XML
#   include <boost/spirit/include/classic_tree_to_xml.hpp>
# endif

#   include <iostream>

namespace smart
{
  namespace detail
  {
    enum macro_ref_type_e {
      macro_ref_type_normal,
      macro_ref_type_funcall,
      macro_ref_type_pattern,
    };//enum macro_ref_type_e
    struct parsed_macro_ref
    {
      macro_ref_type_e type;
      vm::type_string name;
      std::vector<vm::type_string> args;

      parsed_macro_ref()
	: type( macro_ref_type_normal )
	, name()
	, args()
      {
      }

      parsed_macro_ref( const vm::type_string & nm, macro_ref_type_e e )
	: type( e )
	, name( nm )
	, args()
      {
      }
    };//struct parsed_macro_ref

    //======================================================================

    typedef boost::spirit::classic::file_position_base<std::string> position_t;
    template<typename TTreeIter>
    static inline const position_t & get_position( const TTreeIter & iter )
    {
      return iter->value.begin().get_position();
    }

    template<typename TTreeIter>
    static vm::type_string expanded_macro_value( context & ctx, const TTreeIter & iter );

    template<typename TTreeIter>
    static parsed_macro_ref parse_macro_ref( context & ctx, const TTreeIter & iter )
    {
      assert( iter->value.id() == grammar::id_macro_ref );
      assert( iter->children.empty() || 2 <= iter->children.size() ); //!< '$(' ... ')'

      parsed_macro_ref ref;
      if ( iter->children.empty() ) return ref;
      if ( iter->children.size() == 2 || iter->children.size() == 3 ) {
        //!< $X, $(X), ${X}, the first child may be '$' or '$(' or '${'
        TTreeIter nodeName( iter->children.begin() + 1 );
        ref.name = ctx.const_string( std::string(nodeName->value.begin(), nodeName->value.end()) );
	return ref;
      }

      TTreeIter child( iter->children.begin() + 1 ); //!< skip '$('
      TTreeIter const end( iter->children.end() - 1 ); //!< skip ')'
      while( child != end ) {
        switch( child->value.id().to_long() ) {
        case grammar::id_macro_ref:
          ref.name += expanded_macro_value( ctx, child );
          break;

	case grammar::id_macro_ref_args:
	  ref.type = macro_ref_type_funcall;
	  if ( child->children.empty() ) {
	    std::string s(child->value.begin(), child->value.end());
	    ref.args.push_back( ctx.const_string(s) );
	    break;
	  }
	  else goto pack_args;

	case grammar::id_macro_ref_pattern:
	  ref.type = macro_ref_type_pattern;
	  goto pack_args;

	pack_args:
	  {
	    TTreeIter arg( child->children.begin() );
	    for(; arg != child->children.end(); ++arg) {
	      switch( arg->value.id().to_long() ) {
	      default:
		{
		  std::string s(arg->value.begin(),arg->value.end());
		  ref.args.push_back( ctx.const_string(s) );
		}
		break;
	      }//switch( arg-type )
	    }//for( args )
	  }//case macro_ref_pattern
	  child = end; //!< ends the iteration
	  break;

        default:
          ref.name += ctx.const_string( std::string(child->value.begin(), child->value.end()) );
	  break;
        }//switch
	
	if ( child == end ) break;
	else ++child;
      }//while( has-more-child )

      if ( ref.type == macro_ref_type_pattern ) {
        assert( ref.args.size() == 2 );
        if ( !ref.args[0].contains('%') ) ref.args[0] = "%" + ref.args[0];
        if ( !ref.args[1].contains('%') ) ref.args[1] = "%" + ref.args[1];
      }//if( macro-ref )
      
      return ref;
    }//parse_macro_ref()

    template<typename TTreeIter>
    static vm::type_string unexpended_macro_value( context & ctx, const TTreeIter & iter )
    {
      vm::type_string value;
      if ( iter->children.empty() || iter->value.id() == grammar::id_macro_ref ) {
        return ctx.const_string( std::string(iter->value.begin(), iter->value.end()) );
      }

      TTreeIter child( iter->children.begin() );
      TTreeIter const end( iter->children.end() );
      for(; child != end; ++child ) {
        std::string s( child->value.begin(), child->value.end() );
        ctx.const_string(s);

        std::string t;
        if ( s == "\\" ) t = ( value.empty() ? "" : " " );
        else t += s;
        value += t;
      }//for

      return value;
    }//unexpended_macro_value()

    template<typename TTreeIter>
    static vm::type_string expanded_macro_value( context & ctx, const TTreeIter & iter )
    {
      assert( iter->value.id() == grammar::id_macro_name ||
	      iter->value.id() == grammar::id_macro_value ||
              iter->value.id() == grammar::id_macro_ref );

      if ( iter->value.id() == grammar::id_macro_ref ) {
	parsed_macro_ref ref( parse_macro_ref(ctx, iter) );
	switch( ref.type ) {
	case macro_ref_type_normal:
	  {
	    builtin::macro m( ctx.mtable()->get( ref.name ) );
	    assert( ref.name == m.name() );
	    return m.expand( ctx );
	  }
	case macro_ref_type_pattern:
	  {
	    builtin::macro m( ctx.mtable()->get( ref.name ) );
	    assert( ref.name == m.name() );
	    assert( ref.args.size() == 2 ); //!< e.g. [%.cpp,%.o]
	    return m.patsubst( ref.args );
	  }
	case macro_ref_type_funcall:
	  {
	    return ctx.invoke( ref.name, ref.args );
	  }
	default:
	  {
	    std::ostringstream err;
	    err<<"invalid macro ref type: "<<ref.type;
            throw compile_error( ctx.file(), iter, err.str() );
	  }
	}//switch( ref-type )
	
	return vm::type_string();
      }//if( macro-ref )

      if ( iter->children.empty() ) {
	std::string s( iter->value.begin(), iter->value.end() );
        return ctx.const_string(s);
      }

      vm::type_string v;

      TTreeIter child( iter->children.begin() );
      TTreeIter const end( iter->children.end() );
      for(; child != end; ++child) {
        switch( child->value.id().to_long() ) {
        case grammar::id_macro_ref:
          v += expanded_macro_value( ctx, child );
          break;
        default:
          {
            std::string s( child->value.begin(), child->value.end() );
            ctx.const_string( s );

            if ( s == "\\" ) v += " ";
            else v += s;
          }
          break;
        }//switch
      }//for
      
      return v;
    }//expanded_macro_value()

    template<typename TTreeIter>
    static void compile_assignment( context & ctx, const TTreeIter & iter )
    {
      assert( iter->value.id() == grammar::id_assignment );
      assert( 0 < iter->children.size() );

      vm::type_string name( expanded_macro_value( ctx, iter->children.begin() ) );
      TTreeIter nodeValue( 1 < iter->children.size()
                           ? iter->children.begin() + 1
			   : iter->children.end() );

      builtin::macro m( ctx.mtable()->map(name) );
      assert( m.name() == name );

      char type( *(iter->value.begin()) );
      switch( type ) {
      case '=': //!< recursive expended
      case '?': //!< recursive if undefined
        {
          if ( type == '?' && m.origin() != builtin::macro::origin_undefined )
            break;
          m.set_origin( builtin::macro::origin_file );
          m.set_flavor( builtin::macro::flavor_recursive );
          if ( nodeValue != iter->children.end() ) {
            m.set_value( unexpended_macro_value(ctx, nodeValue) );
          }
        }
        break;

      case ':': //!< simple expended
        {
          m.set_origin( builtin::macro::origin_file );
          m.set_flavor( builtin::macro::flavor_simple );
          if ( nodeValue != iter->children.end() ) {
            m.set_value( expanded_macro_value( ctx, nodeValue ) );
          }
        }
        break;

      case '+':
        if ( nodeValue == iter->children.end() ) break;
        if ( m.flavor() == builtin::macro::flavor_simple ) { //!< :=
          m.set_value( m.value() + expanded_macro_value( ctx, nodeValue ) );
        }
        else { //!< =, or undefined
          m.set_value( m.value() + unexpended_macro_value(ctx, nodeValue) );
        }
        break;

      default:
        {
          std::ostringstream err;
          err<<"invalid assignment";
          throw compile_error( ctx.file(), iter, err.str() );
        }
        break;

      }//switch( type )
    }//compile_assignment()

    static void split_targets( context & ctx,
                               std::vector<vm::type_string> & out,
                               const vm::type_string & lst )
    {
      const std::string & str( lst );
      typedef boost::split_iterator<std::string::const_iterator> iter_t;
      iter_t it( boost::make_split_iterator(str, boost::token_finder(boost::is_any_of(" \t"))) );
      iter_t const end;
      for(; it != end; ++it) {
	if ( it->empty() ) continue;
        std::string t( boost::copy_range<std::string>( *it ) );
        out.push_back( ctx.const_string(t) );
      }
    }//split_targets()

    template<typename TTreeIter>
    static std::vector<vm::type_string> parse_targets( context & ctx, const TTreeIter & iter )
    {
      std::vector<vm::type_string> vec;
      int id( iter->value.id().to_long() );
      if ( iter->children.empty() ||
           !( id == grammar::id_make_rule_targets ||
              id == grammar::id_make_rule_prereqs ) ) {
        std::string s( iter->value.begin(), iter->value.end() );
        split_targets( ctx, vec, expand( ctx, s ) );
      }
      else {
        TTreeIter it( iter->children.begin() );
        for(; it != iter->children.end(); ++it) {
          std::string s( it->value.begin(), it->value.end() );
          split_targets( ctx, vec, expand( ctx, s ) );
        }//for(each-prerequisites)
      }
      return vec;
    }//parse_targets()

    template<typename TTreeIter>
    static void compile_make_rule( context & ctx, const TTreeIter & iter )
    {
      assert( iter->value.id() == grammar::id_make_rule );
      assert( 2 <= iter->children.size() );

      builtin::make_rule r( true );

      TTreeIter child( iter->children.begin() );
      TTreeIter targets, prereqs, commands;
      targets = prereqs = commands = iter->children.end();

      if ( *child->value.begin() == ':' ) (void)false;
      else targets = child++;
      ++child;

      if ( *child->value.begin() == '\r' ||
           *child->value.begin() == '\n' ||
           *child->value.begin() == ';' )
        (void)false;
      else prereqs = child++;
      ++child;

      if ( child != iter->children.end() ) commands = child;

      bool hasPatternPrereq( false );
      if ( prereqs != iter->children.end() ) {
        std::vector<vm::type_string> vec( parse_targets(ctx, prereqs) );
        std::vector<vm::type_string>::iterator it( vec.begin() );
        for(; it != vec.end(); ++it) {
          bool isPattern( it->contains('%') );
          if ( !hasPatternPrereq ) hasPatternPrereq = isPattern;
          builtin::target target( isPattern ? builtin::target(*it) : ctx.map_target(*it) );
          //assert( 2 <= target.refcount() );
          r.add_prerequisite( target );
          //std::clog<<"prerequisite: "<<target<<std::endl;
        }//for(each-prerequisite)
      }//if(has-prerequisites)

      if ( commands != iter->children.end() ) {
        if ( commands->value.id() == grammar::id_make_rule_commands ) {
          std::string s;
          TTreeIter it( commands->children.begin() );
          for(; it != commands->children.end(); ++it) {
            s.clear();
            if ( it->value.id() == grammar::id_make_rule_command ) {
              if ( it->children.empty() )
                s.assign( it->value.begin(), it->value.end() );
              else {
                TTreeIter i( it->children.begin() );
                for(; i != it->children.end(); ++i) {
                  s += std::string( i->value.begin(), i->value.end() );
                }//for
              }
            }//if( make_rule_command )
            else s = std::string( it->value.begin(), it->value.end() );
            r.add_command( ctx.const_string(s) );
            //std::clog<<"command: "<<s<<std::endl;
          }//for( commands )
        }//if( make_rule_commands )
        else {
          std::string s( commands->value.begin(), commands->value.end() );
          r.add_command( ctx.const_string(s) );
          //std::clog<<"command: "<<s<<std::endl;
        }//not( make_rule_commands )
      }//if(has-commands)

      if ( targets != iter->children.end() ) {
        std::vector<vm::type_string> vec( parse_targets(ctx, targets) );
        std::vector<vm::type_string>::iterator it( vec.begin() );
        for(; it != vec.end(); ++it) {
          bool isPattern( it->contains('%') );
          builtin::target target( isPattern ? ctx.map_pattern(*it) : ctx.map_target(*it) );
          assert( 2 <= target.refcount() );
          if ( !target.rule().commands().empty() && !r.commands().empty() ) {
            std::clog<<ctx.file()
                     <<":"<<get_position(iter).line
                     <<":"<<get_position(iter).column
                     <<":warning: overriding commands for target '"<<*it<<"'"
                     <<std::endl;
          }//if( overriding-commands )
          target.bind( r );
          ctx.set_default_goal_if_null( target );
          //std::clog<<"prerequisite: "<<target<<std::endl;
        }//for(each-target)
      }//if(has-targets)

      return;
    }//compile_make_rule()

    template<typename TTreeIter>
    static void compile_macro_ref( context & ctx, const TTreeIter & iter )
    {
      assert( iter->value.id() == grammar::id_macro_ref );

      parsed_macro_ref ref( parse_macro_ref(ctx, iter) );
      if ( ref.type == macro_ref_type_funcall ) {
        //vm::type_string res( ctx.invoke( ref.name, ref.args ) );
        ctx.invoke( ref.name, ref.args );
      }
    }//compile_macro_ref()

    template<typename TTreeIter>
    static void compile_include_directive( context & ctx, const TTreeIter & iter )
    {
      assert( iter->value.id() == grammar::id_include_directive );
      assert( iter->children.size() == 1 );

      TTreeIter targets( iter->children.begin() );
      std::vector<vm::type_string> vec( parse_targets(ctx, targets) );
      std::for_each(vec.begin(), vec.end(), boost::bind(&context::add_include, &ctx, _1));
    }//compile_include_directive()

    template<typename TTreeIter>
    static void compile_statements( context & ctx, const TTreeIter & iter );

    template<typename TTreeIter>
    static void compile_statement( context & ctx, const TTreeIter & iter )
    {
      switch( iter->value.id().to_long() ) {
      case grammar::id_statements:
        compile_statements( ctx, iter );
        break;

      case grammar::id_assignment:
        compile_assignment( ctx, iter );
        break;

      case grammar::id_make_rule:
        compile_make_rule( ctx, iter );
        break;

      case grammar::id_macro_ref:
        compile_macro_ref( ctx, iter );
        break;

      case grammar::id_include_directive:
        compile_include_directive( ctx, iter );
        break;

      default:
        {
          std::ostringstream err;
          err<<"Unimplemented statement: "<<iter->value.id().to_long();
          throw compile_error( ctx.file(), iter, err.str() );
        }
        
      }//switch( type )
    }//compile_statement()

    template<typename TTreeIter>
    static void compile_statements( context & ctx, const TTreeIter & iter )
    {
      assert( iter->value.id() == grammar::id_statements ||
              iter->value.id() == grammar::id_statement ||
              iter->value.id() == grammar::id_make_rule ||
              iter->value.id() == grammar::id_macro_ref ||
              iter->value.id() == grammar::id_assignment ||
              iter->value.id() == grammar::id_include_directive
              );

      switch( iter->value.id().to_long() ) {
      case grammar::id_statements:
        {
          TTreeIter child( iter->children.begin() );
          TTreeIter const end( iter->children.end() );
          for(; child != end; ++child) compile_statement( ctx, child );
          break;
        }//case statements

      default:
        compile_statement( ctx, iter );
      }//switch( type )
    }//compile_statements()

    template<typename TTree>
    static void compile_tree( context & ctx, const TTree & tree )
    {
      if ( tree.size() <= 0 ) return;
      compile_statements( ctx, tree.begin() );
    }//compile_tree()
  }//namespace detail

  //======================================================================

  vm::type_string expand( const context & ctx, const vm::type_string & str )
  {
    std::vector<vm::type_string> args;
    return expand( ctx, str, args );
  }//expand()

  vm::type_string expand( const context & ctx, const vm::type_string & str,
                          const std::vector<vm::type_string> & args )
  {
    using namespace boost::spirit;
    typedef classic::position_iterator<std::string::const_iterator> iter_t;
    typedef classic::node_iter_data_factory<> factory_t;
    typedef classic::tree_parse_info<iter_t, factory_t> parse_tree_info_t;

    const_cast<context&>(ctx).setup_macro_args( args );

    grammar g;
    const std::string & code( str );
    iter_t beg(code.begin(), code.end()), end;
    parse_tree_info_t pt( classic::ast_parse<factory_t>(beg, end, g.use_parser<1>(), classic::nothing_p) );

    if ( !pt.full ) {
      std::ostringstream err;
      err<<"Syntax error.";
      throw compile_error( ctx.file(), pt.stop.get_position().line,
                           pt.stop.get_position().column, err.str() );
    }

    vm::type_string v;
    v = detail::expanded_macro_value( const_cast<context&>(ctx), pt.trees.begin() );

    const_cast<context&>(ctx).clear_macro_args();

    return v;
  }//expand()

  //======================================================================

  void include( context & ctx, const std::string & filename )
  {
    builtin::target tar( filename );
    if ( !tar.exists() ) {
      tar.update( ctx );
    }
    compiler smc( ctx );
    smc.compile_file( filename );
  }

  //======================================================================

  compiler::compiler( context & ctx )
    : _context( ctx )
  {
  }

  void compiler::compile_file( const std::string & filename )
  {
    std::ifstream ifs( filename.c_str() );
    if ( !ifs ) {
      std::ostringstream err;
      err<<"smart: Can't open script '"<<filename<<"'.";
      throw smart::runtime_error( err.str() );
    }

    ifs.seekg( 0, ifs.end );
    int sz( ifs.tellg() );
    if ( 0 < sz ) {
      std::string code;
      code.resize( sz );
      ifs.seekg( 0, ifs.beg );
      ifs.read( &code[0], sz );

      _context._files.push_back( filename );
      compile( code.begin(), code.end() );
      _context._files.pop_back();
    }
  }

  void compiler::compile( const std::string & code )
  {
    compile( code.begin(), code.end() );
  }

  void compiler::compile( const std::string::const_iterator & codeBeg,
                          const std::string::const_iterator & codeEnd )
  {
    grammar g;
    grammar_skip s;

#   ifdef BOOST_SPIRIT_DEBUG
    BOOST_SPIRIT_DEBUG_GRAMMAR(g);
    BOOST_SPIRIT_DEBUG_GRAMMAR(s);
#   endif//BOOST_SPIRIT_DEBUG

    using namespace boost::spirit;
    typedef classic::position_iterator<std::string::const_iterator> iter_t;
    typedef classic::node_iter_data_factory<> factory_t;
    typedef classic::tree_parse_info<iter_t, factory_t> parse_tree_info_t;

    iter_t beg(codeBeg, codeEnd), end;
    parse_tree_info_t pt( classic::ast_parse<factory_t>(beg, end, g, s) );

#   ifdef BOOST_SPIRIT_DEBUG_XML
    {
      std::map<classic::parser_id, std::string> names;
      names[smart::grammar::id_statements] = "statements";
      names[smart::grammar::id_statement] = "statement";
      names[smart::grammar::id_assignment] = "assignment";
      names[smart::grammar::id_macro_name] = "macro_name";
      names[smart::grammar::id_macro_ref] = "macro_ref";
      names[smart::grammar::id_macro_ref_args] = "macro_ref_args";
      names[smart::grammar::id_macro_ref_pattern] = "macro_ref_pattern";
      names[smart::grammar::id_macro_value] = "macro_value";
      names[smart::grammar::id_make_rule] = "make_rule";
      names[smart::grammar::id_make_rule_targets] = "make_rule_targets";
      names[smart::grammar::id_make_rule_prereqs] = "make_rule_prereqs";
      names[smart::grammar::id_make_rule_commands] = "make_rule_commands";
      names[smart::grammar::id_make_rule_command] = "make_rule_command";
      names[smart::grammar::id_include_directive] = "include_directive";
      std::string code( codeBeg, codeEnd );
      classic::tree_to_xml(std::clog, pt.trees, code, names);
    }
#   endif//BOOST_SPIRIT_DEBUG_XML

    if ( !pt.full ) {
      std::ostringstream err;
      err<<"Syntax error.";
      throw compile_error( _context.file(), pt.stop.get_position().line,
                           pt.stop.get_position().column, err.str() );
    }

    detail::compile_tree( _context, pt.trees );

    _context.include_files();
  }
}//namespace smart

