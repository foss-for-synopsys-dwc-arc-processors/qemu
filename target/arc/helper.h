/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synopsys Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * href="http://www.gnu.org/licenses/lgpl-2.1.html
 */

DEF_HELPER_1(debug, void, env)
DEF_HELPER_2(lr, tl, env, tl)
DEF_HELPER_3(sr, void, env, tl, tl)
DEF_HELPER_2(halt, noreturn, env, tl)
DEF_HELPER_1(rtie, void, env)
DEF_HELPER_1(flush, void, env)
DEF_HELPER_4(raise_exception, noreturn, env, tl, tl, tl)
DEF_HELPER_2(zol_verify, void, env, tl)
DEF_HELPER_2(fake_exception, void, env, tl)
DEF_HELPER_2(set_status32, void, env, tl)
DEF_HELPER_1(get_status32, tl, env)
DEF_HELPER_3(set_status32_bit, void, env, tl, tl)

DEF_HELPER_FLAGS_3(carry_add_flag, TCG_CALL_NO_RWG_SE, tl, tl, tl, tl)
DEF_HELPER_FLAGS_3(overflow_add_flag, TCG_CALL_NO_RWG_SE, tl, tl, tl, tl)
DEF_HELPER_FLAGS_3(overflow_sub_flag, TCG_CALL_NO_RWG_SE, tl, tl, tl, tl)
DEF_HELPER_FLAGS_3(mpymu, TCG_CALL_NO_RWG_SE, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(mpym, TCG_CALL_NO_RWG_SE, tl, env, tl, tl)
DEF_HELPER_FLAGS_3(repl_mask, TCG_CALL_NO_RWG_SE, tl, tl, tl, tl)

/* ARCV3 helpers */
#ifdef TARGET_ARCV3
DEF_HELPER_FLAGS_2(ffs32, TCG_CALL_NO_RWG_SE, tl, env, tl)

DEF_HELPER_FLAGS_3(carry_add_flag32, TCG_CALL_NO_RWG_SE, tl, tl, tl, tl)
DEF_HELPER_FLAGS_3(carry_sub_flag32, TCG_CALL_NO_RWG_SE, tl, tl, tl, tl)
DEF_HELPER_FLAGS_3(overflow_add_flag32, TCG_CALL_NO_RWG_SE, tl, tl, tl, tl)
DEF_HELPER_FLAGS_3(overflow_sub_flag32, TCG_CALL_NO_RWG_SE, tl, tl, tl, tl)

DEF_HELPER_FLAGS_2(rotate_left32, TCG_CALL_NO_RWG_SE, i64, i64, i64)
DEF_HELPER_FLAGS_2(rotate_right32, TCG_CALL_NO_RWG_SE, i64, i64, i64)
DEF_HELPER_FLAGS_2(asr_32, TCG_CALL_NO_RWG_SE, i64, i64, i64)

DEF_HELPER_2(norml, i64, env, i64)
#endif
