require 'test_helper'

class TestMiscellaneous < Test::Unit::TestCase

  def test_that_isa_returns_true_or_false_given_an_object_number
    run_test_as('programmer') do
      a = create(:nothing)
      b = create(a)

      assert_equal 0, isa(a, b)
      assert_equal 1, isa(b, a)

      assert_equal 0, isa(b, :nothing)
      assert_equal 0, isa(:nothing, a)
      assert_equal 0, isa(:nothing, :nothing)

      assert_equal 0, simplify(command(%Q|; return isa(#100000, #1); |))
      assert_equal 0, simplify(command(%Q|; return isa(#1, #100000); |))
      assert_equal 0, simplify(command(%Q|; return isa(#1000000, #1000000); |))
      assert_equal 0, simplify(command(%Q|; return isa(#-2, #2); |))
      assert_equal 0, simplify(command(%Q|; return isa(#1, #-1); |))
    end
  end

  def test_that_respond_to_returns_verb_details_if_the_caller_is_the_owner
    run_test_as('programmer') do
      o = create(:nothing)
      add_verb(o, ['player', 'x', 'foo'], ['none', 'none', 'none'])
      add_verb(o, ['player', '', 'bar'], ['none', 'none', 'none'])
      assert_equal [o, 'foo'], simplify(command(%Q|; return respond_to(#{o}, "foo"); |))
      assert_equal 0, simplify(command(%Q|; return respond_to(#{o}, "bar"); |))
    end
  end

  def test_that_respond_to_returns_verb_details_if_the_caller_is_a_wizard
    o = nil
    run_test_as('programmer') do
      o = create(:nothing)
      add_verb(o, ['player', 'x', 'foo'], ['none', 'none', 'none'])
      add_verb(o, ['player', '', 'bar'], ['none', 'none', 'none'])
    end
    run_test_as('wizard') do
      assert_equal [o, 'foo'], simplify(command(%Q|; return respond_to(#{o}, "foo"); |))
      assert_equal 0, simplify(command(%Q|; return respond_to(#{o}, "bar"); |))
    end
  end

  def test_that_respond_to_returns_verb_details_if_the_object_is_readable
    o = nil
    run_test_as('programmer') do
      o = create(:nothing)
      set(o, 'r', 1)
      add_verb(o, ['player', 'x', 'foo'], ['none', 'none', 'none'])
      add_verb(o, ['player', '', 'bar'], ['none', 'none', 'none'])
    end
    run_test_as('programmer') do
      assert_equal [o, 'foo'], simplify(command(%Q|; return respond_to(#{o}, "foo"); |))
      assert_equal 0, simplify(command(%Q|; return respond_to(#{o}, "bar"); |))
    end
  end

  def test_that_respond_to_returns_true_if_the_verb_is_callable
    o = nil
    run_test_as('programmer') do
      o = create(:nothing)
      set(o, 'r', 0)
      add_verb(o, ['player', 'x', 'foo'], ['none', 'none', 'none'])
    end
    run_test_as('programmer') do
      assert_equal 1, simplify(command(%Q|; return respond_to(#{o}, "foo"); |))
    end
  end

  def test_that_respond_to_returns_false_if_the_verb_is_not_callable
    o = nil
    run_test_as('programmer') do
      o = create(:nothing)
      set(o, 'r', 0)
      add_verb(o, ['player', '', 'foo'], ['none', 'none', 'none'])
    end
    run_test_as('programmer') do
      assert_equal 0, simplify(command(%Q|; return respond_to(#{o}, "foo"); |))
    end
  end

end
