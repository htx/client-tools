#ifndef INCLUDED_Direct3d11_VertexShaderUtilities_H
#define INCLUDED_Direct3d11_VertexShaderUtilities_H

// ======================================================================

struct VectorXyzw
{
	float x;
	float y;
	float z;
	float w;
};

// ======================================================================

inline bool operator !=(VectorXyzw const & lhs, VectorXyzw const & rhs)
{
	return (lhs.x != rhs.x) || (lhs.y != rhs.y) || (lhs.z != rhs.z) || (lhs.w != rhs.w);
}

// ======================================================================

#endif
