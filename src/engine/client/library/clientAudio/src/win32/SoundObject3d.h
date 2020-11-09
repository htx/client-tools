// ============================================================================
//
// SoundObject3d.h
// Copyright Sony Online Entertainment
//
// ============================================================================

#ifndef INCLUDED_SoundObject3d_H
#define INCLUDED_SoundObject3d_H

#include "sharedMath/Vector.h"

//-----------------------------------------------------------------------------
class SoundObject3d
{
public:

	SoundObject3d();

	void alter();

	FMOD::System* mFmodCoreSystem;
	
	Vector		m_positionCurrent;
	Vector		m_vectorForward;
	Vector		m_vectorUp;

private:

	Vector     m_positionPrevious;
};

// ============================================================================

#endif // INCLUDED_SoundObject3d_H
