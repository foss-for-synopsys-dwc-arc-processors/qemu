module SemanticFunctionASTFactory

  def createIf(cond, else_label, done_label)
    return SemanticFunctionAST.function("IF", cond.clone,
                SemanticFunctionAST.function("goto", else_label),
                SemanticFunctionAST.function("goto", done_label))
    return SemanticFunctionAST.new(type: :func, name: "IF", args: [
      cond.clone,
      SemanticFunctionAST.new(type: :func, name: "goto", args: [ else_label ]),
      SemanticFunctionAST.new(type: :func, name: "goto", args: [ done_label ])
    ])
  end

  def createVar(varname)
    return SemanticFunctionAST.new(type: :var, name: varname)
  end

  def notCond(cond)
    return SemanticFunctionAST.new(type: :unicond, name: "!", rhs: cond)
  end

  def defLabel(label)
    return SemanticFunctionAST.new(type: :func, name: "defLabel", args: [ label ])
  end

  def setLabel(label)
    return SemanticFunctionAST.new(type: :func, name: "setLabel", args: [ label ])
  end

  def block(stmt_list)
    return SemanticFunctionAST.new(type: :block, list: stmt_list)
  end

  def reset_counters
    @__Label_counts = {}
  end
  def createTmpVar(label_prefix)
    @__Label_counts ||= {}
    @__Label_counts[label_prefix] ||= 0
    @__Label_counts[label_prefix] += 1
    count = @__Label_counts[label_prefix]
    ret = createVar("#{label_prefix}_#{count}")
    # puts ret.inspect
    return ret
  end

  def createStmtListFromArray(array)
    if(array.count > 0)
      return SemanticFunctionAST.new(type: :stmt_list, head: array.shift, tail: createStmtListFromArray(array))
    else
      return SemanticFunctionAST.nothing
    end
  end

end
