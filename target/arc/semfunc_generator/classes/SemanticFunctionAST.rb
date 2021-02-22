class SemanticFunctionAST

  include Enumerable
  extend SemanticFunctionASTFactory
  include SemanticFunctionASTBlockOperators

  def each(&block)
    yield self
    @object.each_pair do |k, e|
      if(e.class == SemanticFunctionAST)
        e.each(&block)
      end
    end
  end

  def find_parent_node_with_type(type, parents = {})
    if(self.object[:type] == type)
      return self
    else
      if(parents[self] != nil)
        parents[self].find_parent_node_with_type(type, parents)
      else
        return nil
      end
    end
  end


  def initialize(params)
    @object = params
  end

  def clone
    new_elem = SemanticFunctionAST.new({})
    self.object.each_pair do |k, v|
      if(v.class == SemanticFunctionAST)
        new_elem.object[k] = v.clone
      elsif v.class == Array
        new_elem.object[k] = Array.new
        v.each_with_index do |e, i|
          new_elem.object[k][i] = e.clone
        end
      else
        begin
          new_elem.object[k] = v.clone
        rescue
          new_elem.object[k] = v
        end
      end
    end
    return new_elem
  end

  def self.error(string)
    SemanticFunctionAST.new({ type: 'error', message: string })
  end

  def self.nothing
    SemanticFunctionAST.new({ type: :nothing })
  end

  def self.var(name)
    SemanticFunctionAST.new({ type: :var, name: name })
  end

  def self.number(number)
    SemanticFunctionAST.new({ type: :number, number: number })
  end

  def self.assign(lhs, rhs)
    return SemanticFunctionAST.new({ type: :assign, lhs: lhs, rhs: rhs })
  end

  def self.function(name, *args)
    return SemanticFunctionAST.new({ type: :func, name: name, args: args || [] })
  end

  def self.bincond(name, lhs, rhs)
    return SemanticFunctionAST.new({ type: :bincond, name: name, lhs: lhs, rhs: rhs })
  end

  def self.unicond(name, rhs)
    return SemanticFunctionAST.new({ type: :unicond, name: name, rhs: rhs })
  end

  def self.parse(string)
    #puts "Parsing: #{string}"
    SemanticFunctionParser.new.parse(string)
  end
  def self.parse_stmt(str)
    ast = self.parse(str)
    return ast.object[:list].object[:head]
  end

  def getAttr(name)
    return nil if(object[:attrs] == nil || object[:attrs][name] == nil)
    return object[:attrs][name]
  end
  def hasAttr?(name)
    getAttr(name) != nil
  end
  def setAttr(name, value)
    object[:attrs] = object[:attrs] || {}
    object[:attrs][name] = value
  end

  def valid?
    @object[:type] != :nothing
  end
  def shouldSeparate?
    # return true
    return (@object[:type] != :block)
  end

  def object
    return @object
  end

  def traverse(data, &block)
    cont = yield @object, data
    if(cont)
      @object.each_pair do |k,v|
        v.traverse(data, &block) if v.class == SemanticFunctionAST
        v.each { |v1| v1.traverse(data, &block) } if v.class == Array
      end
    end
  end

  def graphviz(filename = "/tmp/tmp.png", g = nil, parent = nil)
    first_call = false
    if(g == nil)
      first_call = true
      g = GraphViz.new( :G, :type => :digraph )
    end

    label = []

    node = g.add_nodes("n#{g.node_count}")
    @object.each_pair do |k,v|
      if v.class == SemanticFunctionAST
        v.graphviz(filename, g, node)
      elsif v.class == Array
        v.each do |v1|
          v1.graphviz(filename, g, node)
        end
      else
        label.push("#{k}: #{v}")
      end
    end
    node[:label] = label.join("\n")
    g.add_edges(parent, node) if(parent != nil)

    if(first_call)
      g.output(:png => filename)
    end
  end

  def traverseASTWithMethodName(data, method_name)
    cont = self.send(method_name, data)
    if(cont)
      @object.each_pair do |k,v|
        v.traverseASTWithMethodName(data, method_name) if v.class == SemanticFunctionAST
      end
    end
  end

  def SemanticFunctionAST.IF(cond, ifthen, ifelse)
    ifthen = ifthen || SemanticFunctionAST.nothing
    ifelse = ifelse || SemanticFunctionAST.nothing
    return SemanticFunctionAST.new(type: :function, name: "IF", args: [cond, ifthen, ifelse])
  end

  def constructDefinitions(data = {})
    #puts " -- #{ object[:type] } ------------------------ "
    data.each_pair do |k, v|
      #puts k
      #puts (v == nil) ? "NIL" : v.debug
    end

    case object[:type]
    when :assign
      # #puts " =========> #{self.debug}"
      new_data = object[:rhs].constructDefinitions(data)
      data[object[:lhs].object[:name]] = new_data["_"]
      #puts "================== #{object[:lhs].object[:name]} => #{new_data["_"].inspect}"
      return data
    when :var
      if data[object[:name]]
        #puts " VAR NAME = #{object[:name]} = #{data[object[:name]].debug}"
        data["_"] = data[object[:name]].clone
      else
        data["_"] = self.clone
      end
      return data
    when :func
      new_func = self.object.clone

      new_func[:args].map! do |arg|
        if(arg.valid?)
          new_data = arg.constructDefinitions(data.clone)
          arg = new_data["_"]
        end
        arg
      end
      data[object[:name]] = SemanticFunctionAST.new(new_func)
      data["_"] = data[object[:name]]


      return data
    when :if
      # One function IF(COND, THEN, ELSE)
      cond_data = object[:cond].constructDefinitions(data.clone)
      then_data = object[:then].constructDefinitions(data.clone)
      else_data = object[:else].constructDefinitions(data.clone)

      elems = (then_data.keys + else_data.keys).uniq

      elems.each do |k|
        td = then_data[k] || SemanticFunctionAST.nothing
        ed = else_data[k] || SemanticFunctionAST.nothing

        if(data[k] != then_data[k] || data[k] != else_data[k] || data[k])
          #puts "SAME #{k}"
          #puts "SAME1 #{then_data[k].debug}" if then_data[k]
          #puts "SAME2 #{else_data[k].debug}" if else_data[k]
          data[k] = SemanticFunctionAST.IF(cond_data["_"], td, ed)
        else
          #puts "DIFFERENT #{k}"
          #puts "DIFFERENT1 #{then_data[k].debug}" if then_data[k]
          #puts "DIFFERENT2 #{else_data[k].debug}" if else_data[k]
        end
      end

      cond_data.each_pair do |k, v|
        data[k] = v if (data[k] != v)
      end

      # puts " == #{SemanticFunctionAST.new(object).debug(false) } =="
      # data.each_pair do |k, v|
      #   puts "      #{k} : #{v.debug(false)}"
      # end

      data["_"] = SemanticFunctionAST.nothing
      return {}

    # when :while
    #   cond_data = object[:cond].constructDefinitions(data.clone)
    #   loop_data = object[:loop].constructDefinitions(data.clone)
    #
    #   loop_dat.each { |k| data[k] = SemanticFunctionAST.new(type: :function, name: "WHILE", args: [cond_data["_"], loop_data[k]]) }
    #   data["_"] = SemanticFunctionAST.nothing
    else
      # puts self.inspect
      # puts self.object[:type]
      elems = {}  #Pre-fill semantic function table

      object.clone.each_pair do |k, v|
        if (v.class == SemanticFunctionAST)
          data.merge!(v.constructDefinitions(data))
          elems[k] = data["_"] || SemanticFunctionAST.nothing
        elsif (v.class == Array)
          v = v.map do |v1|
            v1.constructDefinitions(data)
            data["_"] || SemanticFunctionAST.nothing
          end
          elems[k] = v
        else
          elems[k] = v
        end
        # v.traverseASTWithMethodName(data, :constructDefinitions) if(v.class == SemanticFunctionAST)
      end
      data["_"] = SemanticFunctionAST.new(elems)
    end

    return data
  end

  def debug(pn = false)
    begin
      case @object[:type]
        when /error/
	      return "--- ERROR: #{@object[:message]} ---"
        when /block/
	      return "{ #{@object[:list].debug(pn)} }"
        when /stmt_list/
	      return "#{@object[:head].debug(pn)}; #{@object[:tail].debug(pn)}"
        when /if/
	      ret = "if(#{@object[:cond].debug(pn)}) #{@object[:then].debug(pn)}"
	      ret += " else #{@object[:else].debug(pn)}" if @object[:else].valid?
	      return ret
        when /while/
	      return "while(#{@object[:cond].debug(pn)}) #{@object[:loop].debug(pn)}"
        when /bincond/
	      return "#{@object[:lhs].debug(pn)} #{@object[:name]} #{@object[:rhs].debug(pn)}"
        when /unicond/
	      return "#{@object[:name]} #{@object[:rhs].debug(pn)}"
        when /cond/
	      return @object[:value].debug(pn)
        when /func/
	      return "#{@object[:name]} (#{@object[:args].map{|a| a.debug(pn)}.join(", ")})"
        when /expr_block/
	      return "(#{@object[:value].debug(pn)})"
        when /assign/
	      return "#{@object[:lhs].debug(pn)} = #{@object[:rhs].debug(pn)}"
        when /binop/
          return "#{@object[:lhs].debug(pn)}#{@object[:name]}#{@object[:rhs].debug(pn)}"
        when /uniop/
          return "#{@object[:name]}#{@object[:rhs].debug(pn)}"
        when /var/
          return "#{@object[:name]}"
        when /number/
          return "#{@object[:number]}"
        when /nothing/
          return "NOTHING" if(pn == true)
        else
          puts @object.inspect
          raise "Object type is invalid"
      end
    rescue Exception => e
      return "FAILED TO _DEBUG\n#{self.inspect}\nException at: #{e.backtrace}"
    end
  end

  def debug_encoded()
    ret = debug(false)
    begin
    ret = ret.gsub("+","%2B")
    ret = ret.gsub(";","%3B")
    ret = URI.encode(ret)
    rescue
      ret = "ERROR"
    end
    return ret
  end

  def pp(ind = 0)
    ss = " " * ind

    begin
    case @object[:type]
      when /error/
        return "--- ERROR: #{@object[:message]} ---"
      when /block/
	ret = "#{ss}{\n"
	ret+= "#{@object[:list].pp(ind + 2)}"
	ret+= "#{ss}}"
	return ret
      when /stmt_list/
        ret = ""
        if(object[:head].is_a?(SemanticFunctionAST))
		ret += "#{ss}#{@object[:head].pp(ind)}" if @object[:head].valid?
          ret += ";" if @object[:head].shouldSeparate?
        else
          ret += "// INVALID (#{object[:head].inspect})"
        end
        ret += "     (static)" if @object[:head].hasAttr?(:static)
        # ret += "     (#{@object[:head].object[:attrs].inspect})"
        ret += "\n"
      # ret += "#{ss}" if @object[:tail][:type] != "stmt_list"
        if(object[:tail].is_a?(SemanticFunctionAST))
	  ret += "#{@object[:tail].pp(ind)}" if @object[:tail].valid?
        else
          ret += "// INVALID (#{object[:tail].inspect})"
        end
        # puts " --- \n#{ret}"
	return ret
      when /if/
        ret = "if(#{@object[:cond].pp(ind)})\n"
        ret += "#{@object[:then].pp(ind+2)}"
        if @object[:else].valid?
          ret += "\n"
          ret += "#{ss}else\n"
          ret += "#{@object[:else].pp(ind+2)}"
        end
        return ret
      when /while/
        return "#{ss}while(#{@object[:cond].pp(ind)})\n#{@object[:loop].pp(ind )}"
      when /bincond/
        ret = ""
        ret += "(#{@object[:lhs].pp(ind)} #{@object[:name]} #{@object[:rhs].pp(ind)})"
        return ret
      when /unicond/
        return "#{@object[:name]}#{@object[:rhs].pp(ind)}"
      when /cond/
        return @object[:value].pp(ind)
      when /func/
        ret = ""
        ret += "#{@object[:name]} (#{@object[:args].map{|a| a.pp(ind)}.join(", ")})"
      when /expr_block/
        return "(#{@object[:value].pp(ind)})"
      when /assign/
        return "#{@object[:lhs].pp(ind)} = #{@object[:rhs].pp(ind)}"
      when /binop/
        return "(#{@object[:lhs].pp(ind)} #{@object[:name]} #{@object[:rhs].pp(ind)})"
      when /uniop/
        return "#{@object[:name]}#{@object[:rhs].pp(ind)}"
      when /var/
        return "#{@object[:name]}"
      when /number/
        return "#{@object[:number]}"
      when /nothing/
	return ""
      else
        raise "Object type is invalid"
    end
    rescue
      raise "Failed pretty printing #{stmt.inspect}"
    end
  end

  def ppf(ind = 0)
    ss = " " * ind

    case @object[:type]
      when /error/
        return "--- ERROR: #{@object[:message]} ---"
      when /bincond/
        return "#{@object[:lhs].ppf(ind)} #{@object[:name]} #{@object[:rhs].ppf(ind)}"
      when /unicond/
        return "#{@object[:name]} #{@object[:rhs].ppf(ind)}"
      when /cond/
        return @object[:value].ppf(ind)
      when /func/
        return "#{@object[:name]} (\n#{ss}  #{@object[:args].map{ |a| a.ppf(ind+2) }.join(",\n#{ss}  ")})"
      when /expr_block/
        return "(#{@object[:value].ppf(ind)})"
      when /assign/
        return "#{@object[:lhs].ppf(ind)} = #{@object[:rhs].ppf(ind)}"
      when /binop/
        return "#{@object[:lhs].ppf(ind)}#{@object[:name]}#{@object[:rhs].ppf(ind)}"
      when /uniop/
        return "#{@object[:name]}#{@object[:rhs].ppf(ind)}"
      when /var/
        return "#{@object[:name]}"
      when /number/
        return "#{@object[:number]}"
      when /nothing/
        return "NOTHING"
	return ""
      else
        raise "Object type is invalid"
    end
  end


  def to_c(r={})
    case @object[:type]
      when /binop/
        return "( #{@object[:lhs].to_c(r)} #{@object[:name]} #{@object[:rhs].to_c(r)} )"
      when /uniop/
        return "( #{@object[:name]}#{@object[:rhs].to_c(r)} )"
      when /func/
        return "( #{@object[:name]} ( #{@object[:param].map {|p| p.to_c(r) }.join(",") } ) )"
      when /var/
        var_name = @object[:var].to_sym
        #puts "VAR_NAME= #{var_name}"
        return r[var_name] if r[var_name] != nil
        return @object[:var]
      when /number/
        return "#{@object[:number]}"
      else
        raise "Object type is invalid #{self}"
    end

  end
end
