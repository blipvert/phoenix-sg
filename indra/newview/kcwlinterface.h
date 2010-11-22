/** 
 * @file kcwlinterface.h
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
#include "llmemory.h"	// LLSingleton<>

class KCWindlightInterface : public LLSingleton<KCWindlightInterface>
{
public:
	KCWindlightInterface();
	
	bool ChatCommand(std::string message, std::string from_name, LLUUID source_id, LLUUID owner_id);
	void PacelChange();
	void onClickWLStatusButton();
	void Clear(S32 local_id);
	bool WLset;
	
private:
	bool KCWindlightInterface::callbackParcelWL(const LLSD& notification, const LLSD& response);
	bool KCWindlightInterface::callbackParcelWLClear(const LLSD& notification, const LLSD& response);
	std::set<LLUUID> mAllowedLand;
};
