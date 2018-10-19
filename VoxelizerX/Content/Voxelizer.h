//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#pragma once

#include "XSDXShader.h"
#include "XSDXState.h"
#include "XSDXResource.h"

#include "SharedConst.h"

class Voxelizer
{
public:
	enum Method				: uint32_t
	{
		TRI_PROJ,
		TRI_PROJ_TESS,
		TRI_PROJ_UNION
	};

	enum VertexShaderID		: uint32_t
	{
		VS_TRI_PROJ,
		VS_TRI_PROJ_TESS,
		VS_TRI_PROJ_UNION,
		VS_POINT_ARRAY,
		VS_BOX_ARRAY
	};

	enum HullShaderID		: uint32_t
	{
		HS_TRI_PROJ
	};

	enum DomainShaderID		: uint32_t
	{
		DS_TRI_PROJ
	};

	enum PixelShaderID		: uint32_t
	{
		PS_TRI_PROJ,
		PS_TRI_PROJ_UNION,
		PS_TRI_PROJ_SOLID,
		PS_TRI_PROJ_UNION_SOLID,
		PS_SIMPLE
	};

	enum ComputeShaderID	: uint32_t
	{
		CS_FILL_SOLID,
		CS_RAY_CAST
	};

	Voxelizer(const XSDX::CPDXDevice &pDXDevice, const XSDX::spShader &pShader, const XSDX::spState &pState);
	virtual ~Voxelizer();

	void Init(const uint32_t uWidth, const uint32_t uHeight, const char *szFileName = "Media\\bunny.obj");
	void UpdateFrame(DirectX::CXMVECTOR vEyePt, DirectX::CXMMATRIX mViewProj);
	void Render(const Method eVoxMethod = TRI_PROJ);
	void Render(const XSDX::CPDXUnorderedAccessView &pUAVSwapChain, const Method eVoxMethod = TRI_PROJ);

	static void CreateVertexLayout(const XSDX::CPDXDevice &pDXDevice, XSDX::CPDXInputLayout &pVertexLayout,
		const XSDX::spShader &pShader, const uint8_t uVS);

	static XSDX::CPDXInputLayout &GetVertexLayout();

protected:
	struct CBMatrices
	{
		DirectX::XMMATRIX mWorldViewProj;
		DirectX::XMMATRIX mWorld;
		DirectX::XMMATRIX mWorldIT;
	};

	struct CBPerFrame
	{
		DirectX::XMFLOAT4 vEyePos;
	};

	struct CBPerObject
	{
		DirectX::XMVECTOR vLocalSpaceLightPt;
		DirectX::XMVECTOR vLocalSpaceEyePt;
		DirectX::XMMATRIX mScreenToLocal;
	};

#if	USE_MUTEX
	struct upTexture3Ds
	{
		using ppTexture3D = std::add_pointer_t<XSDX::upTexture3D>;
		XSDX::upTexture3D x;
		XSDX::upTexture3D y;
		XSDX::upTexture3D z;
		XSDX::upTexture3D w;
		ppTexture3D array = &x;
	};
#endif

	void createVB(const uint32_t uNumVert, const uint32_t uStride, const uint8_t *pData);
	void createIB(const uint32_t uNumIndices, const uint32_t *pData);
	void createCBs();
	void voxelize(const Method eVoxMethod, const bool bDepthPeel = false, const uint8_t uMip = 0);
	void voxelizeSolid(const Method eVoxMethod, const uint8_t uMip = 0);
	void renderPointArray();
	void renderBoxArray();
	void renderRayCast(const XSDX::CPDXUnorderedAccessView &pUAVSwapChain);

	CBPerFrame						m_cbPerFrame;
	uint32_t						m_uVertexStride;
	uint32_t						m_uNumIndices;

	uint32_t						m_uNumLevels;

	DirectX::XMFLOAT4				m_vBound;
	DirectX::XMFLOAT2				m_vViewport;

	XSDX::upRawBuffer				m_pVB;
	XSDX::upRawBuffer				m_pIB;
	XSDX::CPDXBuffer				m_pCBMatrices;
	XSDX::CPDXBuffer				m_pCBPerFrame;
	XSDX::CPDXBuffer				m_pCBPerObject;
	XSDX::CPDXBuffer				m_pCBBound;
	XSDX::vCPDXBuffer				m_vpCBPerMipLevels;

#if	USE_MUTEX
	upTexture3Ds					m_pTxGrids;
	XSDX::upTexture3D				m_pTxUint;
#else
	XSDX::upTexture3D				m_pTxGrid;
#endif
	XSDX::upTexture2D				m_pTxKBufferDepth;

	XSDX::spShader					m_pShader;
	XSDX::spState					m_pState;

	XSDX::CPDXDevice				m_pDXDevice;
	XSDX::CPDXContext				m_pDXContext;

	static XSDX::CPDXInputLayout	m_pVertexLayout;
};

using upVoxelizer = std::unique_ptr<Voxelizer>;
using spVoxelizer = std::shared_ptr<Voxelizer>;
using vuVoxelizer = std::vector<upVoxelizer>;
using vpVoxelizer = std::vector<spVoxelizer>;
