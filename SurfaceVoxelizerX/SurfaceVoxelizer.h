#pragma once

#include "XSDXShader.h"
#include "XSDXState.h"
#include "XSDXResource.h"

class SurfaceVoxelizer
{
public:
	enum VertexShaderID		: uint32_t
	{
		VS_TRI_PROJ_TESS,
		VS_TRI_PROJ,
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
		PS_TRI_PROJ_TESS,
		PS_TRI_PROJ,
		PS_SIMPLE
	};

	enum ComputeShaderID	: uint32_t
	{
		CS_DOWN_SAMPLE,
		CS_FILL_SOLID,
		CS_RAY_CAST
	};

	SurfaceVoxelizer(const XSDX::CPDXDevice &pDXDevice, const XSDX::spShader &pShader, const XSDX::spState &pState);
	virtual ~SurfaceVoxelizer();

	void Init(const uint32_t uWidth, const uint32_t uHeight, const char *szFileName = "Media\\bunny.obj");
	void UpdateFrame(DirectX::CXMVECTOR vEyePt, DirectX::CXMMATRIX mViewProj);
	void Render(const bool bTess = true);
	void Render(const XSDX::CPDXUnorderedAccessView &pUAVSwapChain, const bool bTess = true);

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

	struct upTexture3Ds
	{
		using ppTexture3D = std::add_pointer_t<XSDX::upTexture3D>;
		XSDX::upTexture3D x;
		XSDX::upTexture3D y;
		XSDX::upTexture3D z;
		XSDX::upTexture3D w;
		ppTexture3D array = &x;
	};

	void createVB(const uint32_t uNumVert, const uint32_t uStride, const uint8_t *pData);
	void createIB(const uint32_t uNumIndices, const uint32_t *pData);
	void createCBs();
	void voxelize(const bool bTess, const uint8_t uMip = 0);
	void voxelizeSolid(const bool bTess);
	void downSample(const uint32_t i);
	void fillSolid(const uint32_t i);
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

	XSDX::upStructuredBuffer		m_pVBTransformed;

	upTexture3Ds					m_pTxGrids;
	XSDX::upTexture3D				m_pTxMutex;

	XSDX::spShader					m_pShader;
	XSDX::spState					m_pState;

	XSDX::CPDXDevice				m_pDXDevice;
	XSDX::CPDXContext				m_pDXContext;

	static XSDX::CPDXInputLayout	m_pVertexLayout;
};

using upSurfaceVoxelizer = std::unique_ptr<SurfaceVoxelizer>;
using spSurfaceVoxelizer = std::shared_ptr<SurfaceVoxelizer>;
using vuSurfaceVoxelizer = std::vector<upSurfaceVoxelizer>;
using vpSurfaceVoxelizer = std::vector<spSurfaceVoxelizer>;
