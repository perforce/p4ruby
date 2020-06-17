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

class TC_Client < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test client workspace creation"
  end

  def test_client_creation
    puts "03 - Client creation test"
    assert( p4, "Failed to create Perforce client" )
    begin
      assert( p4.connect, "Failed to connect to Perforce server" )

      client = p4.fetch_client
      assert( client.kind_of?( P4::Spec ), "Client wasn't a P4::Spec object" )

      client._root = client_root()
      client._description = "Test client\n"
      assert_block( "Failed to save Perforce client" ) do
        begin
          p4.save_client( client )
          true
        rescue P4Exception
          false
        end
      end

      client2 = p4.fetch_client
      assert( client2.kind_of?( P4::Spec ), "Client2 wasn't a P4::Spec object" )
      assert_equal( client._root, client2._root, "Client roots differ" )
      assert_equal( client._description, client2._description,
                   "Client descriptions differ" )
    ensure
      p4.disconnect
    end
  end
end
