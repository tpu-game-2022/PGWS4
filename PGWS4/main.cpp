//グラデーションは別のファイルに移行しました。
#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <d3dx12.h>
#ifdef _DEBUG
#include <iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace std;
using namespace DirectX;

//const int numberOfColors = 3;
//float color[numberOfColors] = { 1.0f,1.0f,0.0f };
//bool colorFlag = false;

void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
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

//#ifdef _DEBUG
//int main()
//{
//#else
//int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
//#endif
//DebugOutputFormatString("Show window test.");
//getchar();
//return 0;
//}

size_t AlignmentedSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
}

#ifdef _DEBUG
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	if (!SUCCEEDED(result)) return;

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

HWND hwnd = CreateWindow(w.lpszClassName, _T("DX12 テスト "),
	WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, wrc.right - wrc.left, wrc.bottom - wrc.top,
	nullptr, nullptr, w.hInstance, nullptr);

HRESULT D3D12CreateDevice(
	IUnknown * pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice);

#ifdef _DEBUG
EnableDebugLayer();
#endif

D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0,
D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0, };

#ifdef _DEBUG
auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif

std::vector<IDXGIAdapter*> adapters;

IDXGIAdapter* tmpAdapter = nullptr;

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

HRESULT CreateSwapChainForHwnd(
	IUnknown * pDevice, HWND hWnd,
	const DXGI_SWAP_CHAIN_DESC1 * pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC * pFullscreenDesc,
	IDXGIOutput * pRestrictToOutput, IDXGISwapChain1 * *ppSwapChain);

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
result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue, hwnd, &swapchainDesc,
	nullptr, nullptr, (IDXGISwapChain1**)&_swapchain);

D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
heapDesc.NodeMask = 0;
heapDesc.NumDescriptors = 2;
heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

ID3D12DescriptorHeap* rtvHeaps = nullptr;
result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

DXGI_SWAP_CHAIN_DESC swcDesc = {};
result = _swapchain->GetDesc(&swcDesc);

D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

std::vector<ID3D12Resource*>_backBuffers(swcDesc.BufferCount);

for (int idx = 0; idx < swcDesc.BufferCount; ++idx)
{
	result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
	D3D12_CPU_DESCRIPTOR_HANDLE handle
		= rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_dev->CreateRenderTargetView(_backBuffers[idx], &rtvDesc, handle);
}

ID3D12Fence* _fence = nullptr;
UINT64 _fenceVal = 0;
result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

ShowWindow(hwnd, SW_SHOW);

struct Vertex {
	XMFLOAT3 pos;
	XMFLOAT2 uv;
};

Vertex vertices[] =
{
	{{-0.4f,-0.7f,0.0f},{0.0f,1.0f}},
	{{-0.4f,+0.7f,0.0f},{0.0f,0.0f}},
	{{+0.4f,-0.7f,0.0f},{1.0f,1.0f}},
	{{+0.4f,+0.7f,0.0f},{1.0f,0.0f}},
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

unsigned short indices[] =
{
	0,1,2,
	2,1,3,
};

ID3D12Resource* idxBuff = nullptr;

resdesc.Width = sizeof(indices);
result = _dev->CreateCommittedResource(
	&heapprop,
	D3D12_HEAP_FLAG_NONE,
	&resdesc,
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&idxBuff));

unsigned short* mappedIdx = nullptr;
idxBuff->Map(0, nullptr, (void**)&mappedIdx);
std::copy(std::begin(indices), std::end(indices), mappedIdx);
idxBuff->Unmap(0, nullptr);

D3D12_INDEX_BUFFER_VIEW ibView = {};
ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
ibView.Format = DXGI_FORMAT_R16_UINT;
ibView.SizeInBytes = sizeof(indices);

ID3D12Resource* vertBuff = nullptr;

auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices));
result = _dev->CreateCommittedResource(
	&heapProp,
	D3D12_HEAP_FLAG_NONE,
	&resourceDesc,
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&vertBuff));

Vertex* vertMap = nullptr;
result = vertBuff->Map(0, nullptr, (void**)&vertMap);
std::copy(std::begin(vertices), std::end(vertices), vertMap);
vertBuff->Unmap(0, nullptr);

D3D12_VERTEX_BUFFER_VIEW vbView = {};
vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
vbView.SizeInBytes = sizeof(vertices);
vbView.StrideInBytes = sizeof(vertices[0]);

ID3DBlob* _vsBlob = nullptr;
ID3DBlob* _psBlob = nullptr;

ID3DBlob* errorBlob = nullptr;
result = D3DCompileFromFile(
	L"BasicVertexShader.hlsl",
	nullptr,
	D3D_COMPILE_STANDARD_FILE_INCLUDE,
	"BasicVS", "vs_5_0",
	D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
	0,
	&_vsBlob, &errorBlob);

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
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";
		OutputDebugStringA(errstr.c_str());
	}
	exit(1);
}

result = D3DCompileFromFile(
	L"BasicPixelShader.hlsl",
	nullptr,
	D3D_COMPILE_STANDARD_FILE_INCLUDE,
	"BasicPS", "ps_5_0",
	D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
	0,
	&_psBlob, &errorBlob);

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
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";
		OutputDebugStringA(errstr.c_str());
		OutputDebugStringA(errstr.c_str());
	}
	exit(1);
}

D3D12_INPUT_ELEMENT_DESC inputLayout[] =
{
	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
	D3D12_APPEND_ALIGNED_ELEMENT,
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
	D3D12_APPEND_ALIGNED_ELEMENT,
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
};

D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

gpipeline.pRootSignature = nullptr;

gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();
gpipeline.InputLayout.pInputElementDescs = inputLayout;
gpipeline.InputLayout.NumElements = _countof(inputLayout);

gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

gpipeline.RasterizerState.MultisampleEnable = false;

gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
gpipeline.RasterizerState.DepthClipEnable = true;

gpipeline.BlendState.AlphaToCoverageEnable = false;
gpipeline.BlendState.IndependentBlendEnable = false;

D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc{};

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

D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

D3D12_DESCRIPTOR_RANGE descTblRange = {};
descTblRange.NumDescriptors = 1;
descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
descTblRange.BaseShaderRegister = 0;
descTblRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

D3D12_ROOT_PARAMETER rootparam = {};
rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
rootparam.DescriptorTable.NumDescriptorRanges = 1;

rootSignatureDesc.pParameters = &rootparam;
rootSignatureDesc.NumParameters = 1;

D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
samplerDesc.MinLOD = 0.0f;
samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

rootSignatureDesc.pStaticSamplers = &samplerDesc;
rootSignatureDesc.NumStaticSamplers = 1;

ID3DBlob* rootSigBlob = nullptr;
result = D3D12SerializeRootSignature(
	&rootSignatureDesc,
	D3D_ROOT_SIGNATURE_VERSION_1_0,
	&rootSigBlob,
	&errorBlob);

ID3D12RootSignature* rootsignature = nullptr;
result = _dev->CreateRootSignature(
	0,
	rootSigBlob->GetBufferPointer(),
	rootSigBlob->GetBufferSize(),
	IID_PPV_ARGS(&rootsignature));
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

TexMetadata metadata = {};
ScratchImage scratchImg = {};

result = LoadFromWICFile(
	L"img/textest.png", WIC_FLAGS_NONE,
	&metadata, scratchImg);

auto img = scratchImg.GetImage(0, 0, 0);

D3D12_HEAP_PROPERTIES uploadHeapProp = {};

uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

uploadHeapProp.CreationNodeMask = 0;
uploadHeapProp.VisibleNodeMask = 0;

D3D12_RESOURCE_DESC resDesc = {};

resDesc.Format = DXGI_FORMAT_UNKNOWN;
resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

resDesc.Width = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
* img->height;
resDesc.Height = 1;
resDesc.DepthOrArraySize = 1;
resDesc.MipLevels = 1;

resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

resDesc.SampleDesc.Count = 1;
resDesc.SampleDesc.Quality = 0;

ID3D12Resource* uploadbuff = nullptr;

result = _dev->CreateCommittedResource(
	&uploadHeapProp,
	D3D12_HEAP_FLAG_NONE,
	&resDesc,
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&uploadbuff)
);

D3D12_HEAP_PROPERTIES texHeapProp = {};

texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
texHeapProp.CreationNodeMask = 0;
texHeapProp.VisibleNodeMask = 0;

resDesc.Format = metadata.format;
resDesc.Width = static_cast<UINT>(metadata.width);
resDesc.Height = static_cast<UINT>(metadata.height);
resDesc.DepthOrArraySize = static_cast<uint16_t>(metadata.arraySize);
resDesc.MipLevels = static_cast<uint16_t>(metadata.mipLevels);
resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

ID3D12Resource* texbuff = nullptr;
result = _dev->CreateCommittedResource(
	&texHeapProp,
	D3D12_HEAP_FLAG_NONE,
	&resDesc,
	D3D12_RESOURCE_STATE_COPY_DEST,
	nullptr,
	IID_PPV_ARGS(&texbuff));

result = texbuff->WriteToSubresource(
	0,
	nullptr,
	img->pixels,
	static_cast<UINT>(img->rowPitch),
	static_cast<UINT>(img->slicePitch)
);

uint8_t* mapforImg = nullptr;
result = uploadbuff->Map(0, nullptr, (void**)&mapforImg);

auto srcAddress = img->pixels;

auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

for (int y = 0; y < img->height; ++y)
{
	std::copy_n(srcAddress,
		rowPitch,
		mapforImg);


	srcAddress += img->rowPitch;
	mapforImg += rowPitch;
}


uploadbuff->Unmap(0, nullptr);

D3D12_TEXTURE_COPY_LOCATION src = {};

src.pResource = uploadbuff;
src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
src.PlacedFootprint.Offset = 0;
src.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata.width);
src.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata.height);
src.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata.depth);
src.PlacedFootprint.Footprint.RowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
src.PlacedFootprint.Footprint.Format = img->format;

D3D12_TEXTURE_COPY_LOCATION dst = {};

dst.pResource = texbuff;
dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
dst.SubresourceIndex = 0;

_cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

D3D12_RESOURCE_BARRIER BarrierDesc = {};
BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
BarrierDesc.Transition.pResource = texbuff;
BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
BarrierDesc.Transition.StateBefore =
D3D12_RESOURCE_STATE_COPY_DEST;
BarrierDesc.Transition.StateAfter =
D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

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

ID3D12DescriptorHeap* texDescHeap = nullptr;
D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
descHeapDesc.NodeMask = 0;
descHeapDesc.NumDescriptors = 1;
descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap));

D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
srvDesc.Format = metadata.format;
srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
srvDesc.Texture2D.MipLevels = 1;

_dev->CreateShaderResourceView(
	texbuff,
	&srvDesc,
	texDescHeap->GetCPUDescriptorHandleForHeapStart()
);

//↓10月26日(水)の課題
Vertex vertices_Second[] =
{
	{{-0.9f,-0.4f,0.0f},{0.0f,1.0f}},
	{{-0.9f,+0.3f,0.0f},{0.0f,0.0f}},
	{{-0.5f,-0.4f,0.0f},{1.0f,1.0f}},
	{{-0.5f,+0.3f,0.0f},{1.0f,0.0f}},
};

D3D12_HEAP_PROPERTIES heapprop_Second = {};
heapprop_Second.Type = D3D12_HEAP_TYPE_UPLOAD;
heapprop_Second.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
heapprop_Second.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

D3D12_RESOURCE_DESC resdesc_Second = {};
resdesc_Second.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
resdesc_Second.Width = sizeof(vertices_Second);
resdesc_Second.Height = 1;
resdesc_Second.DepthOrArraySize = 1;
resdesc_Second.MipLevels = 1;
resdesc_Second.Format = DXGI_FORMAT_UNKNOWN;
resdesc_Second.SampleDesc.Count = 1;
resdesc_Second.Flags = D3D12_RESOURCE_FLAG_NONE;
resdesc_Second.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

unsigned short indices_Second[] =
{
	0,1,2,
	2,1,3,
};

ID3D12Resource* idxBuff_Second = nullptr;

resdesc.Width = sizeof(indices_Second);
result = _dev->CreateCommittedResource(
	&heapprop_Second,
	D3D12_HEAP_FLAG_NONE,
	&resdesc_Second,
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&idxBuff_Second));

unsigned short* mappedIdx_Second = nullptr;
idxBuff_Second->Map(0, nullptr, (void**)&mappedIdx_Second);
std::copy(std::begin(indices_Second), std::end(indices_Second), mappedIdx_Second);
idxBuff_Second->Unmap(0, nullptr);

D3D12_INDEX_BUFFER_VIEW ibView_Second = {};
ibView_Second.BufferLocation = idxBuff_Second->GetGPUVirtualAddress();
ibView_Second.Format = DXGI_FORMAT_R16_UINT;
ibView_Second.SizeInBytes = sizeof(indices_Second);

ID3D12Resource* vertBuff_Second = nullptr;

auto heapProp_Second = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
auto resourceDesc_Second = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices_Second));
result = _dev->CreateCommittedResource(
	&heapProp_Second,
	D3D12_HEAP_FLAG_NONE,
	&resourceDesc_Second,
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&vertBuff_Second));

Vertex* vertMap_Second = nullptr;
result = vertBuff_Second->Map(0, nullptr, (void**)&vertMap_Second);
std::copy(std::begin(vertices_Second), std::end(vertices_Second), vertMap_Second);
vertBuff_Second->Unmap(0, nullptr);

D3D12_VERTEX_BUFFER_VIEW vbView_Second = {};
vbView_Second.BufferLocation = vertBuff_Second->GetGPUVirtualAddress();
vbView_Second.SizeInBytes = sizeof(vertices_Second);
vbView_Second.StrideInBytes = sizeof(vertices_Second[0]);

ID3DBlob* _vsBlob_Second = nullptr;
ID3DBlob* _psBlob_Second = nullptr;

ID3DBlob* errorBlob_Second = nullptr;
result = D3DCompileFromFile(
	L"BasicVertexShader.hlsl",
	nullptr,
	D3D_COMPILE_STANDARD_FILE_INCLUDE,
	"BasicVS", "vs_5_0",
	D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
	0,
	&_vsBlob_Second, &errorBlob_Second);

if (FAILED(result))
{
	if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
	{
		::OutputDebugStringA("ファイルが見当たりません");
	}
	else
	{
		std::string errstr;
		errstr.resize(errorBlob_Second->GetBufferSize());
		std::copy_n((char*)errorBlob_Second->GetBufferPointer(),
			errorBlob_Second->GetBufferSize(), errstr.begin());
		errstr += "\n";
		OutputDebugStringA(errstr.c_str());
	}
	exit(1);
}

result = D3DCompileFromFile(
	L"BasicPixelShader.hlsl",
	nullptr,
	D3D_COMPILE_STANDARD_FILE_INCLUDE,
	"BasicPS", "ps_5_0",
	D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
	0,
	&_psBlob_Second, &errorBlob_Second);

if (FAILED(result))
{
	if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
	{
		::OutputDebugStringA("ファイルが見当たりません");
	}
	else
	{
		std::string errstr;
		errstr.resize(errorBlob_Second->GetBufferSize());
		std::copy_n((char*)errorBlob_Second->GetBufferPointer(),
			errorBlob_Second->GetBufferSize(), errstr.begin());
		errstr += "\n";
		OutputDebugStringA(errstr.c_str());
		OutputDebugStringA(errstr.c_str());
	}
	exit(1);
}

D3D12_INPUT_ELEMENT_DESC inputLayout_Second[] =
{
	{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
	D3D12_APPEND_ALIGNED_ELEMENT,
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
	D3D12_APPEND_ALIGNED_ELEMENT,
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
};

D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline_Second = {};

gpipeline_Second.pRootSignature = nullptr;

gpipeline_Second.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
gpipeline_Second.VS.BytecodeLength = _vsBlob->GetBufferSize();
gpipeline_Second.PS.pShaderBytecode = _psBlob->GetBufferPointer();
gpipeline_Second.PS.BytecodeLength = _psBlob->GetBufferSize();
gpipeline_Second.InputLayout.pInputElementDescs = inputLayout;
gpipeline_Second.InputLayout.NumElements = _countof(inputLayout);

gpipeline_Second.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

gpipeline_Second.RasterizerState.MultisampleEnable = false;

gpipeline_Second.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
gpipeline_Second.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
gpipeline_Second.RasterizerState.DepthClipEnable = true;

gpipeline_Second.BlendState.AlphaToCoverageEnable = false;
gpipeline_Second.BlendState.IndependentBlendEnable = false;

D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc_Second{};

renderTargetBlendDesc_Second.BlendEnable = false;

renderTargetBlendDesc_Second.LogicOpEnable = false;
renderTargetBlendDesc_Second.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

gpipeline_Second.BlendState.RenderTarget[0] = renderTargetBlendDesc_Second;

gpipeline_Second.InputLayout.pInputElementDescs = inputLayout_Second;
gpipeline_Second.InputLayout.NumElements = _countof(inputLayout_Second);

gpipeline_Second.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
gpipeline_Second.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

gpipeline_Second.NumRenderTargets = 1;
gpipeline_Second.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

gpipeline_Second.SampleDesc.Count = 1;
gpipeline_Second.SampleDesc.Quality = 0;

D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc_Second{};
rootSignatureDesc_Second.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

D3D12_DESCRIPTOR_RANGE descTblRange_Second = {};
descTblRange_Second.NumDescriptors = 1;
descTblRange_Second.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
descTblRange_Second.BaseShaderRegister = 0;
descTblRange_Second.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

D3D12_ROOT_PARAMETER rootparam_Second = {};
rootparam_Second.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
rootparam_Second.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
rootparam_Second.DescriptorTable.pDescriptorRanges = &descTblRange_Second;
rootparam_Second.DescriptorTable.NumDescriptorRanges = 1;

rootSignatureDesc_Second.pParameters = &rootparam_Second;
rootSignatureDesc_Second.NumParameters = 1;

D3D12_STATIC_SAMPLER_DESC samplerDesc_Second = {};
samplerDesc_Second.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
samplerDesc_Second.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
samplerDesc_Second.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
samplerDesc_Second.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
samplerDesc_Second.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
samplerDesc_Second.MaxLOD = D3D12_FLOAT32_MAX;
samplerDesc_Second.MinLOD = 0.0f;
samplerDesc_Second.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
samplerDesc_Second.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

rootSignatureDesc_Second.pStaticSamplers = &samplerDesc_Second;
rootSignatureDesc_Second.NumStaticSamplers = 1;

ID3DBlob* rootSigBlob_Second = nullptr;
result = D3D12SerializeRootSignature(
	&rootSignatureDesc_Second,
	D3D_ROOT_SIGNATURE_VERSION_1_0,
	&rootSigBlob_Second,
	&errorBlob_Second);

ID3D12RootSignature* rootsignature_Second = nullptr;
result = _dev->CreateRootSignature(
	0,
	rootSigBlob_Second->GetBufferPointer(),
	rootSigBlob_Second->GetBufferSize(),
	IID_PPV_ARGS(&rootsignature_Second));
rootSigBlob_Second->Release();

gpipeline_Second.pRootSignature = rootsignature_Second;

ID3D12PipelineState* _pipelinestate_Second = nullptr;
result = _dev->CreateGraphicsPipelineState(&gpipeline_Second, IID_PPV_ARGS(&_pipelinestate_Second));

D3D12_VIEWPORT viewport_Second = {};
viewport_Second.Width = window_width;
viewport_Second.Height = window_height;
viewport_Second.TopLeftX = 0;
viewport_Second.TopLeftY = 0;
viewport_Second.MaxDepth = 1.0f;
viewport_Second.MinDepth = 0.0f;

D3D12_RECT scissorrect_Second = {};
scissorrect_Second.top = 0;
scissorrect_Second.left = 0;
scissorrect_Second.right = scissorrect_Second.left + window_width;
scissorrect_Second.bottom = scissorrect_Second.top + window_height;

TexMetadata metadata_Second = {};
ScratchImage scratchImg_Second = {};

result = LoadFromWICFile(
	L"img/mario.png", WIC_FLAGS_NONE,
	&metadata_Second, scratchImg_Second);

auto img_Second = scratchImg_Second.GetImage(0, 0, 0);

D3D12_HEAP_PROPERTIES uploadHeapProp_Second = {};

uploadHeapProp_Second.Type = D3D12_HEAP_TYPE_UPLOAD;

uploadHeapProp_Second.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
uploadHeapProp_Second.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

uploadHeapProp_Second.CreationNodeMask = 0;
uploadHeapProp_Second.VisibleNodeMask = 0;

D3D12_RESOURCE_DESC resDesc_Second = {};

resDesc_Second.Format = DXGI_FORMAT_UNKNOWN;
resDesc_Second.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;

resDesc_Second.Width = AlignmentedSize(img_Second->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
* img_Second->height;
resDesc_Second.Height = 1;
resDesc_Second.DepthOrArraySize = 1;
resDesc_Second.MipLevels = 1;

resDesc_Second.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
resDesc_Second.Flags = D3D12_RESOURCE_FLAG_NONE;

resDesc_Second.SampleDesc.Count = 1;
resDesc_Second.SampleDesc.Quality = 0;

ID3D12Resource* uploadbuff_Second = nullptr;

result = _dev->CreateCommittedResource(
	&uploadHeapProp_Second,
	D3D12_HEAP_FLAG_NONE,
	&resDesc_Second,
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&uploadbuff_Second)
);

D3D12_HEAP_PROPERTIES texHeapProp_Second = {};

texHeapProp_Second.Type = D3D12_HEAP_TYPE_DEFAULT;
texHeapProp_Second.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
texHeapProp_Second.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
texHeapProp_Second.CreationNodeMask = 0;
texHeapProp_Second.VisibleNodeMask = 0;

resDesc_Second.Format = metadata_Second.format;
resDesc_Second.Width = static_cast<UINT>(metadata_Second.width);
resDesc_Second.Height = static_cast<UINT>(metadata_Second.height);
resDesc_Second.DepthOrArraySize = static_cast<uint16_t>(metadata_Second.arraySize);
resDesc_Second.MipLevels = static_cast<uint16_t>(metadata_Second.mipLevels);
resDesc_Second.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata_Second.dimension);
resDesc_Second.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

ID3D12Resource* texbuff_Second = nullptr;
result = _dev->CreateCommittedResource(
	&texHeapProp_Second,
	D3D12_HEAP_FLAG_NONE,
	&resDesc_Second,
	D3D12_RESOURCE_STATE_COPY_DEST,
	nullptr,
	IID_PPV_ARGS(&texbuff_Second));

result = texbuff_Second->WriteToSubresource(
	0,
	nullptr,
	img_Second->pixels,
	static_cast<UINT>(img_Second->rowPitch),
	static_cast<UINT>(img_Second->slicePitch)
);

uint8_t* mapforImg_Second = nullptr;
result = uploadbuff_Second->Map(0, nullptr, (void**)&mapforImg_Second);

auto srcAddress_Second = img_Second->pixels;

auto rowPitch_Second = AlignmentedSize(img_Second->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

for (int y = 0; y < img_Second->height; ++y)
{
	std::copy_n(srcAddress_Second,
		rowPitch_Second,
		mapforImg_Second);


	srcAddress_Second += img_Second->rowPitch;
	mapforImg_Second += rowPitch_Second;
}


uploadbuff_Second->Unmap(0, nullptr);

D3D12_TEXTURE_COPY_LOCATION src_Second = {};

src_Second.pResource = uploadbuff_Second;
src_Second.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
src_Second.PlacedFootprint.Offset = 0;
src_Second.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata_Second.width);
src_Second.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata_Second.height);
src_Second.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata_Second.depth);
src_Second.PlacedFootprint.Footprint.RowPitch = AlignmentedSize(img_Second->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
src_Second.PlacedFootprint.Footprint.Format = img_Second->format;

D3D12_TEXTURE_COPY_LOCATION dst_Second = {};

dst_Second.pResource = texbuff_Second;
dst_Second.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
dst_Second.SubresourceIndex = 0;

_cmdList->CopyTextureRegion(&dst_Second, 0, 0, 0, &src_Second, nullptr);

D3D12_RESOURCE_BARRIER BarrierDesc_Second = {};
BarrierDesc_Second.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
BarrierDesc_Second.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
BarrierDesc_Second.Transition.pResource = texbuff_Second;
BarrierDesc_Second.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
BarrierDesc_Second.Transition.StateBefore =
D3D12_RESOURCE_STATE_COPY_DEST;
BarrierDesc_Second.Transition.StateAfter =
D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

_cmdList->ResourceBarrier(1, &BarrierDesc_Second);
_cmdList->Close();

ID3D12CommandList* cmdlists_Second[] = { _cmdList };
_cmdQueue->ExecuteCommandLists(1, cmdlists_Second);

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

ID3D12DescriptorHeap* texDescHeap_Second = nullptr;
D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc_Second = {};
descHeapDesc_Second.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
descHeapDesc_Second.NodeMask = 0;
descHeapDesc_Second.NumDescriptors = 1;
descHeapDesc_Second.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

result = _dev->CreateDescriptorHeap(&descHeapDesc_Second, IID_PPV_ARGS(&texDescHeap_Second));

D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc_Second = {};
srvDesc_Second.Format = metadata_Second.format;
srvDesc_Second.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
srvDesc_Second.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
srvDesc_Second.Texture2D.MipLevels = 1;

_dev->CreateShaderResourceView(
	texbuff_Second,
	&srvDesc_Second,
	texDescHeap_Second->GetCPUDescriptorHandleForHeapStart()
);
//↑

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

	auto BarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	_cmdList->SetPipelineState(_pipelinestate);

	auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

	float clearColor[] = { 0.0f,0.0f,0.3f,1.0f };

	_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	_cmdList->ResourceBarrier(1, &BarrierDesc);

	_cmdList->SetGraphicsRootSignature(rootsignature);
	_cmdList->SetDescriptorHeaps(1, &texDescHeap);
	_cmdList->SetGraphicsRootDescriptorTable(
		0,
		texDescHeap->GetGPUDescriptorHandleForHeapStart());

	_cmdList->RSSetViewports(1, &viewport);
	_cmdList->RSSetScissorRects(1, &scissorrect);

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	_cmdList->IASetVertexBuffers(0, 1, &vbView);
	_cmdList->IASetIndexBuffer(&ibView);

	_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

	//↓10月26日(水)の課題
	_cmdList->SetGraphicsRootSignature(rootsignature_Second);
	_cmdList->SetDescriptorHeaps(1, &texDescHeap_Second);
	_cmdList->SetGraphicsRootDescriptorTable(
		0,
		texDescHeap_Second->GetGPUDescriptorHandleForHeapStart());

	_cmdList->RSSetViewports(1, &viewport_Second);
	_cmdList->RSSetScissorRects(1, &scissorrect_Second);

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	_cmdList->IASetVertexBuffers(0, 1, &vbView_Second);
	_cmdList->IASetIndexBuffer(&ibView_Second);

	_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	//↑

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