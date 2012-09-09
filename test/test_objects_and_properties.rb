require 'test_helper'

class TestObjectsAndProperties < Test::Unit::TestCase

  def setup
    run_test_as('programmer') do
      @obj = simplify(command(%Q|; return create($nothing);|))
      add_property(@obj, 'tmp', 0, ['player', 'rw'])
    end
  end

  ## Must redefine this because anonymous objects can't be
  ## passed in/out/through Ruby-space.
  def create(*args)
    simplify(command(%Q|; #{@obj}.tmp = create(#{args.map{|a| value_ref(a)}.join(', ')}); return "#{@obj}.tmp";|))
  end

  SCENARIOS = [
    [:object, 0],
    [:anonymous, 1]
  ]

  ## add_property

  def test_that_add_property_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        r = add_property(o, 'foobar', 0, ['player', ''])
        assert_not_equal E_TYPE, r
        assert_not_equal E_INVARG, r
        assert_not_equal E_PERM, r
        assert_equal 'abc', set(o, 'foobar', 'abc')
        assert_equal 'abc', get(o, 'foobar')
      end
    end
  end

  def test_that_add_property_fails_if_the_owner_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_INVARG, add_property(o, 'foobar', 0, [:nothing, ''])
      end
    end
  end

  def test_that_add_property_fails_if_the_perms_are_garbage
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_INVARG, add_property(o, 'foobar', 0, ['player', 'abc'])
      end
    end
  end

  def test_that_add_property_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, add_property(o, 'foobar', 0, ['player', ''])
      end
    end
  end

  def test_that_add_property_fails_if_the_property_is_built_in
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_INVARG, add_property(o, 'name', 0, ['player', ''])
      end
    end
  end

  def test_that_add_property_fails_if_the_property_is_already_defined_on_an_ancestor
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = simplify(command(%Q|; return create($nothing);|))
        add_property(o, 'foobar', 123, ['player', ''])
        p = create(o, args[1])
        assert_equal E_INVARG, add_property(p, 'foobar', 0, ['player', ''])
      end
    end
  end

  # only permanent objects can have descendants

  def test_that_add_property_fails_if_the_property_is_already_defined_on_a_descendant
    run_test_as('programmer') do
      x = simplify(command(%Q|; return create($nothing);|))
      o = simplify(command(%Q|; return create(#{x});|))
      add_property(o, 'foobar', 0, ['player', 'rw'])
      assert_equal E_INVARG, add_property(x, 'foobar', 0, ['player', ''])
    end
  end

  def test_that_add_property_fails_if_the_programmer_does_not_have_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'w', 0)
      end
      run_test_as('programmer') do
        assert_equal E_PERM, add_property(o, 'foobar', 0, ['player', ''])
      end
    end
  end

  def test_that_add_property_succeeds_if_the_programmer_has_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'w', 1)
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, add_property(o, 'foobar', 0, ['player', ''])
      end
    end
  end

  def test_that_add_property_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'w', 0)
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, add_property(o, 'foobar', 0, ['player', ''])
      end
    end
  end

  def test_that_add_property_fails_if_the_programmer_is_not_the_owner_specified_in_propinfo
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PERM, add_property(o, 'foobar', 0, [:system, ''])
      end
    end
  end

  def test_that_add_property_succeeds_if_the_programmer_is_the_owner_specified_in_propinfo
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_not_equal E_PERM, add_property(o, 'foobar', 0, [player, ''])
      end
    end
  end

  def test_that_add_property_sets_the_owner_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      run_test_as('wizard') do
        o = create(*args)
        assert_not_equal E_PERM, add_property(o, 'foobar', 0, [:system, ''])
      end
    end
  end

  ## delete_property

  def test_that_delete_property_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', ''])
        r = delete_property(o, 'foobar')
        assert_not_equal E_TYPE, r
        assert_not_equal E_INVARG, r
        assert_not_equal E_PERM, r
        assert_not_equal E_PROPNF, r
      end
    end
  end

  def test_that_delete_property_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, delete_property(o, 'foobar')
      end
    end
  end

  def test_that_delete_property_fails_if_the_property_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PROPNF, delete_property(o, 'foobar')
      end
    end
  end

  def test_that_delete_property_fails_if_the_property_is_built_in
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PROPNF, delete_property(o, 'name')
      end
    end
  end

  def test_that_delete_property_fails_if_the_programmer_does_not_have_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', ''])
        set(o, 'w', 0)
      end
      run_test_as('programmer') do
        assert_equal E_PERM, delete_property(o, 'foobar')
      end
    end
  end

  def test_that_delete_property_succeeds_if_the_programmer_has_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', ''])
        set(o, 'w', 1)
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, delete_property(o, 'foobar')
      end
    end
  end

  def test_that_delete_property_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', ''])
        set(o, 'w', 0)
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, delete_property(o, 'foobar')
      end
    end
  end

  ## is_clear_property

  def test_that_is_clear_property_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = simplify(command(%Q|; return create($nothing);|))
        add_property(o, 'foobar', 123, ['player', ''])
        p = create(o, args[1])
        assert_equal 1, is_clear_property(p, 'foobar')
        set(p, 'foobar', 'hello')
        assert_equal 0, is_clear_property(p, 'foobar')
      end
    end
  end

  def test_that_is_clear_property_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, is_clear_property(o, 'foobar')
      end
    end
  end

  def test_that_is_clear_property_fails_if_the_property_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PROPNF, is_clear_property(o, 'foobar')
      end
    end
  end

  def test_that_is_clear_property_returns_false_if_the_property_is_built_in
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal 0, is_clear_property(o, 'name')
      end
    end
  end

  def test_that_is_clear_property_returns_false_if_called_on_the_definer
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', ''])
        assert_equal 0, is_clear_property(o, 'foobar')
      end
    end
  end

  def test_that_is_clear_property_fails_if_the_programmer_does_not_have_read_permission
    SCENARIOS.each do |args|
      p = nil
      run_test_as('programmer') do
        o = simplify(command(%Q|; return create($nothing);|))
        add_property(o, 'foobar', 0, ['player', ''])
        p = create(o, args[1])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, is_clear_property(p, 'foobar')
      end
    end
  end

  def test_that_is_clear_property_succeeds_if_the_programmer_has_read_permission
    SCENARIOS.each do |args|
      p = nil
      run_test_as('programmer') do
        o = simplify(command(%Q|; return create($nothing);|))
        add_property(o, 'foobar', 0, ['player', 'r'])
        p = create(o, args[1])
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, is_clear_property(p, 'foobar')
      end
    end
  end

  def test_that_is_clear_property_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      p = nil
      run_test_as('programmer') do
        o = simplify(command(%Q|; return create($nothing);|))
        add_property(o, 'foobar', 0, ['player', ''])
        p = create(o, args[1])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, is_clear_property(p, 'foobar')
      end
    end
  end

  ## clear_property

  # The implementation of `clear_property()' isn't perfectly
  # consistent with the documentation in the standard server.
  # Specifically, non-wizard, non-owners can't clear a property, even
  # if the property is writable -- contrary to what the docs say.
  # Stunt fixes this inconsistency.

  def test_that_clear_property_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = simplify(command(%Q|; return create($nothing);|))
        add_property(o, 'foobar', 123, ['player', ''])
        p = create(o, args[1])
        set(p, 'foobar', 'hello')
        assert_equal 'hello', get(p, 'foobar')
        clear_property(p, 'foobar')
        assert_equal 123, get(p, 'foobar')
      end
    end
  end

  def test_that_clear_property_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, clear_property(o, 'foobar')
      end
    end
  end

  def test_that_clear_property_fails_if_the_property_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PROPNF, clear_property(o, 'foobar')
      end
    end
  end

  def test_that_clear_property_raises_an_error_if_the_property_is_built_in
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PERM, clear_property(o, 'name')
      end
    end
  end

  def test_that_clear_property_raises_an_error_if_called_on_the_definer
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', ''])
        assert_equal E_INVARG, clear_property(o, 'foobar')
      end
    end
  end

  def test_that_clear_property_fails_if_the_programmer_does_not_have_write_permission
    SCENARIOS.each do |args|
      p = nil
      run_test_as('programmer') do
        o = simplify(command(%Q|; return create($nothing);|))
        add_property(o, 'foobar', 0, ['player', ''])
        p = create(o, args[1])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, clear_property(p, 'foobar')
      end
    end
  end

  def test_that_clear_property_succeeds_if_the_programmer_has_write_permission
    SCENARIOS.each do |args|
      p = nil
      run_test_as('programmer') do
        o = simplify(command(%Q|; return create($nothing);|))
        add_property(o, 'foobar', 0, ['player', 'w'])
        p = create(o, args[1])
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, clear_property(p, 'foobar')
      end
    end
  end

  def test_that_clear_property_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      p = nil
      run_test_as('programmer') do
        o = simplify(command(%Q|; return create($nothing);|))
        add_property(o, 'foobar', 0, ['player', ''])
        p = create(o, args[1])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, clear_property(p, 'foobar')
      end
    end
  end

  ## property_info

  def test_that_property_info_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, [player, 'rw'])
        assert_equal [player, 'rw'], property_info(o, 'foobar')
      end
    end
  end

  def test_that_property_info_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, property_info(o, 'foobar')
      end
    end
  end

  def test_that_property_info_fails_if_the_property_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PROPNF, property_info(o, 'foobar')
      end
    end
  end

  def test_that_property_info_raises_an_error_if_the_property_is_built_in
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PROPNF, property_info(o, 'name')
      end
    end
  end

  def test_that_property_info_fails_if_the_programmer_does_not_have_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', ''])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, property_info(o, 'foobar')
      end
    end
  end

  def test_that_property_info_succeeds_if_the_programmer_has_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', 'r'])
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, property_info(o, 'foobar')
      end
    end
  end

  def test_that_property_info_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', ''])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, property_info(o, 'foobar')
      end
    end
  end

  ## set_property_info

  def test_that_set_property_info_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, [player, 'rw'])
        set_property_info(o, 'foobar', [player, 'c'])
        assert_equal [player, 'c'], property_info(o, 'foobar')
      end
    end
  end

  def test_that_set_property_info_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, set_property_info(o, 'foobar', [player, ''])
      end
    end
  end

  def test_that_set_property_info_fails_if_the_property_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PROPNF, set_property_info(o, 'foobar', [player, ''])
      end
    end
  end

  def test_that_set_property_info_raises_an_error_if_the_property_is_built_in
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PROPNF, set_property_info(o, 'name', [player, ''])
      end
    end
  end

  def test_that_set_property_info_fails_if_the_programmer_does_not_have_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', ''])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, set_property_info(o, 'foobar', [player, ''])
      end
    end
  end

  def test_that_set_property_info_fails_even_if_the_programmer_has_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', 'w'])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, set_property_info(o, 'foobar', [player, ''])
      end
    end
  end

  def test_that_set_property_info_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_property(o, 'foobar', 0, ['player', ''])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, set_property_info(o, 'foobar', [player, ''])
      end
    end
  end

  ## properties

  def test_that_properties_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal [], properties(o)
        add_property(o, 'foobar', 0, [player, 'rw'])
        assert_equal ['foobar'], properties(o)
      end
    end
  end

  def test_that_properties_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, properties(o)
      end
    end
  end

  def test_that_properties_fails_if_the_programmer_does_not_have_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'r', 0)
      end
      run_test_as('programmer') do
        assert_equal E_PERM, properties(o)
      end
    end
  end

  def test_that_properties_succeeds_if_the_programmer_has_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'r', 1)
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, properties(o)
      end
    end
  end

  def test_that_properties_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'r', 0)
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, properties(o)
      end
    end
  end

end
