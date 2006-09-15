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
#include "TremorDecoder.h"
#ifdef MMF_AVAILABLE
#include "ImplementationUIDs.hrh"

// -----------------------------------------------------------------------------
// ImplementationTable
// Exported proxy for instantiation method resolution.
// -----------------------------------------------------------------------------
//
COggPlayController* NewOggTremorControllerL()
    {
	// Create the file server session
	RFs* fs = new RFs;
	if (!fs)
		User::Leave(KErrNoMemory);

	// Connect to the file server
	TInt err = fs->Connect();
	if (err != KErrNone)
	{
		delete fs;
		User::Leave(err);
	}

	// Create the decoder
    CTremorDecoder* decoder = new CTremorDecoder(*fs);
	if (!decoder)
	{
		fs->Close();
		delete fs;

		User::Leave(KErrNoMemory);
	}

	// Create the controller, transferring ownership of the file server session and decoder
    return COggPlayController::NewL(fs, decoder);
    }

const TImplementationProxy ImplementationTable[] =
	{
#if defined(SERIES60)
//FL		{{KOggTremorUidControllerImplementation}, NewOggTremorControllerL}
		IMPLEMENTATION_PROXY_ENTRY(KOggTremorUidControllerImplementation, NewOggTremorControllerL)
#else
		{{KOggTremorUidControllerImplementation}, NewOggTremorControllerL}
#endif
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


#else
EXPORT_C COggPlayController* NewOggTremorControllerL()
    {
	// Create the file server session
	RFs* fs = new RFs;
	if (!fs)
		User::Leave(KErrNoMemory);

	// Connect to the file server
	TInt err = fs->Connect();
	if (err != KErrNone)
	{
		delete fs;
		User::Leave(err);
	}

	// Create the decoder
    CTremorDecoder* decoder = new CTremorDecoder(*fs);
	if (!decoder)
	{
		fs->Close();
		delete fs;

		User::Leave(KErrNoMemory);
	}

	// Create the controller, transferring ownership of the file server session and decoder
    return COggPlayController::NewL(fs, decoder);
    }
#endif

#if !defined(EKA2)
// -----------------------------------------------------------------------------
// E32Dll DLL Entry point
// -----------------------------------------------------------------------------
//
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
	{
	return(KErrNone);
	}

#endif // EKA2
//  End of File
