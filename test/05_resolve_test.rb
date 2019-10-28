# vim:ts=2:sw=2:et:
#-------------------------------------------------------------------------------
# Copyright (c) 1997-2007, Perforce Software, Inc.	All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1.  Redistributions of source code must retain the above copyright
#	  notice, this list of conditions and the following disclaimer.
#
# 2.  Redistributions in binary form must reproduce the above copyright
#	  notice, this list of conditions and the following disclaimer in the
#	  documentation and/or other materials provided with the distribution.
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

class TC_Resolve < Test::Unit::TestCase

  include P4RubyTest

  def name
    "Test resolve operations"
  end

  def test_resolve
    assert(p4, "Failed to create Perforce client")

    begin
      test_dir = "test_resolve"
      Dir.mkdir(test_dir)
      file = "foo"
      bar = "bar"
      fname = File.join(test_dir, file)
      bname = File.join(test_dir, bar)

      assert(p4.connect, "Failed to connect to Perforce server")
      assert(create_client, "Failed to create test workspace")

      # Create the file to test resolve
      File.open(fname, 'w') { |fd| fd.puts("First Line!") }
      p4.run_add(fname)
      assert_equal(1, p4.run_opened.length, "Unexpected number of open files")

      change = p4.fetch_change
      assert_kind_of(P4::Spec, change, "Change form is not a P4::Spec")
      change._description = "First resolve submit"
      assert_equal("First resolve submit", change._description,
                   "Change description not set properly")
      assert_submit("Failed to add file", change)
      assert_equal(0, p4.run_opened.length, "Unexpected number of open files")

      # Create a second revision of the file
      p4.run_edit(fname)
      File.open(fname, "a") { |fd| fd.puts("Second Line.") }
      assert_equal(1, p4.run_opened.length, "Unexpected number of open files")

      change = p4.fetch_change
      assert_kind_of(P4::Spec, change, "Change form is not a P4::Spec")
      change._description = "Second resolve submit"
      assert_equal("Second resolve submit", change._description,
                   "Change description not set properly")
      assert_submit("Failed to add file", change)
      assert_equal(0, p4.run_opened.length, "Unexpected number of open files")

      assert_nothing_raised("Problem scheduling resolve for {#fname}") do
        #	Now sync to rev #1
        p4.run_sync(fname + "#1")

        #	open the file for edit and sync to schedule a resolve
        p4.run_edit(fname)
        p4.run_sync(fname)
      end

      # ...and test a standard resolve
      p4.run_resolve do |md|
        client = p4.client
        assert(md.class == P4::MergeData, "Merge data wasn't a P4::MergeData object")
        assert_equal("//#{client}/#{fname}", md.your_name, "Unexpected Your name: #{md.your_name}")
        assert_equal("//depot/#{fname}#2", md.their_name, "Unexpected Their name: #{md.their_name}")
        assert_equal("//depot/#{fname}#1", md.base_name, "Unexpected Base name: #{md.base_name}")
        assert_equal("at", md.merge_hint, "Unexpected merge hint: #{md.merge_hint}")
        "ay"
      end
      change = p4.fetch_change
      assert_kind_of(P4::Spec, change, "Change form is not a P4::Spec")
      change._description = "Third resolve submit"
      assert_equal("Third resolve submit", change._description,
                   "Change description not set properly")
      assert_submit("Failed to add file", change)
      assert_equal(0, p4.run_opened.length, 'Unexpected number of open files')

      #	Test to check that exceptions cause the resolve to stop at
      #	the point that the exception happened.
      assert_nothing_raised("Problem scheduling resolve for #{fname}") do
        p4.run_sync(fname + "#1")
        p4.run_edit(fname)
        p4.run_sync(fname + "#2")
        p4.run_edit(fname)
        p4.run_sync(fname)
      end

      resolve_output = p4.run_resolve('-n')

      assert_equal(2, resolve_output.length, "Unexpected number of resolves scheduled")
      assert_raise(RuntimeError) do
        count = 0
        p4.run_resolve do
        |md|
          raise RuntimeError, "Force an exception during a resolve." unless count > 0
          count += 1
          "at"
        end
      end
      assert_equal(2, p4.run_resolve("-n").length, "Unexpected number of resolves scheduled")
      p4.run_revert('//...')

      #
      # ACTION RESOLVE TEST
      #

      if (p4.server_level >= 31)

        # Schedule a branch resolve
        assert_nothing_raised("Problem scheduling branch resolve from 'foo' to 'bar'") do
          p4.run_integ("-Rb", '//depot/test_resolve/foo', '//depot/test_resolve/bar')
        end

        assert_equal(1, p4.run_resolve("-n").length, "Unexpected number of resolves scheduled")
        p4.run_resolve do
        |md|
          if (md.action_resolve?)
            assert_kind_of(P4::MergeData, md, "Merge data wasn't a P4::MergeData object")
            assert_nil(md.your_name, "Unexpected Your name: #{md.your_name}")
            assert_nil(md.their_name, "Unexpected Their name: #{md.their_name}")
            assert_nil(md.base_name, "Unexpected Base name: #{md.base_name}")

            info = md.info.shift
            assert_equal("//depot/test_resolve/foo", info['fromFile'], "Unexpected fromFile in info: #{info['fromFile']}")
            assert_equal("branch", info['resolveType'], "Unexpected resolveType in info: #{info['resolveType']}")

            assert_equal("ignore", md.yours_action, "Unexpected yours_action: #{md.yours_action}")
            assert_equal("branch", md.their_action, "Unexpected their_action: #{md.their_action}")
            assert_nil(md.merge_action, "Unexpected merge_action: #{md.merge_action}")
            assert_equal("Branch resolve", md.action_type, "Unexpected type: #{md.action_type}")
            assert_equal("at", md.merge_hint, "Unexpected merge_hint: #{md.merge_hint}")
            md.merge_hint
          elsif (md.content_resolve?)
            assert(false, "Unexpected content resolve scheduled")
          else
            assert(false, "Unknown resolve type scheduled");
          end
        end

        change = p4.fetch_change
        assert_kind_of(P4::Spec, change, "Change form is not a P4::Spec")
        change._description = "Fourth resolve submit"
        assert_equal("Fourth resolve submit", change._description, "Change description not set properly")
        assert_submit("Failed to add file", change)
        assert_equal(0, p4.run_opened.length, "Unexpected number of open files")

        # Schedule a content and filetype resolve
        p4.run_edit("-t+x", fname)
        File.open(fname, "a") { |fd| fd.puts("Third Line.") }
        p4.run_edit("-t+w", bname)
        assert_equal(2, p4.run_opened.length, "Unexpected number of open files")

        change = p4.fetch_change
        assert_kind_of(P4::Spec, change, "Change form is not a P4::Spec")
        change._description = "Fifth resolve submit"
        assert_equal("Fifth resolve submit", change._description, "Change description not set properly")
        assert_submit("Failed to add file", change)
        assert_equal(0, p4.run_opened.length, "Unexpected number of open files")

        assert_nothing_raised("Problem scheduling branching resolve from 'foo' to 'bar'") do
          source = "//depot/" + fname
          target = "//depot/" + bname
          #	Force integration with 3rd generation integration engine
          p4.run_integ("-3", source, target)
        end

        assert_equal(2, p4.run_resolve("-n").length, "Unexpected number of resolves scheduled")
        p4.run_resolve do
        |md|
          if (md.action_resolve?)
            assert_kind_of(P4::MergeData, md, "Merge data wasn't a P4::MergeData object")
            assert_nil(md.your_name, "Unexpected Your name: #{md.your_name}")
            assert_nil(md.their_name, "Unexpected Their name: #{md.their_name}")
            assert_nil(md.base_name, "Unexpected Base name: #{md.base_name}")

            info = md.info.shift
            assert_equal("//depot/test_resolve/foo", info['fromFile'], "Unexpected fromFile in info: #{info['fromFile']}")
            assert_equal("filetype", info['resolveType'], "Unexpected resolveType in info: #{info['resolveType']}")

            assert_equal("(text+w)", md.yours_action, "Unexpected yours_action: #{md.yours_action}")
            assert_equal("(text+x)", md.their_action, "Unexpected their_action: #{md.their_action}")
            assert_equal("(text+wx)", md.merge_action, "Unexpected merge_action: #{md.merge_action}")
            assert_equal("Filetype resolve", md.action_type, "Unexpected type: #{md.action_type}")
            assert_equal("am", md.merge_hint, "Unexpected merge_hint: #{md.merge_hint}")
            md.merge_hint
          elsif (md.content_resolve?)
            client = p4.client
            assert_kind_of(P4::MergeData, md, "Merge data wasn't a P4::MergeData object")
            assert_equal("//#{client}/#{bname}", md.your_name, "Unexpected Your name: #{md.your_name}")
            assert_equal("//depot/#{fname}#4", md.their_name, "Unexpected Their name: #{md.their_name}")
            assert_equal("//depot/#{fname}#3", md.base_name, "Unexpected Base name: #{md.base_name}")
            assert_equal("at", md.merge_hint, "Unexpected merge_hint: #{md.merge_hint}")

            assert_kind_of(Array, md.info, "Resolve information wasn't an array.")
            assert_equal(2, md.info.length, "Unexpected resolve information: #{md.info.inspect}")
            assert_nil(md.yours_action, "Unexpected yours_action: #{md.yours_action}")
            assert_nil(md.their_action, "Unexpected their_action: #{md.their_action}")
            assert_nil(md.merge_action, "Unexpected merge_action: #{md.merge_action}")
            assert_nil(md.action_type, "Unexpected type: #{md.action_type}")
            md.merge_hint
          else
            assert(false, "Unknown resolve type scheduled");
          end
        end
        change = p4.fetch_change
        assert_kind_of(P4::Spec, change, "Change form is not a P4::Spec")
        change._description = "Sixth resolve submit"
        assert_equal("Sixth resolve submit", change._description, "Change description not set properly")
        assert_submit("Failed to add file", change)
        assert_equal(0, p4.run_opened.length, "Unexpected number of open files")
      end
      #
      #	Test binary resolves
      #
      srcbin = "src.bin"
      tgtbin = "tgt.bin"
      srcname = File.join(test_dir, srcbin)
      tgtname = File.join(test_dir, tgtbin)

      File.open(srcname, 'w') { |fd| fd.puts("First line in binary file!") }
      p4.run_add("-t", "binary", srcname)

      change = p4.fetch_change
      assert_kind_of(P4::Spec, change, "Change form is not a P4::Spec")
      change._description = "First binary resolve submit"
      assert_equal("First binary resolve submit", change._description, "Change description not set properly")
      assert_submit("Failed to add file", change)
      assert_equal(0, p4.run_opened.length, "Unexpected number of open files")

      # Branch
      assert_nothing_raised("Problem branching from 'src.bin' to 'tgt.bin'") do
        p4.run_integ('//depot/test_resolve/src.bin', '//depot/test_resolve/tgt.bin')
      end

      change = p4.fetch_change
      assert(change.kind_of?(P4::Spec), "Change form is not a P4::Spec")
      change._description = "Second binary resolve submit"
      assert(change._description == "Second binary resolve submit", "Change description not set properly")
      assert_submit("Failed to add file", change)
      assert_equal(0, p4.run_opened.length, "Unexpected number of open files")

      #	Edit
      p4.run_edit("-t", "+x", srcname)
      File.open(srcname, 'a') { |fd| fd.puts("Second line in binary file!") }
      assert_equal(1, p4.run_opened.length, "Unexpected number of open files")

      change = p4.fetch_change
      assert_kind_of(P4::Spec, change, "Change form is not a P4::Spec")
      change._description = "Third binary resolve submit"
      assert_equal("Third binary resolve submit", change._description, "Change description not set properly")
      assert_submit("Failed to add file", change)
      assert_equal(0, p4.run_opened.length, "Unexpected number of open files")

      #	Integrate
      assert_nothing_raised("Problem integrating from 'src.bin' to 'tgt.bin'") do
        if (p4.server_level >= 31)
          p4.run_integ('-3', '//depot/test_resolve/src.bin', '//depot/test_resolve/tgt.bin')
        else
          p4.run_integ('//depot/test_resolve/src.bin', '//depot/test_resolve/tgt.bin')
        end
      end
      assert_equal(1, p4.run_opened.length, "Unexpected number of open files")
      if (p4.server_level >= 31)
        assert_equal(2, p4.run_resolve("-n").length, "Unexpected number of resolves scheduled")
      else
        assert_equal(1, p4.run_resolve("-n").length, "Unexpected number of resolves scheduled")
      end
      p4.run_resolve do
      |md|
        if (md.action_resolve?)
          assert_kind_of(P4::MergeData, md, "Merge data wasn't a P4::MergeData object")
          assert_nil(md.your_name, "Unexpected Your name: #{md.your_name}")
          assert_nil(md.their_name, "Unexpected Their name: #{md.their_name}")
          assert_nil(md.base_name, "Unexpected Base name: #{md.base_name}")

          info = md.info.shift
          assert_equal("//depot/test_resolve/src.bin", info['fromFile'], "Unexpected fromFile in info: #{info['fromFile']}")
          assert_equal("filetype", info['resolveType'], "Unexpected resolveType in info: #{info['resolveType']}")

          assert_equal("(binary)", md.yours_action, "Unexpected yours_action: #{md.yours_action}")
          assert_equal("(binary+x)", md.their_action, "Unexpected their_action: #{md.their_action}")
          assert_nil(md.merge_action, "Unexpected merge_action: #{md.merge_action}")
          assert_equal("Filetype resolve", md.action_type, "Unexpected type: #{md.action_type}")
          assert_equal("at", md.merge_hint, "Unexpected merge_hint: #{md.merge_hint}")
          md.merge_hint
        elsif (md.content_resolve?)
          assert_kind_of(P4::MergeData, md, "Merge data wasn't a P4::MergeData object")
          assert_nil(md.base_name, "Unexpected Base name: #{md.base_name}")
          assert_nil(md.your_name, "Unexpected Your name: #{md.your_name}")
          assert_nil(md.their_name, "Unexpected Their name: #{md.their_name}")
          assert_nil(md.base_path, "Unexpected base_path: #{md.base_path}")
          assert_equal(File.expand_path(tgtname), md.your_path.tr("\\", "/"), "Unexpected your_path: #{md.your_path}")
          assert(md.their_path, "their_path not set correctly")
          assert_equal("at", md.merge_hint, "Unexpected merge_hint: #{md.merge_hint}")

          assert_kind_of(Array, md.info, "Resolve information wasn't an array.")
          assert_equal(2, md.info.length, "Unexpected resolve information: #{md.info.inspect}")
          assert_nil(md.yours_action, "Unexpected yours_action: #{md.yours_action}")
          assert_nil(md.their_action, "Unexpected their_action: #{md.their_action}")
          assert_nil(md.merge_action, "Unexpected merge_action: #{md.merge_action}")
          assert_nil(md.action_type, "Unexpected type: #{md.action_type}")
          md.merge_hint
        else
          assert(false, "Unknown resolve type scheduled");
        end
      end
    ensure
      p4.run_revert('//...') unless (p4.run_opened.empty?)
      p4.disconnect
    end
  end

  #
  # Local method to help ensure submits are working
  #
  def assert_submit(msg, *args)
    assert_block(msg) do
      begin
        result = @p4.run_submit(args)
        if (result[-1].has_key?('submittedChange'))
          true
        else
          false
        end
      rescue P4Exception
        false
      end
    end
  end

end
