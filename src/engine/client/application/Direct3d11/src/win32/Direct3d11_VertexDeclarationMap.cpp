#include "FirstDirect3d11.h"
#include "Direct3d11_VertexDeclarationMap.h"

#include <d3dcompiler.h>


#include "Direct3d11.h"
#include "Direct3d11_VertexBufferDescriptorMap.h"

#include "clientGraphics/VertexBufferFormat.h"
#include "clientGraphics/VertexBufferDescriptor.h"

#include <map>

// ======================================================================

namespace Direct3d11_VertexDeclarationMapNamespace
{
	class Key
	{
	public:

		Key(VertexBufferFormat const * const * vertexBufferFormat, int count);
		bool operator < (const Key &rhs) const;

	private:
	
		enum
		{
			MAX_VERTEX_BUFFERS = 2
		};

		uint32 m_key[MAX_VERTEX_BUFFERS];
	};

	typedef std::map<Key, ID3D11InputLayout *> VertexDeclarationMap;

	VertexDeclarationMap *ms_vertexDeclarationMap;
	D3D11_INPUT_ELEMENT_DESC     ms_vertexElements[32];
}
using namespace Direct3d11_VertexDeclarationMapNamespace;

// ======================================================================

Direct3d11_VertexDeclarationMapNamespace::Key::Key(VertexBufferFormat const * const * vertexBufferFormat, int count)
{
	DEBUG_FATAL(count > MAX_VERTEX_BUFFERS, ("Too many VB's in a VBVector %d/%d", count, MAX_VERTEX_BUFFERS));

	int i = 0;
	for (i = 0; i < count; ++i)
		m_key[i] = vertexBufferFormat[i]->getFlags();
	for ( ; i < MAX_VERTEX_BUFFERS; ++i)
		m_key[i] = 0;
}

// ----------------------------------------------------------------------

bool Direct3d11_VertexDeclarationMapNamespace::Key::operator < (const Key &rhs) const
{
	for (int i = 0; i < MAX_VERTEX_BUFFERS-1; ++i)
	{
		if (m_key[i] < rhs.m_key[i])
			return true;
		if (m_key[i] > rhs.m_key[i])
			return false;
	}

	return m_key[MAX_VERTEX_BUFFERS-1] < rhs.m_key[MAX_VERTEX_BUFFERS-1];
}

// ======================================================================

void Direct3d11_VertexDeclarationMap::install()
{
	ms_vertexDeclarationMap = new VertexDeclarationMap;
}

// ----------------------------------------------------------------------

void Direct3d11_VertexDeclarationMap::remove()
{
	if (ms_vertexDeclarationMap)
	{
		while (!ms_vertexDeclarationMap->empty())
		{
			ms_vertexDeclarationMap->begin()->second->Release();
			ms_vertexDeclarationMap->erase(ms_vertexDeclarationMap->begin());
		}

		delete ms_vertexDeclarationMap;
		ms_vertexDeclarationMap = nullptr;
	}
}

// ----------------------------------------------------------------------

ID3D11InputLayout *Direct3d11_VertexDeclarationMap::fetchVertexDeclaration(VertexBufferFormat const * const * vertexBufferFormats, int count)
{
	Key key(vertexBufferFormats, count);
	
	// search for the key
	VertexDeclarationMap::iterator f = ms_vertexDeclarationMap->find(key);
	
	// return it if found
	if (f != ms_vertexDeclarationMap->end())
	{
		ID3D11InputLayout *vertexDeclaration = f->second;
		vertexDeclaration->AddRef();
		return vertexDeclaration;
	}
	
	unsigned int vertexElement = 0;
	int textureCoordinate = 0;
	std::string dummyElementsStr;
	
	for (int i = 0; i < count; ++i)
	{
		VertexBufferFormat const & vertexBufferFormat = *vertexBufferFormats[i];
		VertexBufferDescriptor const & descriptor = Direct3d11_VertexBufferDescriptorMap::getDescriptor(vertexBufferFormat);
		
		if (vertexBufferFormat.isTransformed())
		{
			ms_vertexElements[vertexElement].InputSlot			= 0;//i;
			ms_vertexElements[vertexElement].AlignedByteOffset	= descriptor.offsetPosition;
			ms_vertexElements[vertexElement].Format				= DXGI_FORMAT_R32G32B32A32_FLOAT;
			ms_vertexElements[vertexElement].InputSlotClass		= D3D11_INPUT_PER_VERTEX_DATA;
			ms_vertexElements[vertexElement].SemanticName		= "POSITION";
			ms_vertexElements[vertexElement].SemanticIndex		= 0;
			
			++vertexElement;

			dummyElementsStr += "float4 position : POSITION0;\n";// : register(vPosition0)
		}
		else if (vertexBufferFormat.hasPosition())
		{
			ms_vertexElements[vertexElement].InputSlot			= 0;//i;
			ms_vertexElements[vertexElement].AlignedByteOffset	= descriptor.offsetPosition;
			ms_vertexElements[vertexElement].Format				= DXGI_FORMAT_R32G32B32_FLOAT;
			ms_vertexElements[vertexElement].InputSlotClass		= D3D11_INPUT_PER_VERTEX_DATA;
			ms_vertexElements[vertexElement].SemanticName		= "POSITION";
			ms_vertexElements[vertexElement].SemanticIndex		= 0;
			
			++vertexElement;

			dummyElementsStr += "float4 position : POSITION0;\n"; //: register(v0)
		}	

		if (vertexBufferFormat.hasNormal())
		{
			ms_vertexElements[vertexElement].InputSlot			= 0;//i;
			ms_vertexElements[vertexElement].AlignedByteOffset	= descriptor.offsetNormal;
			ms_vertexElements[vertexElement].Format				= DXGI_FORMAT_R32G32B32_FLOAT;
			ms_vertexElements[vertexElement].InputSlotClass		= D3D11_INPUT_PER_VERTEX_DATA;
			ms_vertexElements[vertexElement].SemanticName		= "NORMAL";
			ms_vertexElements[vertexElement].SemanticIndex		= 0;
			
			++vertexElement;

			dummyElementsStr += "float4 normal : NORMAL0;\n";//: register(v3)
		}

		if (vertexBufferFormat.hasPointSize())
		{
			ms_vertexElements[vertexElement].InputSlot				= 0;//i;
			ms_vertexElements[vertexElement].AlignedByteOffset		= descriptor.offsetPointSize;
			ms_vertexElements[vertexElement].Format					= DXGI_FORMAT_R32_FLOAT;
			ms_vertexElements[vertexElement].InputSlotClass			= D3D11_INPUT_PER_VERTEX_DATA;
			ms_vertexElements[vertexElement].SemanticName			= "PSIZE";
			ms_vertexElements[vertexElement].SemanticIndex			= 0;
			
			++vertexElement;

			dummyElementsStr += "float4 psize : PSIZE0;\n"; // : register(v4)
		}

		if (vertexBufferFormat.hasColor0())
		{
			ms_vertexElements[vertexElement].InputSlot				= 0;//i;
			ms_vertexElements[vertexElement].AlignedByteOffset      = descriptor.offsetColor0;
			ms_vertexElements[vertexElement].Format					= DXGI_FORMAT_R32G32B32_FLOAT;
			ms_vertexElements[vertexElement].InputSlotClass			= D3D11_INPUT_PER_VERTEX_DATA;
			ms_vertexElements[vertexElement].SemanticName			= "COLOR";
			ms_vertexElements[vertexElement].SemanticIndex			= 0;
			
			++vertexElement;

			dummyElementsStr += "float4 color0 : COLOR0;\n"; //: register(v5)
		}

		if (vertexBufferFormat.hasColor1())
		{
			ms_vertexElements[vertexElement].InputSlot				= 0;//i;
			ms_vertexElements[vertexElement].AlignedByteOffset      = descriptor.offsetColor1;
			ms_vertexElements[vertexElement].Format					= DXGI_FORMAT_R32G32B32_FLOAT;
			ms_vertexElements[vertexElement].InputSlotClass			= D3D11_INPUT_PER_VERTEX_DATA;
			ms_vertexElements[vertexElement].SemanticName			= "COLOR";
			ms_vertexElements[vertexElement].SemanticIndex			= 1;
			
			++vertexElement;

			dummyElementsStr += "float4 color1 : COLOR1;\n"; //: register(v6)
		}

		const int numberOfTextureCoordinateSets = vertexBufferFormat.getNumberOfTextureCoordinateSets();
		for (int j = 0; j < numberOfTextureCoordinateSets; ++j)
		{
			ms_vertexElements[vertexElement].InputSlot			= 0;//i;
			ms_vertexElements[vertexElement].AlignedByteOffset  = descriptor.offsetTextureCoordinateSet[j];

			switch (vertexBufferFormat.getTextureCoordinateSetDimension(j))
			{
				case 1:	ms_vertexElements[vertexElement].Format = DXGI_FORMAT_R32_FLOAT; dummyElementsStr += "float"; break;
				case 2:	ms_vertexElements[vertexElement].Format = DXGI_FORMAT_R32G32_FLOAT; dummyElementsStr += "float2"; break;
				case 3:	ms_vertexElements[vertexElement].Format = DXGI_FORMAT_R32G32B32_FLOAT; dummyElementsStr += "float3"; break;
				case 4:	ms_vertexElements[vertexElement].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; dummyElementsStr += "float4"; break;
			}

			ms_vertexElements[vertexElement].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			ms_vertexElements[vertexElement].SemanticName = "TEXCOORD";
			ms_vertexElements[vertexElement].SemanticIndex = textureCoordinate;

			dummyElementsStr += " txcset";
			dummyElementsStr += std::to_string(j);
			dummyElementsStr += " : TEXCOORD";
			dummyElementsStr += std::to_string(j);
			dummyElementsStr += ";\n";
			/*dummyElementsStr += " : register(v";
			dummyElementsStr += std::to_string((j + 7));
			dummyElementsStr +=");\n";*/
						
			++textureCoordinate;
			++vertexElement;
		}
	}

	DEBUG_FATAL(vertexElement == 0, ("No vertex elements defined"));
	DEBUG_FATAL(vertexElement > 31, ("Too many vertex elements defined"));
	
	ID3D11InputLayout *vertexDeclaration = nullptr;

	//=========================

	const char* shaderTemplate = " \
	struct InputVertex \
	{ \
		%s \
	}; \
	\
	struct OutputVertex \
	{ \
		float4 position : POSITION0; \
	}; \
	\
	OutputVertex main(InputVertex iv) \
	{ \
		OutputVertex o; \
		o.position = float4(0.0, 0.0, 0.0, 0.0); \
		return o; \
	} \
	";
	
	char shaderBuf[2024];
	sprintf(shaderBuf, shaderTemplate, dummyElementsStr.c_str());

	ID3DBlob *error = nullptr;
	ID3DBlob *compiledDummy = nullptr;
	LPTSTR errorText = nullptr;
	
	HRESULT hresult = D3DCompile(shaderBuf, strlen(shaderBuf), nullptr, nullptr, nullptr, "main", "vs_4_0_level_9_1", D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 0, &compiledDummy, &error);

	FATAL(FAILED(hresult), ("create dummyshader fail. %s", error->GetBufferPointer()));
	
	//=========================
		
	hresult = Direct3d11::getDevice()->CreateInputLayout(ms_vertexElements, vertexElement, compiledDummy->GetBufferPointer(), compiledDummy->GetBufferSize(), &vertexDeclaration);

	FATAL(FAILED(hresult), ("create input map fail."));

	NOT_NULL(vertexDeclaration);

	const bool inserted = ms_vertexDeclarationMap->insert(VertexDeclarationMap::value_type(key, vertexDeclaration)).second;
	UNREF(inserted);
	DEBUG_FATAL(!inserted, ("item already existed in map"));

	vertexDeclaration->AddRef();
	
	return vertexDeclaration;
}

// ----------------------------------------------------------------------

ID3D11InputLayout *Direct3d11_VertexDeclarationMap::fetchVertexDeclaration(VertexBufferFormat const &vertexBufferFormat)
{
	VertexBufferFormat const * const format[1] = { &vertexBufferFormat };
	return fetchVertexDeclaration(format, 1);
}

// ======================================================================
