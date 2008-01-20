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

#include <e32svr.h> 
#include <charconv.h>
#include <utf.h>

#include <OggOs.h>

#if defined(SERIES60V3)
#include "S60V3ImplementationUIDs.hrh"
#else
#include "ImplementationUIDs.hrh"
#endif

#include "OggLog.h"
#include "OggPlayController.h"
#include "OggPlayControllerCustomCommands.h"

#include "TremorDecoder.h"
#include "FlacDecoder.h"

#if defined(MP3_SUPPORT)
#include "MadDecoder.h"
#endif

// Nokia phones don't seem to like large buffers, so just use one size (4K)
#define USE_FIXED_SIZE_BUFFERS

// Some of the old Nokias want to lock the file
// Enable this macro to compile in a workaround
// #define MUSIC_PLAYER_REQUIRES_EXCLUSIVE_ACCESS

// Some of them also don't like being initially told that the duration is 0
// Older versions of OggPlay would just lie and return 5:00, but I don't really like that solution
// Enable this macro to obtain the duration when the data source is added
// #define MUSIC_PLAYER_REQUIRES_DURATION

// Buffer sizes to use
// (approx. 0.1s of audio per buffer)
#if defined(USE_FIXED_SIZE_BUFFERS)
const TInt KBufferSize48K = 4096;
const TInt KBufferSize32K = 4096;
const TInt KBufferSize22K = 4096;
const TInt KBufferSize16K = 4096;
const TInt KBufferSize11K = 4096;
#else
const TInt KBufferSize48K = 16384;
const TInt KBufferSize32K = 12288;
const TInt KBufferSize22K = 8192;
const TInt KBufferSize16K = 6144;
const TInt KBufferSize11K = 4096;
#endif

COggPlayController* COggPlayController::NewL()
	{
	// Take ownership of the file session and the decoder
    COggPlayController* self = new(ELeave) COggPlayController();
    CleanupStack::PushL(self);
    self->ConstructL();

    CleanupStack::Pop();
    return self;
}

COggPlayController::COggPlayController()
	{
	}

COggPlayController::~COggPlayController()
	{
	iState = EStateDestroying;

	delete iStream;
	delete iStreamMessage;

	if (iAudioOutput)
        iAudioOutput->SinkThreadLogoff();

	if (iDecoder)
		iDecoder->Clear();

	delete iOggSource;
	delete iDecoder;

	iFs.Close();
    COggLog::Exit();
	}

void COggPlayController::ConstructL()
	{
	User::LeaveIfError(iFs.Connect());
		
	// Construct custom command parsers
    CMMFAudioPlayDeviceCustomCommandParser* audPlayDevParser = CMMFAudioPlayDeviceCustomCommandParser::NewL(*this);
    CleanupStack::PushL(audPlayDevParser);

    AddCustomCommandParserL(*audPlayDevParser); // parser now owned by controller framework
    CleanupStack::Pop(audPlayDevParser);

    CMMFAudioPlayControllerCustomCommandParser* audPlayCtrlParser = CMMFAudioPlayControllerCustomCommandParser::NewL(*this);
    CleanupStack::PushL(audPlayCtrlParser);

	AddCustomCommandParserL(*audPlayCtrlParser); // parser now owned by controller framework
    CleanupStack::Pop(audPlayCtrlParser);

    iState = EStateNotOpened;
    iOggSource = new(ELeave) COggSource(*this);

	// Open an audio stream (used for determining audio capabilities)
	iStream = CMdaAudioOutputStream::NewL(*this);
	iStream->Open(&iSettings);
	}

_LIT(KRandomRingingToneFileName, "random_ringing_tone.ogg");
_LIT(KRandomRingingToneTitle, "this is a random ringing tone");
void COggPlayController::AddDataSourceL(MDataSource& aDataSource)
	{
    if (iState != EStateNotOpened)
        User::Leave(KErrNotReady);

    iRandomRingingTone = EFalse;
    TUid uid = aDataSource.DataSourceType();
    if (uid == KUidMmfFileSource)
		{
        CMMFFile* aFile = STATIC_CAST(CMMFFile*, &aDataSource);
        iFileName = aFile->FullName();

	    TParsePtrC aP(iFileName);
        TBool result = aP.NameAndExt().CompareF(KRandomRingingToneFileName) == 0;
        if (result)
			{
            // Use a random file, for ringing tones
            TFileName aPathToSearch = aP.Drive();
            aPathToSearch.Append(aP.Path());
            CDir* dirList;
            User::LeaveIfError(iFs.GetDir(aPathToSearch, KEntryAttMaskSupported, ESortByName, dirList));

			TInt nbFound=0;
            TInt i; 
            for (i = 0 ; i<dirList->Count() ;i++)
				{
                TParsePtrC aParser((*dirList)[i].iName);
                TBool found = (aParser.Ext().CompareF(_L(".ogg")) == 0);
                TBool theRandomTone = (((*dirList)[i].iName).CompareF(KRandomRingingToneFileName) == 0);
                if (found && !theRandomTone)
                    nbFound++;
				}

			if (nbFound == 0)
				{
                // Only the random_ringing_tone.ogg in that directory, leave
                User::Leave(KErrNotFound);
				}

#if defined(SERIES60V3)
			TInt64 rnd64 = Math::Random();
			TInt64 max64 = MAKE_TINT64(1, 0);

			TInt64 nbFound64 = nbFound;
			TInt64 picked64 = (rnd64 * nbFound64) / max64;
			TInt picked = (TInt) picked64;
#else
			TInt64 rnd64 = TInt64(0, Math::Random());
			TInt64 max64 = TInt64(1, 0);

			TInt64 nbFound64 = nbFound;
			TInt64 picked64 = (rnd64 * nbFound64) / max64;
			TInt picked = picked64.GetTInt();
#endif

            nbFound = -1;
            for (i = 0 ; i<dirList->Count() ; i++)
				{
                TParsePtrC aParser((*dirList)[i].iName);
                TBool found = (aParser.Ext().CompareF(_L(".ogg")) == 0); 
                TBool theRandomTone = (((*dirList)[i].iName).CompareF(KRandomRingingToneFileName) == 0);
                if (found && !theRandomTone)
                    nbFound++;

                if (nbFound == picked)
                    break;
				}

            iFileName = aPathToSearch;
            iFileName.Append((*dirList)[i].iName);
            
            TRACEF(COggLog::VA(_L("Random iFileName choosen %S "), &iFileName));
            delete dirList;

            iRandomRingingTone = ETrue;
			}
		}
    else
        User::Leave(KErrNotSupported);
    
	TRACEF(COggLog::VA(_L("OggPlayController::iFileName %S "), &iFileName));

	// Open the file here (so we can validate the source and get the tags)
    OpenFileL();

#if defined(MUSIC_PLAYER_REQUIRES_DURATION)
	// For some inexplicable reason the MMF asks for the duration of the file (by calling DurationL()) when the file is opened.
	// Unfortunately at that point we don't know the duration, so we just return 0.
	// On the N70 and E60 this doesn't cause any problems, but on some of the older Nokias...

	// Complete the open operation to get the duration
	// Note that this will probably make "Find files" run a fair bit slower, unfortunately
	OpenCompleteL();
#endif

#if defined(MUSIC_PLAYER_REQUIRES_EXCLUSIVE_ACCESS)
	// Some of the old Nokias want to lock the file (heaven knows why)
	// To get around this we close the file (and re-open it later)
	iDecoder->Clear();

	delete iDecoder;
	iDecoder = NULL;

	iState = EStateNotOpened;
#endif
	}

void COggPlayController::AddDataSinkL(MDataSink& aDataSink)
	{
    if (aDataSink.DataSinkType() != KUidMmfAudioOutput)
        User::Leave(KErrNotSupported);

	if (iAudioOutput)
        User::Leave(KErrNotSupported);

    iAudioOutput = static_cast<CMMFAudioOutput*>(&aDataSink);
    iAudioOutput->SinkThreadLogon(*this);

    iAudioOutput->SetSinkPrioritySettings(iMMFPrioritySettings);
	}

void COggPlayController::RemoveDataSourceL(MDataSource& /*aDataSource*/)
	{
	}

void COggPlayController::RemoveDataSinkL(MDataSink&  aDataSink)
	{
    if (iState==EStatePlaying)
        User::Leave(KErrNotReady);

    if (iAudioOutput==&aDataSink) 
        iAudioOutput = NULL;
	}

void COggPlayController::SetPrioritySettings(const TMMFPrioritySettings& aPrioritySettings)
	{ 
    iMMFPrioritySettings = aPrioritySettings;
    if (iAudioOutput)
        iAudioOutput->SetSinkPrioritySettings(aPrioritySettings);
	}

TInt COggPlayController::SendEventToClient(const TMMFEvent& aEvent)
    {
    TRACEF(COggLog::VA(_L("Event %i %i"), aEvent.iEventType,aEvent.iErrorCode));
    TMMFEvent myEvent = aEvent;
    if  (myEvent.iErrorCode == KErrUnderflow)
		myEvent.iErrorCode = KErrNone; // Client expect KErrNone when playing as completed correctly

    if (aEvent.iErrorCode == KErrDied)
        iState = EStateInterrupted;

	return DoSendEventToClient(myEvent);
    }

void COggPlayController::ResetL()
	{
    iAudioOutput = NULL;
	iDecoder->Clear();

	delete iDecoder;
	iDecoder = NULL;

	iState = EStateNotOpened;
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
	if (theRate==8000)
		rt= TMdaAudioDataSettings::ESampleRate8000Hz;
	else if (theRate==11025)
		rt= TMdaAudioDataSettings::ESampleRate11025Hz;
	else if (theRate==16000)
		rt= TMdaAudioDataSettings::ESampleRate16000Hz;
	else if (theRate==22050)
		rt= TMdaAudioDataSettings::ESampleRate22050Hz;
	else if (theRate==32000)
		rt= TMdaAudioDataSettings::ESampleRate32000Hz;
	else if (theRate==44100)
		rt= TMdaAudioDataSettings::ESampleRate44100Hz;
	else if (theRate==48000)
		rt= TMdaAudioDataSettings::ESampleRate48000Hz;
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
		err = SetAudioProperties(usedRate, usedChannels);

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

#if !defined(USE_FIXED_SIZE_BUFFERS)
	if (iUsedChannels == 1)
		bufferSize /= 2;
#endif

	// We are finished with the stream, so delete it
	delete iStream;
	iStream = NULL;

	// Set up the source with the used rate and channels
	iOggSource->ConstructL(bufferSize, theRate, iUsedRate, theChannels, iUsedChannels);

	TInt outStreamByteRate = iUsedChannels * iUsedRate * 2;
	iMaxBytesBetweenFrequencyBins = outStreamByteRate / 14; /* KOggControlFreq */
	}

void COggPlayController::PrimeL()
	{
    if (iState == EStatePrimed) 
          return;

    if (iAudioOutput == NULL)
        User::Leave(KErrNotReady);

	if (iState == EStateNotOpened)
		OpenFileL();

	if (iState == EStateOpenInfo)
		OpenCompleteL();

	iAudioOutput->SinkPrimeL();
	iState = EStatePrimed;
	}

void COggPlayController::PlayL()
	{
	if ((iState != EStatePrimed) && (iState != EStatePaused))
        User::Leave(KErrNotReady);
    
	iLastFreqArrayIdx = 0;
	iBytesSinceLastFrequencyBin = 0;

	// Clear the frequency analyser
	Mem::FillZ(&iFreqArray, sizeof(iFreqArray));

    // Do our own rate conversion, some firmware do it very badly.
    if (iUsedRate == 0 && (iStreamState == EStreamOpened) && (iStreamError == KErrNone))
		SetAudioCapsL(iDecoder->Rate(), iDecoder->Channels());
	else if (iStreamState == EStreamNotOpen)
		{
		// The audio stream isn't ready, so start playing later
		PlayDeferred();
		return;
		}
	else if (iStreamError != KErrNone)
		{
		// There is a stream error, we can't play
		User::Leave(iStreamError);
		}

	// Start playing now 
	PlayNowL();
	}

void COggPlayController::PlayDeferred()
	{
	iPlayRequestPending = ETrue;
	}

void COggPlayController::PlayNowL()
	{
	CFakeFormatDecode* fake = new(ELeave) CFakeFormatDecode(TFourCC(' ', 'P', '1', '6'), iUsedChannels, iUsedRate, iDecoder->Bitrate());
	CleanupStack::PushL(fake);
    
	// NegiotiateL leaves with KErrNotSupported if you don't give a CMMFFormatDecode for it's source:
	// indeed, it looks to be pretty useless if the source if not a CMMFFormatDecode. So use a 
	// fake CMMFFormatDecode class that just reports the configuration info (see CFakeFormatDecode)
	iAudioOutput->NegotiateL(*fake);
	CleanupStack::PopAndDestroy(fake);

    iAudioOutput->SinkPlayL();
    
    // send first buffer to sink - sending a NULL buffer prompts it to request some data the usual way
    iOggSource->SetSink(iAudioOutput);
    iAudioOutput->EmptyBufferL(NULL, iOggSource, TMediaId(KUidMediaTypeAudio));

    iState = EStatePlaying;
	}

void COggPlayController::CustomCommand(TMMFMessage& aMessage)
    {   
    if (aMessage.Destination().InterfaceId().iUid != ((TInt32) KOggPlayUidControllerImplementation))
        {
        aMessage.Complete(KErrNotSupported);    
        return;
        }

	TInt err = KErrNone;
    switch (aMessage.Function())
        {
		case EOggPlayControllerStreamWait:
			// Complete now if the stream is open, otherwise mark that we have received the request
			if (iStreamState == EStreamOpened)
				aMessage.Complete(iStreamError);
			else
				{
				iStreamState = EStreamStateRequested;
				iStreamMessage = new TMMFMessage(aMessage);
				if (!iStreamMessage)
					aMessage.Complete(KErrNoMemory);
				}
			break;

		case EOggPlayControllerCCGetAudioProperties:
			{
			if (iState == EStateNotOpened)
				{
				TRAPD(err, OpenFileL());
				if (err != KErrNone)
					{
					aMessage.Complete(err);
					return;
					}
				}

			if (iState == EStateOpenInfo)
				{
				TRAPD(err, OpenCompleteL());
				if (err != KErrNone)
					{
					aMessage.Complete(err);
					return;
					}
				}

			TInt audioProperties[6];
			audioProperties[0] = iRate;
			audioProperties[1] = iChannels;
			audioProperties[2] = iBitRate;
			audioProperties[3] = iFileSize;

			TInt64 fileLength = iFileLength.Int64();
			Mem::Move(&audioProperties[4], &fileLength, 8);

			TPckg<TInt [6]> dataTo(audioProperties);
			aMessage.WriteDataToClient(dataTo);
			aMessage.Complete(KErrNone);
			break;
			}
		
		case EOggPlayControllerCCGetFrequencies:
            {
            TRAP(err, GetFrequenciesL(aMessage));
		    aMessage.Complete(err);  
            break;
            }

        case EOggPlayControllerCCSetVolumeGain:
            {
            TRAP(err, SetVolumeGainL(aMessage));
		    aMessage.Complete(err);  
            break;
			}

		default:
            {
            err = KErrNotSupported;
		    aMessage.Complete(err);
            break;
            }
        }
    }

void COggPlayController::PauseL()
	{
	if (iPlayRequestPending)
		{
		iPlayRequestPending = EFalse;
		return;
		}

    if (iState != EStatePlaying)
        User::Leave(KErrNotReady);

	// On my N70 SinkPauseL() does pause the audio,
	// but restarting playback generates KErrNotReady events.
	// SinkStopL() seems to work just fine though. Strange.
	// iAudioOutput->SinkPauseL();

	iAudioOutput->SinkStopL();
    iState = EStatePaused;
	}

void COggPlayController::StopL()
	{
	if (iPlayRequestPending)
		{
		iPlayRequestPending = EFalse;
		return;
		}

	if ((iState != EStatePrimed) && (iState != EStatePlaying)) 
        User::Leave(KErrNotReady);

    iAudioOutput->SinkStopL();
    iDecoder->Clear();

	delete iDecoder;
	iDecoder = NULL;

	iOggSource->iTotalBufferBytes = 0;
    iState = EStateNotOpened;
	}

TTimeIntervalMicroSeconds COggPlayController::PositionL() const
	{
	if (iDecoder && (iState != EStateNotOpened))
		{
		TInt64 positionBytes = iOggSource->iTotalBufferBytes;
		TInt64 positionMilliseconds = (TInt64(500)*positionBytes)/TInt64(iUsedRate*iUsedChannels);

		return TTimeIntervalMicroSeconds(TInt64(1000)*positionMilliseconds);
		}
    else
		return TTimeIntervalMicroSeconds(0);
	}

void COggPlayController::SetPositionL(const TTimeIntervalMicroSeconds& aPosition)
	{
	TInt64 positionMilliseconds = aPosition.Int64() / 1000;
    if (iDecoder)
		iDecoder->Setposition(positionMilliseconds);

	iOggSource->iTotalBufferBytes = (positionMilliseconds*TInt64(iUsedRate*iUsedChannels))/500;
	iLastPlayTotalBytes = iOggSource->iTotalBufferBytes;
	}

TTimeIntervalMicroSeconds COggPlayController::DurationL() const
	{
	return iFileLength;
	}

void COggPlayController::GetNumberOfMetaDataEntriesL(TInt& aNumberOfEntries)
	{
    aNumberOfEntries = 5;
	}

CMMFMetaDataEntry* COggPlayController::GetMetaDataEntryL(TInt aIndex)
	{
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

void COggPlayController::OpenFileL()
	{
	iDecoder = GetDecoderL();
	TInt err = iDecoder->OpenInfo(iFileName);
	if (err != KErrNone)
		{
		iDecoder->Clear();

		delete iDecoder;
		iDecoder = NULL;

		User::Leave(err);
		}
        
	// Parse tag information and put it in the provided buffers.
	iDecoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);

	// Save the audio properties
	iRate = iDecoder->Rate();
	iChannels = iDecoder->Channels();
	iBitRate = iDecoder->Bitrate();

	// Change the title, to let know the client that we have a random ringing tone
	if (iRandomRingingTone)
		iTitle = KRandomRingingToneTitle;

	iState = EStateOpenInfo;
	}

void COggPlayController::OpenCompleteL()
	{
	TInt err = iDecoder->OpenComplete();
	if (err != KErrNone)
		{
		iDecoder->Clear();

		delete iDecoder;
		iDecoder = NULL;

		iState = EStateNotOpened;
		User::Leave(err);
		}

	// Save the remaining audio properties
	iFileLength = iDecoder->TimeTotal() * 1000;
	iFileSize = iDecoder->FileSize();

	iState = EStateOpen;
	}

_LIT(KOggExt, ".ogg");
_LIT(KOgaExt, ".oga");
_LIT(KFlacExt, ".flac");
_LIT(KMp3Ext, ".mp3");
MDecoder* COggPlayController::GetDecoderL()
	{
	TParsePtrC p(iFileName);
	TFileName ext(p.Ext());

	MDecoder* decoder = NULL;
	if (ext.CompareF(KOggExt) == 0)
		decoder = new(ELeave) CTremorDecoder(iFs);

	if (ext.CompareF(KOgaExt) == 0)
		decoder = new(ELeave) CFlacDecoder(iFs);

	if (ext.CompareF(KFlacExt) == 0)
		decoder = new(ELeave) CNativeFlacDecoder(iFs);

#if defined(MP3_SUPPORT)
	if (ext.CompareF(KMp3Ext) == 0)
		decoder = new(ELeave) CMadDecoder(iFs);
#endif

	if (!decoder)
		User::Leave(KErrNotSupported);

	return decoder;
	}

void COggPlayController::MapdSetVolumeRampL(const TTimeIntervalMicroSeconds& aRampDuration)
	{
    if (!iAudioOutput)
		User::Leave(KErrNotReady);

	iAudioOutput->SoundDevice().SetVolumeRamp(aRampDuration);
	}

void COggPlayController::MapdSetBalanceL(TInt aBalance)
	{
    if (!iAudioOutput)
		User::Leave(KErrNotReady);

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
    if (!iAudioOutput)
		User::Leave(KErrNotReady);

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
    if (!iAudioOutput)
		User::Leave(KErrNotReady);
    
    TInt maxVolume = iAudioOutput->SoundDevice().MaxVolume();
	if( ( aVolume < 0 ) || ( aVolume > maxVolume ))
	    User::Leave(KErrArgument);
    
    iAudioOutput->SoundDevice().SetVolume(aVolume);
	}

void COggPlayController::MapdGetMaxVolumeL(TInt& aMaxVolume)
	{
    if (!iAudioOutput)
		User::Leave(KErrNotReady);
    
    aMaxVolume = iAudioOutput->SoundDevice().MaxVolume();
	}

void COggPlayController::MapdGetVolumeL(TInt& aVolume)
	{
    if (!iAudioOutput)
		User::Leave(KErrNotReady);
    
	aVolume = iAudioOutput->SoundDevice().Volume();
	}

void COggPlayController::MapcSetPlaybackWindowL(const TTimeIntervalMicroSeconds& /*aStart*/, const TTimeIntervalMicroSeconds& /*aEnd*/)
	{
    // No implementation
    User::Leave(KErrNotSupported);
	}

void COggPlayController::MapcDeletePlaybackWindowL()
	{
    // No implementation
    User::Leave(KErrNotSupported);
	}

void COggPlayController::MapcGetLoadingProgressL(TInt& /*aPercentageComplete*/)
	{ 
    // No implementation
    User::Leave(KErrNotSupported);
	}

TInt COggPlayController::GetNewSamples(TDes8 &aBuffer)
	{
    if (iState != EStatePlaying)
        return 0;

	if ((iBytesSinceLastFrequencyBin >= iMaxBytesBetweenFrequencyBins) && !iRequestingFrequencyBins)
		{
		// Make a request for frequency data
		iDecoder->GetFrequencyBins(iFreqArray[iLastFreqArrayIdx].iFreqCoefs, KNumberOfFreqBins);

		// Mark that we have issued the request
		iRequestingFrequencyBins = ETrue;
		}

	TInt len = aBuffer.Length();
    TInt ret = iDecoder->Read(aBuffer, len);
    if (ret>0)
		{
        aBuffer.SetLength(len + ret);
		iBytesSinceLastFrequencyBin += ret;

		if (iRequestingFrequencyBins)
			{
			// Determine the status of the request
			TInt requestingFrequencyBins = iDecoder->RequestingFrequencyBins();
			if (!requestingFrequencyBins)
				{
				// The frequency request has completed
				iRequestingFrequencyBins = EFalse;
				iBytesSinceLastFrequencyBin = 0;

				// Time stamp the data
				iFreqArray[iLastFreqArrayIdx].iPcmByte = iOggSource->iTotalBufferBytes;

				// Move to the next array entry
				iLastFreqArrayIdx++;
				if (iLastFreqArrayIdx == KFreqArrayLength)
					iLastFreqArrayIdx = 0;

				// Mark it invalid
#if defined(SERIES60V3)
				iFreqArray[iLastFreqArrayIdx].iPcmByte = KMaxTInt64;
#else
				iFreqArray[iLastFreqArrayIdx].iPcmByte = TInt64(KMaxTInt32, KMaxTUint32);
#endif
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

void  COggPlayController::GetFrequenciesL(TMMFMessage& aMessage)
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

	TInt64 positionBytes = iLastPlayTotalBytes + TInt64(2)*TInt64(iUsedChannels)*TInt64(iAudioOutput->SoundDevice().SamplesPlayed());
	TInt idx = iLastFreqArrayIdx;
	for (TInt i = 0 ; i<KFreqArrayLength ; i++)
		{
		if (iFreqArray[idx].iPcmByte <= positionBytes)
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
    CleanupStack::PopAndDestroy(buf);
	}

void COggPlayController::MaoscOpenComplete(TInt aError)
	{
	if (iStreamState == EStreamStateRequested)
		{
		iStreamMessage->Complete(aError);

		delete iStreamMessage;
		iStreamMessage = NULL;
		}

	iStreamState = EStreamOpened;
	iStreamError = aError;

	if (iPlayRequestPending && (iStreamError == KErrNone))
		{
		// Start playing now (ignore error)
		TRAPD(err, 	{ SetAudioCapsL(iDecoder->Rate(), iDecoder->Channels()); PlayNowL(); });
		}

	iPlayRequestPending = EFalse;
	}

void COggPlayController::MaoscBufferCopied(TInt /* aError */, const TDesC8& /* aBuffer */)
	{
	}

void COggPlayController::MaoscPlayComplete(TInt /* aError */)
	{
	}


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
    iOggSampleRateConverter = new(ELeave) COggSampleRateConverter;
    
    iOggSampleRateConverter->Init(&iSampleRateFillBuffer, aBufferSize, aBufferSize-1024,
	aInputRate, aOutputRate, aInputChannel, aOutputChannel);

	iOggSampleRateConverter->SetVolumeGain(iGain);
	}

void COggSource::SetVolumeGain(TGainType aGain)
	{
	iGain = aGain;
	if (iOggSampleRateConverter)
		iOggSampleRateConverter->SetVolumeGain(aGain);
	}

void COggSource::SetSink(MDataSink* aSink)
    {
    iSink = aSink;
    }

void COggSource::ConstructSourceL(const TDesC8& /*aInitData*/)
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

void COggSource::FillBufferL(CMMFBuffer* aBuffer, MDataSink* aConsumer, TMediaId aMediaId)
    {   
    if ((aMediaId.iMediaType==KUidMediaTypeAudio)&&(aBuffer->Type()==KUidMmfDescriptorBuffer))
		{
        CMMFDataBuffer* db = static_cast<CMMFDataBuffer*>(aBuffer);
        if (iOggSampleRateConverter->FillBuffer(db->Data()) == KErrCompletion)
            db->SetLastBuffer(ETrue);

		iTotalBufferBytes += db->Data().Length();
		SetSink(aConsumer);
        aConsumer->BufferFilledL(db);
	    }
    else
		User::Leave(KErrNotSupported);
    }


void COggSource::BufferEmptiedL(CMMFBuffer* aBuffer)
	{
    if ((aBuffer->Type()==KUidMmfDescriptorBuffer) || (aBuffer->Type()==KUidMmfTransferBuffer) || (aBuffer->Type()==KUidMmfPtrBuffer))
		{    
        CMMFDataBuffer* db = static_cast<CMMFDataBuffer*>(aBuffer);
        if (iOggSampleRateConverter->FillBuffer(db->Data()) == KErrCompletion)
            db->SetLastBuffer(ETrue);

		iTotalBufferBytes += db->Data().Length();
		iSink->EmptyBufferL(db, this, TMediaId(KUidMediaTypeAudio));
		}
    else
		User::Leave(KErrNotSupported);
	}


_LIT(KFakeFormatDecodePanic, "FakeFormatDecode");
CFakeFormatDecode::CFakeFormatDecode(TFourCC aFourCC, TUint aChannels, TUint aSampleRate, TUint aBitRate)
: iFourCC(aFourCC), iChannels(aChannels), iSampleRate(aSampleRate), iBitRate(aBitRate)
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
