#!/usr/bin/env ruby

require 'erb'
require_relative 'init.rb'

HEADER = <<EOF
/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synppsys Inc.
 * Contributed by Cupertino Miranda <cmiranda@synopsys.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * http://www.gnu.org/licenses/lgpl-2.1.html
 */

#include "qemu/osdep.h"
#include "translate.h"
#include "target/arc/semfunc.h"

EOF

ERB_TEMPLATE = <<EOF
/*
 * <%= @name %>
<%= print_lists("   Variables", @variables) %>
<%= print_lists("   Functions", @functions) %>
 * --- code ---
<%= @pretty_code %>
 */

int
arc_gen_<%= @name %>(DisasCtxt *ctx<%= @tcg_variables %>)
{
    int ret = DISAS_NEXT;
<%= @tcg_code %>
    return ret;
}


EOF

def print_lists(name, elems, max_length = 80, prefix: " * ")
  ret = ""
  to_print = prefix + name + ": "

  elems.each_with_index do |e, i|
    if(to_print.length + e.length + 2 > max_length)
      ret += to_print.gsub(/ $/, "") + "\n"
      to_print = prefix + (" " * (name.length + 2))
    end

    to_print += e
    to_print += ", " if i + 1 != elems.length
  end

  ret += to_print
  return ret.split("\n").map { |a| a.gsub(/[ \t]+$/, "") }.join("\n")
end


def error(line_num, message)
  puts "Error at semfunc.c:#{line_num} -- #{message}"
  exit(-1)
end

EMPTY_ENTRY = {
  name: "invalid",
  code: ""
}

funcs = {}
funcs_in_order = []

current_func = nil
func_name = nil
in_comment = false
in_code = false
in_field_read = nil
line_num = 1;
File.read("../semfunc.c").each_line do |l|
    if(l =~ /^\/[*]/)
        #  puts "ENTERED IN COMMENT at line #{line_num}"
        in_comment = true
    elsif (in_comment == true && l =~ /\*\/[ \t]*$/)
        in_comment = false
        if(in_code == true)
          #  puts "END_COMMENT at line #{line_num}"
          #  puts current_func[:code]
          current_func[:ast] = SemanticFunctionParser.new.parse(current_func[:code])
          funcs[func_name] = current_func
          funcs_in_order.push(func_name)
        end
        in_code = false
    elsif (in_comment == true)
      if(l =~ /^ [*][ ]+([A-Z0-9_]+)$/)
        func_name = $1
        current_func = EMPTY_ENTRY.clone()
        current_func["Variables"] = []
        current_func["Functions"] = []
        current_func[:name] = func_name
        current_func[:code] = ""
      elsif(in_field_read != nil && l =~ /^ [*][ \t]+([@a-zA-Z0-9, ]+)$/)
        data = $1
        data.split(/,[ ]*/).each do |d_entry|
          current_func[in_field_read].push(d_entry)
        end
      elsif(l =~ /^ [*][ \t]+([a-zA-Z]+): ([@a-zA-Z0-9, ]+)$/)
        field = $1
        data = $2
        if(current_func[field].nil?)
          error(line_num, "Field '#{field}' not valid.")
        end
        data.split(/,[ ]*/).each do |d_entry|
          #puts "#{field} = #{d_entry}"
          current_func[field].push(d_entry)
        end
        in_field_read = field
      elsif(l =~ /^ [*] --- code ---$/)
        in_field_read = nil
        in_code = true
      elsif(in_code)
        current_func[:code] = "#{current_func[:code]}#{l[3..-1]}"
      end
    end
    line_num += 1
end

def fix_indentation_tcg_code(code)

  ret = ""
  in_comment = false
  indent = 0
  #puts code

  code.split("\n").each_with_index do |line, idx|

    if(in_comment == false)
      if(line =~ /^[ \t]+$/)
        #ret += "1: "
        ret += "\n"
      elsif(line =~ /^[ \t;]+$/)
        #ret += "2: \n"
        ret += ""
      else
        indent -= (line.scan(/}/).size - 1) * 4

        #ret += "9: "
        line =~ /^[ \t]*(.+)$/
        code = $1
        ret += "#{" "*indent}#{$1}\n"

        indent += (line.scan(/{/).size - 1) * 4
      end
    end

  end

  ret1 = ""
  in_else = nil
  else_content = ""
  perhaps_else = false;
  ret.split("\n").each_with_index do |line, i|
    if(line.index("else {") != nil)
      #puts "1- #{line}"
       in_else = i
       else_content = " else {\n"
    elsif(line.index("}") != nil && in_else != nil)
      if(in_else + 1 != i)
        #puts "2- #{line}"
        else_content += line
        ret1 += else_content + "\n"
      end
      else_content = ""
      in_else = nil
    elsif(line.index("}") != nil && in_else == nil)
      ret1 += line
      else_content += line
      perhaps_else = true
    else
      if(in_else != nil)
        perhaps_else = false
        #puts "3- #{line}"
        else_content += line + "\n"
      else
        #puts "4- #{line}"
        ret1 += "\n" if(perhaps_else == true)
        ret1 += "#{line}\n"
        perhaps_else = false
      end
    end
  end
  return ret1
end


class FuncBinding
  def initialize(data, opts)
    @name = data[:name]
    @functions = data["Functions"]
    @variables = data["Variables"]
    @code = data[:code].gsub(/\t/, '')
    @pretty_code = data[:code].split("\n").map { |l| " * #{l}" }.join("\n")
    @ast = SemanticFunctionParser.new.parse(@code)
    tcg_code = QEmuCompiler.new.generate(@ast, [], opts[:debug])
    @tcg_code = fix_indentation_tcg_code(tcg_code)
    @tcg_variables = @variables.map { |a| "TCGv #{a.gsub("@", "")}"}.unshift("").join(", ")
  end
end


# Options parsing
opts = { debug: false }
while(ARGV.count > 0)
  opt = ARGV.shift
  if(opt == '-f' && (tmp = ARGV.shift) != nil)
    opts[:filter] = tmp
  elsif(opt == '-d')
    puts "HERE"
    opts[:debug] = true
  end
end

puts HEADER
funcs_in_order.each do |name|
  next if(opts[:filter] && opts[:filter] != name)
  data = funcs[name]
  #puts name
  next if data[:code].nil?
  erb = ERB.new(ERB_TEMPLATE)
  MyClass = erb.def_class(FuncBinding, 'render()')
  puts MyClass.new(data, opts).render()
  self.class.send(:remove_const, :MyClass)
end
