#!/usr/bin/ruby
require "P4"

#*******************************************************************************
# Construct your client
# 
p4 = P4.new()

#*******************************************************************************
# Perforce client environment - getting the default settings
# 
# p4.client	- Get P4CLIENT 
# p4.host	- Get P4HOST
# p4.password	- Get P4PASSWD
# p4.port	- Get P4PORT
# p4.user	- Get P4USER
# 
print <<EOS

Perforce settings:

P4PORT    =	#{p4.port}
P4USER    =	#{p4.user}
P4CLIENT  =	#{p4.client}

EOS

#*******************************************************************************
# Perforce client environment - setting specific values
#
# Uncomment the settings below as required
# 
# p4.client = "tonys_client" 
# p4.host = "myhostname" 
# p4.password = "ruby" 
# p4.port = "localhost:1666" 
# p4.user = "tony" 
#
p4.port = "localhost:1666" 


#*******************************************************************************
# Connect to Perforce
begin
  p4.connect()
rescue P4Exception
  puts( "Failed to connect to Perforce" )
  raise
end

#*******************************************************************************
# Toggle tagged mode
p4.tagged = false
p4.tagged = true

#*******************************************************************************
# Running commands. All run* methods return an array. That can mean one line of
# output per array element, or in tagged mode, that can mean an array of hashes. 
#
# By default a P4Exception is raised if any errors or warnings are encountered 
# during command execution. You can also opt to have exceptions raised only
# for errors (and not warnings), or not at all by setting the exception level.
# The available levels are:
#
#     0 - Exceptions disabled
#     1 - Exceptions for errors
#     2 - Exceptions for errors and warnings
#
# For example:
#
#    p4.exception_level( 1 )
#
# You can fetch the results of the command from within a rescue block
# by calling P4#output; the errors with P4#errors and the warnings with
# P4#warnings
#
#*******************************************************************************

#*******************************************************************************
# "p4 user -o" produces an array with a single hash entry. 
# 

begin
  user_spec = p4.run( "user", "-o" ).shift

  print <<EOS

User details:

	User Name:	#{user_spec[ "User" ]}
	Full Name: 	#{user_spec[ "FullName" ]}
	Email Address:	#{user_spec[ "Email" ]}

EOS


#*******************************************************************************
# Now that we have the user's details, we can update them. Since this
# example is invasive, it's commented out by default.
#
#  user_spec[ "Email" ].upcase!
#  p4.input( user_spec )
#  p4.run( "user", "-i" )
#

#*******************************************************************************
# You can also run Perforce commands by invoking the method "run_<command>"
# rather than passing the command name as an argument to the run method. For
# example

  info 		= p4.run_info()
  user_spec 	= p4.run_user( "-o" ).shift
  user_list 	= p4.run_users()
  protections	= p4.run_protect( "-o" ).shift

#*******************************************************************************
# There are also shortcut methods to make form editing easy. Any method 
# taking the form "fetch_<command>" is equivalent to running "p4 <command> -o"
# and likewise any method taking the form "save_<command>" is equivalent to
# running "p4 <command> -i". These methods do not return an array - they 
# return only one element, since that's all that Perforce will return to you.
#
# Note that all of the "save*" methods require an argument. The argument
# can be either a string containing the edited form, or it can be the edited 
# hash returned from a previous  "fetch*" call.
#

  client_spec = p4.fetch_client()
  client_spec[ "Owner" ] = "tony"
# p4.save_client( client_spec )

rescue P4Exception => msg
  puts( msg )
  p4.warnings.each { |w| puts( w ) }
  p4.errors.each { |e| puts( e ) }
  p4.output.each { |o| puts( o ) }
end

