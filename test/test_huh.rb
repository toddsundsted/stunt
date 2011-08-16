require 'test_helper'

class TestHuh < Test::Unit::TestCase

  def test_that_huh_works_on_players
    run_test_with_prefix_and_suffix_as('programmer') do
      add_verb(player, [player, 'xd', 'huh'], ['this', 'none', 'this'])
      set_verb_code(player, 'huh') do |vc|
        vc << %Q|notify(player, "huh");|
      end
      move(player, :nothing)
      assert_equal 'huh', command(%Q|zip|)
    end
  end

  def test_that_huh_does_not_work_on_locations
    run_test_with_prefix_and_suffix_as('programmer') do
      o = create(:nothing)
      add_verb(o, [player, 'xd', 'accept'], ['this', 'none', 'this'])
      set_verb_code(o, 'accept') do |vc|
        vc << %Q|return 1;|
      end
      add_verb(o, [player, 'xd', 'huh'], ['this', 'none', 'this'])
      set_verb_code(o, 'huh') do |vc|
        vc << %Q|notify(player, "huh");|
      end
      move(player, o)
      assert_not_equal 'huh', command(%Q|zip|)
    end
  end

end
