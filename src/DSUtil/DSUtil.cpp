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

#include "stdafx.h"
#include <Vfw.h>
#include <winddk/devioctl.h>
#include <winddk/ntddcdrm.h>
#include "DSUtil.h"
#include "vd.h"
#include <moreuuids.h>
#include <emmintrin.h>
#include <math.h>
#include <InitGuid.h>
#include <d3d9types.h>

void CStringToBin(CString str, CAtlArray<BYTE>& data)
{
	str.Trim();
	ASSERT((str.GetLength()&1) == 0);
	data.SetCount(str.GetLength()/2);

	BYTE b = 0;

	str.MakeUpper();
	for (size_t i = 0, j = str.GetLength(); i < j; i++) {
		TCHAR c = str[i];
		if (c >= '0' && c <= '9') {
			if (!(i&1)) {
				b = ((char(c-'0')<<4)&0xf0)|(b&0x0f);
			} else {
				b = (char(c-'0')&0x0f)|(b&0xf0);
			}
		} else if (c >= 'A' && c <= 'F') {
			if (!(i&1)) {
				b = ((char(c-'A'+10)<<4)&0xf0)|(b&0x0f);
			} else {
				b = (char(c-'A'+10)&0x0f)|(b&0xf0);
			}
		} else {
			break;
		}

		if (i&1) {
			data[i>>1] = b;
			b = 0;
		}
	}
}

CString BinToCString(BYTE* ptr, int len)
{
	CString ret;

	while (len-- > 0) {
		TCHAR high, low;
		high = (*ptr>>4) >= 10 ? (*ptr>>4)-10 + 'A' : (*ptr>>4) + '0';
		low = (*ptr&0xf) >= 10 ? (*ptr&0xf)-10 + 'A' : (*ptr&0xf) + '0';

		CString str;
		str.Format(_T("%c%c"), high, low);
		ret += str;

		ptr++;
	}

	return(ret);
}

static void FindFiles(CString fn, CAtlList<CString>& files)
{
	CString path = fn;
	path.Replace('/', '\\');
	path = path.Left(path.ReverseFind('\\')+1);

	WIN32_FIND_DATA findData;
	HANDLE h = FindFirstFile(fn, &findData);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			files.AddTail(path + findData.cFileName);
		} while (FindNextFile(h, &findData));

		FindClose(h);
	}
}

CString GetDriveLabel(TCHAR drive)
{
	CString label;

	CString path;
	path.Format(_T("%c:\\"), drive);
	TCHAR VolumeNameBuffer[MAX_PATH], FileSystemNameBuffer[MAX_PATH];
	DWORD VolumeSerialNumber, MaximumComponentLength, FileSystemFlags;
	if (GetVolumeInformation(path,
							 VolumeNameBuffer, MAX_PATH, &VolumeSerialNumber, &MaximumComponentLength,
							 &FileSystemFlags, FileSystemNameBuffer, MAX_PATH)) {
		label = VolumeNameBuffer;
	}

	return(label);
}

DVD_HMSF_TIMECODE RT2HMSF(REFERENCE_TIME rt, double fps) // use to remember the current position
{
	DVD_HMSF_TIMECODE hmsf = {
		(BYTE)((rt/10000000/60/60)),
		(BYTE)((rt/10000000/60)%60),
		(BYTE)((rt/10000000)%60),
		(BYTE)(1.0*((rt/10000)%1000) * fps / 1000)
	};

	return hmsf;
}

DVD_HMSF_TIMECODE RT2HMS_r(REFERENCE_TIME rt) // use only for information (for display on the screen)
{
	rt = (rt + 5000000) / 10000000;
	DVD_HMSF_TIMECODE hmsf = {
		(BYTE)(rt / 3600),
		(BYTE)(rt / 60 % 60),
		(BYTE)(rt % 60),
		0
	};

	return hmsf;
}

REFERENCE_TIME HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps)
{
	if (fps == 0) {
		hmsf.bFrames = 0;
		fps = 1;
	}
	return (REFERENCE_TIME)((((REFERENCE_TIME)hmsf.bHours*60+hmsf.bMinutes)*60+hmsf.bSeconds)*1000+1.0*hmsf.bFrames*1000/fps)*10000;
}

void memsetd(void* dst, unsigned int c, size_t nbytes)
{
#ifndef _WIN64
	if (!(g_cpuid.m_flags & g_cpuid.sse2)) {
		__asm {
			mov eax, c
			mov ecx, nbytes
			shr ecx, 2
			mov edi, dst
			cld
			rep stosd
		}
		return;
	}
#endif
	size_t n = nbytes / 4;
	size_t o = n - (n % 4);

	__m128i val = _mm_set1_epi32 ( (int)c );
	if (((uintptr_t)dst & 0x0F) == 0) { // 16-byte aligned
		for (size_t i = 0; i < o; i+=4) {
			_mm_store_si128( (__m128i*)&(((DWORD*)dst)[i]), val );
		}
	} else {
		for (size_t i = 0; i < o; i+=4) {
			_mm_storeu_si128( (__m128i*)&(((DWORD*)dst)[i]), val );
		}
	}

	switch (n - o) {
		case 3:
				((DWORD*)dst)[o + 2] = c;
		case 2:
				((DWORD*)dst)[o + 1] = c;
		case 1:
				((DWORD*)dst)[o + 0] = c;
	}
}

void memsetw(void* dst, unsigned short c, size_t nbytes)
{
	memsetd(dst, c << 16 | c, nbytes);

	size_t n = nbytes / 2;
	size_t o = (n / 2) * 2;
	if ((n - o) == 1) {
		((WORD*)dst)[o] = c;
	}
}

CStringW GetFriendlyName(CStringW DisplayName)
{
	CStringW FriendlyName;

	CComPtr<IBindCtx> pBindCtx;
	CreateBindCtx(0, &pBindCtx);

	CComPtr<IMoniker> pMoniker;
	ULONG chEaten;
	if (S_OK != MkParseDisplayName(pBindCtx, CComBSTR(DisplayName), &chEaten, &pMoniker)) {
		return false;
	}

	CComPtr<IPropertyBag> pPB;
	CComVariant var;
	if (SUCCEEDED(pMoniker->BindToStorage(pBindCtx, 0, IID_IPropertyBag, (void**)&pPB))
			&& SUCCEEDED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL))) {
		FriendlyName = var.bstrVal;
	}

	return FriendlyName;
}

typedef struct {
	CString path;
	HINSTANCE hInst;
	CLSID clsid;
} ExternalObject;

static CAtlList<ExternalObject> s_extobjs;

HRESULT LoadExternalObject(LPCTSTR path, REFCLSID clsid, REFIID iid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	CString fullpath = MakeFullPath(path);

	HINSTANCE hInst = NULL;
	bool fFound = false;

	POSITION pos = s_extobjs.GetHeadPosition();
	while (pos) {
		ExternalObject& eo = s_extobjs.GetNext(pos);
		if (!eo.path.CompareNoCase(fullpath)) {
			hInst = eo.hInst;
			fFound = true;
			break;
		}
	}

	HRESULT hr = E_FAIL;

	if (!hInst) {
		hInst = CoLoadLibrary(CComBSTR(fullpath), TRUE);
	}
	if (hInst) {
		typedef HRESULT (__stdcall * PDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
		PDllGetClassObject p = (PDllGetClassObject)GetProcAddress(hInst, "DllGetClassObject");

		if (p && FAILED(hr = p(clsid, iid, ppv))) {
			CComPtr<IClassFactory> pCF;
			if (SUCCEEDED(hr = p(clsid, __uuidof(IClassFactory), (void**)&pCF))) {
				hr = pCF->CreateInstance(NULL, iid, ppv);
			}
		}
	}

	if (FAILED(hr) && hInst && !fFound) {
		CoFreeLibrary(hInst);
		return hr;
	}

	if (hInst && !fFound) {
		ExternalObject eo;
		eo.path = fullpath;
		eo.hInst = hInst;
		eo.clsid = clsid;
		s_extobjs.AddTail(eo);
	}

	return hr;
}

HRESULT LoadExternalFilter(LPCTSTR path, REFCLSID clsid, IBaseFilter** ppBF)
{
	return LoadExternalObject(path, clsid, __uuidof(IBaseFilter), (void**)ppBF);
}

HRESULT LoadExternalPropertyPage(IPersist* pP, REFCLSID clsid, IPropertyPage** ppPP)
{
	CLSID clsid2 = GUID_NULL;
	if (FAILED(pP->GetClassID(&clsid2))) {
		return E_FAIL;
	}

	POSITION pos = s_extobjs.GetHeadPosition();
	while (pos) {
		ExternalObject& eo = s_extobjs.GetNext(pos);
		if (eo.clsid == clsid2) {
			return LoadExternalObject(eo.path, clsid, __uuidof(IPropertyPage), (void**)ppPP);
		}
	}

	return E_FAIL;
}

void UnloadExternalObjects()
{
	POSITION pos = s_extobjs.GetHeadPosition();
	while (pos) {
		ExternalObject& eo = s_extobjs.GetNext(pos);
		CoFreeLibrary(eo.hInst);
	}
	s_extobjs.RemoveAll();
}

CString MakeFullPath(LPCTSTR path)
{
	CString full(path);
	full.Replace('/', '\\');

	CString fn;
	fn.ReleaseBuffer(GetModuleFileName(AfxGetInstanceHandle(), fn.GetBuffer(MAX_PATH), MAX_PATH));
	CPath p(fn);

	if (full.GetLength() >= 2 && full[0] == '\\' && full[1] != '\\') {
		p.StripToRoot();
		full = CString(p) + full.Mid(1);
	} else if (full.Find(_T(":\\")) < 0) {
		p.RemoveFileSpec();
		p.AddBackslash();
		full = CString(p) + full;
	}

	CPath c(full);
	c.Canonicalize();
	return CString(c);
}

//

GUID GUIDFromCString(CString str)
{
	GUID guid = GUID_NULL;
	HRESULT hr = CLSIDFromString(CComBSTR(str), &guid);
	ASSERT(SUCCEEDED(hr));
	UNREFERENCED_PARAMETER(hr);
	return guid;
}

HRESULT GUIDFromCString(CString str, GUID& guid)
{
	guid = GUID_NULL;
	return CLSIDFromString(CComBSTR(str), &guid);
}

CString CStringFromGUID(const GUID& guid)
{
	WCHAR null[128] = {0}, buff[128];
	StringFromGUID2(GUID_NULL, null, 127);
	return CString(StringFromGUID2(guid, buff, 127) > 0 ? buff : null);
}

CStringW UTF8To16(LPCSTR utf8)
{
	CStringW str;
	int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0)-1;
	if (n < 0) {
		return str;
	}
	str.ReleaseBuffer(MultiByteToWideChar(CP_UTF8, 0, utf8, -1, str.GetBuffer(n), n+1)-1);
	return str;
}

CStringA UTF16To8(LPCWSTR utf16)
{
	CStringA str;
	int n = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, NULL, NULL)-1;
	if (n < 0) {
		return str;
	}
	str.ReleaseBuffer(WideCharToMultiByte(CP_UTF8, 0, utf16, -1, str.GetBuffer(n), n+1, NULL, NULL)-1);
	return str;
}

BOOL CFileGetStatus(LPCTSTR lpszFileName, CFileStatus& status)
{
	try {
		return CFile::GetStatus(lpszFileName, status);
	} catch (CException* e) {
		// MFCBUG: E_INVALIDARG / "Parameter is incorrect" is thrown for certain cds (vs2003)
		// http://groups.google.co.uk/groups?hl=en&lr=&ie=UTF-8&threadm=OZuXYRzWDHA.536%40TK2MSFTNGP10.phx.gbl&rnum=1&prev=/groups%3Fhl%3Den%26lr%3D%26ie%3DISO-8859-1
		TRACE(_T("CFile::GetStatus has thrown an exception\n"));
		e->Delete();
		return false;
	}
}

// filter registration helpers

bool DeleteRegKey(LPCTSTR pszKey, LPCTSTR pszSubkey)
{
	bool bOK = false;

	HKEY hKey;
	LONG ec = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, pszKey, 0, KEY_ALL_ACCESS, &hKey);
	if (ec == ERROR_SUCCESS) {
		if (pszSubkey != 0) {
			ec = ::RegDeleteKey(hKey, pszSubkey);
		}

		bOK = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return bOK;
}

bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValueName, LPCTSTR pszValue)
{
	bool bOK = false;

	CString szKey(pszKey);
	if (pszSubkey != 0) {
		szKey += CString(_T("\\")) + pszSubkey;
	}

	HKEY hKey;
	LONG ec = ::RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, 0);
	if (ec == ERROR_SUCCESS) {
		if (pszValue != 0) {
			ec = ::RegSetValueEx(hKey, pszValueName, 0, REG_SZ,
								 reinterpret_cast<BYTE*>(const_cast<LPTSTR>(pszValue)),
								 (_tcslen(pszValue) + 1) * sizeof(TCHAR));
		}

		bOK = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return bOK;
}

bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValue)
{
	return SetRegKeyValue(pszKey, pszSubkey, 0, pszValue);
}

void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, LPCTSTR chkbytes, LPCTSTR ext, ...)
{
	CString null = CStringFromGUID(GUID_NULL);
	CString majortype = CStringFromGUID(MEDIATYPE_Stream);
	CString subtype = CStringFromGUID(subtype2);

	SetRegKeyValue(_T("Media Type\\") + majortype, subtype, _T("0"), chkbytes);
	SetRegKeyValue(_T("Media Type\\") + majortype, subtype, _T("Source Filter"), CStringFromGUID(clsid));

	DeleteRegKey(_T("Media Type\\") + null, subtype);

	va_list marker;
	va_start(marker, ext);
	for (; ext; ext = va_arg(marker, LPCTSTR)) {
		DeleteRegKey(_T("Media Type\\Extensions"), ext);
	}
	va_end(marker);
}

void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, const CAtlList<CString>& chkbytes, LPCTSTR ext, ...)
{
	CString null = CStringFromGUID(GUID_NULL);
	CString majortype = CStringFromGUID(MEDIATYPE_Stream);
	CString subtype = CStringFromGUID(subtype2);

	POSITION pos = chkbytes.GetHeadPosition();
	for (ptrdiff_t i = 0; pos; i++) {
		CString idx;
		idx.Format(_T("%d"), i);
		SetRegKeyValue(_T("Media Type\\") + majortype, subtype, idx, chkbytes.GetNext(pos));
	}

	SetRegKeyValue(_T("Media Type\\") + majortype, subtype, _T("Source Filter"), CStringFromGUID(clsid));

	DeleteRegKey(_T("Media Type\\") + null, subtype);

	va_list marker;
	va_start(marker, ext);
	for (; ext; ext = va_arg(marker, LPCTSTR)) {
		DeleteRegKey(_T("Media Type\\Extensions"), ext);
	}
	va_end(marker);
}

void UnRegisterSourceFilter(const GUID& subtype)
{
	DeleteRegKey(_T("Media Type\\") + CStringFromGUID(MEDIATYPE_Stream), CStringFromGUID(subtype));
}

void DumpBuffer(BYTE* pBuffer, int nSize)
{
	CString	strMsg;
	int		nPos = 0;
	strMsg.AppendFormat (L"Size : %d\n", nSize);
	for (int i=0; i<3; i++) {
		for (int j=0; j<32; j++) {
			nPos = i*32 + j;
			if (nPos >= nSize) {
				break;
			}
			strMsg.AppendFormat (L"%02x ", pBuffer[nPos]);
		}
		if (nPos >= nSize) {
			break;
		}
		strMsg.AppendFormat (L"\n");
	}

	if (nSize > 32*3) {
		strMsg.AppendFormat (L".../...\n");
		for (int j=32; j>0; j--) {
			strMsg.AppendFormat (L"%02x ", pBuffer[nSize - j]);
		}
	}
	strMsg.AppendFormat(L"\n");

	TRACE (strMsg);
}

// hour, minute, second, millisec
CString ReftimeToString(const REFERENCE_TIME& rtVal)
{
	CString		strTemp;
	LONGLONG	llTotalMs =  ConvertToMilliseconds (rtVal);
	int			lHour	  = (int)(llTotalMs  / (1000*60*60));
	int			lMinute	  = (llTotalMs / (1000*60)) % 60;
	int			lSecond	  = (llTotalMs /  1000) % 60;
	int			lMillisec = llTotalMs  %  1000;

	strTemp.Format (_T("%02d:%02d:%02d,%03d"), lHour, lMinute, lSecond, lMillisec);
	return strTemp;
}

// hour, minute, second (round)
CString ReftimeToString2(const REFERENCE_TIME& rtVal)
{
	CString		strTemp;
	LONGLONG	seconds =  (rtVal + 5000000) / 10000000;
	int			lHour	  = (int)(seconds / 3600);
	int			lMinute	  = (int)(seconds / 60 % 60);
	int			lSecond	  = (int)(seconds % 60);

	strTemp.Format (_T("%02d:%02d:%02d"), lHour, lMinute, lSecond);
	return strTemp;
}

CString DVDtimeToString(const DVD_HMSF_TIMECODE& rtVal, bool bAlwaysShowHours)
{
	CString	strTemp;
	if (rtVal.bHours > 0 || bAlwaysShowHours) {
		strTemp.Format(_T("%02d:%02d:%02d"), rtVal.bHours, rtVal.bMinutes, rtVal.bSeconds);
	} else {
		strTemp.Format(_T("%02d:%02d"), rtVal.bMinutes, rtVal.bSeconds);
	}
	return strTemp;
}

REFERENCE_TIME StringToReftime(LPCTSTR strVal)
{
	REFERENCE_TIME	rt			= 0;
	int				lHour		= 0;
	int				lMinute		= 0;
	int				lSecond		= 0;
	int				lMillisec	= 0;

	if (_stscanf_s (strVal, _T("%02d:%02d:%02d,%03d"), &lHour, &lMinute, &lSecond, &lMillisec) == 4) {
		rt = ( (((lHour*24)+lMinute)*60 + lSecond) * MILLISECONDS + lMillisec ) * (UNITS/MILLISECONDS);
	}

	return rt;
}

const double Rec601_Kr = 0.299;
const double Rec601_Kb = 0.114;
const double Rec601_Kg = 0.587;
COLORREF YCrCbToRGB_Rec601(BYTE Y, BYTE Cr, BYTE Cb)
{

	double rp = Y + 2*(Cr-128)*(1.0-Rec601_Kr);
	double gp = Y - 2*(Cb-128)*(1.0-Rec601_Kb)*Rec601_Kb/Rec601_Kg - 2*(Cr-128)*(1.0-Rec601_Kr)*Rec601_Kr/Rec601_Kg;
	double bp = Y + 2*(Cb-128)*(1.0-Rec601_Kb);

	return RGB (fabs(rp), fabs(gp), fabs(bp));
}

DWORD YCrCbToRGB_Rec601(BYTE A, BYTE Y, BYTE Cr, BYTE Cb)
{

	double rp = Y + 2*(Cr-128)*(1.0-Rec601_Kr);
	double gp = Y - 2*(Cb-128)*(1.0-Rec601_Kb)*Rec601_Kb/Rec601_Kg - 2*(Cr-128)*(1.0-Rec601_Kr)*Rec601_Kr/Rec601_Kg;
	double bp = Y + 2*(Cb-128)*(1.0-Rec601_Kb);

	return D3DCOLOR_ARGB(A, (BYTE)fabs(rp), (BYTE)fabs(gp), (BYTE)fabs(bp));
}


const double Rec709_Kr = 0.2125;
const double Rec709_Kb = 0.0721;
const double Rec709_Kg = 0.7154;

COLORREF YCrCbToRGB_Rec709(BYTE Y, BYTE Cr, BYTE Cb)
{

	double rp = Y + 2*(Cr-128)*(1.0-Rec709_Kr);
	double gp = Y - 2*(Cb-128)*(1.0-Rec709_Kb)*Rec709_Kb/Rec709_Kg - 2*(Cr-128)*(1.0-Rec709_Kr)*Rec709_Kr/Rec709_Kg;
	double bp = Y + 2*(Cb-128)*(1.0-Rec709_Kb);

	return RGB (fabs(rp), fabs(gp), fabs(bp));
}

DWORD YCrCbToRGB_Rec709(BYTE A, BYTE Y, BYTE Cr, BYTE Cb)
{

	double rp = Y + 2*(Cr-128)*(1.0-Rec709_Kr);
	double gp = Y - 2*(Cb-128)*(1.0-Rec709_Kb)*Rec709_Kb/Rec709_Kg - 2*(Cr-128)*(1.0-Rec709_Kr)*Rec709_Kr/Rec709_Kg;
	double bp = Y + 2*(Cb-128)*(1.0-Rec709_Kb);

	return D3DCOLOR_ARGB (A, (BYTE)fabs(rp), (BYTE)fabs(gp), (BYTE)fabs(bp));
}

//
// Usage: SetThreadName (-1, "MainThread");
//
typedef struct tagTHREADNAME_INFO {
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;

void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try {
		RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info );
	}
	__except (EXCEPTION_CONTINUE_EXECUTION) {
	}
}

void HexDump(CString fileName, BYTE* buf, int size)
{
	if (size<=0) {
		return;
	}

	CString dump_str;
	dump_str.Format(_T("Dump size = %d\n"), size);
	int len, i, j, c;

	for (i=0; i<size; i+=16) {
		len = size - i;
		if (len > 16) {
			len = 16;
		}
		dump_str.AppendFormat(_T("%08x "), i);
		for (j=0; j<16; j++) {
			if (j < len) {
				dump_str.AppendFormat(_T(" %02x"), buf[i+j]);
			}
			else {
				dump_str.AppendFormat(_T("   "));
			}
		}
		dump_str.Append(_T(" "));
		for (j=0; j<len; j++) {
			c = buf[i+j];
			if (c < ' ' || c > '~') {
				c = '.';
			}
			dump_str.AppendFormat(_T("%c"), c);
		}
		dump_str.Append(_T("\n"));
	}
	dump_str.Append(_T("\n"));

	if (!fileName.IsEmpty()) {
		CStdioFile file;
		if (file.Open(fileName, CFile::modeCreate|CFile::modeWrite)) {
			file.WriteString(dump_str);
			file.Close();
		}
	}
	else {
		TRACE(dump_str);
	}
}

DWORD GetDefChannelMask(WORD nChannels)
{
	switch (nChannels) {
		case 1: // 1.0 Mono
				return SPEAKER_FRONT_CENTER;
		case 2: // 2.0 Stereo
				return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
		case 3: // 2.1 Stereo
				return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY;
		case 4: // 4.0 Quad
				return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT
					   | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
		case 5: // 4.1
				return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY
					   | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
		case 6: // 5.1 Side (KSAUDIO_SPEAKER_5POINT1_SURROUND)
				return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
					   | SPEAKER_LOW_FREQUENCY | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT;
		case 7: // 6.1 Side
				return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
					   | SPEAKER_LOW_FREQUENCY | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT
					   | SPEAKER_BACK_CENTER;
		case 8: // 7.1 Surround (KSAUDIO_SPEAKER_7POINT1_SURROUND)
				return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
					   | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
					   | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT;
		case 10: // 9.1 Surround
				return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
					   | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT
					   | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT
					   | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT;
		default:
				return 0;
	}
}

void CorrectComboListWidth(CComboBox& m_pComboBox)
{
	// Find the longest string in the combo box.
	if (m_pComboBox.GetCount() <=0 )
		return;

	CString    str;
	CSize      sz;
	int        dx = 0;
	TEXTMETRIC tm;
	CDC*       pDC = m_pComboBox.GetDC();
	CFont*     pFont = m_pComboBox.GetFont();

	// Select the listbox font, save the old font
	CFont* pOldFont = pDC->SelectObject(pFont);
	// Get the text metrics for avg char width
	pDC->GetTextMetrics(&tm);

	for (int i = 0; i < m_pComboBox.GetCount(); i++)
	{
		m_pComboBox.GetLBText(i, str);
		sz = pDC->GetTextExtent(str);

		// Add the avg width to prevent clipping
		sz.cx += tm.tmAveCharWidth;

		if (sz.cx > dx)
			dx = sz.cx;
	}
	// Select the old font back into the DC
	pDC->SelectObject(pOldFont);
	m_pComboBox.ReleaseDC(pDC);

	// Adjust the width for the vertical scroll bar and the left and right border.
	dx += /*::GetSystemMetrics(SM_CXVSCROLL) + */2*::GetSystemMetrics(SM_CXEDGE);

	// Set the width of the list box so that every item is completely visible.
	m_pComboBox.SetDroppedWidth(dx);
}

unsigned int lav_xiphlacing(unsigned char *s, unsigned int v)
{
	unsigned int n = 0;

	while (v >= 0xff) {
		*s++ = 0xff;
		v -= 0xff;
		n++;
	}
	*s = v;
	n++;
	return n;
}
