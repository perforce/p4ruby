# vim:ts=2:sw=2:et:
#-------------------------------------------------------------------------------
# Copyright (c) 2012, Perforce Software, Inc.  All rights reserved.
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

class TC_Progress < Test::Unit::TestCase

  include P4RubyTest
  class SubmitProgress < P4::Progress
    def initialize
      @types = Array.new
      @descs = Array.new
      @totals = Array.new
      @positions = Array.new
      @fails = Array.new
    end

    attr_reader :types, :descs, :totals, :positions, :fails

    def init( type )
      @types.push( type )
    end

    def description( desc, units )
      h = { desc => units }
      @descs.push( h )
    end

    def update( position )
      @positions.push( position )
    end

    def total( total_data )
      @totals.push( total_data )
    end

    def done( fail )
      @fails.push( fail )
    end
  end

  class SyncHandler < P4::OutputHandler
    def initialize
      @totalFiles = 0
      @totalSizes = 0
    end

    attr_reader :totalFiles, :totalSizes

    def outputStat( stat )
      @totalFiles = stat[ "totalFileCount" ].to_i if( stat.include?( "totalFileCount" ) )
      @totalSizes = stat[ "totalFileSize" ].to_i if( stat.include?( "totalFileSize" ) )
      return P4::HANDLED
    end

    def outputInfo( info )
      return P4::HANDLED
    end

    def outputMessage( msg )
      return P4::HANDLED
    end
  end

  class SyncProgress < P4::Progress
    def initialize
      @type = 0
      @position = 0
      @fail = 0
    end

    attr_reader :position

    def init( type )
      @type = type
    end

    def description( desc, units )
    end

    def update( position )
      @position = position
    end

    def total( total_data )
    end

    def done( fail )
      @fail = fail
    end

  end

  def name
    "Test Progress API"
  end

  def test_progress
    assert( p4, "Failed to create Perforce client" )
    begin
      assert( p4.connect, "Failed to connect to Perforce server." )
      assert( create_client, "Failed to create test workspace" )
      if( p4.server_level >= 33 )

        #	Ensure that an exception is raised if progress is not set to a P4::Progress
        assert_raises P4Exception do
          p4.progress = Array.new
        end

        p4.progress = SubmitProgress.new

        #	Create some dummy files to test submit progress
        Dir.mkdir( "progress_test" )
        total = 50
        total.times do
          |i|
          fn = "progress_test/file#{i.to_s}.txt"
          File.open( fn, "w" ) do
            |f|
            f.puts( "*" * 1024 )
          end
          p4.run_add( fn )
        end
        assert( p4.run_opened.length == total, "Unexpected number of open files" )
        p4.run_submit( "-dSubmit some files." )

        assert_equal( total, p4.progress.types.length, "Did not receive #{total} progress init calls" )
        assert_equal( total, p4.progress.descs.length, "Did not receive #{total} progress desc calls" )
        assert_equal( total, p4.progress.totals.length, "Did not receive #{total} progress total calls" )
        assert_equal( total, p4.progress.positions.length, "Did not receive #{total} progress position calls" )
        assert_equal( total, p4.progress.fails.length, "Did not receive #{total} progress done calls" )

        # Ensure no files are open and that all files are present
        assert( p4.run_opened.length == 0 )
        assert( p4.run_files( 'progress_test/...' ).length == total )

        # Quiet sync surpressed all info messages prior to 2014.1, so
        # this test will fail against 2012.2 - 2013.3 servers.  Now skip
        # those versions as the behaviour in the server has changed.
        if( p4.server_level >= 37 )
          p4.handler = SyncHandler.new
          p4.progress = SyncProgress.new
          p4.run_sync( "-f", "-q", "//..." )
          assert_equal( p4.handler.totalFiles, p4.progress.position, "Total does not match position." )
        end
      end
    ensure
      p4.run_revert( "//..." ) unless p4.run_opened.empty?
      p4.disconnect if p4.connected?
    end
  end
end
