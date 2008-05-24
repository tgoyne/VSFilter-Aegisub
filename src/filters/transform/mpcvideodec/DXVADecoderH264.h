/* 
 * $Id$
 *
 * (C) 2006-2007 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#pragma once

#include <dxva.h>
#include "DXVADecoder.h"
#include "H264QuantizationMatrix.h"

typedef enum
{
	NALU_TYPE_SLICE    = 1,
	NALU_TYPE_DPA      = 2,
	NALU_TYPE_DPB      = 3,
	NALU_TYPE_DPC      = 4,
	NALU_TYPE_IDR      = 5,
	NALU_TYPE_SEI      = 6,
	NALU_TYPE_SPS      = 7,
	NALU_TYPE_PPS      = 8,
	NALU_TYPE_AUD      = 9,
	NALU_TYPE_EOSEQ    = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL     = 12
} NALU_TYPE;

class CNalu
{
private :
	int			forbidden_bit;      //! should be always FALSE
	int			nal_reference_idc;  //! NALU_PRIORITY_xxxx
	NALU_TYPE	nal_unit_type;      //! NALU_TYPE_xxxx    

	int			m_nNALStartPos;		//! NALU start (including startcode / size)
	int			m_nNALDataPos;		//! Useful part
	unsigned	m_nDataLen;			//! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)

	BYTE*		m_pBuffer;
	int			m_nCurPos;
	int			m_nNextRTP;
	int			m_nSize;
	int			m_nNALSize;

	bool		MoveToNextStartcode();

public :
	NALU_TYPE	GetType()		{ return nal_unit_type; };
	bool		IsRefFrame()	{ return (nal_reference_idc != 0); };

	int			GetDataLength()	{ return m_nCurPos - m_nNALDataPos; };
	BYTE*		GetDataBuffer() { return m_pBuffer + m_nNALDataPos; };

	int			GetLength()		{ return m_nCurPos - m_nNALStartPos; };
	BYTE*		GetNALBuffer()	{ return m_pBuffer + m_nNALStartPos; };

	void		SetBuffer (BYTE* pBuffer, int nSize, int nNALSize);
	bool		ReadNext();
};


class CDXVADecoderH264 : public CDXVADecoder
{
public:
	CDXVADecoderH264 (CMPCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber);
	CDXVADecoderH264 (CMPCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber);

	virtual HRESULT DecodeFrame   (BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual void	SetExtraData  (BYTE* pDataIn, UINT nSize);
	virtual void	CopyBitstream (BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);
	virtual void	Flush();

protected :


private:

	DXVA_PicParams_H264		m_DXVAPicParams;
	DXVA_Qmatrix_H264		m_DXVAScalingMatrix;
	UINT					m_nCurRefFrame;		// First free RefFrameList position
	DXVA_Slice_H264_Short	m_SliceShort;
	int						m_nNALLength;

	// Private functions
	void					Init();
	HRESULT					DisplayStatus();

	// DXVA functions
	void					ClearRefFramesList();
	void					UpdateRefFramesList (int nFrameNum, bool bRefFrame);
};
