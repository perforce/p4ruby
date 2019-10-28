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

class TC_Files < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test file operations"
  end

  def test_add_files
    assert( p4, "Failed to create Perforce client" )
    begin
      assert( p4.connect, "Failed to connect to Perforce server" )
      assert( create_client, "Failed to create client workspace" )
      assert( p4.run_opened.length == 0, "Shouldn't have any open files" )
      Dir.mkdir( "test_files" )
      %w{ foo bar baz }.each do
        |fn|
        fn = "test_files/#{fn}.txt"
        File.open( fn, "w" ) do
          |f|
          f.puts( "This is a test file" )
        end
        p4.run_add( fn )
      end
      assert( p4.run_opened.length == 3, "Unexpected number of open files" )

      change = p4.fetch_change
      assert( change.kind_of?( P4::Spec ), "Change form is not a spec" )
      change._description = "Add some test files\n"
      assert( change._description == "Add some test files\n",
             "Change description not set properly" )
      assert_submit( "Failed to add files", change )

      # Ensure no files are open and that all files are present
      assert( p4.run_opened.length == 0 )
      assert( p4.run_files( 'test_files/...' ).length == 3 )

      # Now edit the files, and submit another revision.
      assert( p4.run_edit( 'test_files/...' ).length == 3 )
      change = p4.fetch_change
      change._description = "Editing the test files"
      assert_submit( "Failed to submit edits", change )
      assert( p4.run_opened.length == 0 )

      # Now delete the test_files 
      assert( p4.run_delete( 'test_files/...' ) )
      change = p4.fetch_change
      change._description = "Delete the test files"
      assert_submit( "Failed to delete test files", change )
      assert( p4.run_opened.length == 0 )

      # Now re-add the test_files
      assert( p4.run_sync( 'test_files/...#2' ) )
      assert( p4.run_add( 'test_files/...' ) )
      change = p4.fetch_change
      change._description = "Re-add the test files"
      assert_submit( "Failed to re-add test files", change )
      assert( p4.run_opened.length == 0 )

      # Now branch the files
      assert( p4.run_integ( 'test_files/...', 'test_branch/...' ) )
      change = p4.fetch_change
      change._description = "Branching the test files"
      assert_submit( "Failed to submit branch1", change )
      assert( p4.run_opened.length == 0 )

      # And now branch them again
      assert( p4.run_integ( 'test_files/...', 'test_branch2/...' ) )
      change = p4.fetch_change
      change._description = "Branching the test files again"
      assert_submit( "Failed to submit branch2", change )
      assert( p4.run_opened.length == 0 )

      # Now check out 'p4 filelog'
      files = p4.run_filelog( 'test_files/...' )
      assert( files.length == 3 )

      df = files[ 0 ]
      assert( df.depot_file == "//depot/test_files/bar.txt", df.depot_file )
      assert( df.revisions.length == 4 )

      # Check that the latest revision is as expected
      rev = df.revisions[ 0 ]
      assert( rev.rev == 4 )
      assert( rev.type == "text" )
      assert( rev.time.kind_of?( Time ) )
      # Removing this as p4d >= 2010.2 shows 3 integ records, while earlier
      # servers show only 2.
      # assert( rev.integrations.length == 2 )
      assert( rev.integrations[ 0 ].how == "branch into" )
      assert( rev.integrations[ 0 ].file == "//depot/test_branch/bar.txt" )

      # Revision #3 is a delete, so it should not have a digest 
      rev = df.revisions[ 1 ]
      assert( rev.rev == 3 )
      assert( rev.action == "delete" )
      assert( rev.digest == nil )
    ensure
      p4.disconnect
    end
  end

  #
  # Local method to help ensure submits are working
  #
  def assert_submit( msg, *args )
    assert_block( msg ) do
      begin
        result = @p4.run_submit( args )
        if( result[-1].has_key?( 'submittedChange' ) )
          true
        else
          false
        end
      rescue P4Exception
        false
      end
    end
  end

end
