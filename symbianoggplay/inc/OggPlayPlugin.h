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

///////////////////////////////////////////////////////////////////////
// OGGPLAY PLUGIN INTERFACE
///////////////////////////////////////////////////////////////////////

#ifndef OGGPLAYPLUGIN_H
#define OGGPLAYPLUGIN_H

// INCLUDES
#include <E32Base.h>
#include <e32std.h>
#include <MdaAudioSamplePlayer.h>
#include "OggOs.h"

#ifndef MMF_AVAILABLE
/**
 Copy of the header from 7.0s mmfcontroller.h
*/
class CMMFMetaDataEntry : public CBase
	{
public:
	IMPORT_C static CMMFMetaDataEntry* NewL(const TDesC& aName, const TDesC& aValue);
	IMPORT_C ~CMMFMetaDataEntry();
	IMPORT_C const TDesC& Name() const;
	IMPORT_C const TDesC& Value() const;
	IMPORT_C void SetNameL(const TDesC& aName);
	IMPORT_C void SetValueL(const TDesC& aValue);

private:
	CMMFMetaDataEntry();
	void ConstructL(const TDesC& aName, const TDesC& aValue);
private:
/**
The name, or category, of the meta data.
*/
	HBufC* iName;

/**
The value of the meta data.
*/
	HBufC* iValue;
	};



// An abstract base class defining an interface very close to the MMF controller interface

class CPseudoMMFController : public CBase
{
    
public:
    virtual void SetObserver(MMdaAudioPlayerCallback &anObserver) = 0;
    virtual void OpenFile(const TDesC& aFile) = 0;
    virtual void ConstructL()= 0;
    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;
    virtual TTimeIntervalMicroSeconds PositionL() const = 0;
    virtual void SetPositionL(const TTimeIntervalMicroSeconds& aPosition) = 0;
    virtual TTimeIntervalMicroSeconds DurationL() const = 0;
    virtual void GetNumberOfMetaDataEntriesL(TInt& aNumberOfEntries) = 0;
    virtual CMMFMetaDataEntry* GetMetaDataEntryL(TInt aIndex) =0;
};


#endif /*!MMF_AVAILABLE */

#endif