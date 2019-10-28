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
 * Name		: p4result.cc
 *
 * Author	: Tony Smith <tony@perforce.com> or <tony@smee.org>
 *
 * Description	: Ruby class for holding results of Perforce commands 
 *
 ******************************************************************************/

#include <ruby.h>
#include "undefdups.h"
#include <p4/clientapi.h>
#include "gc_hack.h"
#include "p4error.h"
#include "p4utils.h"
#include "p4result.h"

P4Result::P4Result()
{
    output = rb_ary_new();
    warnings = rb_ary_new();
    errors = rb_ary_new();
    messages = rb_ary_new();
    track = rb_ary_new();
    apiLevel = atoi( P4Tag::l_client );
    ID	idP4 	= rb_intern( "P4" );
    ID	idP4Msg	= rb_intern( "Message" );

    VALUE cP4	= rb_const_get_at( rb_cObject, idP4 );
    cP4Msg	= rb_const_get_at( cP4, idP4Msg );

}


void
P4Result::Reset()
{
    output = rb_ary_new();
    warnings = rb_ary_new();
    errors = rb_ary_new();
    messages = rb_ary_new();
    track = rb_ary_new();
}

//
// Direct output - not via a message of any kind. For example,
// binary output.
//
void
P4Result::AddOutput( VALUE v )
{
    rb_ary_push( output, v );

    //
    // Call the ruby thread scheduler to allow another thread to run
    // now. We should perhaps only call this every 'n' times, but I've
    // no idea what a good value for n might be; so for now at least, n=1
    //
    rb_thread_schedule();
}

/*
 * Main distribution of output to the user. This method sorts the
 * output into groups: output, warnings, errors, and sends all output
 * (regardless of severity) to the messages array.
 */
void
P4Result::AddMessage( Error *e )
{

    int s;
    s = e->GetSeverity();

    // 
    // Empty and informational messages are pushed out as output as nothing
    // worthy of error handling has occurred. Warnings go into the warnings
    // list and the rest are lumped together as errors.
    //

    if ( s == E_EMPTY || s == E_INFO )
	rb_ary_push( output, FmtMessage( e ) );
    else if ( s == E_WARN )
	rb_ary_push( warnings, FmtMessage( e ) );
    else
	rb_ary_push( errors, FmtMessage( e ) );

    //
    // Whatever severity, format the message into the messages array, wrapped
    // up in a P4::Message object.
    //
    rb_ary_push( messages, WrapMessage( e ) );

    //
    // Call the ruby thread scheduler to allow another thread to run
    // now. We should perhaps only call this every 'n' times, but I've 
    // no idea what a good value for n might be; so for now at least, n=1
    //
    rb_thread_schedule();
}

void
P4Result::AddTrack( const char *msg )
{
    rb_ary_push( track,  P4Utils::ruby_string( msg ) );

    //
    // Call the ruby thread scheduler to allow another thread to run
    // now. We should perhaps only call this every 'n' times, but I've 
    // no idea what a good value for n might be; so for now at least, n=1
    //
    rb_thread_schedule();
}

void
P4Result::DeleteTrack()
{
    rb_ary_clear( track );

    //
    // Call the ruby thread scheduler to allow another thread to run
    // now. We should perhaps only call this every 'n' times, but I've
    // no idea what a good value for n might be; so for now at least, n=1
    //
    rb_thread_schedule();
}

void
P4Result::AddTrack( VALUE t )
{
    rb_ary_push( track, t );

    //
    // Call the ruby thread scheduler to allow another thread to run
    // now. We should perhaps only call this every 'n' times, but I've 
    // no idea what a good value for n might be; so for now at least, n=1
    //
    rb_thread_schedule();
}

int
P4Result::ErrorCount()
{
    return Length( errors );
}

int
P4Result::WarningCount()
{
    return Length( warnings );
}

void
P4Result::FmtErrors( StrBuf &buf )
{
    Fmt( "[Error]: ", errors, buf );
}

void
P4Result::FmtWarnings( StrBuf &buf )
{
    Fmt( "[Warning]: ", warnings, buf );
}


int
P4Result::Length( VALUE ary )
{
    ID		iLength;
    VALUE 	len;

    iLength = rb_intern( "length" );

    len = rb_funcall( ary, iLength, 0 );
    return NUM2INT( len );
}

void
P4Result::GCMark()
{
    rb_gc_mark( output );
    rb_gc_mark( errors );
    rb_gc_mark( warnings );
    rb_gc_mark( messages );
    rb_gc_mark( track );
}


void
P4Result::Fmt( const char *label, VALUE ary, StrBuf &buf )
{
    ID		idJoin;
    VALUE	s1;
    StrBuf	csep;
    VALUE	rsep;

    buf.Clear();
    // If the array is empty, then we just return
    if( ! Length( ary ) ) return;

    // Not empty, so we'll format it. 
    idJoin 	= rb_intern( "join" );
 
    // This is the string we'll use to prefix each entry in the array
    csep << "\n\t" << label;
    rsep =  P4Utils::ruby_string( csep.Text() );

    // Since Array#join() won't prefix the first element with the separator
    // we'll have to do it manually.
    buf << csep;

    // Join the array elements together, and append the result to the buffer
    s1 		= rb_funcall( ary, idJoin, 1, rsep );
    buf << StringValuePtr( s1 );

    return;
}

VALUE
P4Result::FmtMessage( Error *e )
{
    StrBuf t;
    e->Fmt( t, EF_PLAIN );
    return  P4Utils::ruby_string( t.Text(), t.Length() );
}

VALUE
P4Result::WrapMessage( Error *e )
{
    P4Error *pe = new P4Error( *e );
    return pe->Wrap( cP4Msg );
}
