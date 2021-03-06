/* Copyright (c) 2010
 *
 * Modular Systems All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "llviewerprecompiledheaders.h"

#include "a_phoenixviewerlink.h"

#include "llbufferstream.h"

#include "llappviewer.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>

#include "llsdserialize.h"

#include "llversionviewer.h"
#include "llprimitive.h"
#include "llagent.h"
#include "llnotifications.h"
#include "llimview.h"
#include "llfloaterabout.h"
#include "llviewercontrol.h"
#include "floaterblacklist.h"
#include "llsys.h"
#include "llviewermedia.h"

std::string PhoenixViewerLink::blacklist_version;
LLSD PhoenixViewerLink::blocked_login_info = 0;
LLSD PhoenixViewerLink::phoenix_tags = 0;
BOOL PhoenixViewerLink::msDataDone = FALSE;

PhoenixViewerLink* PhoenixViewerLink::sInstance;

PhoenixViewerLink::PhoenixViewerLink()
{
	sInstance = this;
}

PhoenixViewerLink::~PhoenixViewerLink()
{
	sInstance = NULL;
}

PhoenixViewerLink* PhoenixViewerLink::getInstance()
{
	if(sInstance)return sInstance;
	else
	{
		sInstance = new PhoenixViewerLink();
		return sInstance;
	}
}

static const std::string versionid = llformat("%s %d.%d.%d (%d)", LL_CHANNEL, LL_VERSION_MAJOR, LL_VERSION_MINOR, LL_VERSION_PATCH, LL_VERSION_BUILD);

//void cmdline_printchat(std::string message);

void PhoenixViewerLink::start_download()
{
	//cmdline_printchat("requesting msdata");
	std::string url = "http://phoenixviewer.com/app/msdata/";
	LLSD headers;
	headers.insert("Accept", "*/*");
	headers.insert("User-Agent", LLViewerMedia::getCurrentUserAgent());
	headers.insert("viewer-version", versionid);

	LL_INFOS("Data") << "Downloading msdata." << LL_ENDL;
	LLHTTPClient::get(url,new ModularSystemsDownloader( PhoenixViewerLink::msdata ),headers);
	
	url = "http://phoenixviewer.com/app/blacklist/blacklist.xml";

	LL_INFOS("Blacklist") << "Downloading blacklist.xml" << LL_ENDL;
	LLHTTPClient::get(url,new ModularSystemsDownloader( PhoenixViewerLink::msblacklist ),headers);

	downloadClientTags();
}
void PhoenixViewerLink::downloadClientTags()
{
	if(gSavedSettings.getBOOL("PhoenixDownloadClientTags"))
	{
		//url = "http://phoenixviewer.com/app/client_tags/client_list.xml";
		std::string url("http://phoenixviewer.com/app/client_list.xml");
		if(gSavedSettings.getBOOL("PhoenixDontUseMultipleColorTags"))
		{
			url="http://phoenixviewer.com/app/client_list_unified_colours.xml";
		}
		LLSD headers;
		LLHTTPClient::get(url,new ModularSystemsDownloader( PhoenixViewerLink::updateClientTags),headers);
		LL_INFOS("CLIENTTAGS DOWNLOADER") << "Getting new tags" << LL_ENDL;
	}
	else
	{
		updateClientTagsLocal();
	}
	
}

void PhoenixViewerLink::msblacklist(U32 status,std::string body)
{
	if(status != 200)
	{
		LL_WARNS("Blacklist") << "Something went wrong with the blacklist download status code " << status << LL_ENDL;
	}

	std::istringstream istr(body);
	if (body.size() > 0)
	{
		LLSD data;
		if(LLSDSerialize::fromXML(data,istr) >= 1)
		{
			LL_INFOS("Blacklist") << body.size() << " bytes received, updating local blacklist" << LL_ENDL;
			for(LLSD::map_iterator itr = data.beginMap(); itr != data.endMap(); ++itr)
			{
				if(itr->second.has("name"))
					LLFloaterBlacklist::addEntry(LLUUID(itr->first),itr->second);
			}
		}
		else
		{
			LL_INFOS("Blacklist") << "Failed to parse blacklist.xml" << LL_ENDL;
		}
	}
	else
	{
		LL_INFOS("Blacklist") << "Empty blacklist.xml" << LL_ENDL;
	}
}

void PhoenixViewerLink::updateClientTags(U32 status,std::string body)
{
    std::string client_list_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "client_list.xml");

    std::istringstream istr(body);
    LLSD data;
    if(LLSDSerialize::fromXML(data, istr) >= 1)
	{
		llofstream export_file;
        export_file.open(client_list_filename);
        LLSDSerialize::toPrettyXML(data, export_file);
        export_file.close();
    }
}

void PhoenixViewerLink::updateClientTagsLocal()
{
	std::string client_list_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "client_list.xml");

	llifstream xml_file(client_list_filename);
	LLSD data;
	if(!xml_file.is_open()) return;
	if(LLSDSerialize::fromXML(data, xml_file) >= 1)
	{
		if(data.has("phoenixTags"))
		{
			phoenix_tags = data["phoenixTags"];
			LLPrimitive::tagstring = PhoenixViewerLink::phoenix_tags[gSavedSettings.getString("PhoenixTagColor")].asString();
		}

		xml_file.close();
	}
}

void PhoenixViewerLink::msdata(U32 status, std::string body)
{
	PhoenixViewerLink* self = getInstance();
	//cmdline_printchat("msdata downloaded");

	LLSD data;
	std::istringstream istr(body);
	LLSDSerialize::fromXML(data, istr);
	if(data.isDefined())
	{
		LLSD& support_agents = data["agents"];

		self->personnel.clear();
		for(LLSD::map_iterator itr = support_agents.beginMap(); itr != support_agents.endMap(); ++itr)
		{
			std::string key = (*itr).first;
			LLSD& content = (*itr).second;
			U8 val = 0;
			if(content.has("support"))val = val | EM_SUPPORT;
			if(content.has("developer"))val = val | EM_DEVELOPER;
			self->personnel[LLUUID(key)] = val;
		}

		LLSD& versions = data["versions"];

		self->versions2.clear();
		for(LLSD::map_iterator itr = versions.beginMap(); itr != versions.endMap(); ++itr)
		{
			std::string key = (*itr).first;
			key += "\n";
			LLSD& content = (*itr).second;
			U8 val = 0;
			if(content.has("beta"))val = val | PH_BETA;
			if(content.has("release"))val = val | PH_RELEASE;
			self->versions2[key] = val;
		}

		LLSD& blocked = data["blocked"];

		self->blocked_versions.clear();
		for (LLSD::array_iterator itr = blocked.beginArray(); itr != blocked.endArray(); ++itr)
		{
			std::string vers = (*itr).asString();
			self->blocked_versions.insert(vers);
		}

		if(data.has("MOTD"))
		{
			self->ms_motd = data["MOTD"].asString();
			gAgent.mMOTD = self->ms_motd;
		}else
		{
			self->ms_motd = "";
		}
		if(data.has("BlockedReason"))
		{
			blocked_login_info = data["BlockedReason"]; 
		}
		if(data.has("phoenixTags"))
		{
			phoenix_tags = data["phoenixTags"];
			LLPrimitive::tagstring = PhoenixViewerLink::phoenix_tags[gSavedSettings.getString("PhoenixTagColor")].asString();
		}
		msDataDone = TRUE;
	}

	//LLSD& dev_agents = data["dev_agents"];
	//LLSD& client_ids = data["client_ids"];
}

BOOL PhoenixViewerLink::is_support(LLUUID id)
{
	PhoenixViewerLink* self = getInstance();
	if(self->personnel.find(id) != self->personnel.end())
	{
		return ((self->personnel[id] & EM_SUPPORT) != 0) ? TRUE : FALSE;
	}
	return FALSE;
}

BOOL PhoenixViewerLink::is_BetaVersion(std::string version)
{
	PhoenixViewerLink* self = getInstance();
	if(self->versions2.find(version) != self->versions2.end())
	{
		return ((self->versions2[version] & PH_BETA) != 0) ? TRUE : FALSE;
	}
	return FALSE;
}

BOOL PhoenixViewerLink::is_ReleaseVersion(std::string version)
{
	PhoenixViewerLink* self = getInstance();
	if(self->versions2.find(version) != self->versions2.end())
	{
		return ((self->versions2[version] & PH_RELEASE) != 0) ? TRUE : FALSE;
	}
	return FALSE;
}

BOOL PhoenixViewerLink::is_developer(LLUUID id)
{
	PhoenixViewerLink* self = getInstance();
	if(self->personnel.find(id) != self->personnel.end())
	{
		return ((self->personnel[id] & EM_DEVELOPER) != 0) ? TRUE : FALSE;
	}
	return FALSE;
}

BOOL PhoenixViewerLink::allowed_login()
{
	PhoenixViewerLink* self = getInstance();
	return (self->blocked_versions.find(versionid) == self->blocked_versions.end());
}


std::string PhoenixViewerLink::processRequestForInfo(LLUUID requester, std::string message, std::string name, LLUUID sessionid)
{
	std::string detectstring = "/reqsysinfo";
	if(!message.find(detectstring)==0)
	{
		//llinfos << "sysinfo was not found in this message, it was at " << message.find("/sysinfo") << " pos." << llendl;
		return message;
	}
	if(!(is_support(requester)||is_developer(requester)))
	{
		return message;
	}
	std::string my_name;
	gAgent.buildFullname(my_name);

	//llinfos << "sysinfo was found in this message, it was at " << message.find("/sysinfo") << " pos." << llendl;
	std::string outmessage("I am requesting information about your system setup.");
	std::string reason("");
	if(message.length()>detectstring.length())
	{
		reason = std::string(message.substr(detectstring.length()));
		//there is more to it!
		outmessage = std::string("I am requesting information about your system setup for this reason : "+reason);
		reason = "The reason provided was : "+reason;
	}
	LLSD args;
	args["REASON"] =reason;
	args["NAME"] = name;
	args["FROMUUID"]=requester;
	args["SESSIONID"]=sessionid;
	LLNotifications::instance().add("PhoenixReqInfo",args,LLSD(), callbackPhoenixReqInfo);

	return outmessage;
}
void PhoenixViewerLink::sendInfo(LLUUID destination, LLUUID sessionid, std::string myName, EInstantMessage dialog)
{

	std::string myInfo1 = getMyInfo(1);
	std::string myInfo2 = getMyInfo(2);	

	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		destination,
		myName,
		myInfo1,
		IM_ONLINE,
		dialog,
		sessionid
		);
	gAgent.sendReliableMessage();
	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		destination,
		myName,
		myInfo2,
		IM_ONLINE,
		dialog,
		sessionid);
	gAgent.sendReliableMessage();
	gIMMgr->addMessage(gIMMgr->computeSessionID(dialog,destination),destination,myName,"Information Sent: "+
		myInfo1+"\n"+myInfo2);
}
void PhoenixViewerLink::callbackPhoenixReqInfo(const LLSD &notification, const LLSD &response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	std::string my_name;
	LLSD subs = LLNotification(notification).getSubstitutions();
	LLUUID uid = subs["FROMUUID"].asUUID();
	LLUUID sessionid = subs["SESSIONID"].asUUID();

	llinfos << "the uuid is " << uid.asString().c_str() << llendl;
	gAgent.buildFullname(my_name);
	//LLUUID sessionid = gIMMgr->computeSessionID(IM_NOTHING_SPECIAL,uid);
	if ( option == 0 )//yes
	{
		sendInfo(uid,sessionid,my_name,IM_NOTHING_SPECIAL);
	}
	else
	{

		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			uid,
			my_name,
			"Request Denied.",
			IM_ONLINE,
			IM_NOTHING_SPECIAL,
			sessionid
			);
		gAgent.sendReliableMessage();
		gIMMgr->addMessage(sessionid,uid,my_name,"Request Denied");
	}
}
//part , 0 for all, 1 for 1st half, 2 for 2nd
std::string PhoenixViewerLink::getMyInfo(int part)
{
	std::string info("");
	if(part!=2)
	{

		info.append(LLFloaterAbout::get_viewer_version());
		info.append("\n");
		info.append(LLFloaterAbout::get_viewer_build_version());
		info.append("\n");
		info.append(LLFloaterAbout::get_viewer_region_info("I am "));
		info.append("\n");
		if(part==1)return info;
	}
	info.append(LLFloaterAbout::get_viewer_misc_info());
	return info;
}

ModularSystemsDownloader::ModularSystemsDownloader(void (*callback)(U32, std::string)) : mCallback(callback) {}
ModularSystemsDownloader::~ModularSystemsDownloader(){}
void ModularSystemsDownloader::error(U32 status, const std::string& reason){}
void ModularSystemsDownloader::completedRaw(
			U32 status,
			const std::string& reason,
			const LLChannelDescriptors& channels,
			const LLIOPipe::buffer_ptr_t& buffer)
{
	LLBufferStream istr(channels, buffer.get());
	std::stringstream strstrm;
	strstrm << istr.rdbuf();
	std::string result = std::string(strstrm.str());
	mCallback(status, result);
}
