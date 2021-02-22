class SemanticFunctionParser

prechigh
  nonassoc UMINUS IF ELSE WHILE
#  left '*' '/'
#  left '+' '-'
  left BINOP BINCOMP
  left BINCOND
preclow

rule

  block: '{' '}' { return SemanticFunctionAST.new({ type: :block, list: SemanticFunctionAST.nothing }) }
  block: '{' block '}'     { return val[1] }
       | '{' stmt_list '}' { return SemanticFunctionAST.new({ type: :block, list: val[1] }) }
       | stmt		   { return SemanticFunctionAST.new({ type: :block, list: SemanticFunctionAST.new({ type: :stmt_list, head: val[0], tail: SemanticFunctionAST.nothing }) }) }

  stmt: block_stmt       { return val[0] }
      | var '=' expr	   { return SemanticFunctionAST.new({ type: :assign, lhs: val[0], rhs: val[2] }) }
      | func  { return val[0] }

  block_stmt: IF '(' cond ')' block ELSE block  { return SemanticFunctionAST.new({ type: :if, cond: val[2], then: val[4], else: val[6] }) }
            | IF '(' cond ')' block		  { return SemanticFunctionAST.new({ type: :if, cond: val[2], then: val[4], else: SemanticFunctionAST.nothing }) }
            | WHILE '(' cond ')' block	  { return SemanticFunctionAST.new({ type: :while, cond: val[2], loop: val[4] }) }

  func: FUNC '(' func_args ')'	{ return SemanticFunctionAST.new({ type: :func, name: val[0], args: val[2] }) }

  expr: UNIOP expr	{ return SemanticFunctionAST.new({ type: :uniop, name: val[0], rhs: val[1] }) }
      | expr BINOP expr	{ return SemanticFunctionAST.new({ type: :binop, name: val[1], lhs: val[0], rhs: val[2] }) }
      | '(' expr ')'	{ return val[1] }
      | func    { return val[0] }
      | leaf		{ return val[0] }

  arg: expr {val[0]}
     | cond {val[0]}
#     | '{' stmt_list '}' { return SemanticFunctionAST.new({ type: :block, list: val[1] }) }

  func_args: arg ',' func_args	  { return [val[0]] + val[2] }
	   | arg		  { return [val[0]] }
     |          { return [] }

  cond: '(' cond ')' { return val[1] }
      | cond BINCOND cond   { return SemanticFunctionAST.new({ type: :bincond, name: val[1], lhs: val[0], rhs: val[2] }) }
      | cond BINCOMP cond   { return SemanticFunctionAST.new({ type: :bincond, name: val[1], lhs: val[0], rhs: val[2] }) }
      | UNIOP cond	    { return SemanticFunctionAST.new({ type: :unicond, name: val[0], rhs: val[1] }) }
      | expr		    { val[0] }
      | leaf		    { val[0] }
#      | expr		    { return SemanticFunctionAST.new({ type: :cond, value: val[0] }) }
#      | leaf		    { return SemanticFunctionAST.new({ type: :cond, value: val[0] }) }

  stmt_list: stmt ';' stmt_list   { return SemanticFunctionAST.new({ type: :stmt_list, head: val[0], tail: val[2]}) }
           | block_stmt stmt_list { return SemanticFunctionAST.new({ type: :stmt_list, head: val[0], tail: val[1]}) }
           | stmt ';'             { return SemanticFunctionAST.new({ type: :stmt_list, head: val[0], tail: SemanticFunctionAST.nothing }) }
           | stmt                 { return SemanticFunctionAST.new({ type: :stmt_list, head: val[0], tail: SemanticFunctionAST.nothing }) }

  leaf: var               { return val[0] }
      | '[' STRING ']'  { return SemanticFunctionAST.new(type: :string, value: val[0]) }
      | NUMBER            { return SemanticFunctionAST.new(type: :number, number: val[0]) }
      | HEX_NUMBER        { return SemanticFunctionAST.new(type: :number, number: val[0]) }

  var: VAR                { return SemanticFunctionAST.new(type: :var, name: val[0]) }

end

---- inner

def parse(str)
  orig_str = str
  str = str.gsub(" ", "").gsub("\n", "").gsub("\r", "")
  @yydebug = true
  @q = []
  until str.empty?
    append = ""
    case str
      when /\A(if)/
        @q.push [:IF, $1]
      when /\A(else)/
        @q.push [:ELSE, $1]
      when /\A(while)/
        @q.push [:WHILE, $1]
      when /\A(&&|\|\||\^\^)/
        @q.push [:BINCOND, $1]
      when /\A(&|\||\^|<<|>>|-|\+|\/|\*)/
        @q.push [:BINOP, $1]
      when /\A(==|!=|<=|<|>=|>)/
        @q.push [:BINCOMP, $1]
      when /\A([\~!])/
        @q.push [:UNIOP, $1]
      when /\A(a)\]/
        @q.push [:STRING, $1]
        append = "]"
      when /\A(^[a-zA-Z][a-zA-Z0-9]*)\(/
        @q.push [:FUNC, $1]
        append = '('
      when /\A(@?[a-zA-Z_][a-zA-Z0-9_]*)/
        @q.push [:VAR, $1]
      when /\A0x([0-9a-fA-F])+/
        @q.push [:HEX_NUMBER, $&.to_i(16)]
      when /\A\d+/
        @q.push [:NUMBER, $&.to_i]
      when /\A.|\n/o
        s = $&
        @q.push [s, s]
#     # when /\A([\+\-\*\/]|<<|>>|&)/
#     #   @q.push [:BINOP, $1]
    end
    str = append + $'
  end
  @q.push [false, '$end']
#  begin
    do_parse
#  rescue
#    return SemanticFunctionAST.error("Error parsing: --#{orig_str}--")
#  end
end

 def next_token
  @q.shift
 end

 def on_error(t, val, vstack)
   raise ParseError, sprintf("\nparse error on value %s (%s)",
                             val.inspect, token_to_str(t) || '?')
 end

---- footer
