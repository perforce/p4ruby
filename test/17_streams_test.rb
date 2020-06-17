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

class TC_Streams < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test streams"
  end

  def test_streams
    puts "17 - Streams test"
    assert( p4, "Failed to create Perforce client" )

    #	Create a streams depot
    begin
      assert( p4.connect, "Failed to connect to Perforce server" )
      depot = p4.fetch_depot( "Stream" )

      # Check server_level to ensure that the server support streams.
      if( p4.server_level >=30 && p4.api_level >= 70 )
        assert( p4.streams?, "Streams are not enabled" )

        # Create a new streams depot and make sure that it's listed.
        depot[ 'type' ] = "stream"
        p4.save_depot( depot )
        assert_equal( 2, p4.run_depots.length, "Streams depot not created" )

        #	Disable streams
        assert( p4.api_level >= 70, "API level (#{p4.api_level}) too low" )
        p4.streams=false
        assert( !p4.streams?, "Failed to disable streams" )

        # Note: as of 2016.1, streams depot is no reason to need the streams
        # tag to include streams depots. (From email with npoole)
        len = p4.run_depots.length
        assert_equal( 2, len, "Streams depot not included in depots command" )

        #	Enable streams and set the api_level < 70
        p4.streams=true
        assert( p4.streams?, "Failed to enable streams" )
        oldLevel = p4.api_level
        p4.api_level = 69
        assert( p4.api_level < 70, "API level (#{p4.api_level}) too high" )
        len = p4.run_depots.length
        assert_equal( 2, len, "Streams depot not included in depots command" )
        # reset the level
        p4.api_level = oldLevel

        # Fetch a stream from the server, check that
        # an 'extraTag' field (such as 'firmerThanParent' exists, and save
        # the spec
        s = p4.fetch_stream( "//Stream/MAIN" )
        assert(s.has_key?("firmerThanParent"), "'extraTag' field missing from spec." )
        s[ 'Type' ] = "mainline"
        o = p4.save_stream( s )
        assert( o.length == 1, "Unexpected output when creating a stream" )
        assert( o[0] =~ /saved/, "Failed to create a stream" )
      else
        puts "\tTest Skipped: Streams requires a 2011.1 or later Perforce Server and P4API."
      end
    ensure
      p4.disconnect
    end

    begin
      assert( p4.connect, "Failed to connect to Perforce server" )
      if( p4.server_level >=30 && p4.api_level >= 70 )
        len = p4.run_streams.length
        assert_equal( 1, len, "Failed to save stream spec" )
        p4.run_streams
      end
    ensure
      p4.disconnect
    end
  end
end
