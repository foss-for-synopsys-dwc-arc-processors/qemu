class QEmuTranslator

  extend TranslatorFinal
  include ConstantTables

  def self.translation_rules
    ret = {
      SemanticFunctionAST.function("defLabel", SemanticFunctionAST.var("name")) =>
        "TCGLabel *$name = gen_new_label()",
      SemanticFunctionAST.function("setLabel", SemanticFunctionAST.var("name")) =>
        "gen_set_label($name)",
      SemanticFunctionAST.function("createTmpVar", SemanticFunctionAST.var("name")) =>
        "TCGv $name = tcg_temp_new()",
      SemanticFunctionAST.function("defVariable", SemanticFunctionAST.var("name")) =>
        "TCGv $name = tcg_temp_local_new()",
      SemanticFunctionAST.function("freeVariable", SemanticFunctionAST.var("name")) =>
        "tcg_temp_free($name)",
      SemanticFunctionAST.function("freeReference", SemanticFunctionAST.var("name")) =>
        "if($name != NULL) tcg_temp_free($name)",
      SemanticFunctionAST.function("defReference", SemanticFunctionAST.var("name")) =>
        "TCGv $name = NULL /* REFERENCE */",

      SemanticFunctionAST.assign(
        SemanticFunctionAST.var("a"),
        SemanticFunctionAST.function("HELPER",
                                     SemanticFunctionAST.var("helper"),
                                     SemanticFunctionAST.var("..."))) =>
        "ARC_HELPER($helper, $a, $varargs_)",
      SemanticFunctionAST.function("HELPER",
                                   SemanticFunctionAST.var("helper"),
                                   SemanticFunctionAST.var("...")) =>
      "ARC_HELPER($helper, NULL, $varargs_)",

      SemanticFunctionAST.function("goto", SemanticFunctionAST.var("label")) => "tcg_gen_br($label)",

      # SemanticFunctionAST.function("_func", SemanticFunctionAST.var("a")) => "$func($a)",
      # SemanticFunctionAST.function("_func", SemanticFunctionAST.number("1")) => "$func($1)",

      SemanticFunctionAST.parse_stmt("a = b") => "tcg_gen_mov_tl($a, $b)",
      SemanticFunctionAST.parse_stmt("a = 1") => "tcg_gen_movi_tl($a, $1)",
    }

    match = SemanticFunctionAST.function("defStaticVariable", SemanticFunctionAST.var("type"), SemanticFunctionAST.var("name"))
    ret[match] = Proc.new { |stmt_ast, repl, mappings, to_do|
      ret = "#{mappings["type"]} #{mappings["name"]}"
      if mappings["type"] == "TCGvPtr"
        ret = "TCGv #{mappings["name"]} = NULL"
      end
      if mappings["type"] == "TCGv"
        ret = "TCGv #{mappings["name"]} = tcg_temp_local_new()"
      end
      ret
    }

    match = Proc.new { |ast|
      ret = { result: false, mappings: {} }
      ret = { result: true, mappings: {} } if(ast.object[:type] == :if)
      ret
    }
    ret[match] = Proc.new { |stmt_ast, repl, mappings, to_do|
      tmp =  "  if (#{stmt_ast.object[:cond].pp}) {\n"
      tmp += @translator.generate(stmt_ast.object[:then], false)
      tmp += "    }\n"
      if(stmt_ast.object[:else].valid?)
        tmp += "  else {\n"
        tmp += @translator.generate(stmt_ast.object[:else], false)
        tmp += "    }\n"
      end
      return tmp
    }

    # Any other static statement.
    match = Proc.new { |ast|
      ret = { result: false, mappings: {} }
      ret = { result: true, mappings: {} } if(ast.hasAttr?(:static))
      ret
    }
    ret[match] = Proc.new { |stmt_ast, repl, mappings, to_do|
      tmp =  stmt_ast.pp
      tmp
    }


    {
      "+": "add",
      "-": "sub",
      "*": "mul",
      "/": "div",
      "&": "and",
      "|": "or",
      "^": "xor",
      "<<": "shl",
      ">>": "shr",

    }.each_pair do |k, v|
      ret[SemanticFunctionAST.parse_stmt("a = b #{k} c")] = "tcg_gen_#{v}_tl($a, $b, $c)"
      ret[SemanticFunctionAST.parse_stmt("a = b #{k} 1")] = "tcg_gen_#{v}i_tl($a, $b, $1)"
      ret[SemanticFunctionAST.parse_stmt("a = 2 #{k} c")] = "tcg_gen_#{v}fi_tl($a, $2, $c)"
    end


    options1 = {
      SemanticFunctionAST.var("b") => "$b",
      SemanticFunctionAST.number("1") => "$1",
      SemanticFunctionAST.function("_func1", SemanticFunctionAST.var("...")) => "$func1($varargs_func1)"
    }
    options2 = {
      SemanticFunctionAST.var("c") => "$c",
      SemanticFunctionAST.number("2") => "$2",
      SemanticFunctionAST.function("_func2", SemanticFunctionAST.var("...")) => "$func2($varargs_func2)"
    }


    # Combinations of options
    options1.each_pair do |m1, r1|
      options2.each_pair do |m2, r2|
        next if m1.object[:type] == :number && m2.object[:type] == :number

        reverted_immediate = false
        #Revert immediate value and condition
        if(m1.object[:type] == :number)
          tmp = [m1, r1]
          m1 = m2; r2 = r1
          reverted_immediate = true
          m2 = tmp[0]; r2 = tmp[1]
        end
        type = "i" if(m2.object[:type] == :number)

        {
          "&&": "and",
          "||": "or",
          "^^": "xor",
        }.each_pair do |k, v|
          op = v

          # A = B && C (for example)
          rhs = SemanticFunctionAST.new(type: :bincond, name: k.to_s, lhs: m1.clone, rhs: m2.clone)
          match = SemanticFunctionAST.new(type: :assign, lhs: SemanticFunctionAST.var("a"), rhs: rhs)
          ret[match] = "tcg_gen_#{op}_tl($a, #{r1}, #{r2})"

        end

        {
          "==": "TCG_COND_EQ",
          "!=": "TCG_COND_NE",
          "<": "TCG_COND_LT",
          ">": "TCG_COND_GT",
          "<=": "TCG_COND_LE",
          ">=": "TCG_COND_GE",
        }.each_pair do |k, v|

          op = v
          if(reverted_immediate == true)
            case (k)
            when "<"
              op = "TCG_COND_GE"
            when ">"
              op = "TCG_COND_LE"
            when "<="
              op = "TCG_COND_GT"
            when ">="
              op = "TCG_COND_LT"
            end
          end

          # A = B == C (for example)
          rhs = SemanticFunctionAST.new(type: :bincond, name: k.to_s, lhs: m1.clone, rhs: m2.clone)
          match = SemanticFunctionAST.new(type: :assign, lhs: SemanticFunctionAST.var("a"), rhs: rhs)
          ret[match] = "tcg_gen_setcond#{type}_tl(#{op}, $a, #{r1}, #{r2})"

          # IF(cond, label1, label2) # TODO: Label2 is expected to be equal to label1
          cond = SemanticFunctionAST.new(type: :bincond, name: k.to_s, lhs: m1.clone, rhs: m2.clone)
          ifcond_match = SemanticFunctionAST.function("IF", cond,
            SemanticFunctionAST.var("label1"),
            SemanticFunctionAST.var("label2")
          )
          ret[ifcond_match] = "tcg_gen_brcond#{type}_tl(#{op}, #{r1}, #{r2}, $label1)"
          # Proc.new { |stmt_ast, repl, mappings, to_do|
            # mappings.each_pair do |k, v|
            #   mappings[k] = "arc_#{v}" if (v =~ /^(true|false)$/)
            # end
          # }
        end
      end
    end

    {
      "!": nil,
    }.each_pair do |k, v|
      [:unicond, :uniop].each do |type|
        rhs = SemanticFunctionAST.new(type: type, name: k.to_s, rhs: SemanticFunctionAST.var("b"))
        match = SemanticFunctionAST.new(type: :assign, lhs: SemanticFunctionAST.var("a"), rhs: rhs)
        ret[match] = "tcg_gen_xori_tl($a, $b, 1);\ntcg_gen_andi_tl($a, $a, 1)"

        rhs1 = SemanticFunctionAST.new(type: type, name: k.to_s, rhs: SemanticFunctionAST.function("_func", SemanticFunctionAST.var("...")))
        match1 = SemanticFunctionAST.new(type: :assign, lhs: SemanticFunctionAST.var("a"), rhs: rhs1)
        ret[match1] = "tcg_gen_xori_tl($a, $func($varargs_func), 1);\ntcg_gen_andi_tl($a, $a, 1)"
      end
    end

    {
      "~": "not",
    }.each_pair do |k, v|
      [:unicond, :uniop].each do |type|
        rhs = SemanticFunctionAST.new(type: type, name: k.to_s, rhs: SemanticFunctionAST.var("b"))
        match = SemanticFunctionAST.new(type: :assign, lhs: SemanticFunctionAST.var("a"), rhs: rhs)
        ret[match] = "tcg_gen_#{v}_tl($a, $b)"

        rhs1 = SemanticFunctionAST.new(type: type, name: k.to_s, rhs: SemanticFunctionAST.function("_func", SemanticFunctionAST.var("...")))
        match1 = SemanticFunctionAST.new(type: :assign, lhs: SemanticFunctionAST.var("a"), rhs: rhs1)
        ret[match1] = "tcg_gen_#{v}_tl($a, $func($varargs_func))"
      end
    end

    DIRECT_TCG_FUNC_TRANSLATIONS.each_pair do |f1, f2|
      #ret[SemanticFunctionAST.assign(SemanticFunctionAST.var("a"), SemanticFunctionAST.function(f, SemanticFunctionAST.var("...")))] = "#{f}($a, $varargs_)"
      match = SemanticFunctionAST.assign(SemanticFunctionAST.var("a"), SemanticFunctionAST.function(f1, SemanticFunctionAST.var("...")))
      ret[match] = Proc.new { |stmt_ast, repl, mappings, to_do|
	ret = ""
	if(mappings["varargs_"].class == Array)
	  mappings["varargs_"] = mappings["varargs_"].map { |a| a.debug() }.join(", ")
	end
	if(mappings["varargs_"] =~ /^$/)
	  ret = "#{f2}($a)"
	else
	  ret = "#{f2}($a, $varargs_)"
	end
	ret
      }
    end

    TEMP_CREATING_FUNCTIONS.each do |f|
      #ret[SemanticFunctionAST.assign(SemanticFunctionAST.var("a"), SemanticFunctionAST.function(f, SemanticFunctionAST.var("...")))] = "#{f}($a, $varargs_)"
      match = SemanticFunctionAST.assign(SemanticFunctionAST.var("a"), SemanticFunctionAST.function(f, SemanticFunctionAST.var("...")))
      ret[match] = Proc.new { |stmt_ast, repl, mappings, to_do|
	ret = ""
	if(mappings["varargs_"].class == Array)
	  mappings["varargs_"] = mappings["varargs_"].map { |a| a.debug() }.join(", ")
	end
	if(mappings["varargs_"] =~ /^$/)
	  ret = "#{f}($a)"
	else
	  ret = "#{f}($a, $varargs_)"
	end
	ret
      }
    end

    ret[SemanticFunctionAST.assign(SemanticFunctionAST.var("a"), SemanticFunctionAST.function("_func", SemanticFunctionAST.var("...")))] =
      "tcg_gen_mov_tl($a, $func($varargs_func))"

    #  "$a = $func($varargs_func)"
    #ret[SemanticFunctionAST.assign(SemanticFunctionAST.var("a"), SemanticFunctionAST.function("_func", SemanticFunctionAST.var("...")))] = Proc.new { |stmt_ast, repl, mappings, to_do|
    #  lhs = stmt_ast.object[:lhs]
    #  if(lhs.hasAttr?(:reference))
    #    tmp =  stmt_ast.pp
    #  else
    #    tmp = "tcg_gen_mov_tl()"
    #  end
    #  tmp = "tcg_gen_mov_tl()"
    #  tmp
    #}
    ret[SemanticFunctionAST.function("_func", SemanticFunctionAST.var("..."))] = "$func($varargs_func)"



    return ret
  end
end
