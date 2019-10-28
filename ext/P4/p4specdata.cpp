/*******************************************************************************

Copyright (c) 2009, Perforce Software, Inc.  All rights reserved.

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
 * Name		: p4specdata.cpp
 *
 * Author	: Tony Smith <tony@perforce.com> or <tony@smee.org>
 *
 * Description	: Ruby bindings for the Perforce API. SpecData subclass for
 * 		  P4Ruby. This class allows for manipulation of Spec data
 * 		  stored in a Ruby hash using the standard Perforce classes
 *
 ******************************************************************************/

#include <ruby.h>
#include "undefdups.h"
#include <p4/clientapi.h>
#include <p4/i18napi.h>
#include <p4/spec.h>
#include <p4/debug.h>
#include "p4rubydebug.h"
#include "p4utils.h"
#include "p4specdata.h"

StrPtr *
SpecDataRuby::GetLine( SpecElem *sd, int x, const char **cmt )
{
	*cmt = 0;
	VALUE val;
	VALUE key;
	StrBuf t;

	key = P4Utils::ruby_string( sd->tag.Text(), sd->tag.Length() );
	val = rb_hash_aref( hash, key );
	if( val == Qnil ) return 0;

	if( !sd->IsList() )
	{
	    last = StringValuePtr( val );
	    return &last;
	}

	// It's a list, which means we should have an array value here
	
	if( !rb_obj_is_kind_of( val, rb_cArray ) )
	{
	    rb_warn( "%s should be an array element. Ignoring...", 
		    sd->tag.Text() );
	    return 0;
	}
	val = rb_ary_entry( val, x );
	if( val == Qnil ) return 0;

	last = StringValuePtr( val );
	return &last;
}

void	
SpecDataRuby::SetLine( SpecElem *sd, int x, const StrPtr *v, Error *e )
{
	VALUE 	key;
	VALUE	val;
	VALUE	ary;
	StrBuf	t;

	key = P4Utils::ruby_string( sd->tag.Text(), sd->tag.Length() );
	val = P4Utils::ruby_string( v->Text(), v->Length() );

	if( sd->IsList() )
	{
	    ary = rb_hash_aref( hash, key );
	    if( ary == Qnil )
	    {
		ary = rb_ary_new();
		rb_hash_aset( hash, key, ary );
	    }
	    rb_ary_store( ary, x, val );
	}
	else
	{
	    rb_hash_aset( hash, key, val );
	}
	return;
}
