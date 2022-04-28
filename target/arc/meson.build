dir = meson.current_source_dir()

gen32 = [
  dir + '/semfunc.c',
]
gen64 = [
  dir + '/semfunc-v3.c',
]

arc_decoder = find_program('decoder_fragments/arc_gen_decoder.py')

gen = custom_target('gen-decoder',
                    input : ['decoder_fragments/arc64-tbl.h', 'decoder_fragments/arc-tbl.h'],
                    output : ['opcodes.def', 'v2_em_dtree.def', 'v2_hs_dtree.def',
                              'v3_hs5x_dtree.def', 'v3_hs6x_dtree.def'],
                    command : [arc_decoder, '@INPUT@',
                        '--opcodes', '@OUTPUT0@',
                        '--arcv2em', '@OUTPUT1@',
                        '--arcv2hs', '@OUTPUT2@',
                        '--arcv3hs5x', '@OUTPUT3@',
                        '--arcv3hs6x', '@OUTPUT4@'])

arc_softmmu_ss = ss.source_set()
arc_softmmu_ss.add(gen)
arc_softmmu_ss.add(when: 'TARGET_ARC32', if_true: gen32)
arc_softmmu_ss.add(when: 'TARGET_ARC64', if_true: gen64)
arc_softmmu_ss.add(when: 'CONFIG_USER_ONLY', if_false: files('arc-semi.c'))

arc_softmmu_ss.add(files(
  'translate.c',
  'helper.c',
  'cpu.c',
  'op_helper.c',
  'gdbstub.c',
  'regs.c',
  'regs-impl.c',
  'semfunc-helper.c',
  'mpu.c',
  'timer.c',
  'irq.c',
  'cache.c',
  'arconnect.c',
  'mmu-v6.c',
  'mmu.c',
  'mmu-common.c',
  'decoder.c',
  'decoder-v3.c',
  'decoder-v2.c',
  'semfunc-weak.c',
))

target_arch += {'arc': arc_softmmu_ss}
target_softmmu_arch += {'arc': arc_softmmu_ss}
