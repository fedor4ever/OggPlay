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


CPluginInfo::CPluginInfo()
  {
  }

CPluginInfo* CPluginInfo::NewL(const TDesC& anExtension, 
const CMMFFormatImplementationInformation &aFormatInfo, const TUid aControllerUid)
	{
	CPluginInfo* self = new(ELeave) CPluginInfo;
	CleanupStack::PushL(self);
	self->ConstructL(anExtension, aFormatInfo, aControllerUid);

	CleanupStack::Pop(self);
	return self;
	}

void CPluginInfo::ConstructL(const TDesC& anExtension, 
const CMMFFormatImplementationInformation &aFormatInfo, const TUid aControllerUid) 
	{
    iExtension = anExtension.AllocL();
    iFormatUid = aFormatInfo.Uid();
    iName = aFormatInfo.DisplayName().AllocL();
    iSupplier = aFormatInfo.Supplier().AllocL();
    iVersion = aFormatInfo.Version();
    iControllerUid = aControllerUid;
	}

CPluginInfo::~CPluginInfo()
	{
    delete iExtension;
    delete iName;
    delete iSupplier;
	}


CPluginSupportedList::CPluginSupportedList()
	{
	}

void CPluginSupportedList::ConstructL()
	{
	iPlugins = new(ELeave) CArrayPtrFlat<TExtensionList>(3);
	}

CPluginSupportedList::~CPluginSupportedList()
	{
	for (TInt i = 0 ; i<iPlugins->Count() ; i++)
		{
		iPlugins->At(i)->listPluginInfos->ResetAndDestroy();
		delete(iPlugins->At(i)->listPluginInfos);
		}

	iPlugins->ResetAndDestroy();
	delete iPlugins;
	}

void CPluginSupportedList::AddPluginL(const TDesC &aExtension,
const CMMFFormatImplementationInformation &aFormatInfo, const TUid aControllerUid)
	{
    CPluginInfo* info = CPluginInfo::NewL(aExtension, aFormatInfo, aControllerUid);
    CleanupStack::PushL(info);
    
    AddExtension(aExtension);
    TInt index = FindListL(aExtension);

    // Add the plugin to the existing list
    TExtensionList* list = iPlugins->At(index);
    list->listPluginInfos->AppendL(info);
    
    CleanupStack::Pop(info);
	}

void CPluginSupportedList::SelectPluginL(const TDesC& anExtension, TUid aSelectedUid)
	{ 
    TInt index = FindListL(anExtension);
    if(index <0) 
      User::Leave(KErrNotFound);
       
    TExtensionList * list = iPlugins->At(index);
       
    if (aSelectedUid == TUid::Null())
		{
    	list->selectedPlugin = NULL;
    	return;
		}
    
    TBool found = EFalse;
    for(TInt i =0; i<list->listPluginInfos->Count(); i++)
		{
        if (list->listPluginInfos->At(i)->iControllerUid == aSelectedUid)
			{
            list->selectedPlugin = list->listPluginInfos->At (i) ;
            found = ETrue;
			}
		}

    if (!found)
		User::Leave(KErrNotFound);
	}

CDesCArrayFlat* CPluginSupportedList::SupportedExtensions()
	{
    CDesCArrayFlat * extensions = NULL;
    TRAPD(err,
		{
        extensions = new (ELeave) CDesCArrayFlat (3);
        CleanupStack::PushL(extensions);
        for (TInt i=0; i<iPlugins->Count(); i++)
           extensions->AppendL(iPlugins->At(i)->extension);

		CleanupStack::Pop();
	    });

    return extensions;
	}

TInt CPluginSupportedList::FindListL(const TDesC &anExtension)
	{
    for (TInt i = 0 ; i<iPlugins->Count() ; i++)
		{
        if (anExtension.CompareF(iPlugins->At(i)->extension) == 0)
            return i;
		}

    return KErrNotFound;
	}
    
CPluginInfo* CPluginSupportedList::GetSelectedPluginInfo(const TDesC &anExtension)
	{
    TInt index = FindListL(anExtension);
    if(index<0) 
      return NULL;

	TExtensionList * list = iPlugins->At(index);
    if (!list)
      return NULL;

	return list->selectedPlugin;
	}

CArrayPtrFlat<CPluginInfo>* CPluginSupportedList::GetPluginInfoList(const TDesC &anExtension)
	{
	TInt index = FindListL(anExtension);
    if(index<0) 
      return NULL;

    TExtensionList * list = iPlugins->At(index);
    if (!list)
      return NULL;

	return list->listPluginInfos;		
	}

void CPluginSupportedList::AddExtension(const TDesC &anExtension)
	{
	TInt index = FindListL(anExtension);
    if(index>=0) 
      return; // The extension already exists
    
    // List not found, create a new list
    TExtensionList * list = new(ELeave) TExtensionList;	
    CleanupStack::PushL(list);
    list->listPluginInfos = new(ELeave) CArrayPtrFlat <CPluginInfo>(1);
    list->extension = anExtension;
    list->selectedPlugin = NULL;

    iPlugins->AppendL(list);
    CleanupStack::Pop(list);
	}


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


COggPluginAdaptor::COggPluginAdaptor(COggMsgEnv* aEnv, MPlaybackObserver* aObserver)
:CAbsPlayback(aObserver), iEnv(aEnv), iVolume(KMaxVolume), iGain(ENoGain)
	{
	}

COggPluginAdaptor::~COggPluginAdaptor()
	{
	delete iPlayer;
	delete iFileInfoUtil;
	}

void COggPluginAdaptor::ConstructL()
	{
	iPluginSupportedList.ConstructL();

	// iPluginInfos will be updated, if required plugin has been found
    SearchPluginsL(_L("ogg"), ETrue);
    SearchPluginsL(_L("oga"), ETrue);
    SearchPluginsL(_L("flac"), ETrue);

	SearchPluginsL(_L("mp3"), ETrue);
    SearchPluginsL(_L("aac"), ETrue);
    SearchPluginsL(_L("mp4"), ETrue);
    SearchPluginsL(_L("m4a"), ETrue);

#if defined(SERIES60V3)
	SearchPluginsL(_L("wma"), ETrue);
#endif

	SearchPluginsL(_L("mid"), EFalse); // Disabled by default
    SearchPluginsL(_L("amr"), EFalse); // Disabled by default
	}

TInt COggPluginAdaptor::Info(const TDesC& aFileName, MFileInfoObserver& aFileInfoObserver)
	{
    // That's unfortunate, but we must open the file first before the info 
    // can be retrieved. This adds some serious delay when discovering the files
    TRAPD(err, OpenInfoL(aFileName, aFileInfoObserver));
    return err;
	}

TInt COggPluginAdaptor::FullInfo(const TDesC& aFileName, MFileInfoObserver& aFileInfoObserver)
	{
    // That's unfortunate, but we must open the file first before the info 
    // can be retrieved. This adds some serious delay when discovering the files
    TRAPD(err, OpenInfoL(aFileName, aFileInfoObserver, ETrue));
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

void COggPluginAdaptor::OpenL(const TDesC& aFileName)
	{
    TRACEF(COggLog::VA(_L("OpenL %S"), &aFileName));

	iFileInfo.iFileName = aFileName;
    ConstructAPlayerL();
  
    // Find the selected plugin, corresponding to file type
    TParsePtrC p(aFileName);
    TPtrC pp(p.Ext().Mid(1));
    CPluginInfo * info = GetPluginListL().GetSelectedPluginInfo(pp);
    if (info == NULL)
      	User::Leave(KErrNotFound);

    iPluginControllerUID = info->iControllerUid;
    iPlayer->OpenFileL(aFileName, KNullUid, iPluginControllerUID);       
	iState = EStreamOpen;
	}

void COggPluginAdaptor::OpenInfoL(const TDesC& aFileName, MFileInfoObserver& aFileInfoObserver, TBool aFullInfo)
	{
	ConstructAFileInfoUtilL();

	// Find the selected plugin, corresponding to file type
	TParsePtrC p(aFileName);
	TPtrC pp(p.Ext().Mid(1));
	CPluginInfo* info = GetPluginListL().GetSelectedPluginInfo(pp);
	if (info == NULL)
		User::Leave(KErrNotFound);

	iFileOpenObserver.SetFileInfo(aFileName);
	iFileOpenObserver.iFullInfo = aFullInfo;
	iFileOpenObserver.iControllerUid = info->iControllerUid;
	iFileOpenObserver.iFileInfoObserver = &aFileInfoObserver;

	iFileInfoUtil->OpenFileL(aFileName, KNullUid, info->iControllerUid);
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

TInt COggPluginAdaptor::Open(const TDesC& aFileName)
{
    TRAPD(err, OpenL(aFileName));
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
    TRACEF(_L("COggPluginAdaptor::Play() In"));
    iInterrupted = EFalse;
    if ((iState == EClosed) || (iState == EStreamOpen))
        return; // Cannot play if we haven't opened a file

    TRAPD(err, iPlayer->PlayL()); 
    if (err)
		return; // Silently ignore the problem :-(

	iState = EPlaying;
	iObserver->ResumeUpdates();

	TRACEF(_L("COggPluginAdaptor::Play() Out"));
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
    TRACEF(_L("COggPluginAdaptor::SetVolume()"));
    if ((aVol<0) || (aVol >KMaxVolume))
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

		iObserver->NotifyFileOpen(aErr);
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

CPluginSupportedList& COggPluginAdaptor::GetPluginListL()
	{
    return iPluginSupportedList;
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
    
        TBuf8<10> ex8;
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
        iPluginSupportedList.AddPluginL(aExtension, *formatArray[i], controllers[ii]->Uid());
            
        if (isEnabled)
			{
			// This will be overruled by the .ini file, if the file exist.
			iPluginSupportedList.SelectPluginL(aExtension, controllers[ii]->Uid());
			}
		}
    
    CleanupStack::PopAndDestroy(3, cSelect); /* cSelect, fSelect, controllers */
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

#endif // PLUGIN_SYSTEM
