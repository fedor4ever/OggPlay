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


#include <aknlists.h> 
#include <barsread.h>  // Resource reading
#include <aknapp.h>
#include <s32file.h>

#include "OggUserHotkeys.h"
#include <OggPlay.rsg>
#include "OggPlay.hrh"
#include "OggLog.h"


////////////////////////////////////////////////////////////////
//
// class COggUserHotkeysData
//
////////////////////////////////////////////////////////////////

COggUserHotkeysData* COggUserHotkeysData::NewL()
  {
  COggUserHotkeysData *self = new (ELeave) COggUserHotkeysData;

  for( TInt i=0; i<COggUserHotkeysData::ENofHotkeys; i++ ) 
    self->iHotkeys[i] = ENoHotkey;

  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();
  self->modelFileName = BaflUtils::DriveAndPathFromFullName(appUi->Application()->AppFullName());
  self->modelFileName.Append(_L("HotKeys.ini"));
  TRACE(COggLog::VA(_L("HKey=%S"), &self->modelFileName )); // FIXIT
#if defined(__WINS__)
  self->modelFileName = _L("C:\\HotKeys.ini");
  TRACE(COggLog::VA(_L("HKey=%S"), &self->modelFileName ));
#endif

  self->InternalizeModelL();
  return self;
  }


COggUserHotkeysData::~COggUserHotkeysData()
  {
  TRAPD(newerMind, ExternalizeModelL() ); // Auch..
  }


void COggUserHotkeysData::SetHotkey( TInt aRow, TInt aCode )
  {
  for( TInt i=0; i<ENofHotkeys; i++ )
    {
    if( i == aRow )
      continue;
    // Prevent replicas
    if( iHotkeys[i] == aCode )
      iHotkeys[i] = ENoHotkey;
    }
  iHotkeys[aRow] = aCode;
  }


COggUserHotkeysData::THotkeys COggUserHotkeysData::Hotkey( TInt aScanCode ) const
  {
  for( TInt i=0; i<ENofHotkeys; i++ )
    {
    if( iHotkeys[i] == aScanCode )
      {
      switch( i ) {
        case 0 :  return EFastForward;
        case 1 :  return ERewind;
        default : return ENotAssigned;
        }
      }
    }
  return ENotAssigned;
  }


void COggUserHotkeysData::InternalizeModelL()
  {
	RFile in;
  if( in.Open(CCoeEnv::Static()->FsSession(), modelFileName, EFileRead) != KErrNone)
		return;

	TFileText tf;
	tf.Set(in);
  TBuf<20> line;

  for( TInt i=0; i<COggUserHotkeysData::ENofHotkeys; i++ ) 
    {
	  if(tf.Read(line) == KErrNone) {
		  TLex parse(line);
		  parse.Val(iHotkeys[i]);
      }
    }
  in.Close();
  }


void COggUserHotkeysData::ExternalizeModelL() const
  {
	RFile out;
  TInt err = out.Replace(CCoeEnv::Static()->FsSession(), modelFileName, EFileWrite);
  if( err != KErrNone )
		return;

	TFileText tf;
	tf.Set(out);
  TBuf<20> line;

  for( TInt i=0; i<COggUserHotkeysData::ENofHotkeys; i++ ) 
    {
  	line.Num(iHotkeys[i]);
	  tf.Write(line);
    }
  out.Close();
  }


////////////////////////////////////////////////////////////////
//
// class COggUserHotkeys
//
////////////////////////////////////////////////////////////////

COggUserHotkeys::COggUserHotkeys( COggUserHotkeysData& aData ) : iData(aData)
  {
  }

void COggUserHotkeys::ConstructL(const TRect& aRect)
  {
  TRACEL("COggUserHotkeys::ConstructL. Start");
  CreateWindowL();

  iListBox = new (ELeave) CAknSettingStyleListBox;
  iListBox->SetMopParent(this);
  iListBox->SetContainerWindowL( *this );

	TResourceReader reader;
	iCoeEnv->CreateResourceReaderLC(reader,R_USER_HOTKEY_SETTING_LIST);
	iListBox->ConstructFromResourceL(reader);
	CleanupStack::PopAndDestroy();

  iListBox->Model()->SetOwnershipType(ELbmOwnsItemArray);
	iListBox->CreateScrollBarFrameL(ETrue);
	iListBox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);

  RefreshListboxModel();
 
  iListBox->ActivateL();
  SetRect(aRect);
	ActivateL();
  iListBox->SetRect(Rect());
  }


COggUserHotkeys::~COggUserHotkeys()
  {
  delete iListBox;
  }


void COggUserHotkeys::RefreshListboxModel()
  {
	TBuf<64> keyBuf,listboxBuf;

  CDesCArrayFlat* array = iCoeEnv->ReadDesCArrayResourceL(R_USER_HOTKEY_ITEMS);
  
  CDesCArray* modelArray = static_cast<CDesCArray*>(iListBox->Model()->ItemTextArray());

  for( TInt i=0; i<COggUserHotkeysData::ENofHotkeys; i++ ) 
    {
    keyBuf.Zero();
    if( iData.iHotkeys[i] == ENoHotkey )
      keyBuf.Copy(array->MdcaPoint(COggUserHotkeysData::ENotAssigned));
    else
      keyBuf.Append(iData.iHotkeys[i]);

    TBuf<64> title;
    title.Copy(array->MdcaPoint(i));
    listboxBuf.Format(_L("\t%S\t\t%S"), &title, &keyBuf);
	  modelArray->InsertL(i,listboxBuf);
    TRACE(COggLog::VA(_L("LBox=%S"), &listboxBuf ));  // FIXIT
    if( modelArray->MdcaCount()-1 > i )
      modelArray->Delete(i+1);
    }
	iListBox->HandleItemAdditionL();
  DrawDeferred();
  delete array;
  }


TInt COggUserHotkeys::CountComponentControls() const
  {
  return 1;
  }


CCoeControl* COggUserHotkeys::ComponentControl(TInt /*aIndex*/) const
  {
  return iListBox;
  }


TKeyResponse COggUserHotkeys::OfferKeyEventL(
    const TKeyEvent& aKeyEvent,
    TEventCode aType )
  {
  //TRACEF(COggLog::VA(_L("Key=%i"), aKeyEvent.iScanCode ));  // FIXIT
  TInt validKey = ENoHotkey;
  if( Rng((TInt)'0', aKeyEvent.iScanCode, (TInt)'9') )
    validKey = aKeyEvent.iScanCode;
  else if( aKeyEvent.iScanCode == EStdKeyNkpAsterisk || aKeyEvent.iScanCode == 42 /* Idrk */ )
    validKey = '*';
  else if( aKeyEvent.iScanCode == EStdKeyHash )
    validKey = '#';

  if( validKey != ENoHotkey )
    {
    TInt row = iListBox->CurrentItemIndex();
    iData.SetHotkey(row, validKey);
    RefreshListboxModel();
    return EKeyWasConsumed;
    }
  else
		return iListBox->OfferKeyEventL( aKeyEvent, aType );
  }

// End of File  
