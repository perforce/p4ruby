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
 * Name		: p4result.h
 *
 * Author	: Tony Smith <tony@perforce.com> or <tony@smee.org>
 *
 * Description	: C++ class for holding results of Perforce commands 
 *
 ******************************************************************************/

class P4Result
{
    public:

    P4Result();
    
    // Setting
    void	AddOutput( VALUE v );
    void	AddMessage( Error *e );
    void	AddTrack( const char *msg );
    void	AddTrack( VALUE t );
    void 	DeleteTrack();

    // Getting
    VALUE	GetOutput()	{ return output;	}
    VALUE	GetErrors()	{ return errors;	}
    VALUE	GetWarnings()	{ return warnings;	}
    VALUE	GetMessages()	{ return messages;	}
    VALUE 	GetTrack()	{ return track;         }

    // Get errors/warnings as a formatted string
    void	FmtErrors( StrBuf &buf );
    void	FmtWarnings( StrBuf &buf );

    // Set API level for backwards compatibility
    void	SetApiLevel( int l )	{ apiLevel = l; }
    // Testing
    int		ErrorCount();
    int		WarningCount();

    // Clear previous results
    void	Reset();

    // Ruby garbage collection
    void	GCMark();

    private:
    int		Length( VALUE ary );
    void	Fmt( const char *label, VALUE ary, StrBuf &buf );
    VALUE	FmtMessage( Error *e );
    VALUE	WrapMessage( Error *e );

    VALUE	cP4Msg;
    VALUE	output;
    VALUE	warnings;
    VALUE	errors;
    VALUE	messages;
    VALUE	track;
    int		apiLevel;
};
