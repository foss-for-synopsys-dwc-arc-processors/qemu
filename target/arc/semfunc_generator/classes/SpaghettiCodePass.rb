class SpaghettiCodePass
  private

  # extend SemanticFunctionASTFactory
  include Pass

  def self.spaghetify(ast)
    ret = []
    object = ast.object

    case(object[:type])
    when :block
      ret += spaghetify(object[:list])
    when :stmt_list
      ret += spaghetify(object[:head])
      ret += spaghetify(object[:tail])
    when :if

      cond = ast.object[:cond]

      if(cond.hasAttr?(:static))
        ast.object[:then] = SemanticFunctionAST.block(SemanticFunctionAST.createStmtListFromArray(spaghetify(ast.object[:then])))
        ast.object[:else] = SemanticFunctionAST.block(SemanticFunctionAST.createStmtListFromArray(spaghetify(ast.object[:else])))
        ret.push(ast)
      else
        done_label = SemanticFunctionAST.createTmpVar("done")
        else_label = SemanticFunctionAST.createTmpVar("else")
        else_label = done_label unless object[:else].valid?

        ret.push(SemanticFunctionAST.defLabel(else_label)) if object[:else].valid?
        ret.push(SemanticFunctionAST.defLabel(done_label))

        ret.push(SemanticFunctionAST.function("IF", SemanticFunctionAST.notCond(object[:cond].clone), else_label, done_label))

        ret += spaghetify(object[:then])
        if object[:else].valid?
          ret.push(SemanticFunctionAST.function("goto", done_label))
          ret.push(SemanticFunctionAST.setLabel(else_label))
          ret += spaghetify(object[:else])
        end
        ret.push(SemanticFunctionAST.setLabel(done_label))
      end
    else
      ret.push(ast) if ast.valid?
    end

    return ret
  end

  public
  def self.task(ast)
    # reset_counters
    return SemanticFunctionAST.createStmtListFromArray(spaghetify(ast))
  end
end
