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
                                   TInt /*aBufferSize*/,
                                   TInt aMinimumSamplesInBuffer,
                                   TInt aInputSampleRate,
                                   TInt aOutputSampleRate,
                                   TInt aInputChannel,
                                   TInt aOutputChannel)
{
    iFillBufferProvider = aSampleRateFillBufferProvider;
    iMinimumSamplesInBuffer = aMinimumSamplesInBuffer;
    if (aInputSampleRate != aOutputSampleRate)
    {
        iRateConvertionNeeded = ETrue;
    }
    else
    {
        iRateConvertionNeeded = EFalse;
    }

    if (aInputChannel != aOutputChannel)
    {
        iChannelMixingNeeded = ETrue;
    }
    else
    {
        iChannelMixingNeeded = EFalse;
    }

     __ASSERT_ALWAYS( ( iRateConvertionNeeded && ((aInputChannel != 1) && (!iChannelMixingNeeded) )) == 0,
         User::Panic(_L("only one track can be converted, when rate convertion is needed"), 0));
      
     
    iSamplingRateFactor = static_cast<TReal> (aInputSampleRate) /aOutputSampleRate;

    if (iRateConvertionNeeded)
    {
        __ASSERT_ALWAYS(iSamplingRateFactor>1, User::Panic(_L("Sampling rate factor should be >1"), 0));
        iDtb = static_cast<TUint32> (iSamplingRateFactor*(1<<15) + 0.5);    /* Fixed-point representation */
        iTime = 0;
        iValidX1 = EFalse;
        ix1 = 0;
    }

    if (iRateConvertionNeeded || iChannelMixingNeeded)
    {
        if (iIntermediateBuffer)
        {
            delete iIntermediateBuffer;
            iIntermediateBuffer = NULL;
        }
        // It is a bit of mystery with for HBufC8 we need to divide by 2 the length.
        iIntermediateBuffer = HBufC8::NewL(4096);
    }
}

COggSampleRateConverter::~COggSampleRateConverter()
{
    if (iIntermediateBuffer)
    {
        delete iIntermediateBuffer;
        }
}


void COggSampleRateConverter::SetVolumeGain(TGainType aGain)
{
    iGain = aGain;
}

TInt COggSampleRateConverter::FillBuffer(TDes8 &aBuffer)
{
    long ret=0;
    aBuffer.SetLength(0);

    TPtr8 InterBuffer(NULL,0);
    if (iIntermediateBuffer)
    {
        InterBuffer.Set(iIntermediateBuffer->Des());
    }
  
    // Audio Processing Routing
	// Request frequency bins for each complete buffer
	TBool requestFrequencyBins = ETrue;
    switch( (iChannelMixingNeeded <<1) + iRateConvertionNeeded )
    {
    case 0:
        // No Channel mixing, No rate convertion
        while (aBuffer.Length() <= iMinimumSamplesInBuffer) 
        {
            ret = iFillBufferProvider->GetNewSamples(aBuffer, requestFrequencyBins);
            if (ret <=0) break;

			requestFrequencyBins = EFalse;
        }
        break;
        
    case 1:
        // No Channel mixing, rate convertion
        while(aBuffer.Length() <= iMinimumSamplesInBuffer)
        {
            InterBuffer.SetLength(0);

            TInt remains = (TInt) ( ( aBuffer.MaxLength() - aBuffer.Length() ) * iSamplingRateFactor) + 1;
            if (remains  <4096)
                InterBuffer.Set( (TUint8 *) InterBuffer.Ptr(), 0, remains );

            ret = iFillBufferProvider->GetNewSamples(InterBuffer, requestFrequencyBins);
            if (ret <=0) break;

			requestFrequencyBins = EFalse;
            ConvertRate(InterBuffer, aBuffer );
        }
        break;
    case 2:
        // Channel mixing, no rate convertion
        {
            TPtr8 OutBufferPtr( (TUint8*) aBuffer.Ptr(), aBuffer.MaxLength() );
            while(aBuffer.Length() <= iMinimumSamplesInBuffer)
            {
                InterBuffer.SetLength(0);
                TInt remains = (aBuffer.MaxLength()-aBuffer.Length())<<1;
                if (remains  <4096)
                    InterBuffer.Set( (TUint8 *) InterBuffer.Ptr(), 0, remains );

                ret = iFillBufferProvider->GetNewSamples(InterBuffer, requestFrequencyBins);
                if (ret <=0) break;

				requestFrequencyBins = EFalse;
                ret = ret >> 1;
                MixChannels(InterBuffer, OutBufferPtr);
                OutBufferPtr.Set((TUint8 *) &(OutBufferPtr.Ptr()[ret]),
                    0,
                    OutBufferPtr.MaxLength() - ret);
                aBuffer.SetLength(aBuffer.Length()+ret);
            }
            break;
        }
    case 3:
        // Channel mixing, Rate convertion
        while(aBuffer.Length() <= iMinimumSamplesInBuffer)
        {
            InterBuffer.SetLength(0);
            TInt remains = (TInt)( ( ( aBuffer.MaxLength() - aBuffer.Length() ) << 1) 
                            *(iSamplingRateFactor) ) + 1;
            if (remains  <4096)
                InterBuffer.Set( (TUint8 *) InterBuffer.Ptr(), 0, remains );
            ret = iFillBufferProvider->GetNewSamples(InterBuffer, requestFrequencyBins);
            if (ret <=0) break;

			requestFrequencyBins = EFalse;
            MixChannels(InterBuffer, InterBuffer);
            ConvertRate(InterBuffer, aBuffer);
        }
        break;
    }

    switch (iGain)
    {
    case ENoGain:
        break;

    case EMinus12dB:
        ApplyNegativeGain(aBuffer,2);
        break;

	case EMinus18dB:
        ApplyNegativeGain(aBuffer,3);
        break;

	case EMinus24dB:
        ApplyNegativeGain(aBuffer,4);
        break;

    case EStatic6dB:
        ApplyGain(aBuffer,1);
        break;

    case EStatic12dB:   
        ApplyGain(aBuffer,2);
        break;
    }

	// TRACEF(COggLog::VA(_L("FillBuf out") ));
    return (ret);
}

void COggSampleRateConverter::ConvertRate( TDes8 &aInputBuffer, TDes8 &aOutputBuffer )
{ 
    TInt16 x2;
    TInt32 xL1, xL2;
    TInt32 v;
    TInt16 *X = (TInt16 *) aInputBuffer.Ptr(); 
    
    // Divided by two, because we're dealing with Int16 but buffers are Int8
    // -1 because we use 2 values for each computation
    TInt maxLength = (aInputBuffer.Length()>>1) -1; 

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
        Xp = &X[idx];                 /* Ptr to current input sample */ 
        ix1 = *Xp++;
    }

    y = (TUint16) (iTime & 0x7FFF);
    for (;;) //Forever
    {
        x2 = *Xp;
        xL1 = ix1 * ((1<<15 ) - y);
        xL2 = x2 * y;
        v = xL1 + xL2;
        *output++ = (TInt16) (v >> 15);        /* Deposit output */
        iTime += iDtb;                         /* Move to next sample by time increment */
        idx = (TInt) (iTime)>>15;
        if (idx >= maxLength) break;
        y = (TUint16) (iTime & 0x7FFF);
        Xp = &X[idx];                          /* Ptr to current input sample */
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
    TInt AddedSamples = (((TInt) (output-outputStart))<<1);
    aOutputBuffer.SetLength( aOutputBuffer.Length() +  AddedSamples);
}


void COggSampleRateConverter::MixChannels( TDes8 &aInputBuffer, TDes8 &aOutputBuffer )
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

void COggSampleRateConverter::ApplyGain( TDes8 &aInputBuffer, TInt shiftValue )
{
    
    TInt16 *ptr = (TInt16 *) aInputBuffer.Ptr();
    TInt16 length = (TInt16) (aInputBuffer.Length() >>1); /* Divided by 2
                                              because of Int8 to Int16 cast */
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
        "cmp   r0 , r1 ;"
        "movgt r0 , r1 ;"
        "cmp   r0 , r2 ;"
        "movlt r0 , r2 ;"
        "strh   r0, [%0], #2 ;"
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

void COggSampleRateConverter::ApplyNegativeGain( TDes8 &aInputBuffer, TInt shiftValue )
{
    TInt16 *ptr = (TInt16 *) aInputBuffer.Ptr();
    TInt16 length = (TInt16) (aInputBuffer.Length() >>1); /* Divided by 2
                                              because of Int8 to Int16 cast */
    if (length <= 0) return;
    
#ifdef __WINS__
TInt tmp,i; 
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
