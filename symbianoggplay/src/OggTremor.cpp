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
#include "TremorDecoder.h"
#ifdef MP3_SUPPORT
#include "MadDecoder.h"
#endif

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


#if defined(UIQ)
const TInt KAudioPriority = EMdaPriorityMax;
#else
const TInt KAudioPriority = 70; // S60 audio players typically uses 60-75.
#endif



////////////////////////////////////////////////////////////////
//
// CAbsPlayback
//
////////////////////////////////////////////////////////////////

CAbsPlayback::CAbsPlayback(MPlaybackObserver* anObserver) :
  iState(CAbsPlayback::EClosed),
  iObserver(anObserver),
  iTitle(),
  iAlbum(),
  iArtist(),
  iGenre(),
  iTrackNumber(),
  iTime(0),
  iRate(44100),
  iChannels(2),
  iFileSize(0),
  iBitRate(0)
{
}

////////////////////////////////////////////////////////////////
//
// COggPlayback
//
////////////////////////////////////////////////////////////////

COggPlayback::COggPlayback(COggMsgEnv* anEnv, MPlaybackObserver* anObserver ) : 
  CAbsPlayback(anObserver),
  iEnv(anEnv),
  iSettings(),
  iSentIdx(0),
  iStoppedFromEof(EFalse),
  iFile(0),
  iFileOpen(0),
  iEof(0),
  iBufCount(0)
{
    iSettings.Query();
    
    // These basic rates should be supported by all Symbian OS
    // Higher quality might not be supported.
    iSettings.iChannels  = TMdaAudioDataSettings::EChannelsMono;
    iSettings.iSampleRate= TMdaAudioDataSettings::ESampleRate8000Hz;
    iSettings.iVolume = 0;
};

void COggPlayback::ConstructL() {

  COggAudioCapabilityPoll pollingAudio;
  iAudioCaps = pollingAudio.PollL(); // Discover Audio Capabilities

  iStream  = CMdaAudioOutputStream::NewL(*this);
  
  TRAPD( err, iStream->Open(&iSettings) );
  if (err!= KErrNone) {
    TBuf<256> buf1,buf2;
    CEikonEnv::Static()->ReadResource(buf1, R_OGG_ERROR_7);
    CEikonEnv::Static()->ReadResource(buf2, R_OGG_ERROR_20);
    buf1.AppendNum(err);
    iEnv->OggErrorMsgL(buf2,buf1);
    //OGGLOG.Write(buf2);
    //OGGLOG.Write(buf1);
  }
  iMaxVolume = 1; // This will be updated when stream is opened.
  //OGGLOG.WriteFormat(_L("Max volume is %d"),iMaxVolume);
  TDes8* buffer;

  for (TInt i=0; i<KBuffers; i++) {
    buffer = new(ELeave) TBuf8<KBufferSize>;
    buffer->SetMax();
    CleanupStack::PushL(buffer);
    User::LeaveIfError(iBuffer.Append(buffer));
    CleanupStack::Pop(buffer);
  }

  iStartAudioStreamingTimer = new (ELeave) COggTimer(
      TCallBack( StartAudioStreamingCallBack,this )   );
  
  iStopAudioStreamingTimer = new (ELeave) COggTimer(
      TCallBack( StopAudioStreamingCallBack,this )   );
  iOggSampleRateConverter = new (ELeave) COggSampleRateConverter;


}

COggPlayback::~COggPlayback() {
  if(iDecoder) { 
    delete iDecoder; 
    iDecoder=NULL; 
  }
  delete  iStartAudioStreamingTimer;
  delete  iStopAudioStreamingTimer;
  delete iStream;
  iBuffer.ResetAndDestroy();
  delete iOggSampleRateConverter;
}

void COggPlayback::SetDecoderL(const TDesC& aFileName)
{
  if(iDecoder) { 
    delete iDecoder; 
    iDecoder=NULL; 
  }

  TParsePtrC p( aFileName);

  if(p.Ext().Compare( _L(".ogg"))==0 || p.Ext().Compare( _L(".OGG"))==0) {
      iDecoder=new(ELeave)CTremorDecoder;
  } 
#if defined(MP3_SUPPORT)
  else if(p.Ext().Compare( _L(".mp3"))==0 || p.Ext().Compare( _L(".MP3"))==0) {
    iDecoder=new(ELeave)CMadDecoder;
  }
#endif
  else {
    _LIT(KPanic,"Panic:");
    _LIT(KNotSupported,"File type not supported");
    iEnv->OggErrorMsgL(KPanic,KNotSupported);
  }
  
}

TInt COggPlayback::Open(const TDesC& aFileName)
{

  if (iFileOpen) {
    iDecoder->Close(iFile);
    fclose(iFile);
    iFileOpen= 0;
  }

  iTime= 0;

  if (aFileName.Length() == 0) {
    //OGGLOG.Write(_L("Oggplay: Filenamelength is 0 (Error20 Error8"));
    iEnv->OggErrorMsgL(R_OGG_ERROR_20,R_OGG_ERROR_8);
    return -100;
  }

  // add a zero terminator
  TBuf<512> myname(aFileName);
  myname.Append((wchar_t)0);

  if ((iFile=wfopen((wchar_t*)myname.Ptr(),L"rb"))==NULL) {
    iFileOpen= 0;
    //OGGLOG.Write(_L("Oggplay: File open returns 0 (Error20 Error14)"));
    TRACE(COggLog::VA(_L("COggPlayback::Open(%S). Failed"), &aFileName ));
    iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_14);
    return KErrOggFileNotFound;
  };
  iFileOpen= 1;
  SetDecoderL(aFileName);

  if(iDecoder->Open(iFile) < 0) {
    iDecoder->Close(iFile);
    fclose(iFile);
    iFileOpen= 0;
    //OGGLOG.Write(_L("Oggplay: ov_open not successful (Error20 Error9)"));

    iEnv->OggErrorMsgL(R_OGG_ERROR_20,R_OGG_ERROR_9);
    return -102;
  }

  iDecoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);

  iFileName= aFileName;
  iRate= iDecoder->Rate();
  iChannels=iDecoder->Channels();
  iTime=iDecoder->TimeTotal();
  iBitRate=iDecoder->Bitrate();


  //FIXMAD -- this is currently only used in file information dialog
  //iFileSize= ov_raw_total(&iVf,-1);

  TInt err= SetAudioCaps(iDecoder->Channels(),iDecoder->Rate());
  if (err==KErrNone) {
    unsigned int hi(0);
    iBitRate.Set(hi,iDecoder->Bitrate());
    iState= EOpen;
    return KErrNone;
  }
    
  iDecoder->Close(iFile);
  fclose(iFile);
  iFileOpen=0;

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
  TBool ConvertChannel = EFalse;
  TBool ConvertRate = EFalse;

  TInt usedChannels = theChannels;
  
  if (!(iAudioCaps & TMdaAudioDataSettings::EChannelsStereo) && (theChannels==2) )
  {
      // Only Mono is supported by this phone, we need to mix the 2 channels together
      ConvertChannel = ETrue;
      usedChannels = 1; // Use 1 channel
  }

  if (usedChannels==1) ac= TMdaAudioDataSettings::EChannelsMono;
  else if (usedChannels==2) ac= TMdaAudioDataSettings::EChannelsStereo;
  else {
    iEnv->OggErrorMsgL(R_OGG_ERROR_12,R_OGG_ERROR_10);
    //OGGLOG.Write(_L("Illegal number of channels"));
    return -100;
  }

 
  TInt usedRate = theRate;

  if (theRate==8000) rt= TMdaAudioDataSettings::ESampleRate8000Hz;
  else if (theRate==11025) rt= TMdaAudioDataSettings::ESampleRate11025Hz;
  else if (theRate==16000) rt= TMdaAudioDataSettings::ESampleRate16000Hz;
  else if (theRate==22050) rt= TMdaAudioDataSettings::ESampleRate22050Hz;
  else if (theRate==32000) rt= TMdaAudioDataSettings::ESampleRate32000Hz;
  else if (theRate==44100) rt= TMdaAudioDataSettings::ESampleRate44100Hz;
  else if (theRate==48000) rt= TMdaAudioDataSettings::ESampleRate48000Hz;
  else {
      // Rate not supported by the phone
      ConvertRate = ETrue;
      
      usedRate = 16000; // That rate should be 
                        // supported by all phones
      rt = TMdaAudioDataSettings::ESampleRate16000Hz;
  }

  if ( !(rt & iAudioCaps) )
   {
    // Rate not supported by this phone.
      ConvertRate = ETrue;
      usedRate = 16000; //That rate should be 
                        //supported by all phones
      rt = TMdaAudioDataSettings::ESampleRate16000Hz;
   }
  
  if (ConvertChannel || ConvertRate)
  {
      SamplingRateSupportedMessage(ConvertRate, theRate, ConvertChannel, theChannels);
   }

  iOggSampleRateConverter->Init(this,KBufferSize,(TInt) (0.75*KBufferSize), theRate,usedRate, theChannels,usedChannels);

  TRAPD( error, iStream->SetAudioPropertiesL(rt, ac) );
  if (error)
  {
      TBuf<256> buf,tbuf;
      CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_16);
      CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_20);
      buf.AppendNum(error);
      iEnv->OggErrorMsgL(tbuf,buf);
  }
#ifdef MDCT_FREQ_ANALYSER
  iTimeBetweenTwoSamples = 1.0E6/(theRate*theChannels);
  /* Make a frequency measurement only every .1 seconds, that should be enough */
  iTimeWithoutFreqCalculation = 0;
  iTimeWithoutFreqCalculationLim = (TInt)(0.1E6 / iTimeBetweenTwoSamples);
#endif
  return error;

}


void COggPlayback::SamplingRateSupportedMessage(TBool aConvertRate, TInt aRate, 
                                                TBool aConvertChannel, TInt /*aNbOfChannels*/)
{
    // Print an error/info msg displaying all supported rates by this HW.

    const TInt ratesMask[] = {TMdaAudioDataSettings::ESampleRate8000Hz,
    TMdaAudioDataSettings::ESampleRate11025Hz,
    TMdaAudioDataSettings::ESampleRate16000Hz,
    TMdaAudioDataSettings::ESampleRate22050Hz,
    TMdaAudioDataSettings::ESampleRate32000Hz,
    TMdaAudioDataSettings::ESampleRate44100Hz, 
    TMdaAudioDataSettings::ESampleRate48000Hz };
    const TInt rates[] = {8000,11025,16000,22050,32000,44100,48000};

    HBufC * HbufMsg = HBufC::NewL(1000);
    CleanupStack::PushL(HbufMsg);
    HBufC * HtmpBuf  = HBufC::NewL(500);
    CleanupStack::PushL(HtmpBuf);

    TPtr BufMsg = HbufMsg->Des();
    TPtr TmpBuf = HtmpBuf->Des();

    if (aConvertRate)
    {
        TBuf<128> buf;
        CEikonEnv::Static()->ReadResource(TmpBuf,R_OGG_ERROR_24);
    TBool first = ETrue;
    for (TInt i=0; i<7; i++)
    {
        if (iAudioCaps & ratesMask[i])
        {
            if (!first)
            { 
                // Append a comma
                    buf.Append(_L(", "));
            }
            // Append the audio rate
                buf.AppendNum(rates[i]);
            first = EFalse;
        }
    }
        BufMsg.Format(TmpBuf, aRate, &buf);
    }
    if (aConvertChannel)
    {
        CEikonEnv::Static()->ReadResource(TmpBuf,R_OGG_ERROR_25);
        BufMsg.Append(TmpBuf);
    }
    
    CEikonEnv::Static()->ReadResource(TmpBuf,R_OGG_ERROR_26);
    BufMsg.Append(TmpBuf);

    iEnv->OggWarningMsgL(BufMsg);
    CleanupStack::PopAndDestroy(2);
}

TInt COggPlayback::Volume()
{
  if (!iStream) return KMaxVolume;
  TInt vol=KMaxVolume;
  TRAPD( err, vol=iStream->Volume() );
  return vol;
}

void COggPlayback::SetVolume(TInt aVol)
{
  if (!iStream) return;
  if (aVol>KMaxVolume) aVol= KMaxVolume;
  else if(aVol<0) aVol= 0;

  TInt volume = (TInt) (((TReal) aVol)/KMaxVolume * iMaxVolume);
  if (volume > iMaxVolume)
      volume = iMaxVolume;
  TRAPD( err, iStream->SetVolume(volume); )
}

void COggPlayback::SetPosition(TInt64 aPos)
{
  if (!iStream) return;
  if(iDecoder) iDecoder->Setposition(aPos);
}

TInt64 COggPlayback::Position()
{
  if (!iStream) return TInt64(0);
  if(iDecoder) return iDecoder->Position();
  return 0;
}

TInt64 COggPlayback::Time()
{
  if (iDecoder && iTime==0) {
    iTime=iDecoder->TimeTotal();
  }
  return iTime;
}


#ifdef MDCT_FREQ_ANALYSER
const TInt32 * COggPlayback::GetFrequencyBins(TTime /* aTime */)
{

    // We're not using aTime anymore, Position gives better results
  TTimeIntervalMicroSeconds currentPos = iStream->Position();

  TInt idx;
  idx = iLastFreqArrayIdx;
   
  for (TInt i=0; i<KFreqArrayLength; i++)
    {
      if (iFreqArray[idx].Time < currentPos)
          break;
      
        idx--;
        if (idx < 0)
            idx = KFreqArrayLength-1;
    }
 
    return iFreqArray[idx].FreqCoefs;
}
#else
const void* COggPlayback::GetDataChunk()
{
  TInt idx= iSentIdx;
  if( iSent[idx] ) 
    return iSent[idx]->Ptr();
  else
    return NULL;
}
#endif

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
      CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_14);
      iEnv->OggErrorMsgL(buf,aFileName);
    }
    return KErrOggFileNotFound;
  };

  SetDecoderL(aFileName);
  if(iDecoder->OpenInfo(f) < 0) {
    iDecoder->Close(f);
    fclose(f);
    if (!silent) {
       iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_9);
    }
    return -102;
  }
  iDecoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);

  iFileName= aFileName;
  iRate= iDecoder->Rate();
  iChannels=iDecoder->Channels();
  iBitRate=iDecoder->Bitrate();

  iDecoder->Close(f); 
  fclose(f);
  iFileOpen=0;

  return KErrNone;
}

void COggPlayback::Play() 
{
    switch(iState) {
    case EPaused:
        break;
    case EOpen:  
        iEof=0;
        iStoppedFromEof = EFalse;
        iBufCount= 0;
        break;
    default:
    iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_15);
    RDebug::Print(_L("Oggplay: Tremor - State not Open"));
    return;
  }

  iState = EPlaying;
  // There is something wrong how Nokia audio streaming handles the
                   // first buffers. They are somehow swallowed.
                   // To avoid that, send few (4) almost empty buffers
  iFirstBuffers = 4; 
#ifdef DELAY_AUDIO_STREAMING_START
  //Also to avoid the first buffer problem, wait a short time before streaming, 
  // so that Application drawing have been done. Processor should then
  // be fully available for doing audio thingies.
  iStartAudioStreamingTimer->Wait(0.1E6);
#else
  for (TInt i=0; i<KBuffers; i++) SendBuffer(*iBuffer[i]);
#endif
}

void COggPlayback::Pause()
{
  if (iState != EPlaying) return;
  iState= EPaused;
  TRAPD( err, iStream->Stop() );
}

void COggPlayback::Stop()
{
  if (iState == EClosed ) // FIXIT Was 'EPlaying' and panicked at exit in paused mode
  { 
    return;
  }
  TRAPD( err, iStream->Stop() );
  iState= EClosed;
  if (iFileOpen) {
    iDecoder->Close(iFile);
    fclose(iFile);
    iFileOpen= 0;
  }
  ClearComments();
  iTime= 0;
  iEof= 0;
  iStoppedFromEof = EFalse;

  if (iObserver) iObserver->NotifyUpdate();
}


TInt COggPlayback::GetNewSamples(TDes8 &aBuffer)
{
    if (iEof)
        return(KErrNotReady);
    TInt len = aBuffer.Length();
#ifdef MDCT_FREQ_ANALYSER
    if (iTimeWithoutFreqCalculation > iTimeWithoutFreqCalculationLim )

    {
        iTimeWithoutFreqCalculation = 0;
        iLastFreqArrayIdx++;
        if (iLastFreqArrayIdx >= KFreqArrayLength)
            iLastFreqArrayIdx = 0;
        iDecoder->GetFrequencyBins(iFreqArray[iLastFreqArrayIdx].FreqCoefs,KNumberOfFreqBins);
        iFreqArray[iLastFreqArrayIdx].Time =  TInt64(iLatestPlayTime) ;
    }
    else
    {
        iDecoder->GetFrequencyBins(NULL,0);
    }
#endif
    
    TInt ret=iDecoder->Read(aBuffer,len); 
    if (ret >0)
    {
        aBuffer.SetLength(len + ret);
#ifdef MDCT_FREQ_ANALYSER
        iLatestPlayTime += ret*iTimeBetweenTwoSamples;
        iTimeWithoutFreqCalculation += ret;
        //TRACEF(COggLog::VA(_L("U:%d %f "), ret, iLatestPlayTime ));
#endif


    }
    if (ret == 0)
    {
        iEof= 1;
    }
    return (ret);
}


void COggPlayback::SendBuffer(TDes8& buf)
{
  if (iEof) return;
  if (iState==EPaused) return;

  long ret=0;
  buf.SetLength(KBufferSize);

  TInt cnt = 0;
  if (iFirstBuffers)  {
      buf.FillZ(4000);
      cnt = 4000;
      ret = 4000;
#ifdef MDCT_FREQ_ANALYSER
      iLatestPlayTime += ret*iTimeBetweenTwoSamples;
#endif
      iFirstBuffers--;
  
  } else {
    
     ret = iOggSampleRateConverter->FillBuffer( buf );
  }

  if (ret < 0) {
    // Error in the stream. Bad luck, we will continue anyway.
  } else {
    iBufCount++;
    TRAPD( err, iStream->WriteL(buf) );
    if (err!=KErrNone) {
      // This should never ever happen.
      TBuf<256> cbuf,tbuf;
      CEikonEnv::Static()->ReadResource(cbuf,R_OGG_ERROR_16);
      CEikonEnv::Static()->ReadResource(tbuf,R_OGG_ERROR_17);
      cbuf.AppendNum(err);
      iEnv->OggErrorMsgL(tbuf,cbuf);
      User::Leave(err);
    }
    iSentIdx++;
    if (iSentIdx>=KBuffers) iSentIdx=0;
    iSent[iSentIdx]= &buf;
  }
}

void COggPlayback::MaoscPlayComplete(TInt aError)
{
  // Error codes:
  // KErrCancel      -3
  // KErrUnderflow  -10
  // KErrDied       -13  (interrupted by higher priority)
  // KErrInUse      -14

  if (iState==EPaused || iState==EClosed) return;

  if (aError == KErrUnderflow) 
      return;
  if (aError == KErrInUse) aError= KErrNone;

  if (aError == KErrCancel)
      return;

  if (aError == KErrDied) {
    iObserver->NotifyPlayInterrupted();
    return;
  }

  if (aError != KErrNone) {
    TBuf<256> buf,tbuf;
    CEikonEnv::Static()->ReadResource(tbuf,R_OGG_ERROR_18);
    CEikonEnv::Static()->ReadResource(buf,R_OGG_ERROR_16);
    buf.AppendNum(aError);
    iEnv->OggErrorMsgL(tbuf,buf);
    if (iObserver )
    {
        if (!iEof ) 
            iObserver->NotifyPlayInterrupted();
    }
  }

  
}

void COggPlayback::MaoscBufferCopied(TInt aError, const TDesC8& aBuffer)
{
  // Error codes:
  // KErrCancel      -3  (not sure when this happens, but ignore for now)
  // KErrUnderflow  -10  (ignore this, do as if nothing has happend, and live happily ever after)
  // KErrDied       -13  (interrupted by higher priority)
  // KErrInUse      -14  (the sound device was stolen from us)
  // KErrAbort      -39  (stream was stopped before this buffer was copied)

  if (aError == KErrCancel) aError= KErrNone;

  if (aError == KErrAbort ||
      aError == KErrInUse ||
      aError == KErrDied  ||
      aError == KErrCancel) return;

  if (iState != EPlaying) return;

  TInt b;
  for (b=0; b<KBuffers; b++) if (&aBuffer == iBuffer[b]) break;
  if ( (iEof) && (iSent[iSentIdx] == &aBuffer) )
  {
      // All the buffers have been sent, stop the playback
      // We cannot rely on a MaoscPlayComplete from the CMdaAudioOutputStream
      // since not all phone supports that.

      iStoppedFromEof = ETrue;
      iStopAudioStreamingTimer->Wait(0.1E6);
  }
  if (aError == KErrUnderflow) aError= KErrNone;

  if (aError == KErrNone) SendBuffer(*iBuffer[b]); 
  else {
    // An unknown error condition. This should never ever happen.
	RDebug::Print(_L("Oggplay: MaoscBufferCopied unknown error (Error18 Error16)"));
	TBuf<256> buf,tbuf;
    CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_18);
    CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_16);
    buf.AppendNum(aError);
    iEnv->OggErrorMsgL(tbuf,buf);
  }
}

void COggPlayback::MaoscOpenComplete(TInt aError) 
{ 
  iMaxVolume=iStream->MaxVolume();
  if (aError != KErrNone) {

    TBuf<32> buf;
    CEikonEnv::Static()->ReadResource(buf,R_OGG_ERROR_19);
    buf.AppendNum(aError);
    _LIT(tit,"MaoscOpenComplete");
    iEnv->OggErrorMsgL(tit,buf);

  } else {

    if (iState == EClosed) {
      iState = EFirstOpen;
    } else {
      iState = EOpen;
    }

    iStream->SetPriority(KAudioPriority, EMdaPriorityPreferenceTimeAndQuality);
  }
}


TInt COggPlayback::StartAudioStreamingCallBack(TAny* aPtr)
{
  COggPlayback* self= (COggPlayback*) aPtr;
  
  for (TInt i=0; i<KBuffers; i++) 
      self->SendBuffer(*(self->iBuffer[i]));
  return 0;
}


TInt COggPlayback::StopAudioStreamingCallBack(TAny* aPtr)
{
  COggPlayback* self= (COggPlayback*) aPtr;
  
  self->iStream->Stop();

  if (self->iObserver) {
    if ( self->iEof ) {
      self->Stop();
      self->ClearComments();
      self->iTime= 0;
      self->iObserver->NotifyPlayComplete();
      return 0;
    }
    
  }
  return 0;
}

void COggPlayback::SetVolumeGain(TGainType aGain)
{
    iOggSampleRateConverter->SetVolumeGain(aGain);
}

TInt COggAudioCapabilityPoll::PollL()
    {
    iCaps=0;
    TInt oneSupportedRate = 0;
    for (TInt i=0; i<7; i++)
        {
        switch(i)
            {
            case 0:
                iRate=TMdaAudioDataSettings::ESampleRate8000Hz;
                break;
            case 1:
                iRate= TMdaAudioDataSettings::ESampleRate11025Hz;
                break;
            case 2:
                iRate= TMdaAudioDataSettings::ESampleRate16000Hz;
                break;
            case 3:
                iRate= TMdaAudioDataSettings::ESampleRate22050Hz;
                break;
            case 4:
                iRate= TMdaAudioDataSettings::ESampleRate32000Hz;
                break;
            case 5:
                iRate= TMdaAudioDataSettings::ESampleRate44100Hz;
                break;
            case 6:
                iRate= TMdaAudioDataSettings::ESampleRate48000Hz;
                break;
            }
        iSettings.Query();
        
        iSettings.iChannels  = TMdaAudioDataSettings::EChannelsMono;
        iSettings.iSampleRate= iRate;
        iSettings.iVolume = 0;
        
        iStream  = CMdaAudioOutputStream::NewL(*this);
        iStream->Open(&iSettings);
        CActiveScheduler::Start();

        if (iCaps & iRate){
            // We need to make sure, Nokia 6600 for example won't tell in the
            // Open() that the requested rate is not supported.
            TRAPD(err,iStream->SetAudioPropertiesL(iRate,iSettings.iChannels););
            TRACEF(COggLog::VA(_L("SampleRate Supported:%d"), err ));
            if (err == KErrNone)
            {
                // Rate is *really* supported. Remember it.
                oneSupportedRate = iRate;
            }
            else
            {
                // Rate is not supported.
                iCaps &= ~iRate;
            }
        }

        delete iStream; // This is rude...
        iStream = NULL;
        
        }
    
    // Poll for stereo support
    iSettings.iChannels  = TMdaAudioDataSettings::EChannelsStereo;
    iSettings.iSampleRate= oneSupportedRate;
    iSettings.iVolume = 0;
    
    iStream  = CMdaAudioOutputStream::NewL(*this);
    iStream->Open(&iSettings);
    iRate = TMdaAudioDataSettings::EChannelsStereo;
    CActiveScheduler::Start();
    
    if (iCaps & iRate){
       // We need to make sure, Nokia 6600 for example won't tell in the
       // Open() that the requested rate is not supported.
       TRAPD(err,iStream->SetAudioPropertiesL(iSettings.iSampleRate,iSettings.iChannels););
       TRACEF(COggLog::VA(_L("Stereo Supported:%d"), err ));
       if (err != KErrNone)
       {
          // Rate is not supported.
          iCaps &= ~iRate;
       }
    }
    delete iStream; // This is rude...
    iStream = NULL;
    return iCaps;
    }


void COggAudioCapabilityPoll::MaoscOpenComplete(TInt aError) 
{
    TRACEF(COggLog::VA(_L("SampleRa:%d"), aError ));

    if (aError==KErrNone) 
        {
        // Mode supported.
        iCaps |= iRate;
        }
    CActiveScheduler::Stop();
    
}

void COggAudioCapabilityPoll::MaoscPlayComplete(TInt /*aError*/)
    {
    }

void COggAudioCapabilityPoll::MaoscBufferCopied(TInt /*aError*/, const TDesC8& /*aBuffer*/)
    {
    }
