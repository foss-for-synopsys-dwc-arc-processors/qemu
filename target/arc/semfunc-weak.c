#include "qemu/osdep.h"
#include "target/arc/semfunc.h"

#define DEF_NOT_IMPLEMENT(COUNT, ...) \
int __not_implemented_semfunc_##COUNT(DisasCtxt *ctx, __VA_ARGS__) \
{ \
  assert("SOME SEMFUNC " #COUNT " WAS NOT IMPLEMENTED" == 0); \
  return 0; \
}
int __not_implemented_semfunc_0(DisasCtxt *ctx)
{
  assert("SOME SEMFUNC 0 WAS NOT IMPLEMENTED" == 0);
  return 0;
}
DEF_NOT_IMPLEMENT(1, TCGv a)
DEF_NOT_IMPLEMENT(2, TCGv a, TCGv b)
DEF_NOT_IMPLEMENT(3, TCGv a, TCGv b, TCGv c)
DEF_NOT_IMPLEMENT(4, TCGv a, TCGv b, TCGv c, TCGv d)
#undef DEF_NOT_IMPLEMENT

#define SEMANTIC_FUNCTION_PROTOTYPE_0(NAME) \
  int arc_gen_##NAME (DisasCtxt *ctx) \
         __attribute__ ((weak, alias("__not_implemented_semfunc_0")));
#define SEMANTIC_FUNCTION_PROTOTYPE_1(NAME) \
  int arc_gen_##NAME (DisasCtxt *, TCGv) \
         __attribute__ ((weak, alias("__not_implemented_semfunc_1")));
#define SEMANTIC_FUNCTION_PROTOTYPE_2(NAME) \
  int arc_gen_##NAME (DisasCtxt *, TCGv, TCGv) \
         __attribute__ ((weak, alias("__not_implemented_semfunc_2")));
#define SEMANTIC_FUNCTION_PROTOTYPE_3(NAME) \
  int arc_gen_##NAME (DisasCtxt *, TCGv, TCGv, TCGv) \
         __attribute__ ((weak, alias("__not_implemented_semfunc_3")));
#define SEMANTIC_FUNCTION_PROTOTYPE_4(NAME) \
  int arc_gen_##NAME (DisasCtxt *, TCGv, TCGv, TCGv, TCGv) \
         __attribute__ ((weak, alias("__not_implemented_semfunc_4")));

#define MAPPING(MNEMONIC, NAME, NOPS, ...)
#define CONSTANT(...)
#define SEMANTIC_FUNCTION(NAME, NOPS) \
  SEMANTIC_FUNCTION_PROTOTYPE_##NOPS(NAME)

#include "semfunc-full_mapping.def"

#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION_PROTOTYPE_0
#undef SEMANTIC_FUNCTION_PROTOTYPE_1
#undef SEMANTIC_FUNCTION_PROTOTYPE_2
#undef SEMANTIC_FUNCTION_PROTOTYPE_3
#undef SEMANTIC_FUNCTION_PROTOTYPE_4
#undef SEMANTIC_FUNCTION
