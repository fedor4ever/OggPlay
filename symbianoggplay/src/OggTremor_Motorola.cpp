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
#include "Utf8Fix.h"
#endif

#include <OggPlay.rsg>

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
, iEof( EFalse )
, iDecoding( EFalse )
, iDecoder( NULL )
{
   for ( TInt j = 0; j < KMotoBuffers; j++ )
   {
      iBufferFlag[ j ] = EBufferFree;
   }

   iObserverAO = NULL;
}

// ~COggPlayback
////////////////////////////////////////////////////////////////

COggPlayback::~COggPlayback( void )
{
   __ASSERT_ALWAYS((iState == EClosed) || (iState == EStopped), User::Panic(_L("~COggPlayback"), 0));
	
   delete iDecoder; 
   delete iStream;
   delete iDelete;
 
   iBuffer.ResetAndDestroy();
   delete iObserverAO;
   delete iOggSampleRateConverter;

   iFs.Close();
}

// ConstructL
////////////////////////////////////////////////////////////////

void COggPlayback::ConstructL( void )
{
#ifdef _DEBUG
   RDebug::Print( _L("COggPlayback::ConstructL") );
#endif

   // Set up the session with the file server
   User::LeaveIfError(iFs.Connect());

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

   iObserverAO = new (ELeave) CObserverCallback( this );
   iOggSampleRateConverter = new (ELeave) COggSampleRateConverter;
}

// Public interface methods
////////////////////////////////////////////////////////////////

// Info
////////////////////////////////////////////////////////////////

TInt COggPlayback::Info(const TDesC& aFileName, TBool silent)
{
   if (aFileName.Length()==0) return -100;

   RFile* f = new RFile;
   if ((f->Open(iFs, aFileName, EFileShareReadersOnly)) != KErrNone) 
   {
      if (!silent) 
      {
         TBuf<128> buf;
         CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_14);
         iEnv->OggErrorMsgL(buf,aFileName);
      }

      return KErrOggFileNotFound;
   }

   MDecoder* decoder = GetDecoderL( aFileName );
   if ( decoder->OpenInfo(f, aFileName) < 0 )
   {
      decoder->Close();
	  delete decoder;

      f->Close();
      delete f;

      if (!silent)
      {
         iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_9);
      }
      return -102;
   }
   decoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);

   iFileName = aFileName;
   iRate     = iDecoder->Rate();
   iChannels = iDecoder->Channels();
   iBitRate  = iDecoder->Bitrate();

   decoder->Close();
   delete decoder;

   f->Close();
   delete f;

   return KErrNone;
}

// Open
////////////////////////////////////////////////////////////////

TInt COggPlayback::Open( const TDesC& aFileName )
{
#ifdef _DEBUG
   RDebug::Print( _L("Open:%S"), &aFileName );
#endif

   TBool just_created_api = EFalse;

   if ( iStream && iDecoding )
   {
      ResetStreamApi();
   }

   if ( !iStream )
   {
      CreateStreamApi();
      just_created_api = ETrue;
   }
   
   if (iFile)
   {
      iDecoder->Close();
      iFile->Close();
	
	  delete iFile;
	  iFile = NULL;

	  delete iDecoder;
 	  iDecoder = NULL;
   }
   
   iTime = 0;
   
   if ( aFileName.Length() == 0 )
   {
      iEnv->OggErrorMsgL(R_OGG_ERROR_20,R_OGG_ERROR_8);
      return -100;
   }
   
   iFile = new(ELeave) RFile;
   if ((iFile->Open(iFs, aFileName, EFileShareReadersOnly)) != KErrNone)
   {
      delete iFile;
      iFile = NULL;

	  iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_14);
      return KErrOggFileNotFound;
   }
   
   iDecoder = GetDecoderL(aFileName);
   if(iDecoder->Open(iFile, aFileName) < 0)
   {
      iDecoder->Close();
	  iFile->Close();

      delete iFile;
      iFile = NULL;
    
      delete iDecoder;
      iDecoder = NULL;

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
      if ( !just_created_api )
		  iState = EOpen;
   }
   else
   {
      iDecoder->Close();
      iFile->Close();
  
      delete iFile;
      iFile = NULL;

      delete iDecoder;
      iDecoder = NULL;
   }

   return err;
}

// Pause
////////////////////////////////////////////////////////////////

void COggPlayback::Pause()
{
#ifdef _DEBUG
   RDebug::Print( _L("Pause:%d"), iState );
#endif

   if (iState == EPlaying && !iPlayWhenOpened )
   {
      iState = EPaused;
   }
}

void COggPlayback::Resume()
{
  //Resume is equivalent of Play() 
  Play();
}

// Play
////////////////////////////////////////////////////////////////

void COggPlayback::Play() 
{
#ifdef _DEBUG
   RDebug::Print( _L("Play:%d"), iState );
#endif

#ifdef MDCT_FREQ_ANALYSER
  iLastFreqArrayIdx = 0;
  iLatestPlayTime = 0.0;
#endif

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
         if ( !iDecoding )
         {
         TRAP( err, iStream->DecodeL() );
            if ( err == KErrNone ) iDecoding = ETrue;
         }
      }
      break;
      
   case EStopped:
      if ( iStream )
      {
         iState = EPlaying;
         iPlayWhenOpened = ETrue;
         break;
      }
      // Else fall-thru

   default:
      iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_15);
#ifdef _DEBUG
      RDebug::Print(_L("Oggplay: Tremor - State not Open"));
#endif
      break;
   }
}

// Stop
////////////////////////////////////////////////////////////////

void COggPlayback::Stop()
{
#ifdef _DEBUG
   RDebug::Print( _L("Stop:%d"), iState );
#endif

   if ( (iState == EPlaying && !iPlayWhenOpened) || iState == EPaused)
   { 
      iStream->Stop();
      iState = EStopped;

      if (iFile)
      {
         iDecoder->Close();
         iFile->Close();

	     delete iFile;
	     iFile = NULL;

	     delete iDecoder;
	     iDecoder = NULL;
      }

      for (TInt i = 0; i < KMotoBuffers; i++)
      {
         iBufferFlag[i] = EBufferFree;
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
const TInt32 * COggPlayback::GetFrequencyBins()
{
   TTimeIntervalMicroSeconds currentPos;
   iStream->Position( currentPos );
   
   TInt idx;
   idx = iLastFreqArrayIdx;
   
   for (TInt i=0; i<KFreqArrayLength; i++)
   {
      if (iFreqArray[idx].iTime < currentPos)
      {
         break;
      }
      
      idx--;
      if (idx < 0)
      {
         idx = KFreqArrayLength-1;
      }
   }
   
   return iFreqArray[idx].iFreqCoefs;
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
#ifdef _DEBUG
   RDebug::Print( _L("OnEvent:%d:%d"), aState, aError );
#endif

   switch ( aState )
   {
   case EMAudioFBCallbackStateReady:
      iState = EOpen;
      iMaxVolume = iStream->GetMaxVolume();
      if ( iPlayWhenOpened )
      {
         iPlayWhenOpened = EFalse;
         iStream->SetPCMChannel( iApiStereo );
         iStream->SetPCMSampleRate( (TMSampleRate) iApiRate );
         Play();
      }
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
            iState = EStopped;

            if (iFile)
            {
               iDecoder->Close();
               iFile->Close();

	           delete iFile;
	           iFile = NULL;

	           delete iDecoder;
	           iDecoder = NULL;
            }

            ClearComments();
            iTime = 0;
         }
      }
      break;

   case EMAudioFBCallbackStateDecodeCompleteStopped:
      iDecoding = EFalse;

      // Occurs after stop has been called
      if ( iEof )  // Make sure this is a completed case not a user stop
      {
         if ( iObserverAO ) iObserverAO->NotifyPlayComplete();
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
         iState  = EStopped;
         if ( iObserverAO ) iObserverAO->NotifyPlayInterrupted();
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
#ifdef _DEBUG
   RDebug::Print( _L("OnEvent2:%d:%d"), aState, aError );
#endif

   switch( aState )
   {
   case EMAudioFBCallbackStateDecodeBufferDecoded:
      {
         for (TInt i = 0; i < KMotoBuffers; i++)
         {
            if ( aBuffer == iBuffer[i] )
            {
               iBufferFlag[i] = EBufferFree;
               if ( !iEof && (iState == EPlaying && !iPlayWhenOpened) )
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

TInt COggPlayback::GetNewSamples( TDes8 &aBuffer, TBool aRequestFrequencyBins )
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

         iDecoder->GetFrequencyBins(iFreqArray[iLastFreqArrayIdx].iFreqCoefs, KNumberOfFreqBins);
         iFreqArray[iLastFreqArrayIdx].iTime =  TInt64(iLatestPlayTime) ;
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
#ifdef _DEBUG
   RDebug::Print( _L("COggPlayback::CreateStreamApi:%d"), iState );
#endif

   if ( iDelete )
   {
      delete iDelete;
      iDelete = NULL;
   }
   
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


// ResetStreamApi
////////////////////////////////////////////////////////////////

void COggPlayback::ResetStreamApi( void )
{
#ifdef _DEBUG
   RDebug::Print( _L("ResetStreamApi:%d"), iState );
#endif

   if ( iStream )
   {
      delete iStream;
      iStream = NULL;
   }

   if ( iDelete )
   {
      delete iDelete;
      iDelete = NULL;
   }

   iDecoding = EFalse;
   iState    = EStopped;
   for (TInt i = 0; i < KMotoBuffers; i++)
   {
      iBufferFlag[i] = EBufferFree;
   }
}

// SendBufferL
////////////////////////////////////////////////////////////////

void COggPlayback::SendBufferL( TInt aIndex )
{
#ifdef _DEBUG
   RDebug::Print( _L("SendBufferL:%d"), aIndex );
#endif

   if ( !iEof && (iState == EPlaying) && (!iPlayWhenOpened) && (iBufferFlag[aIndex] == EBufferFree) )
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
   iApiStereo = ( aChannels > 1 ) ? ETrue : EFalse;
   iStream->SetPCMChannel( iApiStereo );

   iApiRate = 16000; // The default
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
      iApiRate = aRate;
      break;

   default:
      break;
   };
   iStream->SetPCMSampleRate( (TMSampleRate) iApiRate );

   iOggSampleRateConverter->Init( this
                                , KMotoBuffers
                                , (TInt) (0.75 * KMotoBuffers)
                                , aRate
                                , iApiRate
                                , aChannels
                                , ( iApiStereo ) ? 2 : 1
                                );

   return ( KErrNone );
}

// GetDecoderL
////////////////////////////////////////////////////////////////
MDecoder* COggPlayback::GetDecoderL(const TDesC& aFileName)
{
  MDecoder* decoder = NULL;

  TParsePtrC p( aFileName);
  if( p.Ext().Compare( _L(".ogg"))==0 || p.Ext().Compare( _L(".OGG"))==0 )
  {
      decoder=new(ELeave) CTremorDecoder(iFs);
  }

#if defined(MP3_SUPPORT)
  else if( p.Ext().Compare( _L(".mp3"))==0 || p.Ext().Compare( _L(".MP3"))==0 )
  {
    decoder=new(ELeave)CMadDecoder;
  }
#endif
  else
  {
    _LIT(KPanic,"Panic:");
    _LIT(KNotSupported,"File type not supported");
    iEnv->OggErrorMsgL(KPanic,KNotSupported);
  }

  return decoder;
}

////////////////////////////////////////////////////////////////
//
// CObserverCallback
//
////////////////////////////////////////////////////////////////
COggPlayback::CObserverCallback::CObserverCallback( COggPlayback *aParent )
: CActive( EPriorityHigh ), iParent( aParent )
{
   CActiveScheduler::Add(this);
   for ( TInt i = 0; i < KMaxIndex; i++ ) iCBType[i] = ECBNone;
}

// NotifyPlayComplete
////////////////////////////////////////////////////////////////

void COggPlayback::CObserverCallback::NotifyPlayComplete( void )
{
   DoIt( ECBComplete );
}

// NotifyUpdate
////////////////////////////////////////////////////////////////

void COggPlayback::CObserverCallback::NotifyUpdate( void )
{
   DoIt( ECBUpdate );
}

// NotifyPlayInterrupted
////////////////////////////////////////////////////////////////

void COggPlayback::CObserverCallback::NotifyPlayInterrupted( void )
{
   DoIt( ECBInterrupted );
}

// DoIt - kick off the AO
////////////////////////////////////////////////////////////////

void COggPlayback::CObserverCallback::DoIt( TInt aCBType )
{
   for ( TInt i = 0; i < KMaxIndex; i++ )
   {
      if ( iCBType[i] == ECBNone )
      {
         iCBType[i] = aCBType;
         if ( !IsActive() )
         {
            iStatus = KErrNone;
            SetActive();
            TRequestStatus *status = &iStatus;
            User::RequestComplete( status, KErrNone );
         }
         break;
      }
   }
}

// RunL
////////////////////////////////////////////////////////////////

void COggPlayback::CObserverCallback::RunL( void )
{
   if ( iParent && iParent->iObserver )
   {
      TInt i = 0;
      for ( ; i < KMaxIndex; i++ )
      {
         switch ( iCBType[i] )
         {
         case ECBComplete:
            iParent->iObserver->NotifyPlayComplete();
            break;

         case ECBUpdate:
            iParent->iObserver->NotifyUpdate();
            break;

         case ECBInterrupted:
            iParent->iObserver->NotifyPlayInterrupted();
            break;

         default:
            break;
         }
         iCBType[i] = ECBNone;
      }
   }
}

// DoCancel
////////////////////////////////////////////////////////////////

void COggPlayback::CObserverCallback::DoCancel( void )
{
}
   
