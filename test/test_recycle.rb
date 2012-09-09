require 'test_helper'

class TestRecycle < Test::Unit::TestCase

  def test_that_the_first_argument_is_required
    run_test_as('programmer') do
      assert_equal E_ARGS, simplify(command("; recycle();"))
    end
  end

  def test_that_the_first_argument_can_be_an_object_or_an_anonymous_object
    run_test_as('wizard') do
      assert_equal E_TYPE, simplify(command("; recycle(1);"))
      assert_equal E_TYPE, simplify(command("; recycle({[]});"))
      assert_not_equal E_TYPE, simplify(command("; recycle($nothing);"))
      assert_not_equal E_TYPE, simplify(command("; recycle(create($object, 0));"))
      assert_not_equal E_TYPE, simplify(command("; recycle(create($anonymous, 1));"))
    end
  end

  def test_that_the_first_argument_must_be_valid
    run_test_as('programmer') do
      assert_equal E_INVARG, simplify(command("; x = create($object, 0); recycle(x); recycle(x);"))
      assert_equal E_INVARG, simplify(command("; x = create($anonymous, 1); recycle(x); recycle(x);"))
    end
  end

  def test_a_variety_of_argument_errors
    run_test_as('wizard') do
      assert_equal E_TYPE, simplify(command("; recycle(1.0);"))
      assert_equal E_TYPE, simplify(command("; recycle([]);"))
      assert_equal E_TYPE, simplify(command("; recycle({});"))
      assert_equal E_TYPE, simplify(command('; recycle("foobar");'))
    end
  end

  def test_that_a_wizard_can_recycle_anything
    m = nil
    n = nil
    run_test_as('wizard') do
      m = create(:object, 0)
      n = create(:object, 0)
      set(m, 'w', 0)
      set(n, 'w', 1)
      add_property(n, 'x', 0, ['player', 'r'])
      add_property(n, 'y', 0, ['player', 'r'])
      command("; #{n}.x = create($object, 0); #{n}.x.w = 0;")
      command("; #{n}.y = create($anonymous, 1); #{n}.y.w = 1;")
    end
    run_test_as('wizard') do
      add_property(n, 'i', 0, ['player', 'r'])
      add_property(n, 'j', 0, ['player', 'r'])
      command("; #{n}.i = create($object, 0);")
      command("; #{n}.j = create($anonymous, 1);")
      assert_not_equal E_PERM, simplify(command("; recycle(#{n}.i);"))
      assert_not_equal E_PERM, simplify(command("; recycle(#{n}.j);"))
      assert_not_equal E_PERM, simplify(command("; recycle(#{n}.x);"))
      assert_not_equal E_PERM, simplify(command("; recycle(#{n}.y);"))
      assert_not_equal E_PERM, simplify(command("; recycle(#{m});"))
      assert_not_equal E_PERM, simplify(command("; recycle(#{n});"))
    end
  end

  def test_that_a_programmer_can_only_recycle_things_it_controls
    m = nil
    n = nil
    run_test_as('programmer') do
      m = create(:object, 0)
      n = create(:object, 0)
      set(m, 'w', 0)
      set(n, 'w', 1)
      add_property(n, 'x', 0, ['player', 'r'])
      add_property(n, 'y', 0, ['player', 'r'])
      command("; #{n}.x = create($object, 0); #{n}.x.w = 0;")
      command("; #{n}.y = create($anonymous, 1); #{n}.y.w = 1;")
    end
    run_test_as('programmer') do
      add_property(n, 'i', 0, ['player', 'r'])
      add_property(n, 'j', 0, ['player', 'r'])
      command("; #{n}.i = create($object, 0);")
      command("; #{n}.j = create($anonymous, 1);")
      assert_not_equal E_PERM, simplify(command("; recycle(#{n}.i);"))
      assert_not_equal E_PERM, simplify(command("; recycle(#{n}.j);"))
      assert_equal E_PERM, simplify(command("; recycle(#{n}.x);"))
      assert_equal E_PERM, simplify(command("; recycle(#{n}.y);"))
      assert_equal E_PERM, simplify(command("; recycle(#{m});"))
      assert_equal E_PERM, simplify(command("; recycle(#{n});"))
    end
  end


  def test_that_recycling_an_object_calls_recycle
    run_test_as('programmer') do
      a = create(:object)
      add_property(a, 'recycle_called', 0, [player, ''])
      add_verb(a, ['player', 'xd', 'recycle'], ['this', 'none', 'this'])
      set_verb_code(a, 'recycle') do |vc|
        vc << %Q<typeof(this) == OBJ || raise(E_INVARG);>
        vc << %Q<#{a}.recycle_called = #{a}.recycle_called + 1;>
      end
      assert_equal 0, get(a, 'recycle_called')
      simplify(command("; recycle(create(#{a}, 0));"))
      assert_equal 1, get(a, 'recycle_called')
    end
  end

  def test_that_recycling_an_anonymous_object_calls_recycle
    run_test_as('programmer') do
      a = create(:object)
      add_property(a, 'recycle_called', 0, [player, ''])
      add_verb(a, ['player', 'xd', 'recycle'], ['this', 'none', 'this'])
      set_verb_code(a, 'recycle') do |vc|
        vc << %Q<typeof(this) == ANON || raise(E_INVARG);>
        vc << %Q<#{a}.recycle_called = #{a}.recycle_called + 1;>
      end
      assert_equal 0, get(a, 'recycle_called')
      simplify(command("; recycle(create(#{a}, 1));"))
      assert_equal 1, get(a, 'recycle_called')
    end
  end

  def test_that_calling_recycle_when_recycling_an_object_fails
    run_test_as('programmer') do
      a = create(:object)
      add_property(a, 'recycle_called', 0, [player, ''])
      add_verb(a, ['player', 'xd', 'recycle'], ['this', 'none', 'this'])
      set_verb_code(a, 'recycle') do |vc|
        vc << %Q<typeof(this) == OBJ || raise(E_INVARG);>
        vc << %Q<#{a}.recycle_called = #{a}.recycle_called + 1;>
        vc << %Q<recycle(this);>
      end
      assert_equal 0, get(a, 'recycle_called')
      simplify(command("; recycle(create(#{a}, 0));"))
      assert_equal 1, get(a, 'recycle_called')
    end
  end

  def test_that_calling_recycle_when_recycling_an_anonymous_object_fails
    run_test_as('programmer') do
      a = create(:object)
      add_property(a, 'recycle_called', 0, [player, ''])
      add_verb(a, ['player', 'xd', 'recycle'], ['this', 'none', 'this'])
      set_verb_code(a, 'recycle') do |vc|
        vc << %Q<typeof(this) == ANON || raise(E_INVARG);>
        vc << %Q<#{a}.recycle_called = #{a}.recycle_called + 1;>
        vc << %Q<recycle(this);>
      end
      assert_equal 0, get(a, 'recycle_called')
      simplify(command("; recycle(create(#{a}, 1));"))
      assert_equal 1, get(a, 'recycle_called')
    end
  end

end
