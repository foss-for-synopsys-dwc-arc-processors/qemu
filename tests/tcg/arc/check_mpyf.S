
.include "macros.inc"

.macro validate res, actual, z, n, c, v, test_num
  assert_eq   \res, \actual, \test_num
  assert_flag REG_STAT_Z, \z, \test_num
  assert_flag REG_STAT_N, \n, \test_num
  assert_flag REG_STAT_C, \c, \test_num
  assert_flag REG_STAT_V, \v, \test_num
.endm

.macro mpy_flags_test val1, val2, res, z=0, n=0, c=0, v=0, test_num=1
  mov    r0, \val1
  mov    r1, \val2
  mpy.f  r2, r0, r1
  validate  \res, r2, \z, \n, \c, \v, \test_num
.endm

.macro mpyu_flags_test val1, val2, res, z=0, n=0, c=0, v=0, test_num=1
  mov    r0, \val1
  mov    r1, \val2
  mpyu.f  r2, r0, r1
  validate  \res, r2, \z, \n, \c, \v, \test_num
.endm

start

	mpy_flags_test 0x12345678, 0x07654321, 0x70b88d78, z=0, n=0, c=0, v=1, test_num=0x01

	mpyu_flags_test 0x1, 0x80000000, 0x80000000, z=0, n=0, c=0, v=0, test_num=0x02

	print	"*** EXIT code 0\n"
end
