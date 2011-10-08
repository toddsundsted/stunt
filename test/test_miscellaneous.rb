require 'test_helper'

class TestMiscellaneous < Test::Unit::TestCase

  def test_that_isa_does_not_crash_the_server
    run_test_as('programmer') do
      result = simplify command %Q|; return isa(#-2, #1); |
      assert_equal E_INVARG, result
    end
  end

end
