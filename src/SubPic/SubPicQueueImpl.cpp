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
#include "SubPicQueueImpl.h"

//
// CSubPicQueueImpl
//

CSubPicQueueImpl::CSubPicQueueImpl(ISubPicAllocator* pAllocator, HRESULT* phr)
	: CUnknown(NAME("CSubPicQueueImpl"), NULL)
	, m_pAllocator(pAllocator)
{
	if (phr) {
		*phr = S_OK;
	}

	if (!m_pAllocator) {
		if (phr) {
			*phr = E_FAIL;
		}
		return;
	}
}

CSubPicQueueImpl::~CSubPicQueueImpl()
{
}

STDMETHODIMP CSubPicQueueImpl::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ISubPicQueue)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// ISubPicQueue

STDMETHODIMP CSubPicQueueImpl::SetSubPicProvider(ISubPicProvider* pSubPicProvider)
{
	CAutoLock cAutoLock(&m_csSubPicProvider);
	m_pSubPicProvider = pSubPicProvider;
	m_pSubPic = NULL;

	return S_OK;
}

// private

HRESULT CSubPicQueueImpl::RenderTo(ISubPic* pSubPic, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double fps, BOOL bIsAnimated)
{
	HRESULT hr = E_FAIL;

	if (!pSubPic) {
		return hr;
	}

	SubPicDesc spd;
	hr = pSubPic->ClearDirtyRect(0xFF000000);
	if (SUCCEEDED(hr)) {
		hr = pSubPic->Lock(spd);
	}
	if (SUCCEEDED(hr)) {
		CRect r(0,0,0,0);
		hr = m_pSubPicProvider->Render(spd, bIsAnimated ? rtStart : ((rtStart+rtStop)/2), fps, r);

		pSubPic->SetStart(rtStart);
		pSubPic->SetStop(rtStop);

		pSubPic->Unlock(r);
	}

	return hr;
}

// ISubPicQueue
STDMETHODIMP CSubPicQueueImpl::SetFPS(double fps)
{
	m_fps = fps;

	return S_OK;
}

STDMETHODIMP_(bool) CSubPicQueueImpl::LookupSubPic(REFERENCE_TIME rtNow, CComPtr<ISubPic> &ppSubPic)
{
	CComPtr<ISubPic> pSubPic;

	{
		CAutoLock cAutoLock(&m_csLock);

		if (!m_pSubPic) {
			if (FAILED(m_pAllocator->AllocDynamic(&m_pSubPic))) {
				return false;
			}
		}

		pSubPic = m_pSubPic;
	}

	if (pSubPic->GetStart() <= rtNow && rtNow < pSubPic->GetStop()) {
		ppSubPic = pSubPic;
	} else {
		if (m_pSubPicProvider) {
			double fps = m_fps;

			POSITION pos = m_pSubPicProvider->GetStartPosition(rtNow, fps);
			if (pos != 0) {
				REFERENCE_TIME rtStart = m_pSubPicProvider->GetStart(pos, fps);
				REFERENCE_TIME rtStop = m_pSubPicProvider->GetStop(pos, fps);

				if (m_pSubPicProvider->IsAnimated(pos)) {
					rtStart = rtNow;
					rtStop = rtNow+1;
				}

				if (rtStart <= rtNow && rtNow < rtStop) {
					SIZE	MaxTextureSize, VirtualSize;
					POINT	VirtualTopLeft;
					HRESULT	hr2;
					if (SUCCEEDED (hr2 = m_pSubPicProvider->GetTextureSize(pos, MaxTextureSize, VirtualSize, VirtualTopLeft))) {
						m_pAllocator->SetMaxTextureSize(MaxTextureSize);
					}

					if (m_pAllocator->IsDynamicWriteOnly()) {
						CComPtr<ISubPic> pStatic;
						HRESULT hr = m_pAllocator->GetStatic(&pStatic);
						if (SUCCEEDED(hr)) {
							hr = RenderTo(pStatic, rtStart, rtStop, fps, false);
						}
						if (SUCCEEDED(hr)) {
							hr = pStatic->CopyTo(pSubPic);
						}
						if (SUCCEEDED(hr)) {
							ppSubPic = pSubPic;
						}
					} else {
						if (SUCCEEDED(RenderTo(m_pSubPic, rtStart, rtStop, fps, false))) {
							ppSubPic = pSubPic;
						}
					}
					if (SUCCEEDED(hr2)) {
						pSubPic->SetVirtualTextureSize (VirtualSize, VirtualTopLeft);
					}
				}
			}

			if (ppSubPic) {
				CAutoLock cAutoLock(&m_csLock);

				m_pSubPic = ppSubPic;
			}
		}
	}

	return(!!ppSubPic);
}
