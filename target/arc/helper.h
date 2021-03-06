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
DEF_HELPER_2(lr, tl, env, i32)
DEF_HELPER_3(sr, void, env, i32, i32)
DEF_HELPER_2(halt, noreturn, env, i32)
DEF_HELPER_1(rtie, void, env)
DEF_HELPER_1(flush, void, env)
DEF_HELPER_4(raise_exception, noreturn, env, i32, i32, i32)
DEF_HELPER_2(zol_verify, void, env, i32)
DEF_HELPER_2(fake_exception, void, env, i32)
DEF_HELPER_2(set_status32, void, env, i32)
DEF_HELPER_1(get_status32, i32, env)
DEF_HELPER_3(set_status32_bit, void, env, i32, i32)

DEF_HELPER_FLAGS_2(ffs, TCG_CALL_NO_RWG_SE, i32, env, i32)
DEF_HELPER_FLAGS_2(fls, TCG_CALL_NO_RWG_SE, i32, env, i32)
DEF_HELPER_FLAGS_3(carry_add_flag, TCG_CALL_NO_RWG_SE, i32, i32, i32, i32)
DEF_HELPER_FLAGS_3(overflow_add_flag, TCG_CALL_NO_RWG_SE, i32, i32, i32, i32)
DEF_HELPER_FLAGS_3(overflow_sub_flag, TCG_CALL_NO_RWG_SE, i32, i32, i32, i32)
DEF_HELPER_FLAGS_3(mpymu, TCG_CALL_NO_RWG_SE, i32, env, i32, i32)
DEF_HELPER_FLAGS_3(mpym, TCG_CALL_NO_RWG_SE, i32, env, i32, i32)
DEF_HELPER_FLAGS_3(repl_mask, TCG_CALL_NO_RWG_SE, i32, i32, i32, i32)
