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

////////////////////////////////////////////////////////////////
//
// CAbsPlayback
//
////////////////////////////////////////////////////////////////

CAbsPlayback::CAbsPlayback( MPlaybackObserver* anObserver )
: iState(CAbsPlayback::EClosed)
, iObserver(anObserver)
, iTime(0)
, iRate(44100)
, iChannels(2)
, iFileSize(0)
, iBitRate(0)
, iAlbum()
, iArtist()
, iFileName()
, iGenre()
, iTitle()
, iTrackNumber()
{
}

////////////////////////////////////////////////////////////////
//
// COggPlayback
//
////////////////////////////////////////////////////////////////

// Construction/destruction methods
////////////////////////////////////////////////////////////////

// COggPlayback
////////////////////////////////////////////////////////////////

COggPlayback::COggPlayback( COggMsgEnv* anEnv, MPlaybackObserver* anObserver )
: CAbsPlayback(anObserver)
, iEnv(anEnv)
, iOggSampleRateConverter( NULL )
, iStream( NULL )
, iDelete( NULL )
, iMaxVolume( 0 )
, iSentIdx( 0 )
, iPlayWhenOpened( EFalse )
, iFile( NULL )
, iFileOpen( EFalse )
, iEof( EFalse )
, iDecoder( NULL )
{
};

// ~COggPlayback
////////////////////////////////////////////////////////////////

COggPlayback::~COggPlayback( void )
{
   if ( iFileOpen )
   {
      iDecoder->Close( iFile );
      fclose( iFile );
      iFileOpen = EFalse;
   }
   if ( iDecoder ) delete iDecoder;
   if ( iStream  ) delete iStream;
   if ( iDelete  ) delete iDelete;
   iBuffer.ResetAndDestroy();
   if ( iOggSampleRateConverter ) delete iOggSampleRateConverter;
}

// ConstructL
////////////////////////////////////////////////////////////////

void COggPlayback::ConstructL( void )
{
   RDebug::Print( _L("COggPlayback::ConstructL") );

   CreateStreamApi();

   iMaxVolume = 1; // This will be updated when stream is opened.

   TDes8 *buffer;
   for ( TInt i = 0; i < KMotoBuffers; i++ ) 
   {
      buffer = new (ELeave) TBuf8<KMotoBufferSize>;
      buffer->SetMax();
      CleanupStack::PushL( buffer );
      User::LeaveIfError( iBuffer.Append( buffer ) );
      CleanupStack::Pop( buffer );
   }

   iOggSampleRateConverter = new (ELeave) COggSampleRateConverter;
}

// Public interface methods
////////////////////////////////////////////////////////////////

// Info
////////////////////////////////////////////////////////////////

TInt COggPlayback::Info(const TDesC& aFileName, TBool silent)
{
   if (aFileName.Length()==0) return -100;
   
   // Close any existing opened file
   if ( iFileOpen )
   {
      iDecoder->Close( iFile ); 
      fclose( iFile );
      iFileOpen = EFalse;
   }

   FILE* f;
   
   // add a zero terminator
   TBuf<512> myname(aFileName);
   myname.Append((wchar_t)0);
   
   if ((f=wfopen((wchar_t*)myname.Ptr(),L"rb"))==NULL) 
   {
      if (!silent) 
      {
         TBuf<128> buf;
         CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_14);
         iEnv->OggErrorMsgL(buf,aFileName);
      }
      return KErrOggFileNotFound;
   }

   SetDecoderL( aFileName );
   if ( iDecoder->OpenInfo(f,myname) < 0 )
   {
      iDecoder->Close(f);
      fclose(f);
      iFileOpen = EFalse;

      if (!silent)
      {
         iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_9);
      }
      return -102;
   }
   
   iDecoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);
   iFileName = aFileName;
   iRate     = iDecoder->Rate();
   iChannels = iDecoder->Channels();
   iBitRate  = iDecoder->Bitrate();

   iDecoder->Close(f); 
   fclose(f);
   iFileOpen = EFalse;

   return KErrNone;
}

// Open
////////////////////////////////////////////////////////////////

TInt COggPlayback::Open( const TDesC& aFileName )
{
   if ( !iStream ) CreateStreamApi();

   if ( iState == EPlaying ) Stop();
   
   if ( iFileOpen ) 
   {
      iDecoder->Close(iFile);
      fclose(iFile);
      iFileOpen = EFalse;
   }
   
   iTime = 0;
   
   if ( aFileName.Length() == 0 )
   {
      //OGGLOG.Write(_L("Oggplay: Filenamelength is 0 (Error20 Error8"));
      iEnv->OggErrorMsgL(R_OGG_ERROR_20,R_OGG_ERROR_8);
      return -100;
   }
   
   // add a zero terminator
   TBuf<512> myname(aFileName);
   myname.Append((wchar_t)0);
   
   if ( ( iFile = wfopen( (wchar_t*) myname.Ptr(), L"rb" ) ) == NULL ) 
   {
      iFileOpen = EFalse;

      //OGGLOG.Write(_L("Oggplay: File open returns 0 (Error20 Error14)"));
      TRACE(COggLog::VA(_L("COggPlayback::Open(%S). Failed"), &aFileName ));
      iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_14);

      return KErrOggFileNotFound;
   }
   
   iFileOpen = ETrue;
   
   SetDecoderL(aFileName);

   if ( iDecoder->Open(iFile,myname) < 0 )
   {
      iDecoder->Close(iFile);
      fclose(iFile);
      iFileOpen = EFalse;
      
      //OGGLOG.Write(_L("Oggplay: ov_open not successful (Error20 Error9)"));
      iEnv->OggErrorMsgL(R_OGG_ERROR_20,R_OGG_ERROR_9);

      return -102;
   }
   
   iDecoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);

   iFileName = aFileName;
   iRate     = iDecoder->Rate();
   iChannels = iDecoder->Channels();
   iTime     = iDecoder->TimeTotal();
   iBitRate  = iDecoder->Bitrate();
   iFileSize = iDecoder->FileSize();
      
   TInt err = SetAudioCaps( iDecoder->Channels(), iDecoder->Rate() );
   if ( err == KErrNone ) 
   {
      unsigned int hi(0);
      iBitRate.Set(hi,iDecoder->Bitrate());
      iState = EOpen;
   }
   else
   {
      iDecoder->Close(iFile);
      fclose( iFile );
      iFileOpen = EFalse;
   }

   return err;
}

// Pause
////////////////////////////////////////////////////////////////

void COggPlayback::Pause()
{
   if (iState == EPlaying)
   {
      iState = EPaused;
   }
}

// Play
////////////////////////////////////////////////////////////////

void COggPlayback::Play() 
{
   TInt err;

   switch (iState)
   {
   case EOpen:  
      iEof = EFalse;
      // Intentionally fall-thru
      
   case EPaused:
      {
         iState = EPlaying;
         for ( TInt i = 0; i < KMotoBuffers; i++)
         {
            TRAP( err, SendBufferL( i ) );

            if ( iEof ) break;
         }
         TRAP( err, iStream->DecodeL() );
      }
      break;
      
   case EClosed:
      if ( iStream )
      {
         iPlayWhenOpened = ETrue;
         break;
      }
      // Else fall-thru

   default:
      iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_15);
      RDebug::Print(_L("Oggplay: Tremor - State not Open"));
      break;
   }
}

// Stop
////////////////////////////////////////////////////////////////

void COggPlayback::Stop()
{
   if (iState == EPlaying)
   { 
      iStream->Stop();
      iState = EClosed;
      if (iFileOpen)
      {
         iDecoder->Close(iFile);
         fclose(iFile);
         iFileOpen = EFalse;
      }

      ClearComments();

      iTime = 0;
      iEof  = EFalse;
      
      if (iObserver)
      {
         iObserver->NotifyUpdate();
      }
   }
}

// SetPosition
////////////////////////////////////////////////////////////////

void COggPlayback::SetPosition(TInt64 aPos)
{
   if ( iStream )
   {
      if ( iDecoder ) iDecoder->Setposition(aPos);
   }
}

// Position
////////////////////////////////////////////////////////////////

TInt64 COggPlayback::Position()
{
   if ( !iStream ) return TInt64(0);
   if ( iDecoder ) return iDecoder->Position();
   return 0;
}

// Time
////////////////////////////////////////////////////////////////

TInt64 COggPlayback::Time()
{
   if ( iDecoder && iTime == 0 ) 
   {
      iTime=iDecoder->TimeTotal();
   }
   return ( iTime );
}

// Volume
////////////////////////////////////////////////////////////////

TInt COggPlayback::Volume()
{
   TInt vol = KMaxVolume;

   if (iStream)
   {
      TRAPD( err, vol = iStream->GetVolume() );
   }

   return ( vol );
}

// SetVolume
////////////////////////////////////////////////////////////////

void COggPlayback::SetVolume(TInt aVol)
{
   if (iStream)
   {
      if ( aVol > KMaxVolume )
      { 
         aVol = KMaxVolume;
      }
      else if( aVol < 0 )
      {
         aVol = 0;
      }
      
      TInt volume = (TInt) (((TReal) aVol)/KMaxVolume * iMaxVolume);
      if (volume > iMaxVolume)
      {
         volume = iMaxVolume;
      }

      TRAPD( err, iStream->SetVolume(volume) );
   }
}

// SetVolumeGain
////////////////////////////////////////////////////////////////

void COggPlayback::SetVolumeGain( TGainType aGain )
{
   iOggSampleRateConverter->SetVolumeGain(aGain);
}

// GetFrequencyBins
////////////////////////////////////////////////////////////////

#ifdef MDCT_FREQ_ANALYSER
const TInt32 * COggPlayback::GetFrequencyBins(TTime /* aTime */)
{
   // We're not using aTime anymore, Position gives better results
   TTimeIntervalMicroSeconds currentPos;
   iStream->Position( currentPos );
   
   TInt idx;
   idx = iLastFreqArrayIdx;
   
   for (TInt i=0; i<KFreqArrayLength; i++)
   {
      if (iFreqArray[idx].Time < currentPos)
      {
         break;
      }
      
      idx--;
      if (idx < 0)
      {
         idx = KFreqArrayLength-1;
      }
   }
   
   return iFreqArray[idx].FreqCoefs;
}
#endif

// GetDataChunk
////////////////////////////////////////////////////////////////

#ifndef MDCT_FREQ_ANALYSER
const void* COggPlayback::GetDataChunk()
{
   void *p = NULL;

   p = (void*) iBuffer[iSentIdx]->Ptr();

   return ( p );
}
#endif

// OnEvent
////////////////////////////////////////////////////////////////

void COggPlayback::OnEvent( TMAudioFBCallbackState aState, TInt aError )
{
   RDebug::Print( _L("OnEvent/%d/%d"), aState, aError );

   switch ( aState )
   {
   case EMAudioFBCallbackStateReady:
      iState = EOpen;
      iMaxVolume = iStream->GetMaxVolume();
      if ( iPlayWhenOpened ) Play();
      break;

   case EMAudioFBCallbackStateDecodeCompleteBufferListEmpty:
      // Occurs mostly when all data has been played

      // Check if all data has been played
      if ( iEof )
      {
         TInt j;
         for ( j = 0; j < KMotoBuffers; j++ )
         {
            if ( iBufferFlag[ j ] == EBufferInUse ) break;
         }

         if ( j >= KMotoBuffers )
         {
            // All data has been played, so stop
            iStream->Stop();
            iState = EClosed;
            if (iFileOpen)
            {
               iDecoder->Close(iFile);
               fclose(iFile);
               iFileOpen = EFalse;
            }

            ClearComments();
            iTime = 0;
         }
      }
      break;

   case EMAudioFBCallbackStateDecodeCompleteStopped:
      // Occurs after stop has been called
      if ( iEof )  // Make sure this is a completed case not a user stop
      {
         if ( iObserver ) iObserver->NotifyPlayComplete();
         iEof = EFalse;
      }
      break;

   case EMAudioFBCallbackStateDecodeError:
      switch( aError )
      {
      case EMAudioFBCallbackErrorPriorityRejection:
      case EMAudioFBCallbackErrorResourceRejection:
      case EMAudioFBCallbackErrorAlertModeRejection:
      case EMAudioFBCallbackErrorBufferFull:
      case EMAudioFBCallbackErrorForcedStop:
      case EMAudioFBCallbackErrorForcedClose:
      case EMAudioFBCallbackErrorUnknown:
         // Setup the API to be deleted outside of the callback
         iDelete = iStream;
         iStream = NULL;
         iState  = EClosed;
         iObserver->NotifyPlayInterrupted();
         break;

      default:
         break;
      }
      break;

   default:
      break;
   }
}

// OnEvent
////////////////////////////////////////////////////////////////

void COggPlayback::OnEvent
( TMAudioFBCallbackState aState
, TInt                   aError
, TDes8*                 aBuffer
)
{
   switch( aState )
   {
   case EMAudioFBCallbackStateDecodeBufferDecoded:
      {
         for (TInt i = 0; i < KMotoBuffers; i++)
         {
            if ( aBuffer == iBuffer[i] )
            {
               iBufferFlag[i] = EBufferFree;
               if ( !iEof && (iState == EPlaying) )
               {
                  TRAPD( err, SendBufferL( i ) );
               }
               break;
            }
         }
      }
      break;

   default:
      OnEvent( aState, aError );
      break;
   }
}

// GetNewSamples
////////////////////////////////////////////////////////////////

TInt COggPlayback::GetNewSamples( TDes8 &aBuffer )
{
   TInt cnt = 0;
   
   if (!iEof)
   {
      TInt len = aBuffer.Length();
      
#ifdef MDCT_FREQ_ANALYSER
      if (iTimeWithoutFreqCalculation > iTimeWithoutFreqCalculationLim )
      {
         iTimeWithoutFreqCalculation = 0;
         iLastFreqArrayIdx++;
         if (iLastFreqArrayIdx >= KFreqArrayLength)
         {
            iLastFreqArrayIdx = 0;
         }

         iDecoder->GetFrequencyBins(iFreqArray[iLastFreqArrayIdx].FreqCoefs,KNumberOfFreqBins);
         iFreqArray[iLastFreqArrayIdx].Time =  TInt64(iLatestPlayTime) ;
      }
      else
      {
         iDecoder->GetFrequencyBins(NULL,0);
      }
#endif
      
      cnt = iDecoder->Read( aBuffer, len );

      if ( cnt > 0 )
      {
         aBuffer.SetLength( len + cnt );

#ifdef MDCT_FREQ_ANALYSER
         iLatestPlayTime += ( cnt * iTimeBetweenTwoSamples );
         iTimeWithoutFreqCalculation += cnt;
         //TRACEF(COggLog::VA(_L("U:%d %f "), cnt, iLatestPlayTime ));
#endif      
      }
      else
      {
         iEof = ETrue;
      }
   }
   
   return ( cnt );
}

// Private methods
////////////////////////////////////////////////////////////////

// CreateStreamApi
////////////////////////////////////////////////////////////////

void COggPlayback::CreateStreamApi( void )
{
   RDebug::Print( _L("COggPlayback::CreateStreamApi") );

   if ( iDelete ) delete iDelete;
   
   if ( !iStream )
   {
      TMAudioFBBufSettings settings;
      
      settings.iPCMSettings.iSamplingFreq = EMAudioFBSampleRate8K;
      settings.iPCMSettings.iStereo       = EFalse;
      
      iStream = CMAudioFB::NewL( EMAudioFBRequestTypeDecode
                               , EMAudioFBFormatPCM
                               , settings
                               , *this
                               , EMAudioPriorityPlaybackGeneric
                               );
   }
}

// GetString
////////////////////////////////////////////////////////////////

void COggPlayback::GetString( TBuf<256>& aBuf, const char* aStr )
{
   // according to ogg vorbis specifications, the text should be UTF8 encoded
   TPtrC8 p((const unsigned char *)aStr);
   CnvUtfConverter::ConvertToUnicodeFromUtf8(aBuf,p);
   
   // in the real world there are all kinds of coding being used!
   // so we try to find out what it is:
#if !defined(__VC32__)
   TInt i= j_code((char*)aStr,strlen(aStr));
   if (i==BIG5_CODE) 
   {
      CCnvCharacterSetConverter* conv= CCnvCharacterSetConverter::NewL();
      RFs theRFs;
      theRFs.Connect();
      CCnvCharacterSetConverter::TAvailability a= conv->PrepareToConvertToOrFromL(KCharacterSetIdentifierBig5, theRFs);
      if (a==CCnvCharacterSetConverter::EAvailable)
      {
         TInt theState= CCnvCharacterSetConverter::KStateDefault;
         conv->ConvertToUnicode(aBuf, p, theState);
      }
      theRFs.Close();
      delete conv;
   }
#endif
}

// ParseComments
////////////////////////////////////////////////////////////////

void COggPlayback::ParseComments(char** ptr)
{
   ClearComments();
   while (*ptr) 
   {
      char *s = *ptr;
      TBuf<256> buf;
      GetString(buf,s);
      buf.UpperCase();
           if (buf.Find(_L("ARTIST="))      == 0) GetString(iArtist,      s + 7); 
      else if (buf.Find(_L("TITLE="))       == 0) GetString(iTitle,       s + 6);
      else if (buf.Find(_L("ALBUM="))       == 0) GetString(iAlbum,       s + 6);
      else if (buf.Find(_L("GENRE="))       == 0) GetString(iGenre,       s + 6);
      else if (buf.Find(_L("TRACKNUMBER=")) == 0) GetString(iTrackNumber, s + 12);
      ++ptr;
   }
}

// SendBufferL
////////////////////////////////////////////////////////////////

void COggPlayback::SendBufferL( TInt aIndex )
{
   if ( !iEof && (iState == EPlaying) )
   {   
      iBuffer[ aIndex ]->SetLength( 0 );
      
      TInt cnt = iOggSampleRateConverter->FillBuffer( *iBuffer[ aIndex ] );
   
      if (cnt > 0)
      {
         TRAPD( err, iStream->QueueBufferL( iBuffer[ aIndex ] ) );
         if (err != KErrNone)
         {
            // This should never ever happen.
            TBuf<256> cbuf, tbuf;
            CEikonEnv::Static()->ReadResource( cbuf, R_OGG_ERROR_16 );
            CEikonEnv::Static()->ReadResource( tbuf, R_OGG_ERROR_17 );
            cbuf.AppendNum(err);
            iEnv->OggErrorMsgL(tbuf,cbuf);
            User::Leave(err);
         }

         iBufferFlag[ aIndex ] = EBufferInUse;
      }
   }
}

// SetAudioCaps
////////////////////////////////////////////////////////////////

TInt COggPlayback::SetAudioCaps(TInt aChannels, TInt aRate)
{
   TBool stereo = ( aChannels > 1 ) ? EFalse : ETrue;
   iStream->SetPCMChannel( stereo );

   TInt rate = 16000; // The default
   switch( aRate )
   {
   case 8000:
   case 11025:
   case 12000:
   case 16000:
   case 22050:
   case 24000:
   case 32000:
   case 44100:
   case 48000:
      rate = aRate;
      break;

   default:
      break;
   };
   iStream->SetPCMSampleRate( (TMSampleRate) rate );

   iOggSampleRateConverter->Init( this
                                , KMotoBuffers
                                , (TInt) (0.75 * KMotoBuffers)
                                , aRate
                                , rate
                                , aChannels
                                , ( stereo ) ? 2 : 1
                                );

   return ( KErrNone );
}

// SetDecoderL
////////////////////////////////////////////////////////////////

void COggPlayback::SetDecoderL( const TDesC& aFileName )
{
  if( iDecoder )
  { 
    delete iDecoder; 
    iDecoder = NULL; 
  }

  TParsePtrC p( aFileName);

  if( p.Ext().Compare( _L(".ogg"))==0 || p.Ext().Compare( _L(".OGG"))==0 )
  {
      iDecoder=new(ELeave)CTremorDecoder;
  } 
#if defined(MP3_SUPPORT)
  else if( p.Ext().Compare( _L(".mp3"))==0 || p.Ext().Compare( _L(".MP3"))==0 )
  {
    iDecoder=new(ELeave)CMadDecoder;
  }
#endif
  else
  {
    _LIT(KPanic,"Panic:");
    _LIT(KNotSupported,"File type not supported");
    iEnv->OggErrorMsgL(KPanic,KNotSupported);
  }
}

