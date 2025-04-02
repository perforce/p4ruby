# File: extconf.rb

$:.push File.expand_path("../../../lib", __FILE__)

require 'mkmf'
require 'P4/version'
require 'rbconfig'
require 'openssl'
require 'net/http'

# Set this to the main version directory we look up in ftp.perforce.com for the P4API
# This is ignored if you specify the version on the command line.
# Changed the hardcoded string so that the version is now derived from version.rb file
#P4API_VERSION_DIR = 'r19.1'
def p4api_version_dir
        ver=P4::Version.split(".")
        p4_major = ver[0].chars.last(2).join
        p4_minor = ver[1]
        dir = "r" + p4_major + "." + p4_minor
end


#==============================================================================
# Provide platform variables in P4-specific format

def p4osplat
  @p4osplat ||= calculate_p4osplat
end

def calculate_p4osplat
  plat = RbConfig::CONFIG['arch'].split(/-/)[0].upcase

  # On Mac OSX, fix the build to 64 bit arch
  if p4osname == 'DARWIN'
    plat = 'X86_64'
  end

  # Translate Ruby's arch names into Perforce's. Mostly the same so
  # only the exceptions are handled here.
  case plat
    when /^I.86$/
      plat = 'X86'
    when /^AMD64$/
      plat = 'X86_64'
    when 'POWERPC'
      plat = 'PPC'
  end

  return plat
end

def p4osname
  @p4osname ||= calculate_p4osname
end

def calculate_p4osname
  osname = RbConfig::CONFIG['arch'].split(/-/)[1].upcase
  osname = osname.gsub(/MSWIN32(_\d+)?/, "NT")
  osname = osname.split('-').shift

  case osname
    when /FREEBSD/
      osname = 'FREEBSD'
    when /DARWIN/
      osname = 'DARWIN'
    when /AIX/
      osname = 'AIX'
    when /SOLARIS/
      osname = 'SOLARIS'
  end

  return osname
end

def p4osver
  @p4osver ||= calculate_p4osver
end

def calculate_p4osver
  ver = ''

  case p4osname
    when 'NT'
      # do nothing
    when /MINGW/
      # do nothing
    when /FREEBSD([0-9]+)/
      ver = $1
    when /DARWIN/
      ver = CONFIG['arch'].upcase.gsub(/.*DARWIN(\d+).*/, '\1')
    when /AIX(5)\.(\d)/
      ver = $1 + $2
    when /SOLARIS2\.(\d+)/
      ver = $1
    else
      # use uname -r to see if it works
      begin
        ver=`uname -r`.chomp
        ver_re = /^(\d+)\.(\d+)/
        md = ver_re.match(ver)
        if (md)
          maj = md[1].to_i
          min = md[2].to_i
          ver = maj.to_s + min.to_s
        end
      rescue
        # Nothing - if it failed, it failed.
      end
  end

  return ver
end

def gcc
  @gcc ||= calculate_gcc
end

def calculate_gcc
  gcc = RbConfig::CONFIG["GCC"]
  return gcc
end

def uname_platform
  @uname_platform ||= calculate_uname_platform
end

def calculate_uname_platform
  plat = "UNKNOWN"
  begin
    plat = `uname -p`
    plat = plat.chomp.upcase
  rescue
    # Nothing - if it failed, it failed.
  end
  plat
end

#==============================================================================
# Setup additional compiler and linker options.
#
# We generally need to launch these things before we configure most of the flags.
# (See the main script at the end.)

def set_platform_opts

  # Expand any embedded variables (like '$(CC)')
  CONFIG["CC"] = RbConfig::CONFIG["CC"]
  CONFIG["LDSHARED"] = RbConfig::CONFIG["LDSHARED"]

  # Make sure we have a CXX value (sometimes there isn't one)
  CONFIG["CXX"] = CONFIG["CC"] unless CONFIG.has_key?("CXX")

  # O/S specific oddities

  case p4osname
    when /DARWIN/
      CONFIG['CC'] = 'xcrun c++'
      CONFIG['CXX'] = 'xcrun c++'
      CONFIG['LDSHARED'] = CONFIG['CXX'] + ' -bundle'
    when /FREEBSD/, /LINUX/
      # FreeBSD 6 and some Linuxes use 'cc' for linking by default. The
      # gcc detection patterns above won't catch that, so for these
      # platforms, we specifically convert cc to c++.
      CONFIG['LDSHARED'].sub!(/^cc/, 'c++')
    when /MINGW32/, /MINGW/
      # When building with MinGW we need to statically link libgcc
      # and make sure we're linking with gcc and not g++. On older
      # Rubies, they use LDSHARED; newer ones (>=1.9) use LDSHAREDXX
      CONFIG['LDSHARED'].sub!(/g\+\+/, 'gcc')
      CONFIG['LDSHAREDXX'].sub!(/g\+\+/, 'gcc')
      CONFIG['LDSHARED'] = CONFIG['LDSHARED'] + ' -static-libgcc'
      CONFIG['LDSHAREDXX'] = CONFIG['LDSHARED'] + ' -static-libgcc'
  end
end

def set_platform_cxxflags
  if (p4osname == 'LINUX') && (gcc == 'yes')
    $CXXFLAGS += " -std=c++11 "
  end
end


def set_platform_cppflags
  $CPPFLAGS += "-DOS_#{p4osname} "
  $CPPFLAGS += "-DOS_#{p4osname}#{p4osver} "
  $CPPFLAGS += "-DOS_#{p4osname}#{p4osver}#{p4osplat} "

  if (p4osname == 'NT')
    $CPPFLAGS += '/DCASE_INSENSITIVE '
  end

  if (p4osname == 'MINGW32')
    $CPPFLAGS += '-DOS_NT -DCASE_INSENSITIVE '
  end

  if (p4osname == 'MINGW')
    $CPPFLAGS += '-DOS_NT -DCASE_INSENSITIVE '
  end

  if (p4osname == 'SOLARIS')
    $CPPFLAGS += '-Dsolaris '
  end

  if (p4osname == 'DARWIN')
    $CPPFLAGS += '-DCASE_INSENSITIVE '
  end
end

def set_platform_cflags
  if (p4osname == 'DARWIN')
    # Only build for 64 bit if we have more than one arch defined in CFLAGS
    $CFLAGS.slice!("-arch i386")
    $CFLAGS.slice!("-arch ppc");
  end
end

# Initialize the base sets of platform libraries *before other initializers*
# to preserve linking order.
def set_platform_libs

  case p4osname
    when 'SOLARIS'
      osver = `uname -r`
      osver.gsub!(/5\./, '2')
      if (osver == '25')
        $LDFLAGS += '/usr/ucblib/libucb.a '
      end
      have_library('nsl')
      have_library('socket')
    when 'NT'
      have_library('advapi32')
      have_library('wsock32')
      have_library('kernel32')
      have_library('oldnames')
    when 'CYGWIN'
      # Clear out 'bogus' libs on cygwin
      CONFIG['LIBS'] = ''
    when 'DARWIN'
      if p4osver.to_i >= 8
        # Only build for 64 bit if we have more than one arch defined in CFLAGS
        $LDFLAGS.slice!('-arch i386')
        $LDFLAGS.slice!('-arch ppc')
        $LDFLAGS += ' -framework CoreFoundation -framework Foundation -framework CoreGraphics'
      end
    when 'LINUX', 'MINGW32', 'MINGW'
      $LDFLAGS += ' -Wl,--allow-multiple-definition'
      have_library('supc++')
  end
end

#==============================================================================
# Manage p4api version
#
# The p4ruby implementation has some branching to support different versions
# of the C++ API. So we need to generate a p4rubyconf.h file that will setup
# this #define based branching based on the C++ API being compiled against.

# This captures the version information of the P4API C++ library we're building
# against. This is mostly parsed into this structure and then spit out into
# a header file we compile into the Ruby API.
class P4ApiVersion

  def P4ApiVersion.load(dir)
    #
    # 2007.2 and later APIs put the Version file in the 'sample'
    # subdirectory. Look there if we can't find it in the API root
    #
    ver_file = dir + "/Version"
    unless File.exist?(ver_file)
      ver_file = dir + "/sample/Version"
      return nil unless File.exist?(ver_file)
    end

    re = Regexp.new('^RELEASE = (\d+)\s+(\d+)\s+(\w*\S*)\s*;')
    rp = Regexp.new('^PATCHLEVEL = (.*)\s*;')
    rs = Regexp.new('^SUPPDATE = (.*)\s*;')

    p4api_version = nil

    File.open(ver_file, "r") do
    |f|
      f.each_line do
      |line|
        if md = re.match(line)
          p4api_version = P4ApiVersion.new(md[1], md[2])
          p4api_version.set_type(md[3])
        elsif md = rp.match(line)
          p4api_version.patchlevel = md[1]
        elsif md = rs.match(line)
          p4api_version.suppdate = md[1]
        end
      end
    end
    puts("Found #{p4api_version} Perforce API in #{dir}")
    return p4api_version
  end

  def initialize(major, minor = nil)
    if (major.kind_of?(String) && !minor)
      if (major =~ /(\d+)\.(\d+)/)
        major = $1
        minor = $2
      else
        raise("Bad API version: #{major}")
      end
    end

    @major = major.to_i
    @minor = minor.to_i
    @type = nil

    @patchlevel = nil
    @suppdate = nil
  end

  def set_type(type)
    if (type.kind_of?(String))
      @type = type
    end
  end

  attr_accessor :patchlevel, :suppdate
  attr_reader :major, :minor, :type

  include Comparable

  def to_s
    if (@type and not @type.empty?)
      "#{major}.#{minor}.#{@type.upcase}"
    else
      "#{major}.#{minor}"
    end
  end

  def to_i
    major << 8 | minor
  end

  def <=>(other)
    hi = @major <=> other.major
    lo = @minor <=> other.minor

    return hi == 0 ? lo : hi
  end
end

def macro_def(macro, value, string=true)
  if (string)
    %Q{#define #{macro}\t"#{value}"}
  else
    %Q{#define #{macro}\t#{value}}
  end
end

def create_p4rubyconf_header(p4api_version, libs)
  File.open("p4rubyconf.h", "w") do
  |ch|
    ch.puts(macro_def("P4APIVER_STRING", p4api_version.to_s))
    ch.puts(macro_def("P4APIVER_ID", p4api_version.to_i, false))
    ch.puts(macro_def("P4API_PATCHLEVEL", p4api_version.patchlevel, false))
    ch.puts(macro_def("P4API_PATCHLEVEL_STRING", p4api_version.patchlevel.to_s))
    ch.puts(macro_def("P4RUBY_VERSION", P4::VERSION))
    ch.puts(macro_def("WITH_LIBS", libs, true))
  end
end

#==============================================================================
# P4API (C++ API) Helpers
#
# We do not have system installers yet, so allow most people to just get a
# version downloaded if since they very likely do not care about it.

# If the user has *not* specified --with-p4api-dir, check the --enable-p4api-download
# flag, and download the p4api before proceeding, unless that's disabled.
#
# This may be a little confusing. If people specify --with-p4api-dir, we want
# use only use that setting. If that setting is wrong, we want to fail.
#
# If they don't set the --with-p4api-dir, we'll proceed as if --enable-p4api-download
# has been set. Otherwise, they can --disable-p4api-download to ensure we
# just don't bother doing anything.
def resolve_p4api_dir
  p4api_dir = nil

  # When running rake compile, use this instead of other options, I'm not sure how
  # gem/bundler options are passed through via rake
  if ENV.has_key?('p4api_dir')
    p4api_dir = ENV['p4api_dir']
    dir_config('p4api', "#{p4api_dir}/include", "#{p4api_dir}/lib")
  end

  if !p4api_dir && !with_config('p4api-dir') && enable_config('p4api-download', true)
    download_api_via_https
    unzip_file
    p4api_dir = downloaded_p4api_dir
    dir_config('p4api', "#{p4api_dir}/include", "#{p4api_dir}/lib")
  elsif with_config('p4api_dir')
    p4api_dir = with_config('p4api-dir')
    dir_config('p4api', "#{p4api_dir}/include", "#{p4api_dir}/lib")
  elsif !p4api_dir
    raise '--with-p4api-dir option has not been specified, and --disable-p4api-download is in effect'
  end

  p4api_dir
end

def resolve_ssl_dirs
  ssl_dir = nil
  # When running rake compile, use this instead of other options, I'm not sure how
  # gem/bundler options are passed through via rake
  if ENV.has_key?('ssl_dir')
    ssl_dir = ENV['ssl_dir']
    dir_config('ssl', "#{ssl_dir}/include", "#{ssl_dir}/lib")
    puts "SSL Path #{ssl_dir}"
  end
  if ENV.has_key?('ssl_include_dir') && ENV.has_key?('ssl_lib_dir')
    ssl_include_dir = ENV['ssl_include_dir']
    ssl_lib_dir = ENV['ssl_lib_dir']
    dir_config('ssl', ssl_include_dir, ssl_lib_dir)
    puts "SSL Includes #{ssl_include_dir} Lib #{ssl_lib_dir}"
  end
  ssl_dir
end

# Our 'cpu' label we use as part of the directory name on ftp.perforce.com
def p4_cpu(os)
  cpu = RbConfig::CONFIG['target_cpu']
  case os
    when :darwin, :linux
      if cpu =~ /i686/
        'x86'
      elsif cpu =~ /universal/
        'x86_64'
      else
        cpu
      end
    else
      case cpu
        when /ia/i
          'ia64'
        else
          cpu
      end
  end
end

# The p4_platform is our label that basically ends up being part of the
# directory name where we can download files from.
def p4_platform_label
  case RbConfig::CONFIG["target_os"].downcase
    when /nt|mswin|mingw|cygwin|msys/
      # Ruby on windows is only MinGW via Rubyinstaller.org, though this may
      # not work on all rubies.
      # There are too many permutations of Windows p4api, to automate.
      raise 'Automatic fetching of p4api from perforce FTP is not supported on Windows'
    when /darwin19|darwin[2-9][0-9]/
      "macosx12#{p4_cpu(:darwin)}"
    when /darwin/      
      "darwin90#{p4_cpu(:darwin)}"
    when /solaris/
      "solaris10#{p4_cpu(:solaris)}"
    when /linux/
      "linux26#{p4_cpu(:linux)}"    
  end
end

def platform_dir_name
  "bin.#{p4_platform_label}"
end

def download_dir(version)
  "perforce/#{version}/#{platform_dir_name}"
end

def filename
  openssl_number = OpenSSL::OPENSSL_VERSION.split(' ')[1].to_s
  openssl_number = openssl_number.slice(0, (openssl_number.rindex('.')))

  if RbConfig::CONFIG['target_os'].downcase =~ /nt|mswin|mingw/
    filename = 'p4api.zip'
    if !openssl_number.to_s.empty?
        case openssl_number.to_s
            when /1.1/
                filename = 'p4api-openssl1.1.1.zip'
            when /1.0/
                filename = 'p4api-openssl1.0.2.zip'
            when /3.*/
                filename = 'p4api-openssl3.zip'
        end
    end
  elsif RbConfig::CONFIG['target_os'].downcase =~ /darwin19|darwin[2-9][0-9]/ || RbConfig::CONFIG['host_cpu'] =~ /aarch64|arm64/
    if !openssl_number.to_s.empty?
      case openssl_number.to_s
          when /1.1/
              filename = 'p4api-openssl1.1.1.tgz'
          when /3.*/
              filename = 'p4api-openssl3.tgz'
      end
    end
  else
    filename = 'p4api.tgz'
    if !openssl_number.to_s.empty?
        case openssl_number.to_s
            when /1.1/
                filename = 'p4api-glibc2.3-openssl1.1.1.tgz'
            when /1.0/
                filename = 'p4api-glibc2.3-openssl1.0.2.tgz'
            when /3.*/
                filename = 'p4api-glibc2.3-openssl3.tgz'
        end
    end
  end
  return filename
end

#############################################
# Downloads the C++ P4API via HTTPS to the local directory, then 'initializes' it
# by unpacking it.
def download_api_via_https

  uri=URI('https://ftp.perforce.com:443')
  dir = download_dir(p4api_version_dir)

  puts "Downloading #{filename} from #{dir} on https://ftp.perforce.com"

  Net::HTTP.start(uri.host, uri.port, :use_ssl =>true) do |http|
      resp = http.get("/" + dir + "/" + filename)
      open(filename, "wb") do |file|
          file.write(resp.body)
      end
  end
end

def unzip_file
  if RbConfig::CONFIG['target_os'].downcase =~ /nt|mswin|mingw/
    `unzip #{filename}`
  else
    `tar xzf #{filename}`
  end
end

def downloaded_p4api_dir
  File.expand_path(Dir.entries('.').select { |x| x =~ /^p4api/ and File.directory?(x) }.first)
end

#==============================================================================
# Main script

puts "p4osname #{p4osname}"
puts "p4osver #{p4osver}"

# Specify different toolsets based on the platform type.
set_platform_opts

# We setup these flags in the beginning, before any libraries are detected,
# based solely on platform detection.
set_platform_cppflags
set_platform_cflags
set_platform_cxxflags

puts "$CPPFLAGS #{$CPPFLAGS}"
puts "$CFLAGS #{$CFLAGS}"
puts "$CXXFLAGS #{$CXXFLAGS}"

# Setup additional system library definitions based on platform type before
# we setup other libraries, in order to preserve linking order
set_platform_libs

puts "$LDFLAGS #{$LDFLAGS}"

p4api_dir = resolve_p4api_dir
puts "P4API Path #{p4api_dir}"

resolve_ssl_dirs

# If we happen to need SSL on Windows, we also need gdi32
if RbConfig::CONFIG['target_os'].downcase =~ /mingw/
  have_library('gdi32') or raise
  have_library('ole32') or raise
  have_library('crypt32') or raise
end

have_library('crypto') or raise
have_library('ssl') or raise
have_library('supp') or raise
have_library('p4script_sqlite') or raise
have_library('p4script_curl') or raise
have_library('p4script') or raise
have_library('p4script_c') or raise
have_library('rpc') or raise
have_library('client') or raise

puts "$libs #{$libs}"

# Parse the Version file into a ruby structure
version_info = P4ApiVersion.load(p4api_dir)
create_p4rubyconf_header(version_info, $libs)

# This will generate a standard extconf.h based on what we discover locally.
# These are typically just 'yes I have such and such a library', which I
# don't believe we need to rely on actually.
create_header

create_makefile('P4')
