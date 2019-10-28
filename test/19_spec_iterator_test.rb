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

class TC_SpecIterator < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test spec iterator"
  end

  def test_spec_iteration
    assert( p4, "Failed to create Perforce client" )
    begin
      labels = Array.new
      assert( p4.connect, "Failed to connect to Perforce server" )

      label = p4.fetch_label("label1")
      assert_kind_of(P4::Spec, label, "Label1 is not of type P4::Spec")
      p4.save_label(label)
      labels.push(label._label)

      label = p4.fetch_label("label2")
      assert_kind_of(P4::Spec, label, "Label2 is not of type P4::Spec")
      p4.save_label(label)
      labels.push(label._label)

      p4.each_labels do
        |l|
        assert_kind_of( P4::Spec, l, "Label from iterator is not of type P4::Spec" )
        assert( labels.include?( l._label ), "Cannot find label '#{l['Label']}' in iteration")
      end

      #	Ensure that arguments passed to 'each_' are passed on and applied
      labels = p4.each_labels( "-e", "label1", "-m1" ) do
        |l|
        assert_equal( "label1", l._label, "Unexpected label in filtered result" )
      end
      assert_equal( 1, labels.size, "Unexpected number of labels" )

      p4.streams = true

      #	Ensure that the iterator does not raise an exception for all known specs
      #	(except Streams, which for some reason does generate a warning...)
      assert_nothing_raised do
        p4.each_clients { |e| }
        p4.each_branches { |e| }
        p4.each_changes { |e| }
        p4.each_jobs { |e| }
        p4.each_users { |e| }
        p4.each_groups { |e| }
        p4.each_depots { |e| }
        p4.each_servers { |e| }
      end

      assert_raise P4Exception do
        p4.each_streams { |e| }
        p4.each_nonexistent_specs { |e| }
      end
    ensure
      p4.disconnect
    end
  end
end
