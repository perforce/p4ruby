require 'rake/clean'

$:.push File.expand_path("../lib", __FILE__)
require 'P4/version'

task 'version' do
  puts P4::VERSION
end

GEM_SPEC = Gem::Specification.new { |t|
  t.name = 'p4ruby'
  t.version = P4::VERSION
  t.platform = Gem::Platform::RUBY
  t.summary = 'Ruby extensions to the C++ Perforce API'
  t.description = t.summary + '.'
  t.author = 'Perforce Software, Inc.'
  t.email = 'support@perforce.com'
  t.homepage = 'https://github.com/perforce/p4ruby'
  t.extensions = ['ext/P4/extconf.rb']
  t.licenses = ['MIT']

  t.files = %w( LICENSE.txt README.md )
  t.files += Dir.glob('ext/**/*.cpp')
  t.files += Dir.glob('ext/**/*.h')
  t.files += Dir.glob('lib/**/*.rb')
  t.files += Dir.glob('p4-bin/**/p4api.*')
  t.files += Dir.glob('lib/**/P4.so*')
  t.metadata = { "documentation_uri" => "https://www.perforce.com/manuals/p4ruby/Content/P4Ruby/Home-p4ruby.html" }
}

begin
  require 'rake/extensiontask'

  unless ENV['P4RUBY_CROSS_PLATFORM'].nil?
    # Set P4RUBY_CROSS_PLATFORM to x86-mingw32, x64-mingw32
    Rake::ExtensionTask.new('P4', GEM_SPEC) do |ext|
      ext.cross_compile = true
      ext.cross_platform = ENV['P4RUBY_CROSS_PLATFORM']
    end
  else
    Rake::ExtensionTask.new('P4', GEM_SPEC)
  end

  Gem::PackageTask.new(GEM_SPEC) do |pkg|
    pkg.need_tar = false
    pkg.need_zip = false
  end

rescue Exception
  #puts 'could not load rake/extensiontask'
end

require 'rake/testtask'

Rake::TestTask.new do |t|
  t.libs << '.'
  t.libs << 'test'
  t.warning = true
  t.verbose = true
  t.test_files = FileList[ 'test/testlib.rb', 'test/*_test.rb']
end

require 'rake/packagetask'

package_task = Rake::PackageTask.new('p4ruby', :noversion) do |p|
  p.need_tar = true
  p.need_zip = true
  p.package_files.include %w(
    Gemfile
    LICENSE.txt
    Rakefile
    README.md
  )
  p.package_files.include 'ext/**/*'
  p.package_files.include 'lib/**/*'
  p.package_files.include 'test/**/*'
end

# On some older platforms (Ruby 1.9) the 'directory' command doesn't work
# correctly.
begin
  desc 'Create doc directory from docbook files (requires ant)'
  directory 'doc' => 'docbook' do
    puts 'Executing docbook'
    sh 'cd docbook && ant publicsite -Ddoc.build.path=../p4-doc/manuals/_build && cd ..'
    doc_files = FileList['doc/**/*']
    package_task.package_files += doc_files
  end
  CLEAN.include('doc')

  desc 'Create build.properties, used to share version numbers in Jenkins tasks'
  file 'build.properties' do
    props = <<-END.gsub(/^ {4}/, '')
      P4RUBY_VERSION=#{P4::VERSION}
    END
    IO.write('build.properties', props)
  end

  # Remove output of 'rake compile' command
  CLEAN.include('lib/P4.bundle')
  CLEAN.include('lib/P4.so')

  CLEAN.include('pkg')

rescue Exception => e
  puts 'Not creating documentation rules used for builds, this is common in test environments'
end
