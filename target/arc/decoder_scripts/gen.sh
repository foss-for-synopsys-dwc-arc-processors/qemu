#!/bin/bash

cat decoder_originals/arc64-tbl.h decoder_originals/arc-tbl.h > /tmp/opcodes_original.full
ruby decoder_scripts/opcodes_process.rb < /tmp/opcodes_original.full | cat > opcodes-full.def
ruby decoder_scripts/decode_tests.rb
