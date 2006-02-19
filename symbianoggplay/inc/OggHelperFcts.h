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

#include <e32base.h>

#ifndef OGGHELPERFCTS_H
#define OGGHELPERFCTS_H
class COggTimer : public CTimer
    {
    public:
        COggTimer(const TCallBack& aCallBack);
        COggTimer(const TCallBack& aCallBack, const TCallBack& aErrorCallBack);

		void Wait(TTimeIntervalMicroSeconds32 interval);
		void At(const TTime& aTime);
        void Cancel();
    private:
        void RunL();
        TCallBack iCallBack;
		TCallBack* iErrorCallBack;
    };

#endif