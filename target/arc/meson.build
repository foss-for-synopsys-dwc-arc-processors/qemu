dir = meson.current_source_dir()

gen32 = [
  dir + '/semfunc.c',
]
gen64 = [
  dir + '/semfunc-v3.c',
]

arc_softmmu_ss = ss.source_set()
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
