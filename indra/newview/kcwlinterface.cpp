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

#include "llstartup.h"
#include "llparcel.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"

#include <boost/regex.hpp>


KCWindlightInterface::KCWindlightInterface()
{
	WLset = false;
}

void KCWindlightInterface::Clear(S32 local_id)
{
	llinfos << "Got WL clear : " << local_id << llendl;
	//TODO: clear per parcel
	if (WLset)
	{
		LLWLParamManager::instance()->mAnimator.mIsRunning = true;
		LLWLParamManager::instance()->mAnimator.mUseLindenTime = true;
		//KC: reset last to Default
		gSavedPerAccountSettings.setString("PhoenixLastWLsetting", "Default");
		WLset = false;
	}
}

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
		
			//const boost::regex Parcel_exp("^(Parcel),WLPreset='(.+)'(,WWPreset='(.+)')?$"); //TODO: this needs improvment ...actually it doesnt work right in sl boost for somereason :/
			const boost::regex Parcel_exp("^(Parcel),WLPreset='(.+)'$"); //temp hack
			//([\\w]{8}-[\\w]{4}-[\\w]{4}-[\\w]{4}-[\\w]{12})
			if(boost::regex_match(data.c_str(), match, Parcel_exp))
			{
				if (match[1]=="Parcel")
				{
					llinfos << "Got Parcel WL : " << match[2] << llendl;
					
					LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
					LLSD payload;
					payload["local_id"] = parcel->getLocalID();
					payload["wlpreset"] = std::string(match[2].first, match[2].second);
					//payload["wwpreset"] = std::string(match[3].first, match[3].second);

					LLSD args;
					args["PARCEL_NAME"] = parcel->getName();
					
					LLNotifications::instance().add("PhoenixWL", args, payload);
				}
				return true;
			}
		//}
	}
	return false;
}

void KCWindlightInterface::PacelChange()
{
	if(LLStartUp::getStartupState() == STATE_STARTED)
	{
		LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
		llinfos << "agent in new parcel: "<< parcel->getLocalID() << " : "  << parcel->getName() << llendl;
		
		//TODO: save settings percel to reuse instead of just clearing
		Clear(parcel->getLocalID());
	}
}

void KCWindlightInterface::onClickWLStatusButton()
{
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	LLSD payload;
	payload["local_id"] = parcel->getLocalID();

	LLSD args;
	args["PARCEL_NAME"] = parcel->getName();
	
	LLNotifications::instance().add("PhoenixWLClear", args, payload);
}

bool ph_set_wl_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	S32 local_id = notification["payload"]["local_id"].asInteger();
	std::string wlpreset = notification["payload"]["wlpreset"].asString();
	std::string wwpreset = notification["payload"]["wwpreset"].asString();

	if (option == 0)
	{
		//TODO: save per parcel
		if (!wlpreset.empty())
		{
			llinfos << "WL set : " << local_id << " : " << wlpreset << llendl;
			LLWLParamManager::instance()->mAnimator.mIsRunning = false;
			LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;
			LLWLParamManager::instance()->loadPreset(wlpreset);
			KCWindlightInterface::instance().WLset = true;
		}
		if (!wwpreset.empty())
		{
			llinfos << "WW set : " << local_id << " : " << wwpreset << llendl;
			LLWaterParamManager::instance()->loadPreset(wwpreset, true);
		}
	}
	return false;
}
static LLNotificationFunctorRegistration ph_set_wl_callback_reg("PhoenixWL", ph_set_wl_callback);

bool ph_clear_wl_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	S32 local_id = notification["payload"]["local_id"].asInteger();

	if (option == 0)
	{
		KCWindlightInterface::instance().Clear(local_id);
	}
	return false;
}
static LLNotificationFunctorRegistration ph_clear_wl_callback_reg("PhoenixWLClear", ph_clear_wl_callback);


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
