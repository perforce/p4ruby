# vim:ts=2:sw=2:et:
#-------------------------------------------------------------------------------
# Copyright (c) 1997-2008, Perforce Software, Inc.  All rights reserved. 
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

class TC_Maps < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test mapping routines"
  end

  def test_maps
    client_map = P4::Map.new
    assert( client_map.class == P4::Map )
    assert( client_map.empty? )

    client_map.insert( "//depot/main/...", "//ws/main/..." )
    assert_equal( client_map.count, 1 )
    assert( !client_map.empty? )
    client_map.insert( "//depot/live/...", "//ws/live/..." )
    assert_equal( client_map.count, 2 )
    client_map.insert( "//depot/bad/...", "//ws/live/bad/..." )
    assert_equal( client_map.count, 4 )

    # Basic translation
    p = client_map.translate( "//depot/main/foo/bar" )
    assert_equal( p, "//ws/main/foo/bar" )
    p = client_map.translate( "//ws/main/foo/bar", false )
    assert_equal( p, "//depot/main/foo/bar" )


    # Map joining. Create another map, and join it to the first
    ws_map = P4::Map.new( [ "//ws/... /home/user/ws/..." ] )
    assert( !ws_map.empty? )

    root_map = P4::Map.join( client_map, ws_map )
    assert( !root_map.empty? )

    # Now translate a depot path to a local path
    p = root_map.translate( "//depot/main/foo/bar" )
    assert_equal( "/home/user/ws/main/foo/bar", p )

    # Now reverse the mappings and try again
    root_map = root_map.reverse
    p = root_map.translate( "/home/user/ws/main/foo/bar" )
    assert_equal( "//depot/main/foo/bar", p )

    # Check space handling in mappings. Insert using both methods. With,
    # and without quotes.
    space_map = P4::Map.new
    space_map.insert( '"//depot/space dir1/..." "//ws/space 1/..."' )
    space_map.insert( '"//depot/space dir2/..."', '"//ws/space 2/..."')
    space_map.insert( '//depot/space dir3/...', '//ws/space 3/...')

    # Test the results using translation.
    p = space_map.translate( "//depot/space dir1/foo" )
    assert_equal( p, "//ws/space 1/foo" )

    p = space_map.translate( "//depot/space dir2/foo" )
    assert_equal( p, "//ws/space 2/foo" )

    p = space_map.translate( "//depot/space dir3/foo" )
    assert_equal( p, "//ws/space 3/foo" )

  end
end
