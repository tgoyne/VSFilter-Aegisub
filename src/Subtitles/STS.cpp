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
#include "STS.h"
#include <atlbase.h>

#include "RealTextParser.h"
#include <fstream>

//
BYTE CharSetList[] = {
	ANSI_CHARSET,
	DEFAULT_CHARSET,
	SYMBOL_CHARSET,
	SHIFTJIS_CHARSET,
	HANGEUL_CHARSET,
	HANGUL_CHARSET,
	GB2312_CHARSET,
	CHINESEBIG5_CHARSET,
	OEM_CHARSET,
	JOHAB_CHARSET,
	HEBREW_CHARSET,
	ARABIC_CHARSET,
	GREEK_CHARSET,
	TURKISH_CHARSET,
	VIETNAMESE_CHARSET,
	THAI_CHARSET,
	EASTEUROPE_CHARSET,
	RUSSIAN_CHARSET,
	MAC_CHARSET,
	BALTIC_CHARSET
};

TCHAR* CharSetNames[] = {
	_T("ANSI"),
	_T("DEFAULT"),
	_T("SYMBOL"),
	_T("SHIFTJIS"),
	_T("HANGEUL"),
	_T("HANGUL"),
	_T("GB2312"),
	_T("CHINESEBIG5"),
	_T("OEM"),
	_T("JOHAB"),
	_T("HEBREW"),
	_T("ARABIC"),
	_T("GREEK"),
	_T("TURKISH"),
	_T("VIETNAMESE"),
	_T("THAI"),
	_T("EASTEUROPE"),
	_T("RUSSIAN"),
	_T("MAC"),
	_T("BALTIC"),
};

int CharSetLen = countof(CharSetList);

//

static DWORD CharSetToCodePage(DWORD dwCharSet)
{
	CHARSETINFO cs= {0};
	::TranslateCharsetInfo((DWORD *)dwCharSet, &cs, TCI_SRCCHARSET);
	return cs.ciACP;
}

int FindChar(CStringW str, WCHAR c, int pos, bool fUnicode, int CharSet)
{
	if (fUnicode) {
		return(str.Find(c, pos));
	}

	int fStyleMod = 0;

	DWORD cp = CharSetToCodePage(CharSet);
	int OrgCharSet = CharSet;

	for (int i = 0, j = str.GetLength(), k; i < j; i++) {
		WCHAR c2 = str[i];

		if (IsDBCSLeadByteEx(cp, (BYTE)c2)) {
			i++;
		} else if (i >= pos) {
			if (c2 == c) {
				return(i);
			}
		}

		if (c2 == '{') {
			fStyleMod++;
		} else if (fStyleMod > 0) {
			if (c2 == '}') {
				fStyleMod--;
			} else if (c2 == 'e' && i >= 3 && i < j-1 && str.Mid(i-2, 3) == L"\\fe") {
				CharSet = 0;
				for (k = i+1; _istdigit(str[k]); k++) {
					CharSet = CharSet*10 + (str[k] - '0');
				}
				if (k == i+1) {
					CharSet = OrgCharSet;
				}

				cp = CharSetToCodePage(CharSet);
			}
		}
	}

	return(-1);
}

static CStringW ToMBCS(CStringW str, DWORD CharSet)
{
	CStringW ret;

	DWORD cp = CharSetToCodePage(CharSet);

	for (ptrdiff_t i = 0, j = str.GetLength(); i < j; i++) {
		WCHAR wc = str.GetAt(i);
		char c[8];

		int len;
		if ((len = WideCharToMultiByte(cp, 0, &wc, 1, c, 8, NULL, NULL)) > 0) {
			for (ptrdiff_t k = 0; k < len; k++) {
				ret += (WCHAR)(BYTE)c[k];
			}
		} else {
			ret += '?';
		}
	}

	return(ret);
}

static CStringW UnicodeSSAToMBCS(CStringW str, DWORD CharSet)
{
	CStringW ret;

	int OrgCharSet = CharSet;

	for (ptrdiff_t j = 0; j < str.GetLength(); ) {
		j = str.Find('{', j);
		if (j >= 0) {
			ret += ToMBCS(str.Left(j), CharSet);
			str = str.Mid(j);

			j = str.Find('}');
			if (j < 0) {
				ret += ToMBCS(str, CharSet);
				break;
			} else {
				int k = str.Find(L"\\fe");
				if (k >= 0 && k < j) {
					CharSet = 0;
					int l = k+3;
					for (; _istdigit(str[l]); l++) {
						CharSet = CharSet*10 + (str[l] - '0');
					}
					if (l == k+3) {
						CharSet = OrgCharSet;
					}
				}

				j++;

				ret += ToMBCS(str.Left(j), OrgCharSet);
				str = str.Mid(j);
				j = 0;
			}
		} else {
			ret += ToMBCS(str, CharSet);
			break;
		}
	}

	return(ret);
}

static CStringW ToUnicode(CStringW str, DWORD CharSet)
{
	CStringW ret;

	DWORD cp = CharSetToCodePage(CharSet);

	for (ptrdiff_t i = 0, j = str.GetLength(); i < j; i++) {
		WCHAR wc = str.GetAt(i);
		char c = wc&0xff;

		if (IsDBCSLeadByteEx(cp, (BYTE)wc)) {
			i++;

			if (i < j) {
				char cc[2];
				cc[0] = c;
				cc[1] = (char)str.GetAt(i);

				MultiByteToWideChar(cp, 0, cc, 2, &wc, 1);
			}
		} else {
			MultiByteToWideChar(cp, 0, &c, 1, &wc, 1);
		}

		ret += wc;
	}

	return(ret);
}

static CStringW MBCSSSAToUnicode(CStringW str, int CharSet)
{
	CStringW ret;

	int OrgCharSet = CharSet;

	for (ptrdiff_t j = 0; j < str.GetLength(); ) {
		j = FindChar(str, '{', 0, false, CharSet);

		if (j >= 0) {
			ret += ToUnicode(str.Left(j), CharSet);
			str = str.Mid(j);

			j = FindChar(str, '}', 0, false, CharSet);

			if (j < 0) {
				ret += ToUnicode(str, CharSet);
				break;
			} else {
				int k = str.Find(L"\\fe");
				if (k >= 0 && k < j) {
					CharSet = 0;
					int l = k+3;
					for (; _istdigit(str[l]); l++) {
						CharSet = CharSet*10 + (str[l] - '0');
					}
					if (l == k+3) {
						CharSet = OrgCharSet;
					}
				}

				j++;

				ret += ToUnicode(str.Left(j), OrgCharSet);
				str = str.Mid(j);
				j = 0;
			}
		} else {
			ret += ToUnicode(str, CharSet);
			break;
		}
	}

	return(ret);
}

CStringW RemoveSSATags(CStringW str, bool fUnicode, int CharSet)
{
	str.Replace (L"{\\i1}", L"<i>");
	str.Replace (L"{\\i}", L"</i>");

	for (ptrdiff_t i = 0, j; i < str.GetLength(); ) {
		if ((i = FindChar(str, '{', i, fUnicode, CharSet)) < 0) {
			break;
		}
		if ((j = FindChar(str, '}', i, fUnicode, CharSet)) < 0) {
			break;
		}
		str.Delete(i, j-i+1);
	}

	str.Replace(L"\\N", L"\n");
	str.Replace(L"\\n", L"\n");
	str.Replace(L"\\h", L" ");

	return(str);
}

//
CStringW GetStr(CStringW& buff, char sep = ',') //throw(...)
{
	buff.TrimLeft();

	int pos = buff.Find(sep);
	if (pos < 0) {
		pos = buff.GetLength();
		if (pos < 1) {
			throw 1;
		}
	}

	CStringW ret = buff.Left(pos);
	if (pos < buff.GetLength()) {
		buff = buff.Mid(pos+1);
	}

	return(ret);
}

int GetInt(CStringW& buff, char sep = ',') //throw(...)
{
	CStringW str;

	str = GetStr(buff, sep);
	str.MakeLower();

	CStringW fmtstr = str.GetLength() > 2 && (str.Left(2) == L"&h" || str.Left(2) == L"0x")
					  ? str = str.Mid(2), L"%x"
							  : L"%d";

	int ret;
	if (swscanf(str, fmtstr, &ret) != 1) {
		throw 1;
	}

	return(ret);
}

double GetFloat(CStringW& buff, char sep = ',') //throw(...)
{
	CStringW str;

	str = GetStr(buff, sep);
	str.MakeLower();

	double ret;
	if (swscanf(str, L"%lf", &ret) != 1) {
		throw 1;
	}

	return ret;
}

static bool LoadFont(CString& font)
{
	int len = font.GetLength();

	CAutoVectorPtr<BYTE> pData;
	if (len == 0 || (len&3) == 1 || !pData.Allocate(len)) {
		return false;
	}

	const TCHAR* s = font;
	const TCHAR* e = s + len;
	for (BYTE* p = pData; s < e; s++, p++) {
		*p = *s - 33;
	}

	for (ptrdiff_t i = 0, j = 0, k = len&~3; i < k; i+=4, j+=3) {
		pData[j+0] = ((pData[i+0]&63)<<2)|((pData[i+1]>>4)& 3);
		pData[j+1] = ((pData[i+1]&15)<<4)|((pData[i+2]>>2)&15);
		pData[j+2] = ((pData[i+2]& 3)<<6)|((pData[i+3]>>0)&63);
	}

	int datalen = (len&~3)*3/4;

	if ((len&3) == 2) {
		pData[datalen++] = ((pData[(len&~3)+0]&63)<<2)|((pData[(len&~3)+1]>>4)&3);
	} else if ((len&3) == 3) {
		pData[datalen++] = ((pData[(len&~3)+0]&63)<<2)|((pData[(len&~3)+1]>>4)& 3);
		pData[datalen++] = ((pData[(len&~3)+1]&15)<<4)|((pData[(len&~3)+2]>>2)&15);
	}

	HANDLE hFont = INVALID_HANDLE_VALUE;

	if (HMODULE hModule = LoadLibrary(_T("GDI32.DLL"))) {
		typedef HANDLE (WINAPI *PAddFontMemResourceEx)( IN PVOID, IN DWORD, IN PVOID , IN DWORD*);
		if (PAddFontMemResourceEx f = (PAddFontMemResourceEx)GetProcAddress(hModule, "AddFontMemResourceEx")) {
			DWORD cFonts;
			hFont = f(pData, datalen, NULL, &cFonts);
		}

		FreeLibrary(hModule);
	}

	if (hFont == INVALID_HANDLE_VALUE) {
		TCHAR path[_MAX_PATH];
		GetTempPath(_MAX_PATH, path);

		DWORD chksum = 0;
		for (ptrdiff_t i = 0, j = datalen>>2; i < j; i++) {
			chksum += ((DWORD*)(BYTE*)pData)[i];
		}

		CString fn;
		fn.Format(_T("%sfont%08x.ttf"), path, chksum);

		CFileStatus	fs;
		if (!CFileGetStatus(fn, fs)) {
			CFile f;
			if (f.Open(fn, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary|CFile::shareDenyNone)) {
				f.Write(pData, datalen);
				f.Close();
			}
		}

		AddFontResource(fn);
	}

	return true;
}

static bool LoadUUEFont(CTextFile* file)
{
	CString s, font;
	while (file->ReadString(s)) {
		s.Trim();
		if (s.IsEmpty()) {
			break;
		}
		if (s[0] == '[') { // check for some standatr blocks
			if (s.Find(_T("[Script Info]")) == 0) {
				break;
			}
			if (s.Find(_T("[V4+ Styles]")) == 0) {
				break;
			}
			if (s.Find(_T("[V4 Styles]")) == 0) {
				break;
			}
			if (s.Find(_T("[Events]")) == 0) {
				break;
			}
			if (s.Find(_T("[Fonts]")) == 0) {
				break;
			}
			if (s.Find(_T("[Graphics]")) == 0) {
				break;
			}
		}
		if (s.Find(_T("fontname:")) == 0) {
			LoadFont(font);
			font.Empty();
			continue;
		}

		font += s;
	}

	if (!font.IsEmpty()) {
		LoadFont(font);
	}

	return true;
}

#ifdef _VSMOD
bool CSimpleTextSubtitle::LoadEfile(CString& img, CString m_fn)
{
	int len = img.GetLength();

	CAutoVectorPtr<BYTE> pData;
	if (len == 0 || (len&3) == 1 || !pData.Allocate(len)) {
		return false;
	}

	const TCHAR* s = img;
	const TCHAR* e = s + len;
	for (BYTE* p = pData; s < e; s++, p++) {
		*p = *s - 33;
	}

	for (ptrdiff_t i = 0, j = 0, k = len&~3; i < k; i+=4, j+=3) {
		pData[j+0] = ((pData[i+0]&63)<<2)|((pData[i+1]>>4)& 3);
		pData[j+1] = ((pData[i+1]&15)<<4)|((pData[i+2]>>2)&15);
		pData[j+2] = ((pData[i+2]& 3)<<6)|((pData[i+3]>>0)&63);
	}

	int datalen = (len&~3)*3/4;

	if ((len&3) == 2) {
		pData[datalen++] = ((pData[(len&~3)+0]&63)<<2)|((pData[(len&~3)+1]>>4)&3);
	} else if ((len&3) == 3) {
		pData[datalen++] = ((pData[(len&~3)+0]&63)<<2)|((pData[(len&~3)+1]>>4)& 3);
		pData[datalen++] = ((pData[(len&~3)+1]&15)<<4)|((pData[(len&~3)+2]>>2)&15);
	}

	// load png image
	MOD_PNGIMAGE t_temp;
	if (t_temp.initImage(pData.m_p,m_fn)) { // save path
		mod_images.Add(t_temp);
	}
	return true;
}


bool CSimpleTextSubtitle::LoadUUEFile(CTextFile* file, CString m_fn)
{
	CString s, img;
	while (file->ReadString(s)) {
		s.Trim();
		if (s.IsEmpty()) {
			break;
		}
		if (s[0] == '[') { // check for some standatr blocks
			if (s.Find(_T("[Script Info]")) == 0) {
				break;
			}
			if (s.Find(_T("[V4+ Styles]")) == 0) {
				break;
			}
			if (s.Find(_T("[V4 Styles]")) == 0) {
				break;
			}
			if (s.Find(_T("[Events]")) == 0) {
				break;
			}
			if (s.Find(_T("[Fonts]")) == 0) {
				break;
			}
			if (s.Find(_T("[Graphics]")) == 0) {
				break;
			}
		}
		// next file
		if (s.Find(_T("filename:")) == 0) {
			LoadEfile(img, m_fn);
			m_fn = s.Mid(10);
			img.Empty();
			continue;
		}

		img += s;
	}

	if (!img.IsEmpty()) {
		LoadEfile(img, m_fn);
	}

	return true;
}
#endif

static bool OpenSubStationAlpha(CTextFile* file, CSimpleTextSubtitle& ret, int CharSet)
{
	bool fRet = false;

	int version = 3, sver = 3;

	CStringW buff;
	while (file->ReadString(buff)) {
		buff.Trim();
		if (buff.IsEmpty() || buff.GetAt(0) == ';') {
			continue;
		}

		CStringW entry;

		//		try {
		entry = GetStr(buff, ':');
		//	}
		//		catch(...) {continue;}

		entry.MakeLower();

		if (entry == L"[script info]") {
			fRet = true;
		} else if (entry == L"playresx") {
			try {
				ret.m_dstScreenSize.cx = GetInt(buff);
			} catch (...) {
				ret.m_dstScreenSize = CSize(0, 0);
				return false;
			}

			if (ret.m_dstScreenSize.cy <= 0) {
				ret.m_dstScreenSize.cy = (ret.m_dstScreenSize.cx == 1280)
										 ? 1024
										 : ret.m_dstScreenSize.cx * 3 / 4;
			}
		} else if (entry == L"playresy") {
			try {
				ret.m_dstScreenSize.cy = GetInt(buff);
			} catch (...) {
				ret.m_dstScreenSize = CSize(0, 0);
				return false;
			}

			if (ret.m_dstScreenSize.cx <= 0) {
				ret.m_dstScreenSize.cx = (ret.m_dstScreenSize.cy == 1024)
										 ? 1280
										 : ret.m_dstScreenSize.cy * 4 / 3;
			}
		} else if (entry == L"wrapstyle") {
			try {
				ret.m_defaultWrapStyle = GetInt(buff);
			} catch (...) {
				ret.m_defaultWrapStyle = 1;
				return false;
			}
		} else if (entry == L"scripttype") {
			if (buff.GetLength() >= 4 && !buff.Right(4).CompareNoCase(L"4.00")) {
				version = sver = 4;
			} else if (buff.GetLength() >= 5 && !buff.Right(5).CompareNoCase(L"4.00+")) {
				version = sver = 5;
			} else if (buff.GetLength() >= 6 && !buff.Right(6).CompareNoCase(L"4.00++")) {
				version = sver = 6;
			}
		} else if (entry == L"collisions") {
			buff = GetStr(buff);
			buff.MakeLower();
			ret.m_collisions = buff.Find(L"reverse") >= 0 ? 1 : 0;
		} else if (entry == L"scaledborderandshadow") {
			buff = GetStr(buff);
			buff.MakeLower();
			ret.m_fScaledBAS = buff.Find(L"yes") >= 0;
		} else if (entry == L"[v4 styles]") {
			fRet = true;
			sver = 4;
		} else if (entry == L"[v4+ styles]") {
			fRet = true;
			sver = 5;
		} else if (entry == L"[v4++ styles]") {
			fRet = true;
			sver = 6;
		} else if (entry == L"style") {
			STSStyle* style = DNew STSStyle;
			if (!style) {
				return false;
			}

			try {
				CString StyleName;
				int alpha = 0;

				StyleName = WToT(GetStr(buff));
				style->fontName = WToT(GetStr(buff));
				style->fontSize = GetFloat(buff);
				for (ptrdiff_t i = 0; i < 4; i++) {
					style->colors[i] = (COLORREF)GetInt(buff);
				}
				style->fontWeight = GetInt(buff) ? FW_BOLD : FW_NORMAL;
				style->fItalic = GetInt(buff);
				if (sver >= 5)	{
					style->fUnderline = GetInt(buff);
				}
				if (sver >= 5)	{
					style->fStrikeOut = GetInt(buff);
				}
				if (sver >= 5)	{
					style->fontScaleX = GetFloat(buff);
				}
				if (sver >= 5)	{
					style->fontScaleY = GetFloat(buff);
				}
				if (sver >= 5)	{
					style->fontSpacing = GetFloat(buff);
				}
				if (sver >= 5)	{
					style->fontAngleZ = GetFloat(buff);
				}
				if (sver >= 4)	{
					style->borderStyle = GetInt(buff);
				}
				style->outlineWidthX = style->outlineWidthY = GetFloat(buff);
				style->shadowDepthX = style->shadowDepthY = GetFloat(buff);
				style->scrAlignment = GetInt(buff);
				style->marginRect.left = GetInt(buff);
				style->marginRect.right = GetInt(buff);
				style->marginRect.top = style->marginRect.bottom = GetInt(buff);
				if (sver >= 6)	{
					style->marginRect.bottom = GetInt(buff);
				}
				if (sver <= 4)	{
					alpha = GetInt(buff);
				}
				style->charSet = GetInt(buff);
				if (sver >= 6)	{
					style->relativeTo = GetInt(buff);
				}

				if (sver <= 4)	{
					style->colors[2] = style->colors[3];    // style->colors[2] is used for drawing the outline
				}
				if (sver <= 4)	{
					alpha = max(min(alpha, 0xff), 0);
				}
				if (sver <= 4) {
					for (ptrdiff_t i = 0; i < 3; i++) {
						style->alpha[i] = alpha;
					}
					style->alpha[3] = 0x80;
				}
				if (sver >= 5)	for (ptrdiff_t i = 0; i < 4; i++) {
						style->alpha[i] = (BYTE)(style->colors[i] >> 24);
						style->colors[i] &= 0xffffff;
					}
				if (sver >= 5)	{
					style->fontScaleX = max(style->fontScaleX, 0);
				}
				if (sver >= 5)	{
					style->fontScaleY = max(style->fontScaleY, 0);
				}
#ifndef _VSMOD // patch f002. negative fontspacing at style
				if (sver >= 5)	{
					style->fontSpacing = max(style->fontSpacing, 0);
				}
#endif
				style->fontAngleX = style->fontAngleY = 0;
				style->borderStyle = style->borderStyle == 1 ? 0 : style->borderStyle == 3 ? 1 : 0;
				style->outlineWidthX = max(style->outlineWidthX, 0);
				style->outlineWidthY = max(style->outlineWidthY, 0);
				style->shadowDepthX = max(style->shadowDepthX, 0);
				style->shadowDepthY = max(style->shadowDepthY, 0);
				if (sver <= 4)	style->scrAlignment = (style->scrAlignment & 4) ? ((style->scrAlignment & 3) + 6) // top
														  : (style->scrAlignment & 8) ? ((style->scrAlignment & 3) + 3) // mid
														  : (style->scrAlignment & 3); // bottom

				StyleName.TrimLeft('*');

				ret.AddStyle(StyleName, style);
			} catch (...) {
				delete style;
				return false;
			}
		} else if (entry == L"[events]") {
			fRet = true;
		} else if (entry == _T("dialogue")) {
			try {
				int hh1, mm1, ss1, ms1_div10, hh2, mm2, ss2, ms2_div10, layer = 0;
				CString Style, Actor, Effect;
				CRect marginRect;

				if (version <= 4) {
					GetStr(buff, '=');		/* Marked = */
					GetInt(buff);
				}
				if (version >= 5) {
					layer = GetInt(buff);
				}
				hh1 = GetInt(buff, ':');
				mm1 = GetInt(buff, ':');
				ss1 = GetInt(buff, '.');
				ms1_div10 = GetInt(buff);
				hh2 = GetInt(buff, ':');
				mm2 = GetInt(buff, ':');
				ss2 = GetInt(buff, '.');
				ms2_div10 = GetInt(buff);
				Style = WToT(GetStr(buff));
				Actor = WToT(GetStr(buff));
				marginRect.left = GetInt(buff);
				marginRect.right = GetInt(buff);
				marginRect.top = marginRect.bottom = GetInt(buff);
				if (version >= 6) {
					marginRect.bottom = GetInt(buff);
				}
				Effect = WToT(GetStr(buff));

				int len = min(Effect.GetLength(), buff.GetLength());
				if (Effect.Left(len) == WToT(buff.Left(len))) {
					Effect.Empty();
				}

				Style.TrimLeft('*');
				if (!Style.CompareNoCase(_T("Default"))) {
					Style = _T("Default");
				}

				ret.Add(buff,
						file->IsUnicode(),
						(((hh1*60 + mm1)*60) + ss1)*1000 + ms1_div10*10,
						(((hh2*60 + mm2)*60) + ss2)*1000 + ms2_div10*10,
						Style, Actor, Effect,
						marginRect,
						layer);
			} catch (...) {
				return false;
			}
		} else if (entry == L"fontname") {
			LoadUUEFont(file);
		}
#ifdef _VSMOD // load png graphic from text resources
		else if (entry == L"filename") {
			ret.LoadUUEFile(file,GetStr(buff));
		}
#endif
	}

	return(fRet);
}
//

CSimpleTextSubtitle::CSimpleTextSubtitle()
{
	m_mode = TIME;
	m_dstScreenSize = CSize(0, 0);
	m_defaultWrapStyle = 0;
	m_collisions = 0;
	m_fScaledBAS = false;
	m_encoding = CTextFile::ASCII;
	m_lcid = 0;
	m_ePARCompensationType = EPCTDisabled;
	m_dPARCompensation = 1.0;

#ifdef _VSMOD // indexing
	ind_size = 0;
#endif
}

CSimpleTextSubtitle::~CSimpleTextSubtitle()
{
	Empty();
}
/*
CSimpleTextSubtitle::CSimpleTextSubtitle(CSimpleTextSubtitle& sts)
{
	*this = sts;
}

CSimpleTextSubtitle& CSimpleTextSubtitle::operator = (CSimpleTextSubtitle& sts)
{
	Copy(sts);

	return(*this);
}
*/

void CSimpleTextSubtitle::Copy(CSimpleTextSubtitle& sts)
{
	if (this != &sts) {
		Empty();

		m_mode = sts.m_mode;
		m_path = sts.m_path;
		m_dstScreenSize = sts.m_dstScreenSize;
		m_defaultWrapStyle = sts.m_defaultWrapStyle;
		m_collisions = sts.m_collisions;
		m_fScaledBAS = sts.m_fScaledBAS;
		m_encoding = sts.m_encoding;
		m_fUsingAutoGeneratedDefaultStyle = sts.m_fUsingAutoGeneratedDefaultStyle;
		CopyStyles(sts.m_styles);
		m_segments.Copy(sts.m_segments);
		__super::Copy(sts);
	}
}

void CSimpleTextSubtitle::Append(CSimpleTextSubtitle& sts, int timeoff)
{
	if (timeoff < 0) {
		timeoff = GetCount() > 0 ? GetAt(GetCount()-1).end : 0;
	}

	for (ptrdiff_t i = 0, j = GetCount(); i < j; i++) {
		if (GetAt(i).start > timeoff) {
			RemoveAt(i, j - i);
			break;
		}
	}

	CopyStyles(sts.m_styles, true);

	for (ptrdiff_t i = 0, j = sts.GetCount(); i < j; i++) {
		STSEntry stse = sts.GetAt(i);
		stse.start += timeoff;
		stse.end += timeoff;
		stse.readorder += GetCount();
		__super::Add(stse);
	}

	CreateSegments();
}

void CSTSStyleMap::Free()
{
	POSITION pos = GetStartPosition();
	while (pos) {
		CString key;
		STSStyle* val;
		GetNextAssoc(pos, key, val);
		delete val;
	}

	RemoveAll();
}

bool CSimpleTextSubtitle::CopyStyles(const CSTSStyleMap& styles, bool fAppend)
{
	if (!fAppend) {
		m_styles.Free();
	}

	POSITION pos = styles.GetStartPosition();
	while (pos) {
		CString key;
		STSStyle* val;
		styles.GetNextAssoc(pos, key, val);

		STSStyle* s = DNew STSStyle;
		if (!s) {
			return false;
		}

		*s = *val;

		AddStyle(key, s);
	}

	return true;
}

void CSimpleTextSubtitle::Empty()
{
	m_dstScreenSize = CSize(0, 0);
	m_styles.Free();
	m_segments.RemoveAll();
	RemoveAll();

#ifdef _VSMOD // indexing
	if (ind_size>0) {
		delete ind_time;
		delete ind_pos;
	}
#endif
}

void CSimpleTextSubtitle::Add(CStringW str, bool fUnicode, int start, int end, CString style, CString actor, CString effect, CRect marginRect, int layer, int readorder)
{
	if (str.Trim().IsEmpty() || start > end) {
		return;
	}

	str.Remove('\r');
	str.Replace(L"\n", L"\\N");
	if (style.IsEmpty()) {
		style = _T("Default");
	}
	style.TrimLeft('*');

	STSEntry sub;
	sub.str = str;
	sub.fUnicode = fUnicode;
	sub.style = style;
	sub.actor = actor;
	sub.effect = effect;
	sub.marginRect = marginRect;
	sub.layer = layer;
	sub.start = start;
	sub.end = end;
	sub.readorder = readorder < 0 ? GetCount() : readorder;

	int n = __super::Add(sub);

#ifndef _VSMOD
	int len = m_segments.GetCount();

	if (len == 0) {
		STSSegment stss(start, end);
		stss.subs.Add(n);
		m_segments.Add(stss);
	} else if (end <= m_segments[0].start) {
		STSSegment stss(start, end);
		stss.subs.Add(n);
		m_segments.InsertAt(0, stss);
	} else if (start >= m_segments[len-1].end) {
		STSSegment stss(start, end);
		stss.subs.Add(n);
		m_segments.Add(stss);
	} else {
		if (start < m_segments[0].start) {
			STSSegment stss(start, m_segments[0].start);
			stss.subs.Add(n);
			start = m_segments[0].start;
			m_segments.InsertAt(0, stss);
		}

		for (size_t i = 0; i < m_segments.GetCount(); i++) {
			STSSegment& s = m_segments[i];

			if (start >= s.end) {
				continue;
			}

			if (end <= s.start) {
				break;
			}

			if (s.start < start && start < s.end) {
				STSSegment stss(s.start, start);
				stss.subs.Copy(s.subs);
				s.start = start;
				m_segments.InsertAt(i, stss);
				continue;
			}

			if (start <= s.start && s.end <= end) {
				for (ptrdiff_t j = 0, k = s.subs.GetCount(); j <= k; j++) {
					if (j == k || sub.readorder < GetAt(s.subs[j]).readorder) {
						s.subs.InsertAt(j, n);
					}
				}
				//				s.subs.Add(n);
			}

			if (s.start < end && end < s.end) {
				STSSegment stss(s.start, end);
				stss.subs.Copy(s.subs);
				for (ptrdiff_t j = 0, k = s.subs.GetCount(); j <= k; j++) {
					if (j == k || sub.readorder < GetAt(stss.subs[j]).readorder) {
						stss.subs.InsertAt(j, n);
					}
				}
				//				stss.subs.Add(n);
				s.start = end;
				m_segments.InsertAt(i, stss);
			}
		}

		if (end > m_segments[m_segments.GetCount()-1].end) {
			STSSegment stss(m_segments[m_segments.GetCount()-1].end, end);
			stss.subs.Add(n);
			m_segments.Add(stss);
		}
	}
#endif
}


#ifdef _VSMOD
void CSimpleTextSubtitle::MakeIndex(int SizeOfSegment)
{
	int cnt = m_segments.GetCount();
	if (SizeOfSegment==0) { // autosize
		// 100000 lines == 1300 segments
		// TODO: make gooood =D
		if (cnt<100) {
			SizeOfSegment = (cnt==0) ? 1 : cnt;
		} else if (cnt<1000) {
			SizeOfSegment = cnt / 50;
		} else {
			SizeOfSegment = cnt / 100;
		}
	}

	ind_size = cnt / SizeOfSegment;

	ind_time = new DWORD[ind_size];
	ind_pos = new DWORD[ind_size];

	for (int i = 0; i<ind_size; i++) {
		int pos = i * SizeOfSegment;
		ind_time[i] = m_segments[pos].start;
		ind_pos[i] = pos;
	}
}
#endif

STSStyle* CSimpleTextSubtitle::CreateDefaultStyle(int CharSet)
{
	CString def(_T("Default"));

	STSStyle* ret = NULL;

	if (!m_styles.Lookup(def, ret)) {
		STSStyle* style = DNew STSStyle();
		style->charSet = CharSet;
		AddStyle(def, style);
		m_styles.Lookup(def, ret);

		m_fUsingAutoGeneratedDefaultStyle = true;
	} else {
		m_fUsingAutoGeneratedDefaultStyle = false;
	}

	return ret;
}

void CSimpleTextSubtitle::ChangeUnknownStylesToDefault()
{
	CAtlMap<CString, STSStyle*, CStringElementTraits<CString> > unknown;
	bool fReport = true;

	for (size_t i = 0; i < GetCount(); i++) {
		STSEntry& stse = GetAt(i);

		STSStyle* val;
		if (!m_styles.Lookup(stse.style, val)) {
			if (!unknown.Lookup(stse.style, val)) {
				unknown[stse.style] = NULL;
			}

			stse.style = _T("Default");
		}
	}
}

void CSimpleTextSubtitle::AddStyle(CString name, STSStyle* style)
{
	if (name.IsEmpty()) {
		name = _T("Default");
	}

	STSStyle* val;
	if (m_styles.Lookup(name, val)) {
		if (*val == *style) {
			delete style;
			return;
		}

		int i, j;
		int len = name.GetLength();

		for (i = len; i > 0 && _istdigit(name[i-1]); i--) {
			;
		}

		int idx = 1;

		CString name2 = name;

		if (i < len && _stscanf(name.Right(len-i), _T("%d"), &idx) == 1) {
			name2 = name.Left(i);
		}

		idx++;

		CString name3;
		do {
			name3.Format(_T("%s%d"), name2, idx);
			idx++;
		} while (m_styles.Lookup(name3));

		m_styles.RemoveKey(name);
		m_styles[name3] = val;

		for (i = 0, j = GetCount(); i < j; i++) {
			STSEntry& stse = GetAt(i);
			if (stse.style == name) {
				stse.style = name3;
			}
		}
	}

	m_styles[name] = style;
}

bool CSimpleTextSubtitle::SetDefaultStyle(STSStyle& s)
{
	STSStyle* val;
	if (!m_styles.Lookup(_T("Default"), val)) {
		return false;
	}
	*val = s;
	m_fUsingAutoGeneratedDefaultStyle = false;
	return true;
}

bool CSimpleTextSubtitle::GetDefaultStyle(STSStyle& s)
{
	STSStyle* val;
	if (!m_styles.Lookup(_T("Default"), val)) {
		return false;
	}
	s = *val;
	return true;
}

void CSimpleTextSubtitle::ConvertToTimeBased(double fps)
{
	if (m_mode == TIME) {
		return;
	}

	for (ptrdiff_t i = 0, j = GetCount(); i < j; i++) {
		STSEntry& stse = (*this)[i];
		stse.start = 1.0 * stse.start * 1000 / fps + 0.5;
		stse.end = 1.0 * stse.end * 1000 / fps + 0.5;
	}

	m_mode = TIME;

	CreateSegments();
}

void CSimpleTextSubtitle::ConvertToFrameBased(double fps)
{
	if (m_mode == FRAME) {
		return;
	}

	for (ptrdiff_t i = 0, j = GetCount(); i < j; i++) {
		STSEntry& stse = (*this)[i];
		stse.start = 1.0 * stse.start * fps / 1000 + 0.5;
		stse.end = 1.0 * stse.end * fps / 1000 + 0.5;
	}

	m_mode = FRAME;

	CreateSegments();
}

int CSimpleTextSubtitle::SearchSub(int t, double fps)
{
	int i = 0, j = GetCount() - 1, ret = -1;

	if (j >= 0 && t >= TranslateStart(j, fps)) {
		return(j);
	}

	while (i < j) {
		int mid = (i + j) >> 1;

		int midt = TranslateStart(mid, fps);

		if (t == midt) {
			while (mid > 0 && t == TranslateStart(mid-1, fps)) {
				--mid;
			}
			ret = mid;
			break;
		} else if (t < midt) {
			ret = -1;
			if (j == mid) {
				mid--;
			}
			j = mid;
		} else if (t > midt) {
			ret = mid;
			if (i == mid) {
				++mid;
			}
			i = mid;
		}
	}

	return ret;
}

const STSSegment* CSimpleTextSubtitle::SearchSubs(int t, double fps, /*[out]*/ int* iSegment, int* nSegments)
{
	int i = 0, j = m_segments.GetCount() - 1, ret = -1;

	if (nSegments) {
		*nSegments = j+1;
	}

	// last segment
	if (j >= 0 && t >= TranslateSegmentStart(j, fps) && t < TranslateSegmentEnd(j, fps)) {
		if (iSegment) {
			*iSegment = j;
		}
		return(&m_segments[j]);
	}

	// after last segment
	if (j >= 0 && t >= TranslateSegmentEnd(j, fps)) {
		if (iSegment) {
			*iSegment = j+1;
		}
		return(NULL);
	}

	// before first segment
	if (j > 0 && t < TranslateSegmentStart(i, fps)) {
		if (iSegment) {
			*iSegment = -1;
		}
		return(NULL);
	}

#ifdef _VSMOD
	// find bounds
	// is this nya?
	for (size_t k = 0; k < ind_size; ++k) {
		if (ind_time[k]>t) {
			if (k==0) {
				break;
			}
			i = ind_pos[k-1];
			j = ind_pos[k];
			break;
		}
	}
#endif

	while (i < j) {
		int mid = (i + j) >> 1;

		int midt = TranslateSegmentStart(mid, fps);

		if (t == midt) {
			ret = mid;
			break;
		} else if (t < midt) {
			ret = -1;
			if (j == mid) {
				mid--;
			}
			j = mid;
		} else if (t > midt) {
			ret = mid;
			if (i == mid) {
				mid++;
			}
			i = mid;
		}
	}

	if (0 <= ret && (size_t)ret < m_segments.GetCount()) {
		if (iSegment) {
			*iSegment = ret;
		}
	}

	if (0 <= ret && (size_t)ret < m_segments.GetCount()
			&& m_segments[ret].subs.GetCount() > 0
			&& TranslateSegmentStart(ret, fps) <= t && t < TranslateSegmentEnd(ret, fps)) {
		return(&m_segments[ret]);
	}

	return(NULL);
}

int CSimpleTextSubtitle::TranslateStart(int i, double fps)
{
	return(i < 0 || GetCount() <= (size_t)i ? -1 :
		   m_mode == TIME ? GetAt(i).start :
		   m_mode == FRAME ? (int)(GetAt(i).start*1000/fps) :
		   0);
}

int CSimpleTextSubtitle::TranslateEnd(int i, double fps)
{
	return(i < 0 || GetCount() <= (size_t)i ? -1 :
		   m_mode == TIME ? GetAt(i).end :
		   m_mode == FRAME ? (int)(GetAt(i).end*1000/fps) :
		   0);
}

int CSimpleTextSubtitle::TranslateSegmentStart(int i, double fps)
{
	return(i < 0 || m_segments.GetCount() <= (size_t)i ? -1 :
		   m_mode == TIME ? m_segments[i].start :
		   m_mode == FRAME ? (int)(m_segments[i].start*1000/fps) :
		   0);
}

int CSimpleTextSubtitle::TranslateSegmentEnd(int i, double fps)
{
	return(i < 0 || m_segments.GetCount() <= (size_t)i ? -1 :
		   m_mode == TIME ? m_segments[i].end :
		   m_mode == FRAME ? (int)(m_segments[i].end*1000/fps) :
		   0);
}

STSStyle* CSimpleTextSubtitle::GetStyle(int i)
{
	CString def = _T("Default");

	STSStyle* style = NULL;
	m_styles.Lookup(GetAt(i).style, style);

	STSStyle* defstyle = NULL;
	m_styles.Lookup(def, defstyle);

	if (!style) {
		style = defstyle;
	}

	ASSERT(style);

	return style;
}

bool CSimpleTextSubtitle::GetStyle(int i, STSStyle& stss)
{
	CString def = _T("Default");

	STSStyle* style = NULL;
	m_styles.Lookup(GetAt(i).style, style);

	STSStyle* defstyle = NULL;
	m_styles.Lookup(def, defstyle);

	if (!style) {
		if (!defstyle) {
			defstyle = CreateDefaultStyle(DEFAULT_CHARSET);
		}

		style = defstyle;
	}

	if (!style) {
		ASSERT(0);
		return false;
	}

	stss = *style;
	if (stss.relativeTo == 2 && defstyle) {
		stss.relativeTo = defstyle->relativeTo;
	}

	return true;
}

int CSimpleTextSubtitle::GetCharSet(int i)
{
	STSStyle stss;
	GetStyle(i, stss);
	return(stss.charSet);
}

bool CSimpleTextSubtitle::IsEntryUnicode(int i)
{
	return(GetAt(i).fUnicode);
}

void CSimpleTextSubtitle::ConvertUnicode(int i, bool fUnicode)
{
	STSEntry& stse = GetAt(i);

	if (stse.fUnicode ^ fUnicode) {
		int CharSet = GetCharSet(i);

		stse.str = fUnicode
				   ? MBCSSSAToUnicode(stse.str, CharSet)
				   : UnicodeSSAToMBCS(stse.str, CharSet);

		stse.fUnicode = fUnicode;
	}
}

CStringA CSimpleTextSubtitle::GetStrA(int i, bool fSSA)
{
	return(WToA(GetStrWA(i, fSSA)));
}

CStringW CSimpleTextSubtitle::GetStrW(int i, bool fSSA)
{
	bool fUnicode = IsEntryUnicode(i);
	int CharSet = GetCharSet(i);

	CStringW str = GetAt(i).str;

	if (!fUnicode) {
		str = MBCSSSAToUnicode(str, CharSet);
	}

	if (!fSSA) {
		str = RemoveSSATags(str, fUnicode, CharSet);
	}

	return(str);
}

CStringW CSimpleTextSubtitle::GetStrWA(int i, bool fSSA)
{
	bool fUnicode = IsEntryUnicode(i);
	int CharSet = GetCharSet(i);

	CStringW str = GetAt(i).str;

	if (fUnicode) {
		str = UnicodeSSAToMBCS(str, CharSet);
	}

	if (!fSSA) {
		str = RemoveSSATags(str, fUnicode, CharSet);
	}

	return(str);
}

void CSimpleTextSubtitle::SetStr(int i, CStringA str, bool fUnicode)
{
	SetStr(i, AToW(str), false);
}

void CSimpleTextSubtitle::SetStr(int i, CStringW str, bool fUnicode)
{
	STSEntry& stse = GetAt(i);

	str.Replace(L"\n", L"\\N");

	if (stse.fUnicode && !fUnicode) {
		stse.str = MBCSSSAToUnicode(str, GetCharSet(i));
	} else if (!stse.fUnicode && fUnicode) {
		stse.str = UnicodeSSAToMBCS(str, GetCharSet(i));
	} else {
		stse.str = str;
	}
}

static int comp1(const void* a, const void* b)
{
	int ret = ((STSEntry*)a)->start - ((STSEntry*)b)->start;
	if (ret == 0) {
		ret = ((STSEntry*)a)->layer - ((STSEntry*)b)->layer;
	}
	if (ret == 0) {
		ret = ((STSEntry*)a)->readorder - ((STSEntry*)b)->readorder;
	}
	return(ret);
}

static int comp2(const void* a, const void* b)
{
	return(((STSEntry*)a)->readorder - ((STSEntry*)b)->readorder);
}

void CSimpleTextSubtitle::Sort(bool fRestoreReadorder)
{
	qsort(GetData(), GetCount(), sizeof(STSEntry), !fRestoreReadorder ? comp1 : comp2);
	CreateSegments();
}

static int intcomp(const void* i1, const void* i2)
{
	return(*((int*)i1) - *((int*)i2));
}

void CSimpleTextSubtitle::CreateSegments()
{
	m_segments.RemoveAll();

	size_t i, j;

	CAtlArray<int> breakpoints;

	for (i = 0; i < GetCount(); i++) {
		STSEntry& stse = GetAt(i);
		breakpoints.Add(stse.start);
		breakpoints.Add(stse.end);
	}

	qsort(breakpoints.GetData(), breakpoints.GetCount(), sizeof(int), intcomp);

	int* ptr = breakpoints.GetData(), prev = ptr ? *ptr : NULL;

	for (i = breakpoints.GetCount(); i > 0; i--, ptr++) {
		if (*ptr != prev) {
			m_segments.Add(STSSegment(prev, *ptr));
			prev = *ptr;
		}
	}

	for (i = 0; i < GetCount(); i++) {
		STSEntry& stse = GetAt(i);
		for (j = 0; j < m_segments.GetCount() && m_segments[j].start < stse.start; j++) {
			;
		}
		for (; j < m_segments.GetCount() && m_segments[j].end <= stse.end; j++) {
			m_segments[j].subs.Add(i);
		}
	}

	OnChanged();
	/*
		for(i = 0, j = m_segments.GetCount(); i < j; i++)
		{
			STSSegment& stss = m_segments[i];

			TRACE(_T("%d - %d"), stss.start, stss.end);

			for(ptrdiff_t k = 0, l = stss.subs.GetCount(); k < l; k++)
			{
				TRACE(_T(", %d"), stss.subs[k]);
			}

			TRACE(_T("\n"));
		}
	*/
}

bool CSimpleTextSubtitle::Open(CString fn, int CharSet, CString name)
{
	Empty();

	CTextFile f;
	if (!f.Open(fn)) {
		return false;
	}

	return(Open(&f, CharSet, L""));
}

static int CountLines(CTextFile* f, ULONGLONG from, ULONGLONG to)
{
	int n = 0;
	CString s;
	f->Seek(from, 0);
	while (f->ReadString(s) && f->GetPosition() < to) {
		n++;
	}
	return(n);
}

bool CSimpleTextSubtitle::Open(CTextFile* f, int CharSet, CString name)
{
	Empty();

	ULONGLONG pos = f->GetPosition();

	if (!OpenSubStationAlpha(f, *this, CharSet) /*|| !GetCount()*/) {
		f->Seek(pos, 0);
		Empty();
		return false;
	}

	m_mode = TIME;
	m_encoding = f->GetEncoding();
	m_path = f->GetFilePath();

	//		Sort();
	CreateSegments();
#ifdef _VSMOD // indexing
	MakeIndex(0);
#endif

	CreateDefaultStyle(CharSet);

	ChangeUnknownStylesToDefault();

	if (m_dstScreenSize == CSize(0, 0)) {
		m_dstScreenSize = CSize(384, 288);
	}

	return true;
}

bool CSimpleTextSubtitle::Open(BYTE* data, int len, int CharSet, CString name)
{
	TCHAR path[_MAX_PATH];
	if (!GetTempPath(_MAX_PATH, path)) {
		return false;
	}

	TCHAR fn[_MAX_PATH];
	if (!GetTempFileName(path, _T("vs"), 0, fn)) {
		return false;
	}

	FILE* tmp = _tfopen(fn, _T("wb"));
	if (!tmp) {
		return false;
	}

	int i = 0;
	for (; i <= (len-1024); i += 1024) {
		fwrite(&data[i], 1024, 1, tmp);
	}
	if (len > i) {
		fwrite(&data[i], len - i, 1, tmp);
	}

	fclose(tmp);

	bool fRet = Open(fn, CharSet, name);

	_tremove(fn);

	return(fRet);
}

////////////////////////////////////////////////////////////////////

STSStyle::STSStyle()
{
	SetDefault();
}

#ifdef _VSMOD
STSStyle::STSStyle(STSStyle& s)
{
	SetDefault();
	mod_CopyStyleFrom(s);
}
#endif

void STSStyle::SetDefault()
{
	marginRect = CRect(20, 20, 20, 20);
	scrAlignment = 2;
	borderStyle = 0;
	outlineWidthX = outlineWidthY = 2;
	shadowDepthX = shadowDepthY = 3;
	colors[0] = 0x00ffffff;
	colors[1] = 0x0000ffff;
	colors[2] = 0x00000000;
	colors[3] = 0x00000000;
	alpha[0] = 0x00;
	alpha[1] = 0x00;
	alpha[2] = 0x00;
	alpha[3] = 0x80;
	charSet = DEFAULT_CHARSET;
	fontName = _T("Arial");
	fontSize = 18;
	fontScaleX = fontScaleY = 100;
	fontSpacing = 0;
	fontWeight = FW_BOLD;
	fItalic = false;
	fUnderline = false;
	fStrikeOut = false;
	fBlur = 0;
	fGaussianBlur = 0;
	fontShiftX = fontShiftY = fontAngleZ = fontAngleX = fontAngleY = 0;
	relativeTo = 2;
#ifdef _VSMOD
	// patch m001. Vertical fontspacing
	mod_verticalSpace = 0;
	// patch m002. Z-coord
	mod_z = 0;
	// patch m003. random text points
	mod_rand.clear();
	// patch m004. gradient colors
	mod_grad.clear();
	// patch m007. symbol rotating
	mod_fontOrient = 0;
#endif
}

bool STSStyle::operator == (STSStyle& s)
{
	return(marginRect == s.marginRect
		   && scrAlignment == s.scrAlignment
		   && borderStyle == s.borderStyle
		   && outlineWidthX == s.outlineWidthX
		   && outlineWidthY == s.outlineWidthY
		   && shadowDepthX == s.shadowDepthX
		   && shadowDepthY == s.shadowDepthY
		   && *((int*)&colors[0]) == *((int*)&s.colors[0])
		   && *((int*)&colors[1]) == *((int*)&s.colors[1])
		   && *((int*)&colors[2]) == *((int*)&s.colors[2])
		   && *((int*)&colors[3]) == *((int*)&s.colors[3])
		   && alpha[0] == s.alpha[0]
		   && alpha[1] == s.alpha[1]
		   && alpha[2] == s.alpha[2]
		   && alpha[3] == s.alpha[3]
		   && fBlur == s.fBlur
		   && fGaussianBlur == s.fGaussianBlur
		   && relativeTo == s.relativeTo
#ifdef _VSMOD
		   // patch m001. Vertical fontspacing
		   && mod_verticalSpace == s.mod_verticalSpace
		   // patch m002. Z-coord
		   && mod_z == s.mod_z
		   // patch m003. random text points
		   && mod_rand == s.mod_rand
		   // patch m004. gradient colors
		   && mod_grad == s.mod_grad
		   // patch m007. symbol rotating
		   && mod_fontOrient == s.mod_fontOrient
		   // patch m008. distort
		   && mod_distort == s.mod_distort
		   // patch m011. jitter
		   && mod_jitter == s.mod_jitter
#endif
		   && IsFontStyleEqual(s));
}

bool STSStyle::IsFontStyleEqual(STSStyle& s)
{
	return(
			  charSet == s.charSet
			  && fontName == s.fontName
			  && fontSize == s.fontSize
			  && fontScaleX == s.fontScaleX
			  && fontScaleY == s.fontScaleY
			  && fontSpacing == s.fontSpacing
			  && fontWeight == s.fontWeight
			  && fItalic == s.fItalic
			  && fUnderline == s.fUnderline
			  && fStrikeOut == s.fStrikeOut
			  && fontAngleZ == s.fontAngleZ
			  && fontAngleX == s.fontAngleX
			  && fontAngleY == s.fontAngleY
			  // patch f001. fax fay patch (many instances at line)
			  && fontShiftX == s.fontShiftX
			  && fontShiftY == s.fontShiftY);
}

#ifdef _VSMOD
void STSStyle::mod_CopyStyleFrom(STSStyle& s)
{
	marginRect = s.marginRect;
	scrAlignment = s.scrAlignment;
	borderStyle = s.borderStyle;
	outlineWidthX = s.outlineWidthX;
	outlineWidthY = s.outlineWidthY;
	shadowDepthX = s.shadowDepthX;
	shadowDepthY = s.shadowDepthY;
	*((int*)&colors[0]) = *((int*)&s.colors[0]);
	*((int*)&colors[1]) = *((int*)&s.colors[1]);
	*((int*)&colors[2]) = *((int*)&s.colors[2]);
	*((int*)&colors[3]) = *((int*)&s.colors[3]);
	alpha[0] = s.alpha[0];
	alpha[1] = s.alpha[1];
	alpha[2] = s.alpha[2];
	alpha[3] = s.alpha[3];
	fBlur = s.fBlur;
	fGaussianBlur = s.fGaussianBlur;
	relativeTo = s.relativeTo;

	//patch m001. Vertical fontspacing
	mod_verticalSpace = s.mod_verticalSpace;
	//patch m002. Z-coord
	mod_z = s.mod_z;
	//patch m003. random text points
	mod_rand = s.mod_rand;
	//patch m004. gradient colors
	mod_grad = s.mod_grad;
	// patch m007. symbol rotating
	mod_fontOrient = s.mod_fontOrient;
	// patch m008. distort
	mod_distort = s.mod_distort;
	// patch m011. jitter
	mod_jitter = s.mod_jitter;
	// font
	charSet = s.charSet;
	fontName = s.fontName;
	fontSize = s.fontSize;
	fontScaleX = s.fontScaleX;
	fontScaleY = s.fontScaleY;
	fontSpacing = s.fontSpacing;
	fontWeight = s.fontWeight;
	fItalic = s.fItalic;
	fUnderline = s.fUnderline;
	fStrikeOut = s.fStrikeOut;
	fontAngleZ = s.fontAngleZ;
	fontAngleX = s.fontAngleX;
	fontAngleY = s.fontAngleY;
	// patch f001. fax fay patch (many instances at line)
	fontShiftX = s.fontShiftX;
	fontShiftY = s.fontShiftY;
}

STSStyle STSStyle::operator = (const STSStyle& s)
{
	if (this != &s) {
		mod_CopyStyleFrom(s);
	}
	return *this;
}
#endif

STSStyle& STSStyle::operator = (LOGFONT& lf)
{
	charSet = lf.lfCharSet;
	fontName = lf.lfFaceName;
	HDC hDC = GetDC(0);
	fontSize = -MulDiv(lf.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
	ReleaseDC(0, hDC);
	//	fontAngleZ = lf.lfEscapement/10.0;
	fontWeight = lf.lfWeight;
	fItalic = lf.lfItalic;
	fUnderline = lf.lfUnderline;
	fStrikeOut = lf.lfStrikeOut;
	return *this;
}

LOGFONTA& operator <<= (LOGFONTA& lfa, STSStyle& s)
{
	lfa.lfCharSet = s.charSet;
	strncpy_s(lfa.lfFaceName, LF_FACESIZE, CStringA(s.fontName), _TRUNCATE);
	HDC hDC = GetDC(0);
	lfa.lfHeight = -MulDiv((int)(s.fontSize+0.5), GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ReleaseDC(0, hDC);
	lfa.lfWeight = s.fontWeight;
	lfa.lfItalic = s.fItalic?-1:0;
	lfa.lfUnderline = s.fUnderline?-1:0;
	lfa.lfStrikeOut = s.fStrikeOut?-1:0;
	return(lfa);
}

LOGFONTW& operator <<= (LOGFONTW& lfw, STSStyle& s)
{
	lfw.lfCharSet = s.charSet;
	wcsncpy_s(lfw.lfFaceName, LF_FACESIZE, CStringW(s.fontName), _TRUNCATE);
	HDC hDC = GetDC(0);
	lfw.lfHeight = -MulDiv((int)(s.fontSize+0.5), GetDeviceCaps(hDC, LOGPIXELSY), 72);
	ReleaseDC(0, hDC);
	lfw.lfWeight = s.fontWeight;
	lfw.lfItalic = s.fItalic?-1:0;
	lfw.lfUnderline = s.fUnderline?-1:0;
	lfw.lfStrikeOut = s.fStrikeOut?-1:0;
	return(lfw);
}

CString& operator <<= (CString& style, STSStyle& s)
{
	style.Format(_T("%d;%d;%d;%d;%d;%d;%f;%f;%f;%f;0x%06x;0x%06x;0x%06x;0x%06x;0x%02x;0x%02x;0x%02x;0x%02x;%d;%s;%f;%f;%f;%f;%d;%d;%d;%d;%d;%f;%f;%f;%f;%d"),
				 s.marginRect.left, s.marginRect.right, s.marginRect.top, s.marginRect.bottom,
				 s.scrAlignment, s.borderStyle,
				 s.outlineWidthX, s.outlineWidthY, s.shadowDepthX, s.shadowDepthY,
				 s.colors[0], s.colors[1], s.colors[2], s.colors[3],
				 s.alpha[0], s.alpha[1], s.alpha[2], s.alpha[3],
				 s.charSet,
				 s.fontName,s.fontSize,
				 s.fontScaleX, s.fontScaleY,
				 s.fontSpacing,s.fontWeight,
				 s.fItalic, s.fUnderline, s.fStrikeOut, s.fBlur, s.fGaussianBlur,
				 s.fontAngleZ, s.fontAngleX, s.fontAngleY,
				 s.relativeTo);

	return(style);
}

STSStyle& operator <<= (STSStyle& s, CString& style)
{
	s.SetDefault();

	try {
		CStringW str = TToW(style);
		if (str.Find(';')>=0) {
			s.marginRect.left = GetInt(str, ';');
			s.marginRect.right = GetInt(str, ';');
			s.marginRect.top = GetInt(str, ';');
			s.marginRect.bottom = GetInt(str, ';');
			s.scrAlignment = GetInt(str, ';');
			s.borderStyle = GetInt(str, ';');
			s.outlineWidthX = GetFloat(str, ';');
			s.outlineWidthY = GetFloat(str, ';');
			s.shadowDepthX = GetFloat(str, ';');
			s.shadowDepthY = GetFloat(str, ';');
			for (ptrdiff_t i = 0; i < 4; i++) {
				s.colors[i] = (COLORREF)GetInt(str, ';');
			}
			for (ptrdiff_t i = 0; i < 4; i++) {
				s.alpha[i] = GetInt(str, ';');
			}
			s.charSet = GetInt(str, ';');
			s.fontName = WToT(GetStr(str, ';'));
			s.fontSize = GetFloat(str, ';');
			s.fontScaleX = GetFloat(str, ';');
			s.fontScaleY = GetFloat(str, ';');
			s.fontSpacing = GetFloat(str, ';');
			s.fontWeight = GetInt(str, ';');
			s.fItalic = GetInt(str, ';');
			s.fUnderline = GetInt(str, ';');
			s.fStrikeOut = GetInt(str, ';');
			s.fBlur = GetInt(str, ';');
			s.fGaussianBlur = GetFloat(str, ';');
			s.fontAngleZ = GetFloat(str, ';');
			s.fontAngleX = GetFloat(str, ';');
			s.fontAngleY = GetFloat(str, ';');
			s.relativeTo = GetInt(str, ';');
		}
	} catch (...) {
		s.SetDefault();
	}

	return(s);
}
