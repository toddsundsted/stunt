require 'test_helper'

class TestIndexAndRangeExtensions < Test::Unit::TestCase

  def test_that_caret_works_as_an_index
    run_test_as('programmer') do
      assert_equal 1, eval(%|return {1, 2, 3, 4, 5, 6, 7}[^];|)
      assert_equal 2, eval(%|return {1, 2, 3, 4, 5, 6, 7}[^ + 1];|)
      assert_equal E_RANGE, eval(%|return {}[^];|)
      assert_equal 'f', eval(%|return "foobar"[^];|)
      assert_equal 'o', eval(%|return "foobar"[^ + 1];|)
      assert_equal E_RANGE, eval(%|return ""[^];|)
    end
  end

  def test_that_dollar_works_as_an_index
    run_test_as('programmer') do
      assert_equal 7, eval(%|return {1, 2, 3, 4, 5, 6, 7}[$];|)
      assert_equal 6, eval(%|return {1, 2, 3, 4, 5, 6, 7}[$ - 1];|)
      assert_equal E_RANGE, eval(%|return {}[$];|)
      assert_equal 'r', eval(%|return "foobar"[$];|)
      assert_equal 'a', eval(%|return "foobar"[$ - 1];|)
      assert_equal E_RANGE, eval(%|return ""[$];|)
    end
  end

  def test_that_ranges_work
    run_test_as('programmer') do
      assert_equal [1, 2, 3, 4, 5, 6, 7], eval(%|return {1, 2, 3, 4, 5, 6, 7}[^..$];|)
      assert_equal [2, 3, 4, 5, 6], eval(%|return {1, 2, 3, 4, 5, 6, 7}[^ + 1 .. $ - 1];|)
      assert_equal [], eval(%|return {1, 2, 3, 4, 5, 6, 7}[$..^];|)
      assert_equal 'foobar', eval(%|return "foobar"[^..$];|)
      assert_equal 'ooba', eval(%|return "foobar"[^ + 1 .. $ - 1];|)
      assert_equal '', eval(%|return "foobar"[$..^];|)
    end
  end

  def test_that_decompile_works
    run_test_as('programmer') do
      o = create(:nothing)
      add_verb(o, [player, 'xd', 'foobar'], ['this', 'none', 'this'])
      set_verb_code(o, 'foobar') do |vc|
        vc << 'return "foobar"[^ + 2 ^ 2 .. $ - #0.off];'
      end
      vc = simplify command %|; return verb_code(#{o}, "foobar");|
      assert_equal ['return "foobar"[^ + 2 ^ 2..$ - $off];'], vc
    end
  end

  def test_that_disassemble_works
    run_test_as('programmer') do
      o = create(:nothing)
      add_verb(o, [player, 'xd', 'foobar'], ['this', 'none', 'this'])
      set_verb_code(o, 'foobar') do |vc|
        vc << 'return "foobar"[^..$];'
      end
      vc = simplify command %|; return disassemble(#{o}, "foobar");|
      assert vc[8] =~ /FIRST/
      assert vc[9] =~ /LAST/
    end
  end

  def test_that_exponentiation_has_higher_precedence
    run_test_as('programmer') do
      assert_equal [1, 2, 3, 4], eval(%|return {1, 2, 3, 4, 5, 6, 7}[^ .. 2 ^ 2];|)
      assert_equal 5, eval(%|return {1, 2, 3, 4, 5, 6, 7}[^ + 2 ^ 2];|)
      assert_equal 4, eval(%|return {1, 2, 3, 4, 5, 6, 7}[2 ^ 2];|)
    end
  end

end
