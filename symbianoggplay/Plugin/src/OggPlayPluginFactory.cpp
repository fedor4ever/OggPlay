/*
 *  Copyright (c) 2007 S. Fisher 
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


#include <OggOs.h>
#include "OggPlayController.h"

#if defined(SERIES60V3)
#include "S60V3ImplementationUIDs.hrh"
#else
#include "ImplementationUIDs.hrh"
#endif

// ImplementationTable
// Exported proxy for instantiation method resolution.
COggPlayController* NewOggPlayControllerL()
    {
	// Create the controller
    return COggPlayController::NewL();
    }

const TImplementationProxy ImplementationTable[] =
	{
#if defined(SERIES60)
	IMPLEMENTATION_PROXY_ENTRY(KOggPlayUidControllerImplementation, NewOggPlayControllerL)
#else
	{{KOggPlayUidControllerImplementation}, NewOggPlayControllerL}
#endif
	};

// ImplementationGroupProxy 
// Returns implementation table and count to ECOM
// Returns: ImplementationTable: pointer to implementation table
EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

#if !defined(SERIES60V3)
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
	{
	return KErrNone;
	}

#endif
