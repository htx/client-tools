// ============================================================================
//
// SampleStream.cpp
// Copyright Sony Online Entertainment
//
// ============================================================================

#include "clientAudio/FirstClientAudio.h"
#include "clientAudio/SampleStream.h"

// ============================================================================
//
// SampleStream
//
// ============================================================================

//-----------------------------------------------------------------------------
SampleStream::SampleStream()
 : mFmodStream(nullptr)
 , mFmodChannel(nullptr)
 , m_sound(nullptr)
 , m_status(Audio::PS_notStarted)
 , m_path(nullptr)
{
}

//-----------------------------------------------------------------------------
SampleStream::SampleStream(SampleStream const &sampleStream)
 : mFmodStream(sampleStream.mFmodStream)
 , mFmodChannel(sampleStream.mFmodChannel)
 , m_sound(sampleStream.m_sound)
 , m_status(sampleStream.m_status)
 , m_path(nullptr)
{
	setPath(sampleStream.getPath()->getString());
}

//-----------------------------------------------------------------------------
SampleStream & SampleStream::operator =(SampleStream const &rhs)
{
	if (this != &rhs)
	{
		setPath(rhs.getPath()->getString());

		mFmodStream = rhs.mFmodStream;
		mFmodChannel = rhs.mFmodChannel;
		m_sound = rhs.m_sound;
		m_status = rhs.m_status;
	}

	return *this;
}

//-----------------------------------------------------------------------------
SampleStream::~SampleStream()
{
	if (m_path != nullptr)
	{
		Audio::decreaseReferenceCount(*m_path);
	}

	m_path = nullptr;
	mFmodStream = nullptr;
	mFmodChannel = nullptr;
	m_sound = nullptr;
	m_status = Audio::PS_notStarted;
}

//-----------------------------------------------------------------------------

void SampleStream::setPath(char const *path)
{
	if (m_path != nullptr)
	{
		Audio::decreaseReferenceCount(*m_path);
	}

	bool const cacheSample = false;

	m_path = Audio::increaseReferenceCount(path, cacheSample);
}

//-----------------------------------------------------------------------------

CrcString const * const SampleStream::getPath() const
{
	NOT_NULL(m_path);

	return m_path;
}

//-----------------------------------------------------------------------------

int SampleStream::getChannelStatus() const
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
