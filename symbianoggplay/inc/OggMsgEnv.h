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

#ifndef OGGMSGENV_H
#define OGGMSGENV_H

#include <e32def.h>
#include <e32base.h>

class COggMsgEnv : public CBase
{
public:
    COggMsgEnv(TBool& aWarningsEnabled);
    ~COggMsgEnv();

	TBool WarningsEnabled() const;
    void OggWarningMsgL(const TDesC& aWarning) const;
    static void OggErrorMsgL(const TDesC& aFirstLine, const TDesC& aSecondLine);
    static void OggErrorMsgL(TInt aFirstLineId, TInt aSecondLineId);

private:
    const TBool &iWarningsEnabled;
};

#endif
