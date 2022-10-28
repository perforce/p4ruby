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
 * Name		: specmgr.h
 *
 * Author	: Tony Smith <tony@perforce.com> or <tony@smee.org>
 *
 * Description	: Ruby bindings for the Perforce API. Class for handling
 * 		  Perforce specs. This class provides other classes with
 * 		  generic support for parsing and formatting Perforce
 *		  specs.
 *
 ******************************************************************************/

class StrBufDict;
class SpecMgr 
{
    public:
		SpecMgr();
		~SpecMgr();
	void	SetDebug( int i )	{ debug = i; 	}
	void	SetArrayConversion( int a)	{ convertArray = a; }

	// Clear the spec cache and revert to internal defaults
	void	Reset();

	// Add a spec to the cache
	void	AddSpecDef( const char *type, StrPtr &specDef );
	void	AddSpecDef( const char *type, const char * specDef );

	// Check that a type of spec is known.
	int	HaveSpecDef( const char *type );

	//
	// Parse routine: converts strings into Ruby P4::Spec objects.
	//
	VALUE	StringToSpec( const char *type, const char *spec, Error *e );

	//
	// Format routine. updates a StrBuf object with the form; 
	// that can then be converted to a Ruby string where required. 
	//
	void	SpecToString(const char *type, VALUE hash, StrBuf &b, Error *e);

	//
	// Convert a Perforce StrDict into a Ruby hash. Used when we're 
	// parsing tagged output that is NOT a spec. e.g. output of
	// fstat etc.
	//
	VALUE	StrDictToHash( StrDict *dict, VALUE hash = Qnil );

	// 
	// Convert a Perforce StrDict into a P4::Spec object. This is for
	// 2005.2 and later servers where the forms are supplied pre-parsed
	// into a dictionary - we just need to convert them. The specDef
	// argument tells us what type of spec we're converting.
	//
	VALUE	StrDictToSpec( StrDict *dict, StrPtr *specDef );


	//
	// Return a list of the fields in a given type of spec. Return Qnil
	// if the spec type is not known.
	//
	VALUE	SpecFields( const char *type );

    private:

	void	SplitKey( const StrPtr *key, StrBuf &base, StrBuf &index );
	void	InsertItem( VALUE hash, const StrPtr *var, const StrPtr *val );
	VALUE	NewSpec( StrPtr *specDef );
	VALUE	SpecFields( StrPtr *specDef );

    private:
	int		debug;
	int convertArray;
	StrBufDict *	specs;
};

