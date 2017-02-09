require 'test_helper'

class TestServerVersion < Test::Unit::TestCase

  def test_that_server_version_takes_at_most_one_argument
    run_test_as('programmer') do
      assert_equal E_ARGS, simplify(command(%Q|; server_version(1, 2); |))
    end
  end

  def test_that_server_version_without_arguments_returns_the_version_string
    run_test_as('programmer') do
      assert_match /^([0-9]+\.[0-9]+\.[0-9]+)(\+[a-z]+)?$/, server_version()
    end
  end

  def test_that_server_version_with_an_argument_returns_version_details
    run_test_as('programmer') do
      assert_equal ['major', 'minor', 'release', 'ext', 'string', 'features', 'options', 'source'], server_version(1).map { |k, v| k }
    end
  end

end
