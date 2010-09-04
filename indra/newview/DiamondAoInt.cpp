#include "llviewerprecompiledheaders.h"

#include "DiamondAoInt.h"
#include "floaterao.h"
#include "jc_lslviewerbridge.h"

S32 DiamondAoInt::regchan;
DiamondAoInt::DiamondAoInt()
{
}

DiamondAoInt::~DiamondAoInt()
{
}

bool DiamondAoInt::AOCommand(std::string message)
{
	std::string clip = message.substr(0,8);
	if(clip == "dmdAoInt")
	{
		std::string rest = message.substr(8);
		LLSD args = JCLSLBridge::parse_string_to_list(rest, '|');
		std::string cmd = args[0].asString();
		if(cmd == "on")
		{
			gSavedPerAccountSettings.setBOOL("EmeraldAOEnabled",TRUE);
			LLFloaterAO::run();
		}
		else if(cmd == "off")
		{
			gSavedPerAccountSettings.setBOOL("EmeraldAOEnabled",FALSE);
			LLFloaterAO::run();
		}
		else if(cmd == "status")
		{
			S32 chan = atoi(args[1].asString().c_str());
			std::string tmp="off";
			if(gSavedPerAccountSettings.getBOOL("EmeraldAOEnabled"))tmp="on";
			JCLSLBridge::send_chat_to_object(tmp,chan,gAgent.getID());
		}
		else if(cmd == "regchan")
		{
			regchan = atoi(args[1].asString().c_str());
			JCLSLBridge::send_chat_to_object(std::string("Channel registerd"),regchan,gAgent.getID());
		}
		return true;
	}
	return false;
}

void DiamondAoInt::AOStatusUpdate(bool status)
{
	if(regchan != 0)
	{
		std::string tmp="off";
		if(gSavedPerAccountSettings.getBOOL("EmeraldAOEnabled"))tmp="on";
		JCLSLBridge::send_chat_to_object(tmp,regchan,gAgent.getID());
	}
}