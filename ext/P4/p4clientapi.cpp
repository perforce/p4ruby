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
 * Name		: p4clientapi.cc
 *
 * Author	: Tony Smith <tony@perforce.com> or <tony@smee.org>
 *
 * Description	: Ruby bindings for the Perforce API. Main interface to the
 * 		  Perforce API.
 *
 ******************************************************************************/
#include <ruby.h>
#include "undefdups.h"
#include <p4/clientapi.h>
#include <p4/strtable.h>
#include <p4/i18napi.h>
#include <p4/enviro.h>
#include <p4/hostenv.h>
#include <p4/spec.h>
#include <p4/ignore.h>
#include <p4/debug.h>
#include "p4result.h"
#include "p4rubydebug.h"
#include "clientuserruby.h"
#include "specmgr.h"
#include "p4clientapi.h"
#include "p4utils.h"



/*******************************************************************************
 * Our Ruby classes.
 ******************************************************************************/
extern VALUE 	cP4;	// Base P4 class
extern VALUE 	eP4;	// Exception class


P4ClientApi::P4ClientApi() : ui( &specMgr )
{
    debug = 0;
    server2 = 0;
    depth = 0;
    exceptionLevel = 2;
    maxResults = 0;
    maxScanRows = 0;
    maxLockTime = 0;
    InitFlags();
    apiLevel = atoi( P4Tag::l_client );
    enviro = new Enviro;
    prog = "unnamed p4ruby script";

    client.SetProtocol( "specstring", "" );

    //
    // Load any P4CONFIG file
    //
    HostEnv henv;
    StrBuf cwd;

    henv.GetCwd( cwd, enviro );
    if( cwd.Length() )
	enviro->Config( cwd );

    //
    // Load the current ticket file. Start with the default, and then
    // override it if P4TICKETS is set.
    //
    const char *t;

    henv.GetTicketFile( ticketFile );

    if( (t = enviro->Get("P4TICKETS")) )
	   ticketFile = t;

    //
    // Load the current trust file. Start with the default, and then
    // override it if P4TRUST is set.
    //

    henv.GetTrustFile( trustFile );

    if( (t = enviro->Get("P4TICKETS")) )
        trustFile = t;

    //
    // Load the current P4CHARSET if set.
    //
    if( client.GetCharset().Length() )
	SetCharset( client.GetCharset().Text() );
}

P4ClientApi::~P4ClientApi()
{
    if ( IsConnected() )
    {
	Error e;
	client.Final( &e );
	// Ignore errors
    }
    delete enviro;
}

const char *
P4ClientApi::GetEnv( const char *v)
{
    return enviro->Get( v );
}

void
P4ClientApi::SetEnviroFile( const char *c )
{
    enviro->SetEnviroFile(c);
}

const StrPtr *
P4ClientApi::GetEnviroFile()
{
    return enviro->GetEnviroFile();
}

void
P4ClientApi::SetEVar( const char *var, const char *val )
{
    StrRef sVar( var );
    StrRef sVal( val );
    client.SetEVar( sVar, sVal );
}

void
P4ClientApi::SetVar( const char *var, const char *val )
{
    StrRef sVar( var );
    StrRef sVal( val );
    client.SetVar( sVar, sVal );
}

const StrPtr *
P4ClientApi::GetEVar( const char *var )
{
    StrRef sVar( var );
    return client.GetEVar( sVar );
}

void
P4ClientApi::SetApiLevel( int level )
{
    StrBuf	b;
    b << level;
    apiLevel = level;
    client.SetProtocol( "api", b.Text() );
    ui.SetApiLevel( level );
}

int
P4ClientApi::SetCharset( const char *c )
{
    StrRef cs_none( "none" );

    if( P4RDB_COMMANDS )
	fprintf( stderr, "[P4] Setting charset: %s\n", c );

    if( c && cs_none != c )
    {
	CharSetApi::CharSet cs = CharSetApi::Lookup( c );
	if( cs < 0 )
	{
	    StrBuf	m;
	    m = "Unknown or unsupported charset: ";
	    m.Append( c );
	    Except( "P4#charset=", m.Text() );
	}
#ifdef HAVE_RUBY_ENCODING_H
	CharSetApi::CharSet utf8 = CharSetApi::Lookup( "utf8" );
	client.SetTrans( utf8, cs, utf8, utf8 );
#else
	client.SetTrans( cs, cs, cs, cs );
#endif
	client.SetCharset( c );
	P4Utils::SetCharset( c );
    }
    else
    {
	// Disables automatic unicode detection if called
	// prior to init (2014.2)
	client.SetTrans( 0 );
    }
    return 1;
}

void
P4ClientApi::SetCwd( const char *c )
{
    client.SetCwd( c );
    enviro->Config( StrRef( c ) );
}

void
P4ClientApi::SetTicketFile( const char *p )
{
    client.SetTicketFile( p );
    ticketFile = p;
}

void
P4ClientApi::SetTrustFile( const char *p )
{
    client.SetTrustFile( p );
    trustFile = p;
}

void
P4ClientApi::SetDebug( int d )
{
    debug = d;
    ui.SetDebug( d );
    specMgr.SetDebug( d );

    if( P4RDB_RPC )
        p4debug.SetLevel( "rpc=5" );
    else
        p4debug.SetLevel( "rpc=0" );

	if( P4RDB_SSL )
	    p4debug.SetLevel( "ssl=3" );
	else
	    p4debug.SetLevel( "ssl=0" );
}

void
P4ClientApi::SetArrayConversion( int i )
{
    specMgr.SetArrayConversion( i );
}

void
P4ClientApi::SetProtocol( const char *var, const char *val )
{
   client.SetProtocol( var, val );
}

VALUE
P4ClientApi::SetEnv( const char *var, const char *val )
{
    Error e;

    enviro->Set( var, val, &e );
    if( e.Test() && exceptionLevel )
    {
        Except( "P4#set_env", &e );
    }

    if( e.Test() )
        return Qfalse;

    // Fixes an issue on OS X where the next enviro->Get doesn't return the
    // cached value
    enviro->Reload();

    return Qtrue;
}

//
// connect to the Perforce server.
//

VALUE
P4ClientApi::Connect()
{
    if ( P4RDB_COMMANDS )
	fprintf( stderr, "[P4] Connecting to Perforce\n" );

    if ( IsConnected() )
    {
	rb_warn( "P4#connect - Perforce client already connected!" );
	return Qtrue;
    }

    return ConnectOrReconnect();
}

VALUE
P4ClientApi::ConnectOrReconnect()
{
    if ( IsTrackMode() )
	client.SetProtocol( "track", "" );

    Error	e;

    ResetFlags();
    client.Init( &e );
    if ( e.Test() && exceptionLevel )
	Except( "P4#connect", &e );

    if ( e.Test() )
	return Qfalse;

    // If a handler is defined, reset the break functionality
    // for the KeepAlive function

    if( ui.GetHandler() != Qnil )
    {
	client.SetBreak( &ui );
    }

    SetConnected();
    return Qtrue;
}


//
// Disconnect session
//
VALUE
P4ClientApi::Disconnect()
{
    if ( P4RDB_COMMANDS )
	fprintf( stderr, "[P4] Disconnect\n" );

    if ( !IsConnected() )
    {
	rb_warn( "P4#disconnect - not connected" );
	return Qtrue;
    }
    Error	e;
    client.Final( &e );
    ResetFlags();

    // Clear the specdef cache.
    specMgr.Reset();

    // Clear out any results from the last command
    ui.Reset();

    return Qtrue;
}

//
// Test whether or not connected
//
VALUE
P4ClientApi::Connected()
{
    if( IsConnected() && !client.Dropped() )
	return Qtrue;
    else if( IsConnected() )
	Disconnect();
    return Qfalse;
}

void
P4ClientApi::Tagged( int enable )
{
    if( enable )
	SetTag();
    else
	ClearTag();
}

int P4ClientApi::SetTrack( int enable )
{
    if ( IsConnected() ) {
		if( exceptionLevel )
		{
			Except( "P4#track=", "Can't change performance tracking once you've connected.");
		}
		return Qfalse;
    }
    else if ( enable ) {
	SetTrackMode();
	ui.SetTrack(true);
    }
    else {
	ClearTrackMode();
	ui.SetTrack(false);
    }
    return Qtrue;
}

void P4ClientApi::SetStreams( int enable )
{
    if ( enable )
       SetStreamsMode();
    else
       ClearStreamsMode();
}

void P4ClientApi::SetGraph( int enable )
{
    if ( enable )
       SetGraphMode();
    else
       ClearGraphMode();
}

int
P4ClientApi::GetServerLevel()
{
    if( !IsConnected() )
	Except( "server_level", "Not connected to a Perforce Server.");
    if( !IsCmdRun() )
	Run( "info", 0, 0 );
    return server2;
}

int
P4ClientApi::ServerCaseSensitive()
{
    if( !IsConnected() )
	Except( "server_case_sensitive?", "Not connected to a Perforce Server.");
    if( !IsCmdRun() )
	Run( "info", 0, 0);
    return !IsCaseFold();
}

int
P4ClientApi::ServerUnicode()
{
    if( !IsConnected() )
	Except( "server_unicode?", "Not connected to a Perforce Server.");
    if( !IsCmdRun() )
	Run( "info", 0, 0);
    return IsUnicode();
}


// Check if the supplied path falls within the view of the ignore file
int
P4ClientApi::IsIgnored( const char *path )
{
    Ignore *ignore = client.GetIgnore();
    if( !ignore ) return 0;

    StrRef p( path );
    return ignore->Reject( p, client.GetIgnoreFile() );
}

//
// Run returns the results of the command. If the client has not been
// connected, then an exception is raised but errors from Perforce
// commands are returned via the Errors() and ErrorCount() interfaces
// and not via exceptions because one failure in a command applied to many
// files would interrupt processing of all the other files if an exception
// is raised.
//

VALUE
P4ClientApi::Run( const char *cmd, int argc, char * const *argv )
{
    // Save the entire command string for our error messages. Makes it
    // easy to see where a script has gone wrong.
    StrBuf	cmdString;
    cmdString << "\"p4 " << cmd;
    for( int i = 0; i < argc; i++ )
        cmdString << " " << argv[ i ];
    cmdString << "\"";

    if ( P4RDB_COMMANDS )
	fprintf( stderr, "[P4] Executing %s\n", cmdString.Text()  );

    if ( depth )
    {
	rb_warn( "Can't execute nested Perforce commands." );
	return Qfalse;
    }

    // Clear out any results from the previous command
    ui.Reset();

    if ( !IsConnected() && exceptionLevel )
	Except( "P4#run", "not connected." );

    if ( !IsConnected() )
	return Qfalse;

    // Tell the UI which command we're running.
    ui.SetCommand( cmd );

    depth++;
    RunCmd( cmd, &ui, argc, argv );
    depth--;

    if( ui.GetHandler() != Qnil) {
	if( client.Dropped() && ! ui.IsAlive() ) {
	    Disconnect();
	    ConnectOrReconnect();
	}
    }

    ui.RaiseRubyException();

    P4Result &results = ui.GetResults();

    if ( results.ErrorCount() && exceptionLevel )
	Except( "P4#run", "Errors during command execution", cmdString.Text() );

    if ( results.WarningCount() && exceptionLevel > 1 )
	Except( "P4#run", "Warnings during command execution",cmdString.Text());

    return results.GetOutput();
}


void
P4ClientApi::RunCmd( const char *cmd, ClientUser *ui, int argc, char * const *argv )
{
    client.SetProg( &prog );
    if( version.Length() )
	client.SetVersion( &version );

    if( IsTag() )
	client.SetVar( "tag" );

    if ( IsStreams() && apiLevel > 69 )
	client.SetVar( "enableStreams", "" );

    if ( IsGraph() && apiLevel > 81 )
    client.SetVar( "enableGraph", "" );

    // If maxresults or maxscanrows is set, enforce them now
    if( maxResults  )	client.SetVar( "maxResults",  maxResults  );
    if( maxScanRows )	client.SetVar( "maxScanRows", maxScanRows );
    if( maxLockTime )	client.SetVar( "maxLockTime", maxLockTime );

    //	If progress is set, set progress var.
    if( ( (ClientUserRuby*)ui)->GetProgress() != Qnil ) client.SetVar( P4Tag::v_progress, 1 );

    client.SetArgv( argc, argv );
    client.Run( cmd, ui );

    // Can only read the protocol block *after* a command has been run.
    // Do this once only.
    if( !IsCmdRun() )
    {
	StrPtr *s = 0;
	if ( (s = client.GetProtocol(P4Tag::v_server2)) )
	    server2 = s->Atoi();

	if( (s = client.GetProtocol(P4Tag::v_unicode)) )
	    if( s->Atoi() )
		SetUnicode();

	if( (s = client.GetProtocol(P4Tag::v_nocase)) )
	    SetCaseFold();
    }
    SetCmdRun();
}


//
// Parses a string supplied by the user into a hash. To do this we need
// the specstring from the server. We try to cache those as we see them,
// but the user may not have executed any commands to allow us to cache
// them so we may have to fetch the spec first.
//

VALUE
P4ClientApi::ParseSpec( const char * type, const char *form )
{
    if ( !specMgr.HaveSpecDef( type ) )
    {
	if( exceptionLevel )
	{
	    StrBuf m;
	    m = "No spec definition for ";
	    m.Append( type );
	    m.Append( " objects." );
	    Except( "P4#parse_spec", m.Text() );
	}
	else
	{
	    return Qfalse;
	}
    }

    // Got a specdef so now we can attempt to parse it.
    Error e;
    VALUE v;
    v = specMgr.StringToSpec( type, form, &e );

    if ( e.Test() )
    {
	if( exceptionLevel )
	    Except( "P4#parse_spec", &e );
	else
	    return Qfalse;
    }

    return v;
}


//
// Converts a hash supplied by the user into a string using the specstring
// from the server. We may have to fetch the specstring first.
//

VALUE
P4ClientApi::FormatSpec( const char * type, VALUE hash )
{
    if ( !specMgr.HaveSpecDef( type ) )
    {
	if( exceptionLevel )
	{
	    StrBuf m;
	    m = "No spec definition for ";
	    m.Append( type );
	    m.Append( " objects." );
	    Except( "P4#format_spec", m.Text() );
	}
	else
	{
	    return Qfalse;
	}
    }

    // Got a specdef so now we can attempt to convert.
    StrBuf	buf;
    Error	e;

    specMgr.SpecToString( type, hash, buf, &e );
    if( !e.Test() )
	return P4Utils::ruby_string( buf.Text() );

    if( exceptionLevel )
    {
	StrBuf m;
	m = "Error converting hash to a string.";
	if( e.Test() ) e.Fmt( m, EF_PLAIN );
	Except( "P4#format_spec", m.Text() );
    }
    return Qnil;
}

//
// Returns a hash whose keys contain the names of the fields in a spec of the
// specified type. Not yet exposed to Ruby clients, but may be in future.
//
VALUE
P4ClientApi::SpecFields( const char * type )
{
    if ( !specMgr.HaveSpecDef( type ) )
    {
	if( exceptionLevel )
	{
	    StrBuf m;
	    m = "No spec definition for ";
	    m.Append( type );
	    m.Append( " objects." );
	    Except( "P4#spec_fields", m.Text() );
	}
	else
	{
	    return Qfalse;
	}
    }

    return specMgr.SpecFields( type );
}

//
// Raises an exception or returns Qfalse on bad input
//

VALUE
P4ClientApi::SetInput( VALUE input )
{
    if ( P4RDB_COMMANDS )
	fprintf( stderr, "[P4] Received input for next command\n" );

    if ( ! ui.SetInput( input ) )
    {
	if ( exceptionLevel )
	    Except( "P4#input", "Error parsing supplied data." );
    	else
	    return Qfalse;
    }
    return Qtrue;
}

//
// Sets the handler and connects the SetBreak feature
//
VALUE
P4ClientApi::SetHandler( VALUE handler )
{
    if ( P4RDB_COMMANDS )
        fprintf( stderr, "[P4] Received handler object\n" );

    ui.SetHandler( handler );

    if( handler == Qnil)
	client.SetBreak(NULL);
    else
	client.SetBreak(&ui);

    return Qtrue;
}

VALUE
P4ClientApi::SetProgress( VALUE progress ) {
    if ( P4RDB_COMMANDS )
        fprintf( stderr, "[P4] Received progress object\n" );

    return ui.SetProgress( progress );
}

VALUE
P4ClientApi::SetSSOHandler( VALUE h )
{
    if ( P4RDB_COMMANDS )
        fprintf( stderr, "[P4] Received SSO handler object\n" );

    ui.SetRubySSOHandler( h );

    return Qtrue;
}


void
P4ClientApi::GCMark()
{
    if ( P4RDB_GC )
	fprintf( stderr, "[P4] Ruby asked us to do garbage collection\n" );

    // We don't hold Ruby objects. But our UI does.
    ui.GCMark();
}

void
P4ClientApi::Except( const char *func, const char *msg )
{
    StrBuf	m;
    StrBuf	errors;
    StrBuf	warnings;
    int		terminate = 0;

    m << "[" << func << "] " << msg;

    // Now append any errors and warnings to the text
    ui.GetResults().FmtErrors( errors );
    ui.GetResults().FmtWarnings( warnings );

    if( errors.Length() )
    {
	m << "\n" << errors;
	terminate++;
    }

    if( exceptionLevel > 1 && warnings.Length() )
    {
	m << "\n" << warnings;
	terminate++;
    }

    if( terminate )
	m << "\n\n";

    rb_raise( eP4, "%s", m.Text() );
}

void
P4ClientApi::Except( const char *func, const char *msg, const char *cmd )
{
    StrBuf m;

    m << msg;
    m << "( " << cmd << " )";
    Except( func, m.Text() );
}

void
P4ClientApi::Except( const char *func, Error *e )
{
    StrBuf	m;

    e->Fmt( &m );
    Except( func, m.Text() );
}

//
// SSO Handlers
//

VALUE
P4ClientApi::SetEnableSSO( VALUE e )
{
    return ui.EnableSSO( e );
}

VALUE
P4ClientApi::GetEnableSSO()
{
    return ui.SSOEnabled();
}

VALUE
P4ClientApi::GetSSOVars()
{
    return ui.GetSSOVars();
}

VALUE
P4ClientApi::SetSSOPassResult( VALUE r )
{
    return ui.SetSSOPassResult( r );
}

VALUE
P4ClientApi::GetSSOPassResult()
{
    return ui.GetSSOPassResult();
}

VALUE
P4ClientApi::SetSSOFailResult( VALUE r )
{
   return ui.SetSSOFailResult( r );
}

VALUE
P4ClientApi::GetSSOFailResult()
{
    return ui.GetSSOFailResult();
}
