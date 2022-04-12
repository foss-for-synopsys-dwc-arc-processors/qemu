#include "qemu/osdep.h"
#include "target/arc/semfunc.h"

#define SEMANTIC_FUNCTION_PROTOTYPE_0(NAME) \
  int __attribute__ ((weak)) arc_gen_##NAME (DisasCtxt *ctx) { \
      assert("SOME SEMFUNC " #NAME " WAS NOT IMPLEMENTED" == 0); \
      return 0; \
  }
#define SEMANTIC_FUNCTION_PROTOTYPE_1(NAME) \
  int __attribute__ ((weak)) arc_gen_##NAME (DisasCtxt *ctx, TCGv a) { \
      assert("SOME SEMFUNC " #NAME " WAS NOT IMPLEMENTED" == 0); \
      return 0; \
  }
#define SEMANTIC_FUNCTION_PROTOTYPE_2(NAME) \
  int __attribute__ ((weak)) arc_gen_##NAME (DisasCtxt *ctx, TCGv a, TCGv b) { \
      assert("SOME SEMFUNC " #NAME " WAS NOT IMPLEMENTED" == 0); \
      return 0; \
  }
#define SEMANTIC_FUNCTION_PROTOTYPE_3(NAME) \
  int __attribute__ ((weak)) arc_gen_##NAME (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c) { \
      assert("SOME SEMFUNC " #NAME " WAS NOT IMPLEMENTED" == 0); \
      return 0; \
  }
#define SEMANTIC_FUNCTION_PROTOTYPE_4(NAME) \
  int __attribute__ ((weak)) arc_gen_##NAME (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c, TCGv d) { \
      assert("SOME SEMFUNC " #NAME " WAS NOT IMPLEMENTED" == 0); \
      return 0; \
  }

#define MAPPING(MNEMONIC, NAME, NOPS, ...)
#define CONSTANT(...)
#define SEMANTIC_FUNCTION(NAME, NOPS) \
  SEMANTIC_FUNCTION_PROTOTYPE_##NOPS(NAME)

#include "semfunc-mapping.def"

#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION_PROTOTYPE_0
#undef SEMANTIC_FUNCTION_PROTOTYPE_1
#undef SEMANTIC_FUNCTION_PROTOTYPE_2
#undef SEMANTIC_FUNCTION_PROTOTYPE_3
#undef SEMANTIC_FUNCTION_PROTOTYPE_4
#undef SEMANTIC_FUNCTION
