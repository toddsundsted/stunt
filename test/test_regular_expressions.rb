require 'test_helper'

class TestRegularExpressions < Test::Unit::TestCase

  def test_that_match_and_rmatch_take_two_or_three_arguments
    run_test_as('programmer') do
      assert_equal E_ARGS, simplify(command(%Q|; match(); |))
      assert_equal E_ARGS, simplify(command(%Q|; match(1); |))
      assert_equal E_ARGS, simplify(command(%Q|; match(1, 2, 3, 4); |))
      assert_equal E_ARGS, simplify(command(%Q|; rmatch(); |))
      assert_equal E_ARGS, simplify(command(%Q|; rmatch(1); |))
      assert_equal E_ARGS, simplify(command(%Q|; rmatch(1, 2, 3, 4); |))
    end
  end

  def test_that_an_invalid_argument_to_match_and_rmatch_raises_an_error
    run_test_as('programmer') do
      assert_equal E_TYPE, simplify(command(%Q|; match(1, 2); |))
      assert_equal E_TYPE, simplify(command(%Q|; rmatch(1, 2); |))
      assert_equal E_INVARG, simplify(command(%Q|; match("", "%("); |))
      assert_equal E_INVARG, simplify(command(%Q|; rmatch("", "%("); |))
    end
  end

  def test_that_a_failed_match_returns_the_empty_list
    run_test_as('programmer') do
      assert_equal [], match('this is a is a test', 'was a')
      assert_equal [], rmatch('this is a is a test', 'was a')
    end
  end

  def test_that_a_successful_match_returns_details_about_the_match
    run_test_as('programmer') do
      assert_equal [6, 9, [[0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1]], 'this is a is a test'], match('this is a is a test', 'is a')
      assert_equal [11, 14, [[0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1]], 'this is a is a test'], rmatch('this is a is a test', 'is a')
    end
  end

  def test_that_a_successful_match_returns_grouping_details
    run_test_as('programmer') do
      assert_equal [1, 4, [[1, 4], [1, 2], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1]], 'abracadabra'], match('abracadabra', '%(%([a-d]+%)ra%)')
      assert_equal [9, 11, [[9, 11], [9, 9], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1], [0, -1]], 'abracadabra'], rmatch('abracadabra', '%(%([a-d]+%)ra%)')
    end
  end

  def test_that_substitute_works_with_match
    run_test_as('programmer') do
      subs = match('*** Welcome to LambdaMOO!!!', '%(%w*%) to %(%w*%)')
      assert_equal 'I thank you for your Welcome here in LambdaMOO.', substitute('I thank you for your %1 here in %2.', subs)
    end
  end

end
