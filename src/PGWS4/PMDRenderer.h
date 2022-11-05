#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>

class Dx12Wrapper;
class PMDActor;

class PMDRenderer
{
	friend PMDActor;
private:
	Dx12Wrapper& _dx12;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelinestate = nullptr;//PMD用パイプライン
	Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootsignature = nullptr;//PMD用ルートシグネチャ

	// デフォルトのテクスチャ(白、黒、グレイスケールグラデーション)
	Microsoft::WRL::ComPtr<ID3D12Resource> _whiteTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _blackTex = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> _gradTex = nullptr;

private:
	// 初期化の部分処理
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* dev);
	static Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(LPCWSTR pFileName, LPCSTR pEntrypoint, LPCSTR pTarget);
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateBasicGraphicsPipeline(
		ID3D12Device* dev, ID3DBlob* vsBlob, ID3DBlob* psBlob, ID3D12RootSignature* rootsignature);
public:
	PMDRenderer(Dx12Wrapper& dx12);
	~PMDRenderer();

	ID3D12PipelineState* GetPipelineState() { return _pipelinestate.Get(); }
	ID3D12RootSignature* GetRootSignature() { return _rootsignature.Get(); }
};

