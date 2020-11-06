#ifndef INCLUDED_Direct3d11_RenderTarget_H
#define INCLUDED_Direct3d11_RenderTarget_H

// ======================================================================

class Texture;
#include "clientGraphics/Texture.def"

// ======================================================================

class Direct3d11_RenderTarget
{
public:

	static void install();
	static void remove();

	static void setRenderTargetToPrimary();
	static void setRenderTarget(Texture *texture, CubeFace cubeFace, int mipmapLevel);
	static bool copyRenderTargetToNonRenderTargetTexture();

	static void lostDevice();
	static void restoreDevice();
};

// ======================================================================

#endif
