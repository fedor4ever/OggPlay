/*
*  Copyright (c) 2004 OggPlayTeam
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

#include "OggPlayController.h"
#include <charconv.h>
#include <utf.h>
#include <string.h>
#include "OggLog.h"
#include "Plugin\ImplementationUIDs.hrh"

#if 1
#include <e32svr.h> 
#define PRINT(x) TRACEF(_L(x));
//#define PRINT(x) RDebug::Print _L(x);
#else
#define PRINT()
#endif

COggPlayController* COggPlayController::NewL(MDecoder * aDecoder)
{
    PRINT("COggPlayController::NewL()");
    COggPlayController* self = new(ELeave) COggPlayController(aDecoder);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
}

COggPlayController::COggPlayController(MDecoder *aDecoder)
{
    iDecoder = aDecoder;
}

COggPlayController::~COggPlayController()
{
    iState = EStateDestroying;
    PRINT("COggPlayController::~COggPlayController In");
    if (iOwnSinkBuffer) 
        delete iSinkBuffer;
    delete iOggSource;
    if (iAudioOutput)
        iAudioOutput->SinkThreadLogoff();
    iDecoder->Clear();
    delete iDecoder;
    CloseSTDLIB();    	// Just in case
    PRINT("COggPlayController::~COggPlayController Out");
    COggLog::Exit();  // MUST BE AFTER LAST TRACE, otherwise will leak memory
}

void COggPlayController::ConstructL()
{
    PRINT("COggPlayController::ConstructL() In");

    // Construct custom command parsers

    CMMFAudioPlayDeviceCustomCommandParser* audPlayDevParser = 
                                     CMMFAudioPlayDeviceCustomCommandParser::NewL(*this);
    CleanupStack::PushL(audPlayDevParser);
    AddCustomCommandParserL(*audPlayDevParser); //parser now owned by controller framework
    CleanupStack::Pop();//audPlayDevParser

    CMMFAudioPlayControllerCustomCommandParser* audPlayCtrlParser =
                                     CMMFAudioPlayControllerCustomCommandParser::NewL(*this);
    CleanupStack::PushL(audPlayCtrlParser);
    AddCustomCommandParserL(*audPlayCtrlParser); //parser now owned by controller framework
    CleanupStack::Pop(audPlayCtrlParser);

    iState = EStateNotOpened;
     
    iFileLength = TTimeIntervalMicroSeconds(1E6);
    PRINT("COggPlayController::ConstructL() Out");
}

void COggPlayController::AddDataSourceL(MDataSource& aDataSource)
{
    PRINT("COggPlayController::AddDataSourceL In");

    if ( iState != EStateNotOpened )
    {
        User::Leave(KOggPlayPluginErrNotReady);
    }

    TUid uid = aDataSource.DataSourceType() ;
    if (uid == KUidMmfFileSource)
    {
        CMMFFile* aFile = STATIC_CAST(CMMFFile*, &aDataSource);
        iFileName = aFile->FullName();
    }
    else
    {
        User::Leave(KOggPlayPluginErrNotSupported);
    }

    
    TRACEF(COggLog::VA(_L("iFileName %S "),&iFileName ));
    // Open the file here, in order to get the tags.
    OpenFileL(iFileName);
    // Save the tags.

    iDecoder->Clear(); // Close the file. This is required for the ringing tone stuff.

    iState = EStateNotOpened;

    iOggSource = new (ELeave) COggSource( *this);
    iOggSource->ConstructL();
    
    PRINT("COggPlayController::AddDataSourceL Out");
}

void COggPlayController::AddDataSinkL(MDataSink& aDataSink)
{
    PRINT("COggPlayController::AddDataSinkL In");
    
    if (aDataSink.DataSinkType() != KUidMmfAudioOutput)
        User::Leave(KErrNotSupported);
    if (iAudioOutput)
        User::Leave(KErrNotSupported);

    iAudioOutput = static_cast<CMMFAudioOutput*>(&aDataSink);
    iAudioOutput->SinkThreadLogon(*this);

    iAudioOutput->SetSinkPrioritySettings(iMMFPrioritySettings);
    PRINT("COggPlayController::AddDataSinkL Out");
}

void COggPlayController::RemoveDataSourceL(MDataSource& /*aDataSource*/)
{
    PRINT("COggPlayController::RemoveDataSourceL");
    // Nothing to do
}
void COggPlayController::RemoveDataSinkL(MDataSink&  aDataSink)
{
    PRINT("COggPlayController::RemoveDataSinkL");

    if (iState==EStatePlaying)
        User::Leave(KErrNotReady);

    if (iAudioOutput==&aDataSink) 
        iAudioOutput=NULL;

}

void COggPlayController::SetPrioritySettings(const TMMFPrioritySettings& aPrioritySettings)
{
    
    PRINT("COggPlayController::SetPrioritySettingsL");
    // Not implemented yet
    iMMFPrioritySettings = aPrioritySettings;
    if (iAudioOutput)
        iAudioOutput->SetSinkPrioritySettings(aPrioritySettings);
}

void COggPlayController::MapdSetVolumeRampL(const TTimeIntervalMicroSeconds& /*aRampDuration*/)
{
    PRINT("COggPlayController::MapdSetVolumeRampL");
    // No implementation
    User::Leave(KErrNotSupported);
}
void COggPlayController::MapdSetBalanceL(TInt /*aBalance*/)
{
    PRINT("COggPlayController::MapdSetBalanceL");
    // No implementation
    User::Leave(KErrNotSupported);
}

void COggPlayController::MapdGetBalanceL(TInt& /*aBalance*/)
{
    PRINT("COggPlayController::MapdGetBalanceL");
    // No implementation
    User::Leave(KErrNotSupported);
}
void COggPlayController::MapcSetPlaybackWindowL(const TTimeIntervalMicroSeconds& /*aStart*/,
                            const TTimeIntervalMicroSeconds& /*aEnd*/)
{
    PRINT("COggPlayController::MapcSetPlaybackWindowL");
    // No implementation
    User::Leave(KErrNotSupported);
}
void COggPlayController::MapcDeletePlaybackWindowL()
{
    
    PRINT("COggPlayController::MapcDeletePlaybackWindowL");
    // No implementation
    User::Leave(KErrNotSupported);
}
void COggPlayController::MapcGetLoadingProgressL(TInt& /*aPercentageComplete*/)
{
    
    PRINT("COggPlayController::MapcGetLoadingProgressL");
    // No implementation
    User::Leave(KErrNotSupported);
}


TInt COggPlayController::SendEventToClient(const TMMFEvent& aEvent)
    {
    PRINT("COggPlayController::SendEventToClient");

    TRACEF(COggLog::VA(_L("Event %i %i"), aEvent.iEventType,aEvent.iErrorCode ));
    TMMFEvent myEvent = aEvent;
    if  (myEvent.iErrorCode == KErrUnderflow)
     myEvent.iErrorCode = KErrNone; // Client expect KErrNone when playing as completed correctly

    if (aEvent.iErrorCode == KErrDied)
        iState = EStateInterrupted;
    return DoSendEventToClient( myEvent );
    }

void COggPlayController::ResetL()
{
    PRINT("COggPlayController::ResetL");
    iAudioOutput=NULL;
    iDecoder->Clear();
    fclose(iFile);
    iState= EStateNotOpened;
}

void COggPlayController::PrimeL()
{
    
    PRINT("COggPlayController::PrimeL");
    if (iState == EStatePrimed) 
          return; // Nothing to do
    
    if (iState == EStateInterrupted) 
    {
        iDecoder->Clear();
        iState = EStateNotOpened;
    }

    if ( (iAudioOutput==NULL) || (iState!=EStateNotOpened) )
        User::Leave(KErrNotReady);
  
    iState = EStateOpen;
    OpenFileL(iFileName);

    iAudioOutput->SinkPrimeL();
    
    if (!iSinkBuffer)
    {
        if (!iAudioOutput->CanCreateSinkBuffer())
        {
            iSinkBuffer = CMMFDescriptorBuffer::NewL(0x4000);
            iOwnSinkBuffer = ETrue;
        }
        else
        {
            iSinkBuffer = iAudioOutput->CreateSinkBufferL(TMediaId(KUidMediaTypeAudio), iOwnSinkBuffer);
        }
        
    }
    
    iState=EStatePrimed;


    PRINT("COggPlayController::PrimeL Out");

}

void COggPlayController::PlayL()
{
    
    PRINT("COggPlayController::PlayL In");

    if (iState!=EStatePrimed)
        User::Leave(KErrNotReady);
    CFakeFormatDecode* fake = CFakeFormatDecode::NewL(
    			TFourCC(' ', 'P', '1', '6'),
    			iDecoder->Channels(), 
                iDecoder->Rate(),
    			iDecoder->Bitrate());
	CleanupStack::PushL(fake);
    
    // NegiotiateL leaves with KErrNotSupported if you don't give a CMMFFormatDecode for it's source:
    // indeed, it looks to be pretty useless if the source if not a CMMFFormatDecode. So use a 
    // fake CMMFFormatDecode class that just reports the configuration info (see CFakeFormatDecode)
	iAudioOutput->NegotiateL(*fake);
 
	CleanupStack::PopAndDestroy();
    
    //iMMFSource->SourcePlayL();

    iAudioOutput->SinkPlayL();
    
    // send first buffer to sink - sending a NULL buffer prompts it to request some data
    // the usual way

    iOggSource->SetSink(iAudioOutput);
    iAudioOutput->EmptyBufferL(NULL, iOggSource, TMediaId(KUidMediaTypeAudio));
    
    iState=EStatePlaying;
    PRINT("COggPlayController:: PlayL ok");   
}

void COggPlayController::PauseL()
{
    PRINT("COggPlayController::PauseL");
    
    if (iState!=EStatePlaying)
        User::Leave(KErrNotReady);

    //iMMFSource->SourcePauseL();
    iAudioOutput->SinkPauseL();
    iOggSource->SourcePauseL();
    iState=EStatePrimed;
}

void COggPlayController::StopL()
{
    PRINT("COggPlayController::StopL");
    
    if ((iState!=EStatePrimed)&&(iState!=EStatePlaying)) 
        User::Leave(KErrNotReady);

    iAudioOutput->SinkStopL();

    iState=EStateNotOpened;

    iDecoder->Clear();

    PRINT("COggPlayController::StopL Out");
}

TTimeIntervalMicroSeconds COggPlayController::PositionL() const
{
    //PRINT("COggPlayController::PositionL");
    
    if(iDecoder && (iState != EStateNotOpened) )
        return( TTimeIntervalMicroSeconds(iDecoder->Position( ) * 1000) );
    else
    return TTimeIntervalMicroSeconds(0);
}

void COggPlayController::SetPositionL(const TTimeIntervalMicroSeconds& aPosition)
{
    
    PRINT("COggPlayController::SetPositionL");
   
    if(iDecoder) iDecoder->Setposition( aPosition.Int64() / 1000.0);
}

TTimeIntervalMicroSeconds  COggPlayController::DurationL() const
{
    
    //PRINT("COggPlayController::DurationL");
    return (iFileLength);
}


void COggPlayController::GetNumberOfMetaDataEntriesL(TInt& aNumberOfEntries)
{
    
    PRINT("COggPlayController::GetNumberOfMetaDataEntriesL");
    aNumberOfEntries = 5;
}

CMMFMetaDataEntry* COggPlayController::GetMetaDataEntryL(TInt aIndex)
{
    PRINT("COggPlayController::GetMetaDataEntryL");
    switch(aIndex)
    {
    case 0:
        return (CMMFMetaDataEntry::NewL(_L("title"), iTitle));
        break;
        
    case 1:
        return (CMMFMetaDataEntry::NewL(_L("album"), iAlbum));
        break;
        
    case 2:
        return (CMMFMetaDataEntry::NewL(_L("artist"), iArtist));
        break;
        
    case 3:
        return (CMMFMetaDataEntry::NewL(_L("genre"), iGenre));
        break;
        
    case 4:
        return (CMMFMetaDataEntry::NewL(_L("albumtrack"),  iTrackNumber));
        break;
    }
    return NULL;
}

void COggPlayController::OpenFileL(const TDesC& aFile)
{
    
    // add a zero terminator
    TBuf<512> myname(aFile);
    myname.Append((wchar_t)0);
    
    if ((iFile=wfopen((wchar_t*)myname.Ptr(),L"rb"))==NULL) {
        iState= EStateNotOpened;
        
        TRACEF(COggLog::VA(_L("OpenFileL failed %S"), &myname ));
        User::Leave(KOggPlayPluginErrFileNotFound);
    };
    iState= EStateOpen;
    if( iDecoder->Open(iFile) < 0) {
        iDecoder->Clear();
        fclose(iFile);
        iState= EStateNotOpened;
        User::Leave(KOggPlayPluginErrOpeningFile);
    }
        
      // Parse tag information and put it in the provided buffers.
    iDecoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);
    iFileLength = iDecoder->TimeTotal() * 1000;

} 


void COggPlayController::MapdSetVolumeL(TInt aVolume)
{
    TRACEF(COggLog::VA(_L("COggPlayController::SetVolumeL %i"), aVolume ));
   
    if (!iAudioOutput) User::Leave(KErrNotReady);
    
    TInt maxVolume = iAudioOutput->SoundDevice().MaxVolume();
	if( ( aVolume < 0 ) || ( aVolume > maxVolume ))
	    User::Leave(KErrArgument);
    
    iAudioOutput->SoundDevice().SetVolume(aVolume);

}

void COggPlayController::MapdGetMaxVolumeL(TInt& aMaxVolume)
{
    PRINT("COggPlayController::MapdGetMaxVolumeL");
    
    aMaxVolume = iAudioOutput->SoundDevice().MaxVolume();
}

void COggPlayController::MapdGetVolumeL(TInt& aVolume)
{
    PRINT("COggPlayController::MapdGetVolumeL");
   
    if (!iAudioOutput) User::Leave(KErrNotReady);
    
	aVolume = iAudioOutput->SoundDevice().Volume();
}


TInt COggPlayController::GetNewSamples(TDes8 &aBuffer) 
{
    if (iState != EStatePlaying)
        return(0);

    TInt len = aBuffer.Length();
    TInt ret=iDecoder->Read(aBuffer, len);
    if (ret >0)
    {
        aBuffer.SetLength(len + ret);
    }
    if (ret == 0)
    {
        iState = EStateOpen;
        return(KErrCompletion);
    }
    return (ret);
}


/************************************************************************************
*
* COggSource
*
*************************************************************************************/
   
COggSource::COggSource(MOggSampleRateFillBuffer &aSampleRateFillBuffer) : 
  MDataSource(TUid::Uid(KOggTremorUidPlayFormatImplementation)),
  iSampleRateFillBuffer(aSampleRateFillBuffer)
{
}

COggSource::~COggSource()
{
    delete (iOggSampleRateConverter);
}

void COggSource::ConstructL()
{
    // We use the Sample Rate converter only for the buffer filling and the gain settings,
    // sample rate conversion is done by MMF.

    PRINT("COggSource::ConstructL()");
    iOggSampleRateConverter = new (ELeave) COggSampleRateConverter;
    
    iOggSampleRateConverter->Init(&iSampleRateFillBuffer, 
        KBufferSize, (TInt) (0.75*KBufferSize), 
        1, 1,   // The MMF will take care of rate conversion
        2, 2 ); // The MMF will take care of channel mixing
    PRINT("COggSource::ConstructL() Out");
}

void COggSource::SetSink(MDataSink* aSink)
    {
    iSink=aSink;
    }

void COggSource::ConstructSourceL(  const TDesC8& /*aInitData*/ )
    {
    }

CMMFBuffer* COggSource::CreateSourceBufferL(TMediaId /*aMediaId*/, TBool &aReference)
    {
    aReference = EFalse;
    return NULL;
    }

TBool COggSource::CanCreateSourceBuffer()
    {
    return EFalse;
    }

TFourCC COggSource::SourceDataTypeCode(TMediaId aMediaId)
    {
    if (aMediaId.iMediaType==KUidMediaTypeAudio)
        {
        return 1; // only support 1st stream for now
        }
    else return 0;
    }

void COggSource::FillBufferL(CMMFBuffer* aBuffer, MDataSink* aConsumer,TMediaId aMediaId)
    {
    
    if ((aMediaId.iMediaType==KUidMediaTypeAudio)&&(aBuffer->Type()==KUidMmfDescriptorBuffer))
        {
        //BufferEmptiedL(aBuffer);
        CMMFDataBuffer* db = static_cast<CMMFDataBuffer*>(aBuffer);
        if (iOggSampleRateConverter->FillBuffer(db->Data()) == KErrCompletion)
        {
            PRINT("COggSource::FillBufferL LastBuffer");
            db->SetLastBuffer(ETrue);
        }
        SetSink(aConsumer);
        aConsumer->BufferFilledL(db);
        }
    else User::Leave(KErrNotSupported);
    }


void COggSource::BufferEmptiedL(CMMFBuffer* aBuffer)
{
    if ( (aBuffer->Type()==KUidMmfDescriptorBuffer) || (aBuffer->Type()==KUidMmfTransferBuffer))
    {    
        CMMFDataBuffer* db = static_cast<CMMFDataBuffer*>(aBuffer);
        if (iOggSampleRateConverter->FillBuffer(db->Data()) == KErrCompletion)
        {
            PRINT("COggSource::BufferEmptiedL Last Buffer");
            db->SetLastBuffer(ETrue);
        }
        iSink->EmptyBufferL(db, this, TMediaId(KUidMediaTypeAudio));
    }
    else User::Leave(KErrNotSupported);
}


/************************************************************************************
*
* CFakeFormatDecode
*
*************************************************************************************/

CFakeFormatDecode* CFakeFormatDecode::NewL(TFourCC aFourCC, TUint aChannels, TUint aSampleRate, TUint aBitRate)
	{
	CFakeFormatDecode* self = new(ELeave) CFakeFormatDecode;
	self->iFourCC = aFourCC;
	self->iChannels = aChannels;
	self->iSampleRate = aSampleRate;
	self->iBitRate = aBitRate;
	return self;
	}
	
CFakeFormatDecode::CFakeFormatDecode()
	{
	}
    
CFakeFormatDecode::~CFakeFormatDecode()
	{
	}
    
TUint CFakeFormatDecode::Streams(TUid /*aMediaType*/) const
	{
	User::Panic(KFakeFormatDecodePanic, 1);
	return 0;
	}
		
TTimeIntervalMicroSeconds CFakeFormatDecode::FrameTimeInterval(TMediaId /*aMediaType*/) const
	{
	User::Panic(KFakeFormatDecodePanic, 2);
	return TTimeIntervalMicroSeconds(0);
	}
	
TTimeIntervalMicroSeconds CFakeFormatDecode::Duration(TMediaId /*aMediaType*/) const
	{
	User::Panic(KFakeFormatDecodePanic, 3);
	return TTimeIntervalMicroSeconds(0);
	}
		
void CFakeFormatDecode::FillBufferL(CMMFBuffer* /*aBuffer*/, MDataSink* /*aConsumer*/, TMediaId /*aMediaId*/)
	{
	User::Panic(KFakeFormatDecodePanic, 4);
	}
	
CMMFBuffer* CFakeFormatDecode::CreateSourceBufferL(TMediaId /*aMediaId*/, TBool& /*aReference*/)
	{
	User::Panic(KFakeFormatDecodePanic, 4);
	return NULL;
	}
	
TFourCC CFakeFormatDecode::SourceDataTypeCode(TMediaId /*aMediaId*/)
	{
	return iFourCC;
	}
	
TUint CFakeFormatDecode::NumChannels()
	{
	return iChannels;
	}
	
TUint CFakeFormatDecode::SampleRate()
	{
	return iSampleRate;
	}
		
TUint CFakeFormatDecode::BitRate()
	{
	return iBitRate;
	}
