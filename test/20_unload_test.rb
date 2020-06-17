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

class TC_Unload < Test::Unit::TestCase

  include P4RubyTest
  def name
    "Test unload depot"
  end

  def test_unload
    puts "20 - Unload test"
    assert( p4, "Failed to create Perforce client" )

    begin
      assert( p4.connect, "Failed to connect to Perforce server." )
      old_client = p4.client
      if( p4.server_level >= 33 )

        # Create our test workspace
        assert( create_client, "Failed to create test workspace" )

        #	Create an unload depot
        depot = p4.fetch_depot( "unload_depot" )
        assert_kind_of(P4::Spec, depot, "unload_test is not of type P4::Spec.")
        depot._type = "unload"
        p4.save_depot(depot)

        #	Ensure that the client is created
        clients = p4.run_clients( "-e", p4.client )
        assert_equal(1, clients.size, "Unexpected number of client workspaces." )

        # Add some files to the depot so we have something to work with
        assert( add_sample_content, "Failed to add sample content" )

        # Save our current have list
        sync = p4.run_have
        assert( sync.size > 0, "Have list was empty!" )

        #	Unload the client workspace and check it was successful
        assert_nothing_raised do
          p4.run_unload( "-c", p4.client )
        end
        clients = p4.run_clients( "-U", "-e", p4.client )
        assert_equal( 1, clients.size, "Unload depot does not contain unloaded workspace." )

        #	Reload the client workspace
        assert_nothing_raised do
          p4.run_reload( "-c", p4.client )
        end
        clients = p4.run_clients( "-U", "-e", p4.client )
        assert_equal( 0, clients.size, "Unload depot does not contain unloaded workspace." )
        have = p4.run_have
        assert_equal( sync.size, have.size, "Unexpected number of files sync'd to reloaded workspace." )
      else
        puts "\tTest Skipped: Unload depot requires a 2012.2 or later Perforce Server."
      end

    ensure
      p4.client = old_client
      p4.disconnect if p4.connected?
    end
  end
end
