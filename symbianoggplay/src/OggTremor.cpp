/*
 *  Copyright (c) 2003 L. H. Wilden.
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

#ifdef __VC32__
#pragma warning( disable : 4244 ) // conversion from __int64 to unsigned int: Possible loss of data
#endif

#include "OggOs.h"
#include "OggLog.h"
#include "OggTremor.h"

#include <barsread.h>
#include <eikbtpan.h>
#include <eikcmbut.h>
#include <eiklabel.h>
#include <eikmover.h>
#include <eikbtgpc.h>
#include <eikon.hrh>
#include <eikon.rsg>
#include <charconv.h>

#include <utf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ivorbiscodec.h"
#include "ivorbisfile.h"

#if !defined(__VC32__)
#include "autoconvert.c"
#endif

#include <OggPlay.rsg>
////////////////////////////////////////////////////////////////
//
// CAbsPlayback
//
////////////////////////////////////////////////////////////////

CAbsPlayback::CAbsPlayback(MPlaybackObserver* anObserver) :
  iState(CAbsPlayback::EClosed),
  iTitle(),
  iAlbum(),
  iArtist(),
  iGenre(),
  iTrackNumber(),
  iTime(0),
  iRate(44100),
  iChannels(2),
  iFileSize(0),
  iBitRate(0),
  iObserver(anObserver)
{
}

void
CAbsPlayback::SetObserver(MPlaybackObserver* anObserver)
{
  iObserver= anObserver;
}

void CAbsPlayback::ClearComments()
{
  iArtist.SetLength(0);
  iTitle.SetLength(0);
  iAlbum.SetLength(0);
  iGenre.SetLength(0);
  iTrackNumber.SetLength(0);
  iFileName.SetLength(0);
}

TInt CAbsPlayback::Rate()
{ 
  return iRate;
}

TInt CAbsPlayback::Channels()
{
  return iChannels;
}

TInt CAbsPlayback::FileSize()
{
  return iFileSize;
}

TInt CAbsPlayback::BitRate()
{
  return iBitRate.GetTInt();
}

const TFileName& CAbsPlayback::FileName()
{
  return iFileName;
}

const TDesC& CAbsPlayback::Artist()
{
  return iArtist;
}

const TDesC& CAbsPlayback::Album()
{
  return iAlbum;
}

const TDesC& CAbsPlayback::Title()
{
  return iTitle;
}

const TDesC& CAbsPlayback::Genre()
{
  return iGenre;
}

const TDesC& CAbsPlayback::TrackNumber()
{
  return iTrackNumber;
}

CAbsPlayback::TState CAbsPlayback::State()
{
  return iState;
}


////////////////////////////////////////////////////////////////
//
// COggPlayback
//
////////////////////////////////////////////////////////////////

COggPlayback::COggPlayback(CEikonEnv* anEnv, MPlaybackObserver* anObserver ) : 
  CAbsPlayback(anObserver),
  iEnv(anEnv),
  iSettings(),
  iSentIdx(0),
  iVf(),
  iFile(0),
  iFileOpen(0),
  iEof(0),
  iBufCount(0),
  iCurrentSection(0)
{
  iSettings.iChannels  = TMdaAudioDataSettings::EChannelsStereo;
  iSettings.iSampleRate= TMdaAudioDataSettings::ESampleRate44100Hz;
  iSettings.iMaxVolume = 100;
  iSettings.iVolume    = 100;
};

void COggPlayback::ConstructL() {

  iStream  = CMdaAudioOutputStream::NewL(*this);
  
  TRAPD( err, iStream->Open(&iSettings) );
  if (err!= KErrNone) {
    TBuf<256> buf1,buf2;
    iEnv->ReadResource(buf1, R_OGG_ERROR_7);
    iEnv->ReadResource(buf2, R_OGG_ERROR_20);
    buf1.AppendNum(err);
    iEnv->InfoWinL(buf2,buf1);
    //OGGLOG.Write(buf2);
    //OGGLOG.Write(buf1);
  }

  iMaxVolume=iStream->MaxVolume();
  //OGGLOG.WriteFormat(_L("Max volume is %d"),iMaxVolume);
  TDes8* buffer;

  for (TInt i=0; i<KBuffers; i++) {
    buffer = new(ELeave) TBuf8<KBufferSize>;
    buffer->SetMax();
    CleanupStack::PushL(buffer);
    User::LeaveIfError(iBuffer.Append(buffer));
    CleanupStack::Pop(buffer);
  }

}

COggPlayback::~COggPlayback() {
  delete iStream;
  iBuffer.ResetAndDestroy();
}

TInt COggPlayback::Open(const TDesC& aFileName)
{

  if (iFileOpen) {
    ov_clear(&iVf);
    fclose(iFile);
    iFileOpen= 0;
  }

  iTime= 0;

  if (aFileName.Length() == 0) {
    //OGGLOG.Write(_L("Oggplay: Filenamelength is 0 (Error20 Error8"));
    iEnv->InfoWinL(R_OGG_ERROR_20,R_OGG_ERROR_8);
    return -100;
  }

  // add a zero terminator
  TBuf<512> myname(aFileName);
  myname.Append((wchar_t)0);

  if ((iFile=wfopen((wchar_t*)myname.Ptr(),L"rb"))==NULL) {
    iFileOpen= 0;
    //OGGLOG.Write(_L("Oggplay: File open returns 0 (Error20 Error14)"));
    iEnv->InfoWinL(R_OGG_ERROR_20,R_OGG_ERROR_14);
    return -101;
  };
  iFileOpen= 1;
  
  if(ov_open(iFile, &iVf, NULL, 0) < 0) {
    ov_clear(&iVf);
    fclose(iFile);
    iFileOpen= 0;
    //OGGLOG.Write(_L("Oggplay: ov_open not successful (Error20 Error9)"));
    iEnv->InfoWinL(R_OGG_ERROR_20,R_OGG_ERROR_9);
    return -102;
  }

  ParseComments(ov_comment(&iVf,-1)->user_comments);

  iFileName= aFileName;


  vorbis_info *vi= ov_info(&iVf,-1);

  iRate= vi->rate;
  iChannels= vi->channels;
  iFileSize= ov_raw_total(&iVf,-1);

  TInt err= SetAudioCaps(vi->channels,vi->rate);
  if (err==KErrNone) {
    unsigned int hi(0);
    ogg_int64_t bitrate= vi->bitrate_nominal;
    iBitRate.Set(hi,bitrate);
    iState= EOpen;
    return KErrNone;
  }
    
  ov_clear(&iVf);
  fclose(iFile);
  iFileOpen=0;

  TBuf<256> buf,tbuf;
  iEnv->ReadResource(buf, R_OGG_ERROR_16);
  iEnv->ReadResource(tbuf, R_OGG_ERROR_20);
  buf.AppendNum(err);
  iEnv->InfoWinL(tbuf,buf);
  //OGGLOG.Write(_L("Oggplay: Error on setaudiocaps (Error16 Error20)"));
  //OGGLOG.WriteFormat(_L("Maybe audio format not supported (%d channels, %l Hz)"),vi->channels,vi->rate);
  //OGGLOG.Write(tbuf);
  //OGGLOG.Write(buf);
  return err;
}

void COggPlayback::GetString(TBuf<256>& aBuf, const char* aStr)
{
  // according to ogg vorbis specifications, the text should be UTF8 encoded
  TPtrC8 p((const unsigned char *)aStr);
  CnvUtfConverter::ConvertToUnicodeFromUtf8(aBuf,p);

  // in the real world there are all kinds of coding being used!
  // so we try to find out what it is:
#if !defined(__VC32__)
  TInt i= j_code((char*)aStr,strlen(aStr));
  if (i==BIG5_CODE) {
    CCnvCharacterSetConverter* conv= CCnvCharacterSetConverter::NewL();
    RFs theRFs;
    theRFs.Connect();
    CCnvCharacterSetConverter::TAvailability a= conv->PrepareToConvertToOrFromL(KCharacterSetIdentifierBig5, theRFs);
    if (a==CCnvCharacterSetConverter::EAvailable) {
      TInt theState= CCnvCharacterSetConverter::KStateDefault;
      conv->ConvertToUnicode(aBuf, p, theState);
    }
    theRFs.Close();
    delete conv;
  }
#endif
}

void COggPlayback::ParseComments(char** ptr)
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

TInt COggPlayback::SetAudioCaps(TInt theChannels, TInt theRate)
{
  TMdaAudioDataSettings::TAudioCaps ac;
  TMdaAudioDataSettings::TAudioCaps rt;

  if (theChannels==1) ac= TMdaAudioDataSettings::EChannelsMono;
  else if (theChannels==2) ac= TMdaAudioDataSettings::EChannelsStereo;
  else {
    iEnv->InfoWinL(R_OGG_ERROR_12,R_OGG_ERROR_10);
    //OGGLOG.Write(_L("Illegal number of channels"));
    return -100;
  }

  if (theRate==8000) rt= TMdaAudioDataSettings::ESampleRate8000Hz;
  else if (theRate==11025) rt= TMdaAudioDataSettings::ESampleRate11025Hz;
  else if (theRate==16000) rt= TMdaAudioDataSettings::ESampleRate16000Hz;
  else if (theRate==22050) rt= TMdaAudioDataSettings::ESampleRate22050Hz;
  else if (theRate==32000) rt= TMdaAudioDataSettings::ESampleRate32000Hz;
  else if (theRate==44100) rt= TMdaAudioDataSettings::ESampleRate44100Hz;
  else if (theRate==48000) rt= TMdaAudioDataSettings::ESampleRate48000Hz;
  else {
    iEnv->InfoWinL(R_OGG_ERROR_12, R_OGG_ERROR_13);
    return -101;
  }

  TRAPD( error, iStream->SetAudioPropertiesL(rt, ac) );

  return error;
}

TInt COggPlayback::Volume()
{
  if (!iStream) return 100;
  TInt vol=100;
  TRAPD( err, vol=iStream->Volume() );
  return vol;
}

void COggPlayback::SetVolume(TInt aVol)
{
  if (!iStream) return;
  if (aVol>100) aVol= 100;
  else if(aVol<0) aVol= 0;
  TRAPD( err, iStream->SetVolume(aVol) );
}

void COggPlayback::SetPosition(TInt64 aPos)
{
  if (!iStream) return;
  ogg_int64_t pos= aPos.GetTInt();
  ov_time_seek( &iVf, pos);
}

TInt64 COggPlayback::Position()
{
  if (!iStream) return TInt64(0);
  ogg_int64_t pos= ov_time_tell(&iVf);
  unsigned int hi(0);
  return TInt64(hi,pos);
}

TInt64 COggPlayback::Time()
{
  if (iTime==0) {
    ogg_int64_t pos(0);
    pos= ov_time_total(&iVf,-1);
    unsigned int hi(0);
    iTime.Set(hi,pos);
  }
  return iTime;
}

const void* COggPlayback::GetDataChunk()
{
  TInt idx= iSentIdx;
  return iSent[idx]->Ptr();
}

TInt COggPlayback::Info(const TDesC& aFileName, TBool silent)
{
  if (aFileName.Length()==0) return -100;

  FILE* f;

  // add a zero terminator
  TBuf<512> myname(aFileName);
  myname.Append((wchar_t)0);

  if ((f=wfopen((wchar_t*)myname.Ptr(),L"rb"))==NULL) {
    if (!silent) {
      TBuf<128> buf;
      iEnv->ReadResource(buf, R_OGG_ERROR_14);
      iEnv->InfoWinL(buf,aFileName);
    }
    return -101;
  };
  OggVorbis_File vf;
  if(ov_open(f, &vf, NULL, 0) < 0) {
    fclose(f);
    if (!silent) iEnv->InfoWinL(R_OGG_ERROR_20,R_OGG_ERROR_9);
    return -102;
  }

  ParseComments(ov_comment(&vf,-1)->user_comments);

  iFileName= aFileName;

  vorbis_info *vi= ov_info(&vf,-1);
  iRate= vi->rate;
  iChannels= vi->channels;
  iFileSize= ov_raw_total(&vf,-1);

  ogg_int64_t pos(0);
  pos= ov_time_total(&vf,-1);
  unsigned int hi(0);
  iTime.Set(hi,pos);

  pos= vi->bitrate_nominal;
  iBitRate.Set(hi,pos);

  ov_clear(&vf);
  fclose(f);
 
  return KErrNone;
}

void COggPlayback::Play() 
{
  if (iState == EPaused) {
    iState= EPlaying;
    for (TInt i=0; i<KBuffers; i++) SendBuffer(*iBuffer[i]);
    return;
  }

  if (iState != EOpen) {
    iEnv->InfoWinL(R_OGG_ERROR_20,R_OGG_ERROR_15);
    RDebug::Print(_L("Oggplay: Tremor - State not Open"));
    return;
  }

  iState = EPlaying;
  iEof=0;
  iBufCount= 0;
  iCurrentSection= 0;

  for (TInt i=0; i<KBuffers; i++) SendBuffer(*iBuffer[i]);
}

void COggPlayback::Pause()
{
  if (iState != EPlaying) return;
  iState= EPaused;
  TRAPD( err, iStream->Stop() );
}

void COggPlayback::Stop()
{
  TRAPD( err, iStream->Stop() );
  iState= EClosed;
  if (iFileOpen) {
    ov_clear(&iVf);
    fclose(iFile);
    iFileOpen= 0;
  }
  ClearComments();
  iTime= 0;
  iEof= 0;

  /*
  // wait until all buffers are played (or 0.5 sec max):
  int n=0;
  iPlayComplete= 0;
  while (n<5 && !iPlayComplete) {
    User::After(TTimeIntervalMicroSeconds32(100000));
    n++;
  }
  */

  if (iObserver) iObserver->NotifyUpdate();
}

void COggPlayback::SendBuffer(TDes8& buf)
{
  if (iEof) return;
  if (iState==EPaused) return;

  long ret=0;

  buf.SetLength(KBufferSize);
  ret= ov_read(&iVf,(char*) buf.Ptr(), KBufferSize, &iCurrentSection);

  if (ret == 0) {
    iEof= 1;
  } else if (ret < 0) {
    // Error in the stream. Bad luck, we will continue anyway.
  } else {
    iBufCount++;
    buf.SetLength(ret);
    TRAPD( err, iStream->WriteL(buf) );
    if (err!=KErrNone) {
      // This should never ever happen.
      TBuf<256> cbuf,tbuf;
      iEnv->ReadResource(cbuf,R_OGG_ERROR_16);
      iEnv->ReadResource(tbuf,R_OGG_ERROR_17);
      cbuf.AppendNum(err);
      iEnv->InfoWinL(tbuf,cbuf);
      User::Leave(err);
    }
    iSent[iSentIdx]= &buf;
    iSentIdx++;
    if (iSentIdx>=KBuffers) iSentIdx=0;
  }
}

void COggPlayback::MaoscPlayComplete(TInt aError)
{
  // Error codes:
  // KErrCancel      -3
  // KErrUnderflow  -10
  // KErrInUse      -14

  if (iState==EPaused || iState==EClosed) return;

  if (aError == KErrUnderflow) aError= KErrNone; // no more buffers coming, the stream ends
  if (aError == KErrInUse) aError= KErrNone;

  if (aError == KErrCancel) return;

  if (aError != KErrNone) {
    TBuf<256> buf,tbuf;
    iEnv->ReadResource(tbuf,R_OGG_ERROR_18);
    iEnv->ReadResource(buf,R_OGG_ERROR_16);
    buf.AppendNum(aError);
    iEnv->InfoWinL(tbuf,buf);
  }

  if (iObserver) {
    if (iEof /*&& iState==EPlaying*/) {
      Stop();
      ClearComments();
      iTime= 0;
      iObserver->NotifyPlayComplete();
      //Stop();
      return;
    }
    if (!iEof /*&& iState==EPlaying && aError!=KErrNone*/) iObserver->NotifyPlayInterrupted();
  }
}

void COggPlayback::MaoscBufferCopied(TInt aError, const TDesC8& aBuffer)
{
  // Error codes:
  // KErrCancel      -3  (not sure when this happens, but ignore for now)
  // KErrUnderflow  -10  (ignore this, do as if nothing has happend, and live happily ever after)
  // KErrInUse      -14  (the sound device was stolen from us)
  // KErrAbort      -39  (stream was stopped before this buffer was copied)

  if (aError == KErrCancel) aError= KErrNone;

  if (aError == KErrAbort ||
      aError == KErrInUse ||
      aError == KErrCancel) return;

  if (iState != EPlaying) return;

  TInt b;
  for (b=0; b<KBuffers; b++) if (&aBuffer == iBuffer[b]) break;

  if (aError == KErrUnderflow) aError= KErrNone;

  if (aError == KErrNone) SendBuffer(*iBuffer[b]); 
  else {
    // An unknown error condition. This should never ever happen.
	RDebug::Print(_L("Oggplay: MaoscBufferCopied unknown error (Error18 Error16)"));
	TBuf<256> buf,tbuf;
    iEnv->ReadResource(tbuf, R_OGG_ERROR_18);
    iEnv->ReadResource(buf, R_OGG_ERROR_16);
    buf.AppendNum(aError);
    iEnv->InfoWinL(tbuf,buf);
  }
}

void COggPlayback::MaoscOpenComplete(TInt aError) 
{ 
  if (aError != KErrNone) {

    TBuf<32> buf;
    iEnv->ReadResource(buf,R_OGG_ERROR_19);
    buf.AppendNum(aError);
    _LIT(tit,"MaoscOpenComplete");
    iEnv->InfoWinL(tit,buf);

  } else {

    if (iState == EClosed) {
      iState = EFirstOpen;
    } else {
      iState = EOpen;
    }

    iStream->SetPriority(EMdaPriorityMax, EMdaPriorityPreferenceTimeAndQuality);
  }
}
