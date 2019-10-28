/*******************************************************************************

 Copyright (c) 2001-2012, Perforce Software, Inc.  All rights reserved.

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
 * Name		: ClientProgressRuby.h
 *
 * Author	: Jayesh Mistry <jmistry@perforce.com>
 *
 * Description	: C++ Subclass for ClientProgress used by P4Ruby
 * 					Allows Perforce API to indicate progress of calls to P4Ruby
 *
 ******************************************************************************/
#include "ruby.h"
#include "undefdups.h"
#include "gc_hack.h"
#include "extconf.h"
#include "p4utils.h"
#include "p4/clientapi.h"
#include "p4/clientprog.h"
#include "clientprogressruby.h"

extern VALUE eP4;

ClientProgressRuby::ClientProgressRuby(VALUE prog, int t) {
	progress = prog;
	ID method = rb_intern("init");
	VALUE type  = INT2NUM( t );
	if (rb_respond_to(progress, method))
		rb_funcall(progress, method, 1, type);
	else
		rb_raise(eP4, "P4::Progress#init not implemented");
}

ClientProgressRuby::~ClientProgressRuby() {
	rb_gc_mark( progress );
}

void ClientProgressRuby::Description(const StrPtr *d, int u) {
	ID method = rb_intern("description");
	VALUE desc = P4Utils::ruby_string(d->Text());
	VALUE units = INT2NUM( u );
	if (rb_respond_to(progress, method))
		rb_funcall(progress, method, 2, desc, units);
	else
		rb_raise(eP4, "P4::Progress#description not implemented");
}

void ClientProgressRuby::Total(long t) {
	VALUE total = LONG2NUM( t );
	ID method = rb_intern("total");
	if (rb_respond_to(progress, method))
		rb_funcall(progress, method, 1, total);
	else
		rb_raise(eP4, "P4::Progress#total not implemented");
}

int ClientProgressRuby::Update(long pos) {
	VALUE position = LONG2NUM( pos );
	ID method = rb_intern( "update" );
	if( rb_respond_to(progress, method))
		rb_funcall( progress, method, 1, position );
	else
		rb_raise(eP4, "P4::Progress#update not implemented");

	return 0;
}

void ClientProgressRuby::Done(int f) {
	VALUE fail = INT2NUM( f );
	ID method = rb_intern( "done" );
	if( rb_respond_to(progress, method))
		rb_funcall( progress, method, 1, fail );
	else
		rb_raise(eP4, "P4::Progress#done not implemented");
}
