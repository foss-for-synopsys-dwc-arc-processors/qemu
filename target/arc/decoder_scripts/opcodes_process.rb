#!/usr/bin/ruby

def parse_entry(l, level = 0)
  content = nil
  # Clear spaces and tabs

  l = l.strip

  # puts "L(#{level}) = #{l}"
  if(l =~ /^{(.+)/)
    a = parse_entry($1, level + 1)
    return { result: [a[:result]] + parse_entry(a[:remain], level)[:result], remain: "" }
  elsif(l =~ /^}(.+)/)
    return { result: [], remain: $1 }
  elsif(l =~ /^,(.+)/)
    return parse_entry($1, level)
  elsif(l =~ /^}(.+)/)
    return { result: [], remain: $1 }
  elsif(l =~ /([^,}]+)(.+)/)
    a = parse_entry($2, level)
    return { result: [$1.strip] + a[:result], remain: a[:remain] }
  else
    return { result: [], remain: "" }
  end
end

operands = { }
flags = { }

operands["OP_EMPTY"] = []
flags["FLAG_C_EMPTY"] = []

opcodes = []

ARCH_MAP = {
  "ARC_OPCODE_ARC600": "arc600",
  "ARC_OPCODE_ARC700": "arc600",
  "ARC_OPCODE_ARCv2EM": "v2em",
  "ARC_OPCODE_ARCv2HS": "v2hs"
}


latest_pattern = nil
latest_asm = nil
STDIN.read.split("\n").each do |a|
  if(a =~ /^\/[*] [a-z]/)
    puts a

    if(a =~ /\/[*] ([^ ]+[  +][^ ]+) ([01][^ .]+)\.?[ ]+\*\//)
      latest_asm = $1.chomp
      latest_pattern = $2
    elsif(a =~ /\/[*] ([^ ]+)[ ]+([01][^ .]+)\.?[ ]+\*\//)
      latest_asm = $1.chomp
      latest_pattern = $2
    else
      puts "Error: comment does not contain patter"
      exit -1
    end
    

  elsif(a =~ /^{/)
    #puts "A = #{a}"
    a = parse_entry(a)[:result][0]

    a[6].join()
    a[7]

    #puts a.inspect
    ops = a[6].map { |n| n.gsub(/^0$/, "EMPTY") }
    ops = ops.map { |n| n.gsub(/^OPERAND_/, "") }
    ops_name = "OP_#{ops.join("_")}"

    #operands[ops_name] = ops
    #puts "OPERANDS_LIST(#{ops_name}, #{ops.map { |a| "OPERAND_#{a}" }.join(", ")})"
    operands[ops_name] = ops if operands[ops_name].nil?

    fls = a[7].map { |n| n.gsub(/^0$/, "C_EMPTY") }
    flags_name = "FLAG_#{fls.join("_")}"
    #puts "FLAG_LIST(#{flags_name}, #{flags.map { |a| "#{a}_F" }.join(", ")})"
    flags[flags_name] = fls if flags[flags_name].nil?

    arches = a[3].split("|").map { |a| a = $1 if(a =~ /^[ ]*([^ ]+)[ ]*$/); a.gsub(/^ARC_OPCODE_/, "") }
    arches.select! { |a| a !~ /(700|600)/ }

    arches.each do |arch|
      puts "OPCODE(#{a[0].gsub("\"","")}, #{a[1]}, #{a[2]}, #{arch}, #{a[4]}, #{a[5]},#{ops_name}, #{flags_name}, #{latest_pattern}, \"#{latest_asm}\")"
    end
    opcodes.push [a[0].gsub("\"", ""), a[1], a[2], a[3], a[4], a[5], operands, flags]

  else
    puts a
  end
end

operands.each_pair do |k, v|
  if(v.count > 0)
    puts "OPERANDS_LIST(#{k}, #{v.count}, #{v.map { |a| "OPERAND_#{a}" }.join(", ")})"
  else
    puts "OPERANDS_LIST(#{k}, 0)"
  end
end
flags.each_pair do |k, v|
  if(v.count > 0)
    puts "FLAGS_LIST(#{k}, #{v.count}, #{v.map { |a| "#{a}" }.join(", ")})"
  else
    puts "FLAGS_LIST(#{k}, 0)"
  end
end

opcodes.map { |a| a[0] }.uniq.sort.each do |n|
    puts "MNEMONIC(#{n})"
end
