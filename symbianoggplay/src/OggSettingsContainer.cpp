/*
 *  Copyright (c) 2003 P. Wolff
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

#include "OggSettingsContainer.h"
#include <aknlists.h>
#include <aknutils.h>

void COggSettingsContainer::ConstructL(const TRect& aRect, TUid aId)
{
    CreateWindowL();
    
    // A switch case would be better looking here, but can't do with UID.
    
    if (aId ==  KOggPlayUidSettingsView)
    {
        COggS60Utility::DisplayStatusPane(R_OGG_SETTINGS);
        iListBox = new (ELeave) COggplayDisplaySettingItemList((COggPlayAppUi &)*CEikonEnv::Static()->AppUi());
        iListBox->ConstructFromResourceL(R_OGGPLAY_DISPLAY_SETTING_ITEM_LIST);
    }
    else if (aId ==  KOggPlayUidCodecSelectionView)
    {
        COggS60Utility::DisplayStatusPane(R_OGG_CODEC_SELECTION_SETTINGS_TITLE);
        iListBox = new (ELeave) COggplayCodecSelectionSettingItemList((COggPlayAppUi &)*CEikonEnv::Static()->AppUi());
        iListBox->ConstructFromResourceL(R_OGGPLAY_CODEC_SELECTION_SETTING_ITEM_LIST);
        
    } else
    {
        User::Leave(KErrNotSupported);
    }
    
    SetRect(aRect);
    ActivateL();
    iListBox->SetMopParent(this);
    iListBox->LoadSettingsL();
    
    iListBox->SetRect(aRect);
    iListBox->ActivateL();
    
}

COggSettingsContainer::~COggSettingsContainer()
    {
    if (iListBox)
	    {
      TRAPD(ignore, iListBox->StoreSettingsL());
      ((COggPlayAppUi*)CEikonEnv::Static()->AppUi())->WriteIniFileOnNextPause();
      delete iListBox;
	    }
    }

void COggSettingsContainer::SizeChanged()
    {
    if (iListBox) iListBox->SetRect(Rect());
    }

TInt COggSettingsContainer::CountComponentControls() const
    {
    if (iListBox)
      return 1;
    else
      return 0;
    }

CCoeControl* COggSettingsContainer::ComponentControl(TInt aIndex) const
    {
    switch ( aIndex )
        {
        case 0:
            return iListBox;
        default:
            return NULL;
        }
    }

void COggSettingsContainer::Draw(const TRect& aRect) const
    {
    CWindowGc& gc = SystemGc();
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushColor(KRgbGray);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.DrawRect(aRect);
    }

void COggSettingsContainer::HandleControlEventL(
    CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
    {
    }

TKeyResponse COggSettingsContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
  TKeyResponse response=EKeyWasNotConsumed;
  if (iListBox)
  {
    if ((aKeyEvent.iCode == EKeyUpArrow) || (aKeyEvent.iCode == EKeyDownArrow) ||
      (aKeyEvent.iCode == EKeyDevice3))
    {
      response = iListBox->OfferKeyEventL(aKeyEvent, aType);
    }
  }
  return response;
}



// ---------------------------------------------------------

#if defined(SERIES60)
COggplayDisplaySettingItemList::COggplayDisplaySettingItemList(COggPlayAppUi& aAppUi)
: iData(aAppUi.iSettings), iAppUi(aAppUi)
{
}


CAknSettingItem* COggplayDisplaySettingItemList::CreateSettingItemL(TInt aIdentifier)
{
  switch (aIdentifier)
		{
  case EOggSettingScanDir:
    return new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, 
      iData.iScanmode);
  case EOggSettingManeuvringSpeed:
    return new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, 
      iData.iManeuvringSpeed);
  case EOggSettingAutoPlayId:
    return new (ELeave) CAknBinaryPopupSettingItem(aIdentifier,
      iData.iAutoplay);
  case EOggSettingWarningsId:
    return new (ELeave) CAknBinaryPopupSettingItem(aIdentifier,
      iData.iWarningsEnabled);
  case EOggSettingRskIdle:
    return new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, 
      iData.iRskIdle);
  case EOggSettingRskPlay:
    return new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, 
      iData.iRskPlay);
  case EOggSettingVolumeBoost:
     return new (ELeave) CGainSettingItem(aIdentifier,   iAppUi);
  default:
    break;
		}
  return NULL;
}



/////////////////////////////////////
// 
// Overload of the Gain setting, so that change will take place right away
// 
/////////////////////////////////////
 
CGainSettingItem:: CGainSettingItem( TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknEnumeratedTextPopupSettingItem ( aIdentifier , aAppUi.iSettings.iGainType ), iAppUi(aAppUi)

    {
    }

 
void  CGainSettingItem:: EditItemL ( TBool aCalledFromMenu )
     {
     CAknEnumeratedTextPopupSettingItem::EditItemL( aCalledFromMenu );
     iAppUi.iOggPlayback->SetVolumeGain( (TGainType)InternalValue () ) ;
     }


///////////////////////////////////////////////////////////////////////////////////////////////
// 
// Plugin Selection Dialogs
// 
///////////////////////////////////////////////////////////////////////////////////////////////

COggplayCodecSelectionSettingItemList::COggplayCodecSelectionSettingItemList(COggPlayAppUi& aAppUi)
: iData(aAppUi.iSettings), iAppUi(aAppUi)
{
}

CAknSettingItem* COggplayCodecSelectionSettingItemList::CreateSettingItemL(TInt aIdentifier)
{
    CCodecSelectionSettingItem *item;
    switch (aIdentifier)
    {
    case  EOggCodecSelectionSettingOGG:
        item = new (ELeave) CCodecSelectionSettingItem(aIdentifier, 
            iSelectedIndexes[0], iAppUi.iOggPlayback, _L("ogg") );
        iCodecSelectionSettingItemArray[0] = item;
        break;
    case  EOggCodecSelectionSettingMP3:
        item = new (ELeave) CCodecSelectionSettingItem(aIdentifier, 
            iSelectedIndexes[1], iAppUi.iOggPlayback, _L("mp3") );
        iCodecSelectionSettingItemArray[1] = item;
        break;
    case  EOggCodecSelectionSettingAAC:
        item = new (ELeave) CCodecSelectionSettingItem(aIdentifier, 
            iSelectedIndexes[2], iAppUi.iOggPlayback, _L("aac") );
        iCodecSelectionSettingItemArray[2] = item;
        break;
    default:
        item = NULL;
        break;
    }
    return item;
}

void COggplayCodecSelectionSettingItemList::EditItemL  ( TInt aIndex, TBool aCalledFromMenu )
{
    CAknSettingItemList::EditItemL(aIndex, aCalledFromMenu);

    // When coming back from the edit, convert the value from index to UID
    iData.iControllerUid[aIndex] = iCodecSelectionSettingItemArray[aIndex]->GetSelectedUid();   
}

CCodecSelectionSettingItem::CCodecSelectionSettingItem(TInt aResourceId, TInt &aValue, 
                                                       CAbsPlayback * aPlayback,
                                                       const TDesC & anExtension):
CAknEnumeratedTextPopupSettingItem(aResourceId,aValue),
iPlayback(*aPlayback)
{
    iPluginList = NULL;
    TRAPD(foo, iPluginList = &iPlayback.GetPluginListL(anExtension)); // Set iPluginList
}

void CCodecSelectionSettingItem::CompleteConstructionL()
{
    if (iPluginList)
    {
        /* Build the list "on the fly" */ 
        CArrayPtr<CAknEnumeratedText>* enumTextArray = new (ELeave) CArrayPtrFlat<CAknEnumeratedText> (4) ;
        CleanupStack::PushL(enumTextArray);
        
        CArrayPtr<HBufC>* enumPopUp = new (ELeave)  CArrayPtrFlat<HBufC> (4);
        CleanupStack::PushL(enumPopUp);

        CArrayPtrFlat <CPluginInfo> & list = iPluginList->GetPluginInfoList();
        // Selected item first
        for (TInt i=0;i<list.Count(); i++)
        {
            HBufC16 * name = list[i]->iName->AllocLC();
            CAknEnumeratedText *enumText = new (ELeave) CAknEnumeratedText(i,name);
            enumTextArray->AppendL(enumText);
            CleanupStack::Pop(name);

            // Allocate the name, again :-(
            name = list[0]->iName->AllocLC();
            enumPopUp->AppendL(name);
            CleanupStack::Pop(name);
            if ( list[i]->iControllerUid == iPluginList->GetSelectedPluginInfo().iControllerUid )
            {
                SetExternalValue(i);
                SetInternalValue(i);
            }
        }
        SetEnumeratedTextArrays(enumTextArray,enumPopUp);
        CleanupStack::Pop(2); // enumTextArray,enumPopUp
    }
    else
    {
        // No plugin found.
        CAknEnumeratedTextPopupSettingItem::CompleteConstructionL();
    }
}

void  CCodecSelectionSettingItem::EditItemL ( TBool aCalledFromMenu )
{
    if (iPluginList)
    {
        CAknEnumeratedTextPopupSettingItem::EditItemL( aCalledFromMenu );
        
        CCodecInfoList * info = new (ELeave) CCodecInfoList(*iPluginList->GetPluginInfoList().At(InternalValue() ));
        COggPlayAppUi * appUi=(COggPlayAppUi *)CEikonEnv::Static()->AppUi();
        info->ConstructL(appUi->ClientRect());
        delete info;

    }
}

TInt32 CCodecSelectionSettingItem::GetSelectedUid()
{

    if (iPluginList)
    {
        CArrayPtrFlat <CPluginInfo> & list = iPluginList->GetPluginInfoList();
        TUid selectedUid = list[InternalValue()]->iControllerUid;
        iPluginList->SelectPlugin(selectedUid);
        return(selectedUid.iUid);
    }
    else
        return NULL;
}



/////////////////////////////////////
// 
// Display the plugin information
// 
/////////////////////////////////////

CCodecInfoList::CCodecInfoList( CPluginInfo& aPluginInfo ) 
: iPluginInfo(aPluginInfo)
  {
  }

void CCodecInfoList::ConstructL(const TRect& aRect)
  {
  CreateWindowL();
  SetRect(aRect);

  // Create a CBA
  iCba = CEikButtonGroupContainer::NewL(CEikButtonGroupContainer::ECba,
		CEikButtonGroupContainer::EHorizontal, this, R_AVKON_SOFTKEYS_OK_EMPTY, *this);
  iCba->SetBoundingRect(aRect.Size());

  iListBox = new (ELeave) CAknDoubleStyleListBox;
  iListBox->SetMopParent(this);
  iListBox->SetContainerWindowL( *this );
  iListBox->ConstructL( this, EAknListBoxSelectionList );
  iListBox->Model()->SetOwnershipType(ELbmOwnsItemArray);

  // Scroll bar arrow indicators
  iListBox->CreateScrollBarFrameL(ETrue);
	iListBox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);

  RefreshListboxModel();
 
  iListBox->SetRect(aRect.Size());
  
  iEikonEnv->EikAppUi()->AddToStackL(this,ECoeStackPriorityDialog);

  ActivateL();
  // Dialog like behaviour.
  if (!iWait.IsStarted())
  {
     iWait.Start();
  }
  
  // 'Dialog' returned
  iEikonEnv->EikAppUi()->RemoveFromStack(this);

  delete iListBox;
  iListBox = NULL;
  delete iCba;
  iCba = NULL;
  }

void CCodecInfoList::ProcessCommandL (TInt aCommandId)
{
    if (aCommandId == EAknSoftkeyOk )
    {
        if (iWait.IsStarted())
            iWait.AsyncStop();
    }

}

CCodecInfoList::~CCodecInfoList()
  {
    if (iWait.IsStarted())
        iWait.AsyncStop();
  delete iListBox;
  delete iCba;
  }

void CCodecInfoList::RefreshListboxModel()
{
    TBuf<128> keyBuf,listboxBuf;
    
    CDesCArrayFlat* array = iCoeEnv->ReadDesCArrayResourceL(R_CODEC_INFO_ITEMS);
    CDesCArray* modelArray = static_cast<CDesCArray*>(iListBox->Model()->ItemTextArray());
    
    for( TInt i=0; i<4; i++ ) 
    {
        keyBuf.Zero();
        
        TBuf<64> title;
        title.Copy(array->MdcaPoint(i));
        switch (i)
        {
        case 0:
            // Name
            listboxBuf.Format(_L("\t%S\t%S"), &title, iPluginInfo.iName);
            break;
        case 1:
            // Supplier
            listboxBuf.Format(_L("\t%S\t%S"), &title, iPluginInfo.iSupplier);
            break;
        case 2:
            // Version
            listboxBuf.Format(_L("\t%S\t%i"), &title, iPluginInfo.iVersion);
            break;
        case 3:
            // Version
            TBuf <64> aaa;
            aaa = iPluginInfo.iControllerUid.Name();
            listboxBuf.Format(_L("\t%S\t%S"), &title, &aaa);
            break;
            
        }
        
        modelArray->AppendL(listboxBuf);
    }
    iListBox->HandleItemAdditionL();
    DrawDeferred();
    delete array;
}

TInt CCodecInfoList::CountComponentControls() const
  {
  return 1;
  }


CCoeControl* CCodecInfoList::ComponentControl(TInt /*aIndex*/) const
  {
  return iListBox;
  }


TKeyResponse CCodecInfoList::OfferKeyEventL(
    const TKeyEvent& aKeyEvent,
    TEventCode aType )
  {
  return iListBox->OfferKeyEventL( aKeyEvent, aType );
  }



#endif /* SERIES60 */
