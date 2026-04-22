#ifndef ARC_CPU_QOM_H
#define ARC_CPU_QOM_H

#include "hw/core/cpu.h"

#define TYPE_ARC_CPU "arc-cpu"

#define TYPE_ARC_CPU_EM   ARC_CPU_TYPE_NAME("arcem")
#define TYPE_ARC_CPU_HS   ARC_CPU_TYPE_NAME("archs")
#define TYPE_ARC_CPU_HS5X ARC_CPU_TYPE_NAME("hs5x")
#define TYPE_ARC_CPU_HS6X ARC_CPU_TYPE_NAME("hs6x")

OBJECT_DECLARE_CPU_TYPE(ARCCPU, ARCCPUClass, ARC_CPU)

#define ARC_CPU_TYPE_SUFFIX "-" TYPE_ARC_CPU
#define ARC_CPU_TYPE_NAME(model) model ARC_CPU_TYPE_SUFFIX

#endif
