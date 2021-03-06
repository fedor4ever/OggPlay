/*
 *  Copyright (c) 2004 OggPlay Team
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

#ifndef OGGPLAYCONTROLLER_H
#define OGGPLAYCONTROLLER_H

#include <e32base.h>
#include <e32std.h>
#include <e32math.h>
#include <ImplementationProxy.h>
#include <mmfcontroller.h>
#include <mmf\common\mmfstandardcustomcommands.h>
#include <mmf\server\mmfformat.h>
#include <MmfAudioOutput.h>
#include <MmfFile.h>
#include <mdaaudiooutputstream.h>
#include <mda/common/audio.h>

#include "OggRateConvert.h"
#include "OggPlayDecoder.h"

const TInt KMaxCommentLength = 256;

class RFile;
class COggSource;
class COggPlayController :	public CMMFController,
public MMMFAudioPlayDeviceCustomCommandImplementor, public MMMFAudioPlayControllerCustomCommandImplementor,
public MAsyncEventHandler, public MOggSampleRateFillBuffer, public MMdaAudioOutputStreamCallback
	{
public:
    // The decoder passed in the NewL is owned and will be destroyed by the controller
	static COggPlayController* NewL();
	~COggPlayController();

private:
    COggPlayController();
	void ConstructL();

	// From MMdaAudioOutputStreamCallback
	void MaoscPlayComplete(TInt aError);
	void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
	void MaoscOpenComplete(TInt aError);

public:
	// Functions from base classes

	/**
    * From CMMFController Add data source to controller.
    * @since
    * @param aDataSource A reference to the data source.
    * @return void
    */
	void AddDataSourceL(MDataSource& aDataSource);
	
	/**
    * From CMMFController Add data sink to controller.
    * @since
    * @param aDataSink A reference to the data sink.
    * @return void
    */
	void AddDataSinkL(MDataSink& aDataSink);

	/**
    * From CMMFController Remove data source from controller.
    * @since
    * @param  aDataSource A reference to the data source.
    * @return void
    */
	void RemoveDataSourceL(MDataSource& aDataSource);

	/**
    * From CMMFController Remove data sink from controller.
    * @since
    * @param aDataSink A reference to the data sink.
    * @return void
    */
	void RemoveDataSinkL(MDataSink& aDataSink);
        
	/**
    * From CMMFController Handle custom commands to controller.
    * @since
    * @param aMessage Message to controller.
    * @return void
    */
	void CustomCommand(TMMFMessage& aMessage);
        
	/**
    * From CMMFController Set priority settings.
    * @since
    * @param aPrioritySettings Wanted priority.
    * @return void
    */
	void SetPrioritySettings(const TMMFPrioritySettings& aPrioritySettings);

    /**
    * From MAsyncEventHandler
    */
    TInt SendEventToClient(const TMMFEvent& aEvent);

	/**
    * From CMMFController Reset controller.
    * @since
    * @param void
    * @return void
    */
	void ResetL();

	/**
    * From CMMFController  Primes controller.
    * @since
    * @param void
    * @return void
    */
	void PrimeL();

	/**
    * From CMMFController Start recording.
    * @since
    * @param void
    * @return void
    */
	void PlayL();

	/**
    * From CMMFController Pause recording.
    * @since
    * @param void
    * @return void
    */
	void PauseL();

	/**
    * From CMMFController Stop recording.
    * @since
    * @param void
    * @return void
    */
	void StopL();

	/**
    * From CMMFController Returns current recording position.
    * @since
    * @param void
    * @return Current position of recording.
    */
	TTimeIntervalMicroSeconds PositionL() const;

	/**
    * From CMMFController Sets current recording position. Not supported.
    * @since
    * @param aPosition Reference to wanted position.
    * @return void
    */
	void SetPositionL(const TTimeIntervalMicroSeconds& aPosition);

	/**
    * From CMMFController Returns current maximum duration.
    * @since
    * @param void 
    * @return Maximum remaining duration of recording.
    */
	TTimeIntervalMicroSeconds DurationL() const;

	void GetNumberOfMetaDataEntriesL(TInt& aNumberOfEntries);
	CMMFMetaDataEntry* GetMetaDataEntryL(TInt aIndex);

    void MapdSetVolumeL(TInt aVolume);
    void MapdGetMaxVolumeL(TInt& aMaxVolume);
    void MapdGetVolumeL(TInt& aVolume);
    void MapdSetVolumeRampL(const TTimeIntervalMicroSeconds& aRampDuration);
    void MapdSetBalanceL(TInt aBalance);
    void MapdGetBalanceL(TInt& aBalance);
    void MapcSetPlaybackWindowL(const TTimeIntervalMicroSeconds& aStart, const TTimeIntervalMicroSeconds& aEnd);
    void MapcDeletePlaybackWindowL();
    void MapcGetLoadingProgressL(TInt& aPercentageComplete);
        
    // From MOggSampleRateFillBuffer
    TInt GetNewSamples(TDes8 &aBuffer);

private:
	// Internal Functions
	MDecoder* GetDecoderL();
    void OpenFileL();
	void OpenCompleteL();

	void GetFrequenciesL(TMMFMessage& aMessage);
    void SetVolumeGainL(TMMFMessage& aMessage);

	TBool GetNextLowerRate(TInt& usedRate, TMdaAudioDataSettings::TAudioCaps& rt);
	void SetAudioCapsL(TInt theChannels, TInt theRate);

	void PlayNowL();
	void PlayDeferred();

private:
    // Controller internal states
	enum TOggPlayControllerState
	    {
		EStateNotOpened,
        EStateOpenInfo,
        EStateOpen,
        EStatePrimed,
		EStatePlaying,
        EStatePaused,
        EStateDestroying,
        EStateInterrupted
		};

	TOggPlayControllerState iState;
        
    TBuf<KMaxCommentLength> iAlbum;
    TBuf<KMaxCommentLength> iArtist;
    TBuf<KMaxCommentLength> iGenre;
    TBuf<KMaxCommentLength> iTitle;
    TBuf<KMaxCommentLength> iTrackNumber;
    TTimeIntervalMicroSeconds iFileLength;

	RFs iFs;
    TFileName iFileName;
    MDecoder *iDecoder;
        
    CMMFAudioOutput* iAudioOutput;
    COggSource* iOggSource;

	TMMFPrioritySettings iMMFPrioritySettings;
    TBool iRandomRingingTone;
    TInt iUsedRate;
    TInt iUsedChannels;

	enum TStreamState
		{
		EStreamNotOpen,
		EStreamStateRequested,
		EStreamOpened
		};

	CMdaAudioOutputStream* iStream;
	TMdaAudioDataSettings iSettings;
	TBool iStreamError;
	TStreamState iStreamState;
	TMMFMessage* iStreamMessage;
	TBool iPlayRequestPending;

	class TFreqBins 
		{
	public:
		TInt64 iPcmByte;
		TInt32 iFreqCoefs[KNumberOfFreqBins];
		};

	TFreqBins iFreqArray[KFreqArrayLength];
	TInt iLastFreqArrayIdx;

	TBool iRequestingFrequencyBins;
	TInt iBytesSinceLastFrequencyBin;
	TInt iMaxBytesBetweenFrequencyBins;
	TInt64 iLastPlayTotalBytes;

	TInt iRate;
	TInt iChannels;
	TInt iBitRate;
	TInt iFileSize;
	};


/**
Fake format decode class: this is required to satisfy CMMFAudioOutput::NegotiateL
which requires a CMMFFormatDecode: it will query it for configuration info 
(channels, sample rate etc.) and use this information to configure DevSound. This
class has no other functionality other than reporting this information.
*/
class CFakeFormatDecode : public CMMFFormatDecode
	{
public:
	CFakeFormatDecode(TFourCC aFourCC, TUint aChannels, TUint aSampleRate, TUint aBitRate);
	~CFakeFormatDecode();
	
	virtual TUint Streams(TUid aMediaType) const;
	virtual TTimeIntervalMicroSeconds FrameTimeInterval(TMediaId aMediaType) const;
	virtual TTimeIntervalMicroSeconds Duration(TMediaId aMediaType) const;
	virtual void FillBufferL(CMMFBuffer* aBuffer, MDataSink* aConsumer, TMediaId aMediaId);
	virtual CMMFBuffer* CreateSourceBufferL(TMediaId aMediaId, TBool &aReference);
	virtual TFourCC SourceDataTypeCode(TMediaId aMediaId);
	virtual TUint NumChannels();
	virtual TUint SampleRate();
	virtual TUint BitRate();

private:
	TFourCC iFourCC;
	TUint iChannels;
	TUint iSampleRate;
	TUint iBitRate;
	};


class COggSource : public CBase, public MDataSource
{
public:
    COggSource(MOggSampleRateFillBuffer &aSampleRateFillBuffer);
    ~COggSource();

    void ConstructL(TInt aBufferSize, TInt aInputRate, TInt aOutputRate, TInt aInputChannel, TInt aOutputChannel);

    // from MDataSource:
    virtual TFourCC SourceDataTypeCode(TMediaId aMediaId);
    virtual void FillBufferL(CMMFBuffer* aBuffer, MDataSink* aConsumer,TMediaId aMediaId);
    virtual void BufferEmptiedL(CMMFBuffer* aBuffer);
    virtual TBool CanCreateSourceBuffer();
    virtual CMMFBuffer* CreateSourceBufferL(TMediaId aMediaId, TBool &aReference);
    virtual void ConstructSourceL(  const TDesC8& aInitData );
    
    // From MOggSampleRateFillBuffer 
    TInt GetNewSamples(TDes8 &aBuffer);

    // Own functions
    void SetSink(MDataSink* aSink);
	void SetVolumeGain(TGainType aGain);

public:
	TInt64 iTotalBufferBytes;

private:
    COggSampleRateConverter *iOggSampleRateConverter;
    MDataSink * iSink;
    MOggSampleRateFillBuffer &iSampleRateFillBuffer;

	TGainType iGain;
};

#endif    
