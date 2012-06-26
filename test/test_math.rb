require 'test_helper'

class TestMath < Test::Unit::TestCase

  def test_that_random_0_is_invalid
    run_test_as('programmer') do
      assert_equal E_INVARG, simplify(command(%Q|; return random(0); |))
    end
  end

  def test_that_random_1_returns_1
    run_test_as('programmer') do
      assert_equal 1, simplify(command(%Q|; return random(1); |))
    end
  end

  def test_that_random_returns_a_number_between_1_and_2147483647
    run_test_as('programmer') do
      1000.times do
        r = simplify command %Q|; return random(); |
        assert r > 0 && r <= 2147483647
      end
    end
  end

end
