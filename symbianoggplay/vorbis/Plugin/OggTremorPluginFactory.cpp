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


// INCLUDE FILES
#include <OggOs.h>
#include "OggPlayController.h"
#ifdef MMF_AVAILABLE
#include "ImplementationUIDs.hrh"
#include "TremorDecoder.h"


#ifdef MMF_AVAILABLE
COggPlayController* NewOggTremorControllerL()
#else
EXPORT_C COggPlayController* NewOggTremorControllerL()
#endif
    {
    CTremorDecoder * decoder = new (ELeave) CTremorDecoder;
    return COggPlayController::NewL(decoder);
    }



// -----------------------------------------------------------------------------
// ImplementationTable
// Exported proxy for instantiation method resolution.
// -----------------------------------------------------------------------------
//
const TImplementationProxy ImplementationTable[] =
	{
		{{KOggTremorUidControllerImplementation}, NewOggTremorControllerL}
	};

// -----------------------------------------------------------------------------
// ImplementationGroupProxy 
// Returns implementation table and count to ECOM
// Returns: ImplementationTable: pointer to implementation table
// -----------------------------------------------------------------------------
//
EXPORT_C const TImplementationProxy* ImplementationGroupProxy(
	TInt& aTableCount)	// reference used to return implementation count
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

#endif


// -----------------------------------------------------------------------------
// E32Dll DLL Entry point
// -----------------------------------------------------------------------------
//
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
	{
	return(KErrNone);
	}


//  End of File