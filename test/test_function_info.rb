require 'test_helper'

class TestFunctionInfo < Test::Unit::TestCase

  def test_that_function_info_takes_at_most_one_argument
    run_test_as('programmer') do
      assert_equal E_ARGS, simplify(command(%Q|; function_info(1, 2); |))
    end
  end

  def test_that_an_invalid_argument_to_function_info_raises_an_error
    run_test_as('programmer') do
      assert_equal E_TYPE, simplify(command(%Q|; function_info(1); |))
      assert_equal E_TYPE, simplify(command(%Q|; function_info(1.0); |))
      assert_equal E_INVARG, simplify(command(%Q|; function_info("foobar"); |))
    end
  end

  def test_that_function_info_without_arguments_returns_information_on_all_functions
    run_test_as('programmer') do
      assert_equal [], function_info().reject { |f| f[0] =~ /^[a-z0-9_]+$/ && f[1].is_a?(Fixnum) && f[2].is_a?(Fixnum) && f[3].is_a?(Array) }
    end
  end

  def test_that_function_info_with_an_argument_returns_information
    run_test_as('programmer') do
      assert_equal ['function_info', 0, 1, [2]], function_info('function_info')
    end
  end

end
