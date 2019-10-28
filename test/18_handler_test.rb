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

class TC_OutputHandler < Test::Unit::TestCase

  include P4RubyTest

  class MyOutputHandler < P4::OutputHandler
    def initialize
      @statOutput = Array.new
      @infoOutput = Array.new
      @msgOutput = Array.new
    end

    attr_reader :statOutput, :infoOutput, :msgOutput

    def outputStat( stat )
      @statOutput.push( stat )
      return P4::HANDLED
    end

    def outputInfo( info )
      @infoOutput.push( info )
      return P4::HANDLED
    end

    def outputMessage( msg )
      @msgOutput.push( msg )
      return P4::HANDLED
    end
  end

  def name
    "Test output handler"
  end

  def test_outputHandler
    h = P4::ReportHandler.new
    assert( h.class == P4::ReportHandler, "Failed to create P4::OutputHandler" )
    assert( p4, "Failed to create Perforce client" )

    begin
      assert( p4.connect, "Failed to connect to Perforce server" )
      assert( create_client, "Failed to create test workspace" )

      # Create a set of files to add, that can then be queried with our new
      # handler
      dir = "handler_files"
      Dir.mkdir( dir )
      files = %w{ foo bar baz }
      files.each do
        |fn|
        fn = "#{dir}/#{fn}.txt"
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
      assert_submit( p4, "Failed to add files", change )

      # Ensure no files are open
      assert( p4.run_opened.length == 0 )

      # Create an instance of our new output handler
      h = MyOutputHandler.new
      assert( h.class == MyOutputHandler, "Failed to create MyOutputHandler" )

      # Check that the output goes into the Handler object and that the
      # Handler object contains the correct number of files
      p4.with_handler( h ) do
        assert_equal( 0, p4.run_files( "#{dir}/..." ).length, "Does not return empty list")
      end
      assert_equal( files.length, h.statOutput.length, "Less files than expected" )
      assert_equal( 0, h.msgOutput.length, "Unexpected messages" )
    ensure
      p4.disconnect
    end
  end

  #
  # Local method to help ensure submits are working
  #
  def assert_submit( p4, msg, *args )
    assert_block( msg ) do
      begin
        result = p4.run_submit( args )
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
