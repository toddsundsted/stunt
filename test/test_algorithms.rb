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

  def test_that_hashing_works
    run_test_as('programmer') do
      assert_equal "900150983CD24FB0D6963F7D28E17F72", string_hash("abc", "md5")
      assert_equal "A9993E364706816ABA3E25717850C26C9CD0D89D", string_hash("abc", "sha1")
      assert_equal "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD", string_hash("abc", "sha256")
      assert_equal "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD", string_hash("abc")
      assert_equal "8215EF0796A20BCAAAE116D3876C664A", string_hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "md5")
      assert_equal "84983E441C3BD26EBAAE4AA1F95129E5E54670F1", string_hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "sha1")
      assert_equal "248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1", string_hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "sha256")
      assert_equal "248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1", string_hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")

      assert_equal "900150983CD24FB0D6963F7D28E17F72", binary_hash("abc", "md5")
      assert_equal "A9993E364706816ABA3E25717850C26C9CD0D89D", binary_hash("abc", "sha1")
      assert_equal "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD", binary_hash("abc", "sha256")
      assert_equal "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD", binary_hash("abc")
      assert_equal "8215EF0796A20BCAAAE116D3876C664A", binary_hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "md5")
      assert_equal "84983E441C3BD26EBAAE4AA1F95129E5E54670F1", binary_hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "sha1")
      assert_equal "248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1", binary_hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "sha256")
      assert_equal "248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1", binary_hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")

      assert_equal "CA9C491AC66B2C62500882E93F3719A8", binary_hash("~00~00~00~00~00", "md5")
      assert_equal "A10909C2CDCAF5ADB7E6B092A4FABA558B62BD96", binary_hash("~00~00~00~00~00", "sha1")
      assert_equal "8855508AADE16EC573D21E6A485DFD0A7624085C1A14B5ECDD6485DE0C6839A4", binary_hash("~00~00~00~00~00", "sha256")
      assert_equal "8855508AADE16EC573D21E6A485DFD0A7624085C1A14B5ECDD6485DE0C6839A4", binary_hash("~00~00~00~00~00")

      assert_equal "CA9C491AC66B2C62500882E93F3719A8", binary_hash("~00~00~00~00~00", "md5")
      assert_equal "A10909C2CDCAF5ADB7E6B092A4FABA558B62BD96", binary_hash("~00~00~00~00~00", "sha1")
      assert_equal "8855508AADE16EC573D21E6A485DFD0A7624085C1A14B5ECDD6485DE0C6839A4", binary_hash("~00~00~00~00~00", "sha256")
      assert_equal "8855508AADE16EC573D21E6A485DFD0A7624085C1A14B5ECDD6485DE0C6839A4", binary_hash("~00~00~00~00~00")

      assert_equal "0236065F9B0E6AB6C6C860791121DB4E", binary_hash("~A7~CE~44~8E~D2~4C~16~61~F6~F2~01~55~5A~32~BC~B0~29~EC~02~86~76~CD~9B~05~E6~36~F3~2E~76~78~3C~F0~D1~45~40~C7~DA~F3~C7~C0~AA~43~1E~D2~D0~03~5D~21~F7~0C~C9~19~F0~82~67~76~E4~19~3A~02~F0~7E~F8~BE~CC~6A~27~46~C2~C3~B8~1C~91~1F~7E~F7~F5~50~D9~0E~D8~D6~89~DA~86~B5~95~E0~66~0C~4A~92~01~2B~8A~AE~2E~7F~3B~88", "md5")
      assert_equal "9A2766D4D1740DCACD13D150B89626C63E2F83EE", binary_hash("~A7~CE~44~8E~D2~4C~16~61~F6~F2~01~55~5A~32~BC~B0~29~EC~02~86~76~CD~9B~05~E6~36~F3~2E~76~78~3C~F0~D1~45~40~C7~DA~F3~C7~C0~AA~43~1E~D2~D0~03~5D~21~F7~0C~C9~19~F0~82~67~76~E4~19~3A~02~F0~7E~F8~BE~CC~6A~27~46~C2~C3~B8~1C~91~1F~7E~F7~F5~50~D9~0E~D8~D6~89~DA~86~B5~95~E0~66~0C~4A~92~01~2B~8A~AE~2E~7F~3B~88", "sha1")
      assert_equal "A088FB0606E595E8A248393A296BE79C5A0FE7417FE267334E2113D6613B8D70", binary_hash("~A7~CE~44~8E~D2~4C~16~61~F6~F2~01~55~5A~32~BC~B0~29~EC~02~86~76~CD~9B~05~E6~36~F3~2E~76~78~3C~F0~D1~45~40~C7~DA~F3~C7~C0~AA~43~1E~D2~D0~03~5D~21~F7~0C~C9~19~F0~82~67~76~E4~19~3A~02~F0~7E~F8~BE~CC~6A~27~46~C2~C3~B8~1C~91~1F~7E~F7~F5~50~D9~0E~D8~D6~89~DA~86~B5~95~E0~66~0C~4A~92~01~2B~8A~AE~2E~7F~3B~88", "sha256")
      assert_equal "A088FB0606E595E8A248393A296BE79C5A0FE7417FE267334E2113D6613B8D70", binary_hash("~A7~CE~44~8E~D2~4C~16~61~F6~F2~01~55~5A~32~BC~B0~29~EC~02~86~76~CD~9B~05~E6~36~F3~2E~76~78~3C~F0~D1~45~40~C7~DA~F3~C7~C0~AA~43~1E~D2~D0~03~5D~21~F7~0C~C9~19~F0~82~67~76~E4~19~3A~02~F0~7E~F8~BE~CC~6A~27~46~C2~C3~B8~1C~91~1F~7E~F7~F5~50~D9~0E~D8~D6~89~DA~86~B5~95~E0~66~0C~4A~92~01~2B~8A~AE~2E~7F~3B~88")

      assert_equal "99914B932BD37A50B983C5E7C90AE93B", value_hash([], "md5")
      assert_equal "BF21A9E8FBC5A3846FB05B4FA0859E0917B2202F", value_hash([], "sha1")
      assert_equal "44136FA355B3678A1146AD16F7E8649E94FB4FC21FE77E8310C060F61CAAFF8A", value_hash([], "sha256")
      assert_equal "44136FA355B3678A1146AD16F7E8649E94FB4FC21FE77E8310C060F61CAAFF8A", value_hash([])

      assert_equal "D751713988987E9331980363E24189CE", value_hash({}, "md5")
      assert_equal "97D170E1550EEE4AFC0AF065B78CDA302A97674C", value_hash({}, "sha1")
      assert_equal "4F53CDA18C2BAA0C0354BB5F9A3ECBE5ED12AB4D8E11BA873C2F11161202B945", value_hash({}, "sha256")
      assert_equal "4F53CDA18C2BAA0C0354BB5F9A3ECBE5ED12AB4D8E11BA873C2F11161202B945", value_hash({})

      string = toliteral([1, 2, 3, {:nothing => ["fee", "fi", "fo", "fum"]}])
      assert_equal "CF4BC5C55A11D6ECF1913148CCC98FFE", string_hash(string, "md5")
      assert_equal "CF4BC5C55A11D6ECF1913148CCC98FFE", value_hash([1, 2, 3, {:nothing => ["fee", "fi", "fo", "fum"]}], "md5")
      assert_equal "965D05B1BA7E8EF2F3D0F8A9335B86661344794C", string_hash(string, "sha1")
      assert_equal "965D05B1BA7E8EF2F3D0F8A9335B86661344794C", value_hash([1, 2, 3, {:nothing => ["fee", "fi", "fo", "fum"]}], "sha1")
      assert_equal "DC8CE6D88FA5A949979DF79FD745F859D9250FA0A20766FFA3DF88B81EFCEA68", string_hash(string, "sha256")
      assert_equal "DC8CE6D88FA5A949979DF79FD745F859D9250FA0A20766FFA3DF88B81EFCEA68", value_hash([1, 2, 3, {:nothing => ["fee", "fi", "fo", "fum"]}])
      assert_equal "DC8CE6D88FA5A949979DF79FD745F859D9250FA0A20766FFA3DF88B81EFCEA68", string_hash(string)
      assert_equal "DC8CE6D88FA5A949979DF79FD745F859D9250FA0A20766FFA3DF88B81EFCEA68", value_hash([1, 2, 3, {:nothing => ["fee", "fi", "fo", "fum"]}])
    end
  end

  def test_that_hmacing_works
    run_test_as('programmer') do
      assert_equal "5BDCC146BF60754E6A042426089575C75A003F089D2739839DEC58B964EC3843", string_hmac("what do ya want for nothing?", "Jefe")
      assert_equal "5BDCC146BF60754E6A042426089575C75A003F089D2739839DEC58B964EC3843", binary_hmac("what do ya want for nothing?", "Jefe")
      assert_equal "6E5C2A18FEBCCACA669AC5F4415342714850E76BA86824A3371270E6DD69956F", value_hmac("what do ya want for nothing?", "Jefe")
      assert_equal "B0344C61D8DB38535CA8AFCEAF0BF12B881DC200C9833DA726E9376C2E32CFF7", string_hmac("Hi There", "~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b")
      assert_equal "B0344C61D8DB38535CA8AFCEAF0BF12B881DC200C9833DA726E9376C2E32CFF7", binary_hmac("Hi There", "~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b")
      assert_equal "69AB6C68D61A3B85453AB106B356640D2D3F8D4945C67781423B7A8D48565A02", value_hmac("Hi There", "~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b~0b")
      assert_equal "9B09FFA71B942FCB27635FBCD5B0E944BFDC63644F0713938A7F51535C3A35E2", string_hmac("This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.", "~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa")
      assert_equal "9B09FFA71B942FCB27635FBCD5B0E944BFDC63644F0713938A7F51535C3A35E2", binary_hmac("This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.", "~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa")
      assert_equal "951A56011AB193C6C74E68CFB855D25ADFF63C488FDE458039AE3F4108EE37F0", value_hmac("This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm.", "~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa")
      assert_equal "60E431591EE0B67F0D8A26AACBF5B77F8E0BC6213728C5140546040F0EE37F54", string_hmac("Test Using Larger Than Block-Size Key - Hash Key First", "~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa")
      assert_equal "60E431591EE0B67F0D8A26AACBF5B77F8E0BC6213728C5140546040F0EE37F54", binary_hmac("Test Using Larger Than Block-Size Key - Hash Key First", "~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa")
      assert_equal "EC69C60BE95151FEF7E303AD7231C7165E4120683F7F218CE0661F7B8317534C", value_hmac("Test Using Larger Than Block-Size Key - Hash Key First", "~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa~aa")
      assert_equal "44D1527E6295BF1055924DEBECC4499776903B9C37FF7D6519452230A7C30AF8", string_hmac("Test Using Key with nulls", "123456789~00123456789~00123456789~00")
      assert_equal "44D1527E6295BF1055924DEBECC4499776903B9C37FF7D6519452230A7C30AF8", binary_hmac("Test Using Key with nulls", "123456789~00123456789~00123456789~00")
      assert_equal "415BD3379F2E94D6F38ED2D54BEFB367912B7043FE033D3B6C66A78EA51A153C", value_hmac("Test Using Key with nulls", "123456789~00123456789~00123456789~00")
      assert_equal "B613679A0814D9EC772F95D778C35FC5FF1697C493715653C6C712144292C5AD", string_hmac("", "")
      assert_equal "B613679A0814D9EC772F95D778C35FC5FF1697C493715653C6C712144292C5AD", binary_hmac("", "")
      assert_equal "140B47411F10521CC48851F43C9ABE73016E1D45627C8523D25CBF7FE64F8B03", value_hmac("", "")
    end
  end

  def test_that_encoding_and_decoding_bad_binary_strings_does_not_crash_the_server
    run_test_as('programmer') do
      assert_equal E_INVARG, encode_base64('~00~#!~00')
      assert_equal E_INVARG, decode_base64('~00~#!~00')
    end
  end

  def test_that_bad_base64_strings_fail
    run_test_as('programmer') do
      assert_equal E_INVARG, decode_base64('MTIzNDU2Nzg5ADEyMzQ1N')
      assert_equal E_INVARG, decode_base64('///')
    end
  end

  def test_that_specifying_fake_hash_algorithms_fails
    run_test_as('programmer') do
      assert_equal E_INVARG, string_hash("abc", "bozo")
      assert_equal E_INVARG, binary_hash("abc", "con")
      assert_equal E_INVARG, value_hash("abc", "carne")
    end
  end

  def test_that_hashing_bad_binary_strings_does_not_crash_the_server
    run_test_as('programmer') do
      assert_equal E_INVARG, binary_hash('~00~#!~00')
    end
  end

  def test_that_hmacing_bad_binary_string_keys_does_not_crash_the_server
    run_test_as('programmer') do
      assert_equal E_INVARG, string_hmac('', '~00~#!~00')
      assert_equal E_INVARG, binary_hmac('', '~00~#!~00')
      assert_equal E_INVARG, value_hmac('','~00~#!~00')
    end
  end

  def test_that_hmacing_bad_binary_strings_does_not_crash_the_server
    run_test_as('programmer') do
      assert_equal E_INVARG, binary_hmac('~00~#!~00', 'key')
    end
  end

end
