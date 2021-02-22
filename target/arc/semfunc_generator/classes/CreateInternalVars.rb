class CreateInternalVars
  private

  # extend SemanticFunctionASTFactory
  include Pass
  include ConstantTables
  extend TranslatorAST



  def self.translation_rules
    ret = {}

    create_var = Proc.new { |stmt, repl, mappings, to_do|

      var = stmt.object[:lhs]
      rhs = stmt.object[:rhs]
      if(@@vars[var.object[:name]].nil? && var.object[:name] !~ /@.+/)
        # puts "VAR = #{var.object[:name]}"
        if(var.hasAttr?(:static))
          defVar = SemanticFunctionAST.function("defStaticVariable", stmt.getAttr(:static), stmt.object[:lhs])
        else
	  #puts "NAME = #{rhs.object.inspect}"
	  #puts "L = #{LIST_OF_FUNCTIONS.index(rhs.object[:name])}"


	  defVar = SemanticFunctionAST.function("defVariable", stmt.object[:lhs])
	  #if(rhs.object[:type] == :func && !TEMP_CREATING_FUNCTIONS.index(rhs.object[:name]).nil?)
	  #  #puts " IN HERE"
	  #  stmt.object[:lhs].setAttr(:reference, true)
          #  defVar = SemanticFunctionAST.function("defReference", stmt.object[:lhs])
	  #else
	  #  defVar = SemanticFunctionAST.function("defVariable", stmt.object[:lhs])



	  #end
        end
        # to_do[:pre_pend].push(defVar)
        @@vars[var.object[:name]] = defVar
        # @@vars1[var.object[:name]] = {
        #   defVar: defVar,
        #   stmt: stmt,
        #   block: stmt.find_parent_node_with_type(:block)
        # }
      # elsif(@@vars[var.object[:name]].nil? && var.object[:name] =~ /@.+/)
      end
    }

    match = SemanticFunctionAST.new(type: :assign,
                                    lhs: SemanticFunctionAST.var("a"),
                                    rhs: SemanticFunctionAST.var("_"))
    ret[match] = create_var

    match = Proc.new { |ast|
      ret = { result: false, mappings: {} }
      ret = { result: true, mappings: {} } if(ast.object[:type] == :if)
      ret
    }
    ret[match] = Proc.new { |stmt_ast, repl, mappings, to_do|
      generate(stmt_ast.object[:then])
      generate(stmt_ast.object[:else])
      nil
    }

    return ret
  end


  public
  def self.task(ast)
    @@vars = {}
    # @@vars1 = {}
    self.generate(ast)
    # puts "AST = #{ast.class}"
    # puts ast.debug

    # NOTE: Add free variables to end of semfunc.
    new_stmt_list = SemanticFunctionAST.createStmtListFromArray(@@vars.values)
    new_stmt_list.append_in_stmt_list(ast)
    ast = new_stmt_list

    list = []
    @@vars.each_pair do |var_name, func|

      # puts "VAR: #{var_name}"
      # puts "STMT: #{@@vars1[var_name][:stmt]}"
      # puts "BLOCK:\n#{@@vars1[var_name][:block].class}"

      # list.push(var_name) if func.object[:type] == :func &&
      #                        (func.object[:name] == "defVariable" ||
      #                         func.object[:name] == "defReference")

      if(func.object[:type] == :func)
        if(func.object[:name] == "defVariable")
          list.push(SemanticFunctionAST.function("freeVariable", SemanticFunctionAST.var(var_name)))
        elsif(func.object[:name] == "defReference")
          list.push(SemanticFunctionAST.function("freeReference", SemanticFunctionAST.var(var_name)))
        end
      end
    end
    # list = list.map do |var_name|
    #   SemanticFunctionAST.function("freeVariable", SemanticFunctionAST.var(var_name))
    # end
    stmt_list = SemanticFunctionAST.createStmtListFromArray(list)
    ast.append_in_stmt_list(stmt_list)

    # @@vars.each_pair do |k, v|
    #   puts "FREE: #{k} : #{v.pp}"
    #   ast.prepend_in_stmt_list(SemanticFunctionAST.function("freeVariable", SemanticFunctionAST.createVar(k)))
    # end

    # puts ast.pp

    return ast
  end
end
