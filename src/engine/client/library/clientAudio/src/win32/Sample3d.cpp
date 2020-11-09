// ============================================================================
//
// Sample3d.cpp
// Copyright Sony Online Entertainment
//
// ============================================================================

#include "clientAudio/FirstClientAudio.h"
#include "clientAudio/Sample3d.h"

// ============================================================================
//
// Sample3d
//
// ============================================================================

//-----------------------------------------------------------------------------

Sample3d::Sample3d()
 : mFmodSample(nullptr)
 , mFmodChannel(nullptr)
 , m_sound(nullptr)
 , m_status(Audio::PS_notStarted)
 , m_path(nullptr)
 , m_fileSize(0)
{
}

//-----------------------------------------------------------------------------

Sample3d::Sample3d(Sample3d const &sample3d)
 : mFmodSample(sample3d.mFmodSample)
 , mFmodChannel(sample3d.mFmodChannel)
 , m_sound(sample3d.m_sound)
 , m_status(sample3d.m_status)
 , m_path(nullptr)
 , m_fileSize(sample3d.m_fileSize)
{
	setPath(sample3d.getPath()->getString(), sample3d.m_fileSize);
}

//-----------------------------------------------------------------------------

Sample3d & Sample3d::operator =(Sample3d const &rhs)
{
	if (this != &rhs)
	{
		setPath(rhs.getPath()->getString(), rhs.m_fileSize);

		mFmodSample = rhs.mFmodSample;
		mFmodChannel = rhs.mFmodChannel;
		m_sound = rhs.m_sound;
		m_status = rhs.m_status;
		m_fileSize = rhs.m_fileSize;
	}

	return *this;
}

//-----------------------------------------------------------------------------

Sample3d::~Sample3d()
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
	m_fileSize = 0;
}

//-----------------------------------------------------------------------------

void Sample3d::setPath(char const *path, int const fileSize)
{
	if (m_path != nullptr)
	{
		Audio::decreaseReferenceCount(*m_path);
	}

	bool const cacheSample = true;

	m_path = Audio::increaseReferenceCount(path, cacheSample);
	m_fileSize = fileSize;
}

//-----------------------------------------------------------------------------

CrcString const * const Sample3d::getPath() const
{
	NOT_NULL(m_path);

	return m_path;
}

//-----------------------------------------------------------------------------

int Sample3d::getFileSize() const
{
	return m_fileSize;
}

//-----------------------------------------------------------------------------

int Sample3d::getChannelStatus() const
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
