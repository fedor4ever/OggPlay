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

#ifndef _OggTremor_Motorola_h
#define _OggTremor_Motorola_h

#include "cmaudiofb.h"

const TInt KMotoBuffers     = 12;
const TInt KMotoBufferSize  = 1024 * 6;

////////////////////////////////////////////////////////////////
//
// COggPlayback class
//
// An implementation of TAbsPlayback for the Ogg/Vorbis format:
//
///////////////////////////////////////////////////////////////

class COggPlayback : public MMAudioFBObserver, 
                     public MOggSampleRateFillBuffer,
		               public CAbsPlayback
{
public:
   // Construction/Destruction methods
   COggPlayback( COggMsgEnv* anEnv, MPlaybackObserver* anObserver = NULL );
   virtual ~COggPlayback( void );
   void   ConstructL( void );

   // Implementation of CAbsPlayback pure virtual methods 
   TInt   Info( const TDesC& aFileName, TBool silent = EFalse );
   TInt   Open( const TDesC& aFileName );
   void   Pause( void );
   void Resume();
   void   Play( void );
   void   Stop( void );
   void   SetPosition( TInt64 aPos );
   TInt64 Position( void );
   TInt64 Time( void );
   void   SetVolume( TInt aVol );
   void   SetVolumeGain(TGainType aGain);
   TInt   Volume( void );
#ifdef MDCT_FREQ_ANALYSER
   const TInt32* GetFrequencyBins();
#else
   const void* GetDataChunk( void );
#endif

   // Other local methods
   virtual TInt GetNewSamples( TDes8 &aBuffer );
   
private:
   
   // Implementation of MMAudioFBObserver methods
   void OnEvent( TMAudioFBCallbackState aState, TInt aError );
   void OnEvent( TMAudioFBCallbackState aState, TInt aError, TDes8 *aBuffer );

   void CreateStreamApi( void );
   void GetString( TBuf<256>& aBuf, const char* aStr );
   void ParseComments( char** ptr );
   void ResetStreamApi( void );
   void SendBufferL( TInt aIndex );
   TInt SetAudioCaps( TInt aChannels, TInt aRate );
   void SetDecoderL( const TDesC& aFileName );
   
   COggMsgEnv              *iEnv;   
   COggSampleRateConverter *iOggSampleRateConverter;
   
   CMAudioFB               *iStream;
   CMAudioFB               *iDelete;

   RPointerArray<TDes8>     iBuffer;
   enum { EBufferFree, EBufferInUse };
   TInt                     iBufferFlag[KMotoBuffers];

   TInt                     iMaxVolume;
   TInt                     iSentIdx;
   TBool                    iPlayWhenOpened;
   
   // Communication with the tremor/ogg codec:
   //-----------------------------------------
   FILE*                    iFile;
   TBool                    iFileOpen;
   TBool                    iEof;            // true after ov_read has encounted the eof
   TBool                    iDecoding;
   MDecoder*                iDecoder;

   TInt                     iApiRate;
   TInt                     iApiStereo;

   class CObserverCallback : public CActive
   {
   public:
      CObserverCallback( COggPlayback *aParent );

      void NotifyPlayComplete( void );
      void NotifyUpdate( void );
      void NotifyPlayInterrupted( void );

   private:
      void DoIt( TInt aCBType );
      void RunL( void );
      void DoCancel( void );

      enum { KMaxIndex = 5 };
      enum { ECBNone, ECBComplete, ECBUpdate, ECBInterrupted };
      TInt iCBType[KMaxIndex];

      COggPlayback *iParent;
   };
   
   friend CObserverCallback;
   CObserverCallback *iObserverAO;

#ifdef MDCT_FREQ_ANALYSER
   TReal  iLatestPlayTime;
   TReal  iTimeBetweenTwoSamples;
   
   typedef struct 
   {
      TTimeIntervalMicroSeconds Time;
      TInt32 FreqCoefs[KNumberOfFreqBins];
   } TFreqBins;
   
   TFixedArray<TFreqBins,KFreqArrayLength> iFreqArray;
   TInt iLastFreqArrayIdx;
   TInt iTimeWithoutFreqCalculation;
   TInt iTimeWithoutFreqCalculationLim;
#endif
};

#endif

