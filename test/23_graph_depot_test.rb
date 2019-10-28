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

class TC_GraphDepot < Test::Unit::TestCase

  include P4RubyTest

  MIN_SERVER_LEVEL  = 42
  MIN_API_LEVEL     = 81

  def name
    "Test graph depot"
  end

  def test_graph_depot
    assert( p4, "Failed to create Perforce client" )

    #	Create a graph depot
    begin
      assert( p4.connect, "Failed to connect to Perforce server" )
      depot = p4.fetch_depot( "GD0" )

      # Check server_level to ensure that the server support graph depot.
      if( p4.server_level >= MIN_SERVER_LEVEL && p4.api_level >= MIN_API_LEVEL )
        p4.run_info()
        assert( p4.graph?, "Graph depots are not enabled" )

        # Create a new graph depot and make sure that it's listed.
        existing = p4.run_depots.length
        depot[ 'type' ] = "graph"
        p4.save_depot( depot )
        assert_equal( existing + 1, p4.run_depots.length, "Graph depot not created" )

        #	Disable graph
        assert( p4.api_level >= MIN_API_LEVEL, "API level (#{p4.api_level}) too low" )
        p4.graph=false
        assert( !p4.graph?, "Failed to disable graph depots" )

        len = p4.run_depots.length
        assert_equal( 1, len, "Graph depot included in depots command" )

        #	Enable graph and set the api_level < 81
        p4.graph=true
        assert( p4.graph?, "Failed to enable graph depots" )
        old_level = p4.api_level
        p4.api_level = MIN_SERVER_LEVEL - 1
        assert( p4.api_level < MIN_SERVER_LEVEL, "API level (#{p4.api_level}) too high" )
        len = p4.run_depots.length
        assert_equal( 1, len, "Graph depot included in depots command" )
        p4.api_level = old_level
      end
    ensure
      p4.disconnect
    end

    begin
      assert( p4.connect, "Failed to connect to Perforce server" )
      if( p4.server_level >= MIN_SERVER_LEVEL && p4.api_level >= MIN_API_LEVEL )
        len = p4.run_repos.length
        assert_equal( 0, len, "Failed to get repos list" )
      end
    ensure
      p4.disconnect
    end
  end

  def test_repos
    def make_user(p4, u)
      user = p4.fetch_user(u)
      p4.input = user
      p4.run('user', '-i', '-f')
    end

    begin
      assert( p4, "Failed to create Perforce client" )
      assert( p4.connect, "Failed to connect to Perforce server" )

      # Check server_level to ensure that the server support graph depot.
      if( p4.server_level < MIN_SERVER_LEVEL || p4.api_level < MIN_API_LEVEL )
        puts "Skipping test, server or api level is too low"
        return
      end

      depot = p4.fetch_depot( "-t", "graph", "GD1" )
      p4.save_depot( depot )

      # create some users: u_dev, u_useless
      make_user(p4, 'u_dev')
      make_user(p4, 'u_useless')

      # set GD1 permissions: read acces for u_useless, create-repo for u_dev
      p4.run('grant-permission','-d','GD1','-p','read','-u','u_useless')
      p4.run('grant-permission','-d','GD1','-p','write-all','-u','u_dev')
      p4.run('grant-permission','-d','GD1','-p','create-repo','-u','u_dev')

      perms=p4.run('show-permission','-d','GD1')
      # just make sure there are 4, the original owner permission and the 3 we just set
      assert_equal( 4, perms.length, "Wrong number of permissions: #{perms}")

      # create a repo, leave permissions the defaults
      repo = p4.fetch_repo('//GD1/pizza')
      p4.save_repo(repo)
      repos = p4.run_repos
      assert_equal( 1, repos.length, "Wrong number of repos: #{repos}")

      # check default permissions (5, original 4 from depot plus owner of repo)
      perms=p4.run('show-permission','-n','//GD1/pizza')
      assert_equal( 5, perms.length, "Wrong number of permissions: #{perms}")

      # add one
      p4.run('grant-permission','-n','//GD1/pizza','-p','write-ref','-u','u_useless')
      perms=p4.run('show-permission','-n','//GD1/pizza')
      assert_equal( 6, perms.length, "Wrong number of permissions: #{perms}")

      # revoke it
      p4.run('revoke-permission','-n','//GD1/pizza','-p','write-ref','-u','u_useless')
      perms=p4.run('show-permission','-n','//GD1/pizza')
      assert_equal( 5, perms.length, "Wrong number of permissions: #{perms}")

      # check various users
      # return value is "none" or the permission we asked about
      perms=p4.run('check-permission','-n','//GD1/pizza','-p','write-ref','-u','u_dev')
      assert_not_equal( perms[0]['perm'][0], 'none', "Wrong check-permission result")

      perms=p4.run('check-permission','-n','//GD1/pizza','-p','write-ref','-u','u_useless')
      assert_equal( perms[0]['perm'][0], 'none', "Wrong check-permission result")

      # make a branch (reference) restriction, only the current user can submit
      me=p4.user
      p4.run('grant-permission','-n','//GD1/pizza','-u',me,'-p','restricted-ref','-r',
        'refs/heads/master')
      # now check if u_dev can write-ref to that specific branch
      perms=p4.run('check-permission','-n','//GD1/pizza','-r','refs/heads/master','-p','write-ref','-u','u_dev')
      assert_equal( perms[0]['perm'][0], 'none', "Wrong check-permission result")
      # also check that u_dev can write to another random ref
      perms=p4.run('check-permission','-n','//GD1/pizza','-r','refs/heads/delicious','-p','write-ref','-u','u_dev')
      assert_not_equal( perms[0]['perm'][0], 'none', "Wrong check-permission result")

    ensure
      p4.disconnect
    end
  end
end
