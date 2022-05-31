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
#ifdef TARGET_ARC64
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

DEF_HELPER_2(llock, tl, env, tl)
DEF_HELPER_3(scond, tl, env, tl, tl)
#if defined(TARGET_ARC64)
DEF_HELPER_2(llockl, tl, env, tl)
DEF_HELPER_3(scondl, tl, env, tl, tl)
#elif defined(TARGET_ARC32)
DEF_HELPER_2(llockd, i64, env, tl)
DEF_HELPER_3(scondd, tl, env, tl, i64)
#endif

DEF_HELPER_FLAGS_3(fdadd, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fdsub, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fdmul, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fddiv, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fdmin, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fdmax, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(fsadd, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fssub, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fsmul, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fsdiv, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fsmin, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fsmax, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(fhadd, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fhsub, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fhmul, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fhdiv, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fhmin, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(fhmax, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_4(fdmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(fdmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(fdnmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(fdnmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_4(fsmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(fsmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(fsnmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(fsnmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_4(fhmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(fhmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(fhnmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(fhnmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_3(fdcmp, TCG_CALL_NO_RWG, void, env, i64, i64)
DEF_HELPER_FLAGS_3(fscmp, TCG_CALL_NO_RWG, void, env, i64, i64)
DEF_HELPER_FLAGS_3(fhcmp, TCG_CALL_NO_RWG, void, env, i64, i64)
DEF_HELPER_FLAGS_3(fdcmpf, TCG_CALL_NO_RWG, void, env, i64, i64)
DEF_HELPER_FLAGS_3(fscmpf, TCG_CALL_NO_RWG, void, env, i64, i64)
DEF_HELPER_FLAGS_3(fhcmpf, TCG_CALL_NO_RWG, void, env, i64, i64)

DEF_HELPER_FLAGS_2(fdsqrt, TCG_CALL_NO_RWG, i64, env, i64)
DEF_HELPER_FLAGS_2(fssqrt, TCG_CALL_NO_RWG, i64, env, i64)
DEF_HELPER_FLAGS_2(fhsqrt, TCG_CALL_NO_RWG, i64, env, i64)

DEF_HELPER_FLAGS_2(fs2d, TCG_CALL_NO_RWG, i64, env, i64)
DEF_HELPER_FLAGS_2(fd2s, TCG_CALL_NO_RWG, i64, env, i64)

DEF_HELPER_2(fl2d, i64, env, i64)
DEF_HELPER_2(fd2l, i64, env, i64)
DEF_HELPER_2(fd2l_rz, i64, env, i64)
DEF_HELPER_2(ful2d, i64, env, i64)
DEF_HELPER_2(fd2ul, i64, env, i64)
DEF_HELPER_2(fd2ul_rz, i64, env, i64)

DEF_HELPER_2(fint2d, i64, env, i64)
DEF_HELPER_2(fd2int, i64, env, i64)
DEF_HELPER_2(fd2int_rz, i64, env, i64)
DEF_HELPER_2(fuint2d, i64, env, i64)
DEF_HELPER_2(fd2uint, i64, env, i64)
DEF_HELPER_2(fd2uint_rz, i64, env, i64)

DEF_HELPER_2(fl2s, i64, env, i64)
DEF_HELPER_2(fs2l, i64, env, i64)
DEF_HELPER_2(fs2l_rz, i64, env, i64)
DEF_HELPER_2(ful2s, i64, env, i64)
DEF_HELPER_2(fs2ul, i64, env, i64)
DEF_HELPER_2(fs2ul_rz, i64, env, i64)

DEF_HELPER_2(fint2s, i64, env, i64)
DEF_HELPER_2(fs2int, i64, env, i64)
DEF_HELPER_2(fs2int_rz, i64, env, i64)
DEF_HELPER_2(fuint2s, i64, env, i64)
DEF_HELPER_2(fs2uint, i64, env, i64)
DEF_HELPER_2(fs2uint_rz, i64, env, i64)

DEF_HELPER_2(fdrnd, i64, env, i64)
DEF_HELPER_2(fdrnd_rz, i64, env, i64)
DEF_HELPER_2(fsrnd, i64, env, i64)
DEF_HELPER_2(fsrnd_rz, i64, env, i64)

DEF_HELPER_FLAGS_2(fs2h, TCG_CALL_NO_RWG, i64, env, i64)
DEF_HELPER_FLAGS_2(fs2h_rz, TCG_CALL_NO_RWG, i64, env, i64)
DEF_HELPER_FLAGS_2(fh2s, TCG_CALL_NO_RWG, i64, env, i64)

//DEF_HELPER_FLAGS_3(fdsgnj, TCG_CALL_NO_RWG, i64, env, i64, i64)
//DEF_HELPER_FLAGS_3(fdsgnjn, TCG_CALL_NO_RWG, i64, env, i64, i64)
//DEF_HELPER_FLAGS_3(fdsgnjx, TCG_CALL_NO_RWG, i64, env, i64, i64)
//
//DEF_HELPER_FLAGS_3(fssgnj, TCG_CALL_NO_RWG, i64, env, i64, i64)
//DEF_HELPER_FLAGS_3(fssgnjn, TCG_CALL_NO_RWG, i64, env, i64, i64)
//DEF_HELPER_FLAGS_3(fssgnjx, TCG_CALL_NO_RWG, i64, env, i64, i64)
//
//DEF_HELPER_FLAGS_3(fhsgnj, TCG_CALL_NO_RWG, i64, env, i64, i64)
//DEF_HELPER_FLAGS_3(fhsgnjn, TCG_CALL_NO_RWG, i64, env, i64, i64)
//DEF_HELPER_FLAGS_3(fhsgnjx, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_5(vfins, TCG_CALL_NO_RWG, i64, env, i64, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfext, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_3(vfrep, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(vfhmul, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfsmul, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfdmul, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(vfhdiv, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfsdiv, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfddiv, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_2(vfhsqrt, TCG_CALL_NO_RWG, i64, env, i64)
DEF_HELPER_FLAGS_2(vfssqrt, TCG_CALL_NO_RWG, i64, env, i64)
DEF_HELPER_FLAGS_2(vfdsqrt, TCG_CALL_NO_RWG, i64, env, i64)

DEF_HELPER_FLAGS_3(vfhadd, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfsadd, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfdadd, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(vfhsub, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfssub, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfdsub, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(vfhaddsub, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfsaddsub, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfdaddsub, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(vfhsubadd, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfssubadd, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfdsubadd, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(vfhmuls, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfsmuls, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfdmuls, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(vfhdivs, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfsdivs, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfddivs, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(vfhadds, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfsadds, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfdadds, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_3(vfhsubs, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfssubs, TCG_CALL_NO_RWG, i64, env, i64, i64)
DEF_HELPER_FLAGS_3(vfdsubs, TCG_CALL_NO_RWG, i64, env, i64, i64)

DEF_HELPER_FLAGS_4(vfhmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfsmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfdmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_4(vfhmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfsmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfdmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_4(vfhnmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfsnmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfdnmadd, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_4(vfhnmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfsnmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfdnmsub, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_4(vfhmadds, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfsmadds, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfdmadds, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_4(vfhmsubs, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfsmsubs, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfdmsubs, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_4(vfhnmadds, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfsnmadds, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfdnmadds, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_4(vfhnmsubs, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfsnmsubs, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)
DEF_HELPER_FLAGS_4(vfdnmsubs, TCG_CALL_NO_RWG, i64, env, i64, i64, i64)

DEF_HELPER_FLAGS_2(vfhexch, TCG_CALL_NO_RWG, i64, env, i64)
DEF_HELPER_FLAGS_2(vfsexch, TCG_CALL_NO_RWG, i64, env, i64)

DEF_HELPER_FLAGS_7(vector_shuffle, TCG_CALL_NO_RWG, i64, env, tl, tl, i64, i64, i64, i64)
