require 'test_helper'

class TestAlgorithms < Test::Unit::TestCase

  def test_that_base64_encoding_works
    run_test_as('programmer') do
      assert_equal 'MTIzNDU2Nzg5ADEyMzQ1Njc4OQAxMjM0NTY3ODkA', encode_base64('123456789~00123456789~00123456789~00')
      assert_equal "p85EjtJMFmH28gFVWjK8sCnsAoZ2zZsF5jbzLnZ4PPDRRUDH2vPHwKpDHtLQA10h9wzJGfCCZ3bkGToC8H74vsxqJ0bCw7gckR9+9/VQ2Q7Y1onahrWV4GYMSpIBK4quLn87iA==", encode_base64("~A7~CED~8E~D2L~16a~F6~F2~01UZ2~BC~B0)~EC~02~86v~CD~9B~05~E66~F3.vx<~F0~D1E@~C7~DA~F3~C7~C0~AAC~1E~D2~D0~03]!~F7~0C~C9~19~F0~82gv~E4~19:~02~F0~7E~F8~BE~CCj'F~C2~C3~B8~1C~91~1F~7E~F7~F5P~D9~0E~D8~D6~89~DA~86~B5~95~E0f~0CJ~92~01+~8A~AE.~7F;~88")
    end
  end

  def test_that_base64_decoding_works
    run_test_as('programmer') do
      assert_equal '123456789~00123456789~00123456789~00', decode_base64('MTIzNDU2Nzg5ADEyMzQ1Njc4OQAxMjM0NTY3ODkA')
      assert_equal "~A7~CED~8E~D2L~16a~F6~F2~01UZ2~BC~B0)~EC~02~86v~CD~9B~05~E66~F3.vx<~F0~D1E@~C7~DA~F3~C7~C0~AAC~1E~D2~D0~03]!~F7~0C~C9~19~F0~82gv~E4~19:~02~F0~7E~F8~BE~CCj'F~C2~C3~B8~1C~91~1F~7E~F7~F5P~D9~0E~D8~D6~89~DA~86~B5~95~E0f~0CJ~92~01+~8A~AE.~7F;~88", decode_base64("p85EjtJMFmH28gFVWjK8sCnsAoZ2zZsF5jbzLnZ4PPDRRUDH2vPHwKpDHtLQA10h9wzJGfCCZ3bkGToC8H74vsxqJ0bCw7gckR9+9/VQ2Q7Y1onahrWV4GYMSpIBK4quLn87iA==")
    end
  end

  def test_that_bad_binary_strings_do_not_crash_the_server
    run_test_as('programmer') do
      assert_equal E_INVARG, encode_base64('~00~#!~00')
    end
  end

  def test_that_bad_base64_strings_fail
    run_test_as('programmer') do
      assert_equal E_INVARG, decode_base64('~00~#!~00')
      assert_equal E_INVARG, decode_base64('MTIzNDU2Nzg5ADEyMzQ1N')
      assert_equal E_INVARG, decode_base64('///')
    end
  end

end
