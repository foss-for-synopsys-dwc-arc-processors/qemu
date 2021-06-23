dir = meson.current_source_dir()

gen32 = [
    dir + '/decoder.c',
    dir + '/semfunc.c',
    dir + '/mmu.c',
]
gen64 = [
    dir + '/decoder-v3.c',
    dir + '/semfunc-v3.c',
    dir + '/mmu-v6.c',
]

arc_softmmu_ss = ss.source_set()
arc_softmmu_ss.add(when: 'TARGET_ARCV2', if_true: gen32)
arc_softmmu_ss.add(when: 'TARGET_ARCV3', if_true: gen64)

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
  'arconnect.c'
))

target_arch += {'arc': arc_softmmu_ss}
target_softmmu_arch += {'arc': arc_softmmu_ss}
