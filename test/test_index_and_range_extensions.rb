require 'test_helper'

class TestIndexAndRangeExtensions < Test::Unit::TestCase

  def test_that_caret_works_as_an_index
    run_test_as('programmer') do
      assert_equal 1, eval(%|return [1 -> 1, 2 -> 2, 3 -> 3, 4 -> 4, 5 -> 5, 6 -> 6, 7 -> 7][^];|)
      assert_equal 2, eval(%|return [1 -> 1, 2 -> 2, 3 -> 3, 4 -> 4, 5 -> 5, 6 -> 6, 7 -> 7][^ + 1];|)
      assert_equal E_RANGE, eval(%|return [][^];|)

      assert_equal({'a' => 'foobar', 'b' => 2, 'c' => 3}, eval(%|t = ["a" -> 1, "b" -> 2, "c" -> 3]; t[^] = "foobar"; return t;|))
      assert_equal({'a' => 1, 'a*' => 'foobar', 'b' => 2, 'c' => 3}, eval(%|t = ["a" -> 1, "b" -> 2, "c" -> 3]; t[^ + "*"] = "foobar"; return t;|))

      assert_equal 1, eval(%|return {1, 2, 3, 4, 5, 6, 7}[^];|)
      assert_equal 2, eval(%|return {1, 2, 3, 4, 5, 6, 7}[^ + 1];|)
      assert_equal E_RANGE, eval(%|return {}[^];|)

      assert_equal ['foobar', 2, 3, 4, 5, 6, 7], eval(%|t = {1, 2, 3, 4, 5, 6, 7}; t[^] = "foobar"; return t;|)
      assert_equal [1, 'foobar', 3, 4, 5, 6, 7], eval(%|t = {1, 2, 3, 4, 5, 6, 7}; t[^ + 1] = "foobar"; return t;|)

      assert_equal 'f', eval(%|return "foobar"[^];|)
      assert_equal 'o', eval(%|return "foobar"[^ + 1];|)
      assert_equal E_RANGE, eval(%|return ""[^];|)

      assert_equal 'xoobar', eval(%|t = "foobar"; t[^] = "x"; return t;|)
      assert_equal 'fxobar', eval(%|t = "foobar"; t[^ + 1] = "x"; return t;|)
    end
  end

  def test_that_dollar_works_as_an_index
    run_test_as('programmer') do
      assert_equal 7, eval(%|return [1 -> 1, 2 -> 2, 3 -> 3, 4 -> 4, 5 -> 5, 6 -> 6, 7 -> 7][$];|)
      assert_equal 6, eval(%|return [1 -> 1, 2 -> 2, 3 -> 3, 4 -> 4, 5 -> 5, 6 -> 6, 7 -> 7][$ - 1];|)
      assert_equal E_RANGE, eval(%|return [][$];|)

      assert_equal({'a' => 1, 'b' => 2, 'c' => 'foobar'}, eval(%|t = ["a" -> 1, "b" -> 2, "c" -> 3]; t[$] = "foobar"; return t;|))
      assert_equal({'a' => 1, 'b' => 2, 'c' => 3, 'c*' => 'foobar'}, eval(%|t = ["a" -> 1, "b" -> 2, "c" -> 3]; t[$ + "*"] = "foobar"; return t;|))

      assert_equal 7, eval(%|return {1, 2, 3, 4, 5, 6, 7}[$];|)
      assert_equal 6, eval(%|return {1, 2, 3, 4, 5, 6, 7}[$ - 1];|)
      assert_equal E_RANGE, eval(%|return {}[$];|)

      assert_equal [1, 2, 3, 4, 5, 6, 'foobar'], eval(%|t = {1, 2, 3, 4, 5, 6, 7}; t[$] = "foobar"; return t;|)
      assert_equal [1, 2, 3, 4, 5, 'foobar', 7], eval(%|t = {1, 2, 3, 4, 5, 6, 7}; t[$ - 1] = "foobar"; return t;|)

      assert_equal 'r', eval(%|return "foobar"[$];|)
      assert_equal 'a', eval(%|return "foobar"[$ - 1];|)
      assert_equal E_RANGE, eval(%|return ""[$];|)

      assert_equal 'foobax', eval(%|t = "foobar"; t[$] = "x"; return t;|)
      assert_equal 'foobxr', eval(%|t = "foobar"; t[$ - 1] = "x"; return t;|)
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
