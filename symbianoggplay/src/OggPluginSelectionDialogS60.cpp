/*
 *  Copyright (c) 2005 OggPlay developpers
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

#include "OggPluginSelectionDialogS60.h"
#include <aknlists.h>
#include <aknutils.h>
#include <aknPopup.h> 


#ifdef SERIES60 
#ifdef MMF_AVAILABLE 
///////////////////////////////////////////////////////////////////////////////////////////////
// 
// Plugin Selection Dialogs
// 
///////////////////////////////////////////////////////////////////////////////////////////////


COggplayCodecSelectionSettingItemList ::COggplayCodecSelectionSettingItemList ( ) 
  {
  }

void COggplayCodecSelectionSettingItemList ::ConstructL(const TRect& aRect)
  {
  CreateWindowL();
 
  iListBox = new (ELeave) CAknSettingStyleListBox;
  iListBox->SetMopParent(this);
  iListBox->SetContainerWindowL( *this );
  iListBox->ConstructL( this, EAknListBoxSelectionList );
  iListBox->Model()->SetOwnershipType(ELbmOwnsItemArray);

  // Scroll bar arrow indicators
  iListBox->CreateScrollBarFrameL(ETrue);
  iListBox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);

  RefreshListboxModel();
 
  iListBox->ActivateL();
  SetRect(aRect);
  ActivateL();
  iListBox->SetRect(Rect());
  
  iWaitTimer = new (ELeave) COggTimer( TCallBack( TimerExpired ,this ) );
              
  }


COggplayCodecSelectionSettingItemList::~COggplayCodecSelectionSettingItemList ()
  {
  delete iListBox;
  delete iExtensionList;
  delete iWaitTimer;
  }


void COggplayCodecSelectionSettingItemList::RefreshListboxModel()
  {
  TBuf<64> listboxBuf;
	   

  // Build the menu on the fly, according to the list plugins.
  CDesCArray* modelArray = static_cast<CDesCArray*>(iListBox->Model()->ItemTextArray());
  modelArray->Reset();
  COggPlayAppUi * appUi = static_cast <COggPlayAppUi*> (CEikonEnv::Static()->AppUi());
  iPluginSupportedList = &appUi->iOggPlayback->GetPluginListL();
  if (!iExtensionList)
     iExtensionList =  iPluginSupportedList->SupportedExtensions();
  
  for (TInt i=0; i<iExtensionList->Count(); i++)
    {
    TBuf <10> extension = iExtensionList->MdcaPoint(i);
    TBuf <50> tmp;
    CPluginInfo * info = appUi->iOggPlayback->GetPluginListL().GetSelectedPluginInfo(extension);
	if ( info == NULL )
	{
	   // Codec has been disabled
   		iEikonEnv->ReadResource(tmp,R_OGG_DISABLED);
	}
	else tmp = *info->iName;
	
    listboxBuf.Format(_L("\t%S\t\t%S"), &extension, &tmp);
	modelArray->InsertL(i,listboxBuf);
    }
    
  iListBox->HandleItemAdditionL();
  DrawDeferred();
  }


TInt COggplayCodecSelectionSettingItemList ::CountComponentControls() const
  {
  return 1;
  }


CCoeControl* COggplayCodecSelectionSettingItemList::ComponentControl(TInt /*aIndex*/) const
  {
  return iListBox;
  }


void COggplayCodecSelectionSettingItemList::ProcessCommandL (TInt /*aCommandId*/)
{
    // User has pressed the "Modify" CBA or pressed the select button.
    // Display the list of available codecs for the selected extension
    
	// Create CEikTextListBox instance, list
    CEikTextListBox* list = new( ELeave ) CAknSinglePopupMenuStyleListBox;
    // Push list'pointer to CleanupStack.
    CleanupStack::PushL( list );
    // Create CAknPopupList instance, popupList
    CAknPopupList* popupList = CAknPopupList::NewL( list, 
                                                    R_AVKON_SOFTKEYS_SELECT_CANCEL,
                                                    AknPopupLayouts::EMenuWindow );
    // Push popupList'pointer to CleanupStack.
    CleanupStack::PushL( popupList );
    // Initialize listbox.
    list->ConstructL( popupList, CEikListBox::ELeftDownInViewRect );
    list->CreateScrollBarFrameL( ETrue );
    list->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff,
                                                     CEikScrollBarFrame::EAuto );
    
    // The extension currently selected in the listbox.
    TBuf <10> selectedExtension = iExtensionList->MdcaPoint(iListBox->CurrentItemIndex());
    
    // Create the list of codecs to select
    CDesCArrayFlat* items = new (ELeave) CDesCArrayFlat(3);
    CArrayPtrFlat <CPluginInfo> * pluginList = iPluginSupportedList->GetPluginInfoList(selectedExtension);
	
    for(TInt i=0; i<pluginList->Count(); i++)
    {
        items->AppendL(*pluginList->At(i)->iName);
    }

    TBuf <50> tmp;
   	iEikonEnv->ReadResource(tmp,R_OGG_USE_NO_CODEC);
	items->AppendL(tmp);
    
    // Push items'pointer to CleanupStack.  
    CleanupStack::PushL( items );
    // Set listitems.
    CTextListBoxModel* model = list->Model();
    model->SetItemTextArray( items );
    model->SetOwnershipType( ELbmOwnsItemArray );
    CleanupStack::Pop(items);
    // Set title .
    // popupList->SetTitleL( _L("Choose") );
    // Show popup list and then show return value.
    TInt popupOk = popupList->ExecuteLD();
    // Pop the popupList's pointer from CleanupStack
    CleanupStack::Pop();

    if ( popupOk )
        {
          // User has selected a new codec.
          TInt index = list->CurrentItemIndex();
	      CPluginInfo * pluginInfo = NULL;
          CArrayPtrFlat <CPluginInfo> *infoList = iPluginSupportedList->GetPluginInfoList(selectedExtension);
          if ( infoList )
             if (index <= infoList->Count()-1 )
               pluginInfo = infoList->At(index);
         
	      if (pluginInfo)
	    	iPluginSupportedList->SelectPluginL(selectedExtension, pluginInfo->iControllerUid);
	      else
	 	    iPluginSupportedList->SelectPluginL(selectedExtension, TUid::Null());
	 		
	 	  RefreshListboxModel();
        }
        
    // Pop and Destroy the list's pointer from CleanupStack
    CleanupStack::PopAndDestroy();
}
  
TInt COggplayCodecSelectionSettingItemList::TimerExpired(TAny* aPtr)
{
    COggplayCodecSelectionSettingItemList* self= ( COggplayCodecSelectionSettingItemList*) aPtr;
    self->ProcessCommandL(0); // Edit the plugin values	
    return 0;
}

TKeyResponse COggplayCodecSelectionSettingItemList::OfferKeyEventL(
    const TKeyEvent& aKeyEvent,
    TEventCode aType )
  {
    if ((aKeyEvent.iScanCode == EStdKeyDevice3) && (aType == EEventKeyDown))
    {
       //Edit the item, but first inform that key has been consumed

        iWaitTimer->Wait(0);
    	return EKeyWasConsumed;
    }
    return iListBox->OfferKeyEventL( aKeyEvent, aType );
  }



#if 0

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
  
#endif

#endif /* MMF_AVAILABLE */

#endif /* SERIES60 */
