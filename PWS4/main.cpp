#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include <vector>
#include<d3dcompiler.h>
#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace std;
using namespace DirectX;

//@brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
//@param format �t�H�[�}�b�g(%d�Ƃ�%f�Ƃ���)
//@param �ϒ���
//@remarks ���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���B
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif // _DEBUG
}

//�ʓ|�����Ǐ����Ȃ���΂����Ȃ��֐�
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//�E�B���h�E���j��ꂽ��Ă΂��
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);//OS�ɑ΂��āu�����̃A�v���͏I���v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//���̏�����s��
}

#ifdef _DEBUG
void EnableDebugLayer() 
{
	ID3D12Debug* debugLayer = nullptr;
	HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	if (!SUCCEEDED(result)) return;

	debugLayer->EnableDebugLayer();//デバッグレイヤーを有効化する
	debugLayer->Release();//有効化したらインターフェイスを開放する
}
#endif // ! _DEBUG


#ifdef _DEBUG
int main()
{

#else �A�N�e�B�u�łȂ��v���v���Z�b�T�@�u���b�N
#endif
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

//�E�B���h�E�N���X�̍쐬���o�^
WNDCLASSEX w = {};

w.cbSize = sizeof(WNDCLASSEX);
w.lpfnWndProc = (WNDPROC)WindowProcedure;//�R�[���o�b�N�֐��̎w��
w.lpszClassName = _T("DX12Sample");//�A�v���P�[�V�����N���X��
w.hInstance = GetModuleHandle(nullptr);//�n���h���̎擾

RegisterClassEx(&w);//�A�v���P�[�V�����N���X�i�E�B���h�E�N���X��OS�ɓ`����j

RECT wrc = { 0,0,window_width,window_height };//�E�B���h�E�T�C�Y��߂�

//�֐���g��ăE�B���h�E�̃T�C�Y��␳����
AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
//�E�B���h�E�I�u�W�F�N�g�̐���
HWND hwnd = CreateWindow(w.lpszClassName,
	_T("DX12�e�X�g"),
	WS_OVERLAPPEDWINDOW,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	wrc.right - wrc.left,
	wrc.bottom - wrc.top,
	nullptr,
	nullptr,
	w.hInstance,
	nullptr);//�ǉ�p�����[�^�[

#ifdef _DEBUG
EnableDebugLayer();
#endif // !_DEBUG

D3D_FEATURE_LEVEL levels[] =
{
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0,
};

#ifdef _DEBUG
auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif // _DEBUG

//�A�_�v�^�[�̗񋓗p
std::vector<IDXGIAdapter*>adapters;
//�����ɓ��̖��O��A�_�v�^�[�I�u�W�F�N�g�����
IDXGIAdapter* tmpAdapter = nullptr;
for (int i = 0;
	_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
	++i)
{
	adapters.push_back(tmpAdapter);
}
for (auto adpt : adapters)
{
	DXGI_ADAPTER_DESC adesc = {};
	adpt->GetDesc(&adesc);//�A�_�v�^�[���I�u�W�F�N�g�̎擾
	std::wstring strDesc = adesc.Description;
	//�T�������A�_�v�^�[�̖��O��m�F
	if (strDesc.find(L"NVIDIA") != std::string::npos)
	{
		tmpAdapter = adpt;
		break;
	}
}

//DirectX�f�o�C�X�̏���
D3D_FEATURE_LEVEL featureLevel;
for (auto lv : levels)
{
	if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
	{
		featureLevel = lv;
		break;//�����\�ȃo�[�W��������������烋�[�v��ł��؂�
	}
}

result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
	IID_PPV_ARGS(&_cmdAllocator));
result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
	_cmdAllocator, nullptr,
	IID_PPV_ARGS(&_cmdList));

D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
//�^�C���A�E�g�Ȃ�
cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
//�A�_�v�^�[��P�����g��Ȃ��Ƃ��͂O�ł悢
cmdQueueDesc.NodeMask = 0;
//�v���C�I���e�B�͓�Ɏw��Ȃ�
cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
//�R�}���h���X�g�ƍ��킹��
cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
//�L���[�쐬
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
//バックバッファーは伸び縮み可能
swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
//�t���b�v��͑��₩�ɂ͔j��
swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
//��Ɏw��Ȃ�
swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
//ウィンドウ、フルスクリーン切り替え可能
swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
result = _dxgiFactory->CreateSwapChainForHwnd(
	_cmdQueue, hwnd,
	&swapchainDesc, nullptr, nullptr,
	(IDXGISwapChain1**)&_swapchain);//�{����QueryInterface����p����
//IDXGISwapChain4*�ւ̕ϊ��`�F�b�N��邪�A
//�����ł͂킩��₷���d�����߂ɃL���X�g�őΉ�

D3D12_DESCRIPTOR_HEAP_DESC hespDesc = {};
hespDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
hespDesc.NodeMask = 0;
hespDesc.NumDescriptors = 2;//�\���̂Q��
hespDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//��Ɏw��Ȃ�

ID3D12DescriptorHeap* rtvHeaps = nullptr;
result = _dev->CreateDescriptorHeap(&hespDesc, IID_PPV_ARGS(&rtvHeaps));

DXGI_SWAP_CHAIN_DESC swcDesc = {};
result = _swapchain->GetDesc(&swcDesc);

std::vector<ID3D12Resource*>_backBuffers(swcDesc.BufferCount);
for (int idx = 0; idx < swcDesc.BufferCount; ++idx)
{
	result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));
	D3D12_CPU_DESCRIPTOR_HANDLE handle
		= rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
}

ID3D12Fence* _fence = nullptr;
UINT64 _fenceval = 0;
result = _dev->CreateFence(_fenceval, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

//ウィンドウ表示
ShowWindow(hwnd, SW_SHOW);

//頂点データ構造体
struct Vertex{

	XMFLOAT3 pos;//xyz座標
	XMFLOAT2 uv;//uv座標
};

Vertex vertices[] = {
	{{-0.4f,-0.7f,0.0f},{0.0f,1.0f}},//左下
	{{-0.4f,+0.7f,0.0f},{0.0f,0.0f}},//左上
	{{+0.4f,-0.7f,0.0f},{1.0f,1.0f}},//右下
	{{+0.4f,+0.7f,0.0f},{1.0f,0.0f}},//右上
};

unsigned short indices[] = {
	0,1,2,
	2,1,3
};

D3D12_HEAP_PROPERTIES heapprop = {};
heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

D3D12_RESOURCE_DESC resdesk = {};
resdesk.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
resdesk.Width = sizeof(vertices);//頂点情報が入るだけのサイズ
resdesk.Height = 1;
resdesk.DepthOrArraySize = 1;
resdesk.MipLevels = 1;
resdesk.Format = DXGI_FORMAT_UNKNOWN;
resdesk.SampleDesc.Count = 1;
resdesk.Flags = D3D12_RESOURCE_FLAG_NONE;
resdesk.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

ID3D12Resource* vertBuff = nullptr;

ID3D12Resource* idxBuff = nullptr;
//設定は、バッファーサイズ以外、頂点バッファーの設定を使いまわしてよい
resdesk.Width = sizeof(indices);
result = _dev->CreateCommittedResource(
	&heapprop,
	D3D12_HEAP_FLAG_NONE,
	&resdesk,
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&idxBuff));

//作ったバッファーにインデックスをコピー
unsigned short* mappedIdx = nullptr;
idxBuff->Map(0, nullptr, (void**)&mappedIdx);
std::copy(std::begin(indices), std::end(indices), mappedIdx);

//インデックスバッファービューを作成
D3D12_INDEX_BUFFER_VIEW ibView = {};
ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
ibView.Format = DXGI_FORMAT_R16_UINT;
ibView.SizeInBytes = sizeof(indices);

result = _dev->CreateCommittedResource(
	&heapprop,
	D3D12_HEAP_FLAG_NONE,
	&resdesk,
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&vertBuff));

Vertex* vertMap = nullptr;
result = vertBuff->Map(0, nullptr, (void**)&vertMap);
std::copy(std::begin(vertices), std::end(vertices), vertMap);
vertBuff->Unmap(0, nullptr);

D3D12_VERTEX_BUFFER_VIEW vbView = {};
vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファーの仮想アドレス
vbView.SizeInBytes = sizeof(vertices);//全バイト数
vbView.StrideInBytes = sizeof(vertices[0]);//1頂点当たりのバイト数

ID3DBlob* _vsBlob = nullptr;
ID3DBlob* _psBlob = nullptr;

ID3DBlob* errorBlob = nullptr;
result = D3DCompileFromFile(
	L"BasicVertexShader.hlsl",//シェーダー名
	nullptr,//defineはなし
	D3D_COMPILE_STANDARD_FILE_INCLUDE,//インクルードはデフォ
	"BasicVS","vs_5_0",//関数はBasicVS、対象シェーダーはvs_5_0
	D3DCOMPILE_DEBUG|D3DCOMPILE_SKIP_OPTIMIZATION,
	0,
	&_vsBlob, &errorBlob);//エラー時はerrorBlobにメッセージを送る
if (FAILED(result)) {//この場合はエラーメッセージが入ってこない
	if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
		::OutputDebugStringA("ファイルが見当たりません");
	}
	else {
		std::string errstr;
		errstr.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";
		OutputDebugStringA(errstr.c_str());
	}
	exit(1);//行儀悪
}

result = D3DCompileFromFile(
	L"BasicPixelShader.hlsl",//シェーダー名
	nullptr,//defineはなし
	D3D_COMPILE_STANDARD_FILE_INCLUDE,//インクルードはデフォ
	"BasicPS", "ps_5_0",//関数はBasicPS、対象シェーダーはps_5_0
	D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
	0,
	&_psBlob, &errorBlob);//エラー時はerrorBlobにメッセージを送る
if (FAILED(result)) {//この場合はエラーメッセージが入ってこない
	if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
		::OutputDebugStringA("ファイルが見当たりません");
	}
	else {
		std::string errstr;
		errstr.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(), errstr.begin());
		errstr += "\n";
		OutputDebugStringA(errstr.c_str());
	}
	exit(1);//行儀悪
}

D3D12_INPUT_ELEMENT_DESC inputLayout[] =
{
	{
		"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
	},
	{//uv（追加）
		"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,
		0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
    },
};

D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

gpipeline.pRootSignature = nullptr;//あとで設定する

gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

//デフォルトのサンプルマスクを表す定数(0xffffffff)
gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

//まだアンチエイリアスは使わないためfalse
gpipeline.RasterizerState.MultisampleEnable = false;

gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
gpipeline.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に

gpipeline.BlendState.AlphaToCoverageEnable = false;
gpipeline.BlendState.IndependentBlendEnable = false;

D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
//ひとまず加算やαブレンディングは使用しない
renderTargetBlendDesc.BlendEnable = false;
//ひとまず論理演算は使用しない
renderTargetBlendDesc.LogicOpEnable = false;
renderTargetBlendDesc.RenderTargetWriteMask =
D3D12_COLOR_WRITE_ENABLE_ALL;

gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列の要素数

gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし

gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

gpipeline.NumRenderTargets = 1;//今は1つのみ
gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

gpipeline.SampleDesc.Count = 1;//サンプルは1ピクセルにつき1
gpipeline.SampleDesc.Quality = 0;//クォリティは最低

D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

D3D12_DESCRIPTOR_RANGE descTblRange = {};
descTblRange.NumDescriptors = 1;//テクスチャ1つ
descTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//種別はテクスチャ
descTblRange.BaseShaderRegister = 0;//0番スロットから
descTblRange.OffsetInDescriptorsFromTableStart =
D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

D3D12_ROOT_PARAMETER rootparam = {};
rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
//ピクセルシェーダーから見える
rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
//ディスクリプタアレンジのアドレス
rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
//ディスクリプタアレンジ数
rootparam.DescriptorTable.NumDescriptorRanges = 1;

rootSignatureDesc.pParameters = &rootparam;//ルートパラメーターの先頭アドレス
rootSignatureDesc.NumParameters = 1;//ルートパラメーター数

D3D12_STATIC_SAMPLER_DESC samplerDesc{};
samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横方向の繰り返し
samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦方向の繰り返し
samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行きの繰り返し
samplerDesc.BorderColor =
D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//ボーダーは黒
samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//線形補完
samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
samplerDesc.MinLOD = 0.0f;//ミップマップ最小値
samplerDesc.ShaderVisibility =
D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダーから見える
samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//リサンプリングしない

rootSignatureDesc.pStaticSamplers = &samplerDesc;
rootSignatureDesc.NumStaticSamplers = 1;

rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
//ピクセルシェーダーから見える
rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
//ピクセルシェーダーから見える
rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
//ディスクリプタアレンジのアドレス
rootparam.DescriptorTable.pDescriptorRanges = &descTblRange;
//ディスクリプタアレンジ数
rootparam.DescriptorTable.NumDescriptorRanges = 1;

rootSignatureDesc.pParameters = &rootparam;//ルートパラメーターの先頭アドレス
rootSignatureDesc.NumParameters = 1;//ルートパラメーター数


ID3DBlob* rootSigBlob = nullptr;
result = D3D12SerializeRootSignature(
	&rootSignatureDesc,//ルートシグネチャ設定
	D3D_ROOT_SIGNATURE_VERSION_1_0,//ルートシグネチャバージョン
	&rootSigBlob,//シェーダーを作った時と同じ
	&rootSigBlob);//エラー処理も同じ

ID3D12RootSignature* rootsignature = nullptr;
result = _dev->CreateRootSignature(
	0,//nodmask。0でよい
	rootSigBlob->GetBufferPointer(),//シェーダーの時と同様
	rootSigBlob->GetBufferSize(),//シェーダーの時と同様
	IID_PPV_ARGS(&rootsignature));
rootSigBlob->Release();//不要になったので開放

gpipeline.pRootSignature = rootsignature;

ID3D12PipelineState* _pipelinestate = nullptr;
result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

D3D12_VIEWPORT viewport = {};
viewport.Width = window_width;//出力先の幅
viewport.Height = window_height;//出力先の高さ
viewport.TopLeftX = 0;//出力先の左上座標X
viewport.TopLeftY = 0;//出力先の左上座標Y
viewport.MaxDepth = 1.0f;//深度最大値
viewport.MinDepth = 0.0f;//深度最小値

D3D12_RECT scissorrect = {};
scissorrect.top = 0;//切り抜き上座標
scissorrect.left = 0;//切り抜き左座標
scissorrect.right = scissorrect.left + window_width;//切り抜き右座標
scissorrect.bottom = scissorrect.top + window_height;//切り抜き下座標

struct TexRGBA
{
	unsigned char R, G, B, A;
};

std::vector<TexRGBA>texturedata(256 * 256);
for (auto& rgba : texturedata)
{
	rgba.R = rand() % 256;
	rgba.G = rand() % 256;
	rgba.B = rand() % 256;
	rgba.A = 255;//αは1.0とする
}

//WritetoSubresouce で転送するためヒープ設定
D3D12_HEAP_PROPERTIES texHeapProp = {};
//特殊な設定なのでDEFAULTでもUPLOADでもない
texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
//ライトバック
texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
//転送はL0、つまりCPU側から直接行う
texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
//単一アダプターのための０
texHeapProp.CreationNodeMask = 0;
texHeapProp.VisibleNodeMask = 0;

D3D12_RESOURCE_DESC resDesc = {};
resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//RGBAフォーマット
resDesc.Width = 256;//幅
resDesc.Height = 256;//高さ
resDesc.DepthOrArraySize = 1;//２Dで配列もないので１
resDesc.SampleDesc.Count = 1;//通常テクスチャなのでアンチエイリアシングしない
resDesc.SampleDesc.Quality = 0;//クオリティは最低
resDesc.MipLevels = 1;//ミップマップしないのでミップ数は1つ
resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//２Dテクスチャ用
resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトは決定しない
resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//特にフラグなし

ID3D12Resource* texbuff = nullptr;
result = _dev->CreateCommittedResource(
	&texHeapProp,
	D3D12_HEAP_FLAG_NONE,//特に指定なし
	&resDesc,
	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,//テクスチャ用指定
	nullptr,
	IID_PPV_ARGS(&texbuff));

result = texbuff->WriteToSubresource(
	0,
	nullptr,//全領域へコピー
	texturedata.data(),//元データアドレス
	sizeof(TexRGBA) * 256,//1ラインサイズ
	sizeof(TexRGBA) * texturedata.size()//全サイズ
);

result = texbuff->WriteToSubresource(
	0,
	nullptr,//全領域へコピー
	texturedata.data(),//元データアドレス
	sizeof(TexRGBA) * 256,//1ラインサイズ
	sizeof(TexRGBA) * (UINT)texturedata.size()//全サイズ
);

ID3D12DescriptorHeap* texDescHeap = nullptr;
D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
//シェーダーから見えるように
descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//マスクは０
descHeapDesc.NodeMask = 0;
//ビューは今のところ1つだけ
descHeapDesc.NumDescriptors = 1;
//シェーダーリソースビュー用
descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//生成
result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&texDescHeap));

D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA(0.0f～1.0fに正規化)
srvDesc.Shader4ComponentMapping =
D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//後述
srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//２Dテクスチャー
srvDesc.Texture2D.MipLevels = 1;//ミップマップは使用しないので1

_dev->CreateShaderResourceView(
	texbuff,//ビューと関連付けるバッファー
	&srvDesc,//先ほど設定したテクスチャ設定情報
	texDescHeap->GetCPUDescriptorHandleForHeapStart()//ヒープのどこに割り当てるか
);

while (true)
{
	MSG msg;
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {

		//�A�v���P�[�V�������I���Ƃ���messeage��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT) {
			break;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	//DirectX処理
		//バックバッファのインデックスを取得
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;// 遷移
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;// 特に指定なし
	BarrierDesc.Transition.pResource = _backBuffers[bbIdx];// バックバッファーリソース
	BarrierDesc.Transition.Subresource = 0;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;// 直前はPRESENT 状態
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;// 今からRT状態
	_cmdList->ResourceBarrier(1, &BarrierDesc);// バリア指定実行


	_cmdList->SetPipelineState(_pipelinestate);

	//レンダーターゲットを指定
	auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

	//画面クリア
	float clearcolor[] = { 0.0f,0.0f,0.3f,1.0f };//白色
	_cmdList->ClearRenderTargetView(rtvH, clearcolor, 0, nullptr);

	_cmdList->SetGraphicsRootSignature(rootsignature);

	_cmdList->SetDescriptorHeaps(1, &texDescHeap);
	_cmdList->SetGraphicsRootDescriptorTable(
		0,//ルートパラメーターインデックス
		texDescHeap->GetGPUDescriptorHandleForHeapStart());//ヒープアドレス

	_cmdList->RSSetViewports(1, &viewport);
	_cmdList->RSSetScissorRects(1, &scissorrect);

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	_cmdList->IASetVertexBuffers(0, 1, &vbView);
	_cmdList->IASetIndexBuffer(&ibView);

	_cmdList->DrawIndexedInstanced(6, 1, 0, 0,0);

	// 前後だけ入れ替える
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	//命令のクローズ
	_cmdList->Close();

	//コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);

	//待ち
	_cmdQueue->Signal(_fence, ++_fenceval);

	if (_fence->GetCompletedValue() != _fenceval) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceval, event);//イベントハンドルの取得
		WaitForSingleObject(event, INFINITE);//イベントが終了するまで無限に待つ
		CloseHandle(event);//イベントハンドルを閉じる
	}
	_cmdAllocator->Reset();//キューをクリア
	_cmdList->Reset(_cmdAllocator, _pipelinestate);//再びコマンドリストをためる準備

	//�t���b�v
	_swapchain->Present(1, 0);
}

//���N���X�͎g��Ȃ��̂œo�^�����
UnregisterClass(w.lpszClassName, w.hInstance);

return 0;
}