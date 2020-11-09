// ============================================================================
//
// Sample3d.h
// Copyright Sony Online Entertainment
//
// ============================================================================

#ifndef INCLUDED_Sample3d_H
#define INCLUDED_Sample3d_H

#include "clientAudio/Audio.h"
#include "../../../../../../engine/shared/library/sharedFoundation/include/public/sharedFoundation/CrcString.h"

//-----------------------------------------------------------------------------
class Sample3d
{
public:

	enum
	{
		ChannelPlaying = 2,
		ChannelReady = 3
	};
	
	Sample3d();
	Sample3d(Sample3d const &sample3d);
	Sample3d & operator =(Sample3d const &rhs);
	~Sample3d();

	void                    setPath(char const *path, int const fileSize);
	CrcString const * const getPath() const;
	int                     getFileSize() const;
	int						getChannelStatus() const;

	FMOD::Sound*			mFmodSample;
	FMOD::Channel*			mFmodChannel;

	Sound2 *				m_sound;
	Audio::PlayBackStatus	m_status;

private:

	CrcString const *     m_path;
	int                   m_fileSize; // bytes
};

// ============================================================================

#endif // INCLUDED_Sample3d_H
