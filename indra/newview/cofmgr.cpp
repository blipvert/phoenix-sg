/** 
 *
 * Copyright (c) 2010, Kitty Barnett
 * 
 * The source code in this file is provided to you under the terms of the 
 * GNU General Public License, version 2.0, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. Terms of the GPL can be found in doc/GPL-license.txt 
 * in this distribution, or online at http://www.gnu.org/licenses/gpl-2.0.txt
 * 
 * By copying, modifying or distributing this software, you acknowledge that
 * you have read and understood your obligations described above, and agree to 
 * abide by those obligations.
 * 
 */

#include "llviewerprecompiledheaders.h"
#include "cofmgr.h"
#include "llagent.h"
#include "llcommonutils.h"
#include "llerror.h"
#include "llvoavatar.h"

// ============================================================================
// Inventory helper classes
//

class LLCOFLinkTargetFetcher : public LLInventoryFetchObserver
{
public:
	LLCOFLinkTargetFetcher() {}
	/*virtual*/ ~LLCOFLinkTargetFetcher() {}

	/*virtual*/ void done()
	{
		LLCOFMgr::instance().setLinkAttachments(true);
		LLCOFMgr::instance().updateAttachments();
		LLCOFMgr::instance().synchWearables();

		gInventory.removeObserver(this);
		delete this;
	}
};

class LLCOFFetcher : public LLInventoryFetchDescendentsObserver
{
public:
	LLCOFFetcher() {}
	/*virtual*/ ~LLCOFFetcher() {}

	/*virtual*/ void done()
	{
		// Now that we have the COF contents we need to fetch the link targets as well
		LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
		gInventory.collectDescendents(LLCOFMgr::getCOF(), folders, items, LLInventoryModel::EXCLUDE_TRASH);

		LLInventoryFetchObserver::item_ref_t idItems; 
		for (S32 idxItem = 0; idxItem < items.count(); idxItem++)
		{
			const LLViewerInventoryItem* pItem = items.get(idxItem);
			if (!pItem)
				continue;
			idItems.push_back(pItem->getLinkedUUID());
		}

		if (!idItems.empty())
		{
			LLCOFLinkTargetFetcher* pFetcher = new LLCOFLinkTargetFetcher();
			pFetcher->fetchItems(idItems);
			if (pFetcher->isEverythingComplete())
				pFetcher->done();
			else
				gInventory.addObserver(pFetcher);
		}

		gInventory.removeObserver(this);
		delete this;
	}
};

class LLDeferredAddLinkTargetFetcher : public LLInventoryFetchObserver
{
public:
	LLDeferredAddLinkTargetFetcher(LLPointer<LLInventoryCallback> cb) : m_Callback(cb) {}
	/*virtual*/ ~LLDeferredAddLinkTargetFetcher() {}

	/*virtual*/ void done()
	{
		for (item_ref_t::const_iterator itItemId = mComplete.begin(); itItemId != mComplete.end(); ++itItemId)
		{
			const LLViewerInventoryItem* pItem = gInventory.getItem(*itItemId);
			if (!pItem)
				continue;
			LLCOFMgr::instance().addCOFItemLink(pItem, m_Callback);
		}

		gInventory.removeObserver(this);
		delete this;
	}
protected:
	LLPointer<LLInventoryCallback> m_Callback;
};

class LLLinkAttachmentCallback : public LLInventoryCallback
{
public:
	LLLinkAttachmentCallback() {}
	/*virtual*/ ~LLLinkAttachmentCallback() {}
	/*virtual*/ void fire(const LLUUID& idItem) { LLCOFMgr::instance().onLinkAttachmentComplete(idItem); }
};

class LLLinkWearableCallback : public LLInventoryCallback
{
public:
	LLLinkWearableCallback() {}
	/*virtual*/ ~LLLinkWearableCallback() {}
	/*virtual*/ void fire(const LLUUID& idItem) { LLCOFMgr::instance().onLinkWearableComplete(idItem); }
};

// ============================================================================
// Helper functions
//

void LLCOFMgr::fetchCOF()
{
	static bool sFetched = false;
	if (!sFetched)
	{
		const LLUUID idCOF = getCOF();
		if (idCOF.isNull())
		{
			LL_ERRS("COFMgr") << "Unable to find (or create) COF" << LL_ENDL;
			return;
		}

		LLInventoryFetchDescendentsObserver::folder_ref_t fetchFolders;
		fetchFolders.push_back(idCOF);

		LLCOFFetcher* pFetcher = new LLCOFFetcher();
		pFetcher->fetchDescendents(fetchFolders);
		if (pFetcher->isEverythingComplete())
			pFetcher->done();
		else
			gInventory.addObserver(pFetcher);

		sFetched = true;
	}
}

void LLCOFMgr::getDescendentsOfAssetType(const LLUUID& idCat, LLInventoryModel::item_array_t& items, 
										 LLAssetType::EType typeAsset, bool fFollowFolderLinks)
{
	LLInventoryModel::cat_array_t folders;
	LLIsType f(typeAsset);
	gInventory.collectDescendentsIf(idCat, folders, items, LLInventoryModel::EXCLUDE_TRASH, f, fFollowFolderLinks);
}

void LLCOFMgr::addCOFItemLink(const LLUUID& idItem, LLPointer<LLInventoryCallback> cb)
{
	const LLViewerInventoryItem *pItem = gInventory.getItem(idItem);
	if (!pItem)
	{
		LLInventoryFetchObserver::item_ref_t idItems; 
		idItems.push_back(idItem);

		LLDeferredAddLinkTargetFetcher* pFetcher = new LLDeferredAddLinkTargetFetcher(cb);
		pFetcher->fetchItems(idItems);

		gInventory.addObserver(pFetcher);
		return;
	}
	addCOFItemLink(pItem, cb);
}

void LLCOFMgr::addCOFItemLink(const LLInventoryItem* pItem, LLPointer<LLInventoryCallback> cb)
{
	if ( (!pItem) || (isLinkInCOF(pItem->getLinkedUUID())) )
	{
		return;
	}

	gInventory.addChangedMask(LLInventoryObserver::LABEL, pItem->getLinkedUUID());

	const std::string strDescr = (pItem->getIsLinkType()) ? pItem->getDescription() : "";
	link_inventory_item(gAgent.getID(), pItem->getLinkedUUID(), getCOF(), pItem->getName(), strDescr, LLAssetType::AT_LINK, cb);
}

bool LLCOFMgr::isLinkInCOF(const LLUUID& idItem)
{
	 LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
	 LLLinkedItemIDMatches f(gInventory.getLinkedItemID(idItem));
	 gInventory.collectDescendentsIf(getCOF(), folders, items, LLInventoryModel::EXCLUDE_TRASH, f);
	 return (!items.empty());
}

void LLCOFMgr::removeCOFItemLinks(const LLUUID& idItem)
{
	gInventory.addChangedMask(LLInventoryObserver::LABEL, idItem);

	LLInventoryModel::cat_array_t folders; LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(getCOF(), folders, items, LLInventoryModel::EXCLUDE_TRASH);

	for (S32 idxItem = 0; idxItem < items.count(); idxItem++)
	{
		const LLInventoryItem* pItem = items.get(idxItem);
		if ( (pItem->getIsLinkType()) && (idItem == pItem->getLinkedUUID()) )
			gInventory.purgeObject(pItem->getUUID());
	}
}

// ============================================================================
// Attachment functions
//

void LLCOFMgr::addAttachment(const LLUUID& idItem)
{
	if ( (isLinkInCOF(idItem)) || (std::find(m_PendingAttachLinks.begin(), m_PendingAttachLinks.end(), idItem) != m_PendingAttachLinks.end()) )
	{
		return;
	}

	gInventory.addChangedMask(LLInventoryObserver::LABEL, idItem);
	m_PendingAttachLinks.push_back(idItem);

	if (m_fLinkAttachments)
	{
		LLPointer<LLInventoryCallback> cb = new LLLinkAttachmentCallback();
		addCOFItemLink(idItem, cb);
	}
}

void LLCOFMgr::removeAttachment(const LLUUID& idItem)
{
	gInventory.addChangedMask(LLInventoryObserver::LABEL, idItem);

	// Remove the attachment from the pending list
	uuid_vec_t::iterator itPending = std::find(m_PendingAttachLinks.begin(), m_PendingAttachLinks.end(), idItem);
	if (itPending != m_PendingAttachLinks.end())
		m_PendingAttachLinks.erase(itPending);
	
	if (m_fLinkAttachments)
	{
	   removeCOFItemLinks(idItem);
	}
}

void LLCOFMgr::setLinkAttachments(bool fEnable)
{
	m_fLinkAttachments = fEnable;

	if (m_fLinkAttachments)
		linkPendingAttachments();
}

void LLCOFMgr::linkPendingAttachments()
{
   LLPointer<LLInventoryCallback> cb = NULL;
   for (uuid_vec_t::const_iterator itPending = m_PendingAttachLinks.begin(); itPending != m_PendingAttachLinks.end(); ++itPending)
	{
		const LLUUID& idAttachItem = *itPending;
		if ( (gAgent.getAvatarObject()->isWearingAttachment(idAttachItem)) && (!isLinkInCOF(idAttachItem)) )
		{
			if (!cb)
				cb = new LLLinkAttachmentCallback();
			addCOFItemLink(idAttachItem, cb);
		}
	}
}

void LLCOFMgr::onLinkAttachmentComplete(const LLUUID& idItem)
{
	const LLUUID& idItemBase = gInventory.getLinkedItemID(idItem);

	// Remove the attachment from the pending list
	uuid_vec_t::iterator itPending = std::find(m_PendingAttachLinks.begin(), m_PendingAttachLinks.end(), idItemBase);
	if (itPending != m_PendingAttachLinks.end())
		m_PendingAttachLinks.erase(itPending);

	// It may have been detached already in which case we should remove the COF link
	if ( (gAgent.getAvatarObject()) && (!gAgent.getAvatarObject()->isWearingAttachment(idItemBase)) )
		removeCOFItemLinks(idItemBase);
}

void LLCOFMgr::updateAttachments()
{
	/*const*/ LLVOAvatar* pAvatar = gAgent.getAvatarObject();
	if (!pAvatar)
		return;

	const LLUUID idCOF = getCOF();

	// Grab all attachment links currently in COF
	LLInventoryModel::item_array_t items;
	getDescendentsOfAssetType(idCOF, items, LLAssetType::AT_OBJECT, true);

	// Include attachments which should be in COF but don't have their link created yet
	uuid_vec_t::iterator itPendingAttachLink = m_PendingAttachLinks.begin();
	while (itPendingAttachLink != m_PendingAttachLinks.end())
	{
		const LLUUID& idItem = *itPendingAttachLink;
		if ( (!pAvatar->isWearingAttachment(idItem)) || (isLinkInCOF(idItem)) )
		{
			itPendingAttachLink = m_PendingAttachLinks.erase(itPendingAttachLink);
			continue;
		}

		LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
		if (pItem)
			items.push_back(pItem);

		++itPendingAttachLink;
	}

	// Don't remove attachments until avatar is fully loaded (should reduce random attaching/detaching/reattaching at log-on)
	LLAgent::userUpdateAttachments(items, !pAvatar->isFullyLoaded());
}

// ============================================================================
// Wearable functions
//

void LLCOFMgr::addWearable(const LLUUID& idItem)
{
	if ( (isLinkInCOF(idItem)) || (std::find(m_PendingWearableLinks.begin(), m_PendingWearableLinks.end(), idItem) != m_PendingWearableLinks.end()) )
	{
		return;
	}

	gInventory.addChangedMask(LLInventoryObserver::LABEL, idItem);
	m_PendingWearableLinks.push_back(idItem);

	LLPointer<LLInventoryCallback> cb = new LLLinkWearableCallback();
	addCOFItemLink(idItem, cb);
}

void LLCOFMgr::removeWearable(const LLUUID& idItem)
{
	gInventory.addChangedMask(LLInventoryObserver::LABEL, idItem);

	// Remove the attachment from the pending list
	uuid_vec_t::iterator itPending = std::find(m_PendingWearableLinks.begin(), m_PendingWearableLinks.end(), idItem);
	if (itPending != m_PendingWearableLinks.end())
		m_PendingWearableLinks.erase(itPending);
	
   removeCOFItemLinks(idItem);
}

void LLCOFMgr::onLinkWearableComplete(const LLUUID& idItem)
{
	const LLUUID& idItemBase = gInventory.getLinkedItemID(idItem);

	// Remove the attachment from the pending list
	uuid_vec_t::iterator itPending = std::find(m_PendingWearableLinks.begin(), m_PendingWearableLinks.end(), idItemBase);
	if (itPending != m_PendingWearableLinks.end())
		m_PendingWearableLinks.erase(itPending);

	// It may have been removed already in which case we should remove the COF link
	if (!gAgent.isWearingItem(idItemBase))
		removeCOFItemLinks(idItemBase);
}

void LLCOFMgr::synchWearables()
{
	const LLUUID idCOF = getCOF();

	// Grab all wearable links currently in COF
	LLInventoryModel::item_array_t items;
	getDescendentsOfAssetType(idCOF, items, LLAssetType::AT_BODYPART, true);
	getDescendentsOfAssetType(idCOF, items, LLAssetType::AT_CLOTHING, true);

	// Transform collection of link LLViewerInventory pointers into collection of target LLUUIDs
	uuid_vec_t curItems;
	for (S32 idxItem = 0; idxItem < items.count(); idxItem++)
	{
		const LLViewerInventoryItem* pItem = items.get(idxItem);
		if (!pItem)
			continue;
		curItems.push_back(pItem->getLinkedUUID());
	}

	// Grab the item UUIDs of all currently worn wearables
	uuid_vec_t newItems;
	for (S32 idxType = 0; idxType < WT_COUNT; idxType++)
	{
		const LLUUID& idItem = gAgent.getWearableItem((EWearableType)idxType);
		if (idItem.isNull())
			continue;
		newItems.push_back(idItem);
	}

	uuid_vec_t addItems, remItems;
	LLCommonUtils::computeDifference(newItems, curItems, addItems, remItems);

	// Add links for worn wearables that aren't linked yet
	for (uuid_vec_t::const_iterator itItem = addItems.begin(); itItem != addItems.end(); ++itItem)
		addWearable(*itItem);

	// Remove links of wearables that aren't worn anymore
	for (uuid_vec_t::const_iterator itItem = remItems.begin(); itItem != remItems.end(); ++itItem)
		removeWearable(*itItem);
}

// ============================================================================
