#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <d3dcompiler.h>
#ifdef _DEBUG
#include <iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace std;
using namespace DirectX;

float xrgb[3] = { 0.66,0.66*2 ,0.66 * 3 };
float rgb[3] = { 0 ,0,0 };

void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif
}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

#ifdef _DEBUG
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	if (!SUCCEEDED(result))
		return;

	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}
#endif

#ifdef _DEBUG
int main()
{
#else
#endif
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

WNDCLASSEX w = {};

w.cbSize = sizeof(WNDCLASSEX);
w.lpfnWndProc = (WNDPROC)WindowProcedure;
w.lpszClassName = _T("DX12Sample");
w.hInstance = GetModuleHandle(nullptr);

RegisterClassEx(&w);
RECT wrc = { 0,0,window_width,window_height };
AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
HWND hwnd = CreateWindow(w.lpszClassName, _T("DX12 テスト"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, wrc.right - wrc.left, wrc.bottom - wrc.top, nullptr, nullptr, w.hInstance, nullptr);

#ifdef _DEBUG
EnableDebugLayer();
#endif

D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, };

#ifdef _DEBUG
auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif

std::vector <IDXGIAdapter*> adapters;

IDXGIAdapter* tmpAdapter = nullptr;
for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
{
	adapters.push_back(tmpAdapter);
}

for (auto adpt : adapters)
{
	DXGI_ADAPTER_DESC adesc = {};
	adpt->GetDesc(&adesc);
	std::wstring strDesc = adesc.Description;

	if (strDesc.find(L"NVIDIA") != std::string::npos)
	{
		tmpAdapter = adpt;
		break;
	}
}

D3D_FEATURE_LEVEL featureLevel;
for (auto lv : levels)
{
	if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
	{
		featureLevel = lv;
		break;
	}
}

result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
cmdQueueDesc.NodeMask = 0;
cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));

DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
swapchainDesc.Width = window_width;
swapchainDesc.Height = window_height;
swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
swapchainDesc.Stereo = false;
swapchainDesc.SampleDesc.Count = 1;
swapchainDesc.SampleDesc.Quality = 0;
swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
swapchainDesc.BufferCount = 2;
swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue, hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)&_swapchain);


D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
heapDesc.NodeMask = 0;
heapDesc.NumDescriptors = 2;
heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

ID3D12DescriptorHeap* rtvHeaps = nullptr;
result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

DXGI_SWAP_CHAIN_DESC swcDesc = {};
result = _swapchain->GetDesc(&swcDesc);

std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
for (int idx = 0; idx < swcDesc.BufferCount; ++idx)
{
	result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
}

ID3D12Fence* _fence = nullptr;
UINT64 _fenceVal = 0;
result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

ShowWindow(hwnd, SW_SHOW);

XMFLOAT3 vertices[] = {
	{+0.0f,+0.9f,0.0f}, // x , y , z
	{+0.13f,+0.3f,0.0f},//1
	{+0.5f,+0.3f,0.0f},//2
	{+0.2f,-0.1f,0.0f},//3
	{+0.35f,-0.8f,0.0f},//4
	{+0.0f,-0.35f,0.0f},//5
	{-0.35f,-0.8f,0.0f},//6
	{-0.2f,-0.1f,0.0f},//7
	{-0.5f,+0.3f,0.0f},//8
	{-0.13f,+0.3f,0.0f},//9
};

D3D12_HEAP_PROPERTIES heapprop = {};
heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

D3D12_RESOURCE_DESC resdesc = {};
resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
resdesc.Width = sizeof(vertices);
resdesc.Height = 1;
resdesc.DepthOrArraySize = 1;
resdesc.MipLevels = 1;
resdesc.Format = DXGI_FORMAT_UNKNOWN;
resdesc.SampleDesc.Count = 1;
resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

ID3D12Resource* vertBuff = nullptr;

result = _dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertBuff));

XMFLOAT3* vertMap = nullptr;
result = vertBuff->Map(0, nullptr, (void**)&vertMap);
std::copy(std::begin(vertices), std::end(vertices), vertMap);
vertBuff->Unmap(0, nullptr);

D3D12_VERTEX_BUFFER_VIEW vbView = {};
vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
vbView.SizeInBytes = sizeof(vertices);
vbView.StrideInBytes = sizeof(vertices[0]);

ID3DBlob* _vsBlob = nullptr;
ID3DBlob* _psBlob = nullptr;

unsigned short indices[] = {
	0,1,9,
	1,2,3,
	3,4,5,
	5,6,7,
	7,8,9,
	1,3,5,
	1,5,9,
	9,5,7
};

ID3D12Resource* idxBuff = nullptr;
result = _dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&idxBuff));

unsigned short* mappedIdx = nullptr;
idxBuff->Map(0, nullptr, (void**)&mappedIdx);
std::copy(std::begin(indices), std::end(indices), mappedIdx);
idxBuff->Unmap(0, nullptr);

D3D12_INDEX_BUFFER_VIEW ibView = {};
ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
ibView.Format = DXGI_FORMAT_R16_UINT;
ibView.SizeInBytes = sizeof(indices);

ID3DBlob* errorBlob = nullptr;
result = D3DCompileFromFile(L"BasicVertexShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BasicVS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_vsBlob, &errorBlob);
if (FAILED(result))
{
	if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
	{
		::OutputDebugStringA("ファイルが見当たりません");
	}
	else
	{
		std::string errstr;
		errstr.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";
		OutputDebugStringA(errstr.c_str());
	}
	exit(1);
}

result = D3DCompileFromFile(L"BasicPixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BasicPS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_psBlob, &errorBlob);
if (FAILED(result))
{
	if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
	{
		::OutputDebugStringA("ファイルが見当たりません");
	}
	else
	{
		std::string errstr;
		errstr.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";
		OutputDebugStringA(errstr.c_str());
	}
	exit(1);
}

D3D12_INPUT_ELEMENT_DESC inputLayout[] =
{
	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
};

D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
gpipeline.pRootSignature = nullptr;

gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
gpipeline.RasterizerState.MultisampleEnable = false;
gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
gpipeline.RasterizerState.DepthClipEnable = true;
gpipeline.BlendState.AlphaToCoverageEnable = false;
gpipeline.BlendState.IndependentBlendEnable = false;

D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
renderTargetBlendDesc.BlendEnable = false;
renderTargetBlendDesc.LogicOpEnable = false;
renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;
gpipeline.InputLayout.pInputElementDescs = inputLayout;
gpipeline.InputLayout.NumElements = _countof(inputLayout);
gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
gpipeline.NumRenderTargets = 1;
gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
gpipeline.SampleDesc.Count = 1;
gpipeline.SampleDesc.Quality = 0;

D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

ID3DBlob* rootSigBlob = nullptr;
result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);

ID3D12RootSignature* rootsignature = nullptr;
result = _dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
rootSigBlob->Release();

gpipeline.pRootSignature = rootsignature;

ID3D12PipelineState* _pipelinestate = nullptr;
result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

D3D12_VIEWPORT viewport = {};
viewport.Width = window_width;
viewport.Height = window_height;
viewport.TopLeftX = 0;
viewport.TopLeftY = 0;
viewport.MaxDepth = 1.0f;
viewport.MinDepth = 0.0f;

D3D12_RECT scissorrect = {};
scissorrect.top = 0;
scissorrect.left = 0;
scissorrect.right = scissorrect.left + window_width;
scissorrect.bottom = scissorrect.top + window_height;

while (true)
{
	MSG msg;
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			break;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
	BarrierDesc.Transition.Subresource = 0;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	_cmdList->SetPipelineState(_pipelinestate);

	auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

	float clearColor[] = { rgb[0],rgb[1],rgb[2],1.0f };
	_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	for (int i = 0; i <= 3; i++)
	{
		xrgb[i] += 0.01f;

		if (xrgb[i] >= 2)
		{
			xrgb[i] = 0;
		}
		else if (xrgb[i] >= 1)
		{
			rgb[i] = 1 - (xrgb[i] - 1);
		}
		else
		{
			rgb[i] = xrgb[i];
		}
	}

	_cmdList->SetGraphicsRootSignature(rootsignature);
	_cmdList->RSSetViewports(1, &viewport);
	_cmdList->RSSetScissorRects(1, &scissorrect);
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_cmdList->IASetVertexBuffers(0, 1, &vbView);
	_cmdList->IASetIndexBuffer(&ibView);
	_cmdList->DrawIndexedInstanced(27, 1, 0, 0, 0);

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	_cmdList->Close();

	ID3D12CommandList* cmdlists[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);

	_cmdQueue->Signal(_fence, ++_fenceVal);

	if (_fence->GetCompletedValue() != _fenceVal)
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	_cmdAllocator->Reset();
	_cmdList->Reset(_cmdAllocator, nullptr);

	_swapchain->Present(1, 0);
}

UnregisterClass(w.lpszClassName, w.hInstance);

return 0;
}