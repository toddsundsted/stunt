require 'test_helper'

class TestObject < Test::Unit::TestCase

  def test_create
    run_test_as('wizard') do
      a = create(NOTHING)
      b = create(a)
      c = create(b)
      d = create(b, a)

      assert_equal NOTHING, parent(a)
      assert_equal a, parent(b)
      assert_equal b, parent(c)
      assert_equal b, parent(d)

      assert_equal [b], children(a)
      assert_equal [c, d], children(b)
      assert_equal [], children(c)
      assert_equal [], children(d)

      e = create(NOTHING, NOTHING)
      f = create(NOTHING, AMBIGUOUS_MATCH)
      g = create(NOTHING, FAILED_MATCH)
      h = create(NOTHING, invalid_object)

      assert_equal player, get(a, 'owner')
      assert_equal player, get(b, 'owner')
      assert_equal player, get(c, 'owner')
      assert_equal a, get(d, 'owner')

      assert_equal e, get(e, 'owner')
      assert_equal AMBIGUOUS_MATCH, get(f, 'owner')
      assert_equal FAILED_MATCH, get(g, 'owner')
      assert_equal invalid_object, get(h, 'owner')
    end
  end

  def test_create_errors
    run_test_as('wizard') do
      assert_equal E_ARGS, create()
      assert_equal E_ARGS, create(1, 2, 3)
      assert_equal E_TYPE, create(1)
      assert_equal E_TYPE, create('1')
      assert_equal E_TYPE, create(1, 2)
      assert_equal E_TYPE, create(:object, 2)
      assert_equal E_TYPE, create(:object, '2')

      assert_equal E_PERM, create(AMBIGUOUS_MATCH)
      assert_equal E_PERM, create(FAILED_MATCH)
      assert_equal E_PERM, create(invalid_object)
    end

    # A variety of tests that check permissions.

    wizard = nil
    a = b = nil

    run_test_as('wizard') do
      wizard = player

      a = create(NOTHING)
      b = create(a, a)
      set(b, 'f', 1)

      assert_equal wizard, get(a, 'owner')
      assert_equal a, get(b, 'owner')
    end

    programmer = nil
    c = d = nil

    run_test_as('programmer') do
      programmer = player

      assert_equal E_PERM, create(a)
      assert_equal E_PERM, create(b, wizard)
      assert_equal E_PERM, create(b, NOTHING)
      assert_equal E_PERM, create(b, invalid_object)
      assert_not_equal E_PERM, create(b)

      c = create(NOTHING)
      d = create(b, programmer)
      set(d, 'f', 1)

      assert_equal programmer, get(c, 'owner')
      assert_equal programmer, get(d, 'owner')
    end

    run_test_as('wizard') do
      assert_not_equal E_PERM, create(a)
      assert_not_equal E_PERM, create(b, wizard)
      assert_not_equal E_PERM, create(c, programmer)
      assert_not_equal E_PERM, create(d)
    end
  end

  def test_parent_chparent
    run_test_as('wizard') do
      a = create(NOTHING)
      b = create(NOTHING)
      c = create(NOTHING)

      assert_equal NOTHING, parent(a)
      assert_equal NOTHING, parent(b)
      assert_equal NOTHING, parent(c)

      chparent(a, b)
      chparent(b, c)

      assert_equal b, parent(a)
      assert_equal c, parent(b)
      assert_equal NOTHING, parent(c)

      assert_equal [], children(a)
      assert_equal [a], children(b)
      assert_equal [b], children(c)

      chparent(a, c)

      assert_equal c, parent(a)
      assert_equal c, parent(b)
      assert_equal NOTHING, parent(c)

      assert_equal [], children(a)
      assert_equal [], children(b)
      assert_equal [b, a], children(c)

      chparent(a, NOTHING)
      chparent(b, NOTHING)
      chparent(c, NOTHING)

      assert_equal NOTHING, parent(a)
      assert_equal NOTHING, parent(b)
      assert_equal NOTHING, parent(c)

      assert_equal [], children(a)
      assert_equal [], children(b)
      assert_equal [], children(c)

      # If two objects define the same property by name, the assigned
      # property value (and info) is not preserved across chparent.

      add_property(a, 'foo', 'foo', [a, 'c'])
      add_property(b, 'foo', 'foo', [b, ''])

      chparent(c, a)
      assert_equal [player, 'c'], property_info(c, 'foo')
      set(c, 'foo', 'bar')
      assert_equal 'bar', get(c, 'foo')
      chparent(c, b)
      assert_equal [b, ''], property_info(c, 'foo')
      assert_equal 'foo', get(c, 'foo')

      m = create(NOTHING)
      n = create(NOTHING)

      # If a descendent object defines a property with the same name
      # as one defined either on the new parent or on one of its
      # ancestors, the chparent fails.

      chparent c, NOTHING

      add_property(n, 'foo', 'foo', [n, 'rw'])

      chparent a, m
      assert_equal E_INVARG, chparent(m, n)

      chparent b, m
      assert_equal E_INVARG, chparent(m, n)

      delete_property(a, 'foo')
      delete_property(b, 'foo')
    end
  end

  def test_parent_chparent_errors
    run_test_as('wizard') do
      a = create(NOTHING)
      b = create(NOTHING)
      c = create(NOTHING)

      chparent a, b
      chparent b, c

      assert_equal E_ARGS, parent()
      assert_equal E_ARGS, parent(1, 2)
      assert_equal E_TYPE, parent(1)
      assert_equal E_TYPE, parent('1')
      assert_equal E_INVARG, parent(:nothing)
      assert_equal E_INVARG, parent(invalid_object)

      assert_equal E_ARGS, chparent()
      assert_equal E_ARGS, chparent(1)
      assert_equal E_ARGS, chparent(1, 2, 3)
      assert_equal E_TYPE, chparent(1, 2)
      assert_equal E_TYPE, chparent(:object, 2)
      assert_equal E_TYPE, chparent(:object, '2')
      assert_equal E_INVARG, chparent(NOTHING, :object)
      assert_equal E_INVARG, chparent(:object, invalid_object)
      assert_equal E_RECMOVE, chparent(:object, :object)
      assert_equal E_RECMOVE, chparent(c, a)
    end

    wizard = nil
    a = b = nil

    run_test_as('wizard') do
      wizard = player

      a = create(NOTHING)
      b = create(a, a)
      set(b, 'f', 1)

      assert_equal wizard, get(a, 'owner')
      assert_equal a, get(b, 'owner')
    end

    programmer = nil
    c = d = nil

    run_test_as('programmer') do
      programmer = player

      c = create(NOTHING)
      d = create(b, programmer)
      set(d, 'f', 1)

      assert_equal E_PERM, chparent(c, a)
      assert_not_equal E_PERM, chparent(c, b)

      assert_equal programmer, get(c, 'owner')
      assert_equal programmer, get(d, 'owner')
    end

    run_test_as('wizard') do
      e = create(NOTHING)

      assert_not_equal E_PERM, chparent(e, a)
      assert_not_equal E_PERM, chparent(e, b)
      assert_not_equal E_PERM, chparent(e, c)
      assert_not_equal E_PERM, chparent(e, d)
    end
  end

  def test_recycle
    run_test_as('wizard') do
      a = kahuna(NOTHING, NOTHING, 'a')
      b = kahuna(a, a, 'b')
      m = kahuna(b, b, 'm')
      n = kahuna(b, b, 'n')

      assert_equal NOTHING, parent(a)
      assert_equal a, parent(b)
      assert_equal b, parent(m)
      assert_equal b, parent(n)

      assert_equal [b], children(a)
      assert_equal [m, n], children(b)
      assert_equal [], children(m)
      assert_equal [], children(n)

      assert_equal NOTHING, get(a, 'location')
      assert_equal a, get(b, 'location')
      assert_equal b, get(m, 'location')
      assert_equal b, get(n, 'location')

      assert_equal [b], get(a, 'contents')
      assert_equal [m, n], get(b, 'contents')
      assert_equal [], get(m, 'contents')
      assert_equal [], get(n, 'contents')

      recycle(b)

      assert_equal NOTHING, parent(a)
      assert_equal E_INVARG, parent(b)
      assert_equal a, parent(m)
      assert_equal a, parent(n)

      assert_equal [m, n], children(a)
      assert_equal E_INVARG, children(b)
      assert_equal [], children(m)
      assert_equal [], children(n)

      assert_equal NOTHING, get(a, 'location')
      assert_equal E_INVIND, get(b, 'location')
      assert_equal NOTHING, get(m, 'location')
      assert_equal NOTHING, get(n, 'location')

      assert_equal [], get(a, 'contents')
      assert_equal E_INVIND, get(b, 'contents')
      assert_equal [], get(m, 'contents')
      assert_equal [], get(n, 'contents')

      assert_equal 'a', call(a, 'a')
      assert_equal 'a', call(m, 'a')
      assert_equal 'a', call(n, 'a')
      assert_equal 'm', call(m, 'm')
      assert_equal 'n', call(n, 'n')

      assert_equal 'a', get(a, 'a')
      assert_equal 'a', get(m, 'a')
      assert_equal 'a', get(n, 'a')
      assert_equal 'm', get(m, 'm')
      assert_equal 'n', get(n, 'n')
    end
  end

  def test_renumber
    run_test_as('wizard') do
      recycle(create(:nothing)) # create a hole

      l = eval(%|for o in [#0..max_object()]; if (!valid(o)); return o; endif; endfor; return $nothing;|)

      a = create(NOTHING)
      b = create(a)
      c = create(b)

      move(b, a)
      move(c, b)

      assert_equal NOTHING, parent(a)
      assert_equal a, parent(b)
      assert_equal b, parent(c)

      assert_equal [b], children(a)
      assert_equal [c], children(b)
      assert_equal [], children(c)

      assert_equal NOTHING, get(a, 'location')
      assert_equal a, get(b, 'location')
      assert_equal b, get(c, 'location')

      assert_equal [b], get(a, 'contents')
      assert_equal [c], get(b, 'contents')
      assert_equal [], get(c, 'contents')

      assert_equal l, renumber(b)

      assert_equal NOTHING, parent(a)
      assert_equal E_INVARG, parent(b)
      assert_equal a, parent(l)
      assert_equal l, parent(c)

      assert_equal [l], children(a)
      assert_equal E_INVARG, children(b)
      assert_equal [c], children(l)
      assert_equal [], children(c)

      assert_equal NOTHING, get(a, 'location')
      assert_equal E_INVIND, get(b, 'location')
      assert_equal a, get(l, 'location')
      assert_equal l, get(c, 'location')

      assert_equal [l], get(a, 'contents')
      assert_equal E_INVIND, get(b, 'contents')
      assert_equal [c], get(l, 'contents')
      assert_equal [], get(c, 'contents')
    end
  end

  def test_move
    run_test_as('wizard') do
      a = create(NOTHING)
      b = create(NOTHING)
      c = create(NOTHING)

      assert_equal NOTHING, get(a, 'location')
      assert_equal NOTHING, get(b, 'location')
      assert_equal NOTHING, get(c, 'location')

      assert_equal [], get(a, 'contents')
      assert_equal [], get(b, 'contents')
      assert_equal [], get(c, 'contents')

      move(b, a)
      move(c, b)

      assert_equal NOTHING, get(a, 'location')
      assert_equal a, get(b, 'location')
      assert_equal b, get(c, 'location')

      assert_equal [b], get(a, 'contents')
      assert_equal [c], get(b, 'contents')
      assert_equal [], get(c, 'contents')
    end
  end

  def test_pass
    run_test_as('wizard') do
      a = kahuna(NOTHING, NOTHING, 'a')
      b = kahuna(a, NOTHING, 'b')
      c = kahuna(b, NOTHING, 'c')

      add_verb(a, ['player', 'xd', 'foo'], ['this', 'none', 'this'])
      set_verb_code(a, 'foo') do |vc|
        vc << %Q|return {"a", @`pass() ! ANY => {}'};|
      end

      assert_equal ['a'], call(a, 'foo')
      assert_equal ['a'], call(b, 'foo')
      assert_equal ['a'], call(c, 'foo')

      add_verb(b, ['player', 'xd', 'foo'], ['this', 'none', 'this'])
      set_verb_code(b, 'foo') do |vc|
        vc << %Q|return {"b", @`pass() ! ANY => {}'};|
      end

      assert_equal ['a'], call(a, 'foo')
      assert_equal ['b', 'a'], call(b, 'foo')
      assert_equal ['b', 'a'], call(c, 'foo')

      add_verb(c, ['player', 'xd', 'foo'], ['this', 'none', 'this'])
      set_verb_code(c, 'foo') do |vc|
        vc << %Q|return {"c", @`pass() ! ANY => {}'};|
      end

      assert_equal ['a'], call(a, 'foo')
      assert_equal ['b', 'a'], call(b, 'foo')
      assert_equal ['c', 'b', 'a'], call(c, 'foo')

      add_verb(c, ['player', 'xd', 'boo'], ['this', 'none', 'this'])
      set_verb_code(c, 'boo') do |vc|
        vc << %Q|return {"c", @pass()};|
      end

      assert_equal E_VERBNF, call(c, 'boo')

      add_verb(a, ['player', 'xd', 'hoo'], ['this', 'none', 'this'])
      set_verb_code(a, 'hoo') do |vc|
        vc << %Q|return {"a", @pass()};|
      end

      assert_equal E_INVIND, call(a, 'hoo')
    end
  end

  def test_verbs_invocation_and_inheritance
    run_test_as('wizard') do
      a = create(NOTHING)
      b = create(a)
      c = create(b)

      add_verb(a, ['player', 'xd', 'foo'], ['this', 'none', 'this'])
      set_verb_code(a, 'foo') do |vc|
        vc << %Q|return "foo";|
      end
      add_verb(a, ['player', 'xd', 'a'], ['this', 'none', 'this'])
      set_verb_code(a, 'a') do |vc|
        vc << %Q|return "a";|
      end

      add_verb(b, ['player', 'xd', 'b'], ['this', 'none', 'this'])
      set_verb_code(b, 'b') do |vc|
        vc << %Q|return "b";|
      end

      add_verb(c, ['player', 'xd', 'c'], ['this', 'none', 'this'])
      set_verb_code(c, 'c') do |vc|
        vc << %Q|return "c";|
      end

      assert_equal 'foo', call(a, 'foo')
      assert_equal 'foo', call(b, 'foo')
      assert_equal 'foo', call(c, 'foo')

      assert_equal 'a', call(a, 'a')
      assert_equal 'b', call(b, 'b')
      assert_equal 'c', call(c, 'c')

      chparent(c, a)
      chparent(b, NOTHING)
      chparent(a, b)

      assert_equal 'foo', call(a, 'foo')
      assert_equal E_VERBNF, call(b, 'foo')
      assert_equal 'foo', call(c, 'foo')

      delete_verb(a, 'foo')

      assert_equal E_VERBNF, call(a, 'foo')
      assert_equal E_VERBNF, call(b, 'foo')
      assert_equal E_VERBNF, call(c, 'foo')

      assert_equal 'a', call(a, 'a')
      assert_equal 'b', call(b, 'b')
      assert_equal 'c', call(c, 'c')

      add_verb(a, ['player', 'xd', 'foo'], ['this', 'none', 'this'])
      set_verb_code(a, 'foo') do |vc|
        vc << %Q|return "foo";|
      end

      add_verb(a, ['player', 'xd', 'foo'], ['this', 'none', 'this'])
      set_verb_code(a, 'foo') do |vc|
        vc << %Q|return verb;|
      end
    end
  end

  def test_properties_and_inheritance
    run_test_as('programmer') do
      a = create(NOTHING)
      b = create(NOTHING)
      c = create(NOTHING)

      m = create(a)
      n = create(a)

      add_property(a, 'a1', 'a1', [player, ''])
      add_property(a, 'a', 'a', [player, ''])
      add_property(b, 'b1', ['b1'], [player, 'r'])
      add_property(b, 'b', ['b'], [player, 'r'])
      add_property(c, 'c', location, [player, 'w'])

      assert_equal [player, ''], property_info(a, 'a')
      assert_equal [player, ''], property_info(a, 'a1')
      assert_equal E_PROPNF, property_info(a, 'b')
      assert_equal E_PROPNF, property_info(a, 'b1')
      assert_equal E_PROPNF, property_info(a, 'c')
      assert_equal [player, ''], property_info(m, 'a1')
      assert_equal [player, ''], property_info(m, 'a')
      assert_equal E_PROPNF, property_info(n, 'b1')
      assert_equal E_PROPNF, property_info(n, 'b')

      assert_equal 'a', get(a, 'a')
      assert_equal 'a1', get(a, 'a1')
      assert_equal E_PROPNF, get(a, 'b')
      assert_equal E_PROPNF, get(a, 'b1')
      assert_equal E_PROPNF, get(a, 'c')
      assert_equal 'a', get(m, 'a')
      assert_equal 'a1', get(m, 'a1')
      assert_equal 'a', get(n, 'a')
      assert_equal 'a1', get(n, 'a1')

      assert_equal 'aa', set(a, 'a', 'aa')
      assert_equal E_PROPNF, set(a, 'b', 'bb')
      assert_equal E_PROPNF, set(a, 'c', 'cc')
      assert_equal '11', set(m, 'a', '11')
      assert_equal 'aa', set(n, 'a', 'aa')

      assert_equal 'aa', get(a, 'a')
      assert_equal E_PROPNF, get(a, 'b')
      assert_equal E_PROPNF, get(a, 'c')
      assert_equal '11', get(m, 'a')
      assert_equal 'aa', get(n, 'a')

      assert_equal 'a', set(a, 'a', 'a')

      assert_equal 'a', get(a, 'a')
      assert_equal E_PROPNF, get(a, 'b')
      assert_equal E_PROPNF, get(a, 'c')
      assert_equal '11', get(m, 'a')
      assert_equal 'aa', get(n, 'a')

      clear_property(m, 'a')
      clear_property(n, 'a')

      assert chparent(a, b)

      assert_equal [player, ''], property_info(a, 'a')
      assert_equal [player, ''], property_info(a, 'a1')
      assert_equal [player, 'r'], property_info(a, 'b')
      assert_equal [player, 'r'], property_info(a, 'b1')
      assert_equal E_PROPNF, property_info(a, 'c')
      assert_equal [player, ''], property_info(m, 'a1')
      assert_equal [player, ''], property_info(m, 'a')
      assert_equal [player, 'r'], property_info(n, 'b1')
      assert_equal [player, 'r'], property_info(n, 'b')

      assert_equal 'a', get(a, 'a')
      assert_equal 'a1', get(a, 'a1')
      assert_equal ['b'], get(a, 'b')
      assert_equal ['b1'], get(a, 'b1')
      assert_equal E_PROPNF, get(a, 'c')
      assert_equal 'a', get(m, 'a')
      assert_equal 'a1', get(m, 'a1')
      assert_equal 'a', get(n, 'a')
      assert_equal 'a1', get(n, 'a1')

      assert chparent(a, c)

      assert_equal [player, ''], property_info(a, 'a')
      assert_equal [player, ''], property_info(a, 'a1')
      assert_equal E_PROPNF, property_info(a, 'b')
      assert_equal E_PROPNF, property_info(a, 'b1')
      assert_equal [player, 'w'], property_info(a, 'c')
      assert_equal [player, ''], property_info(m, 'a1')
      assert_equal [player, ''], property_info(m, 'a')
      assert_equal E_PROPNF, property_info(n, 'b1')
      assert_equal E_PROPNF, property_info(n, 'b')
      assert_equal [player, 'w'], property_info(m, 'c')
      assert_equal [player, 'w'], property_info(n, 'c')

      assert_equal 'a', get(a, 'a')
      assert_equal 'a1', get(a, 'a1')
      assert_equal E_PROPNF, get(a, 'b')
      assert_equal E_PROPNF, get(a, 'b1')
      assert_equal location, get(a, 'c')
      assert_equal 'a', get(m, 'a')
      assert_equal 'a1', get(m, 'a1')
      assert_equal 'a', get(n, 'a')
      assert_equal 'a1', get(n, 'a1')
      assert_equal location, get(m, 'c')
      assert_equal location, get(n, 'c')

      delete_property(c, 'c')
      add_property(c, 'c', 'c', [player, 'c'])

      assert_equal [player, ''], property_info(a, 'a')
      assert_equal [player, ''], property_info(a, 'a1')
      assert_equal E_PROPNF, property_info(a, 'b')
      assert_equal E_PROPNF, property_info(a, 'b1')
      assert_equal [player, 'c'], property_info(a, 'c')
      assert_equal [player, ''], property_info(m, 'a1')
      assert_equal [player, ''], property_info(m, 'a')
      assert_equal E_PROPNF, property_info(n, 'b1')
      assert_equal E_PROPNF, property_info(n, 'b')
      assert_equal [player, 'c'], property_info(m, 'c')
      assert_equal [player, 'c'], property_info(n, 'c')

      assert_equal 'a', get(a, 'a')
      assert_equal 'a1', get(a, 'a1')
      assert_equal E_PROPNF, get(a, 'b')
      assert_equal E_PROPNF, get(a, 'b1')
      assert_equal 'c', get(a, 'c')
      assert_equal 'a', get(m, 'a')
      assert_equal 'a1', get(m, 'a1')
      assert_equal 'a', get(n, 'a')
      assert_equal 'a1', get(n, 'a1')
      assert_equal 'c', get(m, 'c')
      assert_equal 'c', get(n, 'c')

      assert_equal E_PROPNF, delete_property(m, 'a')
      assert_equal E_PROPNF, delete_property(n, 'a')

      delete_property(c, 'c')

      assert_equal 'a', get(a, 'a')
      assert_equal E_PROPNF, get(a, 'b')
      assert_equal E_PROPNF, get(a, 'c')
      assert_equal 'a', get(m, 'a')
      assert_equal 'a', get(n, 'a')
    end
  end

  def test_clear_properties
    x = nil

    run_test_as('programmer') do
      x = create(NOTHING)
      set(x, 'f', 1)

      a = create(x)

      add_property(x, 'x', 'x', [player, 'rc'])
      assert_equal [player, 'rc'], property_info(a, 'x')
      assert_equal 'x', get(a, 'x')

      b = create(NOTHING)
      chparent(b, x)
      assert_equal [player, 'rc'], property_info(b, 'x')
      assert_equal 'x', get(b, 'x')

      set(b, 'x', ['x'])
      assert_equal ['x'], get(b, 'x')

      c = create(x)
      assert_equal [player, 'rc'], property_info(c, 'x')
      assert_equal 'x', get(c, 'x')

      set_property_info(c, 'x', [player, ''])
      assert_equal [player, ''], property_info(c, 'x')
    end

    run_test_as('programmer') do
      a = create(x)
      assert_equal [player, 'rc'], property_info(a, 'x')
      assert_equal 'x', get(a, 'x')

      set(a, 'x', ['x'])
      assert_equal ['x'], get(a, 'x')

      b = create(NOTHING)
      chparent(b, x)
      assert_equal [player, 'rc'], property_info(b, 'x')
      assert_equal 'x', get(b, 'x')

      set_property_info(b, 'x', [player, ''])
      assert_equal [player, ''], property_info(b, 'x')
    end
  end

  def test_parent_children_ancestors_descendants
    run_test_as('programmer') do
      a = create(NOTHING)
      b = create(a)
      c = create(b)
      d = create(c)
      e = create(d)

      assert_equal d, parent(e)
      assert_equal c, parent(d)
      assert_equal b, parent(c)
      assert_equal a, parent(b)
      assert_equal NOTHING, parent(a)

      assert_equal [b], children(a)
      assert_equal [c], children(b)
      assert_equal [d], children(c)
      assert_equal [e], children(d)
      assert_equal [], children(e)

      assert_equal E_RECMOVE, chparent(a, e)
      assert_equal E_RECMOVE, chparent(a, d)
      assert_equal 0, chparent(d, NOTHING)
      assert_equal 0, chparent(a, e)

      assert_equal e, parent(a)
      assert_equal a, parent(b)
      assert_equal b, parent(c)
      assert_equal NOTHING, parent(d)
      assert_equal d, parent(e)

      assert_equal [b], children(a)
      assert_equal [c], children(b)
      assert_equal [], children(c)
      assert_equal [e], children(d)
      assert_equal [a], children(e)

      recycle(a)

      assert_equal E_INVARG, parent(a)
      assert_equal e, parent(b)
      assert_equal b, parent(c)
      assert_equal NOTHING, parent(d)
      assert_equal d, parent(e)

      assert_equal E_INVARG, children(a)
      assert_equal [c], children(b)
      assert_equal [], children(c)
      assert_equal [e], children(d)
      assert_equal [b], children(e)
    end
  end

  def test_various_things_that_can_go_wrong
    run_test_as('programmer') do
      a = create(NOTHING)
      b = create(NOTHING)
      c = create(NOTHING)

      m = create(a)
      n = create(b)

      z = create(NOTHING)

      add_property(a, 'foo', 0, [player, ''])
      add_property(b, 'foo', 0, [player, ''])
      add_property(c, 'foo', 0, [player, ''])

      add_property(m, 'bar', 0, [player, ''])
      add_property(n, 'baz', 0, [player, ''])

      add_property(z, 'bar', 0, [player, ''])

      assert_equal E_INVARG, chparent(a, b)
      assert_equal E_INVARG, chparent(a, c)
      assert_equal E_INVARG, chparent(a, z)

      assert_equal 0, chparent(c, z)
      assert_equal 0, chparent(c, NOTHING)

      assert_equal 0, chparent(z, c)
      assert_equal 0, chparent(z, NOTHING)
    end
  end

  def test_verb_cache
    run_test_as('wizard') do
      vcs = verb_cache_stats
      unless (vcs[0] == 0 && vcs[1] == 0 && vcs[2] == 0 && vcs[3] == 0)
        s = create(NOTHING);
        m = create(s);
        a = create(m)

        add_verb(s, [player, 'xd', 's'], ['this', 'none', 'this'])
        set_verb_code(s, 's') { |vc| vc << %|return "s";| }
        add_verb(m, [player, 'xd', 'm'], ['this', 'none', 'this'])
        set_verb_code(m, 'm') { |vc| vc << %|return "m";| }

        x = create(NOTHING)
        add_verb(x, [player, 'xd', 'x'], ['this', 'none', 'this'])
        set_verb_code(x, 'x') do |vc|
          vc << %|recycle(create($nothing));|
            vc << %|a = verb_cache_stats();|
            vc << %|#{a}:s();|
            vc << %|b = verb_cache_stats();|
            vc << %|#{a}:s();|
            vc << %|c = verb_cache_stats();|
            vc << %|#{a}:s();|
            vc << %|d = verb_cache_stats();|
            vc << %|#{a}:s();|
            vc << %|e = verb_cache_stats();|
            vc << %|return {a, b, c};|
        end

        r = call(x, 'x')
        ra, rb, rc = r

        # a:s()

        assert_equal ra[0], rb[0] # no hit
        assert_equal ra[1], rb[1] # no -hit
        assert_equal ra[2] + 1, rb[2] # miss! (m)

        # a:s()

        assert_equal rb[0] + 1, rc[0] # hit! (m)
        assert_equal rb[1], rc[1] # no -hit
        assert_equal rb[2], rc[2] # no miss

        assert_equal [0, 1, 1], r.map { |z| z[4][1] }
      end
    end
  end

  private

  def kahuna(parent, location, name)
    object = create(parent)
    move(object, location)
    set(object, 'name', name)
    add_property(object, name, name, [player, ''])
    add_verb(object, [player, 'xd', name], ['this', 'none', 'this'])
    set_verb_code(object, name) do |vc|
      vc << %|return this.#{name};|
    end
    object
  end

end
