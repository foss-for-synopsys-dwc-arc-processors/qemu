class SpaghettiCodePass1
  private

  # extend SemanticFunctionASTFactory
  include Pass
  extend Translator

  def self.translation_rules
    ret = {}

    match = SemanticFunctionAST.new(type: :if, name: "_")
    ret[match] =
      Proc.new { |stmt, repl, mappings, to_do|

        binop_lhs = stmt.object[:rhs].object[:lhs]
        binop_rhs = stmt.object[:rhs].object[:rhs]
        changed_lhs = false
        changed_rhs = false

        if(binop_lhs.object[:type] != :var && binop_lhs.object[:type] != :number)
          # puts "IN 1 #{binop_lhs.inspect}"
          var = SemanticFunctionAST.createTmpVar("temp")
          tmp = [
            # SemanticFunctionAST.function("createTmpVar", var),
          ]
          to_do[:pre_pend] = tmp + to_do[:pre_pend]
          assign = SemanticFunctionAST.new(type: :assign, lhs: var, rhs: binop_lhs)
          self.replace(assign, to_do)
          binop_lhs = var
          changed_lhs = true
        end

        if(binop_rhs.object[:type] != :var && binop_rhs.object[:type] != :number)
          # puts "IN 2 #{binop_rhs.inspect}"
          var = SemanticFunctionAST.createTmpVar("temp")
          tmp = [
            # SemanticFunctionAST.function("createTmpVar", var),
            assign = SemanticFunctionAST.new(type: :assign, lhs: var, rhs: binop_rhs)
          ]
          to_do[:pre_pend] = tmp + to_do[:pre_pend]
          self.replace(assign, to_do)
          binop_rhs = var
          changed_rhs = true
        end

        if(changed_lhs == true || changed_rhs == true)
          new_stmt = stmt.clone
          new_stmt.object[:rhs].object[:lhs] = binop_lhs
          new_stmt.object[:rhs].object[:rhs] = binop_rhs
          to_do[:pre_pend].push(new_stmt)
          # to_do[:remove] = true
          stmt.object[:type] = :nothing
        end
      }

    return ret
  end

  public
  def self.task(ast)
    self.generate(ast)
    # puts "AST = #{ast.class}"
    # puts ast.debug
    return ast
  end
end
