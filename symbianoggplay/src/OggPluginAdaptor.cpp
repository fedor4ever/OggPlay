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

#include <e32base.h>
#include "OggAbsPlayback.h"
#include "OggPluginAdaptor.h"
#include <e32svr.h>
#include <badesca.h>
#include "OggLog.h"
#include "f32file.h"
#ifdef MMF_AVAILABLE
#include "OggPlayControllerCustomCommands.h"
#endif /* MMF_AVAILABLE */


////////////////////////////////////////////////////////////////
//
// CPluginInfo
//
////////////////////////////////////////////////////////////////

CPluginInfo::CPluginInfo()
  {
  }

CPluginInfo* CPluginInfo::NewL(const TDesC& anExtension, 
                               const CMMFFormatImplementationInformation &aFormatInfo,
                               const TUid aControllerUid)
{
  CPluginInfo* self = new (ELeave) CPluginInfo();
  CleanupStack::PushL(self);
  self->ConstructL(anExtension, aFormatInfo,aControllerUid);
  CleanupStack::Pop(); // self
  return self;
  }


void
CPluginInfo::ConstructL(const TDesC& anExtension, 
                        const CMMFFormatImplementationInformation &aFormatInfo,
                        const TUid aControllerUid) 
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
    if ( iExtension) delete iExtension;
    if ( iName )     delete iName;
    if ( iSupplier ) delete iSupplier;
}


////////////////////////////////////////////////////////////////
//
// CPluginSupportedList
//
////////////////////////////////////////////////////////////////

CPluginSupportedList::CPluginSupportedList()
{}

void CPluginSupportedList::ConstructL()
{
     iPlugins = new(ELeave) CArrayPtrFlat <TExtensionList>(3);
}

CPluginSupportedList::~CPluginSupportedList()
{
	for (TInt i=0; i<iPlugins->Count();i++)
	{
	  iPlugins->At(i)->listPluginInfos->ResetAndDestroy();
	  delete(iPlugins->At(i)->listPluginInfos);
	}
    iPlugins->ResetAndDestroy();
    delete(iPlugins);
}

void CPluginSupportedList::AddPluginL(
		   const TDesC &anExtension,
           const CMMFFormatImplementationInformation &aFormatInfo,
           const TUid aControllerUid)
{

    CPluginInfo* info = CPluginInfo::NewL(anExtension, aFormatInfo, aControllerUid);
    CleanupStack::PushL(info);
    
    AddExtension(anExtension);
    TInt index = FindListL(anExtension);

    // Add the plugin to the existing list
    TExtensionList * list = iPlugins->At(index);
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
    if ( !found ) User::Leave(KErrNotFound);
    
}

CDesCArrayFlat * CPluginSupportedList::SupportedExtensions()
{
    CDesCArrayFlat * extensions = NULL;
    TRAPD(err,
    {
        extensions = new (ELeave) CDesCArrayFlat (3);
        CleanupStack::PushL(extensions);
        for (TInt i=0; i<iPlugins->Count(); i++)
        {
           extensions->AppendL(iPlugins->At(i)->extension);
        }
        CleanupStack::Pop();
    }
    ) // END OF TRAP
    return(extensions);
}

TInt CPluginSupportedList::FindListL(const TDesC &anExtension)
{
    for (TInt i=0; i<iPlugins->Count(); i++)
    {
        if ( anExtension.CompareF( iPlugins->At(i)->extension ) == 0)
        {
            return(i);
        }
    }
    return(-1); // Not found
}
    
CPluginInfo * CPluginSupportedList::GetSelectedPluginInfo(const TDesC &anExtension)
{
    TInt index = FindListL(anExtension);
    if(index <0) 
      return NULL;
    TExtensionList * list = iPlugins->At(index);
    if (list == NULL)
      return NULL;
    return (list->selectedPlugin);
}

CArrayPtrFlat <CPluginInfo> * CPluginSupportedList::GetPluginInfoList(const TDesC &anExtension)
{   
	TInt index = FindListL(anExtension);
    if(index <0) 
      return NULL;
    TExtensionList * list = iPlugins->At(index);
    if (list == NULL)
      return NULL;
    return (list->listPluginInfos);
		
}
void CPluginSupportedList::AddExtension(const TDesC &anExtension)
{
	TInt index = FindListL(anExtension);
    if(index >=0) 
      return; // The extension already exists
    
    // List not found, create a new list
    TExtensionList * list = new (ELeave) TExtensionList;	
    CleanupStack::PushL(list);
    list->listPluginInfos = new (ELeave) CArrayPtrFlat <CPluginInfo>(1);
    list->extension = anExtension;
    list->selectedPlugin = NULL;
    iPlugins->AppendL(list);
    CleanupStack::Pop(list);
}
	

////////////////////////////////////////////////////////////////
//
// COggPluginAdaptor
//
////////////////////////////////////////////////////////////////
COggPluginAdaptor::COggPluginAdaptor(COggMsgEnv* anEnv, MPlaybackObserver* anObserver = 0)
:CAbsPlayback(anObserver), iEnv(anEnv), iVolume(KMaxVolume), iGain(ENoGain)
{
}

TInt COggPluginAdaptor::Info(const TDesC& aFileName, TBool /*silent*/)
{
    // That's unfortunate, but we must open the file first before the info 
    // can be retrieved. This adds some serious delay when discovering the files
  
    return  Open(aFileName);
}

void COggPluginAdaptor::OpenL(const TDesC& aFileName)
{
    TState oldState = iState;
    iState = EClosed;
    if (aFileName != iFileName)
    {
        TRACEF(COggLog::VA(_L("OpenL %S"), &aFileName ));
        
        iTitle  = KNullDesC;
        iAlbum  = KNullDesC;
        iArtist = KNullDesC;
        iGenre  = KNullDesC;
        iTrackNumber = KNullDesC;

        ConstructAPlayerL( aFileName );
  
        // Find the selected plugin, corresponding to file type
        TParsePtrC p( aFileName);
        TPtrC pp (p.Ext().Mid(1));
        CPluginInfo * info = GetPluginListL().GetSelectedPluginInfo(pp);
        if (info == NULL)
        	User::Leave(KErrNotFound);
        iPluginControllerUID = info->iControllerUid;
        iPlayer->OpenFileL(aFileName, KNullUid , iPluginControllerUID );
       
        // Wait for the init completed callback
        iError = KErrNone;
#ifdef MMF_AVAILABLE
        if (!iWait.IsStarted())
        {
            iWait.Start();
        }
#endif
        User::LeaveIfError(iError);
        iFileName = aFileName;
        TInt nbMetaData;
        CMMFMetaDataEntry * aMetaData;
        TInt err = iPlayer->GetNumberOfMetaDataEntries(nbMetaData);
        if (err)
          nbMetaData = 0;
        HBufC *metadataValue = HBufC::NewLC(128);
		TPtr metadataValueDes(metadataValue->Des());
        for (TInt i=0; i< nbMetaData; i++)
        {
            aMetaData = iPlayer->GetMetaDataEntryL(i);
            ParseMetaDataValueL(*aMetaData, metadataValueDes);

			const TDesC& name(aMetaData->Name());
			const TDesC& value(aMetaData->Value());
			TRACEF(COggLog::VA(_L("MetaData %S : %S"), &name, &value ));

			if ( name  == _L("title") )
                iTitle = *metadataValue;
            if ( name  == _L("album") )
                iAlbum = *metadataValue;
            if ( name  == _L("artist") )
                iArtist = *metadataValue;
            if ( name  == _L("genre") )
                iGenre = *metadataValue;
            if ( name  == _L("albumtrack") )
                iTrackNumber = *metadataValue;

			// If it is not an OggPlay Plugin, there should be some handling here to guess
            // title, trackname, ...
            delete aMetaData;
        }
        
        CleanupStack::PopAndDestroy(metadataValue);
#if 0
        // Currently the MMF Plugin doesn't give access to these.
        // This could be added in the future as a custom command. 
        iRate= iPlayer->Rate();
        iChannels=iPlayer->Channels();
        iBitRate=iPlayer->Bitrate();
#else
        iRate= 0;
        iChannels = 0;
        iBitRate = 0;
#endif
        iState = EOpen;
        SetVolume(iVolume);
    }
    else
        iState = oldState;

	// Set the volume gain
	SetVolumeGain(iGain);
}

void COggPluginAdaptor::ParseMetaDataValueL(CMMFMetaDataEntry &aMetaData, TDes &aDestinationBuffer )
{
 // Try to remove all TABs, CR and LF
 // having those around is screwing up the SW. 
 
  HBufC * tempBuf = aMetaData.Value().AllocL();
  CleanupStack::PushL(tempBuf);
  tempBuf->Des().Zero();
  TLex parse(aMetaData.Value() );
  TBool first=ETrue;
  for ( ; ; )
  {
 	TPtrC16 token = parse.NextToken();
  	if (token.Length() <=0 )
  	   break;
  	if (!first)
   		tempBuf->Des().Append(_L(" "));
    tempBuf->Des().Append(token); 
  	first = EFalse;

  }

  // Crop to the final buffer
  aDestinationBuffer = tempBuf->Left(aDestinationBuffer.MaxLength());
  CleanupStack::PopAndDestroy(tempBuf);
  TRACEF(COggLog::VA(_L("COggPluginAdaptor::ParseMetaDataValueL !%S!"),&aDestinationBuffer));
}

TInt COggPluginAdaptor::Open(const TDesC& aFileName)
{
    TRAPD(leaveCode, OpenL(aFileName));
    return (leaveCode);
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
    TInt time = TInt(iLastPosition.Int64().GetTInt() /1E6);
    TInt hours = time/3600;
    TInt min = (time%3600)/60;
    TInt sec = (time%3600%60);
    TRACEF(COggLog::VA(_L(" %i %i %i"),hours,min,sec));
}

void COggPluginAdaptor::Play()
{
    TRACEF(_L("COggPluginAdaptor::Play() In"));
    iInterrupted = EFalse;
    if (iState == EClosed)
        return;
    TRAPD(err,iPlayer->PlayL()); 
    if (err) return; // Silently ignore the problem :-(

	iState = EPlaying;
	iObserver->ResumeUpdates();
	
	TRACEF(_L("COggPluginAdaptor::Play() Out"));
}

void COggPluginAdaptor::Stop()
{
    TRACEF(_L("COggPluginAdaptor::Stop()"));
    iPlayer->Stop();
    iFileName = KNullDesC;
    iState = EClosed;

    // The Stop is synchroneous within the MMF Framework.
    iObserver->NotifyUpdate();
}

void   COggPluginAdaptor::SetVolume(TInt aVol)
{
    TRACEF(_L("COggPluginAdaptor::SetVolume()"));
    if ( (aVol <0) || (aVol >KMaxVolume) )
    {
    return;
    }
    iVolume = aVol;
    TInt max = iPlayer->MaxVolume();
    TReal relative = ((TReal) aVol)/KMaxVolume;
    TInt vol = (TInt) (relative*max);
    iPlayer->SetVolume(vol) ;
}

void COggPluginAdaptor::SetPosition(TInt64 aPos)
{
    
    TRACEF(COggLog::VA(_L("COggPluginAdaptor::SetPosition %i"), aPos ));
    if (iPlayer)
    {
        // Do not change the position if it is bigger than the lenght of the tune,
        // it would otherwise wrap the tune to the beginning.
        if (aPos*1000 < iPlayer->Duration().Int64() ) 
        {
            iPlayer->Stop(); // Must pause before changing position. Some MMF Plugins panics otherwise.
            iPlayer->SetPosition(aPos * 1000);
            iPlayer->PlayL();
        }
    }
}

TInt64 COggPluginAdaptor::Position()
{
    TTimeIntervalMicroSeconds aPos(0);
    if (iState == EPaused)
        aPos = iLastPosition;
    else if (iPlayer)
    {
        aPos = iPlayer->Position();
        
    }
    if (aPos != TTimeIntervalMicroSeconds(0))
        iLastPosition = aPos;
    
    return(aPos.Int64()/1000); // Dividing by 1000, to get millisecs from microsecs
}

TInt64 COggPluginAdaptor::Time()
{
    if (iPlayer)
        return (iPlayer->Duration().Int64()/1000);
    return(0);
}

TInt COggPluginAdaptor::Volume()
{
    // Not Implemented yet
    return(0);
}

const TInt32* COggPluginAdaptor::GetFrequencyBins()
{
    iFreqBins[0] = 55;
    
    TMMFGetFreqsParams pckg;
    pckg.iFreqBins = iFreqBins;
    TMMFGetFreqsConfig freqConfig(pckg); // Pack the config.
    TPckg <TInt32 [16]> dataFrom(iFreqBins); 
    
    TMMFMessageDestination msg( iPluginControllerUID, KMMFObjectHandleController );
    TPckgBuf<TMMFMessageDestination> packedMsg(msg); // Pack the destination
    
    TInt error = iPlayer->PlayControllerCustomCommandSync( packedMsg, 
                                       EOggPlayControllerCCGetFrequencies, 
                                       freqConfig, 
                                       KNullDesC8, dataFrom );

	if ((error != KErrNone) && (error != KErrNotSupported))
	{
		TRACEF(COggLog::VA(_L("COggPluginAdaptor::GetFrequencyBins %i"), error ));
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
	if (iPlayer)
		err = iPlayer->PlayControllerCustomCommandSync(packedMsg, EOggPlayControllerCCSetVolumeGain, gainConfig, KNullDesC8, dataFrom);

	if (err != KErrNone)
	{
		TRACEF(COggLog::VA(_L("COggPluginAdaptor::SetVolumeGain %i"), err));
	}
}

void COggPluginAdaptor::MoscoStateChangeEvent(CBase* /*aObject*/, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode)
{
    TRACEF(COggLog::VA(_L("MoscoStateChange :%d %d %d "), aPreviousState,  aCurrentState,  aErrorCode));
    
#ifdef MMF_AVAILABLE
    if (iWait.IsStarted())
    {   
        // We should be in wait state only when opening a file.       
        // From not opened to opened
        TRACEF(COggLog::VA(_L("MoscoStateChange : InitComplete %d"), aErrorCode ));
        iError = aErrorCode;
        iWait.AsyncStop(); // Gives back the control to the waiting OpenL()
    }
#else
    if ((aPreviousState == CMdaAudioClipUtility::ENotReady) 
        && (aCurrentState == CMdaAudioClipUtility::EOpen))
    {
        // From not opened to opened
        TRACEF(COggLog::VA(_L("MoscoStateChange : InitComplete %d"), aErrorCode ));
        iError = aErrorCode;
    }
    
#endif

    if ((aCurrentState == CMdaAudioClipUtility::EPlaying) && aErrorCode)
    {
        // From opened to Playing
        // The sound device was stolen by somebody else. (A SMS arrival notice, for example).
            iInterrupted = ETrue;
            iObserver->NotifyPlayInterrupted();
    }
    
    if ((aPreviousState == CMdaAudioClipUtility::EPlaying) 
        && (aCurrentState == CMdaAudioClipUtility::EOpen))
    {
        // From Playing to stopped playing
        TRACEF(COggLog::VA(_L("MoscoStateChange : PlayComplete %d"), aErrorCode ));
        switch (aErrorCode)
        {
        case  KErrDied :
            // The sound device was stolen by somebody else. (A SMS arrival notice, for example).
            iInterrupted = ETrue;
            iObserver->NotifyPlayInterrupted();
            break;
        case KErrUnderflow:
            iState = EClosed;
            break;
        default:
            iObserver->NotifyPlayComplete();
        }
    }

   if ((aPreviousState == CMdaAudioClipUtility::EOpen) && (aCurrentState == CMdaAudioClipUtility::EOpen) && (aErrorCode == KErrNotReady))
	{
		// The plugin isn't ready so try again
	    TRAP(aErrorCode,iPlayer->PlayL()); 
	}
}
 

void COggPluginAdaptor::ConstructL()
{
	iPluginSupportedList.ConstructL();
    // iPluginInfos will be updated, if required plugin has been found
    SearchPluginsL(_L("ogg"), ETrue);
    SearchPluginsL(_L("mp3"), ETrue);
    SearchPluginsL(_L("aac"), ETrue);
    SearchPluginsL(_L("mp4"), ETrue);
    SearchPluginsL(_L("m4a"), ETrue);
    SearchPluginsL(_L("mid"), EFalse); // Disabled by default
    SearchPluginsL(_L("amr"), EFalse); // Disabled by default
}


CPluginSupportedList & COggPluginAdaptor::GetPluginListL()
{
    return iPluginSupportedList;
}


#ifdef MMF_AVAILABLE
////////////////////////////////////////////////////
// Interface using the MMF
////////////////////////////////////////////////////


COggPluginAdaptor::~COggPluginAdaptor()
{
    delete (iPlayer);
}

void COggPluginAdaptor::ConstructAPlayerL(const TDesC & /*anExtension*/)
{
    // Stupid, but true, we must delete the player when loading a new file
    // Otherwise, will panic with -15 on some HW   
    delete(iPlayer);
    iPlayer=NULL;
    iPlayer = CMdaAudioRecorderUtility::NewL(
        *this);
}

void COggPluginAdaptor::SearchPluginsL(const TDesC &anExtension, TBool isEnabled)
{
  
	iPluginSupportedList.AddExtension(anExtension); // Add this extension to the selected list
    CMMFControllerPluginSelectionParameters * cSelect = CMMFControllerPluginSelectionParameters::NewLC();
    CMMFFormatSelectionParameters * fSelect = CMMFFormatSelectionParameters::NewLC();

    fSelect->SetMatchToFileNameL(anExtension);
    cSelect->SetRequiredPlayFormatSupportL(*fSelect);
    
    RMMFControllerImplInfoArray controllers;
    CleanupResetAndDestroyPushL(controllers);
    cSelect->ListImplementationsL(controllers); 
    TInt nbFound = 0;
    
    for (TInt ii=0; ii< controllers.Count(); ii++)
    {
        // Found at least one controller !  
        const RMMFFormatImplInfoArray& formatArray = controllers[ii]->PlayFormats();
    
        TBuf8 <10> anEx8;
        anEx8.Copy(anExtension); // Convert the 16 bit descriptor to 8 bits
        
        TInt found = EFalse;
        TInt i;
        for (i=0; i<formatArray.Count(); i++)
        {            
            const CDesC8Array &extensions = formatArray[i]->SupportedFileExtensions();
            for (TInt j=0; j<extensions.Count(); j++)
            {
                if (extensions[j].FindF(anEx8) != KErrNotFound)
                {
                    found = ETrue;
                    break;
                }
            }
            if (found) 
                break;
        }
        
        __ASSERT_ALWAYS ( found, User::Panic(_L("Extension not found"), 999) );
        
        
        TRACEF(COggLog::VA(_L(".%S Plugin Found:%S %S %i %x"), &anExtension,
            &formatArray[i]->DisplayName(),
            &formatArray[i]->Supplier(),
            formatArray[i]->Version(),
            controllers[ii]->Uid()
            ));
       
        nbFound++;
        iPluginSupportedList.AddPluginL(anExtension,
            *formatArray[i],
            controllers[ii]->Uid() );
            
        if (isEnabled)
        {
           // This will be overruled by the .ini file, if the file exist.
        	iPluginSupportedList.SelectPluginL(anExtension,controllers[ii]->Uid() );
        }
    }
    
    CleanupStack::PopAndDestroy(3); /* cSelect, fSelect, controllers */

}

#else

////////////////////////////////////////////////////
// Interface when MMF is not available
////////////////////////////////////////////////////


COggPluginAdaptor::~COggPluginAdaptor()
{
     delete (iOggPlayer);
     delete (iMp3Player);
     // Finished with the DLLs
    //
     iOggLibrary.Close();
     iMp3Library.Close();
     iPluginInfos->ResetAndDestroy();
     delete (iPluginInfos);
}


void COggPluginAdaptor::SearchPluginsL(const TDesC &anExtension)
{
    CPluginInfo* info  = NULL;

    const TUid KOggPlayDecoderLibraryUid={0x101FD21D};
    if (anExtension == _L("ogg"))
    {
        if (iOggPlayer == NULL)
        {
            TUidType uidType(KDynamicLibraryUid,KOggPlayDecoderLibraryUid);
            TBool found = (iOggLibrary.Load(_L("OGGPLUGINDECODERPREMMF.DLL"), uidType) == KErrNone) ;
            if (found)
            {
                // Function at ordinal 1 creates new C
                TLibraryFunction entry=iOggLibrary.Lookup(1);
                // Call the function to create new CMessenger
                iOggPlayer = (CPseudoMMFController*) entry();
                iOggPlayer->SetObserver(*this);
                
                info = CPluginInfo::NewL(
                    anExtension,
                    _L("OGG tremor decoder"),
                    _L("OggPlay Team"),
                    1);
            }
        }
    } 
       
    if (anExtension == _L("mp3"))
    {
        if (iMp3Player == NULL)
        {
            TUidType uidType(KDynamicLibraryUid,KOggPlayDecoderLibraryUid);
            TBool found = (iMp3Library.Load(_L("MADPLUGINDECODERPREMMF.DLL"), uidType) == KErrNone) ;
            if (found)
            {
                // Function at ordinal 1 creates new C
                TLibraryFunction entry=iMp3Library.Lookup(1);
                // Call the function to create new CMessenger
                iMp3Player = (CPseudoMMFController*) entry();
                iMp3Player->SetObserver(*this);
                
                info = CPluginInfo::NewL(
                    anExtension,
                    _L("MP3 Mad decoder"),
                    _L("?"),
                    1);
            }
        }
    } 
    
    if (info) 
        iPluginInfos->AppendL(info);
}

void COggPluginAdaptor::ConstructAPlayerL(const TDesC &anExtension)
{
    // Load the plugin.
    TParsePtrC p( anExtension);
    TPtrC pp (p.Ext().Mid(1)); // Remove the . in front of the extension
       
    if ( pp.CompareF( _L("ogg") ) == 0 )
    {
        // an Ogg File
      iPlayer = iOggPlayer;
    }

    if ( pp.CompareF( _L("mp3") ) == 0 )
    {
        // an Ogg File
      iPlayer = iMp3Player;
    }
    __ASSERT_ALWAYS(iPlayer, User::Panic(_L("Couldn't construct a Player"), 999) );
}

#endif
