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
 * Name		: clientuserruby.h
 *
 * Author	: Tony Smith <tony@perforce.com> or <tony@smee.org>
 *
 * Description	: Ruby bindings for the Perforce API. User interface class
 * 		  for getting Perforce results into Ruby.
 *
 ******************************************************************************/

/*******************************************************************************
 * ClientUserRuby - the user interface part. Gets responses from the Perforce
 * server, and converts the data to Ruby form for returning to the caller.
 ******************************************************************************/
class SpecMgr;
class ClientProgress;

class P4ClientSSO : public ClientSSO
{
    public:
        P4ClientSSO( SpecMgr *s );

    // Client SSO methods overridden here
    virtual ClientSSOStatus Authorize( StrDict &vars, int maxLength,
                                       StrBuf &result );

    // Local methods
    VALUE EnableSSO( VALUE e );
    VALUE SSOEnabled();
    VALUE SetPassResult( VALUE i );
    VALUE GetPassResult();
    VALUE SetFailResult( VALUE i );
    VALUE GetFailResult();
    VALUE GetSSOVars();

	void GCMark();

    private:

    VALUE SetResult( VALUE i );

    int         ssoEnabled;
    int         resultSet;

    StrBufDict  ssoVars;
    SpecMgr *   specMgr;

    VALUE       result;
};

class ClientUserRuby: public ClientUser, public KeepAlive {
public:
	ClientUserRuby(SpecMgr *s);

	// Client User methods overridden here
	void OutputText(const char *data, int length);
	void Message(Error *e);
	void OutputStat(StrDict *values);
	void OutputBinary(const char *data, int length);
	void InputData(StrBuf *strbuf, Error *e);
	void Diff(FileSys *f1, FileSys *f2, int doPage, char *diffFlags, Error *e);
	void Prompt(const StrPtr &msg, StrBuf &rsp, int noEcho, Error *e);

	int Resolve(ClientMerge *m, Error *e);
	int Resolve(ClientResolveA *m, int preview, Error *e);

	ClientProgress* CreateProgress( int type );
	int ProgressIndicator();

	void Finished();

	// Local methods
	VALUE SetInput(VALUE i);
	void SetCommand(const char *c) {
		cmd = c;
	}
	void SetApiLevel(int l);
	void SetTrack(bool t) {
		track = t;
	}

	P4Result& GetResults() {
		return results;
	}
	int ErrorCount();
	void Reset();

	void RaiseRubyException();

	// GC support
	void GCMark();

	// Debugging support
	void SetDebug(int d) {
		debug = d;
	}

	// Handler support
	VALUE SetHandler(VALUE handler);
	VALUE GetHandler() {
		return handler;
	}

	//	Progress API support
	VALUE SetProgress( VALUE p );
	VALUE GetProgress() {
		return progress;
	}

	// SSO handler support

    VALUE EnableSSO( VALUE e )        { return ssoHandler->EnableSSO( e ); }
    VALUE SSOEnabled()   { return ssoHandler->SSOEnabled(); }
    VALUE SetSSOPassResult( VALUE i ) { return ssoHandler->SetPassResult( i ); }
    VALUE GetSSOPassResult(){ return ssoHandler->GetPassResult();}
    VALUE SetSSOFailResult( VALUE i ) { return ssoHandler->SetFailResult( i ); }
    VALUE GetSSOFailResult(){ return ssoHandler->GetFailResult();}
    VALUE GetSSOVars()  { return ssoHandler->GetSSOVars(); }

	// override from KeepAlive
	virtual int IsAlive() {
		return alive;
	}

private:
	VALUE MkMergeInfo(ClientMerge *m, StrPtr &hint);
	VALUE MkActionMergeInfo(ClientResolveA *m, StrPtr &hint);
	void ProcessOutput(const char * method, VALUE data);
	void ProcessMessage(Error * e);
	bool CallOutputMethod(const char * method, VALUE data);

private:
	StrBuf cmd;
	SpecMgr * specMgr;
	P4Result results;
	VALUE input;
	VALUE mergeData;
	VALUE mergeResult;
	VALUE handler;
	VALUE cOutputHandler;
	VALUE progress;
	VALUE cProgress;
	int debug;
	int apiLevel;
	int alive;
	int rubyExcept;
	bool track;
    P4ClientSSO * ssoHandler;
};

