// ============================================================================
//
// SoundObject3d.cpp
// Copyright Sony Online Entertainment
//
// ============================================================================

#include "clientAudio/FirstClientAudio.h"
#include "clientAudio/SoundObject3d.h"

// ============================================================================
//
// SoundObject3d
//
// ============================================================================

//-----------------------------------------------------------------------------
SoundObject3d::SoundObject3d()
 : mFmodCoreSystem(nullptr)
 , m_positionCurrent(Vector::zero)
 , m_vectorForward(Vector::unitZ)
 , m_vectorUp(Vector::unitY)
 , m_positionPrevious(Vector::zero)
{
}

//-----------------------------------------------------------------------------
void SoundObject3d::alter()
{
	m_positionPrevious = m_positionCurrent;

	NOT_NULL(mFmodCoreSystem);

	FMOD_VECTOR pos = { m_positionCurrent.x, m_positionCurrent.y, m_positionCurrent.z };
	FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
	FMOD_VECTOR fw = { m_vectorForward.x, m_vectorForward.y, m_vectorForward.z };
	FMOD_VECTOR up = { m_vectorUp.x, m_vectorUp.y, m_vectorUp.z };
		
	mFmodCoreSystem->set3DListenerAttributes(0, &pos, &vel, &fw, &up);
}

// ============================================================================
