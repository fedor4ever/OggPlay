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
#include <stdlib.h>
#include "OggLog.h"
#include "Plugin\ImplementationUIDs.hrh"
#include "Plugin\OggPlayControllerCustomCommands.h"

#if 1
#include <e32svr.h> 
#define PRINT(x) TRACEF(_L(x));
//#define PRINT(x) RDebug::Print _L(x);
#else
#define PRINT()
#endif

// Buffer sizes to use
// (approx. 0.1s of audio per buffer)
const TInt KBufferSize48K = 16384;
const TInt KBufferSize32K = 12288;
const TInt KBufferSize22K = 8192;
const TInt KBufferSize16K = 6144;
const TInt KBufferSize11K = 4096;

COggPlayController* COggPlayController::NewL(RFs* aFs, MDecoder *aDecoder)
{
    PRINT("COggPlayController::NewL()");

	// Take ownership of the file session and the decoder
    COggPlayController* self = new COggPlayController(aFs, aDecoder);
	if (!self)
	{
		delete aDecoder;

		aFs->Close();
		delete aFs;

		User::Leave(KErrNoMemory);
	}

    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
}

COggPlayController::COggPlayController(RFs* aFs, MDecoder *aDecoder)
: iFs(aFs), iDecoder(aDecoder), iStreamError(KErrNotReady)
{
}

COggPlayController::~COggPlayController()
{
    PRINT("COggPlayController::~COggPlayController In");

	iState = EStateDestroying;

	delete iStream;
    if (iOwnSinkBuffer) 
        delete iSinkBuffer;

	delete iOggSource;

	if (iAudioOutput)
        iAudioOutput->SinkThreadLogoff();

	iDecoder->Clear();
	if (iFile)
		iFile->Close();

	delete iFile;
	delete iDecoder;

	iFs->Close();
	delete iFs;

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
     
    iFileLength = TTimeIntervalMicroSeconds(5*60*1E6); // We do not want to look at file duration when
                                                       //discovering the file: it takes too long
                                                       // If DurationL is asked before we actually have the information,
                                                       // pretend length is 5 minutes.
    
    iOggSource = new(ELeave) COggSource(*this);

	// Open an audio stream (used for determining audio capabilities)
	iStream = CMdaAudioOutputStream::NewL(*this);
	iStream->Open(&iSettings);

	PRINT("COggPlayController::ConstructL() Out");
}

_LIT(KRandomRingingToneFileName, "random_ringing_tone.ogg");
_LIT(KRandomRingingToneTitle, "this is a random ringing tone");
void COggPlayController::AddDataSourceL(MDataSource& aDataSource)
{
    PRINT("COggPlayController::AddDataSourceL In");

    if ( iState != EStateNotOpened )
    {
        User::Leave(KOggPlayPluginErrNotReady);
    }

    iRandomRingingTone = EFalse;
    TUid uid = aDataSource.DataSourceType() ;
    if (uid == KUidMmfFileSource)
    {
        CMMFFile* aFile = STATIC_CAST(CMMFFile*, &aDataSource);
        iFileName = aFile->FullName();

	    TParsePtrC aP(iFileName);
        TBool result = aP.NameAndExt().CompareF(KRandomRingingToneFileName) == 0;
        if ( result )
        {
            // Use a random file, for ringing tones
            TFileName aPathToSearch = aP.Drive();
            aPathToSearch.Append ( aP.Path() );
            CDir* dirList;
            User::LeaveIfError(iFs->GetDir(aPathToSearch,
                KEntryAttMaskSupported,ESortByName,dirList));
            TInt nbFound=0;
            TInt i; 
            for (i=0;i<dirList->Count();i++)
            {
                TParsePtrC aParser((*dirList)[i].iName);
                TBool found = ( aParser.Ext().CompareF(_L(".ogg")) == 0 );
                TBool theRandomTone = ( ( (*dirList)[i].iName).CompareF(KRandomRingingToneFileName) == 0);
                if (found && !theRandomTone)
                    nbFound++;
            }
            if (nbFound == 0)
            {
                // Only the random_ringing_tone.ogg in that directory, leave
                User::Leave(KErrNotFound);
            }

			TInt64 rnd64 = TInt64(0, Math::Random());
			TInt64 maxInt64 = TInt64(1, 0);
			TInt64 nbFound64 = nbFound;
			TInt64 picked64 = (rnd64 * nbFound64) / maxInt64;
			TInt picked = picked64.Low();

            nbFound=-1;
            for (i=0;i<dirList->Count();i++)
            {
                TParsePtrC aParser((*dirList)[i].iName);
                TBool found = ( aParser.Ext().CompareF(_L(".ogg")) == 0 ); 
                TBool theRandomTone = ( ( (*dirList)[i].iName).CompareF(KRandomRingingToneFileName) == 0);
                if (found && !theRandomTone)
                    nbFound++;
                if (nbFound == picked)
                    break;
            }
            iFileName = aPathToSearch;
            iFileName.Append ( (*dirList)[i].iName );
            
            TRACEF(COggLog::VA(_L("Random iFileName choosen %S "),&iFileName ));
            delete dirList;
            iRandomRingingTone = ETrue;
        }
    }
    else
    {
        User::Leave(KOggPlayPluginErrNotSupported);
    }

    
    TRACEF(COggLog::VA(_L("iFileName %S "),&iFileName ));
    // Open the file here, in order to get the tags.
    OpenFileL(iFileName,ETrue);
    // Save the tags.
    
  iDecoder->Clear(); // Close the file. This is required for the ringing tone stuff.

  if (iFile)
	iFile->Close();
  
  delete iFile;
  iFile = NULL;
  
  iState = EStateNotOpened;
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

	if (iFile)
		iFile->Close();

    delete iFile;
	iFile = NULL;

	iState= EStateNotOpened;
}

TBool COggPlayController::GetNextLowerRate(TInt& usedRate, TMdaAudioDataSettings::TAudioCaps& rt)
{
  TBool retValue = ETrue;
  switch (usedRate)
  {
	case 48000:
		usedRate = 44100;
		rt = TMdaAudioDataSettings::ESampleRate44100Hz;
		break;

	case 44100:
		usedRate = 32000;
		rt = TMdaAudioDataSettings::ESampleRate32000Hz;
		break;

	case 32000:
		usedRate = 22050;
		rt = TMdaAudioDataSettings::ESampleRate22050Hz;
		break;

	case 22050:
		usedRate = 16000;
		rt = TMdaAudioDataSettings::ESampleRate16000Hz;
		break;

	case 16000:
		usedRate = 11025;
		rt = TMdaAudioDataSettings::ESampleRate11025Hz;
		break;

	case 11025:
		usedRate = 8000;
		rt = TMdaAudioDataSettings::ESampleRate8000Hz;
		break;

	default:
		retValue = EFalse;
		break;
  }

  return retValue;
}

void COggPlayController::SetAudioCapsL(TInt theRate, TInt theChannels)
{
  TMdaAudioDataSettings::TAudioCaps ac(TMdaAudioDataSettings::EChannelsMono);
  TMdaAudioDataSettings::TAudioCaps rt(TMdaAudioDataSettings::ESampleRate8000Hz);
  TBool convertChannel = EFalse;
  TBool convertRate = EFalse;

  iUsedRate = theRate;
  if (theRate==8000) rt= TMdaAudioDataSettings::ESampleRate8000Hz;
  else if (theRate==11025) rt= TMdaAudioDataSettings::ESampleRate11025Hz;
  else if (theRate==16000) rt= TMdaAudioDataSettings::ESampleRate16000Hz;
  else if (theRate==22050) rt= TMdaAudioDataSettings::ESampleRate22050Hz;
  else if (theRate==32000) rt= TMdaAudioDataSettings::ESampleRate32000Hz;
  else if (theRate==44100) rt= TMdaAudioDataSettings::ESampleRate44100Hz;
  else if (theRate==48000) rt= TMdaAudioDataSettings::ESampleRate48000Hz;
  else
  {
      // Rate not supported by the phone
	  TRACEF(COggLog::VA(_L("SetAudioCaps: Non standard rate: %d"), theRate));

	  // Convert to nearest rate
      convertRate = ETrue;
	  if (theRate>48000)
	  {
		  iUsedRate = 48000;
	      rt = TMdaAudioDataSettings::ESampleRate48000Hz;
	  }
	  else if (theRate>44100)
	  {
		  iUsedRate = 44100;
	      rt = TMdaAudioDataSettings::ESampleRate44100Hz;
	  }
	  else if (theRate>32000)
	  {
		  iUsedRate = 32000;
	      rt = TMdaAudioDataSettings::ESampleRate32000Hz;
	  }
	  else if (theRate>22050)
	  {
		  iUsedRate = 22050;
	      rt = TMdaAudioDataSettings::ESampleRate22050Hz;
	  }
	  else if (theRate>16000)
	  {
		  iUsedRate = 16000;
	      rt = TMdaAudioDataSettings::ESampleRate16000Hz;
	  }
	  else if (theRate>11025)
	  {
		  iUsedRate = 11025;
	      rt = TMdaAudioDataSettings::ESampleRate11025Hz;
	  }
	  else if (theRate>8000)
	  {
		  iUsedRate = 8000;
	      rt = TMdaAudioDataSettings::ESampleRate8000Hz;
	  }
	  else
	  {
		  // Frequencies less than 8KHz are not supported
	      // iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_12);
		  User::Leave(KErrNotSupported);
	  }
  }

  iUsedChannels = theChannels;
  if (iUsedChannels == 1)
	  ac = TMdaAudioDataSettings::EChannelsMono;
  else if (iUsedChannels == 2)
	  ac = TMdaAudioDataSettings::EChannelsStereo;
  else
  {
    // iEnv->OggErrorMsgL(R_OGG_ERROR_12, R_OGG_ERROR_10);
	User::Leave(KErrNotSupported);
  }

  // Note current settings
  TInt bestRate = iUsedRate;
  TInt convertingRate = convertRate;
  TMdaAudioDataSettings::TAudioCaps bestRT = rt;

  // Try the current settings.
  // Adjust sample rate and channels if necessary
  TInt err = KErrNotSupported;
  while (err == KErrNotSupported)
  {
	TRAP(err, iStream->SetAudioPropertiesL(rt, ac));
	if (err == KErrNotSupported)
	{
		// Frequency is not supported
		// Try dropping the frequency
		convertRate = GetNextLowerRate(iUsedRate, rt);

		// If that doesn't work, try changing stereo to mono
		if (!convertRate && (iUsedChannels == 2))
		{
			// Reset the sample rate
			convertRate = convertingRate;
			iUsedRate = bestRate;
			rt = bestRT;

			// Drop channels to 1
			iUsedChannels = 1;
			ac = TMdaAudioDataSettings::EChannelsMono;
			convertChannel = ETrue;
		}
		else if (!convertRate)
			break; // Give up, nothing supported :-(
	}
  }

  // TBuf<256> buf,tbuf;
  if (err != KErrNone)
  {
	/* CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_16);
	CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_12);
	buf.AppendNum(err);
	iEnv->OggErrorMsgL(tbuf,buf); */

	User::Leave(err);
  }

  // Trace the settings
  TRACEF(COggLog::VA(_L("SetAudioCaps: theRate: %d, theChannels: %d, usedRate: %d, usedChannels: %d"), theRate, theChannels, iUsedRate, iUsedChannels));

  /* if ((convertRate || convertChannel) && iEnv->WarningsEnabled())
  {
	  // Display a warning message
      SamplingRateSupportedMessage(convertRate, theRate, convertChannel, theChannels);

	  // Put the audio properties back the way they were 
	  #if defined(MULTI_THREAD_PLAYBACK)
	  err = SetAudioProperties(usedRate, usedChannels);
	  #else
	  TRAP(err, iStream->SetAudioPropertiesL(rt, ac));
	  #endif

	  // Check that it worked (it should have)
	  if (err != KErrNone)
	  {
		CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_16);
		CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_12);
		buf.AppendNum(err);
		iEnv->OggErrorMsgL(tbuf,buf);

		return err;
	  }
  } */

  // Determine buffer sizes so that we make approx 10-15 calls to the mediaserver per second
  // + another 10-15 calls for position if the frequency analyzer is on screen
  TInt bufferSize = 0;
  switch (iUsedRate)
  {
	case 48000:
	case 44100:
		bufferSize = KBufferSize48K;
		break;

	case 32000:
		bufferSize = KBufferSize32K;
		break;

	case 22050:
		bufferSize = KBufferSize22K;
		break;

	case 16000:
		bufferSize = KBufferSize16K;
		break;

	case 11025:
	case 8000:
		bufferSize = KBufferSize11K;
		break;

	default:
		User::Panic(_L("COggControl:SAC"), 0);
		break;
  }

  if (iUsedChannels == 1)
	  bufferSize /= 2;

  iOggSource->ConstructL(bufferSize, theRate, iUsedRate, theChannels, iUsedChannels);
}

void COggPlayController::PrimeL()
{
    PRINT("COggPlayController::PrimeL");
    if (iState == EStatePrimed) 
          return; // Nothing to do
    
    if (iState == EStateInterrupted) 
    {
        iDecoder->Clear();

		if (iFile)
			iFile->Close();

		delete iFile;
		iFile = NULL;

		iState = EStateNotOpened;
    }

    if ( (iAudioOutput==NULL) || (iState!=EStateNotOpened) )
        User::Leave(KErrNotReady);
  
    iState = EStateOpen;
    OpenFileL(iFileName,EFalse);

    // Do our own rate conversion, some firmware do it very badly.
    if (iUsedRate == 0 && (iStreamError == KErrNone))
		SetAudioCapsL(iDecoder->Rate(), iDecoder->Channels());
	else if (iStreamError != KErrNone)
		User::Leave(iStreamError);

    iAudioOutput->SinkPrimeL();
    if (!iSinkBuffer)
    {
        if (!iAudioOutput->CanCreateSinkBuffer())
        {
            iSinkBuffer = CMMFDescriptorBuffer::NewL(KBufferSize48K);
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
    
	// Clear the frequency analyser
	Mem::FillZ(&iFreqArray, sizeof(iFreqArray));

    CFakeFormatDecode* fake = CFakeFormatDecode::NewL(
    			TFourCC(' ', 'P', '1', '6'),
    			iUsedChannels, 
                iUsedRate,
    			iDecoder->Bitrate());

	CleanupStack::PushL(fake);
    
    // NegiotiateL leaves with KErrNotSupported if you don't give a CMMFFormatDecode for it's source:
    // indeed, it looks to be pretty useless if the source if not a CMMFFormatDecode. So use a 
    // fake CMMFFormatDecode class that just reports the configuration info (see CFakeFormatDecode)
	iAudioOutput->NegotiateL(*fake);
	CleanupStack::PopAndDestroy();
    
    iAudioOutput->SinkPlayL();
    
    // send first buffer to sink - sending a NULL buffer prompts it to request some data the usual way
    iOggSource->SetSink(iAudioOutput);
    iAudioOutput->EmptyBufferL(NULL, iOggSource, TMediaId(KUidMediaTypeAudio));

    iState=EStatePlaying;
    PRINT("COggPlayController:: PlayL ok");
}

void COggPlayController::CustomCommand( TMMFMessage& aMessage )
    {   
    if ( aMessage.Destination().InterfaceId().iUid != KOggTremorUidControllerImplementation )
        {
        aMessage.Complete(KErrNotSupported);    
        return;
        }
    TInt error = KErrNone;
    switch ( aMessage.Function() )
        {
        case EOggPlayControllerCCGetFrequencies:
            {
            TRAP( error, GetFrequenciesL( aMessage ) );
            break;
            }

        case EOggPlayControllerCCSetVolumeGain:
            {
            TRAP(error, SetVolumeGainL(aMessage));
            break;
			}

		default:
            {
            error = KErrNotSupported;
            break;
            }
        }
    aMessage.Complete(error);  
    }

void COggPlayController::PauseL()
{
    PRINT("COggPlayController::PauseL");
    
    if (iState!=EStatePlaying)
        User::Leave(KErrNotReady);

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

	if (iFile)
		iFile->Close();

	delete iFile;
	iFile = NULL;

    PRINT("COggPlayController::StopL Out");
}

TTimeIntervalMicroSeconds COggPlayController::PositionL() const
{
    if(iDecoder && (iState != EStateNotOpened) )
        return( TTimeIntervalMicroSeconds(iDecoder->Position( ) * 1000) );
    else
	 return TTimeIntervalMicroSeconds(0);
}

void COggPlayController::SetPositionL(const TTimeIntervalMicroSeconds& aPosition)
{
    PRINT("COggPlayController::SetPositionL");

	// const TInt64 KConst500 = TInt64(500);
	TInt64 positionMilliseconds = aPosition.Int64() / 1000.0;
    if (iDecoder)
		iDecoder->Setposition(positionMilliseconds);

	if (iOggSource)
		{
		// iOggSource->iTotalBufferBytes = (positionMilliseconds*TInt64(iUsedRate*iUsedChannels))/KConst500;
		iOggSource->iTotalBufferBytes = 0;
		}
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

void COggPlayController::OpenFileL(const TDesC& aFile, TBool aOpenForInfo)
{
	iFile = new(ELeave) RFile;
    if (iFile->Open(*iFs, aFile, EFileShareReadersOnly) != KErrNone)
	{
        iState= EStateNotOpened;

		delete iFile;
		iFile = NULL;
        
        TRACEF(COggLog::VA(_L("OpenFileL failed %S"), &aFile ));
        User::Leave(KOggPlayPluginErrFileNotFound);
    }

    iState= EStateOpen;
    TInt ret=0;
    if (aOpenForInfo)
    {
        ret = iDecoder->OpenInfo(iFile) ;
    }
    else
    {
        ret = iDecoder->Open(iFile) ;
    }
    if( ret < 0) {
        iDecoder->Clear();
		iFile->Close();

		delete iFile;
		iFile = NULL;

		iState= EStateNotOpened;
        User::Leave(KOggPlayPluginErrOpeningFile);
    }
        
    // Parse tag information and put it in the provided buffers.
    iDecoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);

    // Change the title, to let know the client that we have a random ringing tone
    if (iRandomRingingTone)
    {
        iTitle = KRandomRingingToneTitle;
    }

    TRACEF(COggLog::VA(_L("Tags: %S %S %S %S"), &iTitle, &iArtist, &iAlbum, &iGenre ));
    if (!aOpenForInfo)
         iFileLength = iDecoder->TimeTotal() * 1000;
} 

void COggPlayController::MapdSetVolumeRampL(const TTimeIntervalMicroSeconds& aRampDuration)
{
    PRINT("COggPlayController::MapdSetVolumeRampL");
    if (!iAudioOutput) User::Leave(KErrNotReady);

	iAudioOutput->SoundDevice().SetVolumeRamp(aRampDuration);
}

void COggPlayController::MapdSetBalanceL(TInt aBalance)
{
    PRINT("COggPlayController::MapdSetBalanceL");
    if (!iAudioOutput) User::Leave(KErrNotReady);

    if (aBalance < KMMFBalanceMaxLeft)
		aBalance = KMMFBalanceMaxLeft;

	if (aBalance > KMMFBalanceMaxRight)
		aBalance = KMMFBalanceMaxRight;
	
	TInt balanceRange = KMMFBalanceMaxRight-KMMFBalanceMaxLeft;
	TInt leftBalance = (100*(KMMFBalanceMaxRight - aBalance))/balanceRange;
    iAudioOutput->SoundDevice().SetPlayBalanceL(leftBalance, 100-leftBalance);
}

void COggPlayController::MapdGetBalanceL(TInt& aBalance)
{
    PRINT("COggPlayController::MapdGetBalanceL");
    if (!iAudioOutput) User::Leave(KErrNotReady);

	// Initialise values to KMMFBalanceCenter
	TInt leftBalance = 50;
	TInt rightBalance = 50;

	// Read the balance from devsound
    iAudioOutput->SoundDevice().GetPlayBalanceL(leftBalance, rightBalance);

	// Calculate the balance, assuming that leftBalance+rightBalance = 100
	TInt balanceRange = KMMFBalanceMaxRight-KMMFBalanceMaxLeft;
	aBalance = KMMFBalanceMaxLeft + (balanceRange*rightBalance)/100;
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
    if (!iAudioOutput) User::Leave(KErrNotReady);
    
    aMaxVolume = iAudioOutput->SoundDevice().MaxVolume();
}

void COggPlayController::MapdGetVolumeL(TInt& aVolume)
{
    PRINT("COggPlayController::MapdGetVolumeL");
    if (!iAudioOutput) User::Leave(KErrNotReady);
    
	aVolume = iAudioOutput->SoundDevice().Volume();
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

TInt COggPlayController::GetNewSamples(TDes8 &aBuffer, TBool aRequestFrequencyBins) 
{
    if (iState != EStatePlaying)
        return 0;

	if (aRequestFrequencyBins && !iRequestingFrequencyBins)
	{
		// Make a request for frequency data
		iDecoder->GetFrequencyBins(iFreqArray[iLastFreqArrayIdx].iFreqCoefs, KNumberOfFreqBins);

		// Mark that we have issued the request
		iRequestingFrequencyBins = ETrue;
	}

	TInt len = aBuffer.Length();
    TInt ret=iDecoder->Read(aBuffer, len);
    if (ret >0)
    {
        aBuffer.SetLength(len + ret);

		if (iRequestingFrequencyBins)
		{
			// Determine the status of the request
			TInt requestingFrequencyBins = iDecoder->RequestingFrequencyBins();
			if (!requestingFrequencyBins)
			{
				iLastFreqArrayIdx++;
				if (iLastFreqArrayIdx >= KFreqArrayLength)
					iLastFreqArrayIdx = 0;

				// The frequency request has completed
				iFreqArray[iLastFreqArrayIdx].iTime = iOggSource->iTotalBufferBytes;
				iRequestingFrequencyBins = EFalse;
			}
		}
	}

	if (ret == 0)
    {
        iState = EStateOpen;
        return KErrCompletion;
    }

	return ret;
}

void  COggPlayController::GetFrequenciesL(TMMFMessage& aMessage )
{
   	// Get the size of the init data and create a buffer to hold it
    TInt desLength = aMessage.SizeOfData1FromClient();
    HBufC8* buf = HBufC8::NewLC(desLength);
    TPtr8 ptr = buf->Des();
    aMessage.ReadData1FromClientL(ptr);
    
    TMMFGetFreqsParams params;
    TPckgC<TMMFGetFreqsParams> config(params);
    config.Set(*buf);
    params = config();

	TInt64 positionBytes = 2*iUsedChannels*iAudioOutput->SoundDevice().SamplesPlayed();
	TInt idx = iLastFreqArrayIdx;
	for (TInt i=0; i<KFreqArrayLength; i++)
    {
		if (iFreqArray[idx].iTime <= positionBytes)
			break;
      
		idx--;
		if (idx < 0)
			idx = KFreqArrayLength-1;
    }

	TPckg<TInt32[16]> binsPckg(iFreqArray[idx].iFreqCoefs);
    aMessage.WriteDataToClient(binsPckg);

    CleanupStack::PopAndDestroy(buf);
}

void  COggPlayController::SetVolumeGainL(TMMFMessage& aMessage)
{
   	// Get the size of the init data and create a buffer to hold it
    TInt desLength = aMessage.SizeOfData1FromClient();
    HBufC8* buf = HBufC8::NewLC(desLength);
    TPtr8 ptr = buf->Des();
    aMessage.ReadData1FromClientL(ptr);
    
    TMMFSetVolumeGainParams params;
    TPckgC<TMMFSetVolumeGainParams> config(params);
    config.Set(*buf);
    params = config();
    
	iOggSource->SetVolumeGain((TGainType) params.iGain);
    CleanupStack::PopAndDestroy(buf);//buf
}

void COggPlayController::MaoscOpenComplete(TInt aError)
{
	iStreamError = aError;
}

void COggPlayController::MaoscBufferCopied(TInt /* aError */, const TDesC8& /* aBuffer */)
{
}

void COggPlayController::MaoscPlayComplete(TInt /* aError */)
{
}

// COggSource
COggSource::COggSource(MOggSampleRateFillBuffer &aSampleRateFillBuffer)
: MDataSource(TUid::Uid(KOggTremorUidPlayFormatImplementation)), iSampleRateFillBuffer(aSampleRateFillBuffer), iGain(ENoGain)
{
}

COggSource::~COggSource()
{
    delete iOggSampleRateConverter;
}

void COggSource::ConstructL(TInt aBufferSize, TInt aInputRate, TInt aOutputRate, TInt aInputChannel, TInt aOutputChannel)
{
    // We use the Sample Rate converter only for the buffer filling and the gain settings,
    // sample rate conversion is done by MMF.
    PRINT("COggSource::ConstructL()");
    iOggSampleRateConverter = new(ELeave) COggSampleRateConverter;
    
    iOggSampleRateConverter->Init(&iSampleRateFillBuffer, aBufferSize, aBufferSize-1024,
	aInputRate,  aOutputRate, aInputChannel, aOutputChannel);

	iOggSampleRateConverter->SetVolumeGain(iGain);
    PRINT("COggSource::ConstructL() Out");
}

void COggSource::SetVolumeGain(TGainType aGain)
{
	iGain = aGain;
	if (iOggSampleRateConverter)
		iOggSampleRateConverter->SetVolumeGain(aGain);
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
		// only support 1st stream for now
        return 1;
        }
    else
		return 0;
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
		iTotalBufferBytes += db->Data().Length();

		SetSink(aConsumer);
        aConsumer->BufferFilledL(db);
    }
    else
		User::Leave(KErrNotSupported);
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

		iTotalBufferBytes += db->Data().Length();
        iSink->EmptyBufferL(db, this, TMediaId(KUidMediaTypeAudio));
    }
    else User::Leave(KErrNotSupported);
}


// CFakeFormatDecode
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
