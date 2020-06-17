# vim:ts=2:sw=2:et:
#-------------------------------------------------------------------------------
# Copyright (c) 2001-2008, Perforce Software, Inc.  All rights reserved.
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

require 'fileutils.rb'
require 'rbconfig'
require 'rubygems'
require 'P4'
require 'getoptlong'
require 'test/unit'

# if RbConfig
#    MAJOR_VERSION = RbConfig::CONFIG[ 'MAJOR' ].to_i
#    MINOR_VERSION = RbConfig::CONFIG[ 'MINOR' ].to_i
#    if MAJOR_VERSION == 1
#	      require 'test/unit'
#	      if MINOR_VERSION <= 8
#	        require 'test/unit/ui/console/testrunner'
#	      end
#    elsif MAJOR_VERSION == 2
# 	      require 'minitest/unit'
# 	      require 'minitest/autorun'
#    end
# end

#
# Define some common methods and attributes that can be included
# by our Test::Unit::TestCase subclasses.
#
module P4RubyTest
  @@STARTDIR = Dir.getwd
  @@ROOTDIR = 'testroot'
  @@P4D = ENV['P4D_BIN'] || 'p4d'

  attr_accessor :p4

  #
  # Common setup for tests
  #
  def setup
    super
    # The P4CONFIG value is often set by developers, we shouldn't depend on it
    ENV.delete('P4CONFIG')
    ENV.delete('P4ENVIRO')
    ENV['P4ENVIRO'] = enviro_file
    create_workspace_tree
    Dir.chdir(client_root)
    ENV.delete('PWD')
    init_client
    #p4.debug = 1
  end

  #
  # Common cleanup
  #
  def retry_rm_rf(path)
    while (true)
      FileUtils.rm_rf(path)
      return if Dir.glob(path + "/*").empty?
      puts "retry rm -rf #{path}"
    end
  end

  def teardown
    @p4.disconnect if @p4.connected?
    sleep(0.5)
    @p4 = nil
    # Go back to where we started, or we can't remove the tree.
    Dir.chdir(@@STARTDIR)
    unless ENV['P4RUBY_TEST_NOCLEANUP']
      retry_rm_rf(server_root)
      retry_rm_rf(client_root)
      File.unlink(enviro_file) if File.exist?(enviro_file)
      begin
        Dir.rmdir(test_root) if File.directory?(test_root)
        Dir.rmdir(@@ROOTDIR) if File.directory?(@@ROOTDIR)
      rescue Errno::ENOTEMPTY
      end
    end
    true
  end

  #
  # Set up the client workspace for the test
  #
  def create_workspace_tree
    init_client
    FileUtils.mkdir_p(server_root) unless File.directory?(server_root)
    FileUtils.mkdir_p(client_root) unless File.directory?(client_root)
    create_p4config_file
    create_enviro_file
    true
  end

  def p4d_params
    return %(#{log_param} -r #{server_root} -C1 -J off) # -vserver=3 -vrpc=5)
  end

  def init_client
    # Create a P4 object for a test
    @p4 = P4.new
    @p4.charset = nil # Disable auto-detection?
    # If P4RUBY_TEST_PORT is defined, use that instead of rsh
    if ENV.key?('P4RUBY_TEST_PORT')
      @p4.port = ENV['P4RUBY_TEST_PORT']
      puts "using test port: #{@p4.port}"
    else
      @p4.port = %(rsh:#{@@P4D} #{p4d_params} -i)
    end
    @p4.client = name.downcase.gsub(' ', '-')
  end

  # Create the Perforce client for a test
  def create_client
    @p4.connect unless @p4.connected?
    spec = @p4.fetch_client
    spec._root = client_root
    @p4.save_client(spec)
  end

  #
  # Return the path to this test's root
  # Note: for the trust test we have to launch p4d and later stop it
  #       and spaces in the path make spawn do an sh p4d -> p4d which
  #       makes it hard to kill the child process
  #
  def test_root
    [@@STARTDIR, @@ROOTDIR, name.gsub(' ','_')].join('/')
  end

  def enviro_file
    File.expand_path(File.join(test_root, 'p4enviro'))
  end

  #
  # Return the path of the server root
  #
  def server_root
    File.expand_path('server', test_root)
  end

  def windows_test?
    ['Windows_NT','MINGW'].include? ENV['OS']
  end

  #
  # Return the path of the server log param
  #
  def log_param
    return nil unless windows_test?
    # for windows, put a log file in the test_root
    return %(-L "#{File.expand_path('p4d.log', test_root)}")
  end

  #
  # Return the client root
  #
  def client_root
    [test_root, 'workspace'].join('/')
  end

  def enable_unicode
    # On windows, using the -r command to do an upgrade just seems to return
    # an error of 'unexpected arguments'
    #cmd = "#{@@P4D} -r '#{server_root}' -C1 -L log -vserver=3 -xi"
    start_dir = Dir.pwd
    Dir.chdir(server_root)
    cmd = "#{@@P4D} -C1 -L log -vserver=3 -xi"
    # Using IO.popen stops the output from polluting the test
    # output.
    IO.popen(cmd) { |p| p.read }
    #system(cmd)
    Dir.chdir(start_dir)
  end

  #
  # Common method for adding some test files to work with
  #
  def add_sample_content
    Dir.mkdir('test_files')
    %w(foo bar baz).each do
    |fn|
      fn = "test_files/#{fn}.txt"
      File.open(fn, 'w') do
      |f|
        f.puts('This is a test file')
      end
      p4.run_add(fn)
    end
    change = p4.fetch_change
    change._description = 'Test files'
    p4.run_submit(change)
    true
  end

  # Note: when security level changes, it's likely that nothing will happen
  # until you change password
  def configure_security(level)
    results = p4.run_configure('set', "security=#{level}")
    puts "results #{results}"
    true
  end

  private

  def create_p4config_file
    return unless ENV.key?('P4CONFIG')
    File.open(client_root + '/' + ENV['P4CONFIG'], 'w') do
    |f|
      f.puts("P4PORT=#{@p4.port}")
    end
  end

  def create_enviro_file
    return unless ENV.key?('P4ENVIRO')
    File.open(ENV['P4ENVIRO'], 'w+') do |_f|
    end
  end

end
