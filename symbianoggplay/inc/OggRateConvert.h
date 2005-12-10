#ifndef OGGRATECONVERT_H
#define OGGRATECONVERT_H

class MOggSampleRateFillBuffer 
{
public:
   virtual TInt GetNewSamples(TDes8 &aBuffer, TBool aRequestFrequencyBins) = 0;
};


typedef enum {
    EMinus24dB,
    EMinus18dB,
    EMinus12dB,
    ENoGain,
    EStatic6dB,  // A static gain of 6dB, with saturation
    EStatic12dB   // A static gain of 12 dB, with saturation
} TGainType;


class COggSampleRateConverter: public CBase
{
public:
	COggSampleRateConverter();
    ~COggSampleRateConverter();

    void Init(MOggSampleRateFillBuffer * aSampleRateFillBufferProvider,
        TInt aBufferSize,
        TInt aMinimumSamplesInBuffer,
        TInt aInputSampleRate,
        TInt aOutputSampleRate,
        TInt aInputChannel,
        TInt aOutputChannel);
        
    TInt FillBuffer( TDes8 &aBuffer);
    void SetVolumeGain(TGainType aGain);
private:
    void MixChannels( TDes8 &aInputBuffer, TDes8 &aOutputBuffer );
    void ConvertRateMono( TDes8 &aInputBuffer, TDes8 &aOutputBuffer );
    void ConvertRateStereo( TDes8 &aInputBuffer, TDes8 &aOutputBuffer );
    void ApplyGain( TDes8 &aInputBuffer, TInt shiftValue );
    void ApplyNegativeGain( TDes8 &aInputBuffer, TInt shiftValue );
    TInt  iMinimumSamplesInBuffer;
    TBool iRateConvertionNeeded;
    TBool iChannelMixingNeeded;

    // Variables for rate convertion
    TUint32 iDtb;
    TUint32 iTime;
    TBool   iValidX1;
    TInt16  ix1;
    TInt16  ix2;
    TGainType iGain;

    HBufC8 *iIntermediateBuffer;
    MOggSampleRateFillBuffer * iFillBufferProvider;
    TReal  iSamplingRateFactor;

	void (COggSampleRateConverter::*iConvertRateFn)(TDes8& aInputBuffer, TDes8& aOutputBuffer);
};

#endif
