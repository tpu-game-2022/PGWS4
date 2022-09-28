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
	printf(format, valist);
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
#ifdef _DEBUG
	EnableDebugLayer();
#endif // !_DEBUG

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

HRESULT D3D12CreateDevice(
	IUnknown * pAdapter,//�ЂƂ܂���nullptr��OK
	D3D_FEATURE_LEVEL MnimumFeatureLevel,//�Œ��K�v�ȃt�B�[�`���[���x��
	REFIID riid,
	void** ppDevice
);

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
swapchainDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
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

XMFLOAT3 vertics[] = {
	{-1.0f,-1.0f,0.0f},//左下
	{-1.0f,+1.0f,0.0f},//左上
	{+1.0f,-1.0f,0.0f},//右下
};

D3D12_HEAP_PROPERTIES heapprop = {};
heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

D3D12_RESOURCE_DESC resdesk = {};
resdesk.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
resdesk.Width = sizeof(vertics);//頂点情報が入るだけのサイズ
resdesk.Height = 1;
resdesk.DepthOrArraySize = 1;
resdesk.MipLevels = 1;
resdesk.Format = DXGI_FORMAT_UNKNOWN;
resdesk.SampleDesc.Count = 1;
resdesk.Flags = D3D12_RESOURCE_FLAG_NONE;
resdesk.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

ID3D12Resource* vertBuff = nullptr;

result = _dev->CreateCommittedResource(
	&heapprop,
	D3D12_HEAP_FLAG_NONE,
	&resdesk,
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&vertBuff));

XMFLOAT3* vertMap = nullptr;
result = vertBuff->Map(0, nullptr, (void**)&vertMap);
std::copy(std::begin(vertics), std::end(vertics), vertMap);
vertBuff->Unmap(0, nullptr);

D3D12_VERTEX_BUFFER_VIEW vbView = {};
vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファーの仮想アドレス
vbView.SizeInBytes = sizeof(vertics);//全バイト数
vbView.StrideInBytes = sizeof(vertics[0]);//1頂点当たりのバイト数

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
	L"BasicVertexShader.hlsl",//シェーダー名
	nullptr,//defineはなし
	D3D_COMPILE_STANDARD_FILE_INCLUDE,//インクルードはデフォ
	"BasicPS", "ps_5_0",//関数はBasicVS、対象シェーダーはvs_5_0
	D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
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

D3D12_INPUT_ELEMENT_DESC inputLayout[] =
{
	{
		"POSSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
};

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
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;//遷移
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;//特に指定なし
	BarrierDesc.Transition.pResource = _backBuffers[bbIdx];//バックバッファーリソース
	BarrierDesc.Transition.Subresource = 0;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;//直前はPRESENT状態
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;//今からRT状態
	_cmdList->ResourceBarrier(1, &BarrierDesc);//バリア実行

	//レンダーターゲットを指定
	auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

	//画面クリア
	float clearcolor[] = { 1.0f,1.0f,0.0f,1.0f };//黄色
	_cmdList->ClearRenderTargetView(rtvH, clearcolor, 0, nullptr);

	//前後だけ入れ替える
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
	_cmdList->Reset(_cmdAllocator, nullptr);//再びコマンドリストをためる準備

	//�t���b�v
	_swapchain->Present(1, 0);
}

//���N���X�͎g��Ȃ��̂œo�^�����
UnregisterClass(w.lpszClassName, w.hInstance);

return 0;
}