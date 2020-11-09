// ============================================================================
//
// Sample2d.cpp
// Copyright Sony Online Entertainment
//
// ============================================================================

#include "clientAudio/FirstClientAudio.h"
#include "clientAudio/Sample2d.h"

// ============================================================================
//
// Sample2d
//
// ============================================================================

//-----------------------------------------------------------------------------
Sample2d::Sample2d()
 : mFmodSample(nullptr)
 , mFmodChannel(nullptr)
 , m_sound(nullptr)
 , m_status(Audio::PS_notStarted)
 , m_path(nullptr)
{
}

//-----------------------------------------------------------------------------

Sample2d::Sample2d(Sample2d const &sample2d)
 : mFmodSample(sample2d.mFmodSample)
 , mFmodChannel(sample2d.mFmodChannel)
 , m_sound(sample2d.m_sound)
 , m_status(sample2d.m_status)
 , m_path(nullptr)
{
	setPath(sample2d.getPath()->getString());
}

//-----------------------------------------------------------------------------

Sample2d & Sample2d::operator =(Sample2d const &rhs)
{
	if (this != &rhs)
	{
		setPath(rhs.getPath()->getString());

		mFmodSample = rhs.mFmodSample;
		mFmodChannel = rhs.mFmodChannel;
		m_sound = rhs.m_sound;
		m_status = rhs.m_status;
	}

	return *this;
}

//-----------------------------------------------------------------------------

Sample2d::~Sample2d()
{
	if (m_path != nullptr)
	{
		Audio::decreaseReferenceCount(*m_path);
	}

	m_path = nullptr;
	mFmodSample = nullptr;
	mFmodChannel = nullptr;
	m_sound = nullptr;
	m_status = Audio::PS_notStarted;
}

//-----------------------------------------------------------------------------

void Sample2d::setPath(char const *path)
{
	if (m_path != nullptr)
	{
		Audio::decreaseReferenceCount(*m_path);
	}

	bool const cacheSample = false;

	m_path = Audio::increaseReferenceCount(path, cacheSample);
}

//-----------------------------------------------------------------------------

CrcString const * const Sample2d::getPath() const
{
	NOT_NULL(m_path);

	return m_path;
}

//-----------------------------------------------------------------------------

int Sample2d::getChannelStatus() const
{
	if(mFmodChannel == nullptr )
	{
	    return ChannelReady;
	}

	bool playing = false;
	const FMOD_RESULT fr = mFmodChannel->isPlaying(&playing);
	
	if(fr == FMOD_OK && playing) 
	{
		return ChannelPlaying;
	}
	
    return ChannelReady;
}

// ============================================================================
