/*
 *  Copyright (c) 2003 Claus Bjerre
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

//
// Find the target recognizer OggRec.mdl file in Q:\Epoc32\Release\thumb\UREL
// 
// ** IMPORTANT **
// When testing/debugging this application then install the .mdl on a MMC card in 
// \System\Recog. If it chrashes it will prevent the mobile from booting and
// then you don't want it on the C: drive !!!
//

#ifndef _RECOGG_H
#define _RECOGG_H

/// Recognizer for '.ogg' files.
class CApaOggRecognizer : public CApaDataRecognizerType
	{
public:
	CApaOggRecognizer();
	TUint PreferredBufSize();
	TDataType SupportedDataTypeL(TInt aIndex) const;
	void DoRecognizeL(const TDesC& aName, const TDesC8& aBuffer);
    CApaOggRecognizer* CreateRecognizer();
	};

#endif