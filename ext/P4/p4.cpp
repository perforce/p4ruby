/*******************************************************************************

Copyright (c) 2001-2008, Perforce Software, Inc.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1.  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

2.  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL PERFORCE SOFTWARE, INC. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/*******************************************************************************
 * Name		: p4.cc
 *
 * Author	: Tony Smith <tony@perforce.com> or <tony@smee.org>
 *
 * Description	: Ruby bindings for the Perforce API.
 *
 * vim:ts=8:sw=4
 ******************************************************************************/
#include <ruby.h>
#include "undefdups.h"
#include <p4/clientapi.h>
#include <p4/strtable.h>
#include <p4/spec.h>
#include <p4/ident.h>
#include "p4result.h"
#include "specmgr.h"
#include "clientuserruby.h"
#include "p4rubyconf.h"
#include "p4clientapi.h"
#include "p4mergedata.h"
#include "p4mapmaker.h"
#include "p4error.h"
#include "p4utils.h"
#include "extconf.h"


// Our Ident mechanism doesn't really translate to a semantic versioning scheme,
// So it's been simply replaced by the current version string. We'll see if that
// needs to change, since a lot of the information this displayed is available
// via RbConfig locally.
//static Ident ident = {
//	IdentMagic "P4RUBY " P4RUBY_VERSION
//};


/*******************************************************************************
 * Our Ruby classes.
 ******************************************************************************/
VALUE 	cP4;	// Base P4 Class
VALUE	eP4;	// Exception class
VALUE	cP4MD;	// P4::MergeData class
VALUE	cP4Map;	// P4::Map class
VALUE	cP4Msg; // P4::Message class
VALUE	cP4Prog;	//	P4::Progress class


extern "C"
{

//
// Construction/destruction
//

static void p4_free( P4ClientApi *p4 )
{
    delete p4;
}

static void p4_mark( P4ClientApi *p4 )
{
    p4->GCMark();
}

static VALUE p4_new( VALUE pClass )
{
    VALUE  	argv[ 1 ];
    P4ClientApi	*p4 = new P4ClientApi();
    VALUE	self;

    self = Data_Wrap_Struct( pClass, p4_mark, p4_free, p4 );
    rb_obj_call_init( self, 0, argv );
    return self;
}


//
// Session connect/disconnect
//
static VALUE p4_connect( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->Connect();
}

static VALUE p4_disconnect( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->Disconnect();
}

static VALUE p4_connected( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->Connected();
}

static VALUE p4_server_case_sensitive( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    if( p4->ServerCaseSensitive() )
	return Qtrue;
    return Qfalse;
}

static VALUE p4_server_unicode( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    if( p4->ServerUnicode() )
	return Qtrue;
    return Qfalse;
}

static VALUE p4_run_tagged( VALUE self, VALUE tagged )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );

    if( ! rb_block_given_p() )
	rb_raise( rb_eArgError, "P4#run_tagged requires a block" );

    // The user might have passed an integer, or it might be a boolean,
    // we convert to int for consistency.
    int		flag = 0;
    if( tagged == Qtrue )
	flag = 1;
    else if( tagged == Qfalse )
	flag = 0;
    else
	flag = NUM2INT( tagged ) ? 1 : 0;

    int old_value = p4->IsTagged();
    p4->Tagged( flag );

    VALUE ret_val;

    //
    // XXX: This should perhaps be protected with rb_ensure()...
    //
    ret_val = rb_yield( self );

    p4->Tagged( old_value );
    return ret_val;
}

static VALUE p4_get_tagged( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->IsTagged() ? Qtrue : Qfalse;
}

static VALUE p4_set_tagged( VALUE self, VALUE toggle )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );

    // The user might have passed an integer, or it might be a boolean,
    // we convert to int for consistency.
    int		flag = 0;
    if( toggle == Qtrue )
	flag = 1;
    else if( toggle == Qfalse )
	flag = 0;
    else
	flag = NUM2INT( toggle ) ? 1 : 0;

    p4->Tagged( flag );
    return flag ? Qtrue : Qfalse;	// Seems to be ignored...
}

static VALUE p4_get_api_level( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return INT2NUM( p4->GetApiLevel() );
}

static VALUE p4_set_api_level( VALUE self, VALUE level )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetApiLevel( NUM2INT( level ) );
    return self;
}

//
// Getting/Setting Perforce environment
//
static VALUE p4_get_charset( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    StrPtr c = p4->GetCharset();
    return P4Utils::ruby_string( c.Text() );
}

static VALUE p4_set_charset( VALUE self, VALUE c )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );

    // p4.charset = nil prior to connect can be used to
    // disable automatic charset detection
    if( c == Qnil )
	return p4->SetCharset( 0 );

    return p4->SetCharset( StringValuePtr( c ) );
}

static VALUE p4_get_p4config( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    StrPtr c = p4->GetConfig();
    return P4Utils::ruby_string( c.Text() );
}

static VALUE p4_get_cwd( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    StrPtr cwd = p4->GetCwd();
    return P4Utils::ruby_string( cwd.Text() );
}

static VALUE p4_set_cwd( VALUE self, VALUE cwd )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetCwd( StringValuePtr( cwd ) );
    return Qtrue;
}

static VALUE p4_get_client( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    StrPtr client = p4->GetClient();
    return P4Utils::ruby_string( client.Text() );
}

static VALUE p4_set_client( VALUE self, VALUE client )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetClient( StringValuePtr( client ) );
    return Qtrue;
}

static VALUE p4_get_env( VALUE self, VALUE var )
{
    P4ClientApi	*p4;
    const char *val;
    Data_Get_Struct( self, P4ClientApi, p4 );
    val = p4->GetEnv( StringValuePtr( var ) );
    if( !val ) return Qnil;

    return P4Utils::ruby_string( val );
}

static VALUE p4_set_env( VALUE self, VALUE var, VALUE val )
{
    P4ClientApi *p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->SetEnv( StringValuePtr( var ), StringValuePtr( val ) );
}

static VALUE p4_get_enviro_file( VALUE self )
{
    P4ClientApi *p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    const StrPtr *enviro_file = p4->GetEnviroFile();
    return P4Utils::ruby_string( enviro_file->Text() );
}

static VALUE p4_set_enviro_file( VALUE self, VALUE rbstr )
{
    P4ClientApi *p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetEnviroFile( StringValuePtr(rbstr) );
    return Qtrue;
}

static VALUE p4_get_evar( VALUE self, VALUE var )
{
    P4ClientApi	*p4;
    const StrPtr *val;
    Data_Get_Struct( self, P4ClientApi, p4 );
    val = p4->GetEVar( StringValuePtr( var ) );
    if( !val ) return Qnil;

    return P4Utils::ruby_string( val->Text() );
}

static VALUE p4_set_evar( VALUE self, VALUE var, VALUE val )
{
    P4ClientApi *p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetEVar( StringValuePtr( var ), StringValuePtr( val ) );
    return Qtrue;
}

static VALUE p4_get_host( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    StrPtr host = p4->GetHost();
    return P4Utils::ruby_string( host.Text() );
}

static VALUE p4_set_host( VALUE self, VALUE host )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetHost( StringValuePtr( host ) );
    return Qtrue;
}

static VALUE p4_get_ignore( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    StrPtr ignore = p4->GetIgnoreFile();
    return P4Utils::ruby_string( ignore.Text() );
}

static VALUE p4_set_ignore( VALUE self, VALUE file )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetIgnoreFile( StringValuePtr( file ) );
    return Qtrue;
}

static VALUE p4_is_ignored( VALUE self, VALUE path )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    if( p4->IsIgnored( StringValuePtr( path ) ) )
	return Qtrue;
    return Qfalse;
}

static VALUE p4_get_language( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    StrPtr lang = p4->GetLanguage();
    return P4Utils::ruby_string( lang.Text() );
}

static VALUE p4_set_language( VALUE self, VALUE lang )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetLanguage( StringValuePtr( lang ) );
    return Qtrue;
}

static VALUE p4_get_maxresults( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return INT2NUM( p4->GetMaxResults() );
}

static VALUE p4_set_maxresults( VALUE self, VALUE val )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetMaxResults( NUM2INT( val ) );
    return Qtrue;
}

static VALUE p4_get_maxscanrows( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return INT2NUM( p4->GetMaxScanRows() );
}

static VALUE p4_set_maxscanrows( VALUE self, VALUE val )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetMaxScanRows( NUM2INT( val ) );
    return Qtrue;
}

static VALUE p4_get_maxlocktime( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return INT2NUM( p4->GetMaxLockTime() );
}

static VALUE p4_set_maxlocktime( VALUE self, VALUE val )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetMaxLockTime( NUM2INT( val ) );
    return Qtrue;
}

static VALUE p4_get_password( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    StrPtr passwd = p4->GetPassword();
    return P4Utils::ruby_string( passwd.Text() );
}

static VALUE p4_set_password( VALUE self, VALUE passwd )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetPassword( StringValuePtr( passwd ) );
    return Qtrue;
}

static VALUE p4_get_port( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    StrPtr port = p4->GetPort();
    return P4Utils::ruby_string( port.Text() );
}

static VALUE p4_set_port( VALUE self, VALUE port )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    if( p4->Connected() )
	rb_raise( eP4, "Can't change port once you've connected." );

    p4->SetPort( StringValuePtr( port ) );
    return Qtrue;
}

static VALUE p4_get_prog( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return P4Utils::ruby_string( p4->GetProg().Text() );
}

static VALUE p4_set_prog( VALUE self, VALUE prog )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetProg( StringValuePtr( prog ) );
    return Qtrue;
}

static VALUE p4_set_protocol( VALUE self, VALUE var, VALUE val )
{
    P4ClientApi *p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetProtocol( StringValuePtr( var ), StringValuePtr( val ) );
    return Qtrue;
}

static VALUE p4_get_ticket_file( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return P4Utils::ruby_string( p4->GetTicketFile().Text() );
}

static VALUE p4_set_ticket_file( VALUE self, VALUE path )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetTicketFile( StringValuePtr( path ) );
    return Qtrue;
}

static VALUE p4_get_trust_file( VALUE self )
{
    P4ClientApi *p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return P4Utils::ruby_string( p4->GetTrustFile().Text() );
}

static VALUE p4_set_trust_file( VALUE self, VALUE path )
{
    P4ClientApi *p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetTrustFile( StringValuePtr( path ) );
    return Qtrue;
}

static VALUE p4_get_user( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    StrPtr user = p4->GetUser();
    return P4Utils::ruby_string( user.Text() );
}

static VALUE p4_set_user( VALUE self, VALUE user )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetUser( StringValuePtr( user ) );
    return Qtrue;
}

static VALUE p4_get_version( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return P4Utils::ruby_string( p4->GetVersion().Text() );
}

static VALUE p4_set_version( VALUE self, VALUE version )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetVersion( StringValuePtr( version ) );
    return Qtrue;
}

static VALUE p4_get_track( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetTrack() ? Qtrue : Qfalse;
}

static VALUE p4_set_track( VALUE self, VALUE toggle )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );

    // The user might have passed an integer, or it might be a boolean,
    // we convert to int for consistency.
    int		flag = 0;
    if( toggle == Qtrue )
	flag = 1;
    else if( toggle == Qfalse )
	flag = 0;
    else
	flag = NUM2INT( toggle ) ? 1 : 0;

    p4->SetTrack( flag );
    return flag ? Qtrue : Qfalse;	// Seems to be ignored...
}

static VALUE p4_get_streams( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->IsStreams() ? Qtrue : Qfalse;
}

static VALUE p4_set_streams( VALUE self, VALUE toggle )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );

    // The user might have passed an integer, or it might be a boolean,
    // we convert to int for consistency.
    int		flag = 0;
    if( toggle == Qtrue )
	flag = 1;
    else if( toggle == Qfalse )
	flag = 0;
    else
	flag = NUM2INT( toggle ) ? 1 : 0;

    p4->SetStreams( flag );
    return flag ? Qtrue : Qfalse;	// Seems to be ignored...
}

static VALUE p4_get_graph( VALUE self )
{
    P4ClientApi *p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->IsGraph() ? Qtrue : Qfalse;
}

static VALUE p4_set_graph( VALUE self, VALUE toggle )
{
    P4ClientApi *p4;
    Data_Get_Struct( self, P4ClientApi, p4 );

    // The user might have passed an integer, or it might be a boolean,
    // we convert to int for consistency.
    int     flag = 0;
    if( toggle == Qtrue )
    flag = 1;
    else if( toggle == Qfalse )
    flag = 0;
    else
    flag = NUM2INT( toggle ) ? 1 : 0;

    p4->SetGraph( flag );
    return flag ? Qtrue : Qfalse;   // Seems to be ignored...
}

/*******************************************************************************
 * Running commands.  General purpose Run method and method for supplying
 * input to "p4 xxx -i" commands
 ******************************************************************************/

static VALUE p4_run( VALUE self, VALUE args )
{
    int 	i;
    int		argc = 0;
    ID		idFlatten = rb_intern( "flatten" );
    ID		idLength = rb_intern( "length" );
    ID		idTo_S	 = rb_intern( "to_s" );

    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );

    // Flatten the args array, and extract the Perforce command leaving
    // the remaining args in the array.
    VALUE flatArgs = rb_funcall( args, idFlatten, 0 );

    if ( ! NUM2INT( rb_funcall( flatArgs, idLength, 0 ) ) )
	rb_raise( eP4, "P4#run requires an argument" );

    VALUE v = rb_funcall( flatArgs, rb_intern( "shift" ), 0 );
    char *cmd = StringValuePtr( v );
    argc = NUM2INT( rb_funcall( flatArgs, idLength, 0 ) );

    // Allocate storage on the stack so it's automatically reclaimed
    // when we exit.
    char **p4args = RB_ALLOC_N( char *, argc + 1 );

    // Copy the args across
    for ( i = 0; i < argc; i++ )
    {
	VALUE	entry = rb_ary_entry( flatArgs, i );
	VALUE	v = rb_funcall( entry, idTo_S, 0 );
	p4args[ i ] = StringValuePtr( v );
    }
    p4args[ i ] = 0;

    // Run the command
    VALUE res =  p4->Run( cmd, argc, p4args );
    return res;
}

static VALUE p4_set_input( VALUE self, VALUE input )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->SetInput( input );
}

static VALUE p4_get_errors( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetErrors();
}

static VALUE p4_get_messages( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetMessages();
}

static VALUE p4_get_warnings( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetWarnings();
}

static VALUE p4_set_except_level( VALUE self, VALUE level )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->ExceptionLevel( NUM2INT(level) );
    return level;
}

static VALUE p4_get_except_level( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return INT2NUM( p4->ExceptionLevel() );
}

static VALUE p4_get_server_level( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    int level = p4->GetServerLevel();
    return INT2NUM( level );
}

static VALUE p4_parse_spec( VALUE self, VALUE type, VALUE form )
{
    P4ClientApi	*p4;

    Check_Type( form, T_STRING );
    Check_Type( type, T_STRING );

    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->ParseSpec( StringValuePtr(type), StringValuePtr(form) );
}

static VALUE p4_format_spec( VALUE self, VALUE type, VALUE hash )
{
    P4ClientApi	*p4;

    Check_Type( type, T_STRING );
    Check_Type( hash, T_HASH );

    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->FormatSpec( StringValuePtr(type), hash );
}

static VALUE p4_track_output( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetTrackOutput();
}

/*******************************************************************************
 * Self identification
 ******************************************************************************/
static VALUE p4_identify( VALUE self )
{
    StrBuf	s;
    s.Append("P4RUBY ");
    s.Append(P4RUBY_VERSION);
    s.Append(" P4API ");
    s.Append(P4APIVER_STRING);
    s.Append(" PATCHLEVEL ");
    s.Append(P4API_PATCHLEVEL_STRING);
    s.Append(" WITH_LIBS ");
    s.Append(WITH_LIBS);
    return P4Utils::ruby_string( s.Text() );
}

/*******************************************************************************
 * Debugging support
 ******************************************************************************/
static VALUE p4_get_debug( VALUE self)
{
    P4ClientApi *p4;
    Data_Get_Struct( self, P4ClientApi, p4);
    return INT2NUM( p4->GetDebug() );
}

static VALUE p4_set_debug( VALUE self, VALUE debug )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetDebug( NUM2INT(debug) );
    return Qtrue;
}

/*******************************************************************************
 * Handler support
 ******************************************************************************/
static VALUE p4_get_handler( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetHandler();
}

static VALUE p4_set_handler( VALUE self, VALUE handler )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetHandler( handler );
    return Qtrue;
}

/*******************************************************************************
 * Progress support
 ******************************************************************************/
static VALUE p4_get_progress( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetProgress();
}

static VALUE p4_set_progress( VALUE self, VALUE progress )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->SetProgress( progress );
}

/*******************************************************************************
 * SSO handler support
 ******************************************************************************/
static VALUE p4_get_enabled_sso( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetEnableSSO();
}

static VALUE p4_set_enable_sso( VALUE self, VALUE enable )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->SetEnableSSO( enable );
}

static VALUE p4_get_sso_vars( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetSSOVars();
}

static VALUE p4_get_sso_passresult( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetSSOPassResult();
}

static VALUE p4_set_sso_passresult( VALUE self, VALUE result )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->SetSSOPassResult( result );
}

static VALUE p4_get_sso_failresult( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetSSOFailResult();
}

static VALUE p4_set_sso_failresult( VALUE self, VALUE result )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->SetSSOFailResult( result );
}
static VALUE p4_get_ssohandler( VALUE self )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    return p4->GetSSOHandler();
}

static VALUE p4_set_ssohandler( VALUE self, VALUE handler )
{
    P4ClientApi	*p4;
    Data_Get_Struct( self, P4ClientApi, p4 );
    p4->SetSSOHandler( handler );
    return Qtrue;
}

/*******************************************************************************
 * P4::MergeData methods. Construction/destruction defined elsewhere
 ******************************************************************************/

static VALUE p4md_getyourname( VALUE self )
{
    P4MergeData	*md = 0;
    Data_Get_Struct( self, P4MergeData, md );
    return md->GetYourName();
}

static VALUE p4md_gettheirname( VALUE self )
{
    P4MergeData	*md = 0;
    Data_Get_Struct( self, P4MergeData, md );
    return md->GetTheirName();
}

static VALUE p4md_getbasename( VALUE self )
{
    P4MergeData	*md = 0;
    Data_Get_Struct( self, P4MergeData, md );
    return md->GetBaseName();
}

static VALUE p4md_getyourpath( VALUE self )
{
    P4MergeData	*md = 0;
    Data_Get_Struct( self, P4MergeData, md );
    return md->GetYourPath();
}

static VALUE p4md_gettheirpath( VALUE self )
{
    P4MergeData	*md = 0;
    Data_Get_Struct( self, P4MergeData, md );
    return md->GetTheirPath();
}

static VALUE p4md_getbasepath( VALUE self )
{
    P4MergeData	*md = 0;
    Data_Get_Struct( self, P4MergeData, md );
    return md->GetBasePath();
}

static VALUE p4md_getresultpath( VALUE self )
{
    P4MergeData	*md = 0;
    Data_Get_Struct( self, P4MergeData, md );
    return md->GetResultPath();
}

static VALUE p4md_getmergehint( VALUE self )
{
    P4MergeData	*md = 0;
    Data_Get_Struct( self, P4MergeData, md );
    return md->GetMergeHint();
}

static VALUE p4md_runmerge( VALUE self )
{
    P4MergeData	*md = 0;
    Data_Get_Struct( self, P4MergeData, md );
    return md->RunMergeTool();
}

static VALUE p4md_getcontentresolve( VALUE self )
{
	P4MergeData *md = 0;
	Data_Get_Struct( self, P4MergeData, md );
	return md->GetContentResolveStatus();
}

//
//	Additional methods added for action resolve
//
static VALUE p4md_getactionresolve( VALUE self )
{
	P4MergeData *md = 0;
	Data_Get_Struct( self, P4MergeData, md );
	return md->GetActionResolveStatus();
}

static VALUE p4md_getyoursaction( VALUE self )
{
	P4MergeData *md = 0;
	Data_Get_Struct( self, P4MergeData, md );
	return md->GetYoursAction();
}

static VALUE p4md_gettheiraction( VALUE self )
{
	P4MergeData *md = 0;
	Data_Get_Struct( self, P4MergeData, md );
	return md->GetTheirAction();
}

static VALUE p4md_getmergeaction( VALUE self )
{
	P4MergeData *md = 0;
	Data_Get_Struct( self, P4MergeData, md );
	return md->GetMergeAction();
}

static VALUE p4md_getactiontype( VALUE self )
{
	P4MergeData *md = 0;
	Data_Get_Struct( self, P4MergeData, md );
	return md->GetType();
}

static VALUE p4md_getinfo( VALUE self )
{
	P4MergeData *md = 0;
	Data_Get_Struct( self, P4MergeData, md );
	return md->GetMergeInfo();
}

static VALUE p4md_invalidate( VALUE self )
{
	P4MergeData *md = 0;
	Data_Get_Struct( self, P4MergeData, md );
	md->Invalidate();
	return self;
}

static VALUE p4md_tos( VALUE self )
{
	P4MergeData *md = 0;
	Data_Get_Struct( self, P4MergeData, md );
	return md->GetString();
}
/******************************************************************************
 * P4::Map class
 ******************************************************************************/
static void p4map_free( P4MapMaker *m )
{
    delete m;
}

static VALUE p4map_new( int argc, VALUE *argv, VALUE pClass )
{
    //VALUE 	pClass;
    VALUE	array;
    VALUE	self;
    P4MapMaker	*m = new P4MapMaker;

    // First arg is the class
    // pClass = argv[ 0 ];

    // Now instantiate the new object.
    self = Data_Wrap_Struct( pClass, 0, p4map_free, m );
    rb_obj_call_init( self, 0, argv );

    if( argc )
    {
	array = argv[ 0 ];
	if( !rb_obj_is_kind_of( array, rb_cArray ) )
	    rb_raise( rb_eRuntimeError, "Not an array" );

	StrBuf	t;
	ID	idLen = rb_intern( "length" );
	int	len;
	VALUE	entry;

	// Now iterate over the array, inserting the mappings
	len = NUM2INT( rb_funcall( array, idLen, 0 ) );
	for( int i = 0; i < len; i++ )
	{
	    entry = rb_ary_entry( array, i );
	    m->Insert( entry );
	}
    }
    return self;
}

//
// Joins the RHS of the first mapping with the LHS of the second, and
// returns a new P4::Map object made up of the LHS of the first and the
// RHS of the second where the joins match up.
//
static VALUE p4map_join( VALUE pClass, VALUE left, VALUE right )
{
    P4MapMaker *	l = 0;
    P4MapMaker *	r = 0;
    P4MapMaker *	j = 0;
    VALUE 	m;
    VALUE  	argv[ 1 ];

    Data_Get_Struct( left,  P4MapMaker, l );
    Data_Get_Struct( right, P4MapMaker, r );

    j = P4MapMaker::Join( l, r );
    if( !j ) return Qnil;

    m = Data_Wrap_Struct( pClass, 0, p4map_free, j );
    rb_obj_call_init( m, 0, argv );
    return m;
}

//
// Debugging support
//
static VALUE p4map_inspect( VALUE self )
{
    P4MapMaker *	m = 0;
    StrBuf		b;
    StrBuf		tb;

    tb.Alloc( 32 );
    sprintf( tb.Text(), "%p", (void*) self );
    tb.SetLength();

    Data_Get_Struct( self, P4MapMaker, m );

    b << "#<P4::Map:" << tb << "> ";

    m->Inspect( b );
    return P4Utils::ruby_string( b.Text(), b.Length() );
}

//
// Insert a mapping into a P4::Map object. Can be called with either
// one, or two arguments. If one, it's assumed to be a string containing
// either a half-map, or both halves of the mapping.
//
static VALUE p4map_insert( int argc, VALUE *argv, VALUE self )
{
    P4MapMaker *	m = 0;
    StrBuf	t;

    Data_Get_Struct( self, P4MapMaker, m );

    if( argc < 1 || argc > 2 )
	rb_raise( rb_eArgError, "P4::Map#insert takes 1, or 2 arguments" );


    if( argc == 1 )
    {
	// A mapping with only a left hand side.
	m->Insert( *argv );
	return self;
    }

    if( argc == 2 )
    {
	// Separate left- and right-hand strings.
	VALUE left;
	VALUE right;

	left = *argv++;
	right = *argv;

	m->Insert( left, right );
    }
    return self;
}
static VALUE p4map_clear( VALUE self )
{
    P4MapMaker *	m = 0;

    Data_Get_Struct( self, P4MapMaker, m );
    m->Clear();
    return Qtrue;
}

static VALUE p4map_count( VALUE self )
{
    P4MapMaker *	m = 0;

    Data_Get_Struct( self, P4MapMaker, m );
    return INT2NUM( m->Count() );
}

static VALUE p4map_empty( VALUE self )
{
    P4MapMaker *	m = 0;

    Data_Get_Struct( self, P4MapMaker, m );
    return m->Count() ? Qfalse : Qtrue;
}

static VALUE p4map_reverse( VALUE self )
{
    P4MapMaker *	m = 0;
    P4MapMaker *	m2 = 0;
    VALUE		rval;
    VALUE  		argv[ 1 ];

    Data_Get_Struct( self, P4MapMaker, m );
    m2 = new P4MapMaker( *m );
    m2->Reverse();

    rval = Data_Wrap_Struct( cP4Map, 0, p4map_free, m2 );
    rb_obj_call_init( rval, 0, argv );
    return rval;
}

//
// P4::Map#translate( string, fwd=true )
//
static VALUE p4map_trans( int argc, VALUE *argv, VALUE self )
{
    P4MapMaker *	m = 0;
    int			fwd = 1;
    VALUE		string;

    if( argc < 1 || argc > 2 )
	rb_raise( rb_eArgError,
		"Invalid arguments to P4::Map#translate. "
		"Pass the string you wish to translate, and an optional "
		"boolean to indicate whether translation should be in "
		"the forward direction." );

    argc--;
    string = *argv++;

    if( argc && *argv == Qfalse )
	fwd = 0;

    Data_Get_Struct( self, P4MapMaker, m );
    return m->Translate( string, fwd );
}

static VALUE p4map_includes( VALUE self, VALUE string )
{
    P4MapMaker *	m = 0;

    Data_Get_Struct( self, P4MapMaker, m );
    if( m->Translate( string, 1 ) != Qnil )
	return Qtrue;
    if( m->Translate( string, 0 ) != Qnil )
	return Qtrue;
    return Qfalse;
}

static VALUE p4map_lhs( VALUE self )
{
    P4MapMaker *	m = 0;

    Data_Get_Struct( self, P4MapMaker, m );
    return m->Lhs();
}

static VALUE p4map_rhs( VALUE self )
{
    P4MapMaker *	m = 0;

    Data_Get_Struct( self, P4MapMaker, m );
    return m->Rhs();
}

static VALUE p4map_to_a( VALUE self )
{
    P4MapMaker *	m = 0;

    Data_Get_Struct( self, P4MapMaker, m );
    return m->ToA();
}

/*******************************************************************************
 * P4::Message methods. Construction/destruction defined elsewhere
******************************************************************************/
static VALUE p4msg_get_severity( VALUE self )
{
    P4Error *	e = 0;

    Data_Get_Struct( self, P4Error, e );
    return e->GetSeverity();
}

static VALUE p4msg_get_generic( VALUE self )
{
    P4Error *	e = 0;

    Data_Get_Struct( self, P4Error, e );
    return e->GetGeneric();
}

static VALUE p4msg_get_text( VALUE self )
{
    P4Error *	e = 0;

    Data_Get_Struct( self, P4Error, e );
    return e->GetText();
}

static VALUE p4msg_get_dict( VALUE self )
{
    P4Error *   e = 0;

    Data_Get_Struct( self, P4Error, e );
    return e->GetDict();
}

static VALUE p4msg_get_id( VALUE self )
{
    P4Error *	e = 0;

    Data_Get_Struct( self, P4Error, e );
    return e->GetId();
}
static VALUE p4msg_inspect( VALUE self )
{
    P4Error *	e = 0;

    Data_Get_Struct( self, P4Error, e );
    return e->Inspect();
}


/******************************************************************************
 * Extension initialisation
 ******************************************************************************/

void	Init_P4()
{
    // Ruby instantiation
    eP4 = rb_define_class( "P4Exception", rb_eRuntimeError );

    // We ensure this class already exists by loading the version file in P4.rb.
    // If we don't do this, calling things via rake might change the load order
    // in "interesting" ways.
    cP4 = rb_path2class("P4");

    rb_define_singleton_method( cP4, "new", RUBY_METHOD_FUNC(p4_new), 0 );

    // Protocol options
    rb_define_method( cP4, "api_level", RUBY_METHOD_FUNC(p4_get_api_level), 0);
    rb_define_method( cP4, "api_level=", RUBY_METHOD_FUNC(p4_set_api_level), 1);
    rb_define_method( cP4, "streams?", 	RUBY_METHOD_FUNC(p4_get_streams) , 0 );
    rb_define_method( cP4, "streams=", 	RUBY_METHOD_FUNC(p4_set_streams) , 1 );
    rb_define_method( cP4, "tagged",	RUBY_METHOD_FUNC(p4_run_tagged), 1 );
    rb_define_method( cP4, "tagged?",	RUBY_METHOD_FUNC(p4_get_tagged), 0 );
    rb_define_method( cP4, "tagged=",	RUBY_METHOD_FUNC(p4_set_tagged), 1 );
    rb_define_method( cP4, "track?", 	RUBY_METHOD_FUNC(p4_get_track)   , 0 );
    rb_define_method( cP4, "track=", 	RUBY_METHOD_FUNC(p4_set_track)   , 1 );
    rb_define_method( cP4, "graph?",  RUBY_METHOD_FUNC(p4_get_graph) , 0 );
    rb_define_method( cP4, "graph=",  RUBY_METHOD_FUNC(p4_set_graph) , 1 );


    // Perforce client settings.
    //
    rb_define_method( cP4, "charset", 	RUBY_METHOD_FUNC(p4_get_charset) , 0 );
    rb_define_method( cP4, "charset=", 	RUBY_METHOD_FUNC(p4_set_charset) , 1 );
    rb_define_method( cP4, "cwd", 	RUBY_METHOD_FUNC(p4_get_cwd)     , 0 );
    rb_define_method( cP4, "cwd=", 	RUBY_METHOD_FUNC(p4_set_cwd)     , 1 );
    rb_define_method( cP4, "client", 	RUBY_METHOD_FUNC(p4_get_client)  , 0 );
    rb_define_method( cP4, "client=", 	RUBY_METHOD_FUNC(p4_set_client)  , 1 );
    rb_define_method( cP4, "env", 	RUBY_METHOD_FUNC(p4_get_env)     , 1 );
    rb_define_method( cP4, "set_env", 	RUBY_METHOD_FUNC(p4_set_env)     , 2 );
    rb_define_method( cP4, "enviro_file", RUBY_METHOD_FUNC(p4_get_enviro_file), 0);
    rb_define_method( cP4, "enviro_file=", RUBY_METHOD_FUNC(p4_set_enviro_file), 1);
    rb_define_method( cP4, "evar", 	RUBY_METHOD_FUNC(p4_get_evar)     , 1 );
    rb_define_method( cP4, "set_evar", 	RUBY_METHOD_FUNC(p4_set_evar)     , 2 );
    rb_define_method( cP4, "host", 	RUBY_METHOD_FUNC(p4_get_host)    , 0 );
    rb_define_method( cP4, "host=", 	RUBY_METHOD_FUNC(p4_set_host)    , 1 );
    rb_define_method( cP4, "ignore_file",RUBY_METHOD_FUNC(p4_get_ignore) , 0 );
    rb_define_method( cP4, "ignore_file=",RUBY_METHOD_FUNC(p4_set_ignore), 1 );
    rb_define_method( cP4, "ignored?",  RUBY_METHOD_FUNC(p4_is_ignored)  , 1 );
    rb_define_method( cP4, "language",  RUBY_METHOD_FUNC(p4_get_language), 0 );
    rb_define_method( cP4, "language=", RUBY_METHOD_FUNC(p4_set_language), 1 );
    rb_define_method( cP4, "p4config_file",RUBY_METHOD_FUNC(p4_get_p4config),0);
    rb_define_method( cP4, "password", RUBY_METHOD_FUNC(p4_get_password), 0 );
    rb_define_method( cP4, "password=", RUBY_METHOD_FUNC(p4_set_password), 1 );
    rb_define_method( cP4, "port", 	RUBY_METHOD_FUNC(p4_get_port)    , 0 );
    rb_define_method( cP4, "port=", 	RUBY_METHOD_FUNC(p4_set_port)    , 1 );
    rb_define_method( cP4, "prog", 	RUBY_METHOD_FUNC(p4_get_prog)    , 0 );
    rb_define_method( cP4, "prog=", 	RUBY_METHOD_FUNC(p4_set_prog)    , 1 );
    rb_define_method( cP4, "protocol", 	RUBY_METHOD_FUNC(p4_set_protocol), 2 );
    rb_define_method( cP4, "ticket_file", RUBY_METHOD_FUNC(p4_get_ticket_file), 0 );
    rb_define_method( cP4, "ticket_file=", RUBY_METHOD_FUNC(p4_set_ticket_file), 1 );
    rb_define_method( cP4, "trust_file", RUBY_METHOD_FUNC(p4_get_trust_file), 0 );
    rb_define_method( cP4, "trust_file=", RUBY_METHOD_FUNC(p4_set_trust_file), 1 );
    rb_define_method( cP4, "user", 	RUBY_METHOD_FUNC(p4_get_user)    , 0 );
    rb_define_method( cP4, "user=", 	RUBY_METHOD_FUNC(p4_set_user)    , 1 );
    rb_define_method( cP4, "version", 	RUBY_METHOD_FUNC(p4_get_version) , 0 );
    rb_define_method( cP4, "version=", 	RUBY_METHOD_FUNC(p4_set_version) , 1 );


    rb_define_method( cP4, "maxresults", RUBY_METHOD_FUNC(p4_get_maxresults),0);
    rb_define_method( cP4, "maxresults=",RUBY_METHOD_FUNC(p4_set_maxresults),1);
    rb_define_method( cP4, "maxscanrows", RUBY_METHOD_FUNC(p4_get_maxscanrows),0);
    rb_define_method( cP4, "maxscanrows=",RUBY_METHOD_FUNC(p4_set_maxscanrows), 1 );
    rb_define_method( cP4, "maxlocktime", RUBY_METHOD_FUNC(p4_get_maxlocktime), 0 );
    rb_define_method( cP4, "maxlocktime=", RUBY_METHOD_FUNC(p4_set_maxlocktime), 1 );

    // Session Connect/Disconnect
    rb_define_method( cP4, "connect", 	RUBY_METHOD_FUNC(p4_connect)     , 0 );
    rb_define_method( cP4, "connected?",RUBY_METHOD_FUNC(p4_connected)   , 0 );
    rb_define_method( cP4, "disconnect", RUBY_METHOD_FUNC(p4_disconnect) , 0 );

    // Running commands - general purpose commands
    rb_define_method( cP4, "run", 	RUBY_METHOD_FUNC(p4_run)         ,-2 );
    rb_define_method( cP4, "input=", 	RUBY_METHOD_FUNC(p4_set_input)   , 1 );
    rb_define_method( cP4, "errors", 	RUBY_METHOD_FUNC(p4_get_errors)  , 0 );
    rb_define_method( cP4, "messages",	RUBY_METHOD_FUNC(p4_get_messages), 0 );
    rb_define_method( cP4, "warnings",	RUBY_METHOD_FUNC(p4_get_warnings), 0 );
    rb_define_method( cP4, "exception_level", RUBY_METHOD_FUNC(p4_get_except_level), 0 );
    rb_define_method( cP4, "exception_level=", RUBY_METHOD_FUNC(p4_set_except_level), 1 );
    rb_define_method( cP4, "server_level", RUBY_METHOD_FUNC(p4_get_server_level), 0 );
    rb_define_method( cP4, "server_case_sensitive?", RUBY_METHOD_FUNC(p4_server_case_sensitive), 0 );
    rb_define_method( cP4, "track_output",	RUBY_METHOD_FUNC(p4_track_output), 0 );

    rb_define_method( cP4, "server_unicode?", RUBY_METHOD_FUNC(p4_server_unicode), 0 );

    // Spec parsing
    rb_define_method( cP4, "parse_spec", RUBY_METHOD_FUNC(p4_parse_spec), 2 );
    rb_define_method( cP4, "format_spec", RUBY_METHOD_FUNC(p4_format_spec), 2 );

    // Identification
    rb_define_const( cP4, "P4API_VERSION", P4Utils::ruby_string(P4APIVER_STRING));
    rb_define_const( cP4, "P4API_PATCHLEVEL", INT2NUM(P4API_PATCHLEVEL));
    rb_define_const( cP4, "P4RUBY_VERSION", P4Utils::ruby_string(P4RUBY_VERSION) );
    rb_define_singleton_method( cP4, "identify", RUBY_METHOD_FUNC(p4_identify), 0 );

    // Debugging support
    rb_define_method( cP4, "debug", RUBY_METHOD_FUNC(p4_get_debug), 0);
    rb_define_method( cP4, "debug=", RUBY_METHOD_FUNC(p4_set_debug), 1 );

    // Support for OutputHandler
    rb_define_method( cP4, "handler", RUBY_METHOD_FUNC(p4_get_handler), 0);
    rb_define_method( cP4, "handler=", RUBY_METHOD_FUNC(p4_set_handler), 1);

    // Support for Progress API
    rb_define_method( cP4, "progress", RUBY_METHOD_FUNC(p4_get_progress), 0);
    rb_define_method( cP4, "progress=", RUBY_METHOD_FUNC(p4_set_progress), 1);

    // SSO handling
    rb_define_method( cP4, "loginsso", RUBY_METHOD_FUNC(p4_get_enabled_sso), 0);
    rb_define_method( cP4, "loginsso=", RUBY_METHOD_FUNC(p4_set_enable_sso), 1);
    rb_define_method( cP4, "ssovars", RUBY_METHOD_FUNC(p4_get_sso_vars), 0);
    rb_define_method( cP4, "ssopassresult", RUBY_METHOD_FUNC(p4_get_sso_passresult), 0);
    rb_define_method( cP4, "ssopassresult=", RUBY_METHOD_FUNC(p4_set_sso_passresult), 1);
    rb_define_method( cP4, "ssofailresult", RUBY_METHOD_FUNC(p4_get_sso_failresult), 0);
    rb_define_method( cP4, "ssofailresult=", RUBY_METHOD_FUNC(p4_set_sso_failresult), 1);
    rb_define_method( cP4, "ssohandler", RUBY_METHOD_FUNC(p4_get_ssohandler), 0);
    rb_define_method( cP4, "ssohandler=", RUBY_METHOD_FUNC(p4_set_ssohandler), 1);


    // P4::MergeData class
    cP4MD = rb_define_class_under( cP4, "MergeData", rb_cObject );

    rb_define_method( cP4MD, "your_name", RUBY_METHOD_FUNC(p4md_getyourname),0);
    rb_define_method( cP4MD, "their_name", RUBY_METHOD_FUNC(p4md_gettheirname),0);
    rb_define_method( cP4MD, "base_name", RUBY_METHOD_FUNC(p4md_getbasename),0);
    rb_define_method( cP4MD, "your_path", RUBY_METHOD_FUNC(p4md_getyourpath),0);
    rb_define_method( cP4MD, "their_path", RUBY_METHOD_FUNC(p4md_gettheirpath),0);
    rb_define_method( cP4MD, "base_path", RUBY_METHOD_FUNC(p4md_getbasepath),0);
    rb_define_method( cP4MD, "result_path", RUBY_METHOD_FUNC(p4md_getresultpath),0);
    rb_define_method( cP4MD, "merge_hint", RUBY_METHOD_FUNC(p4md_getmergehint),0);
    rb_define_method( cP4MD, "run_merge", RUBY_METHOD_FUNC(p4md_runmerge),0);

    rb_define_method(cP4MD, "action_resolve?", RUBY_METHOD_FUNC(p4md_getactionresolve), 0);
    rb_define_method(cP4MD, "action_type", RUBY_METHOD_FUNC(p4md_getactiontype), 0);
    rb_define_method(cP4MD, "content_resolve?", RUBY_METHOD_FUNC(p4md_getcontentresolve), 0);
    rb_define_method(cP4MD, "info", RUBY_METHOD_FUNC(p4md_getinfo), 0);
    rb_define_method(cP4MD, "invalidate", RUBY_METHOD_FUNC(p4md_invalidate), 0);
    rb_define_method(cP4MD, "merge_action", RUBY_METHOD_FUNC(p4md_getmergeaction), 0);
    rb_define_method(cP4MD, "their_action", RUBY_METHOD_FUNC(p4md_gettheiraction), 0);
    rb_define_method(cP4MD, "to_s", RUBY_METHOD_FUNC(p4md_tos), 0);
    rb_define_method(cP4MD, "yours_action", RUBY_METHOD_FUNC(p4md_getyoursaction), 0);

    // P4::Map class
    cP4Map = rb_define_class_under( cP4, "Map", rb_cObject );
    rb_define_singleton_method( cP4Map, "new", RUBY_METHOD_FUNC(p4map_new), -1);
    rb_define_singleton_method( cP4Map, "join", RUBY_METHOD_FUNC(p4map_join), 2 );
    rb_define_method( cP4Map, "insert", RUBY_METHOD_FUNC(p4map_insert),-1);
    rb_define_method( cP4Map, "inspect", RUBY_METHOD_FUNC(p4map_inspect),0);
    rb_define_method( cP4Map, "clear", RUBY_METHOD_FUNC(p4map_clear),0);
    rb_define_method( cP4Map, "count", RUBY_METHOD_FUNC(p4map_count),0);
    rb_define_method( cP4Map, "empty?", RUBY_METHOD_FUNC(p4map_empty),0);
    rb_define_method( cP4Map, "translate", RUBY_METHOD_FUNC(p4map_trans),-1);
    rb_define_method( cP4Map, "reverse", RUBY_METHOD_FUNC(p4map_reverse),0);
    rb_define_method( cP4Map, "includes?", RUBY_METHOD_FUNC(p4map_includes),1);
    rb_define_method( cP4Map, "lhs", RUBY_METHOD_FUNC(p4map_lhs),0);
    rb_define_method( cP4Map, "rhs", RUBY_METHOD_FUNC(p4map_rhs),0);
    rb_define_method( cP4Map, "to_a", RUBY_METHOD_FUNC(p4map_to_a),0);

    // P4::Message class.
    cP4Msg = rb_define_class_under( cP4, "Message", rb_cObject );
    rb_define_method( cP4Msg, "inspect", RUBY_METHOD_FUNC(p4msg_inspect),0);
    rb_define_method( cP4Msg, "msgid", RUBY_METHOD_FUNC(p4msg_get_id), 0);
    rb_define_method( cP4Msg, "severity", RUBY_METHOD_FUNC(p4msg_get_severity), 0);
    rb_define_method( cP4Msg, "generic", RUBY_METHOD_FUNC(p4msg_get_generic), 0);
    rb_define_method( cP4Msg, "dictionary", RUBY_METHOD_FUNC(p4msg_get_dict), 0);
    rb_define_method( cP4Msg, "to_s", RUBY_METHOD_FUNC(p4msg_get_text), 0);

    //	P4::Progress class.
    cP4Prog = rb_define_class_under( cP4, "Progress", rb_cObject );

};


} // Extern C
