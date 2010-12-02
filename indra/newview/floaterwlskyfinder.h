/*
	Copyright Chris Rehor (Liny Odell) 2010.
	Licensed under the artistic license version 2.
	http://www.perlfoundation.org/artistic_license_2_0
*/

#ifndef FLOATERWLSKYFINDER_H
#define FLOATERWLSKYFINDER_H

#include "llfloaterhtmlsimple.h"

class FloaterSkyfinder : public LLFloaterHtmlSimple, public LLFloaterSingleton<FloaterSkyfinder>
{
	friend class LLUISingleton<FloaterSkyfinder, VisibilityPolicy<LLFloater> >;
	
public:
	FloaterSkyfinder(const LLSD& key);
	~FloaterSkyfinder() { }
	
	static void show(void*);
};

class PhoenixSkyDownloader : public LLHTTPClient::Responder
{
public:
	PhoenixSkyDownloader(const std::string& sky_name, const std::string& display_name);
	~PhoenixSkyDownloader() { }
	void error(U32 status, const std::string& reason);
	void completedRaw(
							  U32 status,
							  const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer);
	static void download_complete_dismissed(const LLSD& notification, const LLSD& response);

private:
	static std::string mSkyName;
	std::string mDisplayName;
	std::string mSkyDir;
	
	bool decompressSky(const std::string& zip, const std::string& dir);
};

#endif // FLOATERWLSKYFINDER_H
