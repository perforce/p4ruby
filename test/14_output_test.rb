# vim:ts=2:sw=2:et:
#-------------------------------------------------------------------------------
# Copyright (c) 2010, Perforce Software, Inc.  All rights reserved.
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

class TC_Output < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test output"
  end

  def test_new_output
    puts "14 - Output test"
    assert( p4, "Failed to create Perforce client" )
    begin
      # First test new output format
      assert( p4.connect, "Failed to connect to Perforce server" )
      assert( create_client, "Failed to create test workspace" )

      # Add test files
      %w{ foo bar baz }.each do
        |name|
        File.open( name, "w" ) do
          |f|
          f.puts( "Test" )
        end
        p4.run_add( name )
      end
      p4.run_submit( '-dtest' )

      # Run a 'p4 sync' and ignore the output, then run it again
      # and we should get a 'file(s) up-to-date' message that we
      # want to trap.
      p4.exception_level = P4::RAISE_NONE
      p4.run_sync
      p4.run_sync

      # Check warnings array contains the warning as a string
      assert( !p4.warnings.empty?, "Didn't get a warning!")
      w = p4.warnings[0]
      assert( w.kind_of?( String ), "Didn't get a String" )
      assert( w =~ /up-to-date/, "Didn't get expected warning" )

      # Now check messages array contains the message object
      assert( !p4.messages.empty?, "Didn't get any messages!" )
      assert( p4.messages.length == 1 )
      w = p4.messages[0]
      assert( w.kind_of?( P4::Message ), "Didn't get a P4::Message object" )
      assert( w.to_s =~ /up-to-date/, "Didn't get expected warning. Got '#{w.to_s}'" )
      assert( w.severity == P4::E_WARN, "Severity was not E_WARN" )
      assert( w.generic == P4::EV_EMPTY, "Wasn't an empty message" )
      assert( w.msgid == 6532, "Got the wrong message: #{w.msgid}" )

      # Sync to none and then sync to head - check number of info, warning
      # and error messages
      p4.run_sync( '//...#none' )
      p4.tagged = false
      p4.run_sync( '//depot/...' )
      infos = p4.messages.select { |m| m.severity == P4::E_INFO }
      warns = p4.messages.select { |m| m.severity == P4::E_WARN }
      errs  = p4.messages.select { |m| m.severity >= P4::E_FAILED }
      assert_equal(3, infos.length, "Wrong number of info messages")
      assert_equal(0, warns.length, "Wrong number of warnings" )
      assert_equal(0, errs.length, "Wrong number of errors" )

      # test getting an error's dictionary (hash)
      p4.run_dirs('//this/is/a/path/that/does/not/exist/*')
      errs  = p4.messages.select { |m| m.severity >= P4::E_FAILED }
      assert_equal(1, errs.length, "Wrong number of errors" )
      dict = errs[0].dictionary

      assert(dict.length > 0, "Empty dictionary")
      assert_not_equal(dict['fmt0'], nil, "No message format present")
      assert_not_equal(dict['code0'], nil, "No message code present")
      assert_not_equal(dict['func'], nil, "No message func present")
    ensure
      p4.disconnect
    end
  end
end
