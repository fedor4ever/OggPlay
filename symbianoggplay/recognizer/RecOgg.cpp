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


#include <apmrec.h>

#include "RecOgg.h"
#include <OggPlayUid.h>

_LIT8(KOggMimeTypeLiteral,KOggMimeType);

// CONSTANTS
//
const TUid KOggPlayRecognizerUid = {KOggPlayRecognizerUidValue};



CApaOggRecognizer::CApaOggRecognizer() : CApaDataRecognizerType( 
                                         KOggPlayRecognizerUid,
                                         CApaDataRecognizerType::ENotRecognized )
  {
  iCountDataTypes = 1;
  }


TUint CApaOggRecognizer::PreferredBufSize()
  {
  // Actually the ogg files seems to have a header 'Oggs' 
  // that probably should be digested in DoRecognizeL() as well.
  return 0;
  }


TDataType CApaOggRecognizer::SupportedDataTypeL( TInt /*aIndex*/ ) const
  {
  return TDataType(KOggMimeTypeLiteral);
  }


void CApaOggRecognizer::DoRecognizeL( const TDesC& aFileName, const TDesC8& /*aBuffer*/ )
  {
  // Can't imagine a .ogg filename smaller than 'a.ogg'
  if( aFileName.Length() < 5 )
    return;

  // Assuming here that if its a .ogg then its a .ogg 
  if( aFileName.Right(4).CompareF(_L(".ogg")) == 0 )
	{
	// Success
	iDataType = TDataType(KOggMimeTypeLiteral);
	iConfidence = EProbable;
	return;
	}

  // Assuming here that if its a .oga then its a .oga 
  if( aFileName.Right(4).CompareF(_L(".oga")) == 0 )
	{
	// Success
	iDataType = TDataType(KOggMimeTypeLiteral);
	iConfidence = EProbable;
 	return;
	}

  // Assuming here that if its a .flac then its a .flac 
  if( aFileName.Right(5).CompareF(_L(".flac")) == 0 )
	{
	// Success
	iDataType = TDataType(KOggMimeTypeLiteral);
	iConfidence = EProbable;
 	return;
	}
  }



EXPORT_C CApaDataRecognizerType* CreateRecognizer()
  {
  CApaDataRecognizerType* self = new CApaOggRecognizer();
  return self;
  }


GLDEF_C TInt E32Dll( TDllReason /*aReason*/ )
  {
  return KErrNone;
  }

