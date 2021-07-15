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
    "Test evil twin scenario"
  end

  # add A1
  # branch A→B
  # move A1→A2
  # readd A1
  # merge A→B

  def test_evil_twin
    puts "07 - Evil twin test"
    assert( p4, "Failed to create Perforce client" )
    begin
      assert( p4.connect, "Failed to connect to Perforce server" )

      ############################
      # Prep workspace
      client = p4.fetch_client
      client._root = client_root()
      client._description = "Test client\n"
      p4.save_client(client)
      Dir.mkdir("A")

      ############################
      # Adding      
      fileA = File.new("A/fileA", "w")            
      fileA.write("Original content")
      fileA.close
      p4.run_add(fileA.path)
      p4.run_submit("-d", "adding fileA")

      ############################
      # Branching
      branch_spec = p4.run("branch", "-o", "evil-twin-test")[0]
      branch_spec._View = ['//depot/A/... //depot/B/...']
      p4.save_branch(branch_spec)
      p4.run("integ", "-b", "evil-twin-test")
      p4.run_submit("-d", "integrating")

      ############################
      # Moving
      p4.run("edit", "A/fileA")
      p4.run("move", "-f", "A/fileA", "A/fileA1")
      p4.run("submit", "-d", "moving")

      ############################
      # Re-adding origianl
      fileA = File.new( "A/fileA", "w" )           
      fileA.write("Re-added A")
      fileA.close
      p4.run_add("A/fileA")
      p4.run_submit("-d", "re-adding")


      ############################
      # Second merge
      p4.run("merge", "-b", "evil-twin-test")
      assert_raises(P4Exception) do
        assert( p4.run("submit", "-d", "integrating") , "Unexpected number of open files" )
      end

    ensure
      p4.disconnect
    end
  end
end
