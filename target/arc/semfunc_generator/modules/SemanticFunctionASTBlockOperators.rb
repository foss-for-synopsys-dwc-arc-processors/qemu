module SemanticFunctionASTBlockOperators

  def prepend_in_stmt_list(elem, parents = {})
    parent = parents[self]
    # puts self
    # parent = parents[parent] if parent.object[:type] != :stmt_list && parent.object[:type] != :block

    if(parent != nil)
      new_stmt_list = SemanticFunctionAST.new(type: :stmt_list, head: elem, tail: self)
      parents[new_stmt_list] = parent
      parents[self] = new_stmt_list

      parent.object.each_pair do |k, v|
        parent.object[k] = new_stmt_list if(v == self)
      end
    end
  end

  def append_in_stmt_list(stmt_list, parents = {})
    return stmt_list if self.object[:type] == :nothing
    raise "self is not of type :stmt_list\n#{self.inspect}" if self.object[:type] != :stmt_list

    if(self.object[:tail].object[:type] == :nothing)
      self.object[:tail] = stmt_list
    else
      self.object[:tail].append_in_stmt_list(stmt_list, parents)
    end
  end

  def remove_from_stmt_list(parents = {})
    elem = self.find_parent_node_with_type(:stmt_list, parents)
    # puts parents.inspect
    parent = parents[elem]
    puts "BLING => #{parent.debug}"
    parents.each_pair do |k, v|
      puts "#{k.debug} => #{v.debug}"
    end
    puts parent
    parent.object.each_pair do |k, v|
      if(v == elem)
        parent.object[k] = elem.object[:tail]
        parents[parent.object[k]] = parent
      end
    end
  end

  def create_stmts_for_expression(tmp_vars = {})
    ret = []
    self.object.each_pair do |k, v|
      if(v.class == SemanticFunctionAST && v.valid?)
        ret += v.create_stmts_for_expression(tmp_vars)
      end
    end

    case(object[:type])
    when :binop
    when :bincond
      var = SemanticFunctionAST.createTmpVar("temp")
      rhs = SemanticFunctionAST.new(type: object[:type], name: object[:name], lhs: tmp_vars[object[:lhs]], rhs: tmp_vars[object[:rhs]])
      ret += [
        # SemanticFunctionAST.function("createTmpVar", var),
        SemanticFunctionAST.new(type: :assign, lhs: var, rhs: rhs)
      ]
      tmp_vars[self] = var
    when :uniop
    when :unicond
      var = SemanticFunctionAST.createTmpVar("temp")
      rhs = SemanticFunctionAST.new(type: object[:type], name: object[:name], rhs: tmp_vars[object[:rhs]])
      ret += [
        # SemanticFunctionAST.function("createTmpVar", var),
        SemanticFunctionAST.new(type: :assign, lhs: var, rhs: rhs)
      ]
      tmp_vars[self] = var
    else
      tmp_vars[self] = self
    end
    return ret
  end


  def traverse_LR_TB(to_do = {}, parents = {}, &block)
    to_do[self] ||= { pre_pend: [], post_pend: [], remove: false }

    do_childs = yield self, to_do[self]

    if(do_childs == true)
      @object.each_pair do |k, e|
        if(e.class == SemanticFunctionAST)
          if(self.object[:type] == :stmt_list || self.object[:type] == :block)
            parents[e] = self
          else
            parents[e] = parents[self]
          end
          e.traverse_LR_TB(to_do, parents, &block)
        end
      end
    end

    # If it is back to the head of the recursion
    if(parents[self] == nil)
      to_do.each_pair do |elem, to_do|
        to_do[:pre_pend].each do |elem1|
          elem.prepend_in_stmt_list(elem1, parents)
        end

        if(to_do[:remove] == true)
          elem.remove_from_stmt_list(parents)
        end
      end
    end
    return self
  end

  def traverse_LR_BT(to_do = {}, parents = {}, &block)
    to_do[self] ||= { pre_pend: [], post_pend: [], remove: false }

    @object.each_pair do |k, e|
      if(e.class == SemanticFunctionAST)
        if(self.object[:type] == :stmt_list || self.object[:type] == :block)
          parents[e] = self
        else
          parents[e] = parents[self]
        end
        e.traverse_LR_BT(to_do, parents, &block)
      end
    end

    yield self, to_do[self]

    # If it is back to the head of the recursion
    if(parents[self] == nil)
      to_do.each_pair do |elem, to_do|
        to_do[:pre_pend].each do |elem1|
          elem.prepend_in_stmt_list(elem1, parents)
        end

        if(to_do[:remove] == true)
          elem.remove_from_stmt_list(parents)
        end
      end
    end
    return self
  end

end
