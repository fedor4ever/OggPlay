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
#include "OggLog.h"

////////////////////////////////////////////////////////////////
//
// COggPluginAdaptor
//
////////////////////////////////////////////////////////////////

COggPluginAdaptor::COggPluginAdaptor(COggMsgEnv* anEnv, MPlaybackObserver* anObserver=0) :
CAbsPlayback(anObserver),
iEnv(anEnv)
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
    
        ConstructAPlayerL();

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
            if ( aMetaData->Name()  == _L("OggPlayPluginTitle") )
                iTitle = aMetaData->Value();
            if ( aMetaData->Name()  == _L("OggPlayPluginAlbum") )
                iAlbum = aMetaData->Value();
            if ( aMetaData->Name()  == _L("OggPlayPluginArtist") )
                iArtist = aMetaData->Value();
            if ( aMetaData->Name()  == _L("OggPlayPluginGenre") )
                iGenre = aMetaData->Value();
            if ( aMetaData->Name()  == _L("OggPlayPluginTrackNumber") )
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
void COggPluginAdaptor::Play()
{
    TRACEF(_L("COggPluginAdaptor::Play() In"));
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
    if (iPlayer)
        iPlayer->SetPosition(aPos);
}

TInt64 COggPluginAdaptor::Position()
{
    TTimeIntervalMicroSeconds aPos = 0;
    if (iPlayer)
       iPlayer->GetPosition(aPos);
    return(aPos.Int64());
}

TInt64 COggPluginAdaptor::Time()
{
    if (iPlayer)
        return (iPlayer->Duration().Int64());
    return(0);
}

TInt COggPluginAdaptor::Volume()
{
    // Not Implemented yet
    return(0);
}

const TInt32 * COggPluginAdaptor::GetFrequencyBins(TTime aTime)
{
    // Not Implemented yet
    return NULL;
}

void COggPluginAdaptor::SetVolumeGain(TGainType aGain)
{
    // Not Implemented yet
}



void COggPluginAdaptor::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration)
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
    iObserver->NotifyPlayComplete();
}

#ifdef MMF_AVAILABLE
////////////////////////////////////////////////////
// Interface using the MMF
////////////////////////////////////////////////////

void COggPluginAdaptor::ConstructL()
    {
    RDebug::Print(_L("Leaving constructl"));
    iFilename = _L("C:\\test.ogg");
    }

COggPluginAdaptor::~COggPluginAdaptor()
{
    delete (iPlayer);
}

void COggPluginAdaptor::ConstructAPlayerL()
{
    // Stupid, but true, we must delete the player when loading a new file
    // Otherwise, will panic with -15 on some HW   
    delete(iPlayer);
    iPlayer=NULL;
    iPlayer = CMdaAudioPlayerUtility::NewL(
        *this);
}
#else

////////////////////////////////////////////////////
// Interface when MMF is not available
////////////////////////////////////////////////////

void COggPluginAdaptor::ConstructL()
{
}

COggPluginAdaptor::~COggPluginAdaptor()
{
     delete (iPlayer);
    // Finished with the DLL
    //
    iLibrary.Close();
}

void COggPluginAdaptor::ConstructAPlayerL()
{
    // Load the plugin.
    // Dynamically load the DLL
    if (iPlayer == NULL)
    {
        const TUid KOggDecoderLibraryUid={0x101FD21D};
        TUidType uidType(KDynamicLibraryUid,KOggDecoderLibraryUid);
        User::LeaveIfError ( iLibrary.Load(_L("OGGPLUGINDECODERPREMMF.DLL"), uidType) );
        
        // Function at ordinal 1 creates new C
        TLibraryFunction entry=iLibrary.Lookup(1);
        // Call the function to create new CMessenger
        iPlayer = (CPseudoMMFController*) entry();
        iPlayer->SetObserver(*this);
    }
}

#endif
