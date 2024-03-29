/*
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_X86_CABAC_H
#define AVCODEC_X86_CABAC_H

#include "libavcodec/cabac.h"
#include "libavutil/attributes.h"
#include "libavutil/x86_cpu.h"
#include "config.h"

#if HAVE_FAST_CMOV
#define BRANCHLESS_GET_CABAC_UPDATE(ret, statep, low, range, tmp)\
        "mov    "tmp"       , %%ecx     \n\t"\
        "shl    $17         , "tmp"     \n\t"\
        "cmp    "low"       , "tmp"     \n\t"\
        "cmova  %%ecx       , "range"   \n\t"\
        "sbb    %%ecx       , %%ecx     \n\t"\
        "and    %%ecx       , "tmp"     \n\t"\
        "xor    %%ecx       , "ret"     \n\t"\
        "sub    "tmp"       , "low"     \n\t"
#else /* HAVE_FAST_CMOV */
#define BRANCHLESS_GET_CABAC_UPDATE(ret, statep, low, range, tmp)\
        "mov    "tmp"       , %%ecx     \n\t"\
        "shl    $17         , "tmp"     \n\t"\
        "sub    "low"       , "tmp"     \n\t"\
        "sar    $31         , "tmp"     \n\t" /*lps_mask*/\
        "sub    %%ecx       , "range"   \n\t" /*RangeLPS - range*/\
        "and    "tmp"       , "range"   \n\t" /*(RangeLPS - range)&lps_mask*/\
        "add    %%ecx       , "range"   \n\t" /*new range*/\
        "shl    $17         , %%ecx     \n\t"\
        "and    "tmp"       , %%ecx     \n\t"\
        "sub    %%ecx       , "low"     \n\t"\
        "xor    "tmp"       , "ret"     \n\t"
#endif /* HAVE_FAST_CMOV */

#define BRANCHLESS_GET_CABAC(ret, statep, low, lowword, range, tmp, tmpbyte, byte, end) \
        "movzbl "statep"    , "ret"                                     \n\t"\
        "mov    "range"     , "tmp"                                     \n\t"\
        "and    $0xC0       , "range"                                   \n\t"\
        "movzbl "MANGLE(ff_h264_lps_range)"("ret", "range", 2), "range" \n\t"\
        "sub    "range"     , "tmp"                                     \n\t"\
        BRANCHLESS_GET_CABAC_UPDATE(ret, statep, low, range, tmp)   \
        "movzbl " MANGLE(ff_h264_norm_shift) "("range"), %%ecx          \n\t"\
        "shl    %%cl        , "range"                                   \n\t"\
        "movzbl "MANGLE(ff_h264_mlps_state)"+128("ret"), "tmp"          \n\t"\
        "shl    %%cl        , "low"                                     \n\t"\
        "mov    "tmpbyte"   , "statep"                                  \n\t"\
        "test   "lowword"   , "lowword"                                 \n\t"\
        " jnz   2f                                                      \n\t"\
        "mov    "byte"      , %%"REG_c"                                 \n\t"\
        "add"OPSIZE" $2     , "byte"                                    \n\t"\
        "movzwl (%%"REG_c")     , "tmp"                                 \n\t"\
        "lea    -1("low")   , %%ecx                                     \n\t"\
        "xor    "low"       , %%ecx                                     \n\t"\
        "shr    $15         , %%ecx                                     \n\t"\
        "bswap  "tmp"                                                   \n\t"\
        "shr    $15         , "tmp"                                     \n\t"\
        "movzbl " MANGLE(ff_h264_norm_shift) "(%%ecx), %%ecx            \n\t"\
        "sub    $0xFFFF     , "tmp"                                     \n\t"\
        "neg    %%ecx                                                   \n\t"\
        "add    $7          , %%ecx                                     \n\t"\
        "shl    %%cl        , "tmp"                                     \n\t"\
        "add    "tmp"       , "low"                                     \n\t"\
        "2:                                                             \n\t"


#if HAVE_7REGS && !defined(BROKEN_RELOCATIONS) && !(defined(__i386) && defined(__clang__) && (__clang_major__<2 || (__clang_major__==2 && __clang_minor__<10)))\
                                               && !(defined(__i386) && !defined(__clang__) && defined(__llvm__) && __GNUC__==4 && __GNUC_MINOR__==2 && __GNUC_PATCHLEVEL__<=1)
#define get_cabac_inline get_cabac_inline_x86
static av_always_inline int get_cabac_inline_x86(CABACContext *c,
                                                 uint8_t *const state)
{
    int bit, tmp;

    __asm__ volatile(
        BRANCHLESS_GET_CABAC("%0", "(%4)", "%1", "%w1",
                             "%2", "%3", "%b3",
                             "%a6(%5)", "%a7(%5)")
        : "=&r"(bit), "+&r"(c->low), "+&r"(c->range), "=&q"(tmp)
        : "r"(state), "r"(c),
          "i"(offsetof(CABACContext, bytestream)),
          "i"(offsetof(CABACContext, bytestream_end))
        : "%"REG_c, "memory"
    );
    return bit & 1;
}
#endif /* HAVE_7REGS && !defined(BROKEN_RELOCATIONS) */

#define get_cabac_bypass_sign get_cabac_bypass_sign_x86
static av_always_inline int get_cabac_bypass_sign_x86(CABACContext *c, int val)
{
    x86_reg tmp;
    __asm__ volatile(
        "movl        %a6(%2), %k1       \n\t"
        "movl        %a3(%2), %%eax     \n\t"
        "shl             $17, %k1       \n\t"
        "add           %%eax, %%eax     \n\t"
        "sub             %k1, %%eax     \n\t"
        "cltd                           \n\t"
        "and           %%edx, %k1       \n\t"
        "add             %k1, %%eax     \n\t"
        "xor           %%edx, %%ecx     \n\t"
        "sub           %%edx, %%ecx     \n\t"
        "test           %%ax, %%ax      \n\t"
        "jnz              1f            \n\t"
        "mov         %a4(%2), %1        \n\t"
        "subl        $0xFFFF, %%eax     \n\t"
        "movzwl         (%1), %%edx     \n\t"
        "bswap         %%edx            \n\t"
        "shrl            $15, %%edx     \n\t"
        "add              $2, %1        \n\t"
        "addl          %%edx, %%eax     \n\t"
        "mov              %1, %a4(%2)   \n\t"
        "1:                             \n\t"
        "movl          %%eax, %a3(%2)   \n\t"

        : "+c"(val), "=&r"(tmp)
        : "r"(c),
          "i"(offsetof(CABACContext, low)),
          "i"(offsetof(CABACContext, bytestream)),
          "i"(offsetof(CABACContext, bytestream_end)),
          "i"(offsetof(CABACContext, range))
        : "%eax", "%edx", "memory"
    );
    return val;
}

#endif /* AVCODEC_X86_CABAC_H */
