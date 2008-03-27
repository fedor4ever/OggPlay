/*
 *  Copyright (c) 2003 L. H. Wilden.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <e32cons.h>
#include "OggRateConvert.h"

COggSampleRateConverter::COggSampleRateConverter()
: iGain(ENoGain)
	{
	}

void COggSampleRateConverter::Init(MOggSampleRateFillBuffer *aSampleRateFillBufferProvider,
TInt /*aBufferSize*/, TInt aMinimumSamplesInBuffer, TInt aInputSampleRate,
TInt aOutputSampleRate, TInt aInputChannel, TInt aOutputChannel)
	{
    iFillBufferProvider = aSampleRateFillBufferProvider;
    iMinimumSamplesInBuffer = aMinimumSamplesInBuffer;
    iRateConvertionNeeded = aInputSampleRate != aOutputSampleRate;
    iChannelMixingNeeded = aInputChannel != aOutputChannel;

	if (aOutputChannel == 2)
		iConvertRateFn = &COggSampleRateConverter::ConvertRateStereo;
	else
		iConvertRateFn = &COggSampleRateConverter::ConvertRateMono;

    iSamplingRateFactor = static_cast<TReal> (aInputSampleRate) /aOutputSampleRate;
    if (iRateConvertionNeeded)
		{
        __ASSERT_ALWAYS(iSamplingRateFactor>1, User::Panic(_L("Sampling rate factor should be >1"), 0));
        iDtb = static_cast<TUint32> (iSamplingRateFactor*(1<<15) + 0.5);    /* Fixed-point representation */
        iTime = 0;
        iValidX1 = EFalse;
        ix1 = 0;
		ix2 = 0;
		}

    if (iRateConvertionNeeded || iChannelMixingNeeded)
		{
		if (!iIntermediateBuffer)
			iIntermediateBuffer = HBufC8::NewL(4096);
		}
	else
		{
		delete iIntermediateBuffer;
		iIntermediateBuffer = NULL;
		}
	}

COggSampleRateConverter::~COggSampleRateConverter()
	{
	delete iIntermediateBuffer;
	}


void COggSampleRateConverter::SetVolumeGain(TGainType aGain)
	{
    iGain = aGain;
	}

TInt COggSampleRateConverter::FillBuffer(TDes8 &aBuffer)
	{
    TInt ret = 0;
	TInt bufLength = aBuffer.Length();

	TPtr8 interBuffer(NULL, 0);
    if (iIntermediateBuffer)
        interBuffer.Set(iIntermediateBuffer->Des());
  
    // Audio Processing Routing
	// Request frequency bins for each complete buffer
    switch( (iChannelMixingNeeded <<1) + iRateConvertionNeeded )
	    {
		case 0:
	        // No Channel mixing, No rate convertion
		    while (aBuffer.Length() <= iMinimumSamplesInBuffer) 
				{
	            ret = iFillBufferProvider->GetNewSamples(aBuffer);
		        if (ret<=0)
					break;
				}
			break;
        
		case 1:
			// No Channel mixing, rate convertion
			while(aBuffer.Length() <= iMinimumSamplesInBuffer)
				{
		        interBuffer.SetLength(0);

			    TInt remains = (TInt) ((aBuffer.MaxLength() - aBuffer.Length()) * iSamplingRateFactor) - 4;
				if (remains<4096)
					interBuffer.Set((TUint8 *) interBuffer.Ptr(), 0, remains);

				ret = iFillBufferProvider->GetNewSamples(interBuffer);
				if (ret<=0)
					break;

				(this->*iConvertRateFn)(interBuffer, aBuffer);			
				}
			break;

		case 2:
			// Channel mixing, no rate conversion
	        {
            TPtr8 outBufferPtr((TUint8*) aBuffer.Ptr(), aBuffer.MaxLength());
            while(aBuffer.Length() <= iMinimumSamplesInBuffer)
		        {
                interBuffer.SetLength(0);
                TInt remains = (aBuffer.MaxLength()-aBuffer.Length())<<1;
                if (remains<4096)
                    interBuffer.Set((TUint8 *) interBuffer.Ptr(), 0, remains);

                ret = iFillBufferProvider->GetNewSamples(interBuffer);
                if (ret <=0)
					break;

                ret = ret >> 1;
                MixChannels(interBuffer, outBufferPtr);
                outBufferPtr.Set((TUint8 *) &(outBufferPtr.Ptr()[ret]), 0, outBufferPtr.MaxLength() - ret);
                aBuffer.SetLength(aBuffer.Length()+ret);
			    }
            break;
			}

		case 3:
        // Channel mixing, Rate convertion
        while(aBuffer.Length() <= iMinimumSamplesInBuffer)
			{
            interBuffer.SetLength(0);
            TInt remains = (TInt) (((aBuffer.MaxLength() - aBuffer.Length()) << 1) * iSamplingRateFactor) - 2;

            if (remains<4096)
                interBuffer.Set((TUint8 *) interBuffer.Ptr(), 0, remains);

			ret = iFillBufferProvider->GetNewSamples(interBuffer);
            if (ret <=0)
				break;

            MixChannels(interBuffer, interBuffer);
            ConvertRateMono(interBuffer, aBuffer);
			}
		break;
		}

	// Apply the gain to the new samples that have been added
	TInt bytesAdded = aBuffer.Length() - bufLength;
	TPtr8 buffer((TUint8*) (aBuffer.Ptr()+bufLength), bytesAdded, bytesAdded);
    switch (iGain)
	    {
		case ENoGain:
			break;

	    case EMinus3dB:
			// 11/16 = -3.25dB
			ApplyNegativeGain(buffer, 11, 4);
	        break;

		case EMinus6dB:
		    ApplyNegativeGain(buffer, 1);
			break;

		case EMinus9dB:
			// 11/32 = -9.28dB
			ApplyNegativeGain(buffer, 11, 5);
	        break;

		case EMinus12dB:
		    ApplyNegativeGain(buffer, 2);
			break;

		case EMinus15dB:
			// 11/64 = -15.3dB
			ApplyNegativeGain(buffer, 11, 6);
	        break;

		case EMinus18dB:
	        ApplyNegativeGain(buffer, 3);
		    break;

		case EMinus21dB:
			// 11/128 = -21.3dB
	        ApplyNegativeGain(buffer, 11, 7);
		    break;

		case EMinus24dB:
			ApplyNegativeGain(buffer, 4);
	        break;

		case EStatic1dB:
			// 9/8 = +1.02dB
			ApplyGain(buffer, 9, 3);
			break;

		case EStatic2dB:
			// 5/4 = +1.94dB
			ApplyGain(buffer, 5, 2);
			break;

		case EStatic4dB:
			// 25/16 = +3.88dB
			ApplyGain(buffer, 25, 4);
			break;

		case EStatic6dB:
			ApplyGain(buffer, 1);
			break;

		case EStatic8dB:
			// 20/8 = +7.96dB
	        ApplyGain(buffer, 20, 3);
		    break;

		case EStatic10dB:
			// 25/8 = +9.90dB
	        ApplyGain(buffer, 25, 3);
		    break;

		case EStatic12dB:   
			ApplyGain(buffer, 2);
			break;
	    }

    return ret;
	}

void COggSampleRateConverter::ConvertRateMono(TDes8 &aInputBuffer, TDes8 &aOutputBuffer)
{
    TInt16 x2;
    TInt32 xL1, xL2;
    TInt32 v;
    TInt16 *X = (TInt16 *) aInputBuffer.Ptr(); 
    
    // Divided by two, because we're dealing with Int16 but buffers are Int8
    // -1 because we use 2 values for each computation
    TInt maxLength = (aInputBuffer.Length()>>1) - 1; 

    TInt16 *Xp;  
    TUint16 y; 

	// Add the output at the end of the existing Descriptor
    TInt16 *output = (TInt16 *) &(aOutputBuffer.Ptr()[aOutputBuffer.Length()]); 
    TInt16 *outputStart = output;

	TInt idx=0;
    if (iValidX1)
    {
        Xp = &X[0];
    }
    else
    {             
        idx = (iTime)>>15; 

		// Ptr to current input sample
		Xp = &X[idx];
        ix1 = *Xp++;
    }

    y = (TUint16) (iTime & 0x7FFF);
    for (;;) // Forever
    {
		// Calculate output sample
		x2 = *Xp;
        xL1 = ix1 * ((1<<15 ) - y);
        xL2 = x2 * y;
        v = xL1 + xL2;
        *output++ = (TInt16) (v >> 15);

		// Move to next sample by time increment
		iTime += iDtb;
        idx = (TInt) (iTime)>>15;
        if (idx >= maxLength) break;
        y = (TUint16) (iTime & 0x7FFF);

		// Ptr to current input sample
		Xp = &X[idx];
        ix1 = *Xp++;
    }

    if (idx == maxLength)
    {
        Xp = &X[idx]; 
        ix1 = *Xp;
        iValidX1 = ETrue;
    }
    else 
    {
        iValidX1 = EFalse;
    }

	iTime = iTime - ((maxLength+1)*(1<<15));
    TInt addedSamples = ((TInt) (output-outputStart))<<1;
    aOutputBuffer.SetLength(aOutputBuffer.Length() +  addedSamples);
}

void COggSampleRateConverter::ConvertRateStereo(TDes8 &aInputBuffer, TDes8 &aOutputBuffer)
{ 
    TInt16 x2;
    TInt32 xL1, xL2;
    TInt32 v;
    TInt16 *X = (TInt16 *) aInputBuffer.Ptr(); 
    
    // Divided by two, because we're dealing with Int16 but buffers are Int8, 
    // -2 because we use 2 values (x2) for each computation
    TInt maxLength = (aInputBuffer.Length()>>1) - 2;

    TInt16 *Xp;  
    TUint16 y; 

	// Add the output at the end of the existing Descriptor
    TInt16 *output = (TInt16 *) &(aOutputBuffer.Ptr()[aOutputBuffer.Length()]); 
    TInt16 *outputStart = output;

    TInt idx=0;
    if (iValidX1)
    {
        Xp = &X[0];
    }
    else
    {             
        idx = (iTime)>>15;
		idx = idx<<1;

		// Ptr to current input sample
		Xp = &X[idx];
        ix1 = *Xp++;
		ix2 = *Xp++;
    }

    y = (TUint16) (iTime & 0x7FFF);
    for (;;) //Forever
    {
		// Calculate output sample
        x2 = *Xp++;
        xL1 = ix1 * ((1<<15 ) - y);
        xL2 = x2 * y;
        v = xL1 + xL2;
        *output++ = (TInt16) (v >> 15);

		// Calculate output sample
        x2 = *Xp;
        xL1 = ix2 * ((1<<15 ) - y);
        xL2 = x2 * y;
        v = xL1 + xL2;
        *output++ = (TInt16) (v >> 15);

		// Move to next sample by time increment
		iTime += iDtb;
        idx = (TInt) (iTime)>>15;
		idx = idx<<1;
        if (idx >= maxLength) break;
        y = (TUint16) (iTime & 0x7FFF);

		// Ptr to current input sample
		Xp = &X[idx];
        ix1 = *Xp++;
		ix2 = *Xp++;
	}

	if (idx == maxLength)
    {
        Xp = &X[idx];
        ix1 = *Xp++;
		ix2 = *Xp;
        iValidX1 = ETrue;
    }
    else 
    {
        iValidX1 = EFalse;
    }

	iTime = iTime - ((maxLength+2)*(1<<14));
    TInt addedSamples = ((TInt) (output-outputStart))<<1;
    aOutputBuffer.SetLength(aOutputBuffer.Length() +  addedSamples);
}

void COggSampleRateConverter::MixChannels(TDes8 &aInputBuffer, TDes8 &aOutputBuffer)
{
    // Input and Output buffer can be the same.
    // The OutputBuffer is overwritten with the new values;

    // Mix the 2 stereo tracks to one track.
    TInt i;
    TInt16 * SampleInPtr = (TInt16*) aInputBuffer.Ptr();
    TInt16 * SampleOutPtr = (TInt16*) aOutputBuffer.Ptr();
    TInt16 Left,Right;

    for (i=0; i<aInputBuffer.Length()>>2; i++)
    {
        Left =  (TInt16) (*SampleInPtr++ >> 1);
        Right = (TInt16) (*SampleInPtr++ >> 1);
        *SampleOutPtr++ = (TInt16) (Left + Right);
    }
    aOutputBuffer.SetLength(aInputBuffer.Length()>>1) ;
}

void COggSampleRateConverter::ApplyGain(TDes8 &aInputBuffer, TInt shiftValue)
{
    TInt16 *ptr = (TInt16 *) aInputBuffer.Ptr();
    TInt16 length = (TInt16) (aInputBuffer.Length() >>1); // Divided by 2 because of Int8 to Int16 cast
    if (length <= 0) return;
    
#ifdef __WINS__
    TInt max = 32767;
    TInt min = -32768;
    TInt tmp,i; 
    for (i=0; i<length; i++)
    {
        tmp = *ptr;
        tmp = tmp << shiftValue;
        if (tmp >max) tmp = max;
        if (tmp <min) tmp = min; 
        *ptr++ = (TInt16) ((0xFFFF) & tmp);
    }
#else
    /* Compiler is lousy, better do it in assembly */    
    asm (        
        "ldr   r1, 3333f;"      /* r1 = Max value : 0x7FFF */
        "ldr   r2, 3333f + 4;"  /* r2 = Min value : 0x8000 */
        "mov   r3, %1;"         /* r3 = loop index */
        "1:" /* Loop Begins here */
        "ldrsh   r0, [%0];"
        "mov   r0, r0, lsl %2;"
        "cmp   r0 , r1;"
        "movgt r0 , r1;"
        "cmp   r0 , r2;"
        "movlt r0 , r2;"
        "strh   r0, [%0], #2;"
        "subs   r3, r3, #1;"
        "bne  1b;"
        "b   2f;"
        "3333:" /* Constant table */
        ".align 0;"
        ".word 32767;"
        ".word -32768;"
        "2:" /* End of the game */
    :  /* no output registers */
    : "r"(ptr), "r"(length), "r"(shiftValue) /* Input */
    : "r0", "r1", "r2", "r3", "cc", "memory" /* Clobbered */
        );
#endif
}

void COggSampleRateConverter::ApplyGain(TDes8 &aInputBuffer, TInt multiplier, TInt shiftValue)
{    
    TInt16 *ptr = (TInt16 *) aInputBuffer.Ptr();
    TInt16 length = (TInt16) (aInputBuffer.Length() >>1); // Divided by 2 because of Int8 to Int16 cast */
    if (length <= 0) return;
    
#ifdef __WINS__
    TInt max = 32767;
    TInt min = -32768;
    TInt tmp,i; 
    for (i=0; i<length; i++)
    {
        tmp = *ptr;
		tmp *= multiplier;
        tmp = tmp >> shiftValue;
        if (tmp>max) tmp = max;
        if (tmp<min) tmp = min; 
        *ptr++ = (TInt16) ((0xFFFF) & tmp);
    }
#else
    /* Compiler is lousy, better do it in assembly */    
    asm (      
		"mov   r1, %2;"         /* Multiplier */
        "ldr   r2, 3333f;"      /* r1 = Max value : 0x7FFF */
        "ldr   r3, 3333f + 4;"  /* r2 = Min value : 0x8000 */
        "mov   r4, %1;"         /* r3 = loop index */

		"1:" /* Loop Begins here */
        "ldrsh   r0, [%0];"
		"mul   r0, r1, r0;"
        "mov   r0, r0, asr %3;"
        "cmp   r0 , r2;"
        "movgt r0 , r2;"
        "cmp   r0 , r3;"
        "movlt r0 , r3;"
        "strh  r0, [%0], #2;"
        "subs  r4, r4, #1;"
        "bne  1b;"
        "b   2f;"

		"3333:" /* Constant table */
        ".align 0;"
        ".word 32767;"
        ".word -32768;"

		"2:" /* End of the game */
    :  /* no output registers */
    : "r"(ptr), "r"(length), "r"(multiplier), "r"(shiftValue) /* Input */
    : "r0", "r1", "r2", "r3", "r4", "cc", "memory" /* Clobbered */
        );
#endif
}

void COggSampleRateConverter::ApplyNegativeGain(TDes8 &aInputBuffer, TInt shiftValue)
{
    TInt16 *ptr = (TInt16 *) aInputBuffer.Ptr();
    TInt16 length = (TInt16) (aInputBuffer.Length() >>1); // Divided by 2 because of Int8 to Int16 cast
    if (length <= 0) return;
    
#ifdef __WINS__
	TInt tmp, i; 
	for (i=0; i<length; i++)
	{
	    tmp = *ptr;
		tmp = tmp >> shiftValue;
	    *ptr++ = (TInt16) ((0xFFFF) & tmp);
	}
#else
    /* Compiler is lousy, better do it in assembly */
    asm (        
        "mov   r1, %1;"         /* r1 = loop index */

		"1:" /* Loop Begins here */
        "ldrsh   r0, [%0];"
        "mov   r0, r0, asr %2;"
        "strh   r0, [%0], #2 ;"
        "subs   r1, r1, #1;"
        "bne  1b;"
    :  /* no output registers */
    : "r"(ptr), "r"(length), "r"(shiftValue) /* Input */
    : "r0", "r1", "cc", "memory" /* Clobbered */
        );
#endif
}

void COggSampleRateConverter::ApplyNegativeGain(TDes8 &aInputBuffer, TInt multiplier, TInt shiftValue)
{
    TInt16 *ptr = (TInt16 *) aInputBuffer.Ptr();
    TInt16 length = (TInt16) (aInputBuffer.Length() >>1); // Divided by 2 because of Int8 to Int16 cast
    if (length <= 0) return;
    
#ifdef __WINS__
	TInt tmp, i; 
	for (i=0; i<length; i++)
	{
	    tmp = *ptr;
		tmp *= multiplier;
		tmp = tmp >> shiftValue;
	    *ptr++ = (TInt16) ((0xFFFF) & tmp);
	}
#else
    /* Compiler is lousy, better do it in assembly */
    asm (
		"mov   r1, %2;"         /* multiplier */
        "mov   r2, %1;"         /* r1 = loop index */

		"1:" /* Loop Begins here */
        "ldrsh   r0, [%0];"
		"mul   r0, r1, r0;"
        "mov   r0, r0, asr %3;"
        "strh   r0, [%0], #2 ;"
        "subs   r2, r2, #1;"
        "bne  1b;"
    :  /* no output registers */
    : "r"(ptr), "r"(length), "r"(multiplier), "r"(shiftValue) /* Input */
    : "r0", "r1", "r2", "cc", "memory" /* Clobbered */
        );
#endif
}
