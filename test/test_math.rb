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

  def test_that_division_by_zero_fails
    run_test_as('programmer') do
      assert_equal E_DIV, simplify(command(%Q|; 1.1 / 0.0; |))
      assert_equal E_DIV, simplify(command(%Q|; 1 / 0; |))
      assert_equal E_DIV, simplify(command(%Q|; 1.1 % 0.0; |))
      assert_equal E_DIV, simplify(command(%Q|; 1 % 0; |))
    end
  end

  def test_the_minint_edge_case
    run_test_as('programmer') do
      assert_equal -2147483648, simplify(command(%Q|; return -2147483648 / -1; |))
      assert_equal 0, simplify(command(%Q|; return -2147483648 % -1; |))
    end
  end

  def test_division
    run_test_as('programmer') do
      assert_equal 5, simplify(command(%Q|; return -15 / -3; |))
      assert_equal -5, simplify(command(%Q|; return -15 / 3; |))
      assert_equal -5, simplify(command(%Q|; return 15 / -3; |))
      assert_equal 5, simplify(command(%Q|; return 15 / 3; |))
    end
    run_test_as('programmer') do
      assert_equal 5.0, simplify(command(%Q|; return -15.0 / -3.0; |))
      assert_equal -5.0, simplify(command(%Q|; return -15.0 / 3.0; |))
      assert_equal -5.0, simplify(command(%Q|; return 15.0 / -3.0; |))
      assert_equal 5.0, simplify(command(%Q|; return 15.0 / 3.0; |))
    end
  end

  def test_modulus
    run_test_as('programmer') do
      assert_equal -3, simplify(command(%Q|; return -15 % -4; |))
      assert_equal -3, simplify(command(%Q|; return -15 % 4; |))
      assert_equal 3, simplify(command(%Q|; return 15 % -4; |))
      assert_equal 3, simplify(command(%Q|; return 15 % 4; |))
    end
    run_test_as('programmer') do
      assert_equal -3.0, simplify(command(%Q|; return -15.0 % -4.0; |))
      assert_equal -3.0, simplify(command(%Q|; return -15.0 % 4.0; |))
      assert_equal 3.0, simplify(command(%Q|; return 15.0 % -4.0; |))
      assert_equal 3.0, simplify(command(%Q|; return 15.0 % 4.0; |))
    end
  end

end
