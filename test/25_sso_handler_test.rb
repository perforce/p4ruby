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
require 'fileutils'

class TC_SSO < Test::Unit::TestCase

  include P4RubyTest

  def name
    "SSO handler"
  end

  def setup
    super

    begin
      assert( p4, "Failed to create Perforce client" )

      # verify that we get a "you need to accept" when connecting
      begin
        p4.connect
        triggers = p4.fetch_triggers
        assert_equal( {}, triggers )
        triggers['Triggers'] = [ 'loginsso auth-check-sso auth pass' ]
        assert_equal( [ 'Triggers saved.' ], p4.save_triggers( triggers ) )
        assert_equal( triggers, p4.fetch_triggers )
	# Set the log so we don't flood stderr
        assert_equal( [{"Action"=>"set", "Name"=>"P4LOG", "ServerName"=>"any", "Value"=>"log"}], p4.run_configure( 'set', 'P4LOG=log' ) )
      rescue P4Exception => e
        assert( false, "Exception thrown: #{e}" )
      ensure
        p4.disconnect
      end
    end
  end

  def test_default
    begin
      puts "25 - SSO handler test"
      assert( p4, "Failed to create Perforce client" )

      # verify that we get a "you need to accept" when connecting
      begin
        p4.connect
        assert_equal( nil, p4.loginsso )
        begin
          p4.run_login( 'Passw0rd' )
        rescue P4Exception => e
          assert( e.to_s.include?("[Error]: Perforce password (P4PASSWD) invalid or unset."), "Exception thrown: #{e}" )
        end
        assert_equal( {}, p4.ssovars )
      rescue P4Exception => e
        assert( false, "Exception thrown: #{e}" )
      ensure
        p4.disconnect
      end
    end
  end

  def test_disabled
    begin
      assert( p4, "Failed to create Perforce client" )

      # verify that we get a "you need to accept" when connecting
      begin
        p4.connect
        p4.loginsso = false
        assert_equal( false, p4.loginsso )
        begin
          p4.run_login( 'Passw0rd' )
        rescue P4Exception => e
          assert( e.to_s.include?("[Error]: Perforce password (P4PASSWD) invalid or unset."), "Exception thrown: #{e}" )
        end
        assert_equal( {}, p4.ssovars )
      rescue P4Exception => e
        assert( false, "Exception thrown: #{e}" )
      ensure
        p4.disconnect
      end
    end
  end

  def test_enabled
    begin
      assert( p4, "Failed to create Perforce client" )

      # verify that we get a "you need to accept" when connecting
      begin
        p4.connect
        p4.loginsso = true
        assert_equal( true, p4.loginsso )
        begin
          assert_equal( [], p4.run_login( 'Passw0rd' ) )
        rescue P4Exception => e
          assert( e.to_s.include?("[Error]: Perforce password (P4PASSWD) invalid or unset."), "Exception thrown: #{e}" )
        end
        assert_equal( {}, p4.ssovars )
      rescue P4Exception => e
	assert( false, "Exception thrown: #{e}" )
      ensure
        p4.disconnect
      end
    end
  end

  def test_enabled_alt_login
    begin
      assert( p4, "Failed to create Perforce client" )

      # verify that we get a "you need to accept" when connecting
      begin
        p4.connect
        p4.loginsso = true
        assert_equal( true, p4.loginsso )
        begin
          assert_equal( [], p4.run( 'login' ) )
        rescue P4Exception => e
          assert( false, "Exception thrown: #{e}" )
        end
        assert_equal( ['user', 'serverAddress', 'P4PORT', 'ssoArgs', 'data'], p4.ssovars.keys )
      rescue P4Exception => e
        assert( false, "Exception thrown: #{e}" )
      ensure
        p4.disconnect
      end
    end
  end

  def test_fail_result
    begin
      assert( p4, "Failed to create Perforce client" )

      # verify that we get a "you need to accept" when connecting
      begin
        p4.connect
        p4.loginsso = true
        assert_equal( true, p4.loginsso )
        assert_equal( nil, p4.ssofailresult )
        assert_equal( nil, p4.ssopassresult )
        p4.ssofailresult = 'My bad result!'
        assert_equal( 'My bad result!', p4.ssofailresult )
        assert_equal( nil, p4.ssopassresult )

        begin
          assert_equal( [], p4.run( 'login' ) )
        rescue P4Exception => e
          assert( e.to_s.include?("[Error]: Single sign-on on client failed: My bad result!"), "Exception thrown: #{e}" )
        end
        assert_equal( [], p4.ssovars.keys )
      rescue P4Exception => e
        assert( false, "Exception thrown: #{e}" )
      ensure
        p4.disconnect
      end
    end
  end

  def test_pass_result
    begin
      assert( p4, "Failed to create Perforce client" )

      # verify that we get a "you need to accept" when connecting
      begin
        p4.connect
        p4.loginsso = true
        assert_equal( true, p4.loginsso )
        assert_equal( nil, p4.ssofailresult )
        assert_equal( nil, p4.ssopassresult )
        p4.ssopassresult = 'My good result!'
        assert_equal( 'My good result!', p4.ssopassresult )
        assert_equal( nil, p4.ssofailresult )

        begin
          assert_equal( ["User", "TicketExpiration"], p4.run( 'login' )[0].keys )
        rescue P4Exception => e
          assert( e.to_s.include?("[Error]: Single sign-on on client failed: My bad result!"), "Exception thrown: #{e}" )
        end
        assert_equal( [], p4.ssovars.keys )
      rescue P4Exception => e
        assert( false, "Exception thrown: #{e}" )
      ensure
        p4.disconnect
      end
    end
  end
end
