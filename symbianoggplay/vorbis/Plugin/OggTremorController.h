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



#ifndef OGGTREMORCONTROLLER_H
#define OGGTREMORCONTROLLER_H

// INCLUDES
#include <E32Base.h>
#include <e32std.h>
#include <OggOs.h>

#ifdef MMF_AVAILABLE
#include <ImplementationProxy.h>
#include <mmfcontroller.h>
#include <mmf\common\mmfstandardcustomcommands.h>
#include <MmfAudioOutput.h>
#include <MmfFile.h>
#else
#include "OggPlayPlugin.h"
#endif

#include "AdvancedStreaming.h"

#include <stdio.h>
#include <stdlib.h>

#include "ivorbiscodec.h"
#include "ivorbisfile.h"

// FORWARD DECLARATIONS


// CLASS DECLARATION

#ifdef MMF_AVAILABLE
class COggTremorController :	public CMMFController, public MAdvancedStreamingObserver
#else
class COggTremorController :	public CPseudoMMFController, public MAdvancedStreamingObserver
#endif

	{

	public:	 // Constructors and destructor

        /**
        * Two-phased constructor.
        */
		static COggTremorController* NewL();

        /**
        * Destructor.
        */
		~COggTremorController();

	private:

		/**
        * Symbian 2nd phase constructor.
        */
		void ConstructL();

	public:	// Functions from base classes

#ifdef MMF_AVAILABLE
		/**
        * From CMMFController Add data source to controller.
        * @since
        * @param aDataSource A reference to the data source.
        * @return void
        */
		void AddDataSourceL(MDataSource& aDataSource);
	
		/**
        * From CMMFController Add data sink to controller.
        * @since
        * @param aDataSink A reference to the data sink.
        * @return void
        */
		void AddDataSinkL(MDataSink& aDataSink);

		/**
        * From CMMFController Remove data source from controller.
        * @since
        * @param  aDataSource A reference to the data source.
        * @return void
        */
		void RemoveDataSourceL(MDataSource& aDataSource);

		/**
        * From CMMFController Remove data sink from controller.
        * @since
        * @param aDataSink A reference to the data sink.
        * @return void
        */
		void RemoveDataSinkL(MDataSink& aDataSink);
        
		/**
        * From CMMFController Handle custom commands to controller.
        * @since
        * @param aMessage Message to controller.
        * @return void
        */
		void CustomCommand(TMMFMessage& aMessage)
			{aMessage.Complete(KErrNotSupported);};//default implementation
        
		/**
        * From CMMFController Set priority settings.
        * @since
        * @param aPrioritySettings Wanted priority.
        * @return void
        */
		void SetPrioritySettings(const TMMFPrioritySettings& aPrioritySettings);

#else
        void SetObserver(MMdaAudioPlayerCallback &anObserver);
        // Non Leaving version of some of the MMF functions.
        // These functions acts as the framework does, calling the observer
        // instead of leaving.
        
        void Play();
        void Pause();
        void Stop();
        void OpenFile(const TDesC& aFile);
#endif
		/**
        * From CMMFController Reset controller.
        * @since
        * @param void
        * @return void
        */
		void ResetL();

		/**
        * From CMMFController  Primes controller.
        * @since
        * @param void
        * @return void
        */
		void PrimeL();

		/**
        * From CMMFController Start recording.
        * @since
        * @param void
        * @return void
        */
		void PlayL();

		/**
        * From CMMFController Pause recording.
        * @since
        * @param void
        * @return void
        */
		void PauseL();

		/**
        * From CMMFController Stop recording.
        * @since
        * @param void
        * @return void
        */
		void StopL();

		/**
        * From CMMFController Returns current recording position.
        * @since
        * @param void
        * @return Current position of recording.
        */
		TTimeIntervalMicroSeconds PositionL() const;

		/**
        * From CMMFController Sets current recording position. Not supported.
        * @since
        * @param aPosition Reference to wanted position.
        * @return void
        */
		void SetPositionL(const TTimeIntervalMicroSeconds& aPosition);

		/**
        * From CMMFController Returns current maximum duration.
        * @since
        * @param void 
        * @return Maximum remaining duration of recording.
        */
		TTimeIntervalMicroSeconds DurationL() const;


		/**
        * From CMMFController Get number of metadata entries.
        * @since
        * @param aNumberOfEntries Reference used to return metadata count.
        * @return void
        */
		void GetNumberOfMetaDataEntriesL(TInt& aNumberOfEntries);

		/**
        * From CMMFController Returns metadata entry.
        * @since
        * @param aIndex Index to metadata entry.
        * @return Metadata entry
        */
		CMMFMetaDataEntry* GetMetaDataEntryL(TInt aIndex);

        

    private: // Internal Functions
        void ParseComments(char** ptr);
        void ClearComments();
        void GetString(TBuf<256>& aBuf, const char* aStr);

        TInt GetNewSamples(TDes8 &aBuffer) ; 
        void NotifyPlayInterrupted(TInt aError) ;
        void OpenFileL(const TDesC& aFile);

	private: // Data

		/**
        * Controller internal states
        */
		enum TOggTremorControllerState
	    {
			EStateOpen = 0,
			EStateStopped,
			EStatePrepared,
			EStatePlaying
		};

	    TOggTremorControllerState iState;

        // OggTremor stuff
        TBuf<100> iFileName;
        TBool iFileOpen;
        FILE *iFile;
        OggVorbis_File           iVf; 
        TInt                     iCurrentSection; // used in the call to ov_read

        
        TBuf<256>                iTitle;
        TBuf<256>                iAlbum;
        TBuf<256>                iArtist;
        TBuf<256>                iGenre;
        TBuf<256>                iTrackNumber;
        TInt64                   iTime;
        TInt                     iRate;
        TInt                     iChannels;
        TInt                     iFileSize;
        TInt64                   iBitRate;
        TBool                    iEof;

        CAdvancedStreaming *iAdvancedStreaming;
#ifndef MMF_AVAILABLE
        MMdaAudioPlayerCallback *iObserver;
#endif
	};

    enum {
        KOggPlayPluginErrNotReady = -200,
        KOggPlayPluginErrNotSupported,
        KOggPlayPluginErrFileNotFound,
        KOggPlayPluginErrOpeningFile
    };

#endif    
            
// End of File
