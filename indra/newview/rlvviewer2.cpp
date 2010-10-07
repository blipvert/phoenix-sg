/** 
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "rlvviewer2.h"

// ============================================================================
// From llappearancemgr.cpp

// Shim class to allow arbitrary boost::bind
// expressions to be run as one-time idle callbacks.
//
// TODO: rework idle function spec to take a boost::function in the first place.
class OnIdleCallbackOneTime
{
public:
	OnIdleCallbackOneTime(nullary_func_t callable):
		mCallable(callable)
	{
	}
	static void onIdle(void *data)
	{
		gIdleCallbacks.deleteFunction(onIdle, data);
		OnIdleCallbackOneTime* self = reinterpret_cast<OnIdleCallbackOneTime*>(data);
		self->call();
		delete self;
	}
	void call()
	{
		mCallable();
	}
private:
	nullary_func_t mCallable;
};

void doOnIdleOneTime(nullary_func_t callable)
{
	OnIdleCallbackOneTime* cb_functor = new OnIdleCallbackOneTime(callable);
	gIdleCallbacks.addFunction(&OnIdleCallbackOneTime::onIdle,cb_functor);
}

// Shim class to allow generic boost functions to be run as
// recurring idle callbacks.  Callable should return true when done,
// false to continue getting called.
//
// TODO: rework idle function spec to take a boost::function in the first place.
class OnIdleCallbackRepeating
{
public:
	OnIdleCallbackRepeating(bool_func_t callable):
		mCallable(callable)
	{
	}
	// Will keep getting called until the callable returns true.
	static void onIdle(void *data)
	{
		OnIdleCallbackRepeating* self = reinterpret_cast<OnIdleCallbackRepeating*>(data);
		bool done = self->call();
		if (done)
		{
			gIdleCallbacks.deleteFunction(onIdle, data);
			delete self;
		}
	}
	bool call()
	{
		return mCallable();
	}
private:
	bool_func_t mCallable;
};

void doOnIdleRepeating(bool_func_t callable)
{
	OnIdleCallbackRepeating* cb_functor = new OnIdleCallbackRepeating(callable);
	gIdleCallbacks.addFunction(&OnIdleCallbackRepeating::onIdle,cb_functor);
}

// ============================================================================
