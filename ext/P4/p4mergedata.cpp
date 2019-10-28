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
 * Name		: p4mergedata.cc
 *
 * Author	: Tony Smith <tony@perforce.com> or <tony@smee.org>
 *
 * Description	: Class for holding merge data
 *
 ******************************************************************************/
#include <ruby.h>
#include "undefdups.h"
#include <p4/clientapi.h>
#include <p4/i18napi.h>
#include <p4/strtable.h>
#include <p4/spec.h>
#include "p4result.h"
#include "p4rubydebug.h"
#include "clientuserruby.h"
#include "p4utils.h"
#include "p4mergedata.h"

static void mergedata_free(P4MergeData *md) {
	delete md;
}

static void mergedata_mark(P4MergeData *md) {
	md->GCMark();
}

P4MergeData::P4MergeData(ClientUser *ui, ClientMerge *m, StrPtr &hint,
		VALUE info) {
	this->debug = 0;
	this->actionmerger = 0;
	this->ui = ui;
	this->merger = m;
	this->hint = hint;
	this->info = info;

	// Extract (forcibly) the paths from the RPC buffer.
	StrPtr *t;
	if ((t = ui->varList->GetVar("baseName"))) base = t->Text();
	if ((t = ui->varList->GetVar("yourName"))) yours = t->Text();
	if ((t = ui->varList->GetVar("theirName"))) theirs = t->Text();

}

P4MergeData::P4MergeData(ClientUser *ui, ClientResolveA *m, StrPtr &hint,
		VALUE info) {
	this->debug = 0;
	this->merger = 0;
	this->ui = ui;
	this->hint = hint;
	this->actionmerger = m;
	this->info = info;

}

VALUE P4MergeData::GetYourName() {
	if (merger && yours.Length())
		return P4Utils::ruby_string(yours.Text());
	else
		return Qnil;
}

VALUE P4MergeData::GetTheirName() {
	if (merger && theirs.Length())
		return P4Utils::ruby_string(theirs.Text());
	else
		return Qnil;
}

VALUE P4MergeData::GetBaseName() {
	if (merger && base.Length())
		return P4Utils::ruby_string(base.Text());
	else
		return Qnil;
}

VALUE P4MergeData::GetYourPath() {
	if (merger && merger->GetYourFile())
		return P4Utils::ruby_string(merger->GetYourFile()->Name());
	else
		return Qnil;
}

VALUE P4MergeData::GetTheirPath() {
	if (merger && merger->GetTheirFile())
		return P4Utils::ruby_string(merger->GetTheirFile()->Name());
	else
		return Qnil;
}

VALUE P4MergeData::GetBasePath() {
	if (merger && merger->GetBaseFile())
		return P4Utils::ruby_string(merger->GetBaseFile()->Name());
	else
		return Qnil;
}

VALUE P4MergeData::GetResultPath() {
	if (merger && merger->GetResultFile())
		return P4Utils::ruby_string(merger->GetResultFile()->Name());
	else
		return Qnil;
}

VALUE P4MergeData::GetMergeHint() {
	if (hint.Length())
		return P4Utils::ruby_string(hint.Text());
	else
		return Qnil;
}

VALUE P4MergeData::RunMergeTool() {
	Error e;
	if (merger) {
		ui->Merge(merger->GetBaseFile(), merger->GetTheirFile(),
				merger->GetYourFile(), merger->GetResultFile(), &e);

		if (e.Test()) return Qfalse;
		return Qtrue;
	}
	return Qfalse;
}

VALUE P4MergeData::GetActionResolveStatus() {
	return actionmerger ? Qtrue : Qfalse;
}

VALUE P4MergeData::GetContentResolveStatus() {
	return merger ? Qtrue : Qfalse;
}

VALUE P4MergeData::GetMergeInfo() {
	return this->info;
}

VALUE P4MergeData::GetMergeAction() {
	//	If we don't have an actionMerger then return nil
	if (actionmerger) {
		StrBuf buf;
		actionmerger->GetMergeAction().Fmt(&buf, EF_PLAIN);
		if (buf.Length())
			return P4Utils::ruby_string(buf.Text());
		else
			return Qnil;
	}
	return Qnil;
}

VALUE P4MergeData::GetYoursAction() {
	if (actionmerger) {
		StrBuf buf;
		actionmerger->GetYoursAction().Fmt(&buf, EF_PLAIN);
		if (buf.Length())
			return P4Utils::ruby_string(buf.Text());
		else
			return Qnil;
	}
	return Qnil;
}

VALUE P4MergeData::GetTheirAction() {
	if (actionmerger) {
		StrBuf buf;
		actionmerger->GetTheirAction().Fmt(&buf, EF_PLAIN);
		if (buf.Length())
			return P4Utils::ruby_string(buf.Text());
		else
			return Qnil;
	}
	return Qnil;
}

VALUE P4MergeData::GetType() {
	if (actionmerger) {
		StrBuf buf;
		actionmerger->GetType().Fmt(&buf, EF_PLAIN);
		if (buf.Length())
			return P4Utils::ruby_string(buf.Text());
		else
			return Qnil;
	}
	return Qnil;
}

void P4MergeData::Invalidate() {
	actionmerger = NULL;
	merger = NULL;
}

VALUE P4MergeData::GetString() {
	StrBuf result;
	StrBuf buffer;

	if (actionmerger) {
		result << "P4MergeData - Action\n";
		actionmerger->GetMergeAction().Fmt(&buffer, EF_INDENT);
		result << "\tmergeAction: " << buffer << "\n";
		buffer.Clear();

		actionmerger->GetTheirAction().Fmt(&buffer, EF_INDENT);
		result << "\ttheirAction: " << buffer << "\n";
		buffer.Clear();

		actionmerger->GetYoursAction().Fmt(&buffer, EF_INDENT);
		result << "\tyoursAction: " << buffer << "\n";
		buffer.Clear();

		actionmerger->GetType().Fmt(&buffer, EF_INDENT);
		result << "\ttype: " << buffer << "\n";
		buffer.Clear();

		result << "\thint: " << hint << "\n";
		return P4Utils::ruby_string(result.Text());
	} else {
		result << "P4MergeData - Content\n";
		if (yours.Length()) result << "yourName: " << yours << "\n";
		if (theirs.Length()) result << "thierName: " << theirs << "\n";
		if (base.Length()) result << "baseName: " << base << "\n";

		if ( merger && merger->GetYourFile())
			result << "\tyourFile: " << merger->GetYourFile()->Name() << "\n";
		if ( merger && merger->GetTheirFile())
			result << "\ttheirFile: " << merger->GetTheirFile()->Name() << "\n";
		if ( merger && merger->GetBaseFile())
			result << "\tbaseFile: " << merger->GetBaseFile()->Name() << "\n";

		return P4Utils::ruby_string(result.Text());
	}
	return Qnil;
}

VALUE P4MergeData::Wrap(VALUE pClass) {
	VALUE md;
	VALUE argv[1];

	md = Data_Wrap_Struct(pClass, mergedata_mark, mergedata_free, this);
	rb_obj_call_init(md, 0, argv);
	return md;
}

void P4MergeData::GCMark() {
	// We don't hold Ruby objects
}

