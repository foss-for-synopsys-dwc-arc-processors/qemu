/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Cupertino Miranda (cmiranda@synopsys.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * http://www.gnu.org/licenses/lgpl-2.1.html
 */

#ifndef ARC64_MMUV3_H
#define ARC64_MMUV3_H

struct arc_mmuv6 {
    struct mmuv6_exception {
      int32_t number;
      uint8_t causecode;
      uint8_t parameter;
    } exception;
};

int mmuv6_enabled(void);

#endif /* ARC64_MMUV3_H */
