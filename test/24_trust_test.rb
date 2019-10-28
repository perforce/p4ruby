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

class TC_Trust < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test trust file settings"
  end

  def setup
    super

    # create the ssl dir
    ssl_dir = server_root + "/ssl"
    FileUtils.mkdir_p ssl_dir
    # needs to be "secure"
    File.chmod(0700, ssl_dir)

    _, dump = IO.pipe
    # launch a p4d process to create a fingerprint file
    gc_pid = spawn({"P4SSLDIR" => ssl_dir}, %(#{@@P4D} -Gc), :out => dump);
    _, status = Process.waitpid2 gc_pid
    if status.exitstatus != 0
      puts "failed to create fingerprints: " + status.to_s
    end

    retries = 10
    @rand_port = 0

    while retries > 0
      # pick a random port from 1024 -> 65000
      @rand_port = "ssl::" + (rand(65000 - 1024) + 1024).to_s
      # launch a p4d to run trust against
      @pid = spawn({"P4SSLDIR" => ssl_dir}, %(#{@@P4D} #{p4d_params} -p #{@rand_port}), :out => dump)
      # sleep for a bit (hack)
      sleep(1)
      status = Process.wait @pid, Process::WNOHANG
      # nil indicates that the pid is still running
      if status.nil?
        # puts "started p4d #{@pid} on #{@rand_port}"
        break
      end
      puts "retry start p4d"
      retries -= 1
    end

    if retries == 0
      puts "Could not find available port, test will fail"
    end

    # also override the p4 to the ssl port-based p4d
    @p4 = P4.new
    @p4.port = @rand_port
    # don't connect, accepting the trust is part of the test
  end

  def teardown
    # send the shutdown signal
    # puts "Stopping #{@pid}"
    if windows_test?
      system("taskkill /f /pid #{@pid}")
    else
      Process.kill "SIGTERM", @pid
    end
    Process.wait @pid

    # last thing is to delete the files that the server is using
    super
  end

  def test_trust_file
    begin
      assert( p4, "Failed to create Perforce client" )

      # set a custom trust file
      trust_file = client_root + '/.p4trust-test'
      p4.trust_file = trust_file
      assert_equal(p4.trust_file, trust_file, 'Trust file not set correctly')

      # verify that we get a "you need to accept" when connecting
      begin
        begin 
          File.stat(trust_file)
          assert(false)
        rescue Exception => e
          # puts "correct exception: " + e.to_s
        end

        p4.connect
        # puts "connected"
        p4.run_info
        assert(false)
      rescue P4Exception => e
        # puts "exception" + e.to_s
      ensure
        p4.disconnect
      end

      # run trust
      begin
        p4.connect
        p4.run('trust', '-y')
        # verify that our file has the fingerprint?
        # see the file is not empty
        assert(File.stat(trust_file).size > 0)
        # final is to just run a command and see that it succeeds
        p4.run_info
      rescue
        # puts "exception" + e.to_s
        assert(false)
      ensure
        p4.disconnect
      end
    end
  end

  def test_default_trust
    begin
      assert( p4, "Failed to create Perforce client" )

      # verify that we get a "you need to accept" when connecting
      begin
        p4.connect
        # puts "connected"
        p4.run_info
        assert(false)
      rescue P4Exception => e
        # puts "exception" + e.to_s
      ensure
        p4.disconnect
      end

      # run trust
      begin
        p4.connect
        p4.run('trust', '-y')
        # verify that our file has the fingerprint
        # easier is to just run a command and see that it succeeds
        p4.run_info
      rescue P4Exception => e
        # puts "exception" + e.to_s
        assert(false)
      ensure
        p4.disconnect
      end
    end
  end
end
