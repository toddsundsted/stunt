require 'test_helper'

class TestExec < Test::Unit::TestCase

  def test_that_exec_fails_for_non_wizards
    run_test_as('programmer') do
      assert_equal E_PERM, exec(['foobar'])
    end
  end

  def test_that_exec_fails_for_nonexistent_executables
    run_test_as('wizard') do
      r = exec(['test_does_not_exist'])
      assert_equal E_INVARG, r
    end
  end

  def test_that_exec_fails_for_bad_paths
    run_test_as('wizard') do
      r = exec(['/test_io'])
      assert_equal E_INVARG, r
      r = exec(['./test_io'])
      assert_equal E_INVARG, r
      r = exec(['../test_io'])
      assert_equal E_INVARG, r
      r = exec(['foo/../../test_io'])
      assert_equal E_INVARG, r
    end
  end

  def test_that_io_works
    run_test_as('wizard') do
      r = exec(['test_io'], 'Hello, world!')
      assert_equal [0, 'Hello, world!', 'Hello, world!'], r
    end
  end

  def test_that_args_works
    run_test_as('wizard') do
      r = exec(['test_args', 'one', 'two', 'three'], '')
      assert_equal [0, 'one two three', ''], r
    end
  end

  def test_that_exit_status_works
    run_test_as('wizard') do
      r = exec(['test_exit_status', '2'], '')
      assert_equal [2, '', ''], r
    end
  end

  def test_that_executables_that_take_a_long_time_work
    run_test_as('wizard') do
      r = exec(['test_with_sleep'], '')
      assert_equal [0, '1~0A2~0A3~0A', ''], r
    end
  end

  def test_that_stdin_takes_the_right_kind_of_binary_string
    run_test_as('wizard') do
      r = exec(['test_io'], '1~ZZ23~0A')
      assert_equal E_INVARG, r
    end
  end

end
