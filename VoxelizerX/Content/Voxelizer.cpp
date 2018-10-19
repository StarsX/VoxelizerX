//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "ObjLoader.h"
#include "Voxelizer.h"

using namespace DirectX;
using namespace DX;
using namespace std;
using namespace XSDX;

const auto g_pNullSRV = static_cast<LPDXShaderResourceView>(nullptr);	// Helper to Clear SRVs
const auto g_pNullUAV = static_cast<LPDXUnorderedAccessView>(nullptr);	// Helper to Clear UAVs
const auto g_uNullUint = 0u;											// Helper to Clear Buffers

CPDXInputLayout	Voxelizer::m_pVertexLayout;

Voxelizer::Voxelizer(const CPDXDevice &pDXDevice, const spShader &pShader, const spState &pState) :
	m_pDXDevice(pDXDevice),
	m_pShader(pShader),
	m_pState(pState),
	m_uVertexStride(0),
	m_uNumIndices(0)
{
	m_pDXDevice->GetImmediateContext(&m_pDXContext);
}

Voxelizer::~Voxelizer()
{
}

void Voxelizer::Init(const uint32_t uWidth, const uint32_t uHeight, const char *szFileName)
{
	m_vViewport.x = static_cast<float>(uWidth);
	m_vViewport.y = static_cast<float>(uHeight);

	ObjLoader objLoader;
	if (!objLoader.Import(szFileName, true, true)) return;

	createVB(objLoader.GetNumVertices(), objLoader.GetVertexStride(), objLoader.GetVertices());
	createIB(objLoader.GetNumIndices(), objLoader.GetIndices());

	// Extract boundary
	const auto vCenter = objLoader.GetCenter();
	m_vBound = XMFLOAT4(vCenter.x, vCenter.y, vCenter.z, objLoader.GetRadius());

	m_uNumLevels = max(static_cast<uint32_t>(log2(GRID_SIZE)), 1);
	createCBs();

#if	USE_MUTEX
	for (auto i = 0ui8; i < 3; ++i)
	{
		m_pTxGrids.array[i] = make_unique<Texture3D>(m_pDXDevice);
		m_pTxGrids.array[i]->Create(GRID_SIZE, GRID_SIZE, GRID_SIZE, DXGI_FORMAT_R32_FLOAT);
	}
	m_pTxGrids.w = make_unique<Texture3D>(m_pDXDevice);
	m_pTxGrids.w->Create(GRID_SIZE, GRID_SIZE, GRID_SIZE, DXGI_FORMAT_R8_UNORM);

	m_pTxUint = make_unique<Texture3D>(m_pDXDevice);
	m_pTxUint->Create(GRID_SIZE, GRID_SIZE, GRID_SIZE, DXGI_FORMAT_R32_UINT);

#else
	m_pTxGrid = make_unique<Texture3D>(m_pDXDevice);
	m_pTxGrid->Create(GRID_SIZE, GRID_SIZE, GRID_SIZE, DXGI_FORMAT_R10G10B10A2_UNORM, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_PACKED_UAV);
#endif

	m_pTxKBufferDepth = make_unique<Texture2D>(m_pDXDevice);
	m_pTxKBufferDepth->Create(GRID_SIZE, GRID_SIZE, static_cast<uint32_t>(GRID_SIZE * DEPTH_SCALE), DXGI_FORMAT_R32_UINT);
}

void Voxelizer::UpdateFrame(DirectX::CXMVECTOR vEyePt, DirectX::CXMMATRIX mViewProj)
{
	// General matrices
	const auto mWorld = XMMatrixScaling(m_vBound.w, m_vBound.w, m_vBound.w) *
		XMMatrixTranslation(m_vBound.x, m_vBound.y, m_vBound.z);
	const auto mWorldI = XMMatrixInverse(nullptr, mWorld);
	const auto mWorldViewProj  = mWorld * mViewProj;
	CBMatrices cbMatrices =
	{
		XMMatrixTranspose(mWorldViewProj),
		XMMatrixTranspose(mWorld),
		XMMatrixIdentity()
	};
	
	// Screen space matrices
	CBPerObject cbPerObject;
	cbPerObject.vLocalSpaceLightPt = XMVector3TransformCoord(XMVectorSet(10.0f, 45.0f, 75.0f, 0.0f), mWorldI);
	cbPerObject.vLocalSpaceEyePt = XMVector3TransformCoord(vEyePt, mWorldI);

	const auto mToScreen = XMMATRIX
	(
		0.5f * m_vViewport.x, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f * m_vViewport.y, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f * m_vViewport.x, 0.5f * m_vViewport.y, 0.0f, 1.0f
	);
	const auto mLocalToScreen = XMMatrixMultiply(mWorldViewProj, mToScreen);
	const auto mScreenToLocal = XMMatrixInverse(nullptr, mLocalToScreen);
	cbPerObject.mScreenToLocal = XMMatrixTranspose(mScreenToLocal);
	
	// Per-frame data
	XMStoreFloat4(&m_cbPerFrame.vEyePos, vEyePt);

	if (m_pCBMatrices) m_pDXContext->UpdateSubresource(m_pCBMatrices.Get(), 0, nullptr, &cbMatrices, 0, 0);
	if (m_pCBPerFrame) m_pDXContext->UpdateSubresource(m_pCBPerFrame.Get(), 0, nullptr, &m_cbPerFrame, 0, 0);
	if (m_pCBPerObject) m_pDXContext->UpdateSubresource(m_pCBPerObject.Get(), 0, nullptr, &cbPerObject, 0, 0);
}

void Voxelizer::Render(const Method eVoxMethod)
{
	voxelize(eVoxMethod);

	renderBoxArray();
}

void Voxelizer::Render(const CPDXUnorderedAccessView &pUAVSwapChain, const Method eVoxMethod)
{
	voxelizeSolid(eVoxMethod);

	renderRayCast(pUAVSwapChain);
}

void Voxelizer::CreateVertexLayout(const CPDXDevice &pDXDevice, CPDXInputLayout &pVertexLayout, const spShader &pShader, const uint8_t uVS)
{
	// Define our vertex data layout for skinned objects
	const auto offset = D3D11_APPEND_ALIGNED_ELEMENT;
	const auto vLayout = vector<D3D11_INPUT_ELEMENT_DESC>
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0,		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, offset,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ThrowIfFailed(pDXDevice->CreateInputLayout(vLayout.data(), static_cast<uint32_t>(vLayout.size()),
		pShader->GetVertexShaderBuffer(uVS)->GetBufferPointer(),
		pShader->GetVertexShaderBuffer(uVS)->GetBufferSize(),
		&pVertexLayout));
}

CPDXInputLayout &Voxelizer::GetVertexLayout()
{
	return m_pVertexLayout;
}

void Voxelizer::createVB(const uint32_t uNumVert, const uint32_t uStride, const uint8_t *pData)
{
	m_uVertexStride = uStride;
	m_pVB = make_unique<RawBuffer>(m_pDXDevice);
	m_pVB->Create(uStride * uNumVert, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_SHADER_RESOURCE, pData);
}

void Voxelizer::createIB(const uint32_t uNumIndices, const uint32_t *pData)
{
	m_uNumIndices = uNumIndices;
	m_pIB = make_unique<RawBuffer>(m_pDXDevice);
	m_pIB->Create(sizeof(uint32_t) * uNumIndices, D3D11_BIND_INDEX_BUFFER | D3D11_BIND_SHADER_RESOURCE, pData);
}

void Voxelizer::createCBs()
{
	auto desc = CD3D11_BUFFER_DESC(sizeof(CBMatrices), D3D11_BIND_CONSTANT_BUFFER);
	ThrowIfFailed(m_pDXDevice->CreateBuffer(&desc, nullptr, &m_pCBMatrices));

	desc.ByteWidth = sizeof(CBPerFrame);
	ThrowIfFailed(m_pDXDevice->CreateBuffer(&desc, nullptr, &m_pCBPerFrame));

	desc.ByteWidth = sizeof(CBPerObject);
	ThrowIfFailed(m_pDXDevice->CreateBuffer(&desc, nullptr, &m_pCBPerObject));

	desc.ByteWidth = sizeof(XMFLOAT4);
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	D3D11_SUBRESOURCE_DATA subResData = { &m_vBound };
	ThrowIfFailed(m_pDXDevice->CreateBuffer(&desc, &subResData, &m_pCBBound));

	m_vpCBPerMipLevels.resize(m_uNumLevels);
	for (auto i = 0ui8; i < m_uNumLevels; ++i)
	{
		const auto fGridSize = static_cast<float>(GRID_SIZE >> i);
		subResData.pSysMem = &fGridSize;
		ThrowIfFailed(m_pDXDevice->CreateBuffer(&desc, &subResData, &m_vpCBPerMipLevels[i]));
	}
}

void Voxelizer::voxelize(const Method eVoxMethod, const bool bDepthPeel, const uint8_t uMip)
{
	auto pRTV = CPDXRenderTargetView();
	auto pDSV = CPDXDepthStencilView();
	m_pDXContext->OMGetRenderTargets(1, &pRTV, &pDSV);

	auto uNumViewports = 1u;
	auto vpBack = D3D11_VIEWPORT();
	m_pDXContext->RSGetViewports(&uNumViewports, &vpBack);

	const auto uOffset = 0u;
	const auto pUAVs =
	{
#if	USE_MUTEX
		m_pTxGrids.x->GetUAV(uMip).Get(),
		m_pTxGrids.y->GetUAV(uMip).Get(),
		m_pTxGrids.z->GetUAV(uMip).Get(),
		m_pTxGrids.w->GetUAV(uMip).Get(),
		m_pTxUint->GetUAV().Get(),
#else
		m_pTxGrid->GetUAV(uMip).Get(),
#endif
		m_pTxKBufferDepth->GetUAV().Get()
	};
	m_pDXContext->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr,
		0, static_cast<uint32_t>(pUAVs.size()), pUAVs.begin(), &g_uNullUint);

	const auto fGridSize = static_cast<float>(GRID_SIZE >> uMip);
	const auto vpSlice = CD3D11_VIEWPORT(0.0f, 0.0f, fGridSize, fGridSize);
	m_pDXContext->RSSetViewports(uNumViewports, &vpSlice);
	m_pDXContext->RSSetState(m_pState->CullNone().Get());

#if	USE_MUTEX
	for (auto i = 0ui8; i < 4; ++i)
		m_pDXContext->ClearUnorderedAccessViewFloat(m_pTxGrids.array[i]->GetUAV(uMip).Get(), DirectX::Colors::Transparent);
	m_pDXContext->ClearUnorderedAccessViewUint(m_pTxUint->GetUAV().Get(), XMVECTORU32{ 0 }.u);
#else
	m_pDXContext->ClearUnorderedAccessViewUint(m_pTxGrid->GetUAV(uMip).Get(), XMVECTORU32{ 0 }.u);
#endif
	if (bDepthPeel) m_pDXContext->ClearUnorderedAccessViewUint(m_pTxKBufferDepth->GetUAV().Get(), XMVECTORU32{ UINT32_MAX }.u);
	
	m_pDXContext->VSSetConstantBuffers(0, 1, m_pCBBound.GetAddressOf());

	const auto uPSTriProj = bDepthPeel ? PS_TRI_PROJ_SOLID : PS_TRI_PROJ;
	const auto uPSTriProjUnion = bDepthPeel ? PS_TRI_PROJ_UNION_SOLID : PS_TRI_PROJ_UNION;

	switch (eVoxMethod)
	{
	case TRI_PROJ_TESS:
		m_pDXContext->DSSetConstantBuffers(0, 1, m_vpCBPerMipLevels[uMip].GetAddressOf());
		m_pDXContext->PSSetConstantBuffers(0, 1, m_vpCBPerMipLevels[uMip].GetAddressOf());

		m_pDXContext->IASetInputLayout(m_pVertexLayout.Get());
		m_pDXContext->IASetVertexBuffers(0, 1, m_pVB->GetBuffer().GetAddressOf(), &m_uVertexStride, &uOffset);
		m_pDXContext->IASetIndexBuffer(m_pIB->GetBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
		m_pDXContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

		m_pDXContext->VSSetShader(m_pShader->GetVertexShader(VS_TRI_PROJ_TESS).Get(), nullptr, 0);
		m_pDXContext->HSSetShader(m_pShader->GetHullShader(HS_TRI_PROJ).Get(), nullptr, 0);
		m_pDXContext->DSSetShader(m_pShader->GetDomainShader(DS_TRI_PROJ).Get(), nullptr, 0);
		m_pDXContext->PSSetShader(m_pShader->GetPixelShader(uPSTriProj).Get(), nullptr, 0);

		m_pDXContext->DrawIndexed(m_uNumIndices, 0, 0);

		m_pDXContext->DSSetShader(nullptr, nullptr, 0);
		m_pDXContext->HSSetShader(nullptr, nullptr, 0);
		break;

	case TRI_PROJ_UNION:
		m_pDXContext->PSSetConstantBuffers(0, 1, m_vpCBPerMipLevels[uMip].GetAddressOf());

		m_pDXContext->IASetInputLayout(m_pVertexLayout.Get());
		m_pDXContext->IASetVertexBuffers(0, 1, m_pVB->GetBuffer().GetAddressOf(), &m_uVertexStride, &uOffset);
		m_pDXContext->IASetIndexBuffer(m_pIB->GetBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
		m_pDXContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_pDXContext->VSSetShader(m_pShader->GetVertexShader(VS_TRI_PROJ_UNION).Get(), nullptr, 0);
		m_pDXContext->PSSetShader(m_pShader->GetPixelShader(uPSTriProjUnion).Get(), nullptr, 0);

		m_pDXContext->DrawIndexedInstanced(m_uNumIndices, 3, 0, 0, 0);
		break;

	default:
		m_pDXContext->VSSetConstantBuffers(1, 1, m_vpCBPerMipLevels[uMip].GetAddressOf());
		m_pDXContext->PSSetConstantBuffers(0, 1, m_vpCBPerMipLevels[uMip].GetAddressOf());

		const auto pNullBuffer = static_cast<LPDXBuffer>(nullptr);
		m_pDXContext->IASetVertexBuffers(0, 1, &pNullBuffer, &g_uNullUint, &g_uNullUint);
		m_pDXContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_pDXContext->VSSetShader(m_pShader->GetVertexShader(VS_TRI_PROJ).Get(), nullptr, 0);
		m_pDXContext->PSSetShader(m_pShader->GetPixelShader(uPSTriProj).Get(), nullptr, 0);

		const auto pSRVs = { m_pIB->GetSRV().Get(), m_pVB->GetSRV().Get() };
		m_pDXContext->VSSetShaderResources(0, static_cast<uint32_t>(pSRVs.size()), pSRVs.begin());

		m_pDXContext->DrawInstanced(3, m_uNumIndices / 3, 0, 0);

		// Reset states
		const auto vpNullSRVs = vLPDXSRV(pSRVs.size(), nullptr);
		m_pDXContext->VSSetShaderResources(0, static_cast<uint32_t>(vpNullSRVs.size()), vpNullSRVs.data());
	}

	// Reset states
	m_pDXContext->IASetInputLayout(nullptr);
	m_pDXContext->RSSetState(nullptr);
	m_pDXContext->RSSetViewports(uNumViewports, &vpBack);
	m_pDXContext->OMSetRenderTargets(1, pRTV.GetAddressOf(), pDSV.Get());
}

void Voxelizer::voxelizeSolid(const Method eVoxMethod, const uint8_t uMip)
{
	voxelize(eVoxMethod, true, uMip);

	// Setup
#if	USE_MUTEX
	const auto pUAV = m_pTxGrids.w->GetUAV().Get();
#else
	const auto pUAV = m_pTxGrid->GetUAV().Get();
#endif
	const auto pSRVs =
	{
		m_pTxKBufferDepth->GetSRV().Get(),
#if	USE_MUTEX
		m_pTxGrids.x->GetSRV().Get(),
		m_pTxGrids.y->GetSRV().Get(),
		m_pTxGrids.z->GetSRV().Get()
#endif
	};
	m_pDXContext->CSSetUnorderedAccessViews(0, 1, &pUAV, &g_uNullUint);
	m_pDXContext->CSSetShaderResources(0, static_cast<uint32_t>(pSRVs.size()), pSRVs.begin());
	m_pDXContext->CSSetConstantBuffers(0, 1, m_vpCBPerMipLevels[uMip].GetAddressOf());

	// Dispatch
	m_pDXContext->CSSetShader(m_pShader->GetComputeShader(CS_FILL_SOLID).Get(), nullptr, 0);
	m_pDXContext->Dispatch(GRID_SIZE / 32, GRID_SIZE / 16, GRID_SIZE);

	// Unset
	const auto vpNullSRVs = vLPDXSRV(pSRVs.size(), nullptr);
	m_pDXContext->CSSetShaderResources(0, static_cast<uint32_t>(vpNullSRVs.size()), vpNullSRVs.data());
	m_pDXContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &g_uNullUint);
}

void Voxelizer::renderPointArray()
{
	const auto uOffset = 0u;
	const auto uGridSize = GRID_SIZE >> SHOW_MIP;

	//const auto pCBs = { m_pCBBound.Get(), m_pCBMatrices.Get() };
	m_pDXContext->VSSetConstantBuffers(0, 1, m_pCBMatrices.GetAddressOf());

	m_pDXContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	m_pDXContext->VSSetShader(m_pShader->GetVertexShader(VS_POINT_ARRAY).Get(), nullptr, 0);
	m_pDXContext->PSSetShader(m_pShader->GetPixelShader(PS_SIMPLE).Get(), nullptr, 0);

	const auto pSRVs =
	{
#if	USE_MUTEX
		m_pTxGrids.x->GetSRV().Get(),
		m_pTxGrids.y->GetSRV().Get(),
		m_pTxGrids.z->GetSRV().Get(),
		m_pTxGrids.w->GetSRV().Get()
#else
		m_pTxGrid->GetSRV().Get()
#endif
	};
	m_pDXContext->VSSetShaderResources(0, static_cast<uint32_t>(pSRVs.size()), pSRVs.begin());

	m_pDXContext->DrawInstanced(uGridSize * uGridSize, uGridSize, 0, 0);

	const auto vpNullSRVs = vLPDXSRV(pSRVs.size(), nullptr);
	m_pDXContext->VSSetShaderResources(0, static_cast<uint32_t>(vpNullSRVs.size()), vpNullSRVs.data());
}

void Voxelizer::renderBoxArray()
{
	const auto uOffset = 0u;
	const auto uGridSize = GRID_SIZE >> SHOW_MIP;

	m_pDXContext->VSSetConstantBuffers(0, 1, m_pCBMatrices.GetAddressOf());

	m_pDXContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	m_pDXContext->VSSetShader(m_pShader->GetVertexShader(VS_BOX_ARRAY).Get(), nullptr, 0);
	m_pDXContext->PSSetShader(m_pShader->GetPixelShader(PS_SIMPLE).Get(), nullptr, 0);

	const auto pSRVs =
	{
#if	USE_MUTEX
		m_pTxGrids.x->GetSRV().Get(),
		m_pTxGrids.y->GetSRV().Get(),
		m_pTxGrids.z->GetSRV().Get(),
		m_pTxGrids.w->GetSRV().Get()
#else
		m_pTxGrid->GetSRV().Get()
#endif
	};
	m_pDXContext->VSSetShaderResources(0, static_cast<uint32_t>(pSRVs.size()), pSRVs.begin());

	m_pDXContext->DrawInstanced(4, 6 * uGridSize * uGridSize * uGridSize, 0, 0);

	const auto vpNullSRVs = vLPDXSRV(pSRVs.size(), nullptr);
	m_pDXContext->VSSetShaderResources(0, static_cast<uint32_t>(vpNullSRVs.size()), vpNullSRVs.data());
}

void Voxelizer::renderRayCast(const CPDXUnorderedAccessView &pUAVSwapChain)
{
	auto pSrc = CPDXResource();
	auto desc = D3D11_TEXTURE2D_DESC();
	pUAVSwapChain->GetResource(&pSrc);
	static_cast<LPDXTexture2D>(pSrc.Get())->GetDesc(&desc);

	// Setup
#if	USE_MUTEX
	const auto pSRV = m_pTxGrids.w->GetSRV().Get();
#else
	const auto pSRV = m_pTxGrid->GetSRV().Get();
#endif
	m_pDXContext->CSSetUnorderedAccessViews(0, 1, pUAVSwapChain.GetAddressOf(), &g_uNullUint);
	m_pDXContext->CSSetShaderResources(0, 1, &pSRV);
	m_pDXContext->CSSetSamplers(0, 1, m_pState->LinearClamp().GetAddressOf());
	m_pDXContext->CSSetConstantBuffers(0, 1, m_pCBPerObject.GetAddressOf());

	// Dispatch
	m_pDXContext->CSSetShader(m_pShader->GetComputeShader(CS_RAY_CAST).Get(), nullptr, 0);
	m_pDXContext->Dispatch(desc.Width >> 5, desc.Height >> 5, 1);

	// Unset
	m_pDXContext->CSSetShaderResources(0, 1, &g_pNullSRV);
	m_pDXContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &g_uNullUint);
}
