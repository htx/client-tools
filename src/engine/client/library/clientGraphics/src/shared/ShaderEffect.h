// ======================================================================
//
// ShaderEffect.h
// copyright (c) 2001 Sony Online Entertainment
//
// ======================================================================

#ifndef INCLUDED_ShaderEffect_H
#define INCLUDED_ShaderEffect_H

// ======================================================================

class Iff;
class ShaderImplementation;
class StaticShaderTemplate;
class VertexBufferFormat;

#include "../../../../../../engine/shared/library/sharedFoundation/include/public/sharedFoundation/PersistentCrcString.h"

// ======================================================================

class ShaderEffect
{
	friend class Direct3d8_StaticShaderData;
	friend class Direct3d9_StaticShaderData;
	friend class Direct3d11_StaticShaderData;

public:

	ShaderEffect(CrcString const & fileName, Iff & iff);
	~ShaderEffect();

	CrcString const &            getCrcString() const;
	const char  *                getName() const;
	void                         fetch() const;
	void                         release() const;

	bool                         containsPrecalculatedVertexLighting() const;

	intptr_t                     getShaderImplementationSortKey() const;
	const ShaderImplementation * getActiveShaderImplementation() const;

private:

	/// Disabled.
	ShaderEffect();

	/// Disabled.
	ShaderEffect(const ShaderEffect &);

	/// Disabled.
	ShaderEffect &operator =(const ShaderEffect &);

private:

	void load(Iff &iff);
	void load_0000(Iff &iff);
	void load_0001(Iff &iff);

private:

	mutable int                  m_users;
	PersistentCrcString          m_name;
	ShaderImplementation const * m_implementation;
	bool                         m_containsPrecalculatedVertexLighting;
};

// ======================================================================

inline CrcString const &ShaderEffect::getCrcString() const
{
	return m_name;
}

// ----------------------------------------------------------------------

inline const char * ShaderEffect::getName() const
{
	return m_name.getString();
}

// ----------------------------------------------------------------------

inline bool ShaderEffect::containsPrecalculatedVertexLighting() const
{
	return m_containsPrecalculatedVertexLighting;
}

// ----------------------------------------------------------------------

inline intptr_t ShaderEffect::getShaderImplementationSortKey() const
{
	return reinterpret_cast<intptr_t>(m_implementation);
}

// ======================================================================

#endif
