/*******************************************************************************

Copyright (c) 2007-2011, Perforce Software, Inc.  All rights reserved.

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
 * Name         : specmgr.cc
 *
 * Description  : Ruby bindings for the Perforce API. Class for handling
 *                Perforce specs. This class provides other classes with
 *                generic support for parsing and formatting Perforce
 *                specs.
 *
 ******************************************************************************/
#include <ctype.h>
#include <ruby.h>
#include "p4utils.h"
#include "undefdups.h"
#include <p4/clientapi.h>
#include <p4/strops.h>
#include <p4/spec.h>
#include <p4/strtable.h>
#include "p4rubyconf.h"
#include "gc_hack.h"
#include "p4rubydebug.h"
#include "p4specdata.h"
#include "specmgr.h"

struct defaultspec {
    const char *type;
    const char *spec;
} speclist[] = {
    {
    "branch",
    "Branch;code:301;rq;ro;fmt:L;len:32;;"
    "Update;code:302;type:date;ro;fmt:L;len:20;;"
    "Access;code:303;type:date;ro;fmt:L;len:20;;"
    "Owner;code:304;fmt:R;len:32;;"
    "Description;code:306;type:text;len:128;;"
    "Options;code:309;type:line;len:32;val:"
    "unlocked/locked;;"
    "View;code:311;fmt:C;type:wlist;words:2;len:64;;"
    },
    {
    "change",
    "Change;code:201;rq;ro;fmt:L;seq:1;len:10;;"
    "Date;code:202;type:date;ro;fmt:R;seq:3;len:20;;"
    "Client;code:203;ro;fmt:L;seq:2;len:32;;"
    "User;code:204;ro;fmt:L;seq:4;len:32;;"
    "Status;code:205;ro;fmt:R;seq:5;len:10;;"
    "Type;code:211;seq:6;type:select;fmt:L;len:10;"
    "val:public/restricted;;"
    "ImportedBy;code:212;type:line;ro;fmt:L;len:32;;"
    "Identity;code:213;type:line;;"
    "Description;code:206;type:text;rq;seq:7;;"
    "JobStatus;code:207;fmt:I;type:select;seq:9;;"
    "Jobs;code:208;type:wlist;seq:8;len:32;;"
    "Stream;code:214;type:line;len:64;;"
    "Files;code:210;type:llist;len:64;;"
    },
    {
    "client",
    "Client;code:301;rq;ro;seq:1;len:32;;"
    "Update;code:302;type:date;ro;seq:2;fmt:L;len:20;;"
    "Access;code:303;type:date;ro;seq:4;fmt:L;len:20;;"
    "Owner;code:304;seq:3;fmt:R;len:32;;"
    "Host;code:305;seq:5;fmt:R;len:32;;"
    "Description;code:306;type:text;len:128;;"
    "Root;code:307;rq;type:line;len:64;;"
    "AltRoots;code:308;type:llist;len:64;;"
    "Options;code:309;type:line;len:64;val:"
    "noallwrite/allwrite,noclobber/clobber,nocompress/compress,"
    "unlocked/locked,nomodtime/modtime,normdir/rmdir,"
    "noaltsync/altsync;;"
    "SubmitOptions;code:313;type:select;fmt:L;len:25;val:"
    "submitunchanged/submitunchanged+reopen/revertunchanged/"
    "revertunchanged+reopen/leaveunchanged/leaveunchanged+reopen;;"
    "LineEnd;code:310;type:select;fmt:L;len:12;val:"
    "local/unix/mac/win/share;;"
    "Stream;code:314;type:line;len:64;;"
    "StreamAtChange;code:316;type:line;len:64;;"
    "ServerID;code:315;type:line;ro;len:64;;"
    "Type;code:318;type:select;len:10;val:"
    "writeable/readonly/graph/partitioned/partitioned-jnl;;"
    "Backup;code:319;type:select;len:10;val:enable/disable;;"
    "View;code:311;fmt:C;type:wlist;words:2;len:64;;"
    "ChangeView;code:317;type:llist;len:64;;"
    },
    {
    "depot",
    "Depot;code:251;rq;ro;len:32;;"
    "Owner;code:252;len:32;;"
    "Date;code:253;type:date;ro;len:20;;"
    "Description;code:254;type:text;len:128;;"
    "Type;code:255;rq;len:10;;"
    "Address;code:256;len:64;;"
    "Suffix;code:258;len:64;;"
    "StreamDepth;code:260;len:64;;"
    "Map;code:257;rq;len:64;;"
    "SpecMap;code:259;type:wlist;len:64;;"
    },
    {
    "group",
    "Group;code:401;rq;ro;len:32;;"
    "Description;code:NNN;type:text;fmt:L:len:128;;"
    "MaxResults;code:402;type:word;len:12;;"
    "MaxScanRows;code:403;type:word;len:12;;"
    "MaxLockTime;code:407;type:word;len:12;;"
    "MaxOpenFiles;code:413;type:word;len:12;;"
    "MaxMemory;code:NNN;type:word;len:12;;"
    "Timeout;code:406;type:word;len:12;;"
    "IdleTimeout;code:NNN;type:word;len:12;;"
    "PasswordTimeout;code:409;type:word;len:12;;"
    "LdapConfig;code:410;type:line;len:128;;"
    "LdapSearchQuery;code:411;type:line;len:128;;"
    "LdapUserAttribute;code:412;type:line;len:128;;"
    "LdapUserDNAttribute;code:414;type:line;len:128;;"
    "Subgroups;code:404;type:wlist;len:32;opt:default;;"
    "Owners;code:408;type:wlist;len:32;opt:default;;"
    "Users;code:405;type:wlist;len:32;opt:default;;"
    },
    {
    "hotfiles",
    "HotFiles;code:1051;fmt:C;type:wlist;words:1;maxwords:3;len:64;opt:default;z;;"
    },
    {
    "job",
    "Job;code:101;rq;len:32;;"
    "Status;code:102;type:select;rq;len:10;"
    "pre:open;val:open/suspended/closed;;"
    "User;code:103;rq;len:32;pre:$user;;"
    "Date;code:104;type:date;ro;len:20;pre:$now;;"
    "Description;code:105;type:text;rq;pre:$blank;;"
    },
    {
    "label",
    "Label;code:301;rq;ro;fmt:L;len:32;;"
    "Update;code:302;type:date;ro;fmt:L;len:20;;"
    "Access;code:303;type:date;ro;fmt:L;len:20;;"
    "Owner;code:304;fmt:R;len:32;;"
    "Description;code:306;type:text;len:128;;"
    "Options;code:309;type:line;len:64;val:"
    "unlocked/locked,noautoreload/autoreload;;"
    "Revision;code:312;type:word;words:1;len:64;;"
    "ServerID;code:315;type:line;ro;len:64;;"
    "View;code:311;fmt:C;type:wlist;len:64;;"
    },
    {
    "ldap",
    "Name;code:801;rq;len:32;;"
    "Host;code:802;rq;type:word;words:1;len:128;;"
    "Port;code:803;rq;type:word;words:1;len:5;;"
    "Encryption;code:804;rq;len:10;val:"
    "none/ssl/tls;;"
    "BindMethod;code:805;rq;len:10;val:"
    "simple/search/sasl;;"
    "Options;code:816;type:line;len:64;val:"
    "nodowncase/downcase,nogetattrs/getattrs,"
    "norealminusername/realminusername;;"
    "SimplePattern;code:806;type:line;len:128;;"
    "SearchBaseDN;code:807;type:line;len:128;;"
    "SearchFilter;code:808;type:line;len:128;;"
    "SearchScope;code:809;len:10;val:"
    "baseonly/children/subtree;;"
    "SearchBindDN;code:810;type:line;len:128;;"
    "SearchPasswd;code:811;type:line;len:128;;"
    "SaslRealm;code:812;type:word;words:1;len:128;;"
    "GroupBaseDN;code:813;type:line;len:128;;"
    "GroupSearchFilter;code:814;type:line;len:128;;"
    "GroupSearchScope;code:815;len:10;val:"
    "baseonly/children/subtree;;"
    "AttributeUid;code:817;type:word;len:128;;"
    "AttributeName;code:818;type:line;len:128;;"
    "AttributeEmail;code:819;type:word;len:128;;"
    },
    {
    "license",
    "License;code:451;len:32;;"
    "License-Expires;code:452;len:10;;"
    "Support-Expires;code:453;len:10;;"
    "Customer;code:454;type:line;len:128;;"
    "Application;code:455;len:32;;"
    "IPaddress;code:456;len:24;;"
    "IPservice;code:461;type:wlist;len:24;;"
    "Platform;code:457;len:32;;"
    "Clients;code:458;len:8;;"
    "Users;code:459;len:8;;"
    "Files;code:460;len:8;;"
    "Repos;code:462;len:8;;"
    "ExtraCapabilities;code:463;type:llist;len:512;;"
    },
    {
    "protect",
    "SubPath;code:502;ro;len:64;;"
    "Update;code:503;type:date;ro;fmt:L;len:20;;"
    "Protections;code:501;fmt:C;type:wlist;words:5;opt:default;z;len:64;;"
    },
    {
    "remote",
    "RemoteID;code:851;rq;ro;fmt:L;len:32;;"
    "Address;code:852;rq;type:line;len:32;;"
    "Owner;code:853;fmt:R;len:32;;"
    "RemoteUser;code:861;fmt:R;len:32;;"
    "Options;code:854;type:line;len:32;val:"
    "unlocked/locked,nocompress/compress,copyrcs/nocopyrcs;;"
    "Update;code:855;type:date;ro;fmt:L;len:20;;"
    "Access;code:856;type:date;ro;fmt:L;len:20;;"
    "Description;code:857;type:text;len:128;;"
    "LastFetch;code:858;fmt:L;len:10;;"
    "LastPush;code:859;fmt:L;len:10;;"
    "DepotMap;code:860;type:wlist;words:2;len:64;;"
    "ArchiveLimits;code:862;type:wlist;words:2;len:64;;"
    },
    {
    "repo",
    "Repo;code:1001;rq;ro;fmt:L;len:128;;"
    "Owner;code:1002;fmt:R;len:32;;"
    "Created;code:1003;type:date;ro;fmt:L;len:20;;"
    "Pushed;code:1004;type:date;ro;fmt:R;len:20;;"
    "ForkedFrom;code:1005;ro;fmt:L;len:128;;"
    "Description;code:1006;type:text;len:128;;"
    "DefaultBranch;code:1007;fmt:L;len:32;;"
    "MirroredFrom;code:1008;fmt:R;len:32;;"
    "Options;code:1009;type:select;len:10;val:lfs/nolfs;;"
    "GconnMirrorServerId;code:1010;fmt:L;len:32;;"
    "GconnMirrorSecretToken;code:NNN;len:36;;"
    "GconnMirrorStatus;code:NNN;len:8;;"
    "GconnMirrorExcludedBranches;code:NNN;len:256;;"
    "GconnMirrorHideFetchUrl;code:NNN;len:5;;"
    },
    {
    "server",
    "ServerID;code:751;rq;ro;len:32;;"
    "Type;code:752;rq;len:32;;"
    "Name;code:753;type:line;len:32;;"
    "Address;code:754;type:line;len:32;;"
    "ExternalAddress;code:755;type:line;len:32;;"
    "Services;code:756;rq;len:128;;"
    "Options;code:764;type:line;len:32;val:"
    "nomandatory/mandatory;;"
    "ReplicatingFrom;code:765;type:line;len:32;;"
    "Description;code:757;type:text;len:128;;"
    "User;code:761;type:line;len:64;;"
    "AllowedAddresses;code:763;type:wlist;len:64;;"
    "UpdateCachedRepos;code:766;type:wlist;len:64;;"
    "ClientDataFilter;code:758;type:wlist;len:64;;"
    "RevisionDataFilter;code:759;type:wlist;len:64;;"
    "ArchiveDataFilter;code:760;type:wlist;len:64;;"
    "DistributedConfig;code:762;type:text;len:128;;"
    },
    {
    "spec",
    "Fields;code:351;type:wlist;words:5;rq;;"
    "Words;code:352;type:wlist;words:2;;"
    "Formats;code:353;type:wlist;words:3;;"
    "Values;code:354;type:wlist;words:2;;"
    "Presets;code:355;type:wlist;words:2;;"
    "Openable;code:362;type:wlist;words:2;;"
    "Maxwords;code:361;type:wlist;words:2;;"
    "Comments;code:356;type:text;;"
    },
    {
    "stream",
    "Stream;code:701;rq;ro;len:64;;"
    "Update;code:705;type:date;ro;fmt:L;len:20;;"
    "Access;code:706;type:date;ro;fmt:L;len:20;;"
    "Owner;code:704;len:32;open:isolate;;"
    "Name;code:703;rq;type:line;len:32;open:isolate;;"
    "Parent;code:702;rq;len:64;open:isolate;;"
    "Type;code:708;rq;type:select;len:32;open:isolate;"
    "val:mainline/virtual/development/release/task/sparsedev/sparserel;;"
    "Description;code:709;type:text;len:128;open:isolate;;"
    "Options;code:707;type:line;len:64;val:"
    "allsubmit/ownersubmit,unlocked/locked,"
    "toparent/notoparent,fromparent/nofromparent,"
    "mergedown/mergeany;open:isolate;;"
    "ParentView;code:NNN;rq;open:isolate;"
    "pre:inherit;val:noinherit/inherit;;"
    "Components;code:NNN;type:wlist;words:3;maxwords:4;len:64;open:propagate;fmt:C;;"
    "Paths;code:710;rq;type:wlist;words:2;maxwords:3;len:64;open:propagate;fmt:C;;"
    "Remapped;code:711;type:wlist;words:2;len:64;open:propagate;fmt:C;;"
    "Ignored;code:712;type:wlist;words:1;len:64;open:propagate;fmt:C;;"
    "View;code:713;type:wlist;words:2;len:64;;"
    "ChangeView;code:714;type:llist;ro;len:64;;"
    },
    {
    "triggers",
    "Triggers;code:551;type:wlist;words:4;len:64;opt:default;z;;"
    },
    {
    "typemap",
    "TypeMap;code:601;fmt:C;type:wlist;words:2;len:64;opt:default;z;;"
    },
    {
    "user",
    "User;code:651;rq;ro;seq:1;len:32;;"
    "Type;code:659;ro;fmt:R;len:10;;"
    "Email;code:652;fmt:R;rq;seq:3;len:32;;"
    "Update;code:653;fmt:L;type:date;ro;seq:2;len:20;;"
    "Access;code:654;fmt:L;type:date;ro;len:20;;"
    "FullName;code:655;fmt:R;type:line;rq;len:32;;"
    "JobView;code:656;type:line;len:64;;"
    "Password;code:657;len:32;;"
    "AuthMethod;code:662;fmt:L;len:10;val:"
    "perforce/perforce+2fa/ldap/ldap+2fa;;"
    "Reviews;code:658;type:wlist;len:64;;"
    },
    { 0, 0}
};

SpecMgr::SpecMgr()
{
    debug = 0;
    specs = 0;
    convertArray = 1;
    Reset();
}

SpecMgr::~SpecMgr()
{
    delete specs;
}

void
SpecMgr::AddSpecDef( const char *type, StrPtr &specDef )
{
    if( specs->GetVar( type ) )
        specs->RemoveVar( type );
    specs->SetVar( type, specDef );
}

void
SpecMgr::AddSpecDef( const char *type, const char *specDef )
{
    if( specs->GetVar( type ) )
        specs->RemoveVar( type );
    specs->SetVar( type, specDef );
}


void
SpecMgr::Reset()
{
    delete specs;
    specs = new StrBufDict;

    for( struct defaultspec *sp = &speclist[ 0 ]; sp->type; sp++ )
        AddSpecDef( sp->type, sp->spec );

}

int
SpecMgr::HaveSpecDef( const char *type )
{
    return specs->GetVar( type ) != 0;
}

//
// Convert a Perforce StrDict into a Ruby hash. Convert multi-level
// data (Files0, Files1 etc. ) into (nested) array members of the hash.
//

VALUE
SpecMgr::StrDictToHash( StrDict *dict, VALUE hash )
{
    StrRef      var, val;
    int         i;

    if( hash == Qnil )
        hash = rb_hash_new();

    for ( i = 0; dict->GetVar( i, var, val ); i++ )
    {
        if ( var == "specdef" || var == "func" || var == "specFormatted" )
            continue;

        InsertItem( hash, &var, &val );
    }
    return hash;
}

//
// Convert a Perforce StrDict into a P4::Spec object
//

VALUE
SpecMgr::StrDictToSpec( StrDict *dict, StrPtr *specDef )
{

    // This converts it to a string, and then to a hash, so we go from one
    // type of dictionary to another, via an intermediate form (a StrBuf).

    Error               e;
    SpecDataTable       dictData( dict );
#if P4APIVER_ID >= 513538
    Spec                s( specDef->Text(), "", &e );
#else
    Spec                s( specDef->Text(), "" );
#endif
    StrBuf              form;

    if( e.Test() ) return Qfalse;

    // Format the StrDict into a StrBuf object
    s.Format( &dictData, &form );

    // Now parse the StrBuf into a new P4::Spec object
    VALUE               spec = NewSpec( specDef );
    SpecDataRuby        hashData( spec );

    s.ParseNoValid( form.Text(), &hashData, &e );
    if( e.Test() ) return Qfalse;

    // Now see if there are any extraTag fields as we'll need to
    // add those fields into our output. Just iterate over them
    // extracting the fields and inserting them as we go.
    int i = 0;
    StrRef et("extraTag" );
    for( i = 0; ; i++ )
    {
        StrBuf tag;
        StrPtr *var;
        StrPtr *val;

        tag << et << i;
        if( !(var = dict->GetVar( tag ) ) )
            break;

        val = dict->GetVar( *var );
        if( !val ) continue;

        InsertItem( spec, var, val );
    }

    return spec;
}

VALUE
SpecMgr::StringToSpec( const char *type, const char *form, Error *e )
{

    StrPtr *            specDef = specs->GetVar( type );
    VALUE               hash = NewSpec( specDef );
    SpecDataRuby        specData( hash );
#if P4APIVER_ID >= 513538
    Spec                s( specDef->Text(), "", e );
#else
    Spec                s( specDef->Text(), "" );
#endif

    if( !e->Test() )
        s.ParseNoValid( form, &specData, e );

    if ( e->Test() )
        return Qfalse;

    return hash;
}


//
// Format routine. updates a StrBuf object with the form content.
// The StrBuf can then be converted to a Ruby string where required.
//
void
SpecMgr::SpecToString( const char *type, VALUE hash, StrBuf &b, Error *e )
{

    StrBuf      buf;
    StrPtr *    specDef = specs->GetVar( type );
    if ( !specDef )
    {
        e->Set( E_FAILED, "No specdef available. Cannot convert hash to a "
                          "Perforce form" );
        return;
    }

    SpecDataRuby        specData( hash );
#if P4APIVER_ID >= 513538
    Spec                s( specDef->Text(), "", e );
#else
    Spec                s( specDef->Text(), "" );
#endif

    if( e->Test() ) return;

    s.Format( &specData, &b );
}

//
// This method returns a hash describing the valid fields in the spec. To
// make it easy on our users, we map the lowercase name to the name defined
// in the spec. Thus, the users can always user lowercase, and if the field
// should be in mixed case, it will be. See P4::Spec::method_missing
//

VALUE
SpecMgr::SpecFields( const char *type )
{
    return SpecFields( specs->GetVar( type ) );
}

VALUE
SpecMgr::SpecFields( StrPtr *specDef )
{
    if( !specDef ) return Qnil;

    //
    // Here we abuse the fact that SpecElem::tag is public, even though it's
    // only supposed to be public to SpecData's subclasses. It's hard to
    // see that changing anytime soon, and it makes this so simple and
    // reliable. So...
    //
    VALUE       hash = rb_hash_new();
    Error       e;

#if P4APIVER_ID >= 513538
    Spec        s( specDef->Text(), "", &e );
#else
    Spec        s( specDef->Text(), "" );
#endif
    if( e.Test() ) return Qnil;

    for( int i = 0; i < s.Count(); i++ )
    {
        StrBuf          k;
        StrBuf          v;
        SpecElem *      se = s.Get( i );

        v = se->tag;
        k = v;
        StrOps::Lower( k );

        rb_hash_aset(hash,
                    P4Utils::ruby_string( k.Text(), k.Length() ),
                    P4Utils::ruby_string( v.Text(), v.Length() ) );
    }
    return hash;
}

//
// Split a key into its base name and its index. i.e. for a key "how1,0"
// the base name is "how" and they index is "1,0". We work backwards from
// the end of the key looking for the first char that is neither a
// digit, nor a comma.
//

void
SpecMgr::SplitKey( const StrPtr *key, StrBuf &base, StrBuf &index )
{
    int i = 0;

    base = *key;
    index = "";
    for ( i = key->Length(); i; i-- )
    {
        char    prev = (*key)[ i-1 ];
        if ( !isdigit( prev ) && prev != ',' )
        {
            base.Set( key->Text(), i );
            index.Set( key->Text() + i );
            break;
        }
    }
}

//
// Insert an element into the response structure. The element may need to
// be inserted into an array nested deeply within the enclosing hash.
//

void
SpecMgr::InsertItem( VALUE hash, const StrPtr *var, const StrPtr *val )
{
    VALUE       ary = 0;
    VALUE       tary = 0;
    VALUE       key;
    ID          idLength = rb_intern( "length" );
    StrBuf      base, index;
    StrRef      comma( "," );

    if (convertArray) {
        SplitKey( var, base, index );
    }

    // If there's no index, then we insert into the top level hash
    // but if the key is already defined then we need to rename the key. This
    // is probably one of those special keys like otherOpen which can be
    // both an array element and a scalar. The scalar comes last, so we
    // just rename it to "otherOpens" to avoid trashing the previous key
    // value
    if ( index == "" )
{
        ID idHasKey     = rb_intern( "has_key?");
        ID idPlus       = rb_intern( "+" );

        key =  P4Utils::ruby_string( var->Text() );
        if ( rb_funcall( hash, idHasKey, 1, key ) == Qtrue )
            key = rb_funcall( key, idPlus, 1,  P4Utils::ruby_string( "s" ) );

        if( P4RDB_DATA )
            fprintf( stderr, "... %s -> %s\n", StringValuePtr( key ), val->Text() );

        rb_hash_aset( hash, key,  P4Utils::ruby_string( val->Text() ) );
        return;
    }

    //
    // Get or create the parent array from the hash.
    //
    key =  P4Utils::ruby_string( base.Text() );
    ary = rb_hash_aref( hash, key );

    if ( Qnil == ary )
    {
        ary = rb_ary_new();
        rb_hash_aset( hash, key, ary );
    }
    else if( rb_obj_is_kind_of( ary, rb_cArray ) != Qtrue )
    {
        //
        // There's an index in our var name, but the name is already defined
        // and the value it contains is not an array. This means we've got a
        // name collision. This can happen in 'p4 diff2' for example, when
        // one file gets 'depotFile' and the other gets 'depotFile2'. In
        // these cases it makes sense to keep the structure flat so we
        // just use the raw variable name.
        //
        if( P4RDB_DATA )
            fprintf( stderr, "... %s -> %s\n", var->Text(), val->Text() );

        rb_hash_aset( hash,  P4Utils::ruby_string( var->Text() ) ,
                             P4Utils::ruby_string( val->Text() ) );
        return;
}

    // The index may be a simple digit, or it could be a comma separated
    // list of digits. For each "level" in the index, we need a containing
    // array.
    if( P4RDB_DATA )
        fprintf( stderr, "... %s -> [", base.Text() );

    for( const char *c = 0 ; ( c = index.Contains( comma ) ); )
    {
        StrBuf  level;
        level.Set( index.Text(), c - index.Text() );
        index.Set( c + 1 );

        // Found another level so we need to get/create a nested array
        // under the current entry. We use the level as an index so that
        // missing entries are left empty deliberately.

        tary = rb_ary_entry( ary, level.Atoi() );
        if ( ! RTEST( tary ) )
{
            tary = rb_ary_new();
            rb_ary_store( ary, level.Atoi(), tary );
        }
        if( P4RDB_DATA )
            fprintf( stderr, "%s][", level.Text() );
        ary = tary;
    }
    int pos = index.Atoi();

    if( P4RDB_DATA )
        fprintf( stderr, "%d] = %s\n", pos, val->Text() );

    rb_ary_store( ary, pos,  P4Utils::ruby_string( val->Text() )  );
}

//
// Create a new P4::Spec object and return it.
//

VALUE
SpecMgr::NewSpec( StrPtr *specDef )
{
    ID          idNew           = rb_intern( "new" );
    ID          idP4            = rb_intern( "P4" );
    ID          idP4Spec        = rb_intern( "Spec" );
    VALUE       cP4             = rb_const_get_at( rb_cObject, idP4 );
    VALUE       cP4Spec         = rb_const_get_at( cP4, idP4Spec );
    VALUE       fields          = SpecFields( specDef );

    return rb_funcall( cP4Spec, idNew, 1, fields );
}
