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


class TC_Info < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test p4 info"
  end

  def test_p4_info
    puts "01 - Info test"
    assert( p4, "Failed to create Perforce client" )
    begin
      assert( p4.connect, "Failed to connect to Perforce server" )
      assert( p4.connected?, "Connection dropped" )

      info = p4.run_info
      assert( info.kind_of?( Array ), "P4#run_info didn't return an array" )

      info = info.shift
      assert( info.kind_of?( Hash ), "P4#run_info not in tagged mode?" )
      assert_equal( info[ 'serverRoot' ], server_root, "Incorrect server root" )
    ensure
      p4.disconnect
    end
  end
end
