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

#include "OggTremorController.h"
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

COggTremorController* COggTremorController::NewL()
{
    PRINT("COggTremorController::NewL()");
    COggTremorController* self = new(ELeave) COggTremorController;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
}

COggTremorController::~COggTremorController()
{
    ov_clear(&iVf);
    CloseSTDLIB();    	
    COggLog::Exit();  
    delete iAdvancedStreaming;
}

void COggTremorController::ConstructL()
{
    PRINT("COggTremorController::ConstructL()");
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
void COggTremorController::AddDataSourceL(MDataSource& aDataSource)
{
    PRINT("COggTremorController::AddDataSourceL");

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

void COggTremorController::AddDataSinkL(MDataSink& aDataSink)
{
    PRINT("COggTremorController::AddDataSinkL");
    TUid uid = aDataSink.DataSinkType() ;
    if (uid != KUidMmfAudioOutput)
    {
        User::Leave(KOggPlayPluginErrNotSupported);
    }
}

void COggTremorController::RemoveDataSourceL(MDataSource& /*aDataSource*/)
{
    PRINT("COggTremorController::RemoveDataSourceL");
    // Not implemented yet
}
void COggTremorController::RemoveDataSinkL(MDataSink& /* aDataSink*/)
{
    PRINT("COggTremorController::RemoveDataSinkL");
    // Not implemented yet
}

void COggTremorController::SetPrioritySettings(const TMMFPrioritySettings& /*aPrioritySettings*/)
{
    
    PRINT("COggTremorController::SetPrioritySettingsL");
    // Not implemented yet
}

#else

void COggTremorController::SetObserver(MMdaAudioPlayerCallback &anObserver)
{
    iObserver = &anObserver;
}


#endif /*MMF_AVAILABLE */

void COggTremorController::ResetL()
{
    PRINT("COggTremorController::ResetL");
    // Not implemented yet
}

void COggTremorController::PrimeL()
{
    
    PRINT("COggTremorController::PrimeL");
    // Not implemented yet
}

void COggTremorController::PlayL()
{
    
    PRINT("COggTremorController::PlayL");
    if (!iFileOpen)
        User::Leave(KErrNotReady);
    iAdvancedStreaming->Play();
}

void COggTremorController::PauseL()
{
    PRINT("COggTremorController::PauseL");
    // Not implemented yet
}

void COggTremorController::StopL()
{
    PRINT("COggTremorController::StopL");
    // Not implemented yet
}

TTimeIntervalMicroSeconds COggTremorController::PositionL() const
{
    PRINT("COggTremorController::PositionL");
    // Not implemented yet
    return NULL;
}

void COggTremorController::SetPositionL(const TTimeIntervalMicroSeconds& /*aPosition*/)
{
    
    PRINT("COggTremorController::SetPositionL");
    // Not implemented yet
}

TTimeIntervalMicroSeconds  COggTremorController::DurationL() const
{
    
    PRINT("COggTremorController::DurationL");
    
    // Not implemented yet
    return TTimeIntervalMicroSeconds(1E6);
}


void COggTremorController::GetNumberOfMetaDataEntriesL(TInt& /*aNumberOfEntries*/)
{
    
    PRINT("COggTremorController::GetNumberOfMetaDataEntriesL");
    // Not implemented yet
}

CMMFMetaDataEntry* COggTremorController::GetMetaDataEntryL(TInt /*aIndex*/)
{
    
    PRINT("COggTremorController::GetMetaDataEntryL");
    // Not implemented yet
    return NULL;
}

void COggTremorController::OpenFileL(const TDesC& aFile)
{
    
    // add a zero terminator
    TBuf<512> myname(aFile);
    myname.Append((wchar_t)0);
    
    if ((iFile=wfopen((wchar_t*)myname.Ptr(),L"rb"))==NULL) {
        iFileOpen= EFalse;
        PRINT("Oggplay: File open returns 0 (Error20 Error14)");
        User::Leave(KOggPlayPluginErrFileNotFound);
    };
    iFileOpen= ETrue;
    iEof = EFalse;
    
    if(ov_open(iFile, &iVf, NULL, 0) < 0) {
        ov_clear(&iVf);
        fclose(iFile);
        iFileOpen= EFalse;
        User::Leave(KOggPlayPluginErrOpeningFile);
    }
    
    ParseComments(ov_comment(&iVf,-1)->user_comments);
    
    vorbis_info *vi= ov_info(&iVf,-1);
    
    iRate= vi->rate;
    iChannels= vi->channels;
    iFileSize= (TInt) ov_raw_total(&iVf,-1);
    
    unsigned int hi(0);
    ogg_int64_t bitrate= vi->bitrate_nominal;
    iBitRate.Set(hi,(TInt) bitrate);
    iState= EStatePrepared;
    iAdvancedStreaming->Open(iChannels, iRate);
} 


#ifndef MMF_AVAILABLE

// Non leaving version of the MMF functions
void COggTremorController::OpenFile(const TDesC& aFile)
{
    TRAPD(error, OpenFileL(aFile));
    iObserver->MapcInitComplete(error, TTimeIntervalMicroSeconds (0) );
}

void COggTremorController::Play()
{
    TRAPD(error, PlayL());
    if (error) 
        iObserver->MapcPlayComplete(error);
}
void COggTremorController::Pause()
{   
    TRAPD(error, PauseL());
    if (error) 
        iObserver->MapcPlayComplete(error);
}
void COggTremorController::Stop()
{
    TRAPD(error, StopL());
    if (error) 
        iObserver->MapcPlayComplete(error);
}

#endif


TInt COggTremorController::GetNewSamples(TDes8 &aBuffer) 
{
    if (iEof)
        return(0);

    TInt len = aBuffer.Length();
    TInt ret=ov_read( &iVf,(char *) &(aBuffer.Ptr()[len]),
        aBuffer.MaxLength()-len
        ,&iCurrentSection);
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

void COggTremorController::NotifyPlayInterrupted(TInt aError)
{
#ifdef MMF_AVAILABLE
    DoSendEventToClient(TMMFEvent(KMMFEventCategoryPlaybackComplete, aError));
#else
   iObserver->MapcPlayComplete(aError);
#endif
}


void COggTremorController::ParseComments(char** ptr)
{
  ClearComments();
  while (*ptr) {
    char* s= *ptr;
    TBuf<256> buf;
    GetString(buf,s);
    buf.UpperCase();
    if (buf.Find(_L("ARTIST="))==0) GetString(iArtist, s+7); 
    else if (buf.Find(_L("TITLE="))==0) GetString(iTitle, s+6);
    else if (buf.Find(_L("ALBUM="))==0) GetString(iAlbum, s+6);
    else if (buf.Find(_L("GENRE="))==0) GetString(iGenre, s+6);
    else if (buf.Find(_L("TRACKNUMBER="))==0) GetString(iTrackNumber,s+12);
    ++ptr;
  }
}

void COggTremorController::ClearComments()
{
  iArtist.SetLength(0);
  iTitle.SetLength(0);
  iAlbum.SetLength(0);
  iGenre.SetLength(0);
  iTrackNumber.SetLength(0);
  iFileName.SetLength(0);
}

void COggTremorController::GetString(TBuf<256>& aBuf, const char* aStr)
{
  // according to ogg vorbis specifications, the text should be UTF8 encoded
  TPtrC8 p((const unsigned char *)aStr);
  CnvUtfConverter::ConvertToUnicodeFromUtf8(aBuf,p);

  // If not UTF8, too bad.
}