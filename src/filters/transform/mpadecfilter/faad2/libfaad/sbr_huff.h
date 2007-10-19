/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Software using this code must display the following message visibly in or
** on each copy of the software:
** "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
** in, for example, the about-box or help/startup screen.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: sbr_huff.h,v 1.20 2007/10/11 18:41:51 menno Exp $
**/

#ifndef __SBR_HUFF_H__
#define __SBR_HUFF_H__

#ifdef __cplusplus
extern "C" {
#endif


void sbr_envelope(bitfile *ld, sbr_info *sbr, uint8_t ch);
void sbr_noise(bitfile *ld, sbr_info *sbr, uint8_t ch);

#ifdef __cplusplus
}
#endif
#endif

