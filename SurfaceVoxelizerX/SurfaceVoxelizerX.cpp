// SurfaceVoxelizerX.cpp : Defines the entry point for the application.
//

//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#include "SurfaceVoxelizerX.h"

using namespace std;
using namespace Concurrency;
using namespace DirectX;
using namespace DX;
using namespace XSDX;

using upCDXUTTextHelper = unique_ptr<CDXUTTextHelper>;

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager		g_DialogResourceManager;	// manager for shared resources of dialogs
CModelViewerCamera				g_Camera;					// A model viewing camera
// CD3DSettingsDlg				g_D3DSettingsDlg;			// Device settings dialog
// CDXUTDialog					g_HUD;						// manages the 3D   
CDXUTDialog						g_SampleUI;					// dialog for sample specific controls
auto							g_bTypedUAVLoad = false;
auto							g_bShowHelp = false;		// If true, it renders the UI control text
auto							g_bShowFPS = true;			// If true, it shows the FPS
auto							g_bLoadingComplete = false;

auto							g_bRayCast = false;

upCDXUTTextHelper				g_pTxtHelper;

upSurfaceVoxelizer				g_pSurfaceVoxelizer;

spShader						g_pShader;
spState							g_pState;

IDXGISwapChain*					g_pSwapChain = nullptr;

SurfaceVoxelizer::Method		g_eVoxMethod = SurfaceVoxelizer::TRI_PROJ;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
enum ButtonID
{
	IDC_TOGGLEFULLSCREEN = 1,
	IDC_TOGGLEREF,
	IDC_CHANGEDEVICE,
	IDC_TOGGLEWARP,
	IDC_TRI_PROJ = 5,
	IDC_TRI_PROJ_TESS,
	IDC_TRI_PROJ_UNION,
	IDC_BOX_ARRAY,
	IDC_RAY_CAST
};

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext);
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
	void* pUserContext);
void CALLBACK OnMouse(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
	bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
	int xPos, int yPos, void *pUserContext);
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext);
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);

bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext);
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext);
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext);
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext);
void CALLBACK OnD3D11DestroyDevice(void* pUserContext);
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
	float fElapsedTime, void* pUserContext);

void InitApp();
void RenderText();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	AllocConsole();
	FILE *stream;
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
	freopen_s(&stream, "CONIN$", "r+t", stdin);
#endif

	// DXUT will create and use the best device
	// that is available on the system depending on which D3D callbacks are set below

	// Set DXUT callbacks
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackMouse(OnMouse, true);
	DXUTSetCallbackKeyboard(OnKeyboard);
	DXUTSetCallbackFrameMove(OnFrameMove);

	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);

	InitApp();
	DXUTInit(true, true, nullptr); // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings(false, true); // Show the cursor and clip it when in full screen
	DXUTCreateWindow(L"Surface Voxelizer");

	auto deviceSettings = DXUTDeviceSettings();
	DXUTApplyDefaultDeviceSettings(&deviceSettings);
	deviceSettings.MinimumFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	deviceSettings.d3d11.AutoCreateDepthStencil = true;
	// UAV cannot be DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
	deviceSettings.d3d11.sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	deviceSettings.d3d11.sd.BufferDesc.Width = 1280;
	deviceSettings.d3d11.sd.BufferDesc.Height = 960;
	deviceSettings.d3d11.sd.Windowed = true;
	deviceSettings.d3d11.sd.BufferUsage |= DXGI_USAGE_UNORDERED_ACCESS;
	deviceSettings.d3d11.DriverType = D3D_DRIVER_TYPE_WARP;

	DXUTCreateDeviceFromSettings(&deviceSettings);
	//DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, 1280, 960);
	DXUTMainLoop(); // Enter into the DXUT render loop

#if defined(DEBUG) | defined(_DEBUG)
	FreeConsole();
#endif

	return DXUTGetExitCode();
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	g_SampleUI.Init(&g_DialogResourceManager);
	g_SampleUI.SetCallback(OnGUIEvent);

	auto iX = -255;
	auto iY = -670;
	g_SampleUI.AddRadioButton(IDC_TRI_PROJ, 0, L"Axis-aligned projection (AAP) of max projected area", iX, iY += 26, 150, 22);
	g_SampleUI.AddRadioButton(IDC_TRI_PROJ_TESS, 0, L"Tessellation for AAP view of max projected area", iX, iY += 26, 150, 22);
	g_SampleUI.AddRadioButton(IDC_TRI_PROJ_UNION, 0, L"Union of 3 axis-aligned projection views", iX, iY += 26, 150, 22);
	g_SampleUI.GetRadioButton(IDC_TRI_PROJ)->SetChecked(true);
	g_SampleUI.AddRadioButton(IDC_BOX_ARRAY, 1, L"Render surface voxels as box array", iX, iY += 36, 150, 22);
	g_SampleUI.AddRadioButton(IDC_RAY_CAST, 1, L"Render solid voxels with raycasting", iX, iY += 26, 150, 22);
	g_SampleUI.GetRadioButton(IDC_BOX_ARRAY)->SetChecked(true);
}

//--------------------------------------------------------------------------------------
// Called right before creating a D3D device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	pDeviceSettings->d3d11.SyncInterval = 0;

	return true;
}

//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	// Update the camera's position based on user input 
	g_Camera.FrameMove(fElapsedTime);
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
	UINT nBackBufferHeight = DXUTGetDXGIBackBufferSurfaceDesc()->Height;

	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos(2, 0);
	g_pTxtHelper->SetForegroundColor(Colors::Yellow);
	g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(DXUTIsVsyncEnabled()));
	g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());

	// Draw help
	if (g_bShowHelp)
	{
		g_pTxtHelper->SetInsertionPos(2, nBackBufferHeight - 20 * 4);
		g_pTxtHelper->SetForegroundColor(Colors::Red);
		g_pTxtHelper->DrawTextLine(L"Controls:");

		g_pTxtHelper->SetInsertionPos(20, nBackBufferHeight - 20 * 3);
		g_pTxtHelper->DrawTextLine(L"Rotate camera: Left mouse button\n"
			L"Zoom camera: Mouse wheel scroll\n");

		g_pTxtHelper->SetInsertionPos(285, nBackBufferHeight - 20 * 3);
		g_pTxtHelper->DrawTextLine(L"Hide help: F1\n"
			L"Quit: ESC\n");
	}
	else
	{
		g_pTxtHelper->SetForegroundColor(Colors::White);
		g_pTxtHelper->DrawTextLine(L"Press F1 for help");
	}

	g_pTxtHelper->End();
}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
	void* pUserContext)
{
	// Pass messages to dialog resource manager calls so GUI state is updated correctly
	*pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing) return 0;

	// Give the dialogs a chance to handle the message first
	*pbNoFurtherProcessing = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing) return 0;

	// Pass all remaining windows messages to camera so it can respond to user input
	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

	return 0;
}

//--------------------------------------------------------------------------------------
// Handle mouse events
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
	bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
	int xPos, int yPos, void *pUserContext)
{
	if (bLeftButtonDown)
	{
	}
	else
	{
	}
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
	if (bKeyDown)
	{
		switch (nChar)
		{
		case VK_F1:
			g_bShowHelp = !g_bShowHelp; break;
		case VK_F2:
			g_bShowFPS = !g_bShowFPS; break;
		}
	}
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	switch (nControlID)
	{
		// Standard DXUT controls
	case IDC_TRI_PROJ:
		g_eVoxMethod = SurfaceVoxelizer::TRI_PROJ;
		break;
	case IDC_TRI_PROJ_TESS:
		g_eVoxMethod = SurfaceVoxelizer::TRI_PROJ_TESS;
		break;
	case IDC_TRI_PROJ_UNION:
		g_eVoxMethod = SurfaceVoxelizer::TRI_PROJ_UNION;
		break;
	case IDC_BOX_ARRAY:
		g_bRayCast = false;
		break;
	case IDC_RAY_CAST:
		g_bRayCast = true;
		break;
	}
}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext)
{
	return true;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr;

	LPDXContext pd3dImmediateContext = DXUTGetD3D11DeviceContext();
	V_RETURN(g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
	g_pTxtHelper = make_unique<CDXUTTextHelper>(pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15);

	D3D11_FEATURE_DATA_D3D11_OPTIONS2 features2;
	hr = pd3dDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &features2, sizeof(D3D11_FEATURE_DATA_D3D11_OPTIONS2));
	g_bTypedUAVLoad = features2.TypedUAVLoadAdditionalFormats;
	//if (g_bTypedUAVLoad) MessageBoxA(nullptr, "Supported!", "Typed UAV load", 0);
	//else MessageBoxA(nullptr, "NOT supported!", "Typed UAV load", 0);

	// Load shaders asynchronously.
	g_pShader = make_shared<Shader>(pd3dDevice);
	g_pState = make_shared<State>(pd3dDevice);

	g_pSurfaceVoxelizer = make_unique<SurfaceVoxelizer>(pd3dDevice, g_pShader, g_pState);

	auto loadVSTask = g_pShader->CreateVertexShader(L"VSTriProj.cso", SurfaceVoxelizer::VS_TRI_PROJ);
	loadVSTask = loadVSTask && g_pShader->CreateVertexShader(L"VSTriProjTess.cso", SurfaceVoxelizer::VS_TRI_PROJ_TESS);
	loadVSTask = loadVSTask && g_pShader->CreateVertexShader(L"VSTriProjUnion.cso", SurfaceVoxelizer::VS_TRI_PROJ_UNION);
	loadVSTask = loadVSTask && g_pShader->CreateVertexShader(L"VSPointArray.cso", SurfaceVoxelizer::VS_POINT_ARRAY);
	loadVSTask = loadVSTask && g_pShader->CreateVertexShader(L"VSBoxArray.cso", SurfaceVoxelizer::VS_BOX_ARRAY);
	const auto loadHSTask = g_pShader->CreateHullShader(L"HSTriProj.cso", SurfaceVoxelizer::HS_TRI_PROJ);
	const auto loadDSTask = g_pShader->CreateDomainShader(L"DSTriProj.cso", SurfaceVoxelizer::DS_TRI_PROJ);
	auto loadPSTask = g_pShader->CreatePixelShader(L"PSTriProj.cso", SurfaceVoxelizer::PS_TRI_PROJ);
	loadPSTask = loadPSTask && g_pShader->CreatePixelShader(L"PSTriProjUnion.cso", SurfaceVoxelizer::PS_TRI_PROJ_UNION);
	loadPSTask = loadPSTask && g_pShader->CreatePixelShader(L"PSTriProjDP.cso", SurfaceVoxelizer::PS_TRI_PROJ_DP);
	loadPSTask = loadPSTask && g_pShader->CreatePixelShader(L"PSTriProjUnionDP.cso", SurfaceVoxelizer::PS_TRI_PROJ_UNION_DP);
	loadPSTask = loadPSTask && g_pShader->CreatePixelShader(L"PSSimple.cso", SurfaceVoxelizer::PS_SIMPLE);
	auto loadCSTask = g_pShader->CreateComputeShader(L"CSDownSample.cso", SurfaceVoxelizer::CS_DOWN_SAMPLE);
	loadCSTask = loadCSTask && g_pShader->CreateComputeShader(L"CSFillSolid.cso", SurfaceVoxelizer::CS_FILL_SOLID);
	loadCSTask = loadCSTask && g_pShader->CreateComputeShader(L"CSGenDirTable.cso", SurfaceVoxelizer::CS_GEN_DIR);
	loadCSTask = loadCSTask && g_pShader->CreateComputeShader(L"CSDownSampleEnc.cso", SurfaceVoxelizer::CS_DOWN_SAMPLE_ENC);
	loadCSTask = loadCSTask && g_pShader->CreateComputeShader(L"CSFillSolidEnc.cso", SurfaceVoxelizer::CS_FILL_SOLID_ENC);
	loadCSTask = loadCSTask && g_pShader->CreateComputeShader(L"CSFillSolidDP.cso", SurfaceVoxelizer::CS_FILL_SOLID_DP);
	loadCSTask = loadCSTask && g_pShader->CreateComputeShader(L"CSRayCast.cso", SurfaceVoxelizer::CS_RAY_CAST);

	const auto createShaderTask = loadVSTask && loadHSTask && loadDSTask && loadPSTask && loadCSTask;

	// Once the mesh is loaded, the object is ready to be rendered.
	createShaderTask.then([pd3dDevice]()
	{
		SurfaceVoxelizer::CreateVertexLayout(pd3dDevice, SurfaceVoxelizer::GetVertexLayout(), g_pShader, SurfaceVoxelizer::VS_TRI_PROJ_TESS);
		g_pShader->ReleaseShaderBuffers();

		// View
		// Setup the camera's view parameters
		const auto vLookAtPt = XMFLOAT4(0.0f, 4.0f, 0.0f, 1.0f);
		const auto vEyePt = XMFLOAT4(-8.0f, 12.0f, 14.0f, 1.0f);
		g_Camera.SetViewParams(XMLoadFloat4(&vEyePt), XMLoadFloat4(&vLookAtPt));

		g_bLoadingComplete = true;
	});

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	HRESULT hr;

	V_RETURN(g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));

	// Setup the camera's projection parameters
	auto fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	g_Camera.SetProjParams(g_fFOVAngleY, fAspectRatio, g_fZNear, g_fZFar);
	g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	g_Camera.SetButtonMasks(MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON);

	// Initialize window size dependent resources
	if (g_pSurfaceVoxelizer) g_pSurfaceVoxelizer->Init(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	g_pSwapChain = pSwapChain;

	//g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
	//g_HUD.SetSize(170, 170);
	g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300);
	g_SampleUI.SetSize(170, 300);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
	float fElapsedTime, void* pUserContext)
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!g_bLoadingComplete || !g_pSwapChain) return;

	// Get the back buffer
	auto pBackBuffer = CPDXTexture2D();
	ThrowIfFailed(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)));

	// Create UAV
	auto pUAVSwapChain = CPDXUnorderedAccessView();
	const auto uavDesc = CD3D11_UNORDERED_ACCESS_VIEW_DESC(pBackBuffer.Get(), D3D11_UAV_DIMENSION_TEXTURE2D);
	ThrowIfFailed(pd3dDevice->CreateUnorderedAccessView(pBackBuffer.Get(), &uavDesc, &pUAVSwapChain));

#if defined(DEBUG) | defined(_DEBUG)
	static auto bFirstTime = true;

	if (bFirstTime)
	{
		// Get the back buffer desc
		auto backBufferSurfaceDesc = D3D11_TEXTURE2D_DESC();
		pBackBuffer->GetDesc(&backBufferSurfaceDesc);

		if (backBufferSurfaceDesc.BindFlags & D3D11_BIND_RENDER_TARGET)
			printf("RTV flag is attached to the current swapchain!\n");
		else printf("RTV flag is NOT attached to the current swapchain!\n");
		if (backBufferSurfaceDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
			printf("UAV flag is attached to the current swapchain!\n");
		else printf("UAV flag is NOT attached to the current swapchain!\n");

		bFirstTime = false;
	}
#endif

	// Set render targets to the screen.
	LPDXRenderTargetView pRTVs[] = { DXUTGetD3D11RenderTargetView() };
	LPDXDepthStencilView pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearRenderTargetView(pRTVs[0], Colors::CornflowerBlue);
	pd3dImmediateContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0u);
	pd3dImmediateContext->OMSetRenderTargets(1u, pRTVs, pDSV);

	// Prepare the constant buffer to send it to the graphics device.
	// Get the projection & view matrix from the camera class
	const auto mProj = g_Camera.GetProjMatrix();
	const auto mView = g_Camera.GetViewMatrix();
	const auto mViewProj = XMMatrixMultiply(mView, mProj);

	// Render
	g_pSurfaceVoxelizer->UpdateFrame(g_Camera.GetEyePt(), mViewProj);
	if (g_bRayCast)
	{
		pd3dImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
		g_pSurfaceVoxelizer->Render(pUAVSwapChain, g_eVoxMethod);
		pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
	}
	else g_pSurfaceVoxelizer->Render(g_eVoxMethod);

	DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
	g_SampleUI.OnRender(fElapsedTime);
	if (g_bShowFPS) RenderText();
	DXUT_EndPerfEvent();
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	g_DialogResourceManager.OnD3D11DestroyDevice();
	DXUTGetGlobalResourceCache().OnDestroyDevice();

	g_bLoadingComplete = false;
	SurfaceVoxelizer::GetVertexLayout().Reset();
	g_pTxtHelper.reset();
	g_pSurfaceVoxelizer.reset();
	g_pState.reset();
	g_pShader.reset();
}
