#pragma once
#include <Windows.h>
#include <string>
#include <map>
#include <functional>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXTex.h>
#include <wrl.h>

class Dx12Wrapper;
class PMDRenderer;
class PMDActor;

class Application
{
public:
	// 画面スクリーン
	unsigned int _window_width = 1280;
	unsigned int _window_height = 720;

	//ウィンドウ周り
	WNDCLASSEX _windowClass;
	HWND _hwnd = 0;

	std::shared_ptr<Dx12Wrapper> _dx12 = nullptr;
	std::shared_ptr<PMDRenderer> _pmdRenderer = nullptr;
	std::shared_ptr<PMDActor> _pmdActor = nullptr;

private:
	// シングルトンのためにコンストラクタをprivate に
	// さらにコピーと代入を禁止する
	Application();
	~Application();
	Application(const Application&) = delete;
	void operator=(const Application&) = delete;
public:
	// Application のシングルトンインスタンスを得る
	static Application& Instance();

	// 初期化
	bool Init();

	// ループ起動
	void Run();

	// 後処理
	void Terminate();
};