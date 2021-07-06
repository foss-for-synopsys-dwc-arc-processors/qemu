#!/bin/bash

ruby decoder_scripts/opcodes_process.rb < decoder_originals/opcodes-v3.def | grep -v ARCv2HS | cat > opcodes-v3.def
ruby decoder_scripts/opcodes_process.rb < decoder_originals/opcodes.def > opcodes.def
ruby decoder_scripts/decode_tests.rb
