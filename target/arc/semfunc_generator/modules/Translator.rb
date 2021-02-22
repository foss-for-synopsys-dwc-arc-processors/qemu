module Translator

  # def map_for_variable
  #   if(rule_pattern.object[:name] =~ /^_(.*)$/)
  #     name = $1
  #     if name == ""
  #       name = "unname#{unname}"
  #       unname += 1
  #     end
  #     return name
  #   end

  # Function that verifies is AST is compatible with AST Pattern
  # The function results a hash of the mapping of variables and number elements
  # the rule pattern
  def is_a_match(ast, rule_pattern, unname = 1)
    ret = { result: true, mappings: {} }

    if(rule_pattern.class == Proc)

      return rule_pattern.call(ast)
    end

    return ret if(rule_pattern.class == TrueClass)
    ret[:result] &= false if(rule_pattern == nil || ast == nil)

    if(ret[:result] == false)
      return ret
    end

    if(rule_pattern.object[:name] =~ /^_(.*)$/)
      name = $1
      if name == ""
        name = "unname#{unname}"
        unname += 1
      end

      ret[:result] &= false if(ast.object[:type] != rule_pattern.object[:type]) if(rule_pattern.object[:type] != :var)
      ret[:mappings][name] = ast.object[:name]
    else
      ret[:result] &= false if(ast.object[:type] != rule_pattern.object[:type])

  # if(ast.object[:type] == :func)
      if(ast.object[:name].class == String && ast.object[:type] != :var)
        # puts "NOW THIS"
        # puts ast.debug
        # puts rule_pattern.inspect
        ret[:result] = false if (ast.object[:name] != rule_pattern.object[:name])
      end

      if(ast.object[:type] == :var)
        ret[:mappings][rule_pattern.object[:name]] = ast.object[:name]
      elsif(ast.object[:type] == :number)
        ret[:mappings][rule_pattern.object[:number]] = ast.object[:number].to_s
      end
    end

    # puts "RULE = #{rule_pattern.debug}"
    # puts "AST  = #{ast.debug}"

    if(ret[:result] == true)

      if(rule_pattern.object[:type] == :func)
        rule_pattern.object[:args].each_with_index do |arg_rule, i|
          if(arg_rule.object[:type] == :var && arg_rule.object[:name] == "...")
            # puts ast.inspect
            ret[:mappings]["varargs_#{name}"] = ast.object[:args][i..-1] || []
          elsif(ast.object[:args][i])
            tmp = is_a_match(ast.object[:args][i], arg_rule, unname)
            ret[:result] &= tmp[:result]
            ret[:mappings].merge!(tmp[:mappings])
          else
            ret[:result] &= false
          end
        end
      end

      rule_pattern.object.each_pair do |k, v|
        if(v.class == SemanticFunctionAST)
          tmp = is_a_match(ast.object[k], v, unname)
          ret[:result] &= tmp[:result]
          ret[:mappings].merge!(tmp[:mappings])
        end
      end
    end

    return ret
  end

  def find_matching_rule(ast)
    rules = self.translation_rules

    rules.each_pair do |k, v|
      tmp = is_a_match(ast, k)
      if(tmp[:result] == true)
        return { replacement: v, mappings: tmp[:mappings], index: rules.keys.index(k) }
      end
    end
    return nil
  end

end
