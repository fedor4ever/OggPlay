/*
*  Copyright (c) 2004 OggPlayTeam
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

#include "OggOs.h"
#include "OggPlayPlugin.h"

#ifndef MMF_AVAILABLE
///////////////////////////////////////////////////////////////////
// Implementation of the MMFMetaDataEntry for pre 7.0s
///////////////////////////////////////////////////////////////////
CMMFMetaDataEntry* CMMFMetaDataEntry::NewL(const TDesC& aName, const TDesC& aValue)
{
    CMMFMetaDataEntry* self = new(ELeave) CMMFMetaDataEntry();
    CleanupStack::PushL(self);
    self->ConstructL(aName, aValue);
    CleanupStack::Pop();
    return self;
}

CMMFMetaDataEntry::~CMMFMetaDataEntry()
{
    delete iName;
    delete iValue;
}

const TDesC& CMMFMetaDataEntry::Name() const
{
    return(*iName);
}
const TDesC& CMMFMetaDataEntry::Value() const
{
    return(*iValue);
}
void CMMFMetaDataEntry::SetNameL(const TDesC& aName)
{
    iName = aName.AllocL();
}
void CMMFMetaDataEntry::SetValueL(const TDesC& aValue)
{
    iValue = aValue.AllocL();
}

CMMFMetaDataEntry::CMMFMetaDataEntry()
{
}

void CMMFMetaDataEntry::ConstructL(const TDesC& aName, const TDesC& aValue)
{
    iName = aName.AllocL();
    iValue = aValue.AllocL();
}
#endif /*MMF_AVAILABLE*/