class UnfoldCode
  private

  # extend SemanticFunctionASTFactory
  include Pass
  include ConstantTables
  extend TranslatorAST

  def self.translation_rules
    ret = {}

    match = Proc.new { |ast|
      ret = { result: false, mappings: {} }
      # ret = { result: true, mappings: {} } if(ast.hasAttr?(static))
      ret = { result: true, mappings: {} } if(ast.object[:type] == :if)
      ret
    }
    ret[match] = Proc.new { |stmt_ast, repl, mappings, to_do|
      case(stmt_ast.object[:type])
      when :if
        self.generate(stmt_ast.object[:then])
        self.generate(stmt_ast.object[:else])
      end
    }

    # match = Proc.new { |ast|
    #   ret = { result: false, mappings: {} }
    #   ret = {result: true, mappings: {} } if (ast.object[:type] == :variable && ast.object[:name] =~ /^(true|false)$/ && !ast.hasAttr?(:static))
    # }
    # ret[match] = Proc.new { |ast|
    #   return SemanticFunctionAST.variable("arc_#{ast.object[:name]}")
    # }

    match = SemanticFunctionAST.new(type: :assign,
                                    lhs: SemanticFunctionAST.var("a"),
                                    rhs: SemanticFunctionAST.new(type: :binop, name: "_"))

    # match = Proc.new { |ast|
    #   ret = { result: false, mappings: {} }
    #   # ret = { result: true, mappings: {} } if(ast.hasAttr?(static))
    #   if(ast.object[:type] == :assign &&
    #      ast.object[:rhs].object[:type] != :func &&
    #      ast.object[:rhs].object[:type] != :var &&
    #      ast.object[:rhs].object[:type] != :number)
    #       ret = { result: true, mappings: {} }
    #   end
    #   ret
    # }


    def self.binOpProcess(stmt, repl, mappings, to_do)
      # puts "STMT = #{stmt.debug}"

      binop_lhs = stmt.object[:rhs].object[:lhs]
      binop_rhs = stmt.object[:rhs].object[:rhs]
      changed_lhs = false
      changed_rhs = false

      if(binop_lhs.object[:type] != :var && binop_lhs.object[:type] != :number)
        # puts "IN 1 #{binop_lhs.inspect}"
        var = SemanticFunctionAST.createTmpVar("temp")
        tmp = [
          # SemanticFunctionAST.function("createTmpVar", var),
          assign = SemanticFunctionAST.new(type: :assign, lhs: var, rhs: binop_lhs)
        ]
        to_do[:pre_pend] = tmp + to_do[:pre_pend]
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
    end
                                    # puts "IAMHERE"
    ret[match] =
      Proc.new { |stmt, repl, mappings, to_do|
        self.binOpProcess(stmt, repl, mappings, to_do)
      }


    # Do not convert special IF function
    func_match =
    Proc.new { |stmt, repl, mappings, to_do|
      changed = false
      new_stmt = nil
      # puts "BLA => #{stmt.debug}"
      args = stmt.object[:args]
      new_args = []
      args.each_with_index do |arg, i|
        new_args[i] = arg

        if(arg.object[:type] != :var) # && arg.object[:type] != :number)
          var = SemanticFunctionAST.createTmpVar("temp")
          # arg_var = self.replace(arg.clone, to_do)
          # puts arg_var.inspect
          tmp = [
            # SemanticFunctionAST.function("createTmpVar", var),
            assign = SemanticFunctionAST.new(type: :assign, lhs: var, rhs: arg)
          ]
          to_do[:pre_pend] = tmp + to_do[:pre_pend]
          self.replace(assign, to_do)
          arg = var
          new_args[i] = var
          changed = true
        end
      end

      if(changed)
        new_stmt = SemanticFunctionAST.function(stmt.object[:name], *new_args)
        to_do[:pre_pend].push(new_stmt)
        stmt.object[:type] = :nothing
      end

      new_stmt
    }

    assign_func_match =
      Proc.new { |stmt, repl, mappings, to_do|
        # puts "FUNC_MATCH"
        lhs = stmt.object[:lhs]
        rhs = stmt.object[:rhs]

        if(lhs.object[:type] == :var &&
            rhs.object[:type] == :func && TEMP_CREATING_FUNCTIONS.index(rhs.object[:name]))
          # puts "INSIDE"
            var = SemanticFunctionAST.createTmpVar("temp")
            assign = SemanticFunctionAST.assign(var, rhs)
            new_stmt = func_match.call(rhs, repl, mappings, to_do)
            if(new_stmt != nil)
              assign = SemanticFunctionAST.assign(var, to_do[:pre_pend].pop)
            end
            to_do[:pre_pend].push(assign)
            assign = SemanticFunctionAST.assign(lhs, var)
            to_do[:pre_pend].push(assign)

            stmt.object[:type] = :nothing
        else
          new_stmt = func_match.call(stmt.object[:rhs], repl, mappings, to_do)
          if(new_stmt != nil)
            new_stmt = SemanticFunctionAST.assign(stmt.object[:lhs], to_do[:pre_pend].pop)
            to_do[:pre_pend].push(new_stmt)
            stmt.object[:type] = :nothing
          end
        end
      }


    ret[SemanticFunctionAST.function("IF", SemanticFunctionAST.var("..."))] = nil
    ret[SemanticFunctionAST.function("_", SemanticFunctionAST.var("..."))] = func_match

    match = Proc.new { |ast|
      ret = { result: false, mappings: {} }
      # ret = { result: true, mappings: {} } if(ast.hasAttr?(static))
      if(ast.object[:type] == :assign && ast.object[:rhs].object[:type] == :func)
        ret = { result: true, mappings: {} }
      end
      ret
    }
    ret[match] = assign_func_match


    assign_cond_match =
    Proc.new { |stmt, repl, mappings, to_do|
      # puts " BINCOND = #{stmt.pp}"

      cond = stmt.object[:rhs]
      changed = false

      if(cond.object[:type] == :bincond)
        elems = { lhs: cond.object[:lhs], rhs: cond.object[:rhs] }
      elsif(cond.object[:type] == :unicond)
        elems = { rhs: cond.object[:rhs] }
      end

      elems.clone.each_pair do |k, v|
        if(v.object[:type] == :func && TEMP_CREATING_FUNCTIONS.index(v.object[:name]))
          var = SemanticFunctionAST.createTmpVar("temp")
          assign = SemanticFunctionAST.assign(var, v)
          new_stmt = func_match.call(v, repl, mappings, to_do)
          if(new_stmt != nil)
            assign = SemanticFunctionAST.assign(var, to_do[:pre_pend].pop)
          end
          to_do[:pre_pend].push(assign)
          elems[k] = var
          changed = true
        end
      end

      if(changed == true)
        if(cond.object[:type] == :bincond)
          new_cond = SemanticFunctionAST.bincond(cond.object[:name], elems[:lhs], elems[:rhs])
          new_stmt = SemanticFunctionAST.assign(stmt.object[:lhs], new_cond)
        elsif(cond.object[:type] == :unicond)
          new_cond = SemanticFunctionAST.unicond(cond.object[:name], elems[:rhs])
          new_stmt = SemanticFunctionAST.assign(stmt.object[:lhs], new_cond)
        end
        to_do[:pre_pend].push(new_stmt)
        stmt.object[:type] = :nothing
      end
    }
    match = Proc.new { |ast|
      ret = { result: false, mappings: {} }
      # ret = { result: true, mappings: {} } if(ast.hasAttr?(static))
      if(ast.object[:type] == :assign && ast.object[:rhs].object[:type] == :bincond)
        ret = { result: true, mappings: {} }
      end
      if(ast.object[:type] == :assign && ast.object[:rhs].object[:type] == :unicond)
        ret = { result: true, mappings: {} }
      end
      ret
    }
    ret[match] = assign_cond_match


    if_cond_match =
    Proc.new { |stmt, repl, mappings, to_do|
      # puts " BINCOND = #{stmt.pp}"
      cond = stmt.object[:cond]

      changed = false
      elems = { cond: bincond.object[:cond] }

      elems.clone.each_pair do |k, v|
        if(v.object[:type] == :func && TEMP_CREATING_FUNCTIONS.index(v.object[:name]))
          var = SemanticFunctionAST.createTmpVar("temp")
          assign = SemanticFunctionAST.assign(var, v)
          new_stmt = func_match.call(v, repl, mappings, to_do)
          if(new_stmt != nil)
            assign = SemanticFunctionAST.assign(var, to_do[:pre_pend].pop)
          end
          to_do[:pre_pend].push(assign)
          elems[k] = var
          changed = true
        end
      end

      if(changed == true)
        new_bincond = SemanticFunctionAST.bincond(bincond.object[:name], elems[:lhs], elems[:rhs])
        new_stmt = SemanticFunctionAST.assign(stmt.object[:lhs], new_bincond)
        to_do[:pre_pend].push(new_stmt)
        stmt.object[:type] = :nothing
      end
    }
    match = Proc.new { |ast|
      ret = { result: false, mappings: {} }
      # ret = { result: true, mappings: {} } if(ast.hasAttr?(static))
      if(ast.object[:type] == :if)
        ret = { result: true, mappings: {} }
      end
      ret
    }
    ret[match] = if_cond_match


    # match = SemanticFunctionAST.new(type: :assign,
    #                                 lhs: SemanticFunctionAST.var("a"),
    #                                 rhs: SemanticFunctionAST.function("_", SemanticFunctionAST.var("...")))
    # ret[match] = Proc.new { |stmt, repl, mappings, to_do|
    #   # puts "BLA -------------"
    #   # puts stmt.debug
    #   # puts " ------------- "
    #
    #   new_func = func_match.call(stmt.object[:rhs], repl, mappings, to_do)
    #
    #   if(new_func != nil)
    #     new_stmt = SemanticFunctionAST.assign(stmt.object[:lhs], new_func)
    #     to_do[:pre_pend].push(new_stmt)
    #     stmt.object[:type] = :nothing
    #   end
    # }

    return ret

  end


  public
  def self.task(ast)
    # SemanticFunctionAST.reset_counters()
    self.generate(ast)
    # puts "AST = #{ast.class}"
    # puts ast.debug
    return ast
  end
end
