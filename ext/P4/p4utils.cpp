// vim:ts=8:sw=4:
/*******************************************************************************

Copyright (c) 2011, Perforce Software, Inc.  All rights reserved.

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
#include <ruby.h>
#ifdef HAVE_RUBY_ENCODING_H  
#include <ruby/encoding.h>
#endif
#include "p4utils.h"

char *P4Utils::charset = 0;

VALUE P4Utils::ruby_string( const char *msg, long len )
{
    VALUE str;
    //	If a length has been passed then use it
    if( len )
    {
	str = rb_str_new( msg, len );
    }
    else
    {
	str = rb_str_new2( msg );
    }

    //	Now check if an encoding should be set for the string.
#ifdef HAVE_RUBY_ENCODING_H
    if( charset )
    {
	rb_enc_associate(str, rb_enc_find("UTF-8"));  
    }
    else 
    {
	rb_enc_associate(str, rb_locale_encoding());  
    }
#endif  

    return str;
}
