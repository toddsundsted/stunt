require 'test_helper'

# Test various in-MOO limits on values.

class TestLimits < Test::Unit::TestCase

  def setup
    run_test_as('wizard') do
      evaluate('add_property($server_options, "max_concat_catchable", 1, {player, "r"})')
      evaluate('add_property($server_options, "max_list_concat", 1100, {player, "r"})')
      evaluate('add_property($server_options, "max_map_concat", 1100, {player, "r"})')
      evaluate('add_property($server_options, "max_string_concat", 1100, {player, "r"})')
      evaluate('add_property($server_options, "fg_seconds", 123, {player, "r"})')
      evaluate('add_property($server_options, "fg_ticks", 2147483647, {player, "r"})')
      evaluate('load_server_options();')
    end
  end

  def teardown
    run_test_as('wizard') do
      evaluate('delete_property($server_options, "max_concat_catchable")')
      evaluate('delete_property($server_options, "max_list_concat")')
      evaluate('delete_property($server_options, "max_map_concat")')
      evaluate('delete_property($server_options, "max_string_concat")')
      evaluate('delete_property($server_options, "fg_seconds")')
      evaluate('delete_property($server_options, "fg_ticks")')
      evaluate('load_server_options();')
    end
  end

  def test_that_list_limits_work
    run_test_as('programmer') do
      assert_equal 1100, simplify(command('; x = {}; for i in [1..1100]; x = {i, @x}; endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = {}; for i in [1..1101]; x = {i, @x}; endfor; return length(x);'))
      assert_equal 1100, simplify(command('; x = {}; for i in [1..1100]; x[1..0] = {i}; endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = {}; for i in [1..1101]; x[1..0] = {i}; endfor; return length(x);'))
      assert_equal 1100, simplify(command('; x = {}; for i in [1..1100]; x = listappend(x, i); endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = {}; for i in [1..1101]; x = listappend(x, i); endfor; return length(x);'))
      assert_equal 1100, simplify(command('; x = {}; for i in [1..1100]; x = listinsert(x, i); endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = {}; for i in [1..1101]; x = listinsert(x, i); endfor; return length(x);'))
      assert_equal 1100, simplify(command('; x = {}; for i in [1..1100]; x = setadd(x, i); endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = {}; for i in [1..1101]; x = setadd(x, i); endfor; return length(x);'))
    end
  end

  def test_that_map_limits_work
    run_test_as('programmer') do
      assert_equal 1100, simplify(command('; x = []; for i in [1..1100]; x[i] = i; endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = []; for i in [1..1101]; x[i] = i; endfor; return length(x);'))
      assert_equal 1100, simplify(command('; x = ["a" -> "a", "b" -> "b"]; for i in [1..1098]; x["b".."a"] = [i -> i]; endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = ["a" -> "a", "b" -> "b"]; for i in [1..1099]; x["b".."a"] = [i -> i]; endfor; return length(x);'))
    end
  end

  def test_that_string_limits_work
    run_test_as('programmer') do
      assert_equal 1100, simplify(command('; x = ""; for i in [1..1100]; x = x + " "; endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = ""; for i in [1..1101]; x = x + " "; endfor; return length(x);'))
      assert_equal 1100, simplify(command('; x = ""; for i in [1..1100]; x = tostr(" ", x); endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = ""; for i in [1..1101]; x = tostr(" ", x); endfor; return length(x);'))
      assert_equal 1022, simplify(command('; x = ""; for i in [1..9]; x = toliteral(x); endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = ""; for i in [1..10]; x = toliteral(x); endfor; return length(x);'))
      assert_equal 1024, simplify(command('; x = " "; for i in [1..10]; x = strsub(x, " ", "  "); endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = " "; for i in [1..11]; x = strsub(x, " ", "  "); endfor; return length(x);'))
      assert_equal 1099, simplify(command('; x = "~"; for i in [1..549]; x = encode_binary(x); endfor; return length(x);'))
      assert_equal E_QUOTA, simplify(command('; x = "~"; for i in [1..550]; x = encode_binary(x); endfor; return length(x);'))
    end
  end

end
