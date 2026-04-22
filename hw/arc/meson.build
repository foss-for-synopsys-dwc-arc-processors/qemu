arc_ss = ss.source_set()
arc_ss.add(files('pic_cpu.c'))
arc_ss.add(when: 'CONFIG_ARC_SIM', if_true: files('sim.c'))
arc_ss.add(when: 'CONFIG_ARC_VIRT', if_true: files('virt.c'))

hw_arch += {'arc': arc_ss}
