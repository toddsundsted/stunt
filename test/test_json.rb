require 'test_helper'

class TestJson < Test::Unit::TestCase

  def test_that_generating_json_works_for_simple_values_in_default_mode
    run_test_as('wizard') do
      assert_equal '1', generate_json(1)
      assert_equal '1.1', generate_json(1.1)
      assert_equal '"1.2"', generate_json("1.2")
      assert_equal '"#13"', generate_json(MooObj.new(13))
      assert_equal '"E_PERM"', generate_json(E_PERM)
      assert_equal '[]', generate_json([])
      assert_equal '{}', generate_json({})
    end
  end

  def test_that_parsing_json_works_for_simple_values_in_default_mode
    run_test_as('wizard') do
      assert_equal 1, parse_json('1')
      assert_equal 1.1, parse_json('1.1')
      assert_equal "1.2", parse_json('\"1.2\"')
      assert_equal [], parse_json('[]')
      assert_equal ({}, parse_json('{}'))
    end
  end

  def test_that_parsing_json_fails_for_obvious_errors_in_default_mode
    run_test_as('wizard') do
      assert_equal E_INVARG, parse_json('#13')
      assert_equal E_INVARG, parse_json('E_PERM')
    end
  end

  def test_that_generating_json_works_for_simple_values_in_common_subset_mode
    run_test_as('wizard') do
      assert_equal '1', generate_json(1, 'common-subset')
      assert_equal '1.1', generate_json(1.1, 'common-subset')
      assert_equal '"1.2"', generate_json("1.2", 'common-subset')
      assert_equal '"#13"', generate_json(MooObj.new(13), 'common-subset')
      assert_equal '"E_PERM"', generate_json(E_PERM, 'common-subset')
      assert_equal '[]', generate_json([], 'common-subset')
      assert_equal '{}', generate_json({}, 'common-subset')
    end
  end

  def test_that_parsing_json_works_for_simple_values_in_common_subset_mode
    run_test_as('wizard') do
      assert_equal 1, parse_json('1', 'common-subset')
      assert_equal 1.1, parse_json('1.1', 'common-subset')
      assert_equal "1.2", parse_json('\"1.2\"', 'common-subset')
      assert_equal [], parse_json('[]', 'common-subset')
      assert_equal ({}, parse_json('{}'), 'common-subset')
    end
  end

  def test_that_parsing_json_fails_for_obvious_errors_in_common_subset_mode
    run_test_as('wizard') do
      assert_equal E_INVARG, parse_json('#13', 'common-subset')
      assert_equal E_INVARG, parse_json('E_PERM', 'common-subset')
    end
  end

  def test_that_generating_json_works_for_simple_values_in_embedded_types_mode
    run_test_as('wizard') do
      assert_equal '1', generate_json(1, 'embedded-types')
      assert_equal '1.1', generate_json(1.1, 'embedded-types')
      assert_equal '"1.2"', generate_json("1.2", 'embedded-types')
      assert_equal '"#13|*o*"', generate_json(MooObj.new(13), 'embedded-types')
      assert_equal '"E_PERM|*e*"', generate_json(E_PERM, 'embedded-types')
      assert_equal '[]', generate_json([], 'embedded-types')
      assert_equal '{}', generate_json({}, 'embedded-types')
    end
  end

  def test_that_parsing_json_works_for_simple_values_in_embedded_types_mode
    run_test_as('wizard') do
      assert_equal 1, parse_json('1', 'embedded-types')
      assert_equal 1.1, parse_json('1.1', 'embedded-types')
      assert_equal "1.2", parse_json('\"1.2\"', 'embedded-types')
      assert_equal [], parse_json('[]', 'embedded-types')
      assert_equal ({}, parse_json('{}'), 'embedded-types')
    end
  end

  def test_that_parsing_json_fails_for_obvious_errors_in_embedded_types_mode
    run_test_as('wizard') do
      assert_equal E_INVARG, parse_json('#13', 'embedded-types')
      assert_equal E_INVARG, parse_json('E_PERM', 'embedded-types')
    end
  end

  def test_that_parsing_json_works_for_simple_values_with_type_info_in_embedded_types_mode
    run_test_as('wizard') do
      assert_equal 1, parse_json('\"1|*i*\"', 'embedded-types')
      assert_equal 1.1, parse_json('\"1.1|*f*\"', 'embedded-types')
      assert_equal "1.2", parse_json('\"1.2|*s*\"', 'embedded-types')
      assert_equal MooObj.new("#13"), parse_json('\"#13|*o*\"', 'embedded-types')
      assert_equal E_PERM, parse_json('\"E_PERM|*e*\"', 'embedded-types')
    end
  end

  def test_that_parsing_json_with_type_info_does_not_work_in_common_subset_mode
    run_test_as('wizard') do
      assert_equal "1|*i*", parse_json('\"1|*i*\"', 'common-subset')
      assert_equal "1.1|*f*", parse_json('\"1.1|*f*\"', 'common-subset')
      assert_equal "1.2|*s*", parse_json('\"1.2|*s*\"', 'common-subset')
      assert_equal "#13|*o*", parse_json('\"#13|*o*\"', 'common-subset')
      assert_equal "E_PERM|*e*", parse_json('\"E_PERM|*e*\"', 'common-subset')
    end
  end

  def test_that_generating_json_works_for_complex_values_in_default_mode
    run_test_as('wizard') do
      assert_equal '[1,1.1,"1.2","#13","E_ARGS",[2,2.2,"#-1","foo"],{"E_PERM":"E_PERM"}]', generate_json([1, 1.1, "1.2", MooObj.new(13), E_ARGS, [2, 2.2, :nothing, "foo"], {E_PERM => E_PERM}])
      assert_equal '{"11":11,"33.3":33.3,"#13":[1,2,3,"E_QUOTA"],"foo":"bar"}', generate_json({11 => 11, 33.3 => 33.3, MooObj.new(13) => [1, 2, 3, E_QUOTA], "foo" => "bar"})
    end
  end

  def test_that_generating_json_works_for_complex_values_in_common_subset_mode
    run_test_as('wizard') do
      assert_equal '[1,1.1,"1.2","#13","E_ARGS",[2,2.2,"#-1","foo"],{"E_PERM":"E_PERM"}]', generate_json([1, 1.1, "1.2", MooObj.new(13), E_ARGS, [2, 2.2, :nothing, "foo"], {E_PERM => E_PERM}], 'common-subset')
      assert_equal '{"11":11,"33.3":33.3,"#13":[1,2,3,"E_QUOTA"],"foo":"bar"}', generate_json({11 => 11, 33.3 => 33.3, MooObj.new(13) => [1, 2, 3, E_QUOTA], "foo" => "bar"}, 'common-subset')
    end
  end

  def test_that_generating_json_works_for_complex_values_in_embedded_types_mode
    run_test_as('wizard') do
      assert_equal '[1,1.1,"1.2","#13|*o*","E_ARGS|*e*",[2,2.2,"#-1|*o*","foo"],{"E_PERM|*e*":"E_PERM|*e*"}]', generate_json([1, 1.1, "1.2", MooObj.new(13), E_ARGS, [2, 2.2, :nothing, "foo"], {E_PERM => E_PERM}], 'embedded-types')
      assert_equal '{"11|*i*":11,"33.3|*f*":33.3,"#13|*o*":[1,2,3,"E_QUOTA|*e*"],"foo":"bar"}', generate_json({11 => 11, 33.3 => 33.3, MooObj.new(13) => [1, 2, 3, E_QUOTA], "foo" => "bar"}, 'embedded-types')
    end
  end

  def test_that_parsing_json_works_for_complex_values_in_default_mode
    run_test_as('wizard') do
      assert_equal [1, 1.1, "1.2", "#13", "E_ARGS", [2, 2.2, "foo"], {"1" => "1"}], parse_json('[1, 1.1, \"1.2\", \"#13\", \"E_ARGS\", [2, 2.2, \"foo\"], {\"1\": \"1\"}]')
      assert_equal ({"11" => 11, "33.3" => 33.3, "#13" => [1, 2, 3, "E_QUOTA"], "foo" => "bar"}, parse_json('{\"11\": 11, \"33.3\": 33.3, \"#13\": [1, 2, 3, \"E_QUOTA\"], \"foo\": \"bar\"}'))
    end
  end

  def test_that_parsing_json_works_for_complex_values_in_common_subset_mode
    run_test_as('wizard') do
      assert_equal [1, 1.1, "1.2", "#13", "E_ARGS", [2, 2.2, "foo"], {"1" => "1"}], parse_json('[1, 1.1, \"1.2\", \"#13\", \"E_ARGS\", [2, 2.2, \"foo\"], {\"1\": \"1\"}]', 'common-subset')
      assert_equal ({"11" => 11, "33.3" => 33.3, "#13" => [1, 2, 3, "E_QUOTA"], "foo" => "bar"}, parse_json('{\"11\": 11, \"33.3\": 33.3, \"#13\": [1, 2, 3, \"E_QUOTA\"], \"foo\": \"bar\"}'), 'common-subset')
    end
  end

  def test_that_parsing_json_works_for_complex_values_in_embedded_types_mode
    run_test_as('wizard') do
      assert_equal [1, 1.1, "1.2", MooObj.new('#13'), E_ARGS, [2, 2.2, "foo"], {1 => "1"}], parse_json('[1, 1.1, \"1.2\", \"#13|*o*\", \"E_ARGS|*e*\", [2, 2.2, \"foo\"], {\"1|*i*\": \"1\"}]', 'embedded-types')
      assert_equal ({11.1 => 11, "33.3" => 33.3, MooObj.new('#13') => [1, 2, 3, E_QUOTA], "foo" => "bar"}, parse_json('{\"11.1|*f*\": 11, \"33.3\": 33.3, \"#13|*o*\": [1, 2, 3, \"E_QUOTA|*e*\"], \"foo\": \"bar\"}', 'embedded-types'))
    end
  end

  def test_that_parsing_json_fails_for_various_reasons
    run_test_as('wizard') do
      assert_equal E_INVARG, parse_json('[1, 1.1, \"1.2\", blah]')
      assert_equal E_INVARG, parse_json('{11: 11, \"33.3\": 33.3, \"foo\": \"bar\"}')



      assert_equal E_INVARG, parse_json('[')
      assert_equal E_INVARG, parse_json('{')
      assert_equal E_INVARG, parse_json('\"')
      #assert_equal E_INVARG, parse_json('12abc')
      assert_equal E_INVARG, parse_json('1..2')
      assert_equal E_INVARG, parse_json('..')
      assert_equal E_INVARG, parse_json('@$*&#*')
      assert_equal E_INVARG, parse_json('[{[{[')

    end
  end

  def test_that_its_an_error_to_specify_anything_other_than_common_subset_or_embedded_types
    run_test_as('wizard') do
      assert_equal E_TYPE, generate_json(1, 1)
      assert_equal E_TYPE, generate_json(1, 1.23)
      assert_equal E_INVARG, generate_json(1, 'foo-bar')
      assert_equal E_TYPE, generate_json(1, :nothing)
      assert_equal E_TYPE, generate_json(1, E_PERM)
      assert_equal E_TYPE, generate_json(1, [])
      assert_equal E_TYPE, generate_json(1, {})
    end
  end

  def test_that_the_argument_to_parse_json_must_be_a_string
    run_test_as('wizard') do
      assert_equal E_TYPE, parse_json(1)
      assert_equal E_TYPE, parse_json(1.1)
      assert_equal E_TYPE, parse_json(E_ARGS)
      assert_equal E_TYPE, parse_json(:nothing)
      assert_equal E_TYPE, parse_json([])
      assert_equal E_TYPE, parse_json({})
    end
  end

  def generate_json(value, mode = nil)
    if mode.nil?
      simplify command %Q|; return generate_json(#{value_ref(value)});|
    else
      simplify command %Q|; return generate_json(#{value_ref(value)}, #{value_ref(mode)});|
    end
  end

  def parse_json(value, mode = nil)
    if mode.nil?
      simplify command %Q|; return parse_json(#{value_ref(value)});|
    else
      simplify command %Q|; return parse_json(#{value_ref(value)}, #{value_ref(mode)});|
    end
  end

  def value_ref(value)
    case value
    when String
      "\"#{value}\""
    when Symbol
      "$#{value.to_s}"
    when MooErr
      value.to_s
    when MooObj
      "##{value.to_s}"
    when Array
      '{' + value.map { |o| value_ref(o).to_s }.join(', ') + '}'
    when Hash
      '[' + value.map { |a, b| value_ref(a).to_s + " -> " + value_ref(b).to_s }.join(', ') + ']'
    else
      value
    end
  end

end
