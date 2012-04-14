//	VirtualDub - Video processing and capture application
//	Graphics support library
//	Copyright (C) 1998-2007 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//  Notes:
//  - VDPixmapBlt is from VirtualDub
//  - sse2 yv12 to yuy2 conversion by Haali
//	(- vd.cpp/h should be renamed to something more sensible already :)


#include "stdafx.h"
#include "vd.h"
#include "vd_asm.h"
#include <intrin.h>

#include <vd2/system/cpuaccel.h>
#include <vd2/system/memory.h>
#include <vd2/system/vdstl.h>

#include <vd2/Kasumi/pixmap.h>
#include <vd2/Kasumi/pixmaputils.h>
#include <vd2/Kasumi/pixmapops.h>

#pragma warning(disable : 4799) // no emms... blahblahblah

void VDCPUTest() {
	SYSTEM_INFO si;

	long lEnableFlags = CPUCheckForExtensions();

	GetSystemInfo(&si);

	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		if (si.wProcessorLevel < 4)
			lEnableFlags &= ~CPUF_SUPPORTS_FPU;		// Not strictly true, but very slow anyway

	// Enable FPU support...

	CPUEnableExtensions(lEnableFlags);

	VDFastMemcpyAutodetect();
}

CCpuID g_cpuid;

CCpuID::CCpuID()
{
	VDCPUTest();

	long lEnableFlags = CPUGetEnabledExtensions();

	int flags = 0;
	flags |= !!(lEnableFlags & CPUF_SUPPORTS_MMX)			? mmx		: 0;			// STD MMX
	flags |= !!(lEnableFlags & CPUF_SUPPORTS_INTEGER_SSE)	? ssemmx	: 0;			// SSE MMX
	flags |= !!(lEnableFlags & CPUF_SUPPORTS_SSE)			? ssefpu	: 0;			// STD SSE
	flags |= !!(lEnableFlags & CPUF_SUPPORTS_SSE2)			? sse2		: 0;			// SSE2
	flags |= !!(lEnableFlags & CPUF_SUPPORTS_3DNOW)			? _3dnow	: 0;			// 3DNow

	// result
	m_flags = (flag_t)flags;
}
