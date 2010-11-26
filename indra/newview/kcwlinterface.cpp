/** 
 * @file kcwlinterface.cpp
 * @brief Windlight Interface
 *
 * Copyright (C) 2010, Kadah Coba
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * The Phoenix Viewer Project, http://www.phoenixviewer.com/
 */

#include "llviewerprecompiledheaders.h"

#include "kcwlinterface.h"

#include "llagent.h"
#include "llcallingcard.h" // isBuddy
#include "llstartup.h"
#include "llparcel.h"
#include "llviewercontrol.h" // gSavedSettings, gSavedPerAccountSettings
#include "llviewermenu.h" // is_agent_friend
#include "llviewerparcelmgr.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"

#include <boost/regex.hpp>

const F32 PARCEL_WL_CHECK_TIME  = 5;

KCWindlightInterface::KCWindlightInterface() :
	LLEventTimer(PARCEL_WL_CHECK_TIME),
	WLset(FALSE)
{

}

void KCWindlightInterface::ParcelChange()
{
	if (!gSavedSettings.getBOOL("PhoenixWLParcelEnabled"))
		return;

	LLParcel *parcel = NULL;
	S32 this_parcel_id = 0;
	std::string desc;
 
 	parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
 
	if (parcel)
	{
		this_parcel_id = parcel->getLocalID();
		desc = parcel->getDesc();
	}

	if ( (this_parcel_id != mLastParcelID) || (mLastParcelDesc != desc) ) //parcel changed
	{
		llinfos << "agent in new parcel: "<< this_parcel_id << " : "  << parcel->getName() << llendl;

		mLastParcelID = this_parcel_id;
		mLastParcelDesc = desc;

		//clear the last notification if its still open
		if (mSetWLNotification && !mSetWLNotification->isRespondedTo())
		{
			LLSD response = mSetWLNotification->getResponseTemplate();
			response["Ignore"] = true;
			mSetWLNotification->respond(response);
		}
		mEventTimer.reset();
		mEventTimer.start();
	}
}

BOOL KCWindlightInterface::tick()
{
	if(LLStartUp::getStartupState() < STATE_STARTED)
		return FALSE;

	LLParcel *parcel = NULL;

	parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

	if (parcel)
	{
		LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
		LoadFromPacel(parcel);
		mEventTimer.stop();
	}

	return FALSE;
}


void KCWindlightInterface::ApplySettings(const LLSD& settings)
{
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (!settings.has("local_id") || (settings["local_id"].asInteger() == parcel->getLocalID()) )
	{
		if (settings.has("wlpreset"))
		{
			llinfos << "WL set : " << settings["local_id"] << " : " << settings["wlpreset"] << llendl;
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;
			LLWLParamManager::instance()->loadPreset(settings["wlpreset"].asString());
			WLset = true;
		}
		if (settings.has("wwpreset"))
		{
			llinfos << "WW set : " << settings["local_id"] << " : " << settings["wwpreset"] << llendl;
			LLWaterParamManager::instance()->loadPreset(settings["wwpreset"].asString(), true);
			WLset = true;
		}
	}
}


void KCWindlightInterface::ResetToRegion()
{
	llinfos << "Got WL clear" << llendl;
	//TODO: clear per parcel
	if (WLset)
	{
		LLWLParamManager::instance()->mAnimator.mIsRunning = true;
		LLWLParamManager::instance()->mAnimator.mUseLindenTime = true;
		LLWLParamManager::instance()->loadPreset("Default", true);
		LLWaterParamManager::instance()->loadPreset("Default", true);
		//KC: reset last to Default
		gSavedPerAccountSettings.setString("PhoenixLastWLsetting", "Default");
		gSavedPerAccountSettings.setString("PhoenixLastWWsetting", "Default");
		WLset = false;
	}
}

//KC: Disabling this for now
#if 0
bool KCWindlightInterface::ChatCommand(std::string message, std::string from_name, LLUUID source_id, LLUUID owner_id)
{
	boost::cmatch match;
	const boost::regex prefix_exp("^\\)\\*\\((.*)");
	if(boost::regex_match(message.c_str(), match, prefix_exp))
	{
		std::string data(match[1].first, match[1].second);
		
		//TODO: expand these or good as is?
		/*const boost::regex setWLpreset_exp("^setWLpreset\\|(.*)");
		const boost::regex setWWpreset_exp("^setWWpreset\\|(.*)");
		if(boost::regex_match(data.c_str(), match, setWLpreset_exp))
		{
			llinfos << "got setWLpreset : " << match[1] << llendl;
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;
			LLWLParamManager::instance()->loadPreset(match[1]);
			return true;
		}
		else if(boost::regex_match(data.c_str(), match, setWWpreset_exp))
		{
			llinfos << "got setWWpreset : " << match[1] << llendl;
			LLWaterParamManager::instance()->loadPreset(match[1], true);
			return true;
		}
		else 
		{*/
		
		//TODO: add save settings for reuse instead of just clearing on parcel change
		//TODO: add support for region wide settings on non-mainland
		//TODO: add support for targeting specfic users
		//TODO: add support for custom settings via notecards or something
		//TODO: improved data processing, possibly just use LLSD as input instead
		
			boost::smatch match2;
			const boost::regex Parcel_exp("^(Parcel),WLPreset=\"([^\"\\r\\n]+)\"(,WWPreset=\"([^\"\\r\\n]+)\")?$");
			//([\\w]{8}-[\\w]{4}-[\\w]{4}-[\\w]{4}-[\\w]{12})
			if(boost::regex_search(data, match2, Parcel_exp))
			{
				if (SetDialogVisible) //TODO: handle this better
					return true;

				if (match2[1]=="Parcel")
				{
					llinfos << "Got Parcel WL : " << match[2] << llendl;
					
					LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
					LLSD payload;
					payload["local_id"] = parcel->getLocalID();
					payload["land_owner"] = parcel->getOwnerID();
					payload["wlpreset"] = std::string(match2[2].first, match2[2].second);
					payload["wwpreset"] = std::string(match2[3].first, match2[3].second);

					LLSD args;
					args["PARCEL_NAME"] = parcel->getName();
					
					LLNotifications::instance().add("PhoenixWL", args, payload, boost::bind(&KCWindlightInterface::callbackParcelWL, this, _1, _2));
					SetDialogVisible = true;
				}
				return true;
			}
		/*}*/
	}
	return false;
}
#endif

void KCWindlightInterface::LoadFromPacel(LLParcel *parcel)
{
	if (!parcel)
		return;

	LLSD payload;
	if (ParsePacelForWLSettings(parcel->getDesc(), payload))
	{
		const LLUUID owner_id = getOwnerID(parcel);
		//basic auth for now
		if (AllowedLandOwners(owner_id))
		{
			ApplySettings(payload);
		}
		else
		{
			LLSD args;
			args["PARCEL_NAME"] = parcel->getName();
			args["OWNER_NAME"] = getOwnerName(parcel);
			payload["parcel_name"] = parcel->getName();
			payload["local_id"] = parcel->getLocalID();
			payload["land_owner"] = owner_id;

			mSetWLNotification = LLNotifications::instance().add("PhoenixWL", args, payload, boost::bind(&KCWindlightInterface::callbackParcelWL, this, _1, _2));
		}
	}
	else
	{ //if nothing defined, reset to region settings
		ResetToRegion();
	}
}

bool KCWindlightInterface::ParsePacelForWLSettings(const std::string& desc, LLSD& settings)
{
	llinfos << "desc: " << desc << llendl;
	
	bool found_settings = false;
	boost::smatch mat_block;
	//parcel desc /*[data goes here]*/
	const boost::regex Parcel_exp("(?i)\\/\\*(?:Windlight)?([\\s\\S]*?)\\*\\/");
	if(boost::regex_search(desc, mat_block, Parcel_exp))
	{
		std::string data1(mat_block[1].first, mat_block[1].second);
		llinfos << "found parcel flags block: " << mat_block[1] << llendl;
		
		boost::smatch match;
		std::string::const_iterator start = mat_block[1].first;
		std::string::const_iterator end = mat_block[1].second;
		//Sky: "preset" Water: "preset"
		const boost::regex key("(?i)(?:(?:(Sky)(?:\\s?@\\s?([\\d])+m?)?)|(Water)):\\s?\"([^\"\\r\\n]+)\"");
		while (boost::regex_search(start, end, match, key, boost::match_default))
		{
			llinfos << "parcel flag: " << match[1] << " : " << match[2] << " : " << match[3] << " : " << match[4] << llendl;

			if (match[1].matched)
			{
				std::string preset(match[4]);
				llinfos << "got sky: " << preset << llendl;
				if(LLWLParamManager::instance()->mParamList.find(preset) != LLWLParamManager::instance()->mParamList.end())
				{
					settings["wlpreset"] = preset;
					found_settings = true;
				}
			}
			else if (match[3].matched)
			{
				std::string preset(match[4]);
				llinfos << "got water: " << preset << llendl;
				if(LLWaterParamManager::instance()->mParamList.find(preset) != LLWaterParamManager::instance()->mParamList.end())
				{
					settings["wwpreset"] = preset;
					found_settings = true;
				}
			}
			
			// update search position 
			start = match[0].second; 
		}
	}

	return found_settings;
}

void KCWindlightInterface::onClickWLStatusButton()
{
	//clear the last notification if its still open
	if (mClearWLNotification && !mClearWLNotification->isRespondedTo())
	{
		LLSD response = mClearWLNotification->getResponseTemplate();
		response["Ignore"] = true;
		mClearWLNotification->respond(response);
	}

	if (WLset)
	{
		LLParcel *parcel = NULL;
 		parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
 		if (parcel)
		{
			//TODO: this could be better
			LLSD payload;
			payload["local_id"] = parcel->getLocalID();
			payload["land_owner"] = getOwnerID(parcel);

			LLSD args;
			args["PARCEL_NAME"] = parcel->getName();
			
			mClearWLNotification = LLNotifications::instance().add("PhoenixWLClear", args, payload, boost::bind(&KCWindlightInterface::callbackParcelWLClear, this, _1, _2));
		}
	}
}

bool KCWindlightInterface::callbackParcelWL(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0)
	{
		mAllowedLand.insert(notification["payload"]["land_owner"].asUUID());
		
		ApplySettings(notification["payload"]);
	}
	else
	{
		ResetToRegion();
	}
	return false;
}

bool KCWindlightInterface::callbackParcelWLClear(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (option == 0)
	{
		LLUUID owner_id = notification["payload"]["land_owner"].asUUID();

		mAllowedLand.erase(owner_id);
		ResetToRegion();
	}
	return false;
}

bool KCWindlightInterface::AllowedLandOwners(const LLUUID& owner_id)
{
	if ( gSavedSettings.getBOOL("PhoenixWLWhitelistAll") ||	// auto all
		(owner_id == gAgent.getID()) ||						// land is owned by agent
		(LLAvatarTracker::instance().isBuddy(owner_id) && gSavedSettings.getBOOL("PhoenixWLWhitelistFriends")) || // is friend's land
		(gAgent.isInGroup(owner_id) && gSavedSettings.getBOOL("PhoenixWLWhitelistGroups")) || // is member of land's group
		(mAllowedLand.find(owner_id) != mAllowedLand.end()) ) // already on whitelist
	{
		return true;
	}
	return false;
}

LLUUID KCWindlightInterface::getOwnerID(LLParcel *parcel)
{
	if (parcel->getIsGroupOwned())
	{
		return parcel->getGroupID();
	}
	return parcel->getOwnerID();
}

std::string KCWindlightInterface::getOwnerName(LLParcel *parcel)
{
	//TODO: say if its a group or avatar on notice
	std::string owner;
	if (parcel->getIsGroupOwned())
	{
		gCacheName->getGroupName(parcel->getGroupID(), owner);
	}
	else
	{
		gCacheName->getFullName(parcel->getOwnerID(), owner);
	}
	return owner;
}

//KC: this is currently not used
//TODO: merge this relay code in to bridge when more final, currently only supports "Parcel,WLPreset='[preset name]'"
/*
integer PHOE_WL_CH = -1346916165;
default
{
    state_entry()
    {
		llListen(PHOE_WL_CH, "", NULL_KEY, "");
    }
    listen( integer iChan, string sName, key kID, string sMsg )
    {
        if ( llGetOwnerKey(kID) == llGetLandOwnerAt(llGetPos()) )
        {
            llOwnerSay(")*(" + sMsg);
        }
    }
}
*/
