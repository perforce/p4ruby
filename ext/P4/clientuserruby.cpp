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
 * Name		: clientuserruby.cc
 *
 * Author	: Tony Smith <tony@perforce.com> or <tony@smee.org>
 *
 * Description	: Ruby bindings for the Perforce API. User interface class
 * 		  for getting Perforce results into Ruby.
 *
 ******************************************************************************/
#include <ctype.h>
#include <ruby.h>
#include "undefdups.h"
#include <p4/clientapi.h>
#include <p4/strtable.h>
#include <p4/clientprog.h>
#include <p4/spec.h>
#include <p4/diff.h>
#include "p4rubyconf.h"
#include "gc_hack.h"
#include "p4result.h"
#include "p4rubydebug.h"
#include "p4mergedata.h"
#include "p4error.h"
#include "clientuserruby.h"
#include "clientprogressruby.h"
#include "specmgr.h"
#include "p4utils.h"

extern VALUE cP4;	// Base P4 class
extern VALUE eP4;	// Exception class
extern VALUE cP4Msg; // Message class

static const int REPORT = 0;
static const int HANDLED = 1;
static const int CANCEL = 2;

/*******************************************************************************
 * ClientUserRuby - the user interface part. Gets responses from the Perforce
 * server, and converts the data to Ruby form for returning to the caller.
 ******************************************************************************/

class SSOShim : public ClientSSO {
public:
	SSOShim(ClientUserRuby *ui) : ui(ui) {}
	virtual ClientSSOStatus	Authorize( StrDict &vars,
				           int maxLength,
				           StrBuf &result )
	{
		return ui->Authorize(vars, maxLength, result);
	}
private:
	ClientUserRuby *ui;
} ;

ClientUserRuby::ClientUserRuby(SpecMgr *s) {
	specMgr = s;
	debug = 0;
	apiLevel = atoi(P4Tag::l_client);
	input = Qnil;
	mergeData = Qnil;
	mergeResult = Qnil;
	handler = Qnil;
	progress = Qnil;
	rubyExcept = 0;
	alive = 1;
	track = false;
    	SetSSOHandler( new SSOShim( this ) );

	ssoEnabled = 0;
	ssoResultSet = 0;
	ssoResult = Qnil;
	ssoHandler = Qnil;

	ID idP4 = rb_intern("P4");
	ID idP4OH = rb_intern("OutputHandler");
	ID idP4Progress = rb_intern("Progress");
	ID idP4SSO = rb_intern("SSOHandler");

	VALUE cP4 = rb_const_get_at(rb_cObject, idP4);
	cOutputHandler = rb_const_get_at(cP4, idP4OH);
	cProgress = rb_const_get_at(cP4, idP4Progress );
	cSSOHandler = rb_const_get_at(cP4, idP4SSO);
}

void ClientUserRuby::Reset() {
	results.Reset();
	rubyExcept = 0;
	// Leave input alone.

	alive = 1;
}

void ClientUserRuby::SetApiLevel(int l) {
	apiLevel = l;
	results.SetApiLevel(l);
}

void ClientUserRuby::Finished() {
	// Reset input coz we should be done with it now. Keeping hold of
	// it just prevents GC from sweeping it if possible
	if (P4RDB_CALLS && input != Qnil)
		fprintf(stderr, "[P4] Cleaning up saved input\n");

	input = Qnil;
}

void ClientUserRuby::RaiseRubyException() {
	if (!rubyExcept) return;
	rb_jump_tag(rubyExcept);
}

/*
 * Handling of output
 */

// returns true if output should be reported
// false if the output is handled and should be ignored
static VALUE CallMethod(VALUE data) {
	VALUE *args = reinterpret_cast<VALUE *>(data);
	return rb_funcall(args[0], (ID) args[1], 1, args[2]);
}

bool ClientUserRuby::CallOutputMethod(const char * method, VALUE data) {
	int answer = REPORT;
	int excepted = 0;

	if (P4RDB_COMMANDS) fprintf(stderr, "[P4] CallOutputMethod\n");

	// some wild hacks to satisfy the rb_protect method

	VALUE args[3] = { handler, (VALUE) rb_intern(method), data };

	VALUE result = rb_protect(CallMethod, (VALUE) args, &excepted);

	if (excepted) { // exception thrown
		alive = 0;
	} else {
		int a = NUM2INT(result);

		if (P4RDB_COMMANDS)
			fprintf(stderr, "[P4] CallOutputMethod returned %d\n", a);

		if (a & CANCEL) {
			if (P4RDB_COMMANDS)
				fprintf(stderr, "[P4] CallOutputMethod cancelled\n");
			alive = 0;
		}
		answer = a & HANDLED;
	}

	return (answer == 0);
}

void ClientUserRuby::ProcessOutput(const char * method, VALUE data) {
	if (this->handler != Qnil) {
		if (CallOutputMethod(method, data)) results.AddOutput(data);
	} else
		results.AddOutput(data);
}

void ClientUserRuby::ProcessMessage(Error * e) {
	if (this->handler != Qnil) {
		int s = e->GetSeverity();

		if (s == E_EMPTY || s == E_INFO) {
			// info messages should be send to outputInfo
			// not outputError, or untagged output looks
			// very strange indeed

			StrBuf m;
			e->Fmt(&m, EF_PLAIN);
			VALUE s = P4Utils::ruby_string(m.Text());

			if (CallOutputMethod("outputInfo", s)) results.AddOutput(s);
		} else {
			P4Error *pe = new P4Error(*e);
			VALUE ve = pe->Wrap(cP4Msg);

			if (CallOutputMethod("outputMessage", ve)) results.AddMessage(e);
		}
	} else
		results.AddMessage(e);
}

/*
 * Very little should use this. Most output arrives via
 * Message() these days, but -Ztrack output, and a few older
 * anachronisms might take this route.
 */
void ClientUserRuby::OutputText(const char *data, int length) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] OutputText()\n");
	if (P4RDB_DATA) fprintf(stderr, "... [%d]%*s\n", length, length, data);
	if (track && length > 4 && data[0] == '-' && data[1] == '-'
			&& data[2] == '-' && data[3] == ' ') {
		int p = 4;
		for (int i = 4; i < length; ++i) {
			if (data[i] == '\n') {
				if (i > p) {
					results.AddTrack(P4Utils::ruby_string(data + p, i - p));
					p = i + 5;
				} else {
					// this was not track data after all,
					// try to rollback the damage done
					ProcessOutput("outputText",
							P4Utils::ruby_string(data, length));
					results.DeleteTrack();
					return;
				}
			}
		}
	} else
		ProcessOutput("outputText", P4Utils::ruby_string(data, length));
}

void ClientUserRuby::Message(Error *e) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] Message()\n");

	if (P4RDB_DATA) {
		StrBuf t;
		e->Fmt(t, EF_PLAIN);
		fprintf(stderr, "... [%s] %s\n", e->FmtSeverity(), t.Text());
	}

	ProcessMessage(e);
}

void ClientUserRuby::HandleError(Error *e) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] Message()\n");

	if (P4RDB_DATA) {
		StrBuf t;
		e->Fmt(t, EF_PLAIN);
		fprintf(stderr, "... [%s] %s\n", e->FmtSeverity(), t.Text());
	}

	ProcessMessage(e);
}

void ClientUserRuby::OutputBinary(const char *data, int length) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] OutputBinary()\n");
	if (P4RDB_DATA) {
		for (int l = 0; l < length; l++) {
			if (l % 16 == 0) fprintf(stderr, "%s... ", l ? "\n" : "");
			fprintf(stderr, "%#hhx ", data[l]);
		}
	}

	//
	// Binary is just stored in a string. Since the char * version of
	// P4Result::AddOutput() assumes it can strlen() to find the length,
	// we'll make the String object here.
	//
	ProcessOutput("outputBinary", P4Utils::ruby_string(data, length));
}

void ClientUserRuby::OutputStat(StrDict *values) {
	StrPtr * spec = values->GetVar("specdef");
	StrPtr * data = values->GetVar("data");
	StrPtr * sf = values->GetVar("specFormatted");
	StrDict * dict = values;
	SpecDataTable specData;
	Error e;

	//
	// Determine whether or not the data we've got contains a spec in one form
	// or another. 2000.1 -> 2005.1 servers supplied the form in a data variable
	// and we use the spec variable to parse the form. 2005.2 and later servers
	// supply the spec ready-parsed but set the 'specFormatted' variable to tell
	// the client what's going on. Either way, we need the specdef variable set
	// to enable spec parsing.
	//
	int isspec = spec && (sf || data);

	//
	// Save the spec definition for later
	//
	if (spec) specMgr->AddSpecDef(cmd.Text(), spec->Text());

	//
	// Parse any form supplied in the 'data' variable and convert it into a
	// dictionary.
	//
	if (spec && data) {
		// 2000.1 -> 2005.1 server's handle tagged form output by supplying the form
		// as text in the 'data' variable. We need to convert it to a dictionary
		// using the supplied spec.
		if (P4RDB_CALLS) fprintf(stderr, "[P4] OutputStat() - parsing form\n");

		// Parse the form. Use the ParseNoValid() interface to prevent
		// errors caused by the use of invalid defaults for select items in
		// jobspecs.

#if P4APIVER_ID >= 513538
		Spec s(spec->Text(), "", &e);
#else
		Spec s( spec->Text(), "" );
#endif
		if (!e.Test()) s.ParseNoValid(data->Text(), &specData, &e);
		if (e.Test()) {
			HandleError(&e);
			return;
		}
		dict = specData.Dict();
	}

	//
	// If what we've got is a parsed form, then we'll convert it to a P4::Spec
	// object. Otherwise it's a plain hash.
	//
	if (isspec) {
		if (P4RDB_CALLS)
			fprintf(stderr,
					"[P4] OutputStat() - Converting to P4::Spec object\n");
		ProcessOutput("outputStat", specMgr->StrDictToSpec(dict, spec));
	} else {
		if (P4RDB_CALLS)
			fprintf(stderr, "[P4] OutputStat() - Converting to hash\n");
		ProcessOutput("outputStat", specMgr->StrDictToHash(dict));
	}
}

/*
 * Diff support for Ruby API. Since the Diff class only writes its output
 * to files, we run the requested diff putting the output into a temporary
 * file. Then we read the file in and add its contents line by line to the 
 * results.
 */

void ClientUserRuby::Diff(FileSys *f1, FileSys *f2, int doPage, char *diffFlags,
		Error *e) {

	if (P4RDB_CALLS) fprintf(stderr, "[P4] Diff() - comparing files\n");

	//
	// Duck binary files. Much the same as ClientUser::Diff, we just
	// put the output into Ruby space rather than stdout.
	//
	if (!f1->IsTextual() || !f2->IsTextual()) {
		if (f1->Compare(f2, e))
			results.AddOutput(P4Utils::ruby_string("(... files differ ...)"));
		return;
	}

	// Time to diff the two text files. Need to ensure that the
	// files are in binary mode, so we have to create new FileSys
	// objects to do this.

	FileSys *f1_bin = FileSys::Create(FST_BINARY);
	FileSys *f2_bin = FileSys::Create(FST_BINARY);
	FileSys *t = FileSys::CreateGlobalTemp(f1->GetType());

	f1_bin->Set(f1->Name());
	f2_bin->Set(f2->Name());

	{
		//
		// In its own block to make sure that the diff object is deleted
		// before we delete the FileSys objects.
		//
#ifndef OS_NEXT
		::
#endif
		Diff d;

		d.SetInput(f1_bin, f2_bin, diffFlags, e);
		if (!e->Test()) d.SetOutput(t->Name(), e);
		if (!e->Test()) d.DiffWithFlags(diffFlags);
		d.CloseOutput(e);

		// OK, now we have the diff output, read it in and add it to
		// the output.
		if (!e->Test()) t->Open(FOM_READ, e);
		if (!e->Test()) {
			StrBuf b;
			while (t->ReadLine(&b, e))
				results.AddOutput(P4Utils::ruby_string(b.Text(), b.Length()));
		}
	}

	delete t;
	delete f1_bin;
	delete f2_bin;

	if (e->Test()) HandleError(e);
}

/*
 * convert input from the user into a form digestible to Perforce. This
 * involves either (a) converting any supplied hash to a Perforce form, or
 * (b) running to_s on whatever we were given. 
 */

void ClientUserRuby::InputData(StrBuf *strbuf, Error *e) {
	if (P4RDB_CALLS)
		fprintf(stderr, "[P4] InputData(). Using supplied input\n");

	VALUE inval = input;

	if (Qtrue == rb_obj_is_kind_of(input, rb_cArray)) {
		inval = rb_ary_shift(input);
	}

	if (Qnil == inval) {
		e->Set(E_FAILED, "No user-input supplied.");
		return;
	}

	if (Qtrue == rb_obj_is_kind_of(inval, rb_cHash)) {
		StrPtr * specDef = varList->GetVar("specdef");

		specMgr->AddSpecDef(cmd.Text(), specDef->Text());
		specMgr->SpecToString(cmd.Text(), inval, *strbuf, e);
		return;
	}

	// Convert whatever's left into a string
	ID to_s = rb_intern("to_s");
	VALUE str = rb_funcall(inval, to_s, 0);
	strbuf->Set(StringValuePtr(str));
}

/*
 * In a script we don't really want the user to see a prompt, so we
 * (ab)use the SetInput() function to allow the caller to supply the
 * answer before the question is asked.
 */
void ClientUserRuby::Prompt(const StrPtr &msg, StrBuf &rsp, int noEcho,
		Error *e) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] Prompt(): %s\n", msg.Text());

	InputData(&rsp, e);
}

/*
 * Do a resolve. We implement a resolve by calling a block.
 */
int ClientUserRuby::Resolve(ClientMerge *m, Error *e) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] Resolve()\n");
	//
	// If rubyExcept is non-zero, we should skip any further
	//	resolves
	//
	if (rubyExcept) return CMS_QUIT;
	//
	// If no block has been passed, default to using the merger's resolve
	//
	if (!rb_block_given_p()) return m->Resolve(e);

	//
	// First detect what the merger thinks the result ought to be
	//
	StrBuf t;
	MergeStatus autoMerge = m->AutoResolve(CMF_FORCE);

	// Now convert that to a string;
	switch (autoMerge) {
	case CMS_QUIT:
		t = "q";
		break;
	case CMS_SKIP:
		t = "s";
		break;
	case CMS_MERGED:
		t = "am";
		break;
	case CMS_EDIT:
		t = "e";
		break;
	case CMS_YOURS:
		t = "ay";
		break;
	case CMS_THEIRS:
		t = "at";
		break;
	}

	mergeData = MkMergeInfo(m, t);

	VALUE r;
	StrBuf reply;

	//
	// Call the block using rb_protect to make sure that if the
	// block raises any exceptions we trap them here. We don't want
	// some random longjmp() trashing the Perforce connection. If an
	// exception is raised, we'll abort the merge.
	//
	r = rb_protect(rb_yield, mergeData, &rubyExcept);
	
	// Make sure the pointers held by the mergeData object are 
	// invalidated. This makes sure we can't dereference a pointer to
	// something that no longer exists if the mergeData object lives
	// longer than this resolve (e.g. exception in block) and its to_s
	// method gets called
	ID invalidate = rb_intern( "invalidate" );
	rb_funcall(mergeData, invalidate, 0);

	if (rubyExcept) return CMS_QUIT;
	reply = StringValuePtr(r);

	if (reply == "ay")
		return CMS_YOURS;
	else if (reply == "at")
		return CMS_THEIRS;
	else if (reply == "am")
		return CMS_MERGED;
	else if (reply == "ae")
		return CMS_EDIT;
	else if (reply == "s")
		return CMS_SKIP;
	else if (reply == "q")
		return CMS_QUIT;
	else {
		StrBuf msg = "[P4] Invalid 'p4 resolve' response: ";
		msg << reply;
		rb_warn( "%s", msg.Text());
	}

	return CMS_QUIT;
}

int ClientUserRuby::Resolve(ClientResolveA *m, int preview, Error *e) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] Resolve(Action)\n");

	//
	// If rubyExcept is non-zero, we should skip any further
	//	resolves
	//
	if (rubyExcept) return CMS_QUIT;

	//
	// If no block has been passed, default to using the merger's resolve
	//
	if (!rb_block_given_p()) return m->Resolve(0, e);

	StrBuf t;
	MergeStatus autoMerge = m->AutoResolve(CMF_FORCE);

	// Now convert that to a string;
	switch (autoMerge) {
	case CMS_QUIT:
		t = "q";
		break;
	case CMS_SKIP:
		t = "s";
		break;
	case CMS_MERGED:
		t = "am";
		break;
	case CMS_EDIT:
		t = "e";
		break;
	case CMS_YOURS:
		t = "ay";
		break;
	case CMS_THEIRS:
		t = "at";
		break;
	default:
		StrBuf msg = "[P4] Unknown automerge result encountered: ";
		msg << autoMerge;
		t = "q";
		break;
	}

	mergeData = MkActionMergeInfo(m, t);

	VALUE r;
	StrBuf reply;

	//
	// Call the block using rb_protect to make sure that if the
	// block raises any exceptions we trap them here. We don't want
	// some random longjmp() trashing the Perforce connection. If an
	// exception is raised, we'll abort the merge.
	//
	r = rb_protect(rb_yield, mergeData, &rubyExcept);
	if (rubyExcept) return CMS_QUIT;

	reply = StringValuePtr(r);

	if (reply == "ay")
		return CMS_YOURS;
	else if (reply == "at")
		return CMS_THEIRS;
	else if (reply == "am")
		return CMS_MERGED;
	else if (reply == "ae")
		return CMS_EDIT;
	else if (reply == "s")
		return CMS_SKIP;
	else if (reply == "q")
		return CMS_QUIT;
	else {
		StrBuf msg = "[P4] Invalid 'p4 resolve' response: ";
		msg << reply;
		rb_warn("%s", msg.Text());
	}
	return CMS_QUIT;
}

/*
 * Return the ClientProgress.
 */
ClientProgress* ClientUserRuby::CreateProgress(int type) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] CreateProgress()\n");

	if( progress == Qnil ) {
		return NULL;
	} else {
		return new ClientProgressRuby( progress, type );
	}
}

/*
 * Simple method to check if a progress indicator has been
 * registered to this ClientUser.
 */
int ClientUserRuby::ProgressIndicator() {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] ProgressIndicator()\n");
	int result = ( progress != Qnil );
	return result;
}
/*
 * Accept input from Ruby and convert to a StrBuf for Perforce
 * purposes.  We just save what we're given here because we may not 
 * have the specdef available to parse it with at this time.
 */

VALUE ClientUserRuby::SetInput(VALUE i) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] SetInput()\n");

	input = i;
	return Qtrue;
}

/*
 * Set the Handler object. Double-check that it is either nil or
 * an instance of OutputHandler to avoid future problems
 */

VALUE ClientUserRuby::SetHandler(VALUE h) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] SetHandler()\n");

	if (Qnil != h && Qfalse == rb_obj_is_kind_of(h, cOutputHandler)) {
		rb_raise(eP4, "Handler needs to be an instance of P4::OutputHandler");
		return Qfalse;
	}

	handler = h;
	alive = 1; // ensure that we don't drop out after the next call

	return Qtrue;
}

/*
 * Set a ClientProgress for the current ClientUser.
 */
VALUE ClientUserRuby::SetProgress(VALUE p) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] SetProgress()\n");
	
	//	Check that p is a kind_of P4::Progress and if it isn't
	//	raise an error.
	VALUE res = rb_obj_is_kind_of( p, cProgress );
	if( ( p != Qnil ) && ( res == Qfalse ) ) {
		rb_raise( eP4, "Progress must be of type P4::Progress" );
		return Qfalse;
	}

	progress = p;
	alive = 1;
	return Qtrue;
}

VALUE ClientUserRuby::MkMergeInfo(ClientMerge *m, StrPtr &hint) {
	ID idP4 = rb_intern("P4");
	ID idP4M = rb_intern("MergeData");

	//
	//	Get the last entry from the results array
	//
	VALUE info = rb_ary_new();
	VALUE output = results.GetOutput();
	int len = RARRAY_LEN(output);
	if( len > 1 ) {
		rb_ary_push( info, rb_ary_entry(output, len - 2) );
		rb_ary_push( info, rb_ary_entry(output, len - 1) );
	}

	VALUE cP4 = rb_const_get_at(rb_cObject, idP4);
	VALUE cP4M = rb_const_get_at(cP4, idP4M);

	P4MergeData *d = new P4MergeData(this, m, hint, info);
	return d->Wrap(cP4M);
}

VALUE ClientUserRuby::MkActionMergeInfo(ClientResolveA *m, StrPtr &hint) {
	ID idP4 = rb_intern("P4");
	ID idP4M = rb_intern("MergeData");

	//
	//	Get the last entry from the results array
	//
	VALUE info = rb_ary_new();
	VALUE output = results.GetOutput();
	int len = RARRAY_LEN(output);
	rb_ary_push( info, rb_ary_entry(output, len - 1) );

	VALUE cP4 = rb_const_get_at(rb_cObject, idP4);
	VALUE cP4M = rb_const_get_at(cP4, idP4M);

	P4MergeData *d = new P4MergeData(this, m, hint, info);
	return d->Wrap(cP4M);
}

//
// GC support
//
void ClientUserRuby::GCMark() {
	if (P4RDB_GC)
		fprintf(stderr,
				"[P4] Marking results and errors for garbage collection\n");

	if (input != Qnil) rb_gc_mark( input);
	if (mergeData != Qnil) rb_gc_mark( mergeData);
	if (mergeResult != Qnil) rb_gc_mark( mergeResult);
	if (handler != Qnil) rb_gc_mark( handler);
	if (progress != Qnil) rb_gc_mark( progress );
	if (ssoResult != Qnil) rb_gc_mark( ssoResult );
	if (ssoHandler != Qnil) rb_gc_mark( ssoHandler );
	rb_gc_mark( cOutputHandler );
	rb_gc_mark( cProgress );
	rb_gc_mark( cSSOHandler );

	results.GCMark();
}


/*
 * Set the Handler object. Double-check that it is either nil or
 * an instance of OutputHandler to avoid future problems
 */

VALUE
ClientUserRuby::SetRubySSOHandler(VALUE h) {
	if (P4RDB_CALLS) fprintf(stderr, "[P4] SetSSOHandler()\n");

	if (Qnil != h && Qfalse == rb_obj_is_kind_of(h, cSSOHandler)) {
		rb_raise(eP4, "Handler needs to be an instance of P4::SSOHandler");
		return Qfalse;
	}

	ssoHandler = h;
	alive = 1; // ensure that we don't drop out after the next call

	return Qtrue;
}


// returns true if output should be reported
// false if the output is handled and should be ignored
static VALUE CallMethodSSO(VALUE data) {
	VALUE *args = reinterpret_cast<VALUE *>(data);
	return rb_funcall(args[0], (ID) rb_intern("authorize"), 2, args[1], args[2]);
}

ClientSSOStatus
ClientUserRuby::CallSSOMethod(VALUE vars, int maxLength, StrBuf &result) {
	ClientSSOStatus answer = CSS_SKIP;
	int excepted = 0;
	result.Clear();

	if (P4RDB_COMMANDS) fprintf(stderr, "[P4] CallSSOMethod\n");

	// some wild hacks to satisfy the rb_protect method

	VALUE args[3] = { ssoHandler, vars, INT2NUM( maxLength ) };

	VALUE res = rb_protect(CallMethodSSO, (VALUE) args, &excepted);

	if (excepted) { // exception thrown
		alive = 0;
		rb_jump_tag(excepted);
	} else if( FIXNUM_P( res ) ) {
		int a = NUM2INT(res);
		if (P4RDB_COMMANDS)
			fprintf(stderr, "[P4] CallSSOMethod returned %d\n", a);

		if( a < CSS_PASS || a > CSS_SKIP )
			rb_raise(eP4, "P4::SSOHandler::authorize returned out of range response");
		answer = (ClientSSOStatus) a;
	} else if( Qtrue == rb_obj_is_kind_of(res, rb_cArray) ) {
		VALUE resval1 = rb_ary_shift(res);
		Check_Type( resval1, T_FIXNUM );
		int a = NUM2INT(resval1);
		if( a < CSS_PASS || a > CSS_SKIP )
			rb_raise(eP4, "P4::SSOHandler::authorize returned out of range response");
		answer = (ClientSSOStatus) a;

		VALUE resval2 = rb_ary_shift(res);
		if( resval2 != Qnil )
		{
			Check_Type( resval2, T_STRING );
			result.Set(StringValuePtr(resval2));
			if (P4RDB_COMMANDS)
				fprintf(stderr, "[P4] CallSSOMethod returned %d, %s\n", a, result.Text());
		}

	} else {
		Check_Type( res, T_STRING );
		answer = CSS_PASS;

		result.Set(StringValuePtr(res));
		if (P4RDB_COMMANDS)
			fprintf(stderr, "[P4] CallSSOMethod returned %s\n", result.Text());

	}

	return answer;
}

ClientSSOStatus
ClientUserRuby::Authorize( StrDict &vars, int maxLength, StrBuf &strbuf )
{
	ssoVars.Clear();

	if( ssoHandler != Qnil )
	{
		ClientSSOStatus res = CallSSOMethod( specMgr->StrDictToHash(&vars), maxLength, strbuf );
		if( res != CSS_SKIP )
			return res;
		if (P4RDB_COMMANDS)
			fprintf(stderr, "[P4] Authorize skipped result from SSO Handler\n" );
	}

	if( !ssoEnabled )
		return CSS_SKIP;

	if( ssoEnabled < 0 )
		return CSS_UNSET;

	if( ssoResultSet )
	{
		strbuf.Clear();
			VALUE resval = ssoResult;

		//if( P4RDB_CALLS )
		//    std::cerr << "[P4] ClientSSO::Authorize(). Using supplied input"
		//              << std::endl;

		if (Qtrue == rb_obj_is_kind_of(ssoResult, rb_cArray)) {
			resval = rb_ary_shift(ssoResult);
		}
	
		if( resval != Qnil ) {
			// Convert whatever's left into a string
			ID to_s = rb_intern("to_s");
			VALUE str = rb_funcall(resval, to_s, 0);
			strbuf.Set(StringValuePtr(str));
		}

		return ssoResultSet == 2 ? CSS_FAIL
				: CSS_PASS;
	}

	ssoVars.CopyVars( vars );
	return CSS_EXIT;
}

VALUE
ClientUserRuby::EnableSSO( VALUE e )
{
	if( e == Qnil )
	{
		ssoEnabled = 0;
		return Qtrue;
	}

	if( e == Qtrue )
	{
		ssoEnabled = 1;
		return Qtrue;
	}

	if( e == Qfalse )
	{
		ssoEnabled = -1;
		return Qtrue;
	}

	return Qfalse;
}

VALUE
ClientUserRuby::SSOEnabled()
{
	if( ssoEnabled == 1 )
	{
		return Qtrue;
	}
	else if( ssoEnabled == -1 )
	{
		return Qfalse;
	}
	else
	{
		return Qnil;
	}
}

VALUE
ClientUserRuby::SetSSOPassResult( VALUE i )
{
	ssoResultSet = 1;
	return SetSSOResult( i );
}

VALUE
ClientUserRuby::GetSSOPassResult()
{
	if( ssoResultSet == 1 )
	{
		return ssoResult;
	}
	else
	{
		return Qnil;
	}
}

VALUE
ClientUserRuby::SetSSOFailResult( VALUE i )
{
	ssoResultSet = 2;
	return SetSSOResult( i );
}

VALUE
ClientUserRuby::GetSSOFailResult()
{
	if( ssoResultSet == 2 )
	{
		return ssoResult;
	}
	else
	{
		return Qnil;
	}
}

VALUE
ClientUserRuby::SetSSOResult( VALUE i )
{
	if (P4RDB_CALLS) fprintf(stderr, "[P4] P4ClientSSO::SetResult()\n");
 
	ssoResult = i;
	return Qtrue;
}

VALUE
ClientUserRuby::GetSSOVars()
{
    	return specMgr->StrDictToHash( &ssoVars );
}