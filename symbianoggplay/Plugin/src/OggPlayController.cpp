/*
*  Copyright (c) 2004 
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

#if 1
#include <e32svr.h>
#define PRINT(x) RDebug::Print _L(x);
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
    iDecoder->Clear();
    CloseSTDLIB();    	// Just in case
    COggLog::Exit();  
    delete iAdvancedStreaming;
    delete iDecoder;
}

void COggPlayController::ConstructL()
{
    PRINT("COggPlayController::ConstructL()");

    // Construct custom command parsers

#ifdef MMF_AVAILABLE
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
#endif

    iState = EStateNotOpened;
    iAdvancedStreaming = new (ELeave) CAdvancedStreaming(*this);
    iAdvancedStreaming->ConstructL();
}

#ifdef MMF_AVAILABLE
///////////////////////////////////////////////////////////////////
// Functions used with the MMF Framework, but useless when MMF is not
// in use (Symbian 7.0 and earlier
///////////////////////////////////////////////////////////////////
void COggPlayController::AddDataSourceL(MDataSource& aDataSource)
{
    PRINT("COggPlayController::AddDataSourceL");

    if ( iState != EStateNotOpened )
    {
        User::Leave(KOggPlayPluginErrNotReady);
    }

    TUid uid = aDataSource.DataSourceType() ;
    if (uid == KUidMmfFileSource)
    {
        CMMFFile* aFile	= STATIC_CAST(CMMFFile*, &aDataSource);
        iFileName = aFile->FullName();
    }
    else
    {
        User::Leave(KOggPlayPluginErrNotSupported);
    }
    OpenFileL(iFileName);
}

void COggPlayController::AddDataSinkL(MDataSink& aDataSink)
{
    PRINT("COggPlayController::AddDataSinkL");
    TUid uid = aDataSink.DataSinkType() ;
    if (uid != KUidMmfAudioOutput)
    {
        User::Leave(KOggPlayPluginErrNotSupported);
    }
}

void COggPlayController::RemoveDataSourceL(MDataSource& /*aDataSource*/)
{
    PRINT("COggPlayController::RemoveDataSourceL");
    // Not implemented yet
}
void COggPlayController::RemoveDataSinkL(MDataSink& /* aDataSink*/)
{
    PRINT("COggPlayController::RemoveDataSinkL");
    // Not implemented yet
}

void COggPlayController::SetPrioritySettings(const TMMFPrioritySettings& /*aPrioritySettings*/)
{
    
    PRINT("COggPlayController::SetPrioritySettingsL");
    // Not implemented yet
}

void COggPlayController::MapdSetVolumeRampL(const TTimeIntervalMicroSeconds& aRampDuration)
{
    PRINT("COggPlayController::MapdSetVolumeRampL");
    // Not implemented yet
}
void COggPlayController::MapdSetBalanceL(TInt aBalance)
{
    PRINT("COggPlayController::MapdSetBalanceL");
    // Not implemented yet
}

void COggPlayController::MapdGetBalanceL(TInt& aBalance)
{
    PRINT("COggPlayController::MapdGetBalanceL");
    // Not implemented yet
}
void COggPlayController::MapcSetPlaybackWindowL(const TTimeIntervalMicroSeconds& aStart,
                            const TTimeIntervalMicroSeconds& aEnd)
{
    PRINT("COggPlayController::MapcSetPlaybackWindowL");
    // Not implemented yet
}
void COggPlayController::MapcDeletePlaybackWindowL()
{
    
    PRINT("COggPlayController::MapcDeletePlaybackWindowL");
    // Not implemented yet
}
void COggPlayController::MapcGetLoadingProgressL(TInt& aPercentageComplete)
{
    
    PRINT("COggPlayController::MapcGetLoadingProgressL");
    // Not implemented yet
}


#else

void COggPlayController::SetObserver(MMdaAudioPlayerCallback &anObserver)
{
    iObserver = &anObserver;
}


#endif /*MMF_AVAILABLE */

void COggPlayController::ResetL()
{
    PRINT("COggPlayController::ResetL");
    // Not implemented yet
}

void COggPlayController::PrimeL()
{
    
    PRINT("COggPlayController::PrimeL");
    // Not implemented yet
}

void COggPlayController::PlayL()
{
    
    PRINT("COggPlayController::PlayL");
    if (iState == EStateNotOpened)
        User::Leave(KErrNotReady);
    if (iState == EStatePaused)
    {
        iState = EStatePlaying;
        iAdvancedStreaming->Play();
        return;
        
    }
    if (iState != EStatePlaying)
    { 
    iDecoder->Setposition(0);
        iState = EStatePlaying;
    iAdvancedStreaming->Play();
    }
    
}

void COggPlayController::PauseL()
{
    PRINT("COggPlayController::PauseL");
    if (iState == EStatePlaying)
        iState = EStatePaused;
}

void COggPlayController::StopL()
{
    PRINT("COggPlayController::StopL");
    if (iState != EStateNotOpened)
        iState = EStateOpen;
}

TTimeIntervalMicroSeconds COggPlayController::PositionL() const
{
    PRINT("COggPlayController::PositionL");
    // Not implemented yet
    return TTimeIntervalMicroSeconds(0);
}

void COggPlayController::SetPositionL(const TTimeIntervalMicroSeconds& /*aPosition*/)
{
    
    PRINT("COggPlayController::SetPositionL");
    // Not implemented yet
}

TTimeIntervalMicroSeconds  COggPlayController::DurationL() const
{
    
    PRINT("COggPlayController::DurationL");
    
    // Not implemented yet
    return TTimeIntervalMicroSeconds(1E6);
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
        return (CMMFMetaDataEntry::NewL(_L("OggPlayPluginTitle"), iTitle));
        break;
        
    case 1:
        return (CMMFMetaDataEntry::NewL(_L("OggPlayPluginAlbum"), iAlbum));
        break;
        
    case 2:
        return (CMMFMetaDataEntry::NewL(_L("OggPlayPlugArtist"), iArtist));
        break;
        
    case 3:
        return (CMMFMetaDataEntry::NewL(_L("OggPlayPluginGenre"), iGenre));
        break;
        
    case 4:
        return (CMMFMetaDataEntry::NewL(_L("OggPlayPluginTrackNumber"),  iTrackNumber));
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
        PRINT("Oggplay: File open returns 0 (Error20 Error14)");
        User::Leave(KOggPlayPluginErrFileNotFound);
    };
    iState= EStateOpen;
    if( iDecoder->Open(iFile) < 0) {
        iDecoder->Clear();
        fclose(iFile);
        iState= EStateNotOpened;
        User::Leave(KOggPlayPluginErrOpeningFile);
    }
    
    iAdvancedStreaming->Open(iDecoder->Channels(), iDecoder->Rate());
    
      // Parse tag information and put it in the provided buffers.
    iDecoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);


} 


void COggPlayController::MapdSetVolumeL(TInt aVolume)
{
    PRINT("COggPlayController::SetVolumeL");
    iAdvancedStreaming->SetVolume(aVolume);
}

void COggPlayController::MapdGetMaxVolumeL(TInt& aMaxVolume)
{
    PRINT("COggPlayController::MapdGetMaxVolumeL");
    aMaxVolume = KMaxVolume;
}

void COggPlayController::MapdGetVolumeL(TInt& aVolume)
{
    PRINT("COggPlayController::MapdGetVolumeL");
    aVolume = iAdvancedStreaming->Volume();
}

#ifndef MMF_AVAILABLE

// Non leaving version of the MMF functions
void COggPlayController::OpenFile(const TDesC& aFile)
{
    TRAPD(error, OpenFileL(aFile));
    iObserver->MapcInitComplete(error, TTimeIntervalMicroSeconds (0) );
}

void COggPlayController::Play()
{
    TRAPD(error, PlayL());
    if (error) 
        iObserver->MapcPlayComplete(error);
}
void COggPlayController::Pause()
{   
    TRAPD(error, PauseL());
    if (error) 
        iObserver->MapcPlayComplete(error);
}
void COggPlayController::Stop()
{
    TRAPD(error, StopL());
    if (error) 
        iObserver->MapcPlayComplete(error);
}


TInt COggPlayController::MaxVolume()
{
    TInt maxvol=0;
    TRAPD(error, MapdGetMaxVolumeL(maxvol););
    // The possible leave is stupidly forgotten here
    return(maxvol);
}
void COggPlayController::SetVolume(TInt aVolume)
{
    TRAPD(error, MapdSetVolumeL(aVolume));
    // The possible leave is stupidly forgotten here
}
TInt COggPlayController::GetVolume(TInt& aVolume) 
{
    TRAPD(error, MapdGetVolumeL(aVolume));
    return(error);
}

#endif


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
    }
    return (ret);
}

void COggPlayController::NotifyPlayInterrupted(TInt aError)
{
#ifdef MMF_AVAILABLE
    DoSendEventToClient(TMMFEvent(KMMFEventCategoryPlaybackComplete, aError));
#else
   iObserver->MapcPlayComplete(aError);
#endif
}

