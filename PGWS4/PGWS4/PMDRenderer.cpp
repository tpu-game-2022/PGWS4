#include "PMDRenderer.h"
#include <d3dcompiler.h>
#include "Dx12Wrapper.h"

using namespace Microsoft::WRL;

static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}


static ComPtr<ID3D12Resource> CreateMonoTexture(ID3D12Device* dev, unsigned int val)
{
	// リソース
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	ComPtr<ID3D12Resource> whiteBuff = nullptr;
	auto result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(whiteBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) { return nullptr; }

	// 初期化データ
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), val); // 全部valで埋める

	// データ転送
	result = whiteBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		static_cast<UINT>(data.size()));

	return whiteBuff;
}

static ComPtr<ID3D12Resource> CreateWhiteTexture(ID3D12Device* dev)
{
	return CreateMonoTexture(dev, 0xff);
}

static ComPtr<ID3D12Resource> CreateBlackTexture(ID3D12Device* dev)
{
	return CreateMonoTexture(dev, 0x00);
}

// デフォルトグラデーションテクスチャ
static ComPtr<ID3D12Resource> CreateGrayGradationTexture(ID3D12Device* dev)
{
	// リソース
	D3D12_HEAP_PROPERTIES texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	ComPtr<ID3D12Resource> gradBuff = nullptr;
	HRESULT result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(gradBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) { return nullptr; }

	// 初期化データ
	std::vector<unsigned int> data(4 * 256);	// 上が白くて下が黒いテクスチャデータを作成
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4)
	{
		unsigned int col = (0xff << 24) | RGB(c, c, c);//RGBAが逆並びのためRGBマクロと0xff<<24を用いて表す
		std::fill(it, it + 4, col);
		--c;
	}

	// データ転送
	result = gradBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * static_cast<UINT>(sizeof(unsigned int)),
		static_cast<UINT>(sizeof(unsigned int) * data.size()));

	return gradBuff;
}

ComPtr<ID3D12RootSignature> PMDRenderer::CreateRootSignature(ID3D12Device* dev)
{
	CD3DX12_DESCRIPTOR_RANGE descTblRanges[4] = {};// テクスチャと定数の2つ
	descTblRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);// 定数[b0] 座標変換用
	descTblRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);//定数[b1](ワールド、ボーン用)
	descTblRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);//定数[b2](マテリアル用)
	descTblRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);// テクスチャ4つ

	CD3DX12_ROOT_PARAMETER rootparam[3] = {};
	rootparam[0].InitAsDescriptorTable(1, &descTblRanges[0]);//ビュープロジェクション変換
	rootparam[1].InitAsDescriptorTable(1, &descTblRanges[1]);//ワールド・ボーン変換
	rootparam[2].InitAsDescriptorTable(2, &descTblRanges[2]);//マテリアル周り

	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(3, rootparam, 2, samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ThrowIfFailed(D3D12SerializeRootSignature(
		&rootSignatureDesc, // ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0, // ルートシグネチャバージョン
		&rootSigBlob, // シェーダーを作ったときと同じ
		&errorBlob)); // エラー処理も同じ

	ComPtr<ID3D12RootSignature> rootsignature = nullptr;
	ThrowIfFailed(dev->CreateRootSignature(
		0, // nodemask。0 でよい
		rootSigBlob->GetBufferPointer(), // シェーダーのときと同様
		rootSigBlob->GetBufferSize(), // シェーダーのときと同様
		IID_PPV_ARGS(rootsignature.ReleaseAndGetAddressOf())));

	return rootsignature;
}

ComPtr<ID3DBlob> PMDRenderer::LoadShader(LPCWSTR pFileName, LPCSTR pEntrypoint, LPCSTR pTarget)
{
	ComPtr<ID3DBlob> sBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT result = D3DCompileFromFile(
		pFileName, // シェーダー名
		nullptr, // define はなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
		pEntrypoint, pTarget, // 関数はBasicVS、対象シェーダーはvs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用および最適化なし
		0,
		&sBlob, &errorBlob); // エラー時はerrorBlob にメッセージが入る

	if (FAILED(result)) {
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
		exit(1);//行儀悪いかな…
	}

	return sBlob;
}

ComPtr<ID3D12PipelineState> PMDRenderer::CreateBasicGraphicsPipeline(
	ID3D12Device* dev, ID3DBlob* vsBlob, ID3DBlob* psBlob, ID3D12RootSignature* rootsignature)
{
	ComPtr<ID3D12PipelineState> pipelinestate = nullptr;

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONE_NO", 0, DXGI_FORMAT_R16G16_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "EDGE_FLG", 0, DXGI_FORMAT_R8_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONENO", 0, DXGI_FORMAT_R16G16_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	gpipeline.pRootSignature = nullptr;// あとで設定する

	gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob);
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob);

	// デフォルトのサンプルマスクを表す定数（0xffffffff）
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングしない

	gpipeline.DepthStencilState.DepthEnable = true; // 深度バッファーを使う
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 書き込む
	gpipeline.DepthStencilState.DepthFunc =
		D3D12_COMPARISON_FUNC_LESS; // 小さいほうを採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeline.InputLayout.pInputElementDescs = inputLayout; // レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout); // レイアウト配列の要素数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // ストリップ時のカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;// 点で構成

	gpipeline.NumRenderTargets = 1;//今は１つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低

	gpipeline.pRootSignature = rootsignature;

	ThrowIfFailed(dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(pipelinestate.ReleaseAndGetAddressOf())));

	return pipelinestate;
}


PMDRenderer::PMDRenderer(Dx12Wrapper& dx12) :_dx12(dx12)
{
	ID3D12Device* dev = dx12.Device().Get();

	// ディフォルトテクスチャ生成
	_whiteTex = CreateWhiteTexture(dev);
	_blackTex = CreateBlackTexture(dev);
	_gradTex = CreateGrayGradationTexture(dev);

	// 描画パイプライン設定
	_rootsignature = CreateRootSignature(dev);
	ComPtr<ID3DBlob> vsBlob = LoadShader(L"BasicVertexShader.hlsl", "BasicVS", "vs_5_0");
	ComPtr<ID3DBlob> psBlob = LoadShader(L"BasicPixelShader.hlsl", "BasicPS", "ps_5_0");
	_pipelinestate = CreateBasicGraphicsPipeline(dev, vsBlob.Get(), psBlob.Get(), _rootsignature.Get());
}

PMDRenderer::~PMDRenderer()
{

}