require 'socket'
require 'yaml'

require 'moo_obj'
require 'moo_err'
require 'general_expression_parser'
require 'fast_error_parser'

module MooSupport

  NOTHING = MooObj.new('#-1')
  AMBIGUOUS_MATCH = MooObj.new('#-2')
  FAILED_MATCH = MooObj.new('#-3')

  E_NONE = MooErr.new('E_NONE')
  E_TYPE = MooErr.new('E_TYPE')
  E_DIV = MooErr.new('E_DIV')
  E_PERM = MooErr.new('E_PERM')
  E_PROPNF = MooErr.new('E_PROPNF')
  E_VERBNF = MooErr.new('E_VERBNF')
  E_VARNF = MooErr.new('E_VARNF')
  E_INVIND = MooErr.new('E_INVIND')
  E_RECMOVE = MooErr.new('E_RECMOVE')
  E_MAXREC = MooErr.new('E_MAXREC')
  E_RANGE = MooErr.new('E_RANGE')
  E_ARGS = MooErr.new('E_ARGS')
  E_NACC = MooErr.new('E_NACC')
  E_INVARG = MooErr.new('E_INVARG')
  E_QUOTA = MooErr.new('E_QUOTA')
  E_FLOAT = MooErr.new('E_FLOAT')

  raise '"./test.yml" configuration file not found' unless File.exists?('./test.yml')

  @@options = YAML.load(File.open('./test.yml'))

  @@options['verbose'] = true if ARGV.include?('--loud')
  @@options['verbose'] = false if ARGV.include?('--quiet')

  protected

  def options
    @@options
  end

  def send_string(string)
    puts "> " + string if options['verbose']
    @sock.puts string
  end

  def run_test_as(*params)
    # The helpers below are unchanged during a test run.
    # Reset them before running a new test.
    @me = nil
    @here = nil

    @host = options['host']
    @port = options['port']
    @sock = TCPSocket.open @host, @port
    send_string "connect #{params.join(' ')}"
    send_string "PREFIX -+-+-"
    send_string "SUFFIX =+=+="
    begin
      yield
    rescue
      @sock.close
      raise $!
    end
  end

  alias run_as run_test_as

  def command(command)
    raise 'failed to enclose test in "run_test_as/run_as" block' unless @sock
    send_string command
    while (true)
      line = @sock.gets.chomp
      break if line == '-+-+-'
    end
    acc = []
    while (true)
      line = @sock.gets.chomp
      break if line == '=+=+='
      acc << line
    end
    acc.each { |a| puts "< " + a } if options['verbose']
    acc.length > 0 ? acc.length > 1 ? acc : acc[0] : 0
  end

  def simplify(result)
    result ? (result[1] == 50 ? fast_error_parse(result) : general_expression_parse(result)) : false
  end

  def true_or_false(result)
    result == 0 || result == "" || result == [] || result == {} ? false : true
  end

  def evaluate(expression)
    simplify command "; return #{expression};"
  end

  ## Support

  def nothing
    NOTHING
  end

  def invalid_object
    simplify command %Q|; return toobj(2147483647);|
  end

  def me
    @me ||= simplify command %Q|; return player;|
  end

  alias player me

  def here
    @here ||= simplify command %Q|; return player.location;|
  end

  alias location here

  def set(object, property, value)
    simplify command %|; return #{obj_ref(object)}.#{property} = #{value_ref(value)};|
  end

  def get(object, property)
    simplify command %|; return #{obj_ref(object)}.#{property};|
  end

  def call(object, verb, *args)
    simplify command %|; return #{obj_ref(object)}:#{verb}(#{args.map{|a| value_ref(a)}.join(', ')});|
  end

  def eval(string)
    simplify command %|; #{string}; |
  end

  ## Manipulating Objects
  ### Fundamental Operations on Objects

  def create(*args)
    simplify command %Q|; return create(#{args.map{|a| value_ref(a)}.join(', ')});|
  end

  def valid(object)
    true_or_false simplify command %Q|; return valid(#{obj_ref(object)});|
  end

  def chparent(*args)
    simplify command %Q|; return chparent(#{args.map{|a| value_ref(a)}.join(', ')});|
  end

  def parent(*args)
    simplify command %Q|; return parent(#{args.map{|a| value_ref(a)}.join(', ')});|
  end

  def children(object)
    simplify command %Q|; return children(#{obj_ref(object)});|
  end

  def ancestors(object)
    simplify command %Q|; return ancestors(#{obj_ref(object)});|
  end

  def descendants(object)
    simplify command %Q|; return descendants(#{obj_ref(object)});|
  end

  def recycle(object)
    simplify command %Q|; return recycle(#{obj_ref(object)});|
  end

  def max_object
    simplify command %Q|; return max_object();|
  end

  ### Object Movement

  def move(what, where)
    simplify command %Q|; return move(#{obj_ref(what)}, #{obj_ref(where)});|
  end

  ### Operations on Properties

  def add_property(object, property, value, property_info)
    property_info = property_info_to_s(property_info)
    simplify command %Q|; return add_property(#{obj_ref(object)}, #{value_ref(property)}, #{value_ref(value)}, #{property_info});|
  end

  def property_info(object, property)
    simplify command %Q|; return property_info(#{obj_ref(object)}, #{value_ref(property)});|
  end

  def set_property_info(object, property, property_info)
    property_info = property_info_to_s(property_info)
    simplify command %Q|; return set_property_info(#{obj_ref(object)}, #{value_ref(property)}, #{property_info});|
  end

  def clear_property(object, property)
    simplify command %Q|; return clear_property(#{obj_ref(object)}, #{value_ref(property)});|
  end

  def delete_property(object, property)
    simplify command %Q|; return delete_property(#{obj_ref(object)}, #{value_ref(property)});|
  end

  ### Operations on Verbs

  def add_verb(object, verb_info, verb_args)
    verb_info = verb_info_to_s(verb_info)
    verb_args = verb_args_to_s(verb_args)
    simplify command %Q|; return add_verb(#{obj_ref(object)}, #{verb_info}, #{verb_args});|
  end

  def delete_verb(object, verb)
    simplify command %Q|; return delete_verb(#{obj_ref(object)}, #{value_ref(verb)});|
  end

  def set_verb_info(object, verb, verb_info)
    verb_info = verb_info_to_s(verb_info)
    simplify command %Q|; return set_verb_info(#{obj_ref(object)}, #{value_ref(verb)}, #{verb_info});|
  end

  def set_verb_args(object, verb, verb_args)
    verb_args = verb_args_to_s(verb_args)
    simplify command %Q|; return set_verb_args(#{obj_ref(object)}, #{value_ref(verb)}, #{verb_args});|
  end

  def set_verb_code(object, verb, &verb_code)
    vc = []
    yield vc
    verb_code = ''
    vc.each { |v| verb_code << v.inspect << ',' }
    verb_code = verb_code.empty? ? '{,' : '{' + verb_code
    verb_code[-1] = '}'
    simplify command %Q|; return set_verb_code(#{object}, #{value_ref(verb)}, #{verb_code});|
  end

  ## MOO-Code Evaluation and Task Manipulation

  def function_info(name)
    simplify command %Q|; return function_info("#{name}");|
  end

  def has_function?(name)
    true_or_false simplify command %Q|; return `function_info("#{name}") ! E_INVARG => {}';|
  end

  ## Administrative Operations

  def reset_max_object
    simplify command %|; return reset_max_object();|
  end

  def renumber(object)
    simplify command %|; return renumber(#{obj_ref(object)});|
  end

  def shutdown
    simplify command %|; shutdown();|
  end

  ## Server Statistics and Miscellaneous Information

  def verb_cache_stats
    simplify command %|; return verb_cache_stats();|
  end

  private

  def obj_ref(obj_ref)
    case obj_ref
    when Integer
      "##{obj_ref.to_s}"
    when Symbol
      "$#{obj_ref.to_s}"
    else
      obj_ref
    end
  end

  def obj_ref_or_list_of_obj_refs(complex)
    case complex
    when Array
      '{' + complex.map { |o| obj_ref(o) }.join(', ') + '}'
    else
      obj_ref(complex)
    end
  end

  def value_ref(value)
    case value
    when String
      "\"#{value}\""
    when Symbol
      "$#{value.to_s}"
    when Array
      '{' + value.map { |o| value_ref(o) }.join(', ') + '}'
    else
      value
    end
  end

  def property_info_to_s(property_info)
    unless property_info.class == String
      owner, flags = property_info
      property_info = %Q|{#{obj_ref(owner)}, "#{flags}"}|
    end
    property_info
  end

  def verb_info_to_s(verb_info)
    unless verb_info.class == String
      owner, flags, name = verb_info
      verb_info = %Q|{#{obj_ref(owner)}, "#{flags}", "#{name}"}|
    end
    verb_info
  end

  def verb_args_to_s(verb_args)
    unless verb_args.class == String
      dobj, prep, iobj = verb_args
      verb_args = %Q|{"#{dobj}", "#{prep}", "#{iobj}"}|
    end
    verb_args
  end

  GENERAL_EXPRESSION_PARSER = GeneralExpressionParser.new
  GENERAL_EXPRESSION_TRANSFORM = GeneralExpressionTransform.new

  def general_expression_parse(str)
    GENERAL_EXPRESSION_TRANSFORM.apply(GENERAL_EXPRESSION_PARSER.parse(str))[1]
  end

  FAST_ERROR_PARSER = FastErrorParser.new
  FAST_ERROR_TRANSFORM = FastErrorTransform.new

  def fast_error_parse(str)
    FAST_ERROR_TRANSFORM.apply(FAST_ERROR_PARSER.parse(str))
  end

end
