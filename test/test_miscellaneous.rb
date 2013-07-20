require 'test_helper'

class TestMiscellaneous < Test::Unit::TestCase

  def test_that_isa_does_not_crash_the_server
    run_test_as('programmer') do
      result = simplify command %Q|; return isa(#-2, #1); |
      assert_equal E_INVARG, result
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
