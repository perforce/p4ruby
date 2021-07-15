# encoding: UTF-8
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

class TC_Unicode < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test unicode support"
  end

  def test_unicode
    enable_unicode()
    puts "98 - Unicode test"
    assert( p4, "Failed to create Perforce client object" )
    p4.charset = "iso8859-1"

    begin
      assert( p4.connect, "Failed to connect to Perforce server" )
      assert( p4.server_unicode?, "Server should be in unicode mode" )
      assert( create_client, "Failed to create test workspace" )

      # Add a test file with a £ sign in it. That has the high bit set
      # so we can test it in both iso8859-1 and utf-8

      tf = "unicode.txt"
      File.open( tf, "w" ) do
        |f|
        f.puts( "This file cost \xa31" )
      end

      p4.run_add( tf )
      assert( p4.run_opened.length() == 1, "There should be only 1 file open")

      p4.run_submit( '-d', "Add unicode test file" )
      assert( p4.run_opened.empty?, "There should be no files open")

      # Now remove the file from the workspace, disconnect, switch to
      # utf8, reconnect and resync the file. Then we'll print it and
      # see that the content contains the unicode sequence for the £
      # symbol.

      p4.run_sync( "#{tf}#none" )
    ensure
      p4.disconnect
    end

    p4.charset = "utf8"
    begin
      assert( p4.connect, "Failed to connect to Perforce server" )
      p4.run_sync
      buf = ""
      File.open( tf ) do
        |f|
        buf = f.read.chomp
      end

      #	The test is subtly different between ruby versions.  For ruby
      #	1.9 we need to force the encoding to 'UTF-8' (rather than the
      #	current loca), because 'p4.charset'	has been set to 'utf8'.
      if RUBY_VERSION.split('.')[0].to_i >= 2 ||
          (RUBY_VERSION.split('.')[0].to_i == 1 and RUBY_VERSION.split('.')[1].to_i > 8)
        buf.force_encoding( "UTF-8" )
        assert( buf == 'This file cost £1', "Unicode support broken" )
      else
        assert( buf == "This file cost \xC2\xA31", "Unicode support broken" )
      end
    ensure
      p4.disconnect
    end
  end
end
