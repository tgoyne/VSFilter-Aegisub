#ifndef FFMPEG_CONFIG_H
#define FFMPEG_CONFIG_H

#ifdef __GNUC__
	#define HAVE_INLINE_ASM 1
	#define HAVE_MMX 1
	#define HAVE_MMX2 1
	#define HAVE_SSE 1
	#define HAVE_SSSE3 1
	#define HAVE_AMD3DNOW 1
	#define HAVE_AMD3DNOWEXT 1

	#define ARCH_X86 1

	#ifdef ARCH_X86_64
		#define HAVE_FAST_64BIT 1
		#define HAVE_CMOV 1
		#define HAVE_FAST_CMOV 1
		#define HAVE_STRUCT_TIMESPEC 1
	#else
		#define ARCH_X86_32 1
		#define ARCH_X86_64 0
	#endif

	#define PTW32_STATIC_LIB 1
	#define restrict restrict
#else
	#define HAVE_INLINE_ASM 0
	#define HAVE_MMX 0
	#define HAVE_MMX2 0
	#define HAVE_SSE 0
	#define HAVE_SSSE3 0
	#define HAVE_AMD3DNOW 0
	#define HAVE_AMD3DNOWEXT 0
	#define ARCH_X86 0
	#define ARCH_X86_32 0
	#define ARCH_X86_64 0
	#define HAVE_FAST_64BIT 0
	#define HAVE_CMOV 0
	#define HAVE_FAST_CMOV 0

	#define restrict
	#define __asm__ __asm
#endif

#define FFMPEG_LICENSE "GPL version 2.1 or later"
#define CC_TYPE "gcc"
#define CC_VERSION __VERSION__

#define ASMALIGN(ZEROBITS) ".align 1 << " #ZEROBITS "\n\t"

#define EXTERN_PREFIX "_"
#define EXTERN_ASM _

#define HAVE_ALTIVEC 0
#define HAVE_ALTIVEC_H 0
#define HAVE_BIGENDIAN 0
#define HAVE_BSWAP 1
#define HAVE_EBP_AVAILABLE 1
#define HAVE_EBX_AVAILABLE 1
#define HAVE_FAST_CLZ 0
#define HAVE_FAST_UNALIGNED 1
#define HAVE_LOCAL_ALIGNED_16 1
#define HAVE_LOCAL_ALIGNED_8 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMALIGN 1
#define HAVE_TEN_OPERANDS 1
#define HAVE_THREADS 1
#define HAVE_W32THREADS 1
#define HAVE_YASM 1

#ifdef __GNUC__
	#define HAVE_ATTRIBUTE_PACKED 1
	#define HAVE_ATTRIBUTE_MAY_ALIAS 1
#else
	#define HAVE_ATTRIBUTE_PACKED 0
	#define HAVE_ATTRIBUTE_MAY_ALIAS 0
	#define EMULATE_FAST_INT
#endif

#define CONFIG_DWT 0
#define CONFIG_HARDCODED_TABLES 0
#define CONFIG_GPL 1
#define CONFIG_GRAY 1
#define CONFIG_H264DSP 1
#define CONFIG_LIBAMR_NB 1
#define CONFIG_LIBXVID 0
#define CONFIG_LPC 0
#define CONFIG_MDCT 1
#define CONFIG_MPEGAUDIO_HP 1
#define CONFIG_SMALL 0
#define CONFIG_ZLIB 1

#define CONFIG_DECODERS 1
#define CONFIG_ENCODERS 0
#define CONFIG_SWSCALE 1
#define CONFIG_SWSCALE_ALPHA 1
#define CONFIG_POSTPROC 0
#define CONFIG_RUNTIME_CPUDETECT 1

#define CONFIG_AASC_DECODER 0
#define CONFIG_AMV_DECODER 1
#define CONFIG_ASV1_DECODER 0
#define CONFIG_ASV2_DECODER 0
#define CONFIG_AVS_DECODER 0
#define CONFIG_CAVS_DECODER 0
#define CONFIG_CINEPAK_DECODER 0
#define CONFIG_COREPNG_DECODER 0
#define CONFIG_CSCD_DECODER 0
#define CONFIG_CYUV_DECODER 0
#define CONFIG_DVVIDEO_DECODER 0
#define CONFIG_EIGHTBPS_DECODER 0
#define CONFIG_FFV1_DECODER 0
#define CONFIG_FFVHUFF_DECODER 0
#define CONFIG_FLV_DECODER 1
#define CONFIG_FRAPS_DECODER 0
#define CONFIG_H261_DECODER 0
#define CONFIG_H263_DECODER 1
#define CONFIG_H263I_DECODER 1
#define CONFIG_H264_DECODER 1
#define CONFIG_HUFFYUV_DECODER 0
#define CONFIG_INDEO2_DECODER 0
#define CONFIG_INDEO3_DECODER 0
#define CONFIG_JPEGLS_DECODER 0
#define CONFIG_LOCO_DECODER 0
#define CONFIG_MJPEG_DECODER 0
#define CONFIG_MJPEGB_DECODER 0
#define CONFIG_MPEG1VIDEO_DECODER 0
#define CONFIG_MPEG2VIDEO_DECODER 1
#define CONFIG_MPEG4_DECODER 1
#define CONFIG_MPEGVIDEO_DECODER 0
#define CONFIG_MSMPEG4V1_DECODER 1
#define CONFIG_MSMPEG4V2_DECODER 1
#define CONFIG_MSMPEG4V3_DECODER 1
#define CONFIG_MSRLE_DECODER 0
#define CONFIG_MSVIDEO1_DECODER 0
#define CONFIG_MSZH_DECODER 0
#define CONFIG_PNG_DECODER 0
#define CONFIG_QPEG_DECODER 0
#define CONFIG_QTRLE_DECODER 0
#define CONFIG_RPZA_DECODER 0
#define CONFIG_RV10_DECODER 1
#define CONFIG_RV20_DECODER 1
#define CONFIG_RV30_DECODER 1
#define CONFIG_RV40_DECODER 1
#define CONFIG_SNOW_DECODER 0
#define CONFIG_SP5X_DECODER 1
#define CONFIG_SVQ1_DECODER 1
#define CONFIG_SVQ3_DECODER 1
#define CONFIG_THEORA_DECODER 1
#define CONFIG_TRUEMOTION1_DECODER 0
#define CONFIG_TRUEMOTION2_DECODER 0
#define CONFIG_TSCC_DECODER 0
#define CONFIG_ULTI_DECODER 0
#define CONFIG_VC1_DECODER 1
#define CONFIG_VCR1_DECODER 0
#define CONFIG_VP3_DECODER 1
#define CONFIG_VP5_DECODER 1
#define CONFIG_VP6_DECODER 1
#define CONFIG_VP6A_DECODER 1
#define CONFIG_VP6F_DECODER 1
#define CONFIG_WMV1_DECODER 1
#define CONFIG_WMV2_DECODER 1
#define CONFIG_WMV3_DECODER 1
#define CONFIG_WNV1_DECODER 0
#define CONFIG_XL_DECODER 0
#define CONFIG_ZLIB_DECODER 0
#define CONFIG_ZMBV_DECODER 0

#define CONFIG_AAC_DECODER 0
#define CONFIG_AC3_DECODER 1
#define CONFIG_ATRAC3_DECODER 0
#define CONFIG_COOK_DECODER 0
#define CONFIG_DCA_DECODER 0
#define CONFIG_EAC3_DECODER 1
#define CONFIG_FLAC_DECODER 0
#define CONFIG_IMC_DECODER 0
#define CONFIG_MACE3_DECODER 0
#define CONFIG_MACE6_DECODER 0
#define CONFIG_MLP_DECODER 1
#define CONFIG_MP1_DECODER 0
#define CONFIG_MP2_DECODER 0
#define CONFIG_MP3_DECODER 0
#define CONFIG_MSGSM_DECODER 0
#define CONFIG_NELLYMOSER_DECODER 1
#define CONFIG_QDM2_DECODER 0
#define CONFIG_RA_144_DECODER 0
#define CONFIG_RA_288_DECODER 0
#define CONFIG_TRUEHD_DECODER 1
#define CONFIG_TRUESPEECH_DECODER 0
#define CONFIG_TTA_DECODER 0
#define CONFIG_VORBIS_DECODER 0
#define CONFIG_WAVPACK_DECODER 0
#define CONFIG_WMAV1_DECODER 0
#define CONFIG_WMAV2_DECODER 0
#define CONFIG_PCM_ALAW_DECODER 0
#define CONFIG_PCM_MULAW_DECODER 0
#define CONFIG_ADPCM_4XM_DECODER 1
#define CONFIG_ADPCM_ADX_DECODER 1
#define CONFIG_ADPCM_CT_DECODER 1
#define CONFIG_ADPCM_EA_DECODER 1
#define CONFIG_ADPCM_G726_DECODER 0
#define CONFIG_ADPCM_IMA_AMV_DECODER 1
#define CONFIG_ADPCM_IMA_DK3_DECODER 1
#define CONFIG_ADPCM_IMA_DK4_DECODER 1
#define CONFIG_ADPCM_IMA_QT_DECODER 1
#define CONFIG_ADPCM_IMA_SMJPEG_DECODER 1
#define CONFIG_ADPCM_IMA_WAV_DECODER 1
#define CONFIG_ADPCM_IMA_WS_DECODER 1
#define CONFIG_ADPCM_MS_DECODER 0
#define CONFIG_ADPCM_SBPRO_2_DECODER 1
#define CONFIG_ADPCM_SBPRO_3_DECODER 1
#define CONFIG_ADPCM_SBPRO_4_DECODER 1
#define CONFIG_ADPCM_SWF_DECODER 1
#define CONFIG_ADPCM_XA_DECODER 1
#define CONFIG_ADPCM_YAMAHA_DECODER 1

#define CONFIG_LIBAMR_NB_DECODER 1

#define CONFIG_DVVIDEO_ENCODER 0
#define CONFIG_FFV1_ENCODER 0
#define CONFIG_FFVHUFF_ENCODER 0
#define CONFIG_FLV_ENCODER 0
#define CONFIG_H261_ENCODER 0
#define CONFIG_H263_ENCODER 0
#define CONFIG_H263P_ENCODER 0
#define CONFIG_H264_ENCODER 0
#define CONFIG_HUFFYUV_ENCODER 0
#define CONFIG_LJPEG_ENCODER 0
#define CONFIG_MJPEG_ENCODER 0
#define CONFIG_MPEG1VIDEO_ENCODER 0
#define CONFIG_MPEG2VIDEO_ENCODER 0
#define CONFIG_MPEG4_ENCODER 0
#define CONFIG_MSMPEG4V1_ENCODER 0
#define CONFIG_MSMPEG4V2_ENCODER 0
#define CONFIG_MSMPEG4V3_ENCODER 0
#define CONFIG_PNG_ENCODER 0
#define CONFIG_RV10_ENCODER 0
#define CONFIG_RV20_ENCODER 0
#define CONFIG_SNOW_ENCODER 0
#define CONFIG_WMV1_ENCODER 0
#define CONFIG_WMV2_ENCODER 0
#define CONFIG_AC3_ENCODER 0

#define CONFIG_AAC_PARSER 0
#define CONFIG_AC3_PARSER 1
#define CONFIG_DCA_PARSER 0
#define CONFIG_H263_PARSER 0
#define CONFIG_H264_PARSER 0
#define CONFIG_MJPEG_PARSER 0
#define CONFIG_MPEGAUDIO_PARSER 0
#define CONFIG_MPEG4VIDEO_PARSER 0
#define CONFIG_MLP_PARSER 1

#endif /* FFMPEG_CONFIG_H */
