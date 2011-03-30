require 'test/unit'

require 'moo_support'

if defined?(Test::Unit::TestCase)

  class Test::Unit::TestCase
    include MooSupport
  end

end
