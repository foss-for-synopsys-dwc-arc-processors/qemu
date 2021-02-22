require_relative "Translator.rb"

module TranslatorAST

  include Translator

  def replace_variable(str, variable, replace)
    return str.gsub(/(\$#{variable})([^$a-zA-Z_])/, "#{replace}\\2")
  end

  def replace(stmt_ast, to_do = {})
    match = find_matching_rule(stmt_ast)
    if(match)
      repl = match[:replacement]
      mappings = match[:mappings]

      if(repl.class == SemanticFunctionAST)
        repl.traverse_LR_TB do |ast|
          ast.object.each_pair do |ok1, ov1|
            if(ov1.class == String && ov1 =~ /^\$(.+)$/)
              repl.object[ok1] = mappings[$1]
            end
          end
        end

        # mappings.each_pair do |k, v|
        #   if(v.class == String)
        #     # repl = repl.gsub(/(\$#{k})([^a-zA-Z_]+)/, "#{v}\\2")
        #     repl = replace_variable(repl, k, v)
        #   elsif(v.class == Array)
        #     tmp = v.map { |e| e.debug }.join(", ")
        #     repl = replace_variable(repl, k, tmp)
        #   end
        # end
        return repl
      elsif(repl.class == Proc)
        repl.call(stmt_ast, repl, mappings, to_do)
      elsif(repl == nil)
        # Do nothing
      else
        return "CAN'T REPLACE ELEMENT OF CLASS #{repl.class}"
      end
    else
      ret = "/*\n"
      ret += "FAILED TO MATCH { #{stmt_ast.debug }}\n"
      ret += " -----------------------\n"
      ret += stmt_ast.inspect
      ret += "\n -----------------------\n"
      ret += "*/"
      return ret
    end
  end

  def generate_for_stmt(ast, to_do)
    object = ast.object
    case(object[:type])
    when :assign, :func, :if
      # puts ast.inspect
      # puts "HERE at #{object[:type]} (#{ast.debug})"
      tmp = replace(ast, to_do)
      ret = false
    when :stmt_list, :block
      ret = true
    else
      # puts "Stopping at #{object[:type]}"
      ret = false
    end
    return ret
  end

  def generate(full_ast)
    result = ""
    full_ast.traverse_LR_TB do |ast, to_do|
      ret = generate_for_stmt(ast, to_do)
      ret
    end
    return result
  end

end
