require 'test_helper'

class TestObjectsAndVerbs < Test::Unit::TestCase

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

  ## add_verb

  def test_that_add_verb_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        r = add_verb(o, ['player', 'x', 'foobar'], ['this', 'none', 'this'])
        assert_not_equal E_TYPE, r
        assert_not_equal E_INVARG, r
        assert_not_equal E_PERM, r
        assert_equal 0, call(o, 'foobar')
      end
    end
  end

  def test_that_add_verb_fails_if_the_owner_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_INVARG, add_verb(o, [:nothing, '', 'foobar'], ['this', 'none', 'this'])
      end
    end
  end

  def test_that_add_verb_fails_if_the_perms_are_garbage
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_INVARG, add_verb(o, ['player', 'abc', 'foobar'], ['this', 'none', 'this'])
      end
    end
  end

  def test_that_add_verb_fails_if_the_args_are_garbage
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_INVARG, add_verb(o, ['player', '', 'foobar'], ['foo', 'bar', 'baz'])
      end
    end
  end

  def test_that_add_verb_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, add_verb(o, ['player', '', 'foobar'], ['this', 'none', 'this'])
      end
    end
  end

  def test_that_add_verb_fails_if_the_programmer_does_not_have_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'w', 0)
      end
      run_test_as('programmer') do
        assert_equal E_PERM, add_verb(o, ['player', '', 'foobar'], ['this', 'none', 'this'])
      end
    end
  end

  def test_that_add_verb_succeeds_if_the_programmer_has_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'w', 1)
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, add_verb(o, ['player', '', 'foobar'], ['this', 'none', 'this'])
      end
    end
  end

  def test_that_add_verb_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'w', 0)
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, add_verb(o, ['player', '', 'foobar'], ['this', 'none', 'this'])
      end
    end
  end

  def test_that_add_verb_fails_if_the_programmer_is_not_the_owner_specified_in_verbinfo
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_PERM, add_verb(o, [:system, '', 'foobar'], ['this', 'none', 'this'])
      end
    end
  end

  def test_that_add_verb_succeeds_if_the_programmer_is_the_owner_specified_in_verbinfo
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_not_equal E_PERM, add_verb(o, [player, '', 'foobar'], ['this', 'none', 'this'])
      end
    end
  end

  def test_that_add_verb_sets_the_owner_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      run_test_as('wizard') do
        o = create(*args)
        assert_not_equal E_PERM, add_verb(o, [:system, '', 'foobar'], ['this', 'none', 'this'])
      end
    end
  end

  ## delete_verb

  def test_that_delete_verb_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, ['player', '', 'foobar'], ['this', 'none', 'this'])
        r = delete_verb(o, 'foobar')
        assert_not_equal E_TYPE, r
        assert_not_equal E_INVARG, r
        assert_not_equal E_PERM, r
        assert_not_equal E_VERBNF, r
      end
    end
  end

  def test_that_delete_verb_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, delete_verb(o, 'foobar')
      end
    end
  end

  def test_that_delete_verb_fails_if_the_verb_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_VERBNF, delete_verb(o, 'foobar')
      end
    end
  end

  def test_that_delete_verb_fails_if_the_programmer_does_not_have_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, ['player', '', 'foobar'], ['this', 'none', 'this'])
        set(o, 'w', 0)
      end
      run_test_as('programmer') do
        assert_equal E_PERM, delete_verb(o, 'foobar')
      end
    end
  end

  def test_that_delete_verb_succeeds_if_the_programmer_has_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, ['player', '', 'foobar'], ['this', 'none', 'this'])
        set(o, 'w', 1)
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, delete_verb(o, 'foobar')
      end
    end
  end

  def test_that_delete_verb_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, ['player', '', 'foobar'], ['this', 'none', 'this'])
        set(o, 'w', 0)
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, delete_verb(o, 'foobar')
      end
    end
  end

  ## verb_info

  def test_that_verb_info_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'rw', 'foobar'], ['this', 'none', 'this'])
        assert_equal [player, 'rw', 'foobar'], verb_info(o, 'foobar')
      end
    end
  end

  def test_that_verb_info_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, verb_info(o, 'foobar')
      end
    end
  end

  def test_that_verb_info_fails_if_the_verb_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_VERBNF, verb_info(o, 'foobar')
      end
    end
  end

  def test_that_verb_info_fails_if_the_programmer_does_not_have_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['this', 'none', 'this'])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, verb_info(o, 'foobar')
      end
    end
  end

  def test_that_verb_info_succeeds_if_the_programmer_has_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'r', 'foobar'], ['this', 'none', 'this'])
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, verb_info(o, 'foobar')
      end
    end
  end

  def test_that_verb_info_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['this', 'none', 'this'])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, verb_info(o, 'foobar')
      end
    end
  end

  ## verb_args

  def test_that_verb_args_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'rw', 'foobar'], ['any', 'on', 'this'])
        assert_equal ['any', 'on top of/on/onto/upon', 'this'], verb_args(o, 'foobar')
      end
    end
  end

  def test_that_verb_args_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, verb_args(o, 'foobar')
      end
    end
  end

  def test_that_verb_args_fails_if_the_verb_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_VERBNF, verb_args(o, 'foobar')
      end
    end
  end

  def test_that_verb_args_fails_if_the_programmer_does_not_have_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, verb_args(o, 'foobar')
      end
    end
  end

  def test_that_verb_args_succeeds_if_the_programmer_has_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'r', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, verb_args(o, 'foobar')
      end
    end
  end

  def test_that_verb_args_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, verb_args(o, 'foobar')
      end
    end
  end

  ## verb_code

  def test_that_verb_code_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'rw', 'foobar'], ['any', 'on', 'this'])
        assert_equal [], verb_code(o, 'foobar')
      end
    end
  end

  def test_that_verb_code_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, verb_code(o, 'foobar')
      end
    end
  end

  def test_that_verb_code_fails_if_the_verb_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_VERBNF, verb_code(o, 'foobar')
      end
    end
  end

  def test_that_verb_code_fails_if_the_programmer_does_not_have_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, verb_code(o, 'foobar')
      end
    end
  end

  def test_that_verb_code_succeeds_if_the_programmer_has_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'r', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, verb_code(o, 'foobar')
      end
    end
  end

  def test_that_verb_code_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, verb_code(o, 'foobar')
      end
    end
  end

  ## set_verb_info

  def test_that_set_verb_info_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'rw', 'foobar'], ['any', 'in', 'this'])
        set_verb_info(o, 'foobar', [player, 'x', 'barfoo'])
        assert_equal [player, 'x', 'barfoo'], verb_info(o, 'barfoo')
      end
    end
  end

  def test_that_set_verb_info_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, set_verb_info(o, 'foobar', [player, '', 'foobar'])
      end
    end
  end

  def test_that_set_verb_info_fails_if_the_verb_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_VERBNF, set_verb_info(o, 'foobar', [player, '', 'foobar'])
      end
    end
  end

  def test_that_set_verb_info_fails_if_the_programmer_does_not_have_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, set_verb_info(o, 'foobar', [player, 'rw', 'barfoo'])
      end
    end
  end

  def test_that_set_verb_info_fails_even_if_the_programmer_has_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'w', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, set_verb_info(o, 'foobar', [player, 'rw', 'barfoo'])
      end
    end
  end

  def test_that_set_verb_info_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, set_verb_info(o, 'foobar', [player, 'rw', 'barfoo'])
      end
    end
  end

  ## set_verb_args

  def test_that_set_verb_args_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'rw', 'foobar'], ['any', 'in', 'this'])
        set_verb_args(o, 'foobar', ['any', 'any', 'any'])
        assert_equal ['any', 'any', 'any'], verb_args(o, 'foobar')
      end
    end
  end

  def test_that_set_verb_args_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, set_verb_args(o, 'foobar', ['any', 'any', 'any'])
      end
    end
  end

  def test_that_set_verb_args_fails_if_the_verb_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_VERBNF, set_verb_args(o, 'foobar', ['any', 'any', 'any'])
      end
    end
  end

  def test_that_set_verb_args_fails_if_the_programmer_does_not_have_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['this', 'in', 'this'])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, set_verb_args(o, 'foobar', ['any', 'any', 'any'])
      end
    end
  end

  def test_that_set_verb_args_succeeds_if_the_programmer_has_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'w', 'foobar'], ['this', 'in', 'this'])
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, set_verb_args(o, 'foobar', ['any', 'any', 'any'])
      end
    end
  end

  def test_that_set_verb_args_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['this', 'in', 'this'])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, set_verb_args(o, 'foobar', ['any', 'any', 'any'])
      end
    end
  end

  ## set_verb_code

  def test_that_set_verb_code_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'rw', 'foobar'], ['any', 'in', 'this'])
        set_verb_code(o, 'foobar', ['1;', '2;', '3;'])
        assert_equal ['1;', '2;', '3;'], verb_code(o, 'foobar')
      end
    end
  end

  def test_that_set_verb_code_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, set_verb_code(o, 'foobar', ['1;', '2;', '3;'])
      end
    end
  end

  def test_that_set_verb_code_fails_if_the_verb_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_VERBNF, set_verb_code(o, 'foobar', ['1;', '2;', '3;'])
      end
    end
  end

  def test_that_set_verb_code_fails_if_the_programmer_does_not_have_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['this', 'in', 'this'])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, set_verb_code(o, 'foobar', ['1;', '2;', '3;'])
      end
    end
  end

  def test_that_set_verb_code_succeeds_if_the_programmer_has_write_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'w', 'foobar'], ['this', 'in', 'this'])
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, set_verb_code(o, 'foobar', ['1;', '2;', '3;'])
      end
    end
  end

  def test_that_set_verb_code_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['this', 'in', 'this'])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, set_verb_code(o, 'foobar', ['1;', '2;', '3;'])
      end
    end
  end

  ## verbs

  def test_that_verbs_works_on_objects
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal [], verbs(o)
        add_verb(o, ['player', '', 'foobar'], ['this', 'none', 'this'])
        assert_equal ['foobar'], verbs(o)
      end
    end
  end

  def test_that_verbs_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, verbs(o)
      end
    end
  end

  def test_that_verbs_fails_if_the_programmer_does_not_have_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'r', 0)
      end
      run_test_as('programmer') do
        assert_equal E_PERM, verbs(o)
      end
    end
  end

  def test_that_verbs_succeeds_if_the_programmer_has_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'r', 1)
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, verbs(o)
      end
    end
  end

  def test_that_verbs_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'r', 0)
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, verbs(o)
      end
    end
  end

  ## respond_to

  def test_that_respond_to_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, respond_to(o, 'foobar')
      end
    end
  end

  def test_that_respond_to_returns_false_if_the_verb_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal 0, respond_to(o, 'foobar')
      end
    end
  end

  def test_that_respond_to_returns_verb_details_if_the_caller_is_the_owner
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, ['player', 'x', 'foo'], ['none', 'none', 'none'])
        add_verb(o, ['player', '', 'bar'], ['none', 'none', 'none'])
        assert_equal 1, simplify(command(%Q|; return respond_to(#{o}, "foo") == {#{o}, "foo"}; |))
        assert_equal 1, simplify(command(%Q|; return respond_to(#{o}, "bar") == 0; |))
      end
    end
  end

  def test_that_respond_to_returns_verb_details_if_the_caller_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, ['player', 'x', 'foo'], ['none', 'none', 'none'])
        add_verb(o, ['player', '', 'bar'], ['none', 'none', 'none'])
      end
      run_test_as('wizard') do
        assert_equal 1, simplify(command(%Q|; return respond_to(#{o}, "foo") == {#{o}, "foo"}; |))
        assert_equal 1, simplify(command(%Q|; return respond_to(#{o}, "bar") == 0; |))
      end
    end
  end

  def test_that_respond_to_returns_verb_details_if_the_object_is_readable
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'r', 1)
        add_verb(o, ['player', 'x', 'foo'], ['none', 'none', 'none'])
        add_verb(o, ['player', '', 'bar'], ['none', 'none', 'none'])
      end
      run_test_as('programmer') do
        assert_equal 1, simplify(command(%Q|; return respond_to(#{o}, "foo") == {#{o}, "foo"}; |))
        assert_equal 1, simplify(command(%Q|; return respond_to(#{o}, "bar") == 0; |))
      end
    end
  end

  def test_that_respond_to_returns_true_if_the_verb_is_callable
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'r', 0)
        add_verb(o, ['player', 'x', 'foo'], ['none', 'none', 'none'])
      end
      run_test_as('programmer') do
        assert_equal 1, simplify(command(%Q|; return respond_to(#{o}, "foo") == 1; |))
      end
    end
  end

  def test_that_respond_to_returns_false_if_the_verb_is_not_callable
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        set(o, 'r', 0)
        add_verb(o, ['player', '', 'bar'], ['none', 'none', 'none'])
      end
      run_test_as('programmer') do
        assert_equal 1, simplify(command(%Q|; return respond_to(#{o}, "bar") == 0; |))
      end
    end
  end

  ## disassemble

  def test_that_disassemble_works
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'rw', 'foobar'], ['this', 'none', 'this'])
        assert_includes disassemble(o, 'foobar'), 'Main code vector:'
      end
    end
  end

  def test_that_disassemble_fails_if_the_object_is_not_valid
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        recycle(o)
        assert_equal E_INVARG, disassemble(o, 'foobar')
      end
    end
  end

  def test_that_disassemble_fails_if_the_verb_does_not_exist
    SCENARIOS.each do |args|
      run_test_as('programmer') do
        o = create(*args)
        assert_equal E_VERBNF, disassemble(o, 'foobar')
      end
    end
  end

  def test_that_disassemble_fails_if_the_programmer_does_not_have_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('programmer') do
        assert_equal E_PERM, disassemble(o, 'foobar')
      end
    end
  end

  def test_that_disassemble_succeeds_if_the_programmer_has_read_permission
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, 'r', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('programmer') do
        assert_not_equal E_PERM, disassemble(o, 'foobar')
      end
    end
  end

  def test_that_disassemble_succeeds_if_the_programmer_is_a_wizard
    SCENARIOS.each do |args|
      o = nil
      run_test_as('programmer') do
        o = create(*args)
        add_verb(o, [player, '', 'foobar'], ['any', 'none', 'any'])
      end
      run_test_as('wizard') do
        assert_not_equal E_PERM, disassemble(o, 'foobar')
      end
    end
  end

end
