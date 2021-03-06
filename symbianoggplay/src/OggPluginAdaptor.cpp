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

// Platform settings
#include <OggOs.h>

// This file is for PLUGIN_SYSTEM only
#if defined(PLUGIN_SYSTEM)

#include <e32base.h>
#include <e32svr.h>
#include <badesca.h>
#include <f32file.h>

#include "OggAbsPlayback.h"
#include "OggPluginAdaptor.h"
#include "OggLog.h"

#if defined(SERIES60V3)
#include "S60V3ImplementationUIDs.hrh"
#else
#include "ImplementationUIDs.hrh"
#endif

#include "OggPlayControllerCustomCommands.h"


void TUtilFileOpenObserver::SetFileInfo(const TDesC& aFileName)
	{
	// Reset the file info
	iFileInfo.iFileName = aFileName;
	iFileInfo.iBitRate = 0;
	iFileInfo.iChannels = 0;
	iFileInfo.iFileSize = 0;
	iFileInfo.iRate = 0;
	iFileInfo.iTime = 0;
  
	iFileInfo.iAlbum = KNullDesC;
	iFileInfo.iArtist = KNullDesC;
	iFileInfo.iGenre = KNullDesC;
	iFileInfo.iTitle = KNullDesC;
	iFileInfo.iTrackNumber = KNullDesC;
	}
	
void TUtilFileOpenObserver::MoscoStateChangeEvent(CBase* aObject, TInt /* aPreviousState */, TInt /* aCurrentState */, TInt aErr)
	{
	CMdaAudioRecorderUtility* util = (CMdaAudioRecorderUtility *) aObject;
	if (aErr == KErrNone)
		COggPluginAdaptor::RetrieveFileInfo(util, iFileInfo, iFullInfo, iControllerUid);

	iFileInfoObserver->FileInfoCallback(aErr, iFileInfo);
	util->Close();
	}


COggPluginAdaptor::COggPluginAdaptor(COggMsgEnv* aEnv, CPluginSupportedList& aPluginSupportedList, MPlaybackObserver* aObserver)
:CAbsPlayback(aPluginSupportedList, aObserver), iEnv(aEnv), iVolume(KMaxVolume), iGain(ENoGain)
	{
	}

COggPluginAdaptor::~COggPluginAdaptor()
	{
	delete iPlayer;
	delete iFileInfoUtil;
	}

void COggPluginAdaptor::ConstructL()
	{
	// Search for these plugins, but don't use them
    SearchPluginsL(_L("ogg"), EFalse); // Use MTP by default
    SearchPluginsL(_L("oga"), EFalse); // Use MTP by default
    SearchPluginsL(_L("flac"), EFalse); // Use MTP by default

	// Search for these plugins and use them if found
	SearchPluginsL(_L("mp3"), ETrue);
    SearchPluginsL(_L("aac"), ETrue);
    SearchPluginsL(_L("mp4"), ETrue);
    SearchPluginsL(_L("m4a"), ETrue);

#if defined(SERIES60V3)
	SearchPluginsL(_L("wma"), ETrue);
#endif

	// Search for these plugins but disable them
	SearchPluginsL(_L("mid"), EFalse); // Disabled by default
    SearchPluginsL(_L("amr"), EFalse); // Disabled by default
	}

TInt COggPluginAdaptor::Info(const TDesC& aFileName, TUid aControllerUid, MFileInfoObserver& aFileInfoObserver)
	{
    // That's unfortunate, but we must open the file first before the info 
    // can be retrieved. This adds some serious delay when discovering the files
    TRAPD(err, OpenInfoL(aFileName, aControllerUid, aFileInfoObserver));
    return err;
	}

TInt COggPluginAdaptor::FullInfo(const TDesC& aFileName, TUid aControllerUid, MFileInfoObserver& aFileInfoObserver)
	{
    // That's unfortunate, but we must open the file first before the info 
    // can be retrieved. This adds some serious delay when discovering the files
    TRAPD(err, OpenInfoL(aFileName, aControllerUid, aFileInfoObserver, ETrue));
    return err;
	}

void COggPluginAdaptor::InfoCancel()
	{
	// Cancel the info request
	// Unfortunately there's no way to do that with the MMF,
	// calling iFileInfoUtil->Close() doesn't work.
		
	// Deleting the file util seems to be the only way
	delete iFileInfoUtil;
	iFileInfoUtil = NULL;
	}

void COggPluginAdaptor::OpenL(const TDesC& aFileName, TUid aControllerUid)
	{
    TRACEF(COggLog::VA(_L("OpenL %S"), &aFileName));

	iFileInfo.iFileName = aFileName;
    ConstructAPlayerL();

    iPluginControllerUID = aControllerUid;
    iPlayer->OpenFileL(aFileName, KNullUid, iPluginControllerUID);       
	iState = EStreamOpen;
	}

void COggPluginAdaptor::OpenInfoL(const TDesC& aFileName, TUid aControllerUid, MFileInfoObserver& aFileInfoObserver, TBool aFullInfo)
	{
	ConstructAFileInfoUtilL();

	iFileOpenObserver.SetFileInfo(aFileName);
	iFileOpenObserver.iFullInfo = aFullInfo;
	iFileOpenObserver.iControllerUid = aControllerUid;
	iFileOpenObserver.iFileInfoObserver = &aFileInfoObserver;

	iFileInfoUtil->OpenFileL(aFileName, KNullUid, aControllerUid);
	}

void COggPluginAdaptor::ParseMetaDataValueL(CMMFMetaDataEntry &aMetaData, TDes &aDestinationBuffer)
	{
	// Try to remove all TABs, CR and LF
	// having those around is screwing up the SW
	HBufC* tempBuf = HBufC::NewL(aMetaData.Value().Length());
	CleanupStack::PushL(tempBuf);

	TPtr tempDes = tempBuf->Des();
	TLex parse(aMetaData.Value());
	TBool first = ETrue;
	for ( ; ; )
		{
		TPtrC token = parse.NextToken();
		if (!token.Length())
			break;

		if (!first)
			tempDes.Append(_L(" "));

		tempDes.Append(token); 
		first = EFalse;
		}

	// Crop to the final buffer
	aDestinationBuffer = tempBuf->Left(aDestinationBuffer.MaxLength());
	CleanupStack::PopAndDestroy(tempBuf);
	}

TInt COggPluginAdaptor::Open(const TOggSource& aSource, TUid aControllerUid)
	{
    TRAPD(err, OpenL(aSource.iSourceName, aControllerUid));
    return err;
	}

void COggPluginAdaptor::Pause()
	{
    TRACEF(_L("COggPluginAdaptor::Pause()"));
    if (!iInterrupted)
        iLastPosition = iPlayer->Position();
    
    iPlayer->Stop();
    iState = EPaused;
	}

void COggPluginAdaptor::Resume()
	{
    TRACEF(_L("COggPluginAdaptor::Resume()"));
    iPlayer->SetPosition(iLastPosition);
    Play();
	}

void COggPluginAdaptor::Play()
	{
    TRACEF(_L("COggPluginAdaptor::Play()"));
    iInterrupted = EFalse;
    if ((iState == EClosed) || (iState == EStreamOpen))
        return; // Cannot play if we haven't opened a file

    TRAPD(err, iPlayer->PlayL()); 
    if (err)
		return; // Silently ignore the problem :-(

	iState = EPlaying;
	iObserver->ResumeUpdates();
	}

void COggPluginAdaptor::Stop()
	{
    TRACEF(_L("COggPluginAdaptor::Stop()"));

	if ((iState == EStopped) || (iState == EClosed))
		return;

	iPlayer->Stop();

	delete iPlayer;
	iPlayer = NULL;
	iState = EClosed;

	ClearComments();
    iObserver->NotifyUpdate();
	}

void COggPluginAdaptor::SetVolume(TInt aVol)
	{
    if ((aVol<0) || (aVol>KMaxVolume))
	    return;

    iVolume = aVol;
	if (iPlayer)
		{
		TInt max = iPlayer->MaxVolume();
		TReal relative = ((TReal) aVol)/KMaxVolume;
		TInt vol = (TInt) (relative*max);
		iPlayer->SetVolume(vol);
		}
	}

void COggPluginAdaptor::SetPosition(TInt64 aPos)
	{
    TRACEF(COggLog::VA(_L("COggPluginAdaptor::SetPosition %i"), aPos ));
    if (iPlayer)
		{
		// Convert to microseconds
		aPos *= 1000;

		// Limit FF to five seconds before the end of the track
		TInt64 duration = iPlayer->Duration().Int64();
		if (aPos>duration)
			aPos = duration - TInt64(5000000);

		// And don't allow RW past the beginning either
		if (aPos<0)
			aPos = 0;

		// Must stop before changing position. Some MMF Plugins panics otherwise.
		iPlayer->Stop();
		iPlayer->SetPosition(aPos);

		iPlayer->PlayL();
		}
	}

TInt64 COggPluginAdaptor::Position()
	{
    TTimeIntervalMicroSeconds pos(0);
    if (iState == EPaused)
        pos = iLastPosition;
    else if (iPlayer)
        pos = iPlayer->Position();

    if (pos != TTimeIntervalMicroSeconds(0))
        iLastPosition = pos;
    
    return pos.Int64()/1000;
	}

TInt64 COggPluginAdaptor::Time()
	{
    if (iPlayer)
		return iPlayer->Duration().Int64()/1000;

	return 0;
	}

TInt COggPluginAdaptor::Volume()
	{
    // Not Implemented yet
    return 0;
	}

const TInt32* COggPluginAdaptor::GetFrequencyBins()
	{
	if (iPluginControllerUID.iUid != ((TInt32) KOggPlayUidControllerImplementation))
		return iFreqBins;

    TMMFGetFreqsParams pckg;
    pckg.iFreqBins = iFreqBins;
    TMMFGetFreqsConfig freqConfig(pckg); // Pack the config.
    TPckg <TInt32 [16]> dataFrom(iFreqBins); 
    
    TMMFMessageDestination msg(iPluginControllerUID, KMMFObjectHandleController);
    TPckgBuf<TMMFMessageDestination> packedMsg(msg); // Pack the destination
    
    TInt err = iPlayer->PlayControllerCustomCommandSync(packedMsg, 
    EOggPlayControllerCCGetFrequencies, freqConfig, KNullDesC8, dataFrom);

	if (err != KErrNone)
		{
		TRACEF(COggLog::VA(_L("COggPluginAdaptor::GetFrequencyBins %i"), err));
		}

	return iFreqBins;
	}

void COggPluginAdaptor::SetVolumeGain(TGainType aGain)
	{
    TMMFSetVolumeGainParams pckg;
    pckg.iGain = (TInt) aGain;
    TMMFSetVolumeGainConfig gainConfig(pckg); // Pack the config.

	TInt dummy;
	TPckg <TInt> dataFrom(dummy); 
    
    TMMFMessageDestination msg(iPluginControllerUID, KMMFObjectHandleController);
    TPckgBuf<TMMFMessageDestination> packedMsg(msg); // Pack the destination
    
	TInt err = KErrNone;
	iGain = aGain;
	if (iPlayer && (iPluginControllerUID.iUid == ((TInt32) KOggPlayUidControllerImplementation)))
		err = iPlayer->PlayControllerCustomCommandSync(packedMsg, EOggPlayControllerCCSetVolumeGain, gainConfig, KNullDesC8, dataFrom);

	if (err != KErrNone)
		{
		TRACEF(COggLog::VA(_L("COggPluginAdaptor::SetVolumeGain %i"), err));
		}
	}

void COggPluginAdaptor::GetAudioProperties(TOggFileInfo& aFileInfo, CMdaAudioRecorderUtility* aUtil, TUid aControllerUid)
	{
	if (aControllerUid.iUid != ((TInt32) KOggPlayUidControllerImplementation))
		{
		aFileInfo.iRate = 0;
		aFileInfo.iChannels = 0;
		aFileInfo.iBitRate = 0;
		aFileInfo.iFileSize = 0;
		aFileInfo.iTime = 0;
		return;
		}

	TInt audioData[6];
    TPckg<TInt [6]> dataFrom(audioData);    
    TMMFMessageDestination msg(aControllerUid, KMMFObjectHandleController);
    TPckgBuf<TMMFMessageDestination> packedMsg(msg);

    TInt err = aUtil->PlayControllerCustomCommandSync(packedMsg, 
    EOggPlayControllerCCGetAudioProperties, KNullDesC8, KNullDesC8, dataFrom);

	if (err == KErrNone)
		{
		aFileInfo.iRate = audioData[0];
        aFileInfo.iChannels = audioData[1];
        aFileInfo.iBitRate = audioData[2];
		aFileInfo.iFileSize = audioData[3];

		Mem::Move(&aFileInfo.iTime, &audioData[4], 8);
		aFileInfo.iTime /= 1000;
		}
	else
		{
		aFileInfo.iRate = 0;
		aFileInfo.iChannels = 0;
		aFileInfo.iBitRate = 0;
		aFileInfo.iFileSize = 0;
		aFileInfo.iTime = 0;

		TRACEF(COggLog::VA(_L("COggPluginAdaptor::GetAudioProperties %i"), err));
		}
	}

void COggPluginAdaptor::MoscoStateChangeEvent(CBase* aObject, TInt aPreviousState, TInt aCurrentState, TInt aErr)
	{
    TRACEF(COggLog::VA(_L("MoscoStateChange :%d %d %d "), aPreviousState,  aCurrentState,  aErr));

	if (aCurrentState == CMdaAudioClipUtility::ENotReady)
		{
		// Error opening file
		iObserver->NotifySourceOpen(aErr);
		}

	if ((aPreviousState == CMdaAudioClipUtility::ENotReady) && (aCurrentState == CMdaAudioClipUtility::EOpen))
		{
		iState = EStopped;
		RetrieveFileInfo((CMdaAudioRecorderUtility *) aObject, iFileInfo, ETrue, iPluginControllerUID);

		// Clear the frequency analyser
		Mem::FillZ(&iFreqBins, sizeof(iFreqBins));

		// Set the volume
        SetVolume(iVolume);
		SetVolumeGain(iGain);

		// Sync with the controller
		if (iPluginControllerUID.iUid == ((TInt32) KOggPlayUidControllerImplementation))
			{
			// Wait until the controller has opened the audio output stream
			TMMFMessageDestination msg(iPluginControllerUID, KMMFObjectHandleController);
			TPckgBuf<TMMFMessageDestination> packedMsg(msg);

			aErr = iPlayer->PlayControllerCustomCommandSync(packedMsg, EOggPlayControllerStreamWait, KNullDesC8, KNullDesC8);
			}

		iObserver->NotifySourceOpen(aErr);
		}

    if (aCurrentState == CMdaAudioClipUtility::EPlaying)
		{
		if (aErr == KErrNone)
			{
			// From opened to Playing
			iObserver->NotifyPlayStarted();
			}
		else
			{
			// From opened to Playing
			// The sound device was stolen by somebody else. (A SMS arrival notice, for example).
			iInterrupted = ETrue;
			iObserver->NotifyPlayInterrupted();
			}
		}
    
    if ((aPreviousState == CMdaAudioClipUtility::EPlaying) && (aCurrentState == CMdaAudioClipUtility::EOpen))
		{
        // From Playing to stopped playing
        TRACEF(COggLog::VA(_L("MoscoStateChange : PlayComplete %d"), aErr));
        switch (aErr)
			{
			case KErrDied:
	            // The sound device was stolen by somebody else. (A SMS arrival notice, for example).
		        iInterrupted = ETrue;
			    iObserver->NotifyPlayInterrupted();
				break;

			case KErrUnderflow:
		        iState = EStopped;
			    break;

			default:
				iObserver->NotifyPlayComplete();
				break;
	        }
		}
	}

void COggPluginAdaptor::ConstructAPlayerL()
	{
	delete iPlayer;
    iPlayer = NULL;

	iState = EClosed;
	iPlayer = CMdaAudioRecorderUtility::NewL(*this);
	}

void COggPluginAdaptor::ConstructAFileInfoUtilL()
	{
    delete iFileInfoUtil;
    iFileInfoUtil = NULL;

	iFileInfoUtil = CMdaAudioRecorderUtility::NewL(iFileOpenObserver);
	}

void COggPluginAdaptor::SearchPluginsL(const TDesC &aExtension, TBool isEnabled)
	{
	iPluginSupportedList.AddExtension(aExtension); // Add this extension to the selected list
    CMMFControllerPluginSelectionParameters* cSelect = CMMFControllerPluginSelectionParameters::NewLC();
    CMMFFormatSelectionParameters* fSelect = CMMFFormatSelectionParameters::NewLC();

    fSelect->SetMatchToFileNameL(aExtension);
    cSelect->SetRequiredPlayFormatSupportL(*fSelect);
    
    RMMFControllerImplInfoArray controllers;
    CleanupResetAndDestroyPushL(controllers);
    cSelect->ListImplementationsL(controllers);

    TInt nbFound = 0;
    for (TInt ii = 0 ; ii<controllers.Count() ; ii++)
		{
        // Found at least one controller!  
        const RMMFFormatImplInfoArray& formatArray = controllers[ii]->PlayFormats();
    
        TBuf8<KMaxFileName> ex8;
        ex8.Copy(aExtension); // Convert the 16 bit descriptor to 8 bits
        
        TInt found = EFalse;
        TInt i;
        for (i = 0 ; i<formatArray.Count() ; i++)
			{            
            const CDesC8Array &extensions = formatArray[i]->SupportedFileExtensions();
            for (TInt j = 0 ; j<extensions.Count() ; j++)
				{
                if (extensions[j].FindF(ex8) != KErrNotFound)
					{
                    found = ETrue;
                    break;
					}
				}

            if (found) 
                break;
        }
        
        __ASSERT_ALWAYS(found, User::Panic(_L("Extension not found"), 999));

        TRACEF(COggLog::VA(_L(".%S Plugin Found:%S %S %i %x"), &aExtension,
        &formatArray[i]->DisplayName(), &formatArray[i]->Supplier(), formatArray[i]->Version(), controllers[ii]->Uid()));
       
        nbFound++;
		TPluginImplementationInformation pluginInfo(formatArray[i]->DisplayName(), formatArray[i]->Supplier(), formatArray[i]->Uid(), formatArray[i]->Version());
        iPluginSupportedList.AddPluginL(aExtension, pluginInfo, controllers[ii]->Uid());
            
        if (isEnabled)
			{
			// This will be overruled by the .ini file, if the file exist.
			iPluginSupportedList.SelectPluginL(aExtension, controllers[ii]->Uid());
			}
		}

	CleanupStack::PopAndDestroy(3, cSelect); // cSelect, fSelect, controllers
	}

void COggPluginAdaptor::RetrieveFileInfo(CMdaAudioRecorderUtility* aUtil, TOggFileInfo& aFileInfo, TBool aFullInfo, TUid aControllerUid)
	{
	TInt nbMetaData;
	TInt err = aUtil->GetNumberOfMetaDataEntries(nbMetaData);
	if (err != KErrNone)
		nbMetaData = 0;

	CMMFMetaDataEntry* metaData;
	TBuf<128> metaDataValue;
	for (TInt i = 0 ; i<nbMetaData ; i++)
		{
		metaData = aUtil->GetMetaDataEntryL(i);
		CleanupStack::PushL(metaData);

		ParseMetaDataValueL(*metaData, metaDataValue);

		const TDesC& name(metaData->Name());
		if (name  == _L("title"))
			aFileInfo.iTitle = metaDataValue;

		if (name  == _L("album"))
			aFileInfo.iAlbum = metaDataValue;

		if (name  == _L("artist"))
			aFileInfo.iArtist = metaDataValue;

		if (name  == _L("genre"))
			aFileInfo.iGenre = metaDataValue;

		if (name  == _L("albumtrack"))
			aFileInfo.iTrackNumber = metaDataValue;

		CleanupStack::PopAndDestroy(metaData);
        }

	if (aFullInfo)
	 	GetAudioProperties(aFileInfo, aUtil, aControllerUid);
	}

TBool COggPluginAdaptor::Seekable()
	{
	// MMF playback can only seek when playing
	return (iState == EPlaying);
	}

#endif // PLUGIN_SYSTEM
