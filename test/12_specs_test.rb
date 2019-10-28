# vim:ts=2:sw=2:et:
#-------------------------------------------------------------------------------
# Copyright (c) 1997-2007, Perforce Software, Inc.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL PERFORCE
# SOFTWARE, INC. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.
#-------------------------------------------------------------------------------

class TC_Specs < Test::Unit::TestCase

  JOBSPEC = \
      "Fields:
        101 Job word 32 required
        102 Status select 10 required
        103 User word 32 required
        104 Date date 20 always
        105 Description text 0 required
        106 Field1 text 0 optional

Values:
        Status open/suspended/closed

Presets:
        Status open
        User $user
        Date $now
        Description $blank"

  BRANCHSPEC = \
      "Branch:	test_branch
Update:	2011/01/01 00:00:00
Access:	2012/01/01 00:00:00
Owner:	test
Description: Created by test.
Options:	unlocked
View:
        //depot/main/... //depot/rel/..."

  CHANGESPEC = \
      "Change:	new
Client:	jmistry_mac_p4ruby
Date:	2012/01/01 00:00:00
User:	jmistry
Status:	new
Type:	restricted
JobStatus:	open
Jobs:	job000001
        job000002
Description:
        <enter description here>

Files:
        //depot/filea
        //depot/fileb"

  CLIENTSPEC = \
"Client:	test_client
Update:	2011/01/01 00:00:00
Access:	2012/01/01 00:00:00
Owner:	test
Host:	test_host
Description:
        Created by test.
Root:	/test/path
AltRoots:
        /alt/path
Options:	noallwrite noclobber nocompress unlocked nomodtime normdir
SubmitOptions:	submitunchanged
Stream:	//stream/main
StreamAtChange:	@200
LineEnd:	local
ServerID:	testid
View:
        //depot/... //test_client/..."

  DEPOTSPEC = \
"Depot:	spec
Owner:	test
Date:	2012/01/01 00:00:00
Description:
        Created by test.
Type:	local
Address:	local
Suffix:	.p4s
Map:	spec/...
SpecMap:	//spec/...
        -//spec/client/..."

    GROUPSPEC = \
"Group:	test_group
MaxResults:	unset
MaxScanRows:	unset
MaxLockTime:	unset
Timeout:	43200
PasswordTimeout:	unset
Subgroups:	test_subgroup
Owners:	test_owner
Users:	test_user"

    LABELSPEC = \
"Label:	label1
Update:	2005/03/07 09:51:29
Access:	2012/01/18 11:11:29
Owner:	jkao
Description:
        Created by jkao.
Options:	unlocked noautoreload
Revision:	@1234
View:
        //depot/..."

    PROTECTSPEC = \
"Protections:
        write user * * //...
        super user test * //..."

    STREAMSPEC = \
"Stream:	//stream/main
Update:	2011/01/01 00:00:00
Access:	2012/01/01 00:00:00
Owner:	test
Name:	main
Parent:	none
Type:	mainline
Description:
        Created by test.
Options:	allsubmit unlocked toparent fromparent
Paths:
        share ...
Remapped:
        test/...	test_remap/...
Ignored:
        excluded/...
View:
        //stream/main/... ...
        //stream/main/test/... test_remap/...
        -//stream/...excluded/... ...excluded/..."

    TRIGGERSPEC = \
"Triggers:
        name form-out change \"/some/script\""

    TYPEMAPSPEC = \
"TypeMap:
        +l //....docx"

    USERSPEC = \
"User:	test
Type:	standard
Email:	test@test
Update:	2011/01/01 00:00:00
Access:	2012/01/01 00:00:00
FullName:	test
JobView:	status=open
Password:	testpass
Reviews:	//depot/path/...
"
    SERVERSPEC = \
"ServerID:	418777F0-F7A5-439B-BAFC-E790E7286188
Type:	server
Name:	Maser
Address:	1666
Services:	standard
Description:
        Created by test."


  include P4RubyTest
  def name
    "Test spec parsing"
  end

  def test_specs
    assert( p4, "Failed to create Perforce client" )
    begin
      assert( p4.connect, "Failed to connect to Perforce server" )

      p4.save_jobspec( JOBSPEC );
      job = p4.fetch_job
      job._field1 = 'some text'
      job._description = 'some more text'

      o = p4.save_job( job )
      assert( o.length == 1 )
      assert_equal( o[ 0 ], 'Job job000001 saved.' )

      spec = p4.fetch_jobspec
      assert( spec.class == P4::Spec )
      spec = p4.format_jobspec( spec )
      assert( spec.class == String )
      spec = p4.parse_jobspec( spec )
      assert( spec.class == P4::Spec )

      spec = p4.fetch_job( 'job000001' )
      assert( spec.has_key?( 'Field1' ) )

      #
      #	Test the Spec Manager
      #

      #	Branch
      assert( branch = p4.parse_branch( BRANCHSPEC ), "Failed to format branch spec" )
      assert_kind_of( P4::Spec, branch, "Branch spec is not a P4::Spec" )
      assert_not_nil( branch['Branch'], "Key 'Branch' missing from branch P4::Spec")
      assert_not_nil( branch['Update'], "Key 'Update' missing from branch P4::Spec")
      assert_not_nil( branch['Access'], "Key 'Access' missing from branch P4::Spec")
      assert_not_nil( branch['Owner'], "Key 'Owner' missing from branch P4::Spec")
      assert_not_nil( branch['Description'], "Key 'Description' missing from branch P4::Spec")
      assert_not_nil( branch['Options'], "Key 'Options' missing from branch P4::Spec")
      assert_not_nil( branch['View'], "Key 'View' missing from branch P4::Spec")

      #	Change
      assert( change = p4.parse_change( CHANGESPEC ), "Failed to format change spec" )
      assert_kind_of( P4::Spec, change, "Change spec is not a P4::Spec" )
      assert_not_nil( change['Change'], "Key 'Change' missing from change P4::Spec")
      assert_not_nil( change['Date'], "Key 'Date' missing from change P4::Spec")
      assert_not_nil( change['Client'], "Key 'Client' missing from change P4::Spec")
      assert_not_nil( change['User'], "Key 'User' missing from change P4::Spec")
      assert_not_nil( change['Status'], "Key 'Status' missing from change P4::Spec")
      assert_not_nil( change['Type'], "Key 'Type' missing from change P4::Spec")
      assert_not_nil( change['Description'], "Key 'Description' missing from change P4::Spec")
      assert_not_nil( change['JobStatus'], "Key 'JobStatus' missing from change P4::Spec")
      assert_not_nil( change['Jobs'], "Key 'Jobs' missing from change P4::Spec")
      assert_not_nil( change['Files'], "Key 'Files' missing from change P4::Spec")

      #	Client
      assert( client = p4.parse_client( CLIENTSPEC ), "Failed to format client spec" )
      assert_kind_of( P4::Spec, client, "Client spec is not a P4::Spec" )
      assert_not_nil( client['Client'], "Key 'Client' missing from client P4::Spec")
      assert_not_nil( client['Update'], "Key 'Update' missing from client P4::Spec")
      assert_not_nil( client['Access'], "Key 'Access' missing from client P4::Spec")
      assert_not_nil( client['Owner'], "Key 'Owner' missing from client P4::Spec")
      assert_not_nil( client['Host'], "Key 'Host' missing from client P4::Spec")
      assert_not_nil( client['Description'], "Key 'Description' missing from client P4::Spec")
      assert_not_nil( client['Root'], "Key 'Root' missing from client P4::Spec")
      assert_not_nil( client['AltRoots'], "Key 'AltRoots' missing from client P4::Spec")
      assert_not_nil( client['Options'], "Key 'Options' missing from client P4::Spec")
      assert_not_nil( client['SubmitOptions'], "Key 'SubmitOptions' missing from client P4::Spec")
      assert_not_nil( client['LineEnd'], "Key 'LineEnd' missing from client P4::Spec")
      assert_not_nil( client['Stream'], "Key 'Stream' missing from client P4::Spec")
      assert_not_nil( client['StreamAtChange'], "Key 'StreamAtChange' missing from client P4::Spec")
      assert_not_nil( client['ServerID'], "Key 'ServerID' missing from client P4::Spec")
      assert_not_nil( client['View'], "Key 'View' missing from client P4::Spec")

      #	Depot
      assert( depot = p4.parse_depot( DEPOTSPEC ), "Failed to format depot spec" )
      assert_kind_of( P4::Spec, depot, "Depot spec is not a P4::Spec" )
      assert_not_nil( depot['Depot'], "Key 'Depot' missing from depot P4::Spec")
      assert_not_nil( depot['Owner'], "Key 'Owner' missing from depot P4::Spec")
      assert_not_nil( depot['Date'], "Key 'Date' missing from depot P4::Spec")
      assert_not_nil( depot['Description'], "Key 'Description' missing from depot P4::Spec")
      assert_not_nil( depot['Type'], "Key 'Type' missing from depot P4::Spec")
      assert_not_nil( depot['Address'], "Key 'Address' missing from depot P4::Spec")
      assert_not_nil( depot['Suffix'], "Key 'Suffix' missing from depot P4::Spec")
      assert_not_nil( depot['Map'], "Key 'Map' missing from depot P4::Spec")
      assert_not_nil( depot['SpecMap'], "Key 'SpecMap' missing from depot P4::Spec")

      #	Group
      assert( group = p4.parse_group( GROUPSPEC ), "Failed to format group spec" )
      assert_kind_of( P4::Spec, group, "Group spec is not a P4::Spec" )
      assert_not_nil( group['Group'], "Key 'Group' missing from group P4::Spec")
      assert_not_nil( group['MaxResults'], "Key 'MaxResults' missing from group P4::Spec")
      assert_not_nil( group['MaxScanRows'], "Key 'MaxScanRows' missing from group P4::Spec")
      assert_not_nil( group['MaxLockTime'], "Key 'MaxLockTime' missing from group P4::Spec")
      assert_not_nil( group['Timeout'], "Key 'Timeout' missing from group P4::Spec")
      assert_not_nil( group['PasswordTimeout'], "Key 'PasswordTimeout' missing from group P4::Spec")
      assert_not_nil( group['Subgroups'], "Key 'Subgroups' missing from group P4::Spec")
      assert_not_nil( group['Owners'], "Key 'Owners' missing from group P4::Spec")
      assert_not_nil( group['Users'], "Key 'Users' missing from group P4::Spec")

      #	Label
      assert( label = p4.parse_label( LABELSPEC ), "Failed to format label spec" )
      assert_kind_of( P4::Spec, label, "Label spec is not a P4::Spec" )
      assert_not_nil( label['Label'], "Key 'Label' missing from label P4::Spec")
      assert_not_nil( label['Update'], "Key 'Update' missing from label P4::Spec")
      assert_not_nil( label['Access'], "Key 'Access' missing from label P4::Spec")
      assert_not_nil( label['Owner'], "Key 'Owner' missing from label P4::Spec")
      assert_not_nil( label['Description'], "Key 'Description' missing from label P4::Spec")
      assert_not_nil( label['Options'], "Key 'Options' missing from label P4::Spec")
      assert_not_nil( label['Revision'], "Key 'Revision' missing from label P4::Spec")
      assert_not_nil( label['View'], "Key 'View' missing from label P4::Spec")

      #	Protect
      assert( protect = p4.parse_protect( PROTECTSPEC ), "Failed to format protect spec" )
      assert_kind_of( P4::Spec, protect, "Protect spec is not a P4::Spec" )
      assert_not_nil( protect['Protections'], "Key 'Protections' missing from protect P4::Spec")

      #	Stream
      assert( stream = p4.parse_stream( STREAMSPEC ), "Failed to format stream spec" )
      assert_kind_of( P4::Spec, stream, "Stream spec is not a P4::Spec" )
      assert_not_nil( stream['Stream'], "Key 'Stream' missing from stream P4::Spec")
      assert_not_nil( stream['Update'], "Key 'Update' missing from stream P4::Spec")
      assert_not_nil( stream['Access'], "Key 'Access' missing from stream P4::Spec")
      assert_not_nil( stream['Owner'], "Key 'Owner' missing from stream P4::Spec")
      assert_not_nil( stream['Name'], "Key 'Name' missing from stream P4::Spec")
      assert_not_nil( stream['Parent'], "Key 'Parent' missing from stream P4::Spec")
      assert_not_nil( stream['Type'], "Key 'Type' missing from stream P4::Spec")
      assert_not_nil( stream['Description'], "Key 'Description' missing from stream P4::Spec")
      assert_not_nil( stream['Options'], "Key 'Options' missing from stream P4::Spec")
      assert_not_nil( stream['Paths'], "Key 'Paths' missing from stream P4::Spec")
      assert_not_nil( stream['Remapped'], "Key 'Remapped' missing from stream P4::Spec")
      assert_not_nil( stream['Ignored'], "Key 'Ignored' missing from stream P4::Spec")
      assert_not_nil( stream['View'], "Key 'View' missing from stream P4::Spec")

      #	Triggers
      assert( trigger = p4.parse_triggers( TRIGGERSPEC ), "Failed to format trigger spec" )
      assert_kind_of( P4::Spec, trigger, "Trigger spec is not a P4::Spec" )
      assert_not_nil( trigger['Triggers'], "Key 'Triggers' missing from trigger P4::Spec")

      #	Typemap
      assert( typemap = p4.parse_typemap( TYPEMAPSPEC ), "Failed to format typemap spec" )
      assert_kind_of( P4::Spec, typemap, "TypeMap spec is not a P4::Spec" )
      assert_not_nil( typemap['TypeMap'], "Key 'TypeMap' missing from typemap P4::Spec")

      #	User
      assert( user = p4.parse_user( USERSPEC ), "Failed to format user spec" )
      assert_kind_of( P4::Spec, user, "User spec is not a P4::Spec" )
      assert_not_nil( user['User'], "Key 'User' missing from user P4::Spec")
      assert_not_nil( user['Type'], "Key 'Type' missing from user P4::Spec")
      assert_not_nil( user['Update'], "Key 'Update' missing from user P4::Spec")
      assert_not_nil( user['Access'], "Key 'Access' missing from user P4::Spec")
      assert_not_nil( user['FullName'], "Key 'FullName' missing from user P4::Spec")
      assert_not_nil( user['JobView'], "Key 'JobView' missing from user P4::Spec")
      assert_not_nil( user['Password'], "Key 'Password' missing from user P4::Spec")
      assert_not_nil( user['Reviews'], "Key 'Reviews' missing from user P4::Spec")

      #	Server
      assert( server = p4.parse_server( SERVERSPEC ), "Failed to format server spec" )
      assert_kind_of( P4::Spec, server, "Server spec is not a P4::Spec" )
      assert_not_nil( server['ServerID'], "Key 'ServerID' missing from server P4::Spec")
      assert_not_nil( server['Type'], "Key 'Type' missing from server P4::Spec")
      assert_not_nil( server['Name'], "Key 'Name' missing from server P4::Spec")
      assert_not_nil( server['Address'], "Key 'Address' missing from server P4::Spec")
      assert_not_nil( server['Services'], "Key 'Services' missing from server P4::Spec")
      assert_not_nil( server['Description'], "Key 'Description' missing from server P4::Spec")
    ensure
      p4.disconnect
    end
  end
end
