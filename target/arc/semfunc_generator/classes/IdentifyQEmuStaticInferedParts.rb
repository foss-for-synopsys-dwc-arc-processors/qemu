class IdentifyQEmuStaticInferedParts
  private

  # extend SemanticFunctionASTFactory
  include Pass
  extend Translator

  public

  rules = {}
  def self.translation_rules
    ret = {}
    functions = {
      "shouldExecuteDelaySlot" => "bool",
      "getAAFlag" => "int",
      "getZZFlag" => "int",
      "getFFlag" => "int",
      "getFlagX" => "bool",
      "nextReg" => "TCGvPtr",
      "instructionHasRegisterOperandIn" => "bool",
      "targetHasOption" => "bool",
      "Ext64" => "TCGv_i64",
      "SignExt64" => "TCGv_i64",
      "SignExtend" => "TCGv",
      "getNFlag" => "TCGv",
      "Zero" => "TCGv",
    }.each_pair do |name, type|
      ret[SemanticFunctionAST.function(name)] = SemanticFunctionAST.var(type)
    end
    # ret[SemanticFunctionAST.function("shouldExecuteDelaySlot")] = SemanticFunctionAST.var("bool")
    # ret[SemanticFunctionAST.function("getAAFlag")] = SemanticFunctionAST.var("int")
    # ret[SemanticFunctionAST.function("getZZFlag")] = SemanticFunctionAST.var("int")
    return ret;
  end
  def self.task(ast)
    static_variables = {}

    ast.traverse_LR_BT do |ast|

      rules = {
      }

      match = self.find_matching_rule(ast)
      if(match)
        if(match[:replacement])
          ast.setAttr(:static, match[:replacement])
        end
      else
        case(ast.object[:type])
        when :var
          name = ast.object[:name]
          ast.setAttr(:static, static_variables[name]) if static_variables[name] != nil && ast.object[:name] !~ /@.+/
        when :assign
          name = ast.object[:lhs].object[:name]
          if(ast.object[:rhs].hasAttr?(:static))
            ast.object[:lhs].setAttr(:static, ast.object[:rhs].getAttr(:static))
            ast.setAttr(:static, ast.object[:rhs].getAttr(:static))
            static_variables[name] = ast.object[:rhs].getAttr(:static) if static_variables[name].nil?
          end
        when :if
          ast.setAttr(:static, ast.object[:cond].getAttr(:static)) if(ast.object[:cond].hasAttr?(:static))
        when :unicond
          ast.setAttr(:static, ast.object[:rhs].getAttr(:static)) if(ast.object[:rhs].hasAttr?(:static))
        when :bincond
          if(ast.object[:lhs].hasAttr?(:static) && ast.object[:rhs].hasAttr?(:static))
            # TODO: Static elements might not have same type. Create a warning
            ast.setAttr(:static, ast.object[:lhs].getAttr(:static))

          elsif(ast.object[:lhs].hasAttr?(:static))
            tmp = ast.object[:rhs]
            if(tmp.object[:type] == :number || tmp.object[:type] == :var )
              tmp.setAttr(:static, ast.object[:lhs].getAttr(:static))
              ast.setAttr(:static, ast.object[:lhs].getAttr(:static))
            end

            # TODO: Verify if other conditions are possible as well.
            # For example, verify if bincond for static and non static conjunctions.
            # Currently no validation is being performed.
          elsif(ast.object[:rhs].hasAttr?(:static))
            tmp = ast.object[:lhs]
            if(tmp.object[:type] == :number || tmp.object[:type] == :var )
              tmp.setAttr(:static, ast.object[:rhs].getAttr(:static))
              ast.setAttr(:static, ast.object[:rhs].getAttr(:static))
            end
          end
        end
      end
      false
    end
  end
end
