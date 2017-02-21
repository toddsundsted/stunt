require 'test_helper'

class TestCallFunction < Test::Unit::TestCase

  def test_that_call_function_fails_if_name_is_not_a_string
    run_test_as('programmer') do
      assert_equal E_TYPE, call_function(1)
    end
  end

  def test_that_call_function_fails_if_function_does_not_exist
    run_test_as('programmer') do
      assert_equal E_INVARG, call_function('foobar')
    end
  end

  def test_that_call_function_can_call_call_function
    run_test_as('programmer') do
      assert_match SERVER_VERSION, call_function('call_function', 'server_version')
    end
  end

  def test_that_call_function_can_call_suspend
    run_test_as('programmer') do
      assert_equal 0, call_function('suspend', 0)
    end
  end

  def test_that_call_function_can_call_functions_that_call_verbs
    run_test_as('wizard') do
      begin
        o = create(:nothing)
        p = '_' + rand(36**5).to_s(36)
        add_property(SYSTEM, p, 0, ['player', 'r'])
        add_verb(o, ['player', 'xd', 'recycle'], ['this', 'none', 'this'])
        set_verb_code(o, 'recycle') do |vc|
          vc << %Q<$system.#{p} = 1;>
        end
        assert_equal 0, get(SYSTEM, p)
        call_function('call_function', 'recycle', o)
        assert_equal 1, get(SYSTEM, p)
      ensure
        delete_property(SYSTEM, p)
      end
    end
  end

  def test_that_call_function_can_call_functions_that_are_overridden
    run_test_as('wizard') do
      begin
        o = create(:nothing)
        chparents(o, parents(SERVER_OPTIONS))
        chparent(SERVER_OPTIONS, o)
        p = create(:nothing)
        chparents(p, parents(SYSTEM))
        chparent(SYSTEM, p)
        add_property(o, 'protect_server_version', 1, ['player', 'r'])
        add_verb(p, ['player', 'xd', 'bf_server_version'], ['this', 'none', 'this'])
        set_verb_code(p, 'bf_server_version') do |vc|
          vc << %Q<return "hello";>
        end
        load_server_options
        assert_equal 'hello', call_function('server_version')
      ensure
        recycle(o)
        recycle(p)
      end
    end
  end

  def test_that_call_function_checks_permissions
    o = nil
    run_test_as('wizard') do
      o = create(:nothing)
    end
    run_test_as('programmer') do
      assert_equal E_PERM, call_function('create', SYSTEM)
      assert_equal E_PERM, call_function('call_function', 'recycle', o)
    end
  end

  def test_that_chains_work
    run_test_as('programmer') do
      assert_match SERVER_VERSION, call_function(
                     'call_function',
                     'call_function',
                     'call_function',
                     'call_function',
                     'server_version'
                   )
    end
  end

end
