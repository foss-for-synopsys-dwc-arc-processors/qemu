#!/usr/bin/env python3

import opcode
import re
import sys
import getopt


input_file = ''

#-----------------------------------------------
operands = { }
flags = { }

operands['OP_EMPTY'] = []
flags['FLAG_C_EMPTY'] = []

opcodes = []

opcodes_cls = []
opcode_input_data = []

def error_with_file(file, lineno, *args):
    """Print an error message from file:line and args and exit."""
    prefix = ''
    if file:
        prefix += f'{file}:'
    if lineno:
        prefix += f'{lineno}:'
    if prefix:
        prefix += ' '
    print(prefix, end='error: ', file=sys.stderr)
    print(*args, file=sys.stderr)

    exit(1)


def error(lineno, *args):
    error_with_file(input_file, lineno, *args)


class Opcode(object):

    def __init__(self, mnemonic, opcode_orig, mask_orig, cpu, classe, subclass, 
                 operands, flags, bitmask, asm_template):
        self.mnemonic = mnemonic
        self.opcode_orig = opcode_orig
        self.mask_orig = mask_orig
        self.cpu = cpu 
        self.classe = classe 
        self.subclass = subclass 
        self.operands = operands
        self.flags = flags
        self.bitmask = bitmask 
        self.asm_template = asm_template

    def __str__(self):
        ostr = f'OPCODE({self.mnemonic}, {self.opcode_orig}, {self.mask_orig}, {self.cpu}, {self.classe}, '
        ostr += f'{self.subclass}, {self.operands}, {self.flags}, {self.bitmask}, "{self.asm_template}")'
        return ostr

    def oprint(self):
        #f'OPCODE({mnemonic}, {a[1]}, {a[2]}, {arch}, {a[4]}, {a[5]}, {ops_name}, {flags_name}, {latest_pattern}, \"{latest_asm}\")\n'
        print(self)
        


def parse_entry(l, level = 0):
    #content = None
  
    # Clear spaces and tabs
    l = l.strip()

    m = re.search(r'^{(.+)', l)
    if m:
        g1 = m.group(1)
        a = parse_entry(g1, level + 1)
        return { 'result': [a['result']] + parse_entry(a['remain'], level)['result'], 'remain': '' }
    
    m = re.search(r'^}(.+)', l)
    if m:
        g1 = m.group(1)
        return { 'result': [], 'remain': g1 }

    m = re.search(r'^,(.+)', l)
    if m:
        g1 = m.group(1)
        return parse_entry(g1, level)
    
    m = re.search(r'^}(.+)', l)
    if m:
        g1 = m.group(1) 
        return { 'result': [], 'remain': g1 }

    m = re.search(r'([^,}]+)(.+)', l)
    if m:
        g1 = m.group(1)
        g2 = m.group(2) 
        a = parse_entry(g2, level)
        return { 'result': [g1.strip()] + a['result'], 'remain': a['remain'] }
    
    return { 'result': [], 'remain': '' }


def get_arches(arches_str):
    arches = []
    exclude_arches = ['ARC700', 'ARC600']
    arches_lst = arches_str.split('|')
    for arch in arches_lst:
        m = re.search(r'^[ ]*([^ ]+)[ ]*$', arch)
        arch = m.group(1)
        arch = re.sub(r'^ARC_OPCODE_', '', arch)
        if arch not in exclude_arches:
            arches.append(arch)
    return arches


def print_operands_list(operands, of):
    of.write('\n')
    for key, val in operands.items():
        if len(val) > 0:
            ops = list(map(lambda n: f'OPERAND_{n}', val))
            strops = ', '.join(ops)
            of.write(f'OPERANDS_LIST({key}, {len(val)}, {strops})\n')
        else:
            of.write(f'OPERANDS_LIST({key}, 0)\n')


def print_flags_list(flags, of):
    for key, val in flags.items():
        if len(val) > 0:
            strflags = ', '.join(val)
            of.write(f'FLAGS_LIST({key}, {len(val)}, {strflags})\n')
        else:
            of.write(f'FLAGS_LIST({key}, 0, 0)\n')


def print_mnemonics(opcodes, of):
    mnemonics = []
    for opcode in opcodes:
        mnemonics.append(opcode[0])
    mnemonics = list(dict.fromkeys(mnemonics))
    mnemonics.sort()
    for mnemonic in mnemonics:
        of.write(f'MNEMONIC({mnemonic})\n')


def open_output(output):
    return open(output, 'w') if output else sys.stdout


def parse_file(f, of):
    lineno = 0
    latest_asm = None
    latest_pattern = None

    for line in f:
        lineno += 1
        
        if re.match(r'^\/[*] [a-z]', line):
            of.write(line)
            match = re.search(r'\/[*] ([^ ]+[  +][^ ]+) ([01][^ .]+)\.?[ ]+\*\/', line)
            if match:
                latest_asm = match.group(1).rstrip()
                latest_pattern = match.group(2)
            else:
                match = re.search(r'\/[*] ([^ ]+)[ ]+([01][^ .]+)\.?[ ]+\*\/', line)
                if match:
                    latest_asm = match.group(1).rstrip()
                    latest_pattern = match.group(2)
                else:
                    error_with_file(f.name, lineno, 'comment does not contain pattern')
        elif re.match(r'^{', line):
            a = parse_entry(line)['result'][0]
            
            ops = list(map(lambda n: re.sub(r'^0$', 'EMPTY', n), a[6]))
            ops = list(map(lambda n: re.sub(r'^OPERAND_', '', n), ops))
            strops = '_'.join(ops)
            ops_name = f'OP_{strops}'
            
            if ops_name not in operands:
                operands[ops_name] = ops 

            fls = list(map(lambda n: re.sub(r'^0$', 'C_EMPTY', n), a[7]))
            strflags = '_'.join(fls)
            flags_name = f'FLAG_{strflags}'

            if flags_name not in flags:
                flags[flags_name] = fls

            arches = get_arches(a[3])
            
            mnemonic = re.sub(r'\"', '', a[0])
            for arch in arches:
                of.write(f'OPCODE({mnemonic}, {a[1]}, {a[2]}, {arch}, {a[4]}, {a[5]}, {ops_name}, {flags_name}, {latest_pattern}, \"{latest_asm}\")\n')
                opcode_cls = Opcode(mnemonic, a[1], a[2], arch, a[4], a[5], ops_name, flags_name, latest_pattern, latest_asm)
                opcodes_cls.append(opcode_cls)
            
            opcodes.append([mnemonic, a[1], a[2], a[3], a[4], a[5], operands, flags])
        else:
            of.write(line)

#{ 'arch': 'ARCv2HS', 'file': 'v2_hs_dtree.def' },
#{ 'arch': 'ARCv2EM', 'file': 'v2_em_dtree.def' },
#{ 'arch': 'ARC64', 'file': 'v3_hs6x_dtree.def' },
#{ 'arch': 'ARC32', 'file': 'v3_hs5x_dtree.def' }    

def compose_pattern(opcode, mask, size):
    o = format(opcode, "b")
    o = f'{"0" * (size - len(o))}{o}'
    o = [c for c in o]
    
    m = format(mask, "b")
    m = f'{"0" * (size - len(m))}{m}'
    m = re.sub(r"0", "-", m)    
    m = [c for c in m]

    ret = ""
    for z in m:
        c = o.pop(0)
        if z != "-":
            ret += c
        else:
            ret += "-"
    return ret


def select_opcodes(match, filter, cpu, opcode_list):
    res = [opc for opc in opcode_list 
        if cpu == opc['cpu'] and (opc['opcode'] & filter == match)]
    return res


def find_common_pattern(opcode_match, filter_mask, cpu, opcode_list):
  ret = ((1 << 32) -1) ^ filter_mask
  for opc in select_opcodes(opcode_match, filter_mask, cpu, opcode_list):
    ret &= opc['mask']
  return ret


def versions_for_mask(mask):
    ret = []

    bindata = format(mask, "b")
    count = bindata.count("1")
    
    for n in range(1 << count):
        r = 0
        lstbit = [c for c in bindata]
        lstbit.reverse()
        for i, bit in enumerate(lstbit):
            if bit == '1':
                r |= n & (1 << i)
            else:
                n <<= 1
            if n == 0:
                break
        ret.append(r)

    return ret


def get_unique(lst):
    ret = []
    for n in lst:
        if n not in ret:
            ret.append(n)
    return ret


def traverse(opcode_match, filter_mask, pattern_mask, cpu, level = 0, opcode_list = opcode_input_data):
    node = {}    
    node['data'] = { }

    new_opcode_list = select_opcodes(opcode_match, filter_mask, cpu, opcode_list)
    count = len(new_opcode_list)
    
    node['data']['opcode_match'] = opcode_match
    node['data']['filter_mask'] = filter_mask
    node['data']['parent_pattern_mask'] = pattern_mask
    node['data']['count'] = count
    node['data']['elems'] = new_opcode_list
    node['data']['patterns'] = []
    
    if count == 1:
        return node
    
    pattern_mask = find_common_pattern(opcode_match, filter_mask, cpu, opcode_list)
    node['data']['pattern_mask'] = pattern_mask
    
    if pattern_mask != 0 and pattern_mask != (((1 << 32) -1) ^ filter_mask):
        node['subtrees'] = {}
        versions = versions_for_mask(pattern_mask)
        
        for match in versions:
            mask = pattern_mask | filter_mask
            tmp = traverse(opcode_match | match, mask, pattern_mask, cpu, level + 1, new_opcode_list)
            node['subtrees'][match] = tmp
    else:    
        patterns = []
        for opc in new_opcode_list:
            
            bmask = opc['bitmask']
            bmask = re.sub(r'[01]', '1', bmask)
            bmask = re.sub(r'[^1]', '0', bmask)
            o = int(bmask, 2)
            p = compose_pattern(opc['opcode'], (((1 << 32) - 1) ^ filter_mask) & o, 32)
            patterns.append(p)
        
        patterns = get_unique(patterns)
        node['data']['patterns'] = patterns
        
    return node


def enum_for_opcode(opc):
    ret = "OPCODE_"
    ret += f"{opc['cpu']}_"
    ret += f"{opc['mnemonic']}_"
    ret += f"{opc['opcode_orig']}_"
    ret += f"{opc['mask_orig']}_"
    ret += f"{opc['class']}_"
    ret += f"{opc['operands']}"
    return ret


def print_as_macros(file, tree, level = 0):
    count = tree['data']['count']
    subtrees = None
    if 'subtrees' in tree: 
        subtrees = tree['subtrees']
    patterns = tree['data']['patterns']
    opcode_list = tree['data']['elems']

    value = tree['data']['opcode_match'] & tree['data']['parent_pattern_mask']
    
    if count == 0:
        return

    if level > 0:
        if count > 0:
            file.write(f'{"  "*level}MATCH_VALUE({hex(value)}) /* {format(value, "b")} */\n')
        if count == 1:
            file.write(f'{"  "*level}  RETURN_MATCH({enum_for_opcode(opcode_list[0])})  /* {opcode_list[0]["bitmask"]} */\n')
        if patterns and len(patterns) > 0:
            for opc in opcode_list:
                file.write(f"{'  '*level}  MULTI_MATCH({enum_for_opcode(opc)})  /* {opc['bitmask']} */\n")

    if subtrees is not None:
        file.write(f"{'  '*level} MATCH_PATTERN({hex(tree['data']['pattern_mask'])}) /* {format(tree['data']['pattern_mask'], 'b')} */\n")
        for _, val in subtrees.items():
            print_as_macros(file, val, level+1)
        file.write(f"{'  '*level} END_MATCH_PATTERN({hex(tree['data']['pattern_mask'])}) /* {format(tree['data']['pattern_mask'], 'b')} */\n")

    if level > 0:
        file.write(f"{'  '*level}END_MATCH_VALUE({hex(value)}) /* {format(value, 'b')} */\n")

def load_opcode_input_data():
    for opc in opcodes_cls:
        opcode = int(opc.opcode_orig, 16)
        mask = int(opc.mask_orig, 16)
    
        if mask < int("0x10000", 16):
            mask = mask << 16
            opcode = opcode << 16

        opcode_input_data.append(
                { 'mnemonic': opc.mnemonic, 'opcode_orig': opc.opcode_orig, 'opcode': opcode, 'mask_orig': opc.mask_orig, 
                  'mask': mask, 'cpu': opc.cpu, 'class': opc.classe, 'subclass': opc.subclass, 'operands': opc.operands, 
                  'flags': opc.flags, 'bitmask': opc.bitmask, 'size': len(opc.bitmask), 'asm_template': opc.asm_template
                }) 


def gen_tree(arch, file):
    tree = traverse(0, 0, 0, arch)
    of = open(file, "w")
    print_as_macros(of, tree)
    of.close

#{ 'arch': 'ARCv2HS', 'file': 'v2_hs_dtree.def' },
#{ 'arch': 'ARCv2EM', 'file': 'v2_em_dtree.def' },
#{ 'arch': 'ARC64', 'file': 'v3_hs6x_dtree.def' },
#{ 'arch': 'ARC32', 'file': 'v3_hs5x_dtree.def' }   

def main():
    opcodes_file = None
    arcv2hs_file = None
    arcv2em_file = None
    arcv3hs6x_file = None
    arcv3hs5x_file = None
    long_opts = ['opcodes=', 'arcv2hs=', 'arcv2em=', 'arcv3hs5x=', 'arcv3hs6x=']

    try:
        (opts, args) = getopt.gnu_getopt(sys.argv[1:], 'o:a:b:c:d:', long_opts)
    except getopt.GetoptError as err:
        error(0, err)

    for o, a in opts:
        if o in ('-o', '--opcodes'):
            opcodes_file = a
        elif o in ('-a', '--arcv2hs'):
            arcv2hs_file = a
        elif o in ('-b', '--arcv2em'):
            arcv2em_file = a
        elif o in ('-c', '--arcv3hs6x'):
            arcv3hs6x_file = a
        elif o in ('-d', '--arcv3hs5x'):
            arcv3hs5x_file = a
        else:
            assert False, 'unhandled option'
    
    if len(args) < 1:
        error(0, 'missing input file')

    with open_output(opcodes_file) as of:
        for filename in args:
            input_file = filename
            try:
                f = open(filename, 'rt', encoding='utf-8')
            except IOError:
                error(0, f'could not open input file ({input_file})')
            parse_file(f, of)
            f.close()
            
        print_operands_list(operands, of)   
        print_flags_list(flags, of)
        print_mnemonics(opcodes, of)
            
        of.close()

    load_opcode_input_data()

    # ARCv2HS
    if arcv2hs_file:
        gen_tree('ARCv2HS', arcv2hs_file)

    # ARCv2EM
    if arcv2em_file:
        gen_tree('ARCv2EM', arcv2em_file)

    # ARCv3HS6x
    if arcv3hs6x_file:
        gen_tree('ARC64', arcv3hs6x_file)

    # ARCv3HS5x
    if arcv3hs5x_file:
        gen_tree('ARC32', arcv3hs5x_file)
            

if __name__ == "__main__":
    main()
