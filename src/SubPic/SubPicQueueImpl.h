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

#include "ISubPic.h"

class CSubPicQueueImpl : public CUnknown, public ISubPicQueue
{
	CCritSec m_csSubPicProvider;
	CComPtr<ISubPicProvider> m_pSubPicProvider;
	CCritSec m_csLock;
	CComPtr<ISubPic> m_pSubPic;
	double m_fps;

	CComPtr<ISubPicAllocator> m_pAllocator;

	HRESULT RenderTo(ISubPic* pSubPic, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double fps, BOOL bIsAnimated);

public:
	CSubPicQueueImpl(ISubPicAllocator* pAllocator, HRESULT* phr);
	virtual ~CSubPicQueueImpl();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicQueue
	STDMETHODIMP SetSubPicProvider(ISubPicProvider* pSubPicProvider);
	STDMETHODIMP SetFPS(double fps);
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME rtNow, CComPtr<ISubPic> &pSubPic);
};
