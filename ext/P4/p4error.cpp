/*******************************************************************************

Copyright (c) 2010, Perforce Software, Inc.  All rights reserved.

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
 * Name		: p4error.cc
 *
 * Author	: Tony Smith <tony@perforce.com> or <tony@smee.org>
 *
 * Description	: Class for bridging Perforce's Error class to Ruby
 *
 ******************************************************************************/
#include <ruby.h>
#include "undefdups.h"
#include <p4/clientapi.h>
#include "p4rubydebug.h"
#include "p4utils.h"
#include "p4error.h"


static void error_free( P4Error *e )
{
    delete e;
}

static void error_mark( P4Error *e )
{
    e->GCMark();
}


P4Error::P4Error( const Error &other )
{
    this->debug = 0;

    error = other;
}

VALUE
P4Error::GetId()
{
    ErrorId *id = error.GetId( 0 );
    if( !id )
	return INT2NUM( 0 );
    return INT2NUM( id->UniqueCode() );
}

VALUE
P4Error::GetGeneric()
{
    return INT2NUM( error.GetGeneric() );
}

VALUE
P4Error::GetSeverity()
{
    return INT2NUM( error.GetSeverity() );
}

VALUE
P4Error::GetText()
{
    StrBuf t;
    error.Fmt( t, EF_PLAIN );
    return P4Utils::ruby_string( t.Text(), t.Length() );
}

VALUE
P4Error::GetDict()
{
    VALUE dictHash = rb_hash_new();
    StrDict* pDict = error.GetDict();
    StrRef key, val;
    // suppress -Wpointer-arith
    for (int i=0;pDict->GetVar(i,key,val) != 0;i++) {
      rb_hash_aset( dictHash,
        P4Utils::ruby_string(key.Text(), key.Length()),
        P4Utils::ruby_string(val.Text(), val.Length()));
    }
    return dictHash;
}

VALUE
P4Error::Inspect()
{
    StrBuf a;
    StrBuf b;

    error.Fmt( a, EF_PLAIN );
    b << "[";
    b << "Gen:" << error.GetGeneric();
    b << "/Sev:" << error.GetSeverity();
    b << "]: ";
    b << a;
    return P4Utils::ruby_string( b.Text(), b.Length() );
}

VALUE
P4Error::Wrap( VALUE pClass )
{
    VALUE e;
    VALUE argv[ 1 ];

    e = Data_Wrap_Struct( pClass, error_mark, error_free, this );
    rb_obj_call_init( e, 0, argv );
    return e;
}

void
P4Error::GCMark()
{
    // We don't hold Ruby objects
}

