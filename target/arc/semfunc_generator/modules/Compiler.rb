module Compiler
  def optimize(ast, log = [], debug = false)
    log.push({ name: 'start', ast: ast })
    SemanticFunctionAST.reset_counters()
    @passes.each do |pass|
      ast = ast.clone
      ast = pass.task(ast)
      log.push({ name: pass.name, ast: ast })
      # puts " -- #{pass.name} --"
      # puts ast.pp
    end
    if(debug == true)
      log.each do |v|
        puts v[:name] + ":"
        puts "  => #{v[:ast].pp}"
      end
    end
    return ast
  end

  def getAST(input)
    if(input.class == String)
      input = SemanticFunctionAST.parse(input)
    elsif(input.class != SemanticFunctionAST)
      abort()
    end
    return input
  end

  def generate(input, log = [], debug = false)
    ast = getAST(input)
    ast = self.optimize(ast, log, debug)
    # puts ast.class
    return @translator.generate(ast, debug)
  end

  def compile(code, log = [], debug = false)
    ast = getAST(code)
    ast = optimize(ast, log, debug)
    return ast
  end
end
