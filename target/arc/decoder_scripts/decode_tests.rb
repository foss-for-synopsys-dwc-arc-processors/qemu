#!/usr/bin/ruby

@opcode_input_data = []

def load_opcodes(opcode_file)
  File.read(opcode_file).split("\n").each do |l|
    if(l =~ /OPCODE\(([^)]+)\)/)
      args = $1.split(/, ?/)
    
      opcode = args[1].to_i(16)
      mask = args[2].to_i(16)

      if(mask < "0x10000".to_i(16))
        mask = mask << 16
        opcode = opcode << 16
        #puts "HEEEEEEERE #{opcode}"
      end

      @opcode_input_data.push({ mnemonic: args[0], opcode_orig: args[1], opcode: opcode, mask_orig: args[2], mask: mask, cpu: args[3], class: args[4], subclass: args[5], operands: args[6], flags: args[7], bitmask: args[8], size: args[8].length, asm_template: args[9..-1].join(",") })
    end
    #puts @opcode_input_data[-1].inspect
  end
end


def compose_pattern(opcode, mask, size)
  o = opcode.to_s(2)
  o = "#{"0" * (size - o.length)}#{o}"
  o = o.chars

  m = mask.to_s(2)
  m = "#{"0" * (size - m.length)}#{m}"
  m = m.gsub("0", "-").chars

  ret = ""
  m.each do |z|
    c = o.shift()
    if(z != "-")
      ret += c
    else
      ret += "-"
    end
  end
  return ret
end

def select_opcodes(match, filter, cpu, opcode_list)
  return opcode_list.select do |opc| 
    (cpu == opc[:cpu] && (opc[:opcode] & filter == match))
     #(compose_pattern(opc[:opcode], filter & opc[:mask]) == compose_pattern(match, filter)))
  end
end

def find_common_pattern(opcode_match, filter_mask, cpu, opcode_list)
  #puts filter_mask.to_s(2)
  ret = ((1 << 32) -1) ^ filter_mask
  #puts "OPCODE_MATCH: #{opcode_match.to_s(2)}"
  select_opcodes(opcode_match, filter_mask, cpu, opcode_list).each do |opc| 
    #puts "  match: #{opc[:opcode].to_s(2)}"
    ret &= opc[:mask]
  end
  return ret
end

def versions_for_mask(mask)
  ret = []
  count = mask.to_s(2).count("1")
  (1 << count).times do |n|
    r = 0
    mask.to_s(2).split("").reverse.each_with_index do |bit, i|
      if(bit == "1")
        r |= n & (1 << i)
      else
        n <<= 1;
      end

      if(n == 0)
        break
      end  
    end
    ret.push(r)
  end
  return ret
end


def traverse(opcode_match, filter_mask, pattern_mask, cpu, level = 0, opcode_list = @opcode_input_data)
  node = {}
  #puts "#{" "*level}OPCODE: 0x#{opcode_match.to_s(16)} MASK: 0x#{filter_mask.to_s(16)}  => PATTERN: #{pattern_mask.to_s(16)}"

  node[:data] = { }

  new_opcode_list = select_opcodes(opcode_match, filter_mask, cpu, opcode_list)
  count = new_opcode_list.count
  #node[:data][:elems] = opcode_list
  node[:data][:opcode_match] = opcode_match
  node[:data][:filter_mask] = filter_mask
  node[:data][:parent_pattern_mask] = pattern_mask
  node[:data][:count] = count
  node[:data][:elems] = new_opcode_list
  node[:data][:patterns] = []

  #puts "opcode_match: #{compose_pattern(opcode_match, filter_mask, 32)}:"
  #puts "count: #{count}"
  new_opcode_list.each do |opc|
    #puts "  " + insn2str(opc)    
  end

  if(count == 1)
    #puts "Found 1 solution."
    return node
  end

  pattern_mask = find_common_pattern(opcode_match, filter_mask, cpu, opcode_list)
  node[:data][:pattern_mask] = pattern_mask

  #puts "pattern_match: #{compose_pattern(pattern_mask, ((1 << 32) - 1), 32)}"

  if(pattern_mask != 0 && pattern_mask != (((1 << 32) -1) ^ filter_mask))
    node[:subtrees] = {}
    versions = versions_for_mask(pattern_mask)
    versions.each do |match|
      mask = pattern_mask | filter_mask
      tmp = traverse(opcode_match | match, mask, pattern_mask, cpu, level + 1, new_opcode_list)
      node[:subtrees][match] = tmp
    end
  else
    #puts node[:data][:count]

    patterns = new_opcode_list.map do |opc| 
      o = opc[:bitmask].gsub(/[01]/, "1").gsub(/[^1]/, "0").to_i(2)
      compose_pattern(opc[:opcode], (((1 << 32) - 1) ^ filter_mask) & o, 32)
    end
    patterns = patterns.uniq
    node[:data][:patterns] = patterns
  end

  return node
end

def insn2str(e)
  return "#{e[:bitmask]} - #{e[:mnemonic]} (#{e[:mask].to_s(2)})"
end

def print_tree(tree, size, level = 0)
  pattern = compose_pattern(tree[:data][:opcode_match], tree[:data][:parent_pattern_mask], size)
  count = tree[:data][:count]
  subtree = tree[:subtrees]
  patterns = tree[:data][:patterns]
  opcode_list = tree[:data][:elems]

  if(count == 0)
    encoding = compose_pattern(tree[:data][:opcode_match], tree[:data][:filter_mask], size)
    puts "#{"  "*level}#{level}: [[#{pattern}]] => EMPTY ENCODING (#{encoding})"
    return
  end
  if(level > 0)
    if(count == 1)
      puts "#{"  "*level}#{level}: [[#{pattern}]] => #{count}          #{opcode_list[0][:asm_template]}"
    else
      puts "#{"  "*level}#{level}: [[#{pattern}]] => #{count}"
    end
    if(patterns && patterns.count > 0)
      patterns.each do |p|
        puts "                        #{p} (#{p.chars.select {|a| a != '-'}.count})"
      end
      opcode_list.each do |opc|
        puts "                                    #{opc[:bitmask]} #{opc[:asm_template]}"
      end
    end
  end
  if(subtree)
    subtree.each_pair do |k, v|
      print_tree(v, size, level+1)
    end
  end
end

def enum_for_opcode(opc)
  ret = "OPCODE_"
  ret += "#{opc[:cpu]}_"
  ret += "#{opc[:mnemonic]}_"
  ret += "#{opc[:opcode_orig]}_"
  ret += "#{opc[:mask_orig]}_"
  ret += "#{opc[:class]}_"
  ret += "#{opc[:operands]}"
  return ret
end

def print_as_macros(file, tree, level = 0)
  count = tree[:data][:count]
  subtree = tree[:subtrees]
  patterns = tree[:data][:patterns]
  opcode_list = tree[:data][:elems]

  value = tree[:data][:opcode_match] & tree[:data][:parent_pattern_mask]


  if(count == 0)
    return
  end

  if(level > 0)
    if(count > 0)
      file.puts "#{"  "*level}MATCH_VALUE(0x#{value.to_s(16)}) /* #{value.to_s(2)} */"
    end
    if(count == 1)
      file.puts "#{"  "*level}  RETURN_MATCH(#{enum_for_opcode(opcode_list[0])})  /* #{opcode_list[0][:bitmask]} */"
    end
    if(patterns && patterns.count > 0)
      #opcode_list.reverse.each do |opc|
      opcode_list.each do |opc|
        file.puts "#{"  "*level}  MULTI_MATCH(#{enum_for_opcode(opc)})  /* #{opc[:bitmask]} */"
      end
    end
  end
  if(subtree)
    file.puts "#{"  "*level} MATCH_PATTERN(0x#{tree[:data][:pattern_mask].to_s(16)}) /* #{tree[:data][:pattern_mask].to_s(2)} */"
    subtree.each_pair do |k, v|
      print_as_macros(file, v, level+1)
    end
    file.puts "#{"  "*level} END_MATCH_PATTERN(0x#{tree[:data][:pattern_mask].to_s(16)}) /* #{tree[:data][:pattern_mask].to_s(2)} */"
  end
  if(level > 0)
    file.puts "#{"  "*level}END_MATCH_VALUE(0x#{value.to_s(16)}) /* #{value.to_s(2)} */"
  end
end


def print_switch_tree(tree, level = 0, offset = 4)
  ret = ""

  if(tree[:data][:count] == 1)
    ret += "return \"#{tree[:data][:opcode_list][0][:asm_template]}\""
  elsif(tree[:subtrees].nil?)
    tree[:data][:opcode_list].each do |opc|
      ret += "if \"#{opc[:asm_template]}\""
    end
  elsif()
    ret += "#{("  "*level)}switch(opcode & #{tree[:data][:pattern_mask]}) {"
    tree[:subtrees].each_pair do |k, t|
      case_value = "0x#{(k.to_i(2) & tree[:data][:pattern_mask]).to_s(16)}"
    end


  else
    puts "FAILED MISERABLY"
    exit -1
  end
end


#puts compose_pattern("0x2300".to_i(16), "0xff00".to_i(16), 32)
#exit 0

#exit -1 if ARGV.count != 1

#load_opcodes(ARGV[0])
#puts @opcode_input_data.inspect

#puts "==> #{find_common_pattern(0, 0, "ARCv2HS", 32).to_s(16)}"
#

files = {
  'opcodes.def' => [
    { arch: "ARCv2HS", file: "v2_hs_dtree.def" },
    { arch: "ARCv2EM", file: "v2_em_dtree.def" },
  ],
  'opcodes-v3.def' => [
    { arch: "V3_ARC64", file: "v3_hs6x_dtree.def" }
  ]
}

files.each_pair do |file, versions|
  @opcode_input_data = []
  load_opcodes(file)
  versions.each do |d|
    tree = traverse(0, 0, 0, d[:arch])
    file = File.open(d[:file], "w")
    print_as_macros(file, tree)
    file.close
  end
end

#puts "HEEEERE"
##print_tree(tree, size)
#print_as_macros(tree, size)



