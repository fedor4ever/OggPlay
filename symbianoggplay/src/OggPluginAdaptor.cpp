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



////////////////////////////////////////////////////////////////
//
// CPluginInfo
//
////////////////////////////////////////////////////////////////

CPluginInfo::CPluginInfo()
  {
  }

CPluginInfo* CPluginInfo::NewL(const TDesC& anExtension, 
	   const TDesC& aName,
	   const TDesC& aSupplier,
	   const TInt aVersion)
  {
  CPluginInfo* self = new (ELeave) CPluginInfo();
  CleanupStack::PushL(self);
  self->ConstructL(anExtension, aName, aSupplier, aVersion);
  CleanupStack::Pop(); // self
  return self;
  }


void
CPluginInfo::ConstructL( const TDesC& anExtension, 
	   const TDesC& aName,
	   const TDesC& aSupplier,
	   const TInt aVersion ) 
{
    iExtension = anExtension.AllocL();
    iName = aName.AllocL();
    iSupplier = aSupplier.AllocL();
    iVersion = aVersion;
}

CPluginInfo::~CPluginInfo()
{
    if ( iExtension) delete iExtension;
    if ( iName )     delete iName;
    if ( iSupplier ) delete iSupplier;
}


////////////////////////////////////////////////////////////////
//
// COggPluginAdaptor
//
////////////////////////////////////////////////////////////////

COggPluginAdaptor::COggPluginAdaptor(COggMsgEnv* anEnv, MPlaybackObserver* anObserver=0) :
CAbsPlayback(anObserver),
iEnv(anEnv), 
iPluginInfos(0)
{
}

TInt COggPluginAdaptor::Info(const TDesC& aFileName, TBool /*silent*/)
{
    // That's unfortunate, but we must open the file first before the info 
    // can be retrieved. This adds some series delay when discovering the files
  
    return  Open(aFileName);
}

void COggPluginAdaptor::OpenL(const TDesC& aFileName)
{
    TState oldState = iState;
    iState = EClosed;
    if (aFileName != iFileName)
    {
    TRACEF(COggLog::VA(_L("OpenL %S"), &aFileName ));
    
        ConstructAPlayerL( aFileName );
        iPlayer->OpenFileL(aFileName);
        // Wait for the init completed callback
        iError = KErrNone;
#ifdef MMF_AVAILABLE
        CActiveScheduler::Start(); // This is not good symbian practise, but hey! 
                                   // It's simple and working :-)
#endif
        User::LeaveIfError(iError);
        iFileName = aFileName;
        TInt nbMetaData;
        CMMFMetaDataEntry * aMetaData;
        User::LeaveIfError( iPlayer->GetNumberOfMetaDataEntries(nbMetaData));
        for (TInt i=0; i< nbMetaData; i++)
        {
            aMetaData = iPlayer->GetMetaDataEntryL(i);
            TRACEF(COggLog::VA(_L("MetaData %S : %S"), &aMetaData->Name(), &aMetaData->Value() ));
            if ( aMetaData->Name()  == _L("title") )
                iTitle = aMetaData->Value();
            if ( aMetaData->Name()  == _L("album") )
                iAlbum = aMetaData->Value();
            if ( aMetaData->Name()  == _L("artist") )
                iArtist = aMetaData->Value();
            if ( aMetaData->Name()  == _L("genre") )
                iGenre = aMetaData->Value();
            if ( aMetaData->Name()  == _L("albumtrack") )
                iTrackNumber = aMetaData->Value();
            // If it is not an OggPlay Plugin, there should be some handling here to guess
            // title, trackname, ...
            delete(aMetaData);
        }
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
        iTime = 5; // FIXME!
    }
    else
        iState = oldState;
}

TInt COggPluginAdaptor::Open(const TDesC& aFileName)
{
    TRAPD(leaveCode, OpenL(aFileName));
    return (leaveCode);
}

void COggPluginAdaptor::Pause()
{
    TRACEF(_L("COggPluginAdaptor::Pause()"));
    iPlayer->Pause();
    iState = EPaused;
}

void COggPluginAdaptor::Resume()
{
    TRACEF(_L("COggPluginAdaptor::Resume()"));
    TBool wasInterrupted = iInterrupted;  
    if (wasInterrupted)
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
    iPlayer->Play();
    iState = EPlaying;
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
    TInt max = iPlayer->MaxVolume();
    TReal relative = ((TReal) aVol)/KMaxVolume;
    TInt vol = (TInt) (relative*max);
    iPlayer->SetVolume(vol) ;
}

void   COggPluginAdaptor::SetPosition(TInt64 aPos)
{
    TRACEF(_L("COggPluginAdaptor::SetPosition"));
    if (iPlayer)
        iPlayer->SetPosition(aPos * 1000);
}

TInt64 COggPluginAdaptor::Position()
{
    TTimeIntervalMicroSeconds aPos(0);
    if (iPlayer)
       iPlayer->GetPosition(aPos);
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

const TInt32 * COggPluginAdaptor::GetFrequencyBins(TTime /*aTime*/)
{
    // Not Implemented yet
    return NULL;
}

void COggPluginAdaptor::SetVolumeGain(TGainType /*aGain*/)
{
    // Not Implemented yet
}



void COggPluginAdaptor::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& /*aDuration*/)
{
    iError = aError;    
    TRACEF(COggLog::VA(_L("MapcInitComplete %d"), aError ));
#ifdef MMF_AVAILABLE
    CActiveScheduler::Stop(); // Gives back the control to the waiting OpenL()
#endif
}

void COggPluginAdaptor::MapcPlayComplete(TInt aError)
{
    TRACEF(COggLog::VA(_L("MapcPlayComplete %d"), aError ));
    if (aError == KErrDied)
    {
        // The sound device was stolen by somebody else. (A SMS arrival notice, for example).
        iObserver->NotifyPlayInterrupted();
        iInterrupted = ETrue;
    }
    else
    iObserver->NotifyPlayComplete();
}

void COggPluginAdaptor::ConstructL()
{
    RDebug::Print(_L("Leaving constructl"));
    
    // Discovery of the supported formats
    iPluginInfos = new(ELeave) CArrayPtrFlat<CPluginInfo>(3);
    // iPluginInfos will be updated, if required plugin has been found
    SearchPluginsL(_L("ogg"));
    SearchPluginsL(_L("mp3"));
    SearchPluginsL(_L("aac"));
}

CArrayPtrFlat <CPluginInfo> & COggPluginAdaptor::PluginList()
{
    return *iPluginInfos;
}

#ifdef MMF_AVAILABLE
////////////////////////////////////////////////////
// Interface using the MMF
////////////////////////////////////////////////////


COggPluginAdaptor::~COggPluginAdaptor()
{
    delete (iPlayer);
    iPluginInfos->ResetAndDestroy();
    delete(iPluginInfos);
}

void COggPluginAdaptor::ConstructAPlayerL(const TDesC & /*anExtension*/)
{
    // Stupid, but true, we must delete the player when loading a new file
    // Otherwise, will panic with -15 on some HW   
    delete(iPlayer);
    iPlayer=NULL;
    iPlayer = CMdaAudioPlayerUtility::NewL(
        *this);
}

void COggPluginAdaptor::SearchPluginsL(const TDesC &anExtension)
{
    CMMFControllerPluginSelectionParameters * cSelect = CMMFControllerPluginSelectionParameters::NewLC();
    CMMFFormatSelectionParameters * fSelect = CMMFFormatSelectionParameters::NewLC();

    fSelect->SetMatchToFileNameL(anExtension);
    cSelect->SetRequiredPlayFormatSupportL(*fSelect);
    
    RMMFControllerImplInfoArray controllers;
    CleanupResetAndDestroyPushL(controllers);
    cSelect->ListImplementationsL(controllers); 
    
    if (controllers.Count() >0 )
    {
        // Found at least one controller !  
        // Take the first one for the time being. At some time, users should be able to select 
        // their favorite one...
        const RMMFFormatImplInfoArray& formatArray = controllers[0]->PlayFormats();
    
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
        
        
        TRACEF(COggLog::VA(_L(".%S Plugin Found:%S %S %i"), &anExtension,
            &formatArray[i]->DisplayName(),
            &formatArray[i]->Supplier(),
            formatArray[i]->Version() ));
        CPluginInfo* info = CPluginInfo::NewL(
            anExtension,
            formatArray[i]->DisplayName(),
            formatArray[i]->Supplier(),
            formatArray[i]->Version());
        
        iPluginInfos->AppendL(info);
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
