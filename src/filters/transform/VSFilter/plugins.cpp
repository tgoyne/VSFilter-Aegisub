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
#include <afxdlgs.h>
#include <atlpath.h>
#include "resource.h"
#include "../../../Subtitles/RTS.h"
#include "../../../SubPic/MemSubPic.h"
#include "../../../SubPic/SubPicQueueImpl.h"
#include "vfr.h"

//
// Generic interface
//

namespace Plugin
{

	class CFilter
	{

	protected:
		float m_fps;
		CCritSec m_csSubLock;
		CComPtr<ISubPicQueue> m_pSubPicQueue;
		CComPtr<ISubPicProvider> m_pSubPicProvider;
		DWORD_PTR m_SubPicProviderId;

	public:
		CFilter(CString fn, int CharSet = DEFAULT_CHARSET, float fps = -1)
			: m_fps(fps), m_SubPicProviderId(0)
		{
			Open(fn, CharSet);
		}

		virtual ~CFilter() {
		}

		bool Render(SubPicDesc& dst, REFERENCE_TIME rt, float fps) {
			if (!m_pSubPicProvider) {
				return false;
			}

			CSize size(dst.w, dst.h);

			if (!m_pSubPicQueue) {
				CComPtr<ISubPicAllocator> pAllocator = new CMemSubPicAllocator(dst.type, size);

				HRESULT hr;
				if (!(m_pSubPicQueue = new CSubPicQueueImpl(pAllocator, &hr)) || FAILED(hr)) {
					m_pSubPicQueue = NULL;
					return false;
				}
			}

			if (m_SubPicProviderId != (DWORD_PTR)(ISubPicProvider*)m_pSubPicProvider) {
				m_pSubPicQueue->SetSubPicProvider(m_pSubPicProvider);
				m_SubPicProviderId = (DWORD_PTR)(ISubPicProvider*)m_pSubPicProvider;
			}

			m_pSubPicQueue->SetFPS(fps);
			CComPtr<ISubPic> pSubPic;
			if (!m_pSubPicQueue->LookupSubPic(rt, pSubPic)) {
				return false;
			}

			CRect r;
			pSubPic->GetDirtyRect(r);

			if (dst.type == MSP_RGB32 || dst.type == MSP_RGB24 || dst.type == MSP_RGB16 || dst.type == MSP_RGB15) {
				dst.h = -dst.h;
			}

			pSubPic->AlphaBlt(r, r, &dst);

			return true;
		}

		bool Open(CString fn, int CharSet = DEFAULT_CHARSET) {
			m_pSubPicProvider = NULL;

			if (CRenderedTextSubtitle* rts = new CRenderedTextSubtitle(&m_csSubLock)) {
				if (rts->Open(CString(fn), CharSet)) {
					m_pSubPicProvider = (ISubPicProvider*)rts;
				}
			}

			return !!m_pSubPicProvider;
		}
	};

	//
	// Avisynth interface
	//

	namespace AviSynth25
	{
#include <avisynth\avisynth25.h>

		static bool s_fSwapUV = false;

		class CTextSubAvisynthFilter : public GenericVideoFilter, public CFilter
		{
		public:
			VFRTranslator *vfr;

			CTextSubAvisynthFilter(PClip c, IScriptEnvironment* env, const char* fn, int CharSet = DEFAULT_CHARSET, float fps = -1, VFRTranslator *vfr = 0) //vfr patch
			:  GenericVideoFilter(c)
			, CFilter(CString(fn), CharSet, fps)
			, vfr(vfr)
			{
				if (!m_pSubPicProvider)
#ifdef _VSMOD
					env->ThrowError("TextSubMod: Can't open \"%s\"", fn);
#else
					env->ThrowError("TextSub: Can't open \"%s\"", fn);
#endif
			}

			PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
				PVideoFrame frame = child->GetFrame(n, env);

				env->MakeWritable(&frame);

				SubPicDesc dst;
				dst.w = vi.width;
				dst.h = vi.height;
				dst.pitch = frame->GetPitch();
				dst.pitchUV = frame->GetPitch(PLANAR_U);
				dst.bits = (void**)frame->GetWritePtr();
				dst.bitsU = frame->GetWritePtr(PLANAR_U);
				dst.bitsV = frame->GetWritePtr(PLANAR_V);
				dst.bpp = dst.pitch/dst.w*8; //vi.BitsPerPixel();
				dst.type =
					vi.IsRGB32() ?( env->GetVar("RGBA").AsBool() ? MSP_RGBA : MSP_RGB32)  :
						vi.IsRGB24() ? MSP_RGB24 :
						vi.IsYUY2() ? MSP_YUY2 :
				/*vi.IsYV12()*/ vi.pixel_type == VideoInfo::CS_YV12 ? (s_fSwapUV?MSP_IYUV:MSP_YV12) :
				/*vi.IsIYUV()*/ vi.pixel_type == VideoInfo::CS_IYUV ? (s_fSwapUV?MSP_YV12:MSP_IYUV) :
						-1;

				float fps = m_fps > 0 ? m_fps : (float)vi.fps_numerator / vi.fps_denominator;

				REFERENCE_TIME timestamp;

				if (!vfr) {
					timestamp = (REFERENCE_TIME)(10000000i64 * n / fps);
				} else {
					timestamp = (REFERENCE_TIME)(10000000 * vfr->TimeStampFromFrameNumber(n));
				}

				Render(dst, timestamp, fps);

				return(frame);
			}
		};

		AVSValue __cdecl TextSubCreateGeneral(AVSValue args, void* user_data, IScriptEnvironment* env)
		{
			if (!args[1].Defined())
#ifdef _VSMOD
				env->ThrowError("TextSubMod: You must specify a subtitle file to use");
#else
				env->ThrowError("TextSub: You must specify a subtitle file to use");
#endif
			VFRTranslator *vfr = 0;
			if (args[4].Defined()) {
				vfr = GetVFRTranslator(args[4].AsString());
			}

			return(new CTextSubAvisynthFilter(
					   args[0].AsClip(),
					   env,
					   args[1].AsString(),
					   args[2].AsInt(DEFAULT_CHARSET),
					   args[3].AsFloat(-1),
					   vfr));
		}

		AVSValue __cdecl TextSubSwapUV(AVSValue args, void* user_data, IScriptEnvironment* env)
		{
			s_fSwapUV = args[0].AsBool(false);
			return(AVSValue());
		}

		AVSValue __cdecl MaskSubCreate(AVSValue args, void* user_data, IScriptEnvironment* env)/*SIIFI*/
		{
			if (!args[0].Defined())
#ifdef _VSMOD
				env->ThrowError("MaskSubMod: You must specify a subtitle file to use");
#else
				env->ThrowError("MaskSub: You must specify a subtitle file to use");
#endif
			if (!args[3].Defined() && !args[6].Defined())
#ifdef _VSMOD
				env->ThrowError("MaskSubMod: You must specify either FPS or a VFR timecodes file");
#else
				env->ThrowError("MaskSub: You must specify either FPS or a VFR timecodes file");
#endif
			VFRTranslator *vfr = 0;
			if (args[6].Defined()) {
				vfr = GetVFRTranslator(args[6].AsString());
			}

			AVSValue rgb32("RGB32");
			AVSValue fps(args[3].AsFloat(25));
			AVSValue  tab[6] = {
				args[1],
				args[2],
				args[3],
				args[4],
				rgb32
			};
			AVSValue value(tab,5);
			const char * nom[5]= {
				"width",
				"height",
				"fps",
				"length",
				"pixel_type"
			};
			AVSValue clip(env->Invoke("Blackness",value,nom));
			env->SetVar(env->SaveString("RGBA"),true);
			//return(new CTextSubAvisynthFilter(clip.AsClip(), env, args[0].AsString()));
			return(new CTextSubAvisynthFilter(
					   clip.AsClip(),
					   env,
					   args[0].AsString(),
					   args[5].AsInt(DEFAULT_CHARSET),
					   args[3].AsFloat(-1),
					   vfr));
		}

		extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
		{
#ifdef _VSMOD
			env->AddFunction("TextSubMod", "c[file]s[charset]i[fps]f[vfr]s", TextSubCreateGeneral, 0);
			env->AddFunction("TextSubModSwapUV", "b", TextSubSwapUV, 0);
			env->AddFunction("MaskSubMod", "[file]s[width]i[height]i[fps]f[length]i[charset]i[vfr]s", MaskSubCreate, 0);
#else
			env->AddFunction("TextSub", "c[file]s[charset]i[fps]f[vfr]s", TextSubCreateGeneral, 0);
			env->AddFunction("TextSubSwapUV", "b", TextSubSwapUV, 0);
			env->AddFunction("MaskSub", "[file]s[width]i[height]i[fps]f[length]i[charset]i[vfr]s", MaskSubCreate, 0);
#endif
			env->SetVar(env->SaveString("RGBA"),false);
			return(NULL);
		}
	}

}
