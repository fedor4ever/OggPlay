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

TInt COggSampleRateConverter::FillBuffer( TDes8 &aBuffer)
{
    long ret=0;
    aBuffer.SetLength(0);

    TPtr8 InterBuffer(NULL,0);
    if (iIntermediateBuffer)
    {
        InterBuffer.Set(iIntermediateBuffer->Des());
    }
  
    // Audio Processing Routing
    switch( (iChannelMixingNeeded <<1) + iRateConvertionNeeded )
    {
    case 0:
        // No Channel mixing, No rate convertion
        while (aBuffer.Length() <= iMinimumSamplesInBuffer) 
        {
            ret = iFillBufferProvider->GetNewSamples(aBuffer);
            if (ret <=0) break;
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

            ret = iFillBufferProvider->GetNewSamples(InterBuffer);
            if (ret <=0) break;
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

                ret = iFillBufferProvider->GetNewSamples(InterBuffer);
                if (ret <=0) break;
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
            ret = iFillBufferProvider->GetNewSamples(InterBuffer);
            MixChannels(InterBuffer, InterBuffer);
            ConvertRate(InterBuffer, aBuffer);
        }
        break;
    }

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
