require 'test_helper'

class TestStringOperations < Test::Unit::TestCase

  def test_that_index_finds_position_of_substring_in_a_string
    run_test_as('programmer') do
      assert_equal 0, index('foobar', 'x')
      assert_equal 2, index('foobar', 'o')
    end
  end

  def test_that_index_finds_position_of_substring_in_a_string_with_case_matters
    run_test_as('programmer') do
      assert_equal 0, index('foobar', 'O', 1)
      assert_equal 2, index('foobar', 'O', 0)
    end
  end

  def test_that_index_finds_position_of_substring_in_a_string_with_offset
    run_test_as('programmer') do
      assert_equal 0, index('foobar', 'o', 0, 3)
      assert_equal 1, index('foobar', 'o', 0, 1)
      assert_equal 2, index('foobar', 'o', 0, 0)
    end
  end

  def test_that_index_offset_cannot_be_negative
    run_test_as('programmer') do
      assert_equal E_INVARG, index('foobar', 'o', 0, -1)
    end
  end

  def test_that_index_offset_can_be_larger_than_the_source_string_length
    run_test_as('programmer') do
      assert_equal 0, index('foobar', 'o', 0, 10)
    end
  end

  def test_that_rindex_finds_position_of_substring_in_a_string
    run_test_as('programmer') do
      assert_equal 0, rindex('foobar', 'x')
      assert_equal 3, rindex('foobar', 'o')
    end
  end

  def test_that_rindex_finds_position_of_substring_in_a_string_with_case_matters
    run_test_as('programmer') do
      assert_equal 0, rindex('foobar', 'O', 1)
      assert_equal 3, rindex('foobar', 'O', 0)
    end
  end

  def test_that_rindex_finds_position_of_substring_in_a_string_with_offset
    run_test_as('programmer') do
      assert_equal 2, rindex('foobar', 'o', 0, -4)
      assert_equal 3, rindex('foobar', 'o', 0, -3)
      assert_equal 3, rindex('foobar', 'o', 0, 0)
    end
  end

  def test_that_rindex_offset_cannot_be_positive
    run_test_as('programmer') do
      assert_equal E_INVARG, rindex('foobar', 'o', 0, 1)
    end
  end

  def test_that_rindex_offset_can_be_larger_than_the_source_string_length
    run_test_as('programmer') do
      assert_equal 0, rindex('foobar', 'o', 0, -10)
    end
  end

end
