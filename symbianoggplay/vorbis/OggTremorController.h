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



#ifndef CAMCCONTROLLER_H
#define CAMCCONTROLLER_H

// INCLUDES
#include <E32Base.h>
#include <e32std.h>
#include <ImplementationProxy.h>

#include <mmfcontroller.h>
#include <mmf\common\mmfstandardcustomcommands.h>

// FORWARD DECLARATIONS


// CLASS DECLARATION

class COggTremorController :	public CMMFController
	{
	public:

		/**
        * Controller internal states
        */
		enum TOggTremorControllerState
	    {
			EStateOpen = 0,
			EStateStopped,
			EStatePrepared,
			EStateRecording
		};

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
        * From CMMFController Set camcorder priority settings.
        * @since
        * @param aPrioritySettings Wanted priority.
        * @return void
        */
		void SetPrioritySettings(const TMMFPrioritySettings& aPrioritySettings);

		/**
        * From CMMFController Handle custom commands to controller.
        * @since
        * @param aMessage Message to controller.
        * @return void
        */
		void CustomCommand(TMMFMessage& aMessage)
			{aMessage.Complete(KErrNotSupported);};//default implementation

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

	private: // Data

		// Controller internal state
	    TOggTremorControllerState iState;
		
	};

#endif    
            
// End of File
