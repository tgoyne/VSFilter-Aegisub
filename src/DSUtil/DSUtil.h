/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "vd.h"
#include "text.h"

#define LCID_NOSUBTITLES -1

extern void memsetd(void* dst, unsigned int c, size_t nbytes);
extern void memsetw(void* dst, unsigned short c, size_t nbytes);
extern unsigned __int64 GetFileVersion(LPCTSTR fn);
extern CStringW GetFriendlyName(CStringW DisplayName);
extern CString MakeFullPath(LPCTSTR path);
extern CString GetMediaTypeName(const GUID& guid);
extern GUID GUIDFromCString(CString str);
extern HRESULT GUIDFromCString(CString str, GUID& guid);
extern CString CStringFromGUID(const GUID& guid);
extern CStringW UTF8To16(LPCSTR utf8);
extern CStringA UTF16To8(LPCWSTR utf16);
extern BOOL CFileGetStatus(LPCTSTR lpszFileName, CFileStatus& status);
extern void DumpBuffer(BYTE* pBuffer, int nSize);
extern CString ReftimeToString(const REFERENCE_TIME& rtVal);
extern CString ReftimeToString2(const REFERENCE_TIME& rtVal);
extern CString DVDtimeToString(const DVD_HMSF_TIMECODE& rtVal, bool bAlwaysShowHours=false);
REFERENCE_TIME StringToReftime(LPCTSTR strVal);
extern COLORREF YCrCbToRGB_Rec601(BYTE Y, BYTE Cr, BYTE Cb);
extern COLORREF YCrCbToRGB_Rec709(BYTE Y, BYTE Cr, BYTE Cb);
extern DWORD	YCrCbToRGB_Rec601(BYTE A, BYTE Y, BYTE Cr, BYTE Cb);
extern DWORD	YCrCbToRGB_Rec709(BYTE A, BYTE Y, BYTE Cr, BYTE Cb);
extern void		SetThreadName( DWORD dwThreadID, LPCSTR szThreadName);
extern void		HexDump(CString fName, BYTE* buf, int size);
extern DWORD	GetDefChannelMask(WORD nChannels);
extern void		CorrectComboListWidth(CComboBox& m_pComboBox);

extern void		getExtraData(const BYTE *format, const GUID *formattype, const size_t formatlen, BYTE *extra, unsigned int *extralen);
extern void		audioFormatTypeHandler(const BYTE *format, const GUID *formattype, DWORD *pnSamples, WORD *pnChannels, WORD *pnBitsPerSample, WORD *pnBlockAlign, DWORD *pnBytesPerSec);

#define QI(i) (riid == __uuidof(i)) ? GetInterface((i*)this, ppv) :
#define QI2(i) (riid == IID_##i) ? GetInterface((i*)this, ppv) :

template <typename T> __inline void INITDDSTRUCT(T& dd)
{
	ZeroMemory(&dd, sizeof(dd));
	dd.dwSize = sizeof(dd);
}

#define countof(array) (sizeof(array)/sizeof(array[0]))

template <class T>
static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
	*phr = S_OK;
	CUnknown* punk = DNew T(lpunk, phr);
	if (punk == NULL) {
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

inline int LNKO(int a, int b)
{
	if (a == 0 || b == 0) {
		return(1);
	}
	while (a != b) {
		if (a < b) {
			b -= a;
		} else if (a > b) {
			a -= b;
		}
	}
	return(a);
}
