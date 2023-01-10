#include "PMDActor.h"
#include "PMDRenderer.h"
#include "Dx12Wrapper.h"
#include <d3dx12.h>

using namespace DirectX;
using namespace Microsoft::WRL;

static  inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

static std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath,
	const char* texPath)
{
	int pathIndex1 = static_cast<int>(modelPath.rfind('/'));
	int pathIndex2 = static_cast<int>(modelPath.rfind('\\'));
	int pathIndex = max(pathIndex1, pathIndex2);
	std::string folderPath = modelPath.substr(0, pathIndex + 1);
	return folderPath + texPath;
}

static std::pair<std::string, std::string>SplitFileName(
	const std::string& path, const char splitter = '*')
{
	int idx = static_cast<int>(path.find(splitter));
	std::pair<std::string, std::string>ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(
		idx + 1, path.length() - idx - 1);
	return ret;
}

static std::string GetExtension(const std::string& path) {
	int idx = static_cast<int>(path.rfind('.'));
	return path.substr(
		idx + 1, path.length() - idx - 1);
}

//  Model/巡音ルカ.pmd 初音ミクmetal.pmd 初音ミク.pmd
HRESULT PMDActor::LoadPMDFile(const char* path) {

	//PMD ヘッダー構造体
	struct PMDHeader
	{
		float version;//00 00 80 3F == 1.00
		char model_name[20];//モデル名
		char comment[256];// モデルコメント
	};

	char signature[3] = {};//シグネチャ
	PMDHeader pmdheader = {};

	std::string strModelPath = path;
	FILE* fp;
	fopen_s(&fp, strModelPath.c_str(), "rb");

	if (fp == nullptr) {
		assert(0);	return ERROR_FILE_NOT_FOUND;
	}

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	unsigned int vertNum;//頂点数を取得
	fread(&vertNum, sizeof(vertNum), 1, fp);


#pragma pack(1)
	//PMDマテリアル構造体
	struct PMDMaterial
	{
		XMFLOAT3 diffuse;//ディフューズ色
		float alpha;//ディフューズα
		float specularity;//スぺキュラの強さ(乗算値)
		XMFLOAT3 specular;//スぺキュラ色
		XMFLOAT3 ambient;//アビエント色
		unsigned char toonIdx;//トゥーン番号(後述)
		unsigned char edgeFlg;

		unsigned int indicesNum;
								
		char texFilePath[20];  
	};
#pragma pack()

	constexpr unsigned int pmdvertex_size = 38;//頂点1つあたりのサイズ
#pragma pack(push,1)
	struct PMD_VERTEX
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT2 uv;
		uint16_t bone_no[2];
		uint8_t weight;
		uint8_t EdgeFlag;
		uint16_t dummy;
	};
#pragma pack(pop)

	std::vector<PMD_VERTEX> vertices(vertNum);//バッファーの確保
	for (unsigned int i = 0; i < vertNum; i++) {
		fread(&vertices[i], pmdvertex_size, 1, fp);
	}

	auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(PMD_VERTEX));

	ComPtr<ID3D12Device> dev = _dx12.Device();

	ThrowIfFailed(_dx12.Device()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,//サイズ変更
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_vertBuff.ReleaseAndGetAddressOf())));

	PMD_VERTEX* vertMap = nullptr;
	ThrowIfFailed(_vertBuff->Map(0, nullptr, (void**)&vertMap));
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	_vertBuff->Unmap(0, nullptr);

	_vbView.BufferLocation = _vertBuff->GetGPUVirtualAddress();//バッファーの仮想アドレス
	_vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(PMD_VERTEX));//全バイト数
	_vbView.StrideInBytes = sizeof(vertices[0]);//１頂点あたりのバイト数

	unsigned int indicesNum;//インデックス数
	fread(&indicesNum, sizeof(indicesNum), 1, fp);

	std::vector<unsigned short> indices;
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	resdesc.Width = indices.size() * sizeof(indices[0]);
	ThrowIfFailed(dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_idxBuff.ReleaseAndGetAddressOf())));

	//作ったバッファーにインデックスデータををコピー
	unsigned short* mapppedIdx = nullptr;
	_idxBuff->Map(0, nullptr, (void**)&mapppedIdx);
	std::copy(std::begin(indices), std::end(indices), mapppedIdx);
	_idxBuff->Unmap(0, nullptr);

	//インデックスバッファービューを作成
	_ibView.BufferLocation = _idxBuff->GetGPUVirtualAddress();
	_ibView.Format = DXGI_FORMAT_R16_UINT;
	_ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

	fread(&_materialNum, sizeof(_materialNum), 1, fp);

	std::vector<PMDMaterial> pmdMaterials(_materialNum);

	fread(
		pmdMaterials.data(),
		pmdMaterials.size() * sizeof(PMDMaterial),
		1,
		fp
	);

	materials.resize(pmdMaterials.size());
	//コピー
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		materials[i].indicesNum = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse = pmdMaterials[i].diffuse;
		materials[i].material.alpha = pmdMaterials[i].alpha;
		materials[i].material.specular = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient = pmdMaterials[i].ambient;
		materials[i].additional.toonIdx = pmdMaterials[i].toonIdx;
	}

	for (int i = 0; i < pmdMaterials.size(); ++i)
	{

		//トゥーンリソースの読み込み toon2(チャレンジ用黒濃いめのトゥーン)
		std::string toonFilePath = "toon/";
		char toonFileName[16];
		sprintf_s(toonFileName, "toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		toonFilePath += toonFileName;
		_toonResources[i] = _dx12.GetTextureByPath(toonFilePath.c_str());

		if (strlen(pmdMaterials[i].texFilePath) == 0) { continue; }

		std::string texFileName = pmdMaterials[i].texFilePath;
		std::string sphFileName = "";
		std::string spaFileName = "";

		if (std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{//スプリッタがある

			auto namepair = SplitFileName(texFileName);

			if (GetExtension(namepair.first) == "sph")
			{
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa")
			{
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else
			{
				texFileName = namepair.first;//他の拡張子でもとにかく最初の物を入れておく
				if (GetExtension(namepair.second) == "sph")
				{
					sphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa")
				{
					spaFileName = namepair.second;
				}
			}
		}
		else {
			std::string ext = GetExtension(pmdMaterials[i].texFilePath);
			if (ext == "sph")
			{
				sphFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else if (ext == "spa")
			{
				spaFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
		}

		//モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る。
		auto texFilePath = GetTexturePathFromModelAndTexPath(
			strModelPath,texFileName.c_str());
		_textureResources[i] = _dx12.GetTextureByPath(texFilePath.c_str());

		if (sphFileName != "") {
			auto sphFilePath = GetTexturePathFromModelAndTexPath(
				strModelPath,sphFileName.c_str());
			_sphResources[i] =  _dx12.GetTextureByPath(sphFilePath.c_str());
		}
		if (spaFileName != "") {
			auto spaFilePath = GetTexturePathFromModelAndTexPath(
				strModelPath,spaFileName.c_str());
			_spaResources[i] =  _dx12.GetTextureByPath(spaFilePath.c_str());
		}
	}

	fclose(fp);

	return S_OK;
}

void PMDActor::CreateMaterialData() {
	//マテリアルバッファーの作成
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	const D3D12_HEAP_PROPERTIES heapPropMat =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	const D3D12_RESOURCE_DESC resDescMat = CD3DX12_RESOURCE_DESC::Buffer(
		materialBuffSize * _materialNum);
	auto _dev = _dx12.Device();
	ThrowIfFailed(_dev->CreateCommittedResource(
		&heapPropMat,
		D3D12_HEAP_FLAG_NONE,
		&resDescMat,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_materialBuff.ReleaseAndGetAddressOf())
	));

	//マップマテリアルにコピー
	char* mapMaterial = nullptr;
	ThrowIfFailed(_materialBuff->Map(0, nullptr, (void**)&mapMaterial));
	for (auto& m : materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material;
		mapMaterial += materialBuffSize;
	}
	_materialBuff->Unmap(0, nullptr);
}

void PMDActor::CreateMaterialAndTextureView() {

	ID3D12Device* _dev = _dx12.Device().Get();

	D3D12_DESCRIPTOR_HEAP_DESC materialDescHeapDesc = {};
	materialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	materialDescHeapDesc.NodeMask = 0;
	materialDescHeapDesc.NumDescriptors = _materialNum * 5; // マテリアル数分（定数1つ、テクスチャ4つ）
	materialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ThrowIfFailed(_dev->CreateDescriptorHeap(
		&materialDescHeapDesc, IID_PPV_ARGS(_materialDescHeap.ReleaseAndGetAddressOf())));

	UINT materialBuffSize = (sizeof(MaterialForHlsl) + 0xff) & ~0xff;
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = _materialBuff->GetGPUVirtualAddress(); // バッファーアドレス
	matCBVDesc.SizeInBytes = static_cast<UINT>(materialBuffSize); // マテリアルの256 アライメントサイズ

	//通常テクスチャビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE matDescHeapH(_materialDescHeap->GetCPUDescriptorHandleForHeapStart());
	UINT incSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (UINT i = 0; i < _materialNum; ++i)
	{
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += incSize;
		matCBVDesc.BufferLocation += static_cast<UINT>(materialBuffSize);

		if (_textureResources[i] == nullptr) {
			srvDesc.Format = _renderer._whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				_renderer._whiteTex.Get(),&srvDesc,matDescHeapH);
		}
		else
		{
			srvDesc.Format = _textureResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				_textureResources[i].Get(),&srvDesc,matDescHeapH);
		}

		matDescHeapH.ptr += incSize;

		if (_sphResources[i] == nullptr) {
			srvDesc.Format = _renderer._whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				_renderer._whiteTex.Get(),&srvDesc,matDescHeapH);
		}
		else
		{
			srvDesc.Format = _sphResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				_sphResources[i].Get(),&srvDesc,matDescHeapH);
		}
		matDescHeapH.ptr += incSize;

		if (_spaResources[i] == nullptr) {
			srvDesc.Format = _renderer._blackTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				_renderer._blackTex.Get(),&srvDesc,matDescHeapH);
		}
		else
		{
			srvDesc.Format = _spaResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				_spaResources[i].Get(),&srvDesc,matDescHeapH);
		}
		matDescHeapH.ptr += incSize;

		if (_toonResources[i] == nullptr) {
			srvDesc.Format = _renderer._gradTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				_renderer._gradTex.Get(),&srvDesc,matDescHeapH);
		}
		else
		{
			srvDesc.Format = _toonResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				_toonResources[i].Get(),&srvDesc,matDescHeapH);
		}
		matDescHeapH.ptr += incSize;

	}

}

void PMDActor::CreateTransformView()
{
	unsigned int buffSize = (sizeof(Transform) + 0xff) & ~0xff;
	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);

	ThrowIfFailed(_dx12.Device()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_transformBuff.ReleaseAndGetAddressOf())
	));

	ThrowIfFailed(_transformBuff->Map(0, nullptr, (void**)&_mappedTransform));
	*_mappedTransform = _transform;

	D3D12_DESCRIPTOR_HEAP_DESC transformDescHeapDesc = {};
	transformDescHeapDesc.NumDescriptors = 1;
	transformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transformDescHeapDesc.NodeMask = 0;

	transformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(_dx12.Device()->CreateDescriptorHeap(&transformDescHeapDesc, IID_PPV_ARGS(_transformHeap.ReleaseAndGetAddressOf())));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _transformBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = buffSize;
	_dx12.Device()->CreateConstantBufferView(&cbvDesc, _transformHeap->GetCPUDescriptorHandleForHeapStart());
}

void* PMDActor::Transform::operator new(size_t size) {
	return _aligned_malloc(size, 16);
}

PMDActor::PMDActor(const char* filepath, PMDRenderer& renderer) :
	_renderer(renderer),
	_dx12(renderer._dx12),
	_angle(0.0f)
{
	ThrowIfFailed(LoadPMDFile(filepath));

	CreateMaterialData();
	CreateMaterialAndTextureView();

	_transform.world = XMMatrixIdentity();
	CreateTransformView();
}

PMDActor::~PMDActor()
{
}

void PMDActor::Update()
{
	_angle += 0.5f;
	_mappedTransform->world = XMMatrixRotationY(_angle);
}

void PMDActor::Draw()
{
	ID3D12GraphicsCommandList* cmdList = _dx12.CommandList().Get();
	ID3D12DescriptorHeap* transheaps[] = { _transformHeap.Get() };
	cmdList->SetDescriptorHeaps(1, transheaps);
	cmdList->SetGraphicsRootDescriptorTable(1, _transformHeap->GetGPUDescriptorHandleForHeapStart());
	cmdList->IASetVertexBuffers(0, 1, &_vbView);
	cmdList->IASetIndexBuffer(&_ibView);
	ID3D12DescriptorHeap* mdh[] = { _materialDescHeap.Get() };
	cmdList->SetDescriptorHeaps(1, mdh);
	D3D12_GPU_DESCRIPTOR_HANDLE materialH = _materialDescHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;
	UINT cbvsrvIncSize = _dx12.Device()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

	for (Material& m : materials)
	{
		cmdList->SetGraphicsRootDescriptorTable(2, materialH);
		cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);
		materialH.ptr += cbvsrvIncSize;
		idxOffset += m.indicesNum;
	}
}



