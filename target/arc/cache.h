/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2019 Synopsys, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ARC_CACHE_H__
#define __ARC_CACHE_H__

struct arc_cache {
    bool ic_disabled;
    bool dc_disabled;
    bool dc_inv_mode;
    uint32_t ic_ivir;
    uint32_t ic_endr;
    uint32_t ic_ptag;
    uint32_t ic_ptag_hi;
    uint32_t dc_endr;
    uint32_t dc_ptag_hi;
};

#endif /* __ARC_CACHE_H__ */
