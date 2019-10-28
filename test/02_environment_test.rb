# vim:ts=2:sw=2:et:
#-------------------------------------------------------------------------------
# Copyright (c) 1997-2014, Perforce Software, Inc.  All rights reserved.
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
require 'rbconfig'

class TC_Enviro < Test::Unit::TestCase
  include P4RubyTest

  def name
    'Test client environment'
  end

  def test_enviro
    assert(p4, 'Failed to create Perforce client')

    set_supported = !((RbConfig::CONFIG['target_os'].downcase =~ /mswin|mingw|darwin/).nil?)
    if !p4.env('P4DESCRIPTION') && set_supported
      assert(p4.set_env('P4DESCRIPTION', 'foo'), 'Cannot set P4DESCRIPTION in registry')
      assert_equal('foo', p4.env('P4DESCRIPTION'))
      assert(p4.set_env('P4DESCRIPTION', ''), 'Cannot clear P4DESCRIPTION from registry')
    end

    p4.user = 'tony'
    p4.client = 'myworkstation'
    p4.port = 'myserver:1666'
    p4.password = 'mypass'
    p4.prog = 'somescript'
    p4.version = '2007.3/12345'

    assert_equal(p4.user, 'tony')
    assert_equal(p4.client, 'myworkstation')
    assert_equal(p4.port, 'myserver:1666')
    assert_equal(p4.password, 'mypass')
    assert_equal(p4.prog, 'somescript')
    assert_equal(p4.version, '2007.3/12345')

    p4.enviro_file = '/tmp/enviro_file'
    assert_equal(p4.enviro_file, '/tmp/enviro_file')
  end
end
