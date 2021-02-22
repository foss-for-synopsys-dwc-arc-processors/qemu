require_relative 'Translator.rb'

module TranslatorFinal

  include Translator

  def replace_variable(str, variable, replace)
    # puts "REPLACE #{str}. #{variable}, #{replace}"
    replace = replace.gsub("@", "")
    #  = $1 if replace =~ /\@(.+)/
    return str.gsub(/(\$#{variable})([^$a-zA-Z_])?/, "#{replace}\\2")
  end

  def replace(stmt_ast, to_do = {}, debug = true)
    ret = ""
    match = find_matching_rule(stmt_ast)
    if(match)
      repl = match[:replacement]
      mappings = match[:mappings]

      ret += "  // Rule with index #{match[:index]}\n" if debug == true

      if(repl.class == String)
        mappings.each_pair do |k, v|

          if(v.class == String)
            # puts v.inspect
            # puts "TRUE" if(v =~ /(true|false)/)

            # repl = repl.gsub(/(\$#{k})([^a-zA-Z_]+)/, "#{v}\\2")
            # if(!stmt_ast.hasAttr?(:static))
              v = "arc_#{v}" if (v =~ /(true|false)/)
            # end
            repl = replace_variable(repl, k, v)

          elsif(v.class == Array)
            # puts "STMT_AST = #{stmt_ast.pp}"
            tmp = v.map { |e| e.debug }.join(", ")
            repl = replace_variable(repl, k, tmp)
          end
        end
        return ret + repl
      elsif(repl.class == Proc)
        repl = repl.call(stmt_ast, repl, mappings, to_do)
        if(repl.class == String)
          if(!stmt_ast.hasAttr?(:static))
            v = "arc_#{v}" if (v =~ /(true|false)/)
          end
          mappings.each_pair { |k, v| repl = replace_variable(repl, k, v) }
          ret += repl
        else
          ret += "RESULT SHOULD BE A String ELEMENT."
        end
        return ret
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

  def generate(full_ast, debug = false)
    result = ""
    full_ast.traverse_LR_TB do |ast, to_do|
      ret = true
      object = ast.object
      case(object[:type])
      when :if
        result +=  "  if (#{object[:cond].pp}) {\n"
        tmp = generate(object[:then], debug)
        result += "  #{tmp};\n"
        result += "    }\n"
        if(object[:else].valid?)
          result += "  else {\n"
          tmp = generate(object[:else], debug)
          result += "  #{tmp};\n"
          result += "    }\n"
        end
        ret = false
      when :assign, :func
        # puts "HERE at #{object[:type]} (#{ast.debug})"
        tmp = replace(ast, to_do, debug)
        result += "  #{tmp};\n"
        ret = false
      when :stmt_list, :block
        ret = true
      else
        # puts "Stopping at #{object[:type]}"
        ret = false
      end
      ret
    end
    return result
  end

end
