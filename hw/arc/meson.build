arc_ss = ss.source_set()
arc_ss.add(files(
  'arc_sim.c',
  'pic_cpu.c',
  'boot.c',
  'arc_virt.c',
))
arc_ss.add(when: 'CONFIG_ARC_VIRT', if_true: files('virt.c'))
arc_ss.add(when: 'CONFIG_ARC_HSDK', if_true: files('hsdk_smp.c'))

hw_arch += {'arc': arc_ss}
