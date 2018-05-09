//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SharedConst.h"
#include "ObjLoader.h"
#include "SurfaceVoxelizer.h"

using namespace DirectX;
using namespace DX;
using namespace std;
using namespace XSDX;

const auto g_pNullSRV = static_cast<LPDXShaderResourceView>(nullptr);	// Helper to Clear SRVs
const auto g_pNullUAV = static_cast<LPDXUnorderedAccessView>(nullptr);	// Helper to Clear UAVs
const auto g_uNullUint = 0u;											// Helper to Clear Buffers

CPDXInputLayout	SurfaceVoxelizer::m_pVertexLayout;

SurfaceVoxelizer::SurfaceVoxelizer(const CPDXDevice &pDXDevice, const spShader &pShader, const spState &pState) :
	m_pDXDevice(pDXDevice),
	m_pShader(pShader),
	m_pState(pState),
	m_uVertexStride(0),
	m_uNumIndices(0)
{
	m_pDXDevice->GetImmediateContext(&m_pDXContext);
}

SurfaceVoxelizer::~SurfaceVoxelizer()
{
}

void SurfaceVoxelizer::Init(const uint32_t uWidth, const uint32_t uHeight, const char *szFileName)
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
	createCBs();

	m_uNumLevels = max(static_cast<uint32_t>(log2(GRID_SIZE)), 1);
	m_pTxGrid = make_unique<Texture3D>(m_pDXDevice);
	m_pTxGrid->Create(GRID_SIZE, GRID_SIZE, GRID_SIZE, DXGI_FORMAT_R16G16B16A16_FLOAT, true, true, false, m_uNumLevels);

	m_pTxMutex = make_unique<Texture3D>(m_pDXDevice);
	m_pTxMutex->Create(GRID_SIZE, GRID_SIZE, GRID_SIZE, DXGI_FORMAT_R32_UINT);

	// Viewport
	m_VpSlice = CD3D11_VIEWPORT(0.0f, 0.0f, GRID_SIZE, GRID_SIZE);
}

void SurfaceVoxelizer::UpdateFrame(DirectX::CXMVECTOR vEyePt, DirectX::CXMMATRIX mViewProj)
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

void SurfaceVoxelizer::Render(const bool bTess)
{
	voxelize(bTess);

	renderBoxArray();
}

void SurfaceVoxelizer::Render(const CPDXUnorderedAccessView &pUAVSwapChain, const bool bTess)
{
	voxelizeSolid(bTess);

	renderRayCast(pUAVSwapChain);
}

void SurfaceVoxelizer::CreateVertexLayout(const CPDXDevice &pDXDevice, CPDXInputLayout &pVertexLayout, const spShader &pShader, const uint8_t uVS)
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

CPDXInputLayout &SurfaceVoxelizer::GetVertexLayout()
{
	return m_pVertexLayout;
}

void SurfaceVoxelizer::createVB(const uint32_t uNumVert, const uint32_t uStride, const uint8_t *pData)
{
	m_uVertexStride = uStride;
	const auto desc = CD3D11_BUFFER_DESC(uStride * uNumVert, D3D11_BIND_VERTEX_BUFFER);
	D3D11_SUBRESOURCE_DATA ssd = { pData };

	ThrowIfFailed(m_pDXDevice->CreateBuffer(&desc, &ssd, &m_pVB));
}

void SurfaceVoxelizer::createIB(const uint32_t uNumIndices, const uint32_t * pData)
{
	m_uNumIndices = uNumIndices;
	const auto desc = CD3D11_BUFFER_DESC(sizeof(uint32_t) * uNumIndices, D3D11_BIND_INDEX_BUFFER);
	D3D11_SUBRESOURCE_DATA ssd = { pData };

	ThrowIfFailed(m_pDXDevice->CreateBuffer(&desc, &ssd, &m_pIB));
}

void SurfaceVoxelizer::createCBs()
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
}

void SurfaceVoxelizer::voxelize(const bool bTess)
{
	auto pRTV = CPDXRenderTargetView();
	auto pDSV = CPDXDepthStencilView();
	m_pDXContext->OMGetRenderTargets(1, &pRTV, &pDSV);

	auto uNumViewports = 1u;
	auto vpBack = D3D11_VIEWPORT();
	m_pDXContext->RSGetViewports(&uNumViewports, &vpBack);

	const auto uOffset = 0u;
	const auto uInitCount = 0u;
	LPDXUnorderedAccessView ppUAVs[] = { m_pTxGrid->GetUAV().Get(), m_pTxMutex->GetUAV().Get() };
	m_pDXContext->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr,
		0, 2, ppUAVs, &uInitCount);
	m_pDXContext->RSSetViewports(uNumViewports, &m_VpSlice);
	m_pDXContext->RSSetState(m_pState->CullNone().Get());

	m_pDXContext->ClearUnorderedAccessViewFloat(m_pTxGrid->GetUAV().Get(), DirectX::Colors::Transparent);
	m_pDXContext->ClearUnorderedAccessViewUint(m_pTxMutex->GetUAV().Get(), XMVECTORU32{ 0 }.u);
	
	m_pDXContext->VSSetConstantBuffers(0, 1, m_pCBBound.GetAddressOf());
	
	m_pDXContext->IASetInputLayout(m_pVertexLayout.Get());
	m_pDXContext->IASetVertexBuffers(0, 1, m_pVB.GetAddressOf(), &m_uVertexStride, &uOffset);
	m_pDXContext->IASetIndexBuffer(m_pIB.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_pDXContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

	if (bTess)
	{
		m_pDXContext->VSSetShader(m_pShader->GetVertexShader(VS_TRI_PROJ_TESS).Get(), nullptr, 0);
		m_pDXContext->HSSetShader(m_pShader->GetHullShader(HS_TRI_PROJ).Get(), nullptr, 0);
		m_pDXContext->DSSetShader(m_pShader->GetDomainShader(DS_TRI_PROJ).Get(), nullptr, 0);
		m_pDXContext->PSSetShader(m_pShader->GetPixelShader(PS_TRI_PROJ_TESS).Get(), nullptr, 0);

		m_pDXContext->DrawIndexed(m_uNumIndices, 0, 0);
	}
	else
	{
		m_pDXContext->VSSetShader(m_pShader->GetVertexShader(VS_TRI_PROJ).Get(), nullptr, 0);
		m_pDXContext->PSSetShader(m_pShader->GetPixelShader(PS_TRI_PROJ).Get(), nullptr, 0);

		m_pDXContext->DrawIndexedInstanced(m_uNumIndices, 3, 0, 0, 0);
	}

	// Reset states
	m_pDXContext->DSSetShader(nullptr, nullptr, 0);
	m_pDXContext->HSSetShader(nullptr, nullptr, 0);
	m_pDXContext->IASetInputLayout(nullptr);
	m_pDXContext->RSSetState(nullptr);
	m_pDXContext->RSSetViewports(uNumViewports, &vpBack);
	m_pDXContext->OMSetRenderTargets(1, pRTV.GetAddressOf(), pDSV.Get());
}

void SurfaceVoxelizer::voxelizeSolid(const bool bTess)
{
	static auto t = 0u;
	voxelize(bTess);

	const auto uIter = m_uNumLevels - 1;
	for (auto i = 0u; i < uIter; ++i) downSample(i);
	if (t >= 2) for (auto i = uIter; i > 0; --i) fillSolid(i);
	t = (t + 1) % 4;
}

void SurfaceVoxelizer::downSample(const uint32_t i)
{
	// Setup
	m_pDXContext->CSSetUnorderedAccessViews(0, 1, m_pTxGrid->GetUAV(i).GetAddressOf(), &g_uNullUint);
	m_pDXContext->CSSetUnorderedAccessViews(1, 1, m_pTxGrid->GetUAV(i + 1).GetAddressOf(), &g_uNullUint);
	m_pDXContext->CSSetConstantBuffers(0, 1, m_pCBPerObject.GetAddressOf());

	// Dispatch
	m_pDXContext->ClearUnorderedAccessViewFloat(m_pTxGrid->GetUAV(i + 1).Get(), DirectX::Colors::Transparent);
	m_pDXContext->CSSetShader(m_pShader->GetComputeShader(CS_DOWN_SAMPLE).Get(), nullptr, 0);
	m_pDXContext->Dispatch(GRID_SIZE >> i >> 1, GRID_SIZE >> i >> 1, GRID_SIZE >> i >> 1);

	// Unset
	m_pDXContext->CSSetUnorderedAccessViews(1, 1, &g_pNullUAV, &g_uNullUint);
	m_pDXContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &g_uNullUint);
}

void SurfaceVoxelizer::fillSolid(const uint32_t i)
{
	// Setup
	m_pDXContext->CSSetUnorderedAccessViews(0, 1, m_pTxGrid->GetUAV(i).GetAddressOf(), &g_uNullUint);
	m_pDXContext->CSSetUnorderedAccessViews(1, 1, m_pTxGrid->GetUAV(i - 1).GetAddressOf(), &g_uNullUint);
	m_pDXContext->CSSetConstantBuffers(0, 1, m_pCBPerObject.GetAddressOf());

	// Dispatch
	m_pDXContext->CSSetShader(m_pShader->GetComputeShader(CS_FILL_SOLID).Get(), nullptr, 0);
	m_pDXContext->Dispatch(GRID_SIZE >> i, GRID_SIZE >> i, GRID_SIZE >> i);

	// Unset
	m_pDXContext->CSSetUnorderedAccessViews(1, 1, &g_pNullUAV, &g_uNullUint);
	m_pDXContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &g_uNullUint);
}

void SurfaceVoxelizer::renderPointArray()
{
	const auto uOffset = 0u;
	const auto uGridSize = GRID_SIZE >> SHOW_MIP;

	//const LPDXBuffer cbs[] = { m_pCBBound.Get(), m_pCBMatrices.Get() };
	m_pDXContext->VSSetConstantBuffers(0, 1, m_pCBMatrices.GetAddressOf());

	m_pDXContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	m_pDXContext->VSSetShader(m_pShader->GetVertexShader(VS_POINT_ARRAY).Get(), nullptr, 0);
	m_pDXContext->PSSetShader(m_pShader->GetPixelShader(PS_SIMPLE).Get(), nullptr, 0);

	m_pDXContext->VSSetShaderResources(0, 1, m_pTxGrid->GetSRV().GetAddressOf());

	m_pDXContext->DrawInstanced(uGridSize * uGridSize, uGridSize, 0, 0);

	m_pDXContext->VSSetShaderResources(0, 1, &g_pNullSRV);
}

void SurfaceVoxelizer::renderBoxArray()
{
	const auto uOffset = 0u;
	const auto uGridSize = GRID_SIZE >> SHOW_MIP;

	m_pDXContext->VSSetConstantBuffers(0, 1, m_pCBMatrices.GetAddressOf());

	m_pDXContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	m_pDXContext->VSSetShader(m_pShader->GetVertexShader(VS_BOX_ARRAY).Get(), nullptr, 0);
	m_pDXContext->PSSetShader(m_pShader->GetPixelShader(PS_SIMPLE).Get(), nullptr, 0);

	m_pDXContext->VSSetShaderResources(0, 1, m_pTxGrid->GetSRV().GetAddressOf());

	m_pDXContext->DrawInstanced(4, 6 * uGridSize * uGridSize * uGridSize, 0, 0);

	m_pDXContext->VSSetShaderResources(0, 1, &g_pNullSRV);
}

void SurfaceVoxelizer::renderRayCast(const CPDXUnorderedAccessView &pUAVSwapChain)
{
	auto pSrc = CPDXResource();
	auto desc = D3D11_TEXTURE2D_DESC();
	pUAVSwapChain->GetResource(&pSrc);
	static_cast<LPDXTexture2D>(pSrc.Get())->GetDesc(&desc);

	// Setup
	m_pDXContext->CSSetUnorderedAccessViews(0, 1, pUAVSwapChain.GetAddressOf(), &g_uNullUint);
	m_pDXContext->CSSetShaderResources(0, 1, m_pTxGrid->GetSRV().GetAddressOf());
	//m_pDXContext->CSSetSamplers(0, 1, m_pState->LinearClamp().GetAddressOf());
	m_pDXContext->CSSetConstantBuffers(0, 1, m_pCBPerObject.GetAddressOf());

	// Dispatch
	m_pDXContext->CSSetShader(m_pShader->GetComputeShader(CS_RAY_CAST).Get(), nullptr, 0);
	m_pDXContext->Dispatch(desc.Width >> 5, desc.Height >> 5, 1);

	// Unset
	m_pDXContext->CSSetShaderResources(0, 1, &g_pNullSRV);
	m_pDXContext->CSSetUnorderedAccessViews(0, 1, &g_pNullUAV, &g_uNullUint);
}
