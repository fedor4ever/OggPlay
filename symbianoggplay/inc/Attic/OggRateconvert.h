class MOggSampleRateFillBuffer 
{
public:
   virtual TInt GetNewSamples(TDes8 &aBuffer) = 0;
};

class COggSampleRateConverter: public CBase
{
public:
    ~COggSampleRateConverter();

    void Init(MOggSampleRateFillBuffer * aSampleRateFillBufferProvider,
        TInt aBufferSize,
        TInt aMinimumSamplesInBuffer,
        TInt aInputSampleRate,
        TInt aOutputSampleRate,
        TInt aInputChannel,
        TInt aOutputChannel);
        
    TInt FillBuffer( TDes8 &aBuffer);
    void MixChannels( TDes8 &aInputBuffer, TDes8 &aOutputBuffer );
    void ConvertRate( TDes8 &aInputBuffer, TDes8 &aOutputBuffer );
    
private:
    TInt  iMinimumSamplesInBuffer;
    TBool iRateConvertionNeeded;
    TBool iChannelMixingNeeded;

    // Variables for rate convertion
    TUint32 iDtb;
    TUint32 iTime;
    TBool   iValidX1;
    TInt16  ix1;

    HBufC8 *iIntermediateBuffer;
    MOggSampleRateFillBuffer * iFillBufferProvider;
    TReal  iSamplingRateFactor ;
};
