#pragma once
#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<map>
#include<string>
#include<wrl.h>

class Dx12Wrapper;
class PMDRenderer;

class PMDActor
{
	friend PMDRenderer;
private:
	PMDRenderer& _renderer;
	Dx12Wrapper& _dx12;

	//シェーダー側に投げられるマテリアルデータ
	struct MaterialForHlsl
	{
		DirectX::XMFLOAT3 diffuse;//ディフューズ色
		float alpha;//ディフューズα
		DirectX::XMFLOAT3 specular;//スぺキュラ色
		float specularity;//スぺキュラの強さ(乗算値)
		DirectX::XMFLOAT3 ambient;//アビエント色
	};

	//それ以外のマテリアルデータ
	struct AdditionalMaterial
	{
		std::string texPath;
		int toonIdx;
		bool edgeFlg;
	};

	//全体をまとめるデータ
	struct Material
	{
		unsigned int indicesNum;
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	std::vector<Material> materials;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _textureResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _sphResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _spaResources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _toonResources;

	Microsoft::WRL::ComPtr<ID3D12Resource> _materialBuff = nullptr;
	unsigned int _materialNum;//マテリアル数
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _materialDescHeap = nullptr;//マテリアルヒープ

	Microsoft::WRL::ComPtr<ID3D12Resource> _vertBuff = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> _idxBuff = nullptr;
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	struct Transform {
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};
	Transform _transform;
	Transform* _mappedTransform = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformBuff = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _transformMat = nullptr;//座標変換行列(今はワールドのみ)
	
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _transformHeap = nullptr;//座標変換ヒープ

	float _angle;//テスト用Y軸回転

public:
	HRESULT LoadPMDFile(const char* path);//PMDファイルのロード
	void CreateMaterialData();//読み込んだマテリアルをもとにマテリアルバッファを作成
	void CreateMaterialAndTextureView();//マテリアル＆テクスチャのビューを作成
	void CreateTransformView();

public:
	PMDActor(const char* filepath, PMDRenderer& renderer);
	~PMDActor();
	PMDActor* Clone();
	void Update();
	void Draw();

};