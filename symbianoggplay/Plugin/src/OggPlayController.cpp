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
    // Normally, custom command parser would be initialized here.
    // But for now, no custom command parser
    iFileOpen = EFalse;
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

    if ( iFileOpen )
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
    if (!iFileOpen)
        User::Leave(KErrNotReady);
    iDecoder->Setposition(0);
    iAdvancedStreaming->Play();
}

void COggPlayController::PauseL()
{
    PRINT("COggPlayController::PauseL");
    // Not implemented yet
}

void COggPlayController::StopL()
{
    PRINT("COggPlayController::StopL");
    // Not implemented yet
}

TTimeIntervalMicroSeconds COggPlayController::PositionL() const
{
    PRINT("COggPlayController::PositionL");
    // Not implemented yet
    return NULL;
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


void COggPlayController::GetNumberOfMetaDataEntriesL(TInt& /*aNumberOfEntries*/)
{
    
    PRINT("COggPlayController::GetNumberOfMetaDataEntriesL");
    // Not implemented yet
}

CMMFMetaDataEntry* COggPlayController::GetMetaDataEntryL(TInt /*aIndex*/)
{
    
    PRINT("COggPlayController::GetMetaDataEntryL");
    // Not implemented yet
    return NULL;
}

void COggPlayController::OpenFileL(const TDesC& aFile)
{
    
    // add a zero terminator
    TBuf<512> myname(aFile);
    myname.Append((wchar_t)0);
    
    if ((iFile=wfopen((wchar_t*)myname.Ptr(),L"rb"))==NULL) {
        iFileOpen= EFalse;
        PRINT("Oggplay: File open returns 0 (Error20 Error14)");
        User::Leave(KOggPlayPluginErrFileNotFound);
    };
    iFileOpen = ETrue;
    if( iDecoder->Open(iFile) < 0) {
        iDecoder->Clear();
        fclose(iFile);
        iFileOpen= EFalse;
        User::Leave(KOggPlayPluginErrOpeningFile);
    }
    
    iAdvancedStreaming->Open(iDecoder->Channels(), iDecoder->Rate());
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

#endif


TInt COggPlayController::GetNewSamples(TDes8 &aBuffer) 
{
    if (iEof)
        return(0);

    TInt len = aBuffer.Length();
    TInt ret=iDecoder->Read(aBuffer, len);
    if (ret >0)
    {
        aBuffer.SetLength(len + ret);
    }
    if (ret == 0)
    {
        iEof= 1;
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

