#ifndef OGGRATECONVERT_H
#define OGGRATECONVERT_H

const TInt KFreqArrayLength = 100; // Length of the memory of the previous freqs bin
const TInt KNumberOfFreqBins = 16; // This shouldn't be changed without making same changes to vorbis library!

class MOggSampleRateFillBuffer 
{
public:
   virtual TInt GetNewSamples(TDes8 &aBuffer, TBool aRequestFrequencyBins) = 0;
};


typedef enum {
    EMinus24dB,
    EMinus21dB,
    EMinus18dB,
    EMinus15dB,
    EMinus12dB,
    EMinus9dB,
    EMinus6dB,
    EMinus3dB,
    ENoGain,
	EStatic1dB, // Static gain with staturation
	EStatic2dB,
	EStatic4dB,
    EStatic6dB,
    EStatic8dB,
    EStatic10dB,
    EStatic12dB
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
        
    TInt FillBuffer(TDes8 &aBuffer);
    void SetVolumeGain(TGainType aGain);
private:
    void MixChannels(TDes8 &aInputBuffer, TDes8 &aOutputBuffer);
    void ConvertRateMono(TDes8 &aInputBuffer, TDes8 &aOutputBuffer);
    void ConvertRateStereo( TDes8 &aInputBuffer, TDes8 &aOutputBuffer);
    void ApplyGain(TDes8 &aInputBuffer, TInt shiftValue);
    void ApplyGain(TDes8 &aInputBuffer, TInt multiplier, TInt shiftValue);
    void ApplyNegativeGain(TDes8 &aInputBuffer, TInt shiftValue);
    void ApplyNegativeGain(TDes8 &aInputBuffer, TInt multiplier, TInt shiftValue);

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
