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

class TC_Shelve < Test::Unit::TestCase
  include P4RubyTest

  def name
    "Test shelving"
  end

  def test_shelving
    puts "16 - Shelving test"
    assert( p4.connect, "Failed to connect to Perforce server" )

    assert( create_client, "Failed to create test workspace" )

    # Add test files
    %w{ foo bar baz }.each do
      |name|
      File.open( name, "w" ) do
        |f|
        f.puts( "Test" )
      end
      p4.run_add( name )
    end
    p4.run_submit( '-dtest' )


    # make sure I have some files available, and get no warning whilst loading them
    p4.run_sync('#0')
    p4.run_sync

    # edit some of them
    %w{ foo bar baz }.each do
      |fn|
      p4.run_edit(fn)
      File.open( fn, "a" ) do
        |f|
        f.puts( "Change for a shelf" )
      end
    end

    # create a pending change
    ch = p4.fetch_change
    ch._description = 'My shelf'

    # shelve the lot

    s = p4.save_shelve(ch).shift
    c = s['change']

    # revert local files
    p4.run_revert('...')
    assert( p4.run_opened.length == 0, "Shouldn't have any open files" )

    # unshelve it again
    p4.run_unshelve('-s', c, '-f')
    assert( p4.run_opened.length == 3, "None or not all files unshelved" )

    # and delete the shelve
    p4.delete_shelve(c)
    p4.run_revert('...')

  end

end
