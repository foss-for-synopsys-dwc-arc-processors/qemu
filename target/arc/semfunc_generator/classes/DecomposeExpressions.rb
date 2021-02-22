class DecomposeExpressions
  private

  extend SemanticFunctionASTFactory
  include Pass

  def self.traverse(ast)
  end

  def self.expandConditions(ast)
    object = ast.object
    case(object[:type])
    when :var
    else
      return ast
    end
    return ret
  end

  public
  def self.task(ast)
    # reset_counters
    tmp_vars_for_nodes = {}
    current_stmt = nil

    ast.traverse_LR_TB() do |ast, to_do|
      object = ast.object
      case(object[:type])
      when :func
        if(object[:name] == "IF")
          to_do[:pre_pend] += ast.object[:args][0].create_stmts_for_expression()

          var = to_do[:pre_pend][-1].object[:lhs]

          ast.object[:args][0] = SemanticFunctionAST.new(type: :bincond, name: "==", lhs: var, rhs: createVar("true"))
        end
      else

      end
      true
    end

    return ast
  end
end
