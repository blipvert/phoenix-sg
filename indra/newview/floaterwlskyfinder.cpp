/*
	Copyright Chris Rehor (Liny Odell) 2010.
	Licensed under the artistic license version 2.
	http://www.perlfoundation.org/artistic_license_2_0
*/

#include "llviewerprecompiledheaders.h"

#include "floaterwlskyfinder.h"
#include "lluictrlfactory.h"
#include "llmediactrl.h"
#include "llnotifications.h"
#include "llcommandhandler.h"
#include "lldir.h"
#include "llfile.h"
#include "llhttpclient.h"
#include "llbufferstream.h"
#include "llviewercontrol.h"
#include "rlvhandler.h"
#include "llwlparammanager.h"

FloaterSkyfinder::FloaterSkyfinder(const LLSD& key = LLSD()) : LLFloaterHtmlSimple("http://phoenixviewer.com/app/windlight_skys/")
{
	setTrusted(true);
	setTitle("Windlight Sky Browser");
	F32 ui = gSavedSettings.getF32("UIScaleFactor");
	reshape((S32)llround(710.0f/ui), (S32)llround(462.0f/ui), FALSE);
	setCanResize(FALSE);
	center();
}

void FloaterSkyfinder::show(void*)
{
	FloaterSkyfinder::showInstance(LLSD());
}

class FloaterSkyfinderHandler : public LLCommandHandler
{
public:
	FloaterSkyfinderHandler() : LLCommandHandler("phwlsky", false) { }
	
	bool handle(const LLSD& tokens, const LLSD& query_map,	LLMediaCtrl* web);
	static void notificationCallback(const LLSD& notification, const LLSD& response);
};

FloaterSkyfinderHandler gFloaterSkyfinderHandler;

bool FloaterSkyfinderHandler::handle(const LLSD& tokens, const LLSD& query_map,	LLMediaCtrl* web)
{
	if(tokens.size() != 2)
		return false;

	if(tokens[0].asString() != "download")
		return false;
	
	std::string sky_name = tokens[1].asString();
	
	std::string display_name;
	if(query_map.has("display_name"))
	{
		display_name = query_map["display_name"].asString();
	}
	else
	{
		display_name = sky_name;
	}
	
	LLSD subs;
	subs["[DISPLAY_NAME]"] = display_name;
	LLSD payload;
	payload["display_name"] = display_name;
	payload["sky_name"] = sky_name;
	
	LLNotifications::instance().add("PhoenixSkyDownloadPrompt", subs, payload, &notificationCallback);
	
	return true;
}

void FloaterSkyfinderHandler::notificationCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if(option != 0)
		return;
	
	std::string sky_name = notification["payload"]["sky_name"];
	std::string display_name = notification["payload"]["display_name"];
	
	std::string user_sky_dir = gDirUtilp->getOSUserAppDir() + gDirUtilp->getDirDelimiter() + "user_settings" + gDirUtilp->getDirDelimiter() + "windlight" + gDirUtilp->getDirDelimiter() + "skies";
	
	// Cheat if we already have it.
	if(LLFile::isdir(user_sky_dir + gDirUtilp->getDirDelimiter() + sky_name) || 
		LLFile::isfile(gDirUtilp->getAppRODataDir() + gDirUtilp->getDirDelimiter()+ "app_settings" + gDirUtilp->getDirDelimiter() + "windlight" + gDirUtilp->getDirDelimiter() + "skies" + gDirUtilp->getDirDelimiter() + sky_name))
	{
		LL_INFOS("SkyDownload") << "Sky already downloaded; using that." << LL_ENDL;
		
		LLSD args;
		args["[DISPLAY_NAME]"] = display_name;;
		LLNotifications::instance().add("PhoenixSkyDownloadComplete", args, LLSD(), &PhoenixSkyDownloader::download_complete_dismissed);
		return;
	}
	
	std::string url = llformat("http://phoenixviewer.com/app/windlight_skys/download/%s.7z", sky_name.c_str());
	
	LL_INFOS("SkyDownload") << "Downloading '" << display_name << "' (" << sky_name << ") sky from " << url << LL_ENDL;
	
	LLHTTPClient::get(url, new PhoenixSkyDownloader(sky_name, display_name));
	return;
}

std::string PhoenixSkyDownloader::mSkyName;
PhoenixSkyDownloader::PhoenixSkyDownloader(const std::string& sky_name, const std::string& display_name) : 
	mDisplayName(display_name)
{
	mSkyName = sky_name;
}

void PhoenixSkyDownloader::error(U32 status, const std::string& reason)
{
	LLSD args;
	args["[REASON]"] = reason;
	args["[STATUS]"] = LLSD::Integer(status);
	args["[DISPLAY_NAME]"] = mDisplayName;
	LLNotifications::instance().add("PhoenixSkyDownloadFailed", args);
}

void PhoenixSkyDownloader::completedRaw(U32 status, const std::string& reason, const LLChannelDescriptors& channels, const LLIOPipe::buffer_ptr_t& buffer)
{
	if(status < 200 || status >= 300)
	{
		error(status, reason);
		return;
	}
	
	std::string sky_dir = gDirUtilp->getOSUserAppDir() + gDirUtilp->getDirDelimiter() + "user_settings" + gDirUtilp->getDirDelimiter() + "windlight" + gDirUtilp->getDirDelimiter() + "skies";
	
	std::string target = sky_dir + gDirUtilp->getDirDelimiter() + mSkyName + ".7z";
	LL_INFOS("SkyDownload") << "Saving file to " << target << LL_ENDL;	

	LLFile::mkdir(sky_dir);
	
	// Save the file.
	LLBufferStream istr(channels, buffer.get());
	llofstream ostr(target, std::ios::binary);
	
	while(istr.good() && ostr.good())
		ostr << istr.rdbuf();
	ostr.close();
	
	decompressSky(target, sky_dir);
	
	LLSD args;
	args["[DISPLAY_NAME]"] = mDisplayName;
	LLNotifications::instance().add("PhoenixSkyDownloadComplete", args, LLSD(), &download_complete_dismissed);
}

void PhoenixSkyDownloader::download_complete_dismissed(const LLSD& notification, const LLSD& response)
{
	if (!(rlv_handler_t::isEnabled() && gRlvHandler.hasBehaviour(RLV_BHVR_SETENV)))
	{
		LLWLParamManager* wlprammgr = LLWLParamManager::instance();
		if(mSkyName != "Default")
		{
			wlprammgr->mAnimator.mIsRunning = false;
			wlprammgr->mAnimator.mUseLindenTime = false;
			wlprammgr->loadPreset(mSkyName,true,true);
		}
		else
		{
			wlprammgr->mAnimator.mIsRunning = true;
			wlprammgr->mAnimator.mUseLindenTime = true;
			wlprammgr->loadPreset("Default");
			//KC: reset last to Default
			gSavedPerAccountSettings.setString("PhoenixLastWLsetting", "Default");
		}
	}
}
	

bool PhoenixSkyDownloader::decompressSky(const std::string& zip, const std::string& dir)
{
	// Forking magic stolen from llvoiceclient.cpp
	// The lzma library is a PITA. And undocumented.
#ifndef LL_WINDOWS
	std::string exe = llformat("%s/7za", gDirUtilp->getExecutableDir().c_str());
	const char *zargv[] = {exe.c_str(), "x", "-y", llformat("-o%s", dir.c_str()).c_str(), zip.c_str(), NULL};
	fflush(NULL);
	pid_t id = vfork();
	if(id == 0)
	{
		execv(exe.c_str(), (char **)zargv);
		_exit(0); // This shouldn't ever be reached.
	}
	return true;
#else
	//PROCESS_INFORMATION pinfo;
	//STARTUPINFOA sinfo;
	//memset(&sinfo, 0, sizeof(sinfo));
	std::string exe_dir = gDirUtilp->getExecutableDir();
	std::string exe_path = "\""+exe_dir +gDirUtilp->getDirDelimiter()+ "7z.exe\"";
	std::string args = llformat(" x -y -o\"%s\" \"%s\"", dir.c_str(), zip.c_str());
	
	// So retarded.  Windows requires that the second parameter to CreateProcessA be a writable (non-const) string...
	/*char *args2 = new char[args.size() + 1];
	strcpy(args2, args.c_str());
	
	if(CreateProcessA(exe_path.c_str(), args2, NULL, NULL, FALSE, 0, NULL, exe_dir.c_str(), &sinfo, &pinfo))
	{
		CloseHandle(pinfo.hThread); // stops leaks - nothing else
	}
	delete[] args2;*/
	std::string theCMD("%COMSPEC% /c START \"Sky Decompression\" " + exe_path + args + " & exit");
	LL_INFOS("SKY DOWNLOAD") << theCMD.c_str() <<LL_ENDL;
	std::system(theCMD.c_str());

	return true;
#endif
}
