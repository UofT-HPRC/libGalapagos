//Filename::hls_ssr_fft_criss_cross_multiplexer.h
#ifndef HLS_SSR_FFT_CRISS_CROSS_MULTIPLEXER_H_
#define HLS_SSR_FFT_CRISS_CROSS_MULTIPLEXER_H_


/*
=========================================================================================
-_-                                                                                   -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
-_- The output data generated by SSR FFT is digit reversed in time direction and      -_-
-_- also shuffled in SSR dimension. To re order the data it is required to buffer     -_-
-_- data and do time reversal and also re-shuffle along SSR dimension to bring data   -_-
-_- in original order. The data re ordering is done in 4 phases. CrissCroosMultiplex  -_-
-_- reads in R memories and shuffles the output connections to output streams during  -_-
-_- writing.  CissCrossMultiplexer generates R if else branches every branch  defines -_-
-_- single cycle mapping of R memories to R output streams. The memory to stream      -_-
-_- mapping changes every cycle.                                                      -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
-_-                                                                                   -_-
 ========================================================================================
 */


#ifdef __HLS_SSR_FFT_LOCAL_LIB_DEVLOPMENT_PROJECT__
	//======================================================================
	// If source files are used for development of hls ssr fft IP locally
	// then set the files path as local.
	//=======================================================================
	#include "hls_ssr_fft_utilities.h"
#else //__HLS_SSR_FFT_LOCAL_LIB_DEVLOPMENT_PROJECT__ not defied
	//======================================================================
	// if the ssr fft source files are to be used in vivado_hls library with
	// released version of the tool then set path according to the placement
	// of the library
	//=======================================================================
	#include "hls/ssr_lib/fft/hls_ssr_fft_utilities.h"
#endif
namespace hls
{
namespace ssr_fft
{
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	template <int stage>
	struct crissCrossMultiplexerClass
	{
		template <int tp_R, int tp_L, typename tt_T,typename tt_T2>
		void crissCrossMultiplexer(int timeIndexAddressOffset,int ssrDimensionAddressOffset,tt_T in[tp_R][tp_L/tp_R], tt_T2 out[tp_R] );
	};



	template <int stage>
	template <int tp_R, int tp_L, typename tt_T,typename tt_T2>
	void crissCrossMultiplexerClass<stage>::crissCrossMultiplexer(int timeIndexAddressOffset,int ssrDimensionAddressOffset,tt_T in[tp_R][tp_L/tp_R], tt_T2 out[tp_R] )
	{
	#pragma HLS INLINE

			if(ssrDimensionAddressOffset==(stage-1))
			{
				SSR_LOOP:
				for (unsigned int r = 0; r < tp_R; r++)
				{
	#pragma HLS UNROLL
					/*This expression is replaced below*///int pingPongSuperSampleIndex = ((stage-1) + r) % tp_R;
					int pingPongSuperSampleIndex = ((stage-1) + r) & ssr_fft_log2BitwiseAndModMask<tp_R>::val;

					int pingPongTimeIndex=r+timeIndexAddressOffset;
				out[r]= in[pingPongSuperSampleIndex][pingPongTimeIndex];
				}
			}
			else
			{
				crissCrossMultiplexerClass<stage-1> obj;
				obj.template crissCrossMultiplexer<tp_R,tp_L,tt_T,tt_T2>(timeIndexAddressOffset,ssrDimensionAddressOffset,in,out);
			}
	}




	template <>
	template <int tp_R, int tp_L, typename tt_T,typename tt_T2>
	void crissCrossMultiplexerClass<1>::crissCrossMultiplexer(int timeIndexAddressOffset,int ssrDimensionAddressOffset,tt_T in[tp_R][tp_L/tp_R], tt_T2 out[tp_R] )
	{
	#pragma HLS INLINE

		SSR_LOOP:
					for (unsigned int r = 0; r < tp_R; r++)
					{
		#pragma HLS UNROLL
						int pingPongSuperSampleIndex = ((1-1) + r) & ssr_fft_log2BitwiseAndModMask<tp_R>::val;
						int pingPongTimeIndex=r+timeIndexAddressOffset;
					out[r]= in[pingPongSuperSampleIndex][pingPongTimeIndex];
					}
	}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
} //namespace hls
} //namespace ssr_fft


#endif //HLS_SSR_FFT_CRISS_CROSS_MULTIPLEXER_H_
