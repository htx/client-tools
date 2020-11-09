// ======================================================================
//
// VideoList.cpp
//
// copyright (c) 2001 Sony Online Entertainment
//
// ======================================================================

#include "clientGraphics/FirstClientGraphics.h"
#include "clientGraphics/VideoList.h"

#include "clientGraphics/Video.h"
#include "sharedFoundation/ExitChain.h"
#include "sharedFoundation/StringCompare.h"
#include "sharedSynchronization/RecursiveMutex.h"

#include <map>

// ======================================================================

namespace VideoListNamespace
{
	// ------------------------------------------------------------------

	void _onVideoDelete(const Video *video);

	RecursiveMutex s_criticalSection;

	// ------------------------------------------------------------------

	typedef stdmap<const char *, Video *, StringCompare>::fwd VideoMap;

	// ------------------------------------------------------------------

	static bool        s_ready;
	static VideoMap   *s_videoMap;

	// ------------------------------------------------------------------

	static inline bool isInstalled()
	{
		return s_videoMap!=0;
	}

	static void remove();
}
using namespace VideoListNamespace;

// ======================================================================

void VideoList::install()
{
	bool binkInstalled = false;
	
	if (!binkInstalled)
	{
		return;
	}

	ExitChain::add(VideoListNamespace::remove, "VideoList");

	s_videoMap = new VideoMap;
}

// ----------------------------------------------------------------------

void VideoListNamespace::remove()
{
	DEBUG_FATAL(!s_videoMap->empty(), ("Videos still allocated"));
	delete s_videoMap;
	s_videoMap = NULL;
}
	
// ----------------------------------------------------------------------

Video *VideoList::fetch(const char *name)
{
	if (!name || !*name)
	{
		WARNING(true, ("Attempt to fetch video with empty name.\n"));
		return 0;
	}

	// ----------------------------------------------

	if (!isInstalled())
	{
		WARNING(true, ("Attempt to fetch video %s when VideoList is not installed.\n", name));
		return 0;
	}

	// ----------------------------------------------

	Video *result = NULL;

	s_criticalSection.enter();
	{
		// search for the file already being loaded
		VideoMap::iterator i = s_videoMap->find(name);
		
		if (i != s_videoMap->end())
		{
			// file was already loaded, so return it
			i->second->fetch();
		}
		else
		{
			result = nullptr;
			
			if (result)
			{
				{
					VideoMap::value_type entry(result->getName(), result);
					std::pair<VideoMap::iterator, bool> result2 = s_videoMap->insert(entry);
					DEBUG_FATAL(!result2.second, ("insert failed"));
				}

				result->fetch();
			}
		}
	}
	s_criticalSection.leave();

	return result;
}

// ----------------------------------------------------------------------

void VideoListNamespace::_onVideoDelete(const Video *video)
{
	s_criticalSection.enter();
	{
		const char *name = video->getName();
		if (name)
		{
			{
				VideoMap::iterator it = s_videoMap->find(name);
				DEBUG_FATAL(it == s_videoMap->end(), ("Could not find named video %s", name));
				if (it!=s_videoMap->end())
				{
					s_videoMap->erase(it);
				}
			}
		}
	}
	s_criticalSection.leave();
}

// ======================================================================
