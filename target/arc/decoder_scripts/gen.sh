#!/bin/bash

cat decoder_originals/arc64-tbl.h decoder_originals/opcodes.def > /tmp/opcodes_original.full
ruby decoder_scripts/opcodes_process.rb < /tmp/opcodes_original.full | cat > opcodes-full.def
ruby decoder_scripts/decode_tests.rb

#ruby decoder_scripts/opcodes_process.rb < decoder_originals/opcodes-v3.def | grep -v ARCv2HS | cat > opcodes-v3.def
#ruby decoder_scripts/opcodes_process.rb < decoder_originals/opcodes.def > opcodes.def
#ruby decoder_scripts/decode_tests.rb
