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



#ifndef OGGPLAYCONTROLLER_H
#define OGGPLAYCONTROLLER_H

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

#include "OggPlayDecoder.h"
#include "AdvancedStreaming.h"

#include <stdio.h>
#include <stdlib.h>

// FORWARD DECLARATIONS


// CLASS DECLARATION

#ifdef MMF_AVAILABLE
class COggPlayController :	public CMMFController,
                            public MMMFAudioPlayDeviceCustomCommandImplementor,
                            public MMMFAudioPlayControllerCustomCommandImplementor,
                            public MAdvancedStreamingObserver
#else
class COggPlayController :	public CPseudoMMFController,
                            public MAdvancedStreamingObserver
#endif

	{

	public:	 // Constructors and destructor

        /**
        * Two-phased constructor.
        */
        // The decoder passed in the NewL is owned and will be destroyed by the controller
		static COggPlayController* NewL(MDecoder *aDecoder);

        /**
        * Destructor.
        */
		~COggPlayController();

	private:

        COggPlayController(MDecoder *aDecoder);
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
        
        TInt MaxVolume();
        void SetVolume(TInt aVolume);
        TInt GetVolume(TInt& aVolume);
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

        void MapdSetVolumeL(TInt aVolume);
        void MapdGetMaxVolumeL(TInt& aMaxVolume);
        void MapdGetVolumeL(TInt& aVolume);
        void MapdSetVolumeRampL(const TTimeIntervalMicroSeconds& aRampDuration);
        void MapdSetBalanceL(TInt aBalance);
        void MapdGetBalanceL(TInt& aBalance);
        void MapcSetPlaybackWindowL(const TTimeIntervalMicroSeconds& aStart,
            const TTimeIntervalMicroSeconds& aEnd);
        void MapcDeletePlaybackWindowL();
        void MapcGetLoadingProgressL(TInt& aPercentageComplete);

    private: // Internal Functions

        TInt GetNewSamples(TDes8 &aBuffer) ; 
        void NotifyPlayInterrupted(TInt aError) ;
        void OpenFileL(const TDesC& aFile);

	private: // Data

		/**
        * Controller internal states
        */
		enum TOggPlayControllerState
	    {
			EStateNotOpened = 0,
            EStateOpen,
			EStatePlaying,
            EStatePaused
		};

	    TOggPlayControllerState iState;

        // OggTremor stuff
        TBuf<100> iFileName;
        FILE *iFile;
       
        
        enum { KMaxStringLength = 256 };
        TBuf<KMaxStringLength>   iAlbum;
        TBuf<KMaxStringLength>   iArtist;
        TBuf<KMaxStringLength>   iGenre;
        TBuf<KMaxStringLength>   iTitle;
        TBuf<KMaxStringLength>   iTrackNumber;


        CAdvancedStreaming *iAdvancedStreaming;
        MDecoder *iDecoder;
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
