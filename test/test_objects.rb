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

      # I am changing how the server interprets the `owner' when it is
      # not a valid object reference -- in all cases, make the owner
      # the object itself (if the caller is a wizard).
      if function_info('create')[3] == [1, 1]
        # the old way...
        assert_equal e, get(e, 'owner')
        assert_equal AMBIGUOUS_MATCH, get(f, 'owner')
        assert_equal FAILED_MATCH, get(g, 'owner')
        assert_equal invalid_object, get(h, 'owner')
      else
        # the new way...
        assert_equal e, get(e, 'owner')
        assert_equal f, get(f, 'owner')
        assert_equal g, get(g, 'owner')
        assert_equal h, get(h, 'owner')
      end

      if function_info('create')[3] == [-1, 1]
        i = create([b, c])
        j = create([b, c], i)
        k = create([], NOTHING)
        l = create([])

        assert_equal [b, c], parent(i)
        assert_equal [b, c], parent(j)
        assert_equal NOTHING, parent(k)
        assert_equal NOTHING, parent(l)

        assert_equal [], children(i)
        assert_equal [], children(j)
        assert_equal [], children(l)
        assert_equal [], children(l)

        assert_equal player, get(i, 'owner')
        assert_equal i, get(j, 'owner')
        assert_equal k, get(k, 'owner')
        assert_equal player, get(l, 'owner')
      end
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

      if function_info('create')[3] == [1, 1]
        assert_equal E_PERM, create(AMBIGUOUS_MATCH)
        assert_equal E_PERM, create(FAILED_MATCH)
        assert_equal E_PERM, create(invalid_object)
      else
        assert_equal E_INVARG, create(AMBIGUOUS_MATCH)
        assert_equal E_INVARG, create(FAILED_MATCH)
        assert_equal E_INVARG, create(invalid_object)
      end

      if function_info('create')[3] == [-1, 1]
        assert_equal E_TYPE, create([1])
        assert_equal E_TYPE, create(['1'])
        assert_equal E_INVARG, create([NOTHING])
        assert_equal E_INVARG, create([AMBIGUOUS_MATCH])
        assert_equal E_INVARG, create([FAILED_MATCH])
        assert_equal E_INVARG, create([invalid_object])

        # If two objects define the same property by name, a new
        # object cannot be created using both of them as parents.

        a = create(NOTHING)
        b = create(NOTHING)

        add_property(a, 'foo', 123, [a, ''])
        add_property(b, 'foo', 'abc', [b, ''])

        # this fails
        max = max_object()
        assert_equal E_INVARG, create([a, b])
        assert_equal E_INVARG, create([b, a])
        assert_equal max, max_object()

        # Duplicate objects are not allowed.

        assert_equal E_INVARG, create([a, a])
        assert_equal E_INVARG, create([b, b])
      end
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
      if function_info('create')[3] == [-1, 1]
        assert_equal E_PERM, create([a, b])
        assert_equal E_PERM, create([b, a])
      end

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

      if function_info('chparent')[3] == [1, -1]
        chparent(a, [b, c])
        chparent(b, [c])
        chparent(c, [])

        assert_equal [b, c], parent(a)
        assert_equal c, parent(b)
        assert_equal NOTHING, parent(c)

        assert_equal [], children(a)
        assert_equal [a], children(b)
        assert_equal [a, b], children(c)

        assert_equal [b, c], ancestors(a)
        assert_equal [c], ancestors(b)
        assert_equal [], ancestors(c)

        assert_equal [], descendants(a)
        assert_equal [a], descendants(b)
        assert_equal [a, b], descendants(c)

        chparent(a, [c])

        assert_equal [c], ancestors(a)
        assert_equal [c], ancestors(b)
        assert_equal [], ancestors(c)

        assert_equal [], descendants(a)
        assert_equal [], descendants(b)
        assert_equal [b, a], descendants(c)

        chparent(a, [])
        chparent(b, [])
      end

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

      if function_info('chparent')[3] == [1, -1]
        chparent(c, [m, n, a])
        assert_equal [player, 'c'], property_info(c, 'foo')
        set(c, 'foo', 'bar')
        assert_equal 'bar', get(c, 'foo')
        chparent(c, [b, m, n])
        assert_equal [b, ''], property_info(c, 'foo')
        assert_equal 'foo', get(c, 'foo')
      end

      # If a descendent object defines a property with the same name
      # as one defined either on the new parent or on one of its
      # ancestors, the chparent fails.

      chparent c, NOTHING

      add_property(n, 'foo', 'foo', [n, 'rw'])

      chparent a, m
      assert_equal E_INVARG, chparent(m, n)

      chparent b, m
      assert_equal E_INVARG, chparent(m, n)

      if function_info('chparent')[3] == [1, -1]
        assert_equal E_INVARG, chparent(m, [n])
        assert_equal E_INVARG, chparent(m, [n, c])
        assert_equal E_INVARG, chparent(m, [c, n])
        assert_not_equal E_INVARG, chparent(m, [c])
      end

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

      if function_info('chparent')[3] == [1, -1]
        assert_equal E_INVARG, chparent(:object, [:nothing])
        assert_equal E_INVARG, chparent(:object, [invalid_object])
        assert_equal E_RECMOVE, chparent(:object, [:object])
        assert_equal E_RECMOVE, chparent(c, [a, b])

        # If two objects define the same property by name, a new
        # object cannot be created using both of them as parents.

        d = create(NOTHING)
        e = create(NOTHING)

        add_property(d, 'foo', 123, [d, ''])
        add_property(e, 'foo', 'abc', [e, ''])

        # this fails
        f = create(NOTHING)
        assert_equal E_INVARG, chparent(f, [d, e])

        # Duplicate objects are not allowed.

        assert_equal E_INVARG, chparent(f, [d, d])
        assert_equal E_INVARG, chparent(f, [e, e])
      end
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
      if function_info('chparent')[3] == [1, -1]
        assert_equal E_PERM, chparent(c, [a, b])
        assert_equal E_PERM, chparent(c, [b, a])
      end

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

      if function_info('create')[3] == [-1, 1]
        x = kahuna(NOTHING, NOTHING, 'x')
        y = kahuna(NOTHING, NOTHING, 'y')
        b = kahuna([x, y], NOTHING, 'b')
        m = kahuna(b, b, 'm')
        n = kahuna(b, b, 'n')

        assert_equal NOTHING, parent(x)
        assert_equal NOTHING, parent(y)
        assert_equal [x, y], parent(b)
        assert_equal b, parent(m)
        assert_equal b, parent(n)

        assert_equal [b], children(x)
        assert_equal [b], children(y)
        assert_equal [m, n], children(b)
        assert_equal [], children(m)
        assert_equal [], children(n)

        assert_equal NOTHING, get(x, 'location')
        assert_equal NOTHING, get(y, 'location')
        assert_equal NOTHING, get(b, 'location')
        assert_equal b, get(m, 'location')
        assert_equal b, get(n, 'location')

        assert_equal [], get(x, 'contents')
        assert_equal [], get(y, 'contents')
        assert_equal [m, n], get(b, 'contents')
        assert_equal [], get(m, 'contents')
        assert_equal [], get(n, 'contents')

        recycle(b)

        assert_equal NOTHING, parent(x)
        assert_equal NOTHING, parent(y)
        assert_equal E_INVARG, parent(b)
        assert_equal [x, y], parent(m)
        assert_equal [x, y], parent(n)

        assert_equal [m, n], children(x)
        assert_equal [m, n], children(y)
        assert_equal E_INVARG, children(b)
        assert_equal [], children(m)
        assert_equal [], children(n)

        assert_equal NOTHING, get(x, 'location')
        assert_equal NOTHING, get(y, 'location')
        assert_equal E_INVIND, get(b, 'location')
        assert_equal NOTHING, get(m, 'location')
        assert_equal NOTHING, get(n, 'location')

        assert_equal [], get(x, 'contents')
        assert_equal [], get(y, 'contents')
        assert_equal E_INVIND, get(b, 'contents')
        assert_equal [], get(m, 'contents')
        assert_equal [], get(n, 'contents')

        assert_equal 'x', call(x, 'x')
        assert_equal 'y', call(m, 'y')
        assert_equal 'y', call(n, 'y')
        assert_equal 'm', call(m, 'm')
        assert_equal 'n', call(n, 'n')

        assert_equal 'y', get(y, 'y')
        assert_equal 'x', get(m, 'x')
        assert_equal 'x', get(n, 'x')
        assert_equal 'm', get(m, 'm')
        assert_equal 'n', get(n, 'n')
      end
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

      if function_info('create')[3] == [-1, 1]
        recycle(create(:nothing)) # create a hole

        l = eval(%|for o in [#0..max_object()]; if (!valid(o)); return o; endif; endfor; return $nothing;|)

        a = create(NOTHING)
        b = create(NOTHING)
        c = create(NOTHING)

        d = create([a, b])
        e = create([a, c])

        move(d, a)
        move(e, b)

        assert_equal NOTHING, parent(a)
        assert_equal NOTHING, parent(b)
        assert_equal NOTHING, parent(c)
        assert_equal [a, b], parent(d)
        assert_equal [a, c], parent(e)

        assert_equal [d, e], children(a)
        assert_equal [d], children(b)
        assert_equal [e], children(c)
        assert_equal [], children(d)
        assert_equal [], children(e)

        assert_equal NOTHING, get(a, 'location')
        assert_equal NOTHING, get(b, 'location')
        assert_equal NOTHING, get(c, 'location')
        assert_equal a, get(d, 'location')
        assert_equal b, get(e, 'location')

        assert_equal [d], get(a, 'contents')
        assert_equal [e], get(b, 'contents')
        assert_equal [], get(c, 'contents')
        assert_equal [], get(d, 'contents')
        assert_equal [], get(e, 'contents')

        assert_equal l, renumber(d)

        assert_equal NOTHING, parent(a)
        assert_equal NOTHING, parent(b)
        assert_equal NOTHING, parent(c)
        assert_equal E_INVARG, parent(d)
        assert_equal [a, b], parent(l)
        assert_equal [a, c], parent(e)

        assert_equal [l, e], children(a)
        assert_equal [l], children(b)
        assert_equal [e], children(c)
        assert_equal E_INVARG, children(d)
        assert_equal [], children(l)
        assert_equal [], children(e)

        assert_equal NOTHING, get(a, 'location')
        assert_equal NOTHING, get(b, 'location')
        assert_equal NOTHING, get(c, 'location')
        assert_equal E_INVIND, get(d, 'location')
        assert_equal a, get(l, 'location')
        assert_equal b, get(e, 'location')

        assert_equal [l], get(a, 'contents')
        assert_equal [e], get(b, 'contents')
        assert_equal [], get(c, 'contents')
        assert_equal E_INVIND, get(d, 'contents')
        assert_equal [], get(l, 'contents')
        assert_equal [], get(e, 'contents')
      end
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

      if function_info('chparent')[3] == [1, -1]
        chparent(c, [a, b])

        assert_equal ['a'], call(a, 'foo')
        assert_equal ['b', 'a'], call(b, 'foo')
        assert_equal ['c', 'a'], call(c, 'foo')

        chparent(c, [b, a])

        assert_equal ['a'], call(a, 'foo')
        assert_equal ['b', 'a'], call(b, 'foo')
        assert_equal ['c', 'b', 'a'], call(c, 'foo')

        chparent(b, NOTHING)

        assert_equal ['a'], call(a, 'foo')
        assert_equal ['b'], call(b, 'foo')
        assert_equal ['c', 'b'], call(c, 'foo')

        delete_verb(b, 'foo')

        assert_equal ['a'], call(a, 'foo')
        assert_equal E_VERBNF, call(b, 'foo')
        assert_equal ['c', 'a'], call(c, 'foo')

        delete_verb(a, 'foo')

        assert_equal E_VERBNF, call(a, 'foo')
        assert_equal E_VERBNF, call(b, 'foo')
        assert_equal ['c'], call(c, 'foo')

        assert_equal [b, a], parent(c)

        assert_equal E_VERBNF, call(c, 'goo')
      end
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

      if function_info('chparent')[3] == [1, -1]
        chparent(c, [a, b])

        assert_equal 'foo', call(a, 'foo')
        assert_equal E_VERBNF, call(b, 'foo')
        assert_equal 'foo', call(c, 'foo')

        assert_equal 'a', call(c, 'a')
        assert_equal 'b', call(c, 'b')
        assert_equal 'c', call(c, 'c')

        chparent(c, [b, a])

        assert_equal 'foo', call(a, 'foo')
        assert_equal E_VERBNF, call(b, 'foo')
        assert_equal 'foo', call(c, 'foo')

        assert_equal 'a', call(c, 'a')
        assert_equal 'b', call(c, 'b')
        assert_equal 'c', call(c, 'c')

        delete_verb(a, 'foo')

        assert_equal E_VERBNF, call(a, 'foo')
        assert_equal E_VERBNF, call(b, 'foo')
        assert_equal E_VERBNF, call(c, 'foo')

        assert_equal 'a', call(a, 'a')
        assert_equal 'b', call(b, 'b')
        assert_equal 'c', call(c, 'c')
      end

      add_verb(a, ['player', 'xd', 'foo'], ['this', 'none', 'this'])
      set_verb_code(a, 'foo') do |vc|
        vc << %Q|return verb;|
      end

      # Test a case that challenges the verb cache.  Both M and N
      # inherit from E.  M also inherits from A.  Because E defines
      # `bar' it is the `first_parent_with_verbs' in the list of
      # ancestors for M.  However, with respect to verbs `a' and
      # `foo', it is NOT the _correct_ first parent, because these
      # verbs are defined in the line of ancestors through A.
      #
      #      n
      #       \
      #        e (bar)
      #       /
      #      m
      #       \
      #        a (foo) - b - c
      #

      if function_info('chparent')[3] == [1, -1]
        chparent c, NOTHING
        chparent b, c

        e = create(NOTHING)
        add_verb(e, ['player', 'xd', 'bar'], ['this', 'none', 'this'])
        set_verb_code(e, 'bar') do |vc|
          vc << %Q|return verb;|
        end

        n = create([e])
        m = create([e, a])

        assert_equal 'bar', call(m, 'bar')
        assert_equal 'bar', call(n, 'bar')

        assert_equal 'foo', call(m, 'foo')
        assert_equal E_VERBNF, call(n, 'foo')

        assert_equal E_VERBNF, call(n, 'c')
        assert_equal 'c', call(m, 'c')

        assert_equal 'a', call(m, 'a')
        assert_equal E_VERBNF, call(n, 'a')

        assert_equal E_VERBNF, call(n, 'b')
        assert_equal 'b', call(m, 'b')

        chparent(m, [e, b])

        assert_equal E_VERBNF, call(n, 'c')
        assert_equal 'c', call(m, 'c')

        assert_equal E_VERBNF, call(m, 'a')
        assert_equal E_VERBNF, call(n, 'a')

        assert_equal E_VERBNF, call(n, 'b')
        assert_equal 'b', call(m, 'b')

        chparent(m, [e, c])

        assert_equal E_VERBNF, call(n, 'c')
        assert_equal 'c', call(m, 'c')

        assert_equal E_VERBNF, call(m, 'a')
        assert_equal E_VERBNF, call(n, 'a')

        assert_equal E_VERBNF, call(n, 'b')
        assert_equal E_VERBNF, call(m, 'b')

        chparent(m, [e])

        assert_equal E_VERBNF, call(n, 'c')
        assert_equal E_VERBNF, call(m, 'c')

        assert_equal E_VERBNF, call(m, 'a')
        assert_equal E_VERBNF, call(n, 'a')

        assert_equal E_VERBNF, call(n, 'b')
        assert_equal E_VERBNF, call(m, 'b')
      end
    end
  end

  def test_command_verbs_and_inheritance
    run_test_as('wizard') do
      if function_info('chparent')[3] == [1, -1]
        a = create(NOTHING)
        b = create(a)
        c = create(b)
        d = create([here, c])

        add_verb(a, ['player', 'd', 'baz'], ['none', 'none', 'none'])
        set_verb_code(a, 'baz') do |vc|
          vc << %Q|notify(player, "baz");|
        end
        add_verb(b, ['player', 'd', 'bar'], ['none', 'none', 'none'])
        set_verb_code(b, 'bar') do |vc|
          vc << %Q|notify(player, "bar");|
        end
        add_verb(c, ['player', 'd', 'foo'], ['none', 'none', 'none'])
        set_verb_code(c, 'foo') do |vc|
          vc << %Q|notify(player, "foo");|
        end
        add_verb(d, ['player', 'd', 'qnz'], ['none', 'none', 'none'])
        set_verb_code(d, 'qnz') do |vc|
          vc << %Q|notify(player, "qnz");|
        end

        move(player, d)

        assert_equal 'foo', command(%Q|foo|)
        assert_equal 'bar', command(%Q|bar|)
        assert_equal 'baz', command(%Q|baz|)
        assert_equal 'qnz', command(%Q|qnz|)

        chparent(d, [c, here])

        assert_equal 'foo', command(%Q|foo|)
        assert_equal 'bar', command(%Q|bar|)
        assert_equal 'baz', command(%Q|baz|)
        assert_equal 'qnz', command(%Q|qnz|)
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

      add_property(c, 'c', location, [player, 'w'])

      if function_info('chparent')[3] == [1, -1]
        assert chparent(a, [b, c])

        assert_equal [player, ''], property_info(a, 'a')
        assert_equal [player, ''], property_info(a, 'a1')
        assert_equal [player, 'r'], property_info(a, 'b')
        assert_equal [player, 'r'], property_info(a, 'b1')
        assert_equal [player, 'w'], property_info(a, 'c')
        assert_equal [player, ''], property_info(m, 'a')
        assert_equal [player, ''], property_info(m, 'a1')
        assert_equal [player, 'r'], property_info(m, 'b')
        assert_equal [player, 'r'], property_info(m, 'b1')
        assert_equal [player, 'w'], property_info(m, 'c')
        assert_equal [player, ''], property_info(n, 'a')
        assert_equal [player, ''], property_info(n, 'a1')
        assert_equal [player, 'r'], property_info(n, 'b')
        assert_equal [player, 'r'], property_info(n, 'b1')
        assert_equal [player, 'w'], property_info(n, 'c')

        assert_equal 'a', get(a, 'a')
        assert_equal 'a1', get(a, 'a1')
        assert_equal ['b'], get(a, 'b')
        assert_equal ['b1'], get(a, 'b1')
        assert_equal location, get(a, 'c')
        assert_equal 'a', get(m, 'a')
        assert_equal 'a', get(n, 'a')
        assert_equal 'a1', get(m, 'a1')
        assert_equal 'a1', get(n, 'a1')
        assert_equal ['b'], get(m, 'b')
        assert_equal ['b'], get(n, 'b')
        assert_equal ['b1'], get(m, 'b1')
        assert_equal ['b1'], get(n, 'b1')
        assert_equal location, get(m, 'c')
        assert_equal location, get(n, 'c')

        assert_equal 'aa', set(a, 'a', 'aa')
        assert_equal ['bb'], set(a, 'b', ['bb'])
        assert_equal location, set(a, 'c', location)
        assert_equal '11', set(m, 'a', '11')
        assert_equal 'aa', set(n, 'a', 'aa')
        assert_equal ['22'], set(m, 'b', ['22'])
        assert_equal ['bb'], set(n, 'b',  ['bb'])
        assert_equal location, set(m, 'c', location)
        assert_equal location, set(n, 'c', location)

        assert_equal 'aa', get(a, 'a')
        assert_equal 'a1', get(a, 'a1')
        assert_equal ['bb'], get(a, 'b')
        assert_equal ['b1'], get(a, 'b1')
        assert_equal location, get(a, 'c')
        assert_equal '11', get(m, 'a')
        assert_equal 'aa', get(n, 'a')
        assert_equal 'a1', get(m, 'a1')
        assert_equal 'a1', get(n, 'a1')
        assert_equal ['22'], get(m, 'b')
        assert_equal ['bb'], get(n, 'b')
        assert_equal ['b1'], get(m, 'b1')
        assert_equal ['b1'], get(n, 'b1')
        assert_equal location, get(m, 'c')
        assert_equal location, get(n, 'c')

        assert_equal 'a', set(a, 'a',  'a')

        assert_equal 'a', get(a, 'a')
        assert_equal 'a1', get(a, 'a1')
        assert_equal ['bb'], get(a, 'b')
        assert_equal ['b1'], get(a, 'b1')
        assert_equal location, get(a, 'c')
        assert_equal '11', get(m, 'a')
        assert_equal 'aa', get(n, 'a')
        assert_equal 'a1', get(m, 'a1')
        assert_equal 'a1', get(n, 'a1')
        assert_equal ['22'], get(m, 'b')
        assert_equal ['bb'], get(n, 'b')
        assert_equal ['b1'], get(m, 'b1')
        assert_equal ['b1'], get(n, 'b1')
        assert_equal location, get(m, 'c')
        assert_equal location, get(n, 'c')

        clear_property(m, 'a')
        clear_property(n, 'a')
        clear_property(m, 'b')
        clear_property(n, 'b')

        assert_equal 'a', get(a, 'a')
        assert_equal 'a1', get(a, 'a1')
        assert_equal ['bb'], get(a, 'b')
        assert_equal ['b1'], get(a, 'b1')
        assert_equal location, get(a, 'c')
        assert_equal 'a', get(m, 'a')
        assert_equal 'a', get(n, 'a')
        assert_equal 'a1', get(m, 'a1')
        assert_equal 'a1', get(n, 'a1')
        assert_equal ['bb'], get(m, 'b')
        assert_equal ['bb'], get(n, 'b')
        assert_equal ['b1'], get(m, 'b1')
        assert_equal ['b1'], get(n, 'b1')
        assert_equal location, get(m, 'c')
        assert_equal location, get(n, 'c')

        assert_equal ['b'], set(a, 'b',  ['b'])

        assert chparent(a, [c, b])

        assert_equal [player, ''], property_info(a, 'a')
        assert_equal [player, ''], property_info(a, 'a1')
        assert_equal [player, 'r'], property_info(a, 'b')
        assert_equal [player, 'r'], property_info(a, 'b1')
        assert_equal [player, 'w'], property_info(a, 'c')
        assert_equal [player, ''], property_info(m, 'a')
        assert_equal [player, ''], property_info(m, 'a1')
        assert_equal [player, 'r'], property_info(m, 'b')
        assert_equal [player, 'r'], property_info(m, 'b1')
        assert_equal [player, 'w'], property_info(m, 'c')
        assert_equal [player, ''], property_info(n, 'a')
        assert_equal [player, ''], property_info(n, 'a1')
        assert_equal [player, 'r'], property_info(n, 'b')
        assert_equal [player, 'r'], property_info(n, 'b1')
        assert_equal [player, 'w'], property_info(n, 'c')

        assert_equal 'a', get(a, 'a')
        assert_equal 'a1', get(a, 'a1')
        assert_equal ['b'], get(a, 'b')
        assert_equal ['b1'], get(a, 'b1')
        assert_equal location, get(a, 'c')
        assert_equal 'a', get(m, 'a')
        assert_equal 'a', get(n, 'a')
        assert_equal 'a1', get(m, 'a1')
        assert_equal 'a1', get(n, 'a1')
        assert_equal ['b'], get(m, 'b')
        assert_equal ['b'], get(n, 'b')
        assert_equal ['b1'], get(m, 'b1')
        assert_equal ['b1'], get(n, 'b1')
        assert_equal location, get(m, 'c')
        assert_equal location, get(n, 'c')

        assert_equal E_PROPNF, delete_property(m, 'a')
        assert_equal E_PROPNF, delete_property(n, 'a')

        delete_property(c, 'c')

        assert_equal 'a', get(a, 'a')
        assert_equal 'a1', get(a, 'a1')
        assert_equal ['b'], get(a, 'b')
        assert_equal ['b1'], get(a, 'b1')
        assert_equal E_PROPNF, get(a, 'c')
        assert_equal 'a', get(m, 'a')
        assert_equal 'a', get(n, 'a')
        assert_equal 'a1', get(m, 'a1')
        assert_equal 'a1', get(n, 'a1')
        assert_equal ['b'], get(m, 'b')
        assert_equal ['b'], get(n, 'b')
        assert_equal ['b1'], get(m, 'b1')
        assert_equal ['b1'], get(n, 'b1')
        assert_equal E_PROPNF, get(m, 'c')
        assert_equal E_PROPNF, get(n, 'c')

        r = create(NOTHING)

        add_property(r, 'rrrr', 'rrrr', [player, ''])
        add_property(r, 'rrr', 'rrr', [player, 'r'])
        add_property(r, 'rr', 'rr', [player, 'w'])

        assert chparent(a, [b, r, c])

        assert_equal [player, ''], property_info(a, 'a')
        assert_equal [player, ''], property_info(a, 'a1')
        assert_equal [player, 'r'], property_info(a, 'b')
        assert_equal [player, 'r'], property_info(a, 'b1')
        assert_equal [player, 'w'], property_info(a, 'rr')
        assert_equal [player, 'r'], property_info(a, 'rrr')
        assert_equal [player, ''], property_info(a, 'rrrr')
        assert_equal E_PROPNF, property_info(a, 'c')
        assert_equal [player, ''], property_info(m, 'a')
        assert_equal [player, ''], property_info(m, 'a1')
        assert_equal [player, 'r'], property_info(m, 'b')
        assert_equal [player, 'r'], property_info(m, 'b1')
        assert_equal [player, 'w'], property_info(m, 'rr')
        assert_equal [player, 'r'], property_info(m, 'rrr')
        assert_equal [player, ''], property_info(m, 'rrrr')
        assert_equal E_PROPNF, property_info(m, 'c')
        assert_equal [player, ''], property_info(n, 'a')
        assert_equal [player, ''], property_info(n, 'a1')
        assert_equal [player, 'r'], property_info(n, 'b')
        assert_equal [player, 'r'], property_info(n, 'b1')
        assert_equal [player, 'w'], property_info(n, 'rr')
        assert_equal [player, 'r'], property_info(n, 'rrr')
        assert_equal [player, ''], property_info(n, 'rrrr')
        assert_equal E_PROPNF, property_info(n, 'c')

        assert_equal 'a', get(a, 'a')
        assert_equal 'a1', get(a, 'a1')
        assert_equal ['b'], get(a, 'b')
        assert_equal ['b1'], get(a, 'b1')
        assert_equal 'rr', get(a, 'rr')
        assert_equal 'rrr', get(a, 'rrr')
        assert_equal 'rrrr', get(a, 'rrrr')
        assert_equal E_PROPNF, get(a, 'c')
        assert_equal 'a', get(m, 'a')
        assert_equal 'a', get(n, 'a')
        assert_equal 'a1', get(m, 'a1')
        assert_equal 'a1', get(n, 'a1')
        assert_equal ['b'], get(m, 'b')
        assert_equal ['b'], get(n, 'b')
        assert_equal ['b1'], get(m, 'b1')
        assert_equal ['b1'], get(n, 'b1')
        assert_equal 'rr', get(m, 'rr')
        assert_equal 'rrr', get(m, 'rrr')
        assert_equal 'rrrr', get(m, 'rrrr')
        assert_equal 'rr', get(n, 'rr')
        assert_equal 'rrr', get(n, 'rrr')
        assert_equal 'rrrr', get(n, 'rrrr')
        assert_equal E_PROPNF, get(m, 'c')
        assert_equal E_PROPNF, get(n, 'c')

        delete_property(r, 'rrr')

        assert_equal [player, ''], property_info(a, 'a')
        assert_equal [player, ''], property_info(a, 'a1')
        assert_equal [player, 'r'], property_info(a, 'b')
        assert_equal [player, 'r'], property_info(a, 'b1')
        assert_equal [player, 'w'], property_info(a, 'rr')
        assert_equal E_PROPNF, property_info(a, 'rrr')
        assert_equal [player, ''], property_info(a, 'rrrr')
        assert_equal E_PROPNF, property_info(a, 'c')
        assert_equal [player, ''], property_info(m, 'a')
        assert_equal [player, ''], property_info(m, 'a1')
        assert_equal [player, 'r'], property_info(m, 'b')
        assert_equal [player, 'r'], property_info(m, 'b1')
        assert_equal [player, 'w'], property_info(m, 'rr')
        assert_equal E_PROPNF, property_info(m, 'rrr')
        assert_equal [player, ''], property_info(m, 'rrrr')
        assert_equal E_PROPNF, property_info(m, 'c')
        assert_equal [player, ''], property_info(n, 'a')
        assert_equal [player, ''], property_info(n, 'a1')
        assert_equal [player, 'r'], property_info(n, 'b')
        assert_equal [player, 'r'], property_info(n, 'b1')
        assert_equal [player, 'w'], property_info(n, 'rr')
        assert_equal E_PROPNF, property_info(n, 'rrr')
        assert_equal [player, ''], property_info(n, 'rrrr')
        assert_equal E_PROPNF, property_info(n, 'c')

        assert_equal 'a', get(a, 'a')
        assert_equal 'a1', get(a, 'a1')
        assert_equal ['b'], get(a, 'b')
        assert_equal ['b1'], get(a, 'b1')
        assert_equal 'rr', get(a, 'rr')
        assert_equal E_PROPNF, get(a, 'rrr')
        assert_equal 'rrrr', get(a, 'rrrr')
        assert_equal E_PROPNF, get(a, 'c')
        assert_equal 'a', get(m, 'a')
        assert_equal 'a', get(n, 'a')
        assert_equal 'a1', get(m, 'a1')
        assert_equal 'a1', get(n, 'a1')
        assert_equal ['b'], get(m, 'b')
        assert_equal ['b'], get(n, 'b')
        assert_equal ['b1'], get(m, 'b1')
        assert_equal ['b1'], get(n, 'b1')
        assert_equal 'rr', get(m, 'rr')
        assert_equal E_PROPNF, get(m, 'rrr')
        assert_equal 'rrrr', get(m, 'rrrr')
        assert_equal 'rr', get(n, 'rr')
        assert_equal E_PROPNF, get(n, 'rrr')
        assert_equal 'rrrr', get(n, 'rrrr')
        assert_equal E_PROPNF, get(m, 'c')
        assert_equal E_PROPNF, get(n, 'c')

        assert chparent(a, [c, b])

        assert_equal [player, ''], property_info(a, 'a')
        assert_equal [player, ''], property_info(a, 'a1')
        assert_equal [player, 'r'], property_info(a, 'b')
        assert_equal [player, 'r'], property_info(a, 'b1')
        assert_equal E_PROPNF, property_info(a, 'rr')
        assert_equal E_PROPNF, property_info(a, 'rrr')
        assert_equal E_PROPNF, property_info(a, 'rrrr')
        assert_equal E_PROPNF, property_info(a, 'c')
        assert_equal [player, ''], property_info(m, 'a')
        assert_equal [player, ''], property_info(m, 'a1')
        assert_equal [player, 'r'], property_info(m, 'b')
        assert_equal [player, 'r'], property_info(m, 'b1')
        assert_equal E_PROPNF, property_info(m, 'rr')
        assert_equal E_PROPNF, property_info(m, 'rrr')
        assert_equal E_PROPNF, property_info(m, 'rrrr')
        assert_equal E_PROPNF, property_info(m, 'c')
        assert_equal [player, ''], property_info(n, 'a')
        assert_equal [player, ''], property_info(n, 'a1')
        assert_equal [player, 'r'], property_info(n, 'b')
        assert_equal [player, 'r'], property_info(n, 'b1')
        assert_equal E_PROPNF, property_info(n, 'rr')
        assert_equal E_PROPNF, property_info(n, 'rrr')
        assert_equal E_PROPNF, property_info(n, 'rrrr')
        assert_equal E_PROPNF, property_info(n, 'c')

        assert_equal 'a', get(a, 'a')
        assert_equal 'a1', get(a, 'a1')
        assert_equal ['b'], get(a, 'b')
        assert_equal ['b1'], get(a, 'b1')
        assert_equal E_PROPNF, get(a, 'rr')
        assert_equal E_PROPNF, get(a, 'rrr')
        assert_equal E_PROPNF, get(a, 'rrrr')
        assert_equal E_PROPNF, get(a, 'c')
        assert_equal 'a', get(m, 'a')
        assert_equal 'a', get(n, 'a')
        assert_equal 'a1', get(m, 'a1')
        assert_equal 'a1', get(n, 'a1')
        assert_equal ['b'], get(m, 'b')
        assert_equal ['b'], get(n, 'b')
        assert_equal ['b1'], get(m, 'b1')
        assert_equal ['b1'], get(n, 'b1')
        assert_equal E_PROPNF, get(m, 'rr')
        assert_equal E_PROPNF, get(m, 'rrr')
        assert_equal E_PROPNF, get(m, 'rrrr')
        assert_equal E_PROPNF, get(n, 'rr')
        assert_equal E_PROPNF, get(n, 'rrr')
        assert_equal E_PROPNF, get(n, 'rrrr')
        assert_equal E_PROPNF, get(m, 'c')
        assert_equal E_PROPNF, get(n, 'c')
      end
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

      if has_function?('ancestors')
        assert_equal [], ancestors(a)
        assert_equal [a], ancestors(b)
        assert_equal [b, a], ancestors(c)
        assert_equal [c, b, a], ancestors(d)
        assert_equal [d, c, b, a], ancestors(e)
      end

      assert_equal [b], children(a)
      assert_equal [c], children(b)
      assert_equal [d], children(c)
      assert_equal [e], children(d)
      assert_equal [], children(e)

      if has_function?('descendants')
        assert_equal [b, c, d, e], descendants(a)
        assert_equal [c, d, e], descendants(b)
        assert_equal [d, e], descendants(c)
        assert_equal [e], descendants(d)
        assert_equal [], descendants(e)
      end

      assert_equal E_RECMOVE, chparent(a, e)
      assert_equal E_RECMOVE, chparent(a, d)
      assert_equal 0, chparent(d, NOTHING)
      assert_equal 0, chparent(a, e)

      assert_equal e, parent(a)
      assert_equal a, parent(b)
      assert_equal b, parent(c)
      assert_equal NOTHING, parent(d)
      assert_equal d, parent(e)

      if has_function?('ancestors')
        assert_equal [e, d], ancestors(a)
        assert_equal [a, e, d], ancestors(b)
        assert_equal [b, a, e, d], ancestors(c)
        assert_equal [], ancestors(d)
        assert_equal [d], ancestors(e)
      end

      assert_equal [b], children(a)
      assert_equal [c], children(b)
      assert_equal [], children(c)
      assert_equal [e], children(d)
      assert_equal [a], children(e)

      if has_function?('descendants')
        assert_equal [b, c], descendants(a)
        assert_equal [c], descendants(b)
        assert_equal [], descendants(c)
        assert_equal [e, a, b, c], descendants(d)
        assert_equal [a, b, c], descendants(e)
      end

      recycle(a)

      assert_equal E_INVARG, parent(a)
      assert_equal e, parent(b)
      assert_equal b, parent(c)
      assert_equal NOTHING, parent(d)
      assert_equal d, parent(e)

      if has_function?('ancestors')
        assert_equal E_INVARG, ancestors(a)
        assert_equal [e, d], ancestors(b)
        assert_equal [b, e, d], ancestors(c)
        assert_equal [], ancestors(d)
        assert_equal [d], ancestors(e)
      end

      assert_equal E_INVARG, children(a)
      assert_equal [c], children(b)
      assert_equal [], children(c)
      assert_equal [e], children(d)
      assert_equal [b], children(e)

      if has_function?('descendants')
        assert_equal E_INVARG, descendants(a)
        assert_equal [c], descendants(b)
        assert_equal [], descendants(c)
        assert_equal [e, b, c], descendants(d)
        assert_equal [b, c], descendants(e)
      end
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

      if function_info('chparent')[3] == [1, -1]
        assert_equal 0, chparent(c, [z])
        assert_equal 0, chparent(c, [])

        assert_equal 0, chparent(z, [c])
        assert_equal 0, chparent(z, [])

        assert_equal E_INVARG, chparent(c, [a, z])
        assert_equal E_INVARG, chparent(c, [b, z])
        assert_equal E_INVARG, chparent(c, [m, z])
        assert_equal E_INVARG, chparent(c, [n, z])

        assert_equal E_INVARG, chparent(z, [a, b])
        assert_equal E_INVARG, chparent(z, [b, c])
        assert_equal E_INVARG, chparent(z, [m, n])
      end
    end
  end

  def test_ways_of_specifying_nothing
    run_test_as('programmer') do
      if function_info('chparent')[3] == [1, -1]
        a = create([])
        b = create(NOTHING)
        assert_equal E_INVARG, create([NOTHING])

        m = create(a)
        n = create([b])

        assert_equal NOTHING, parent(a)
        assert_equal NOTHING, parent(b)
        assert_equal a, parent(m)
        assert_equal b, parent(n)

        x = kahuna(NOTHING, NOTHING, 'x')
        recycle(x)

        assert_equal E_INVARG, create(x)
        assert_equal E_INVARG, create([x])
      end
    end
  end

  def test_verb_cache
    run_test_as('wizard') do
      vcs = verb_cache_stats
      unless (vcs[0] == 0 && vcs[1] == 0 && vcs[2] == 0 && vcs[3] == 0)
        s = create(NOTHING);
        t = create(NOTHING);
        m = create(s);
        n = create(t);
        a = create([m, n])

        add_verb(s, [player, 'xd', 's'], ['this', 'none', 'this'])
        set_verb_code(s, 's') { |vc| vc << %|return "s";| }
        add_verb(t, [player, 'xd', 't'], ['this', 'none', 'this'])
        set_verb_code(t, 't') { |vc| vc << %|return "t";| }
        add_verb(m, [player, 'xd', 'm'], ['this', 'none', 'this'])
        set_verb_code(m, 'm') { |vc| vc << %|return "m";| }
        add_verb(n, [player, 'xd', 'n'], ['this', 'none', 'this'])
        set_verb_code(n, 'n') { |vc| vc << %|return "n";| }

        x = create(NOTHING)
        add_verb(x, [player, 'xd', 'x'], ['this', 'none', 'this'])
        set_verb_code(x, 'x') do |vc|
          vc << %|recycle(create($nothing));|
            vc << %|a = verb_cache_stats();|
            vc << %|#{a}:s();|
            vc << %|b = verb_cache_stats();|
            vc << %|#{a}:s();|
            vc << %|c = verb_cache_stats();|
            vc << %|#{a}:t();|
            vc << %|d = verb_cache_stats();|
            vc << %|#{a}:t();|
            vc << %|e = verb_cache_stats();|
            vc << %|return {a, b, c, d, e};|
        end

        r = call(x, 'x')
        ra, rb, rc, rd, re = r

        # a:s()

        assert_equal ra[0], rb[0] # no hit
        assert_equal ra[1], rb[1] # no -hit
        assert_equal ra[2] + 1, rb[2] # miss! (m)

        # a:s()

        assert_equal rb[0] + 1, rc[0] # hit! (m)
        assert_equal rb[1], rc[1] # no -hit
        assert_equal rb[2], rc[2] # no miss

        # a:t()

        assert_equal rc[0], rd[0] # no hit
        assert_equal rc[1], rd[1] # no -hit!
        assert_equal rc[2] + 2, rd[2] # miss! (on m and n)

        # a:t()

        assert_equal rd[0] + 1, re[0] # hit! (n)
        assert_equal rd[1] + 1, re[1] # -hit! (m)
        assert_equal rd[2], re[2] # no miss

        assert_equal [0, 1, 1, 3, 3], r.map { |z| z[4][1] }
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
