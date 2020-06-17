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

class TC_Track < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test track"
  end

  def test_track
    puts "15 - Track test"
    assert( p4, "Failed to create Perforce client" )
    p4.track = true
    assert( p4.track?, "Failed to set performance tracking" )
    begin
      assert( p4.connect, "Failed to connect to Perforce server" )
      assert_block( "Performance tracking cannot be changed" ) do
        begin
          p4.track = false
          false
        rescue P4Exception
          true
        end
      end

      assert( p4.track?, "Performance tracking cannot be changed" )
      p4.run_info
      assert( !p4.track_output.empty?, "No performance tracking reported" )
      found = false
      p4.track_output.each do
        |o|
        if( o[0...3] == "rpc" )
          found = true
          break
        end
      end
      assert( found, "Failed to report expected performance tracking output" )
    ensure
      p4.disconnect
    end
  end
end
