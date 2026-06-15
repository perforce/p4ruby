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

class TC_Password < Test::Unit::TestCase
  include P4RubyTest
  def name
    "Test 'p4 password' and 'p4 login'"
  end

  def test_password
    puts "10 - Password"
    assert(p4, 'Failed to create Perforce client')

    ticket_file = client_root + '/.non-exist'
    p4.ticket_file = ticket_file
    assert_equal(p4.ticket_file, ticket_file, 'Ticket file not set correctly')
    assert(p4.run_tickets.empty?, 'Tickets not empty for non-existent file.')

    ticket_file = client_root
    p4.ticket_file = ticket_file
    assert_equal(p4.ticket_file, ticket_file, 'Ticket file not set correctly')
    assert(p4.run_tickets.empty?, 'Tickets not empty for directory.')

    ticket_file = client_root + '/.p4tickets'
    p4.ticket_file = ticket_file
    assert_equal(p4.ticket_file, ticket_file, 'Ticket file not set correctly')
    # Use a strong (8+ char) test password so r26.1 SBD's security=4 accepts it.
    test_password = 'P4Test!Pwd99'

    begin
      assert(p4.connect, 'Failed to connect to Perforce server')
      # Ticket file was swapped to an empty file above; re-login so the
      # custom file has a valid ticket. Mirrors P4Python's testPasswords
      # "must login explicitly" step for SBD servers.
      p4.password = SUPER_PASSWORD
      p4.run_login
      assert(p4.fetch_user, 'Failed to fetch user record')
      assert(p4.run_password(SUPER_PASSWORD, test_password), 'Failed to set password')
      p4.password = test_password
      assert(p4.password == test_password, 'Password not set correctly')
      assert(p4.run_login, 'Failed to login to server')
      # Note: p4.password is set by the login above to the ticket.
      # Restore back to SUPER_PASSWORD so the bootstrap-set state is preserved.
      assert(p4.run_password(test_password, SUPER_PASSWORD), 'Failed to restore password')

      old_user = p4.user

      # After run_password above, the prior ticket is invalid. Re-login as
      # super before fetching/creating the foo:bar user spec.
      p4.password = SUPER_PASSWORD
      p4.run_login

      #	Ensure that ticket file is correctly parsed for user names
      #	that contain a ':'.
      # As super (still logged in), create foo:bar's user record and set its
      # initial password. Mirrors P4Go's TestPassword pattern (lines 1243-1250).
      user = p4.fetch_user('foo:bar')
      p4.save_user(user, '-f')
      p4.input = [test_password, test_password]
      p4.run('passwd', 'foo:bar')
      # Clear the password-reset flag so foo:bar can log in.
      p4.save_user(user, '-f')

      # Now switch to foo:bar and log in to write its ticket.
      p4.user = 'foo:bar'
      p4.password = test_password
      assert(p4.password == test_password, 'Password not set correctly')
      assert(p4.run_login, 'Failed to login to server')
      # Note: p4.password is set by the login above to the ticket.

      assert(File.exist?(ticket_file), 'Ticket file not created')

    ensure
      p4.user = old_user if old_user
      p4.disconnect
    end
    tickets = p4.run_tickets
    assert_equal(2, tickets.length, 'Unexpected number of tickets found.')
    tickets.each do
      |ticket|
      assert_kind_of(Hash, ticket, 'Ticket entry is not a Hash object')
      assert_not_nil(ticket['Host'], 'Host field not set in ticket.')
      assert_not_nil(ticket['User'], 'User field not set in ticket.')
      assert_not_nil(ticket['Ticket'], 'Ticket field not set in ticket.')
    end
  end
end
