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

 
#include "OggHelperFcts.h"

COggTimer::COggTimer(const TCallBack& aCallBack)
: CTimer(EPriorityStandard), iCallBack(aCallBack)
    {
    CTimer::ConstructL();
    CActiveScheduler::Add(this);
    }

COggTimer::COggTimer(const TCallBack& aCallBack, const TCallBack& aErrorCallBack)
: CTimer(EPriorityStandard), iCallBack(aCallBack)
    {
	iErrorCallBack = new(ELeave) TCallBack(aErrorCallBack);

	CTimer::ConstructL();
    CActiveScheduler::Add(this);
    }

COggTimer::~COggTimer()
	{
	delete iErrorCallBack;
	}

void COggTimer::RunL()
    {
	TInt err = iStatus.Int();
	if (err == KErrNone)
		iCallBack.CallBack();
	else if (iErrorCallBack)
		iErrorCallBack->CallBack();
    }

void COggTimer::Wait(TTimeIntervalMicroSeconds32 aInterval)
    {
    Cancel(); // Cancel any pending timer operation
    After(aInterval);
    }

void COggTimer::At(const TTime& aTime)
    {
    Cancel(); // Cancel any pending timer operation
	CTimer::At(aTime);
    }

void COggTimer::Cancel()
    {
    CTimer::Cancel(); // Cancel any pending timer operation
    }




