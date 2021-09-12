//------------------------------
// shape_dx.cpp
// 図形の描画環境
//------------------------------
#include "pch.h"
#include <winrt/Windows.UI.Core.h>
#include <windows.ui.xaml.media.dxinterop.h>
#include "shape_dx.h"

using namespace winrt;
using namespace D2D1;
using namespace DirectX;

#define CALLBACK_CONTEXT_ANY	0

namespace DisplayMetrics
{
	// 高解像度ディスプレイは、レンダリングに多くの GPU とバッテリ電力を必要とします。
	// ゲームを完全な再現性を維持して毎秒 60 フレームでレンダリングしようとすると、
	// 高解像度の携帯電話などではバッテリの寿命の短さに悩まされる場合があります。
	// すべてのプラットフォームとフォーム ファクターにわたって完全な再現性を維持してのレンダリングは、
	// 慎重に検討して決定する必要があります。
	static const bool SupportHighResolutions = true;

	// "高解像度" ディスプレイを定義する既定のしきい値.
	// しきい値を超え、SupportHighResolutions が false の場合は,
	// 解像度を 50 % スケールダウンさせる.
	static const float DpiThreshold = 192.0f;		// 標準のデスクトップの 200% 表示。
	static const float WidthThreshold = 1920.0f;	// 幅 1080p。
	static const float HeightThreshold = 1080.0f;	// 高さ 1080p。
};

// 画面の回転の計算に使用する定数。
namespace ScreenRotation
{
	static const XMFLOAT4X4 Rotation0	// 0 度 Z 回転
	(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	static const XMFLOAT4X4 Rotation90	// 90 度 Z 回転
	(
		0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	static const XMFLOAT4X4 Rotation180	// 180 度 Z 回転
	(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	static const XMFLOAT4X4 Rotation270	// 270 度 Z 回転
	(
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
};

namespace winrt::GraphPaper::implementation
{
	static winrt::com_ptr<ID2D1Factory3> create_d2d_factory(void);
	static winrt::com_ptr<IDWriteFactory3> create_dwrite_factory(void);

	winrt::com_ptr<ID2D1Factory3> SHAPE_DX::m_d2dFactory{ create_d2d_factory() };
	winrt::com_ptr<IDWriteFactory3> SHAPE_DX::m_dwriteFactory{ create_dwrite_factory() };

	// デバイスに依存しないピクセル単位 (DIP) の長さを物理的なピクセルの長さに変換します。
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // 最も近い整数値に丸めます。
	}

	// D2D ファクトリを初期化する.
	// 戻り値として com_ptr を返す関数は参照不可.
	static winrt::com_ptr<ID2D1Factory3> create_d2d_factory(void)
	{
		winrt::com_ptr<ID2D1Factory3> d2d_factory;
		// プロジェクトがデバッグ ビルドなら,
		// ファクトリオプションは D2D1_DEBUG_LEVEL_INFORMATION を指定する.
		// ファクトリータイプは D2D1_FACTORY_TYPE_SINGLE_THREADED を指定する.
		D2D1_FACTORY_OPTIONS options;
		ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));
#if defined(_DEBUG)
		options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
		winrt::check_hresult(
			D2D1CreateFactory(
				D2D1_FACTORY_TYPE_SINGLE_THREADED,
				__uuidof(ID2D1Factory3),
				&options,
				d2d_factory.put_void()
			)
		);
		return d2d_factory;
	}

	// DWrite ファクトリを初期化する.
	// 戻り値として com_ptr を返す関数は参照不可.
	static winrt::com_ptr<IDWriteFactory3> create_dwrite_factory(void)
	{
		winrt::com_ptr<IDWriteFactory3> dw_factory;
		// ファクトリタイプには DWRITE_FACTORY_TYPE_SHARED を指定する.
		winrt::check_hresult(
			DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory3),
				reinterpret_cast<::IUnknown**>(dw_factory.put())
			)
		);
		return dw_factory;
	};

	static DXGI_MODE_ROTATION get_display_rotation(const DisplayOrientations native, const DisplayOrientations current)
	{
		// 表示デバイスの既定の方向と現在の方向をもとに, 表示デバイスの回転を決定する.
		// 規定値は, DXGI_MODE_ROTATION_UNSPECIFIED.
		// メモ: DisplayOrientations 列挙型に他の値があっても、NativeOrientation として使用できるのは、
		// Landscape または Portrait のどちらかのみ.
		auto displayRotation = DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_IDENTITY;
		constexpr auto L = DisplayOrientations::Landscape;
		constexpr auto P = DisplayOrientations::Portrait;
		constexpr auto LF = DisplayOrientations::LandscapeFlipped;
		constexpr auto PF = DisplayOrientations::PortraitFlipped;
		// 既定がヨコ向きかつ現在がヨコ向き, または既定がタテ向きかつ現在がタテ向きか判定する.
		if ((native == L && current == L)
			|| (native == P && current == P)) {
			// DXGI_MODE_ROTATION_IDENTITY. 
			displayRotation = DXGI_MODE_ROTATION_IDENTITY;
		}
		// 既定がヨコ向きかつ現在がタテ向き, または既定がタテ向きかつ現在が反ヨコ向きか判定する.
		else if ((native == L && current == P)
			|| (native == P && current == LF)) {
			// DXGI_MODE_ROTATION_ROTATE270.
			displayRotation = DXGI_MODE_ROTATION_ROTATE270;
		}
		// 既定がヨコ向きかつ現在が反ヨコ向き, または既定がタテ向きかつ現在が反タテ向きか判定する.
		else if ((native == L && current == LF)
			|| (native == P && current == PF)) {
			// DXGI_MODE_ROTATION_ROTATE180.
			displayRotation = DXGI_MODE_ROTATION_ROTATE180;
		}
		// 既定がヨコ向きかつ現在が反タテ向き, または既定がタテ向きかつ現在がヨコ向きか判定する.
		else if ((native == L && current == PF)
			|| (native == P && current == L)) {
			// DXGI_MODE_ROTATION_ROTATE90.
			displayRotation = DXGI_MODE_ROTATION_ROTATE90;
		}
		return displayRotation;
	}

	// スワップ チェーンの適切な方向を設定し、回転されたスワップ チェーンにレンダリングするための 2D および
	// 3D マトリックス変換を生成します。
	// 2D および 3D 変換の回転角度は異なります。
	// これは座標空間の違いによります。さらに、
	// 丸めエラーを回避するために 3D マトリックスが明示的に指定されます。
	static void set_display_rotation(
		IDXGISwapChain3* dxgi_swap_chain,
		Matrix3x2F& trans2D,
		DirectX::XMFLOAT4X4& trans3D,
		const DXGI_MODE_ROTATION displayRotation,
		const float logical_width,
		const float logical_height)
	{
		switch (displayRotation)
		{
		case DXGI_MODE_ROTATION_IDENTITY:
			trans2D = Matrix3x2F::Identity();
			trans3D = ScreenRotation::Rotation0;
			break;

		case DXGI_MODE_ROTATION_ROTATE90:
			trans2D =
				Matrix3x2F::Rotation(90.0f) *
				Matrix3x2F::Translation(logical_height, 0.0f);
			trans3D = ScreenRotation::Rotation270;
			break;

		case DXGI_MODE_ROTATION_ROTATE180:
			trans2D =
				Matrix3x2F::Rotation(180.0f) *
				Matrix3x2F::Translation(logical_width, logical_height);
			trans3D = ScreenRotation::Rotation180;
			break;

		case DXGI_MODE_ROTATION_ROTATE270:
			trans2D =
				Matrix3x2F::Rotation(270.0f) *
				Matrix3x2F::Translation(0.0f, logical_width);
			trans3D = ScreenRotation::Rotation90;
			break;

		default:
			throw winrt::hresult_invalid_argument();
		}
		winrt::check_hresult(dxgi_swap_chain->SetRotation(displayRotation));
	}

	// ウィンドウサイズ依存のリソースを作成する.
	// これらのリソースは、ウィンドウサイズが変更されるたびに再作成する必要がある.
	void SHAPE_DX::CreateWindowSizeDependentResources()
	{
		using winrt::Windows::UI::Core::CoreDispatcherPriority;
		using winrt::Windows::UI::Core::DispatchedHandler;

		// 前のウィンドウサイズに固有のコンテキストをクリアするため,
		// レンダーターゲットを D3D コンテキストの出力マージ (OM) ステージに格納する.
		// レンダーターゲットは空のビュー, 深度/ステンシルバッファーはヌルを指定する.
		ID3D11RenderTargetView* nullViews[] = { nullptr };
		m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
		// 空ターゲットを　D2D コンテキストに格納する.
		m_d2dContext->SetTarget(nullptr);
		// ヌルを D2D ビットマップに格納する.
		m_d2dTargetBitmap = nullptr;
		// GPU のコマンドバッファを空のイベントクエリでフラッシュする.
		m_d3dContext->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

		// 論理 DPI と合成倍率の値を, 有効な DPI と有効な合成倍率に格納する.
		m_effectiveDpi = m_logical_dpi;
		m_effectiveCompositionScaleX = m_compositionScaleX;
		m_effectiveCompositionScaleY = m_compositionScaleY;

		// (高解像度のデバイスのバッテリ寿命を上げるためには、より小さいレンダー ターゲットにレンダリングして
		// 出力が提示された場合は GPU で出力をスケーリングできるようする.)
		if constexpr (DisplayMetrics::SupportHighResolutions != true
			&& m_logical_dpi > DisplayMetrics::DpiThreshold) {
			// 高解像度をサポートしない, かつ論理 DPI はしき値を超えるなら,
			// 論理的な幅と高さを, 論理 DPI をもちいて, 
			// ピクセル単位の幅と高さに変換する.
			const float width = ConvertDipsToPixels(m_logical_width, m_logical_dpi);
			const float height = ConvertDipsToPixels(m_logical_height, m_logical_dpi);

			// 変換された幅と高さがしきい値を超えるか判定する.
			if (max(width, height) > DisplayMetrics::WidthThreshold&& min(width, height) > DisplayMetrics::HeightThreshold)
			{
				// 有効な DPI と有効な合成倍率を 1/2 する.
				m_effectiveDpi *= 0.5f;
				m_effectiveCompositionScaleX *= 0.5f;
				m_effectiveCompositionScaleY *= 0.5f;
			}
		}

		// 有効な DPI を用いて, 論理的な幅と高さをピクセル単位に変換し, 
		// 出力領域の幅と高さに格納する.
		// サイズ 0 の DirectX コンテンツが作成されることを防止するため,
		// 出力領域の幅と高さは最低でも 1px とする.
		m_output_width = ConvertDipsToPixels(m_logical_width, m_effectiveDpi);
		m_output_height = ConvertDipsToPixels(m_logical_height, m_effectiveDpi);
		m_output_width = max(m_output_width, 1);
		m_output_height = max(m_output_height, 1);

		// 表示デバイスの既定の方向と現在の方向をもとに, 表示デバイスの回転を決定する.
		// 規定値は, DXGI_MODE_ROTATION_UNSPECIFIED.
		// メモ: DisplayOrientations 列挙型に他の値があっても、NativeOrientation として使用できるのは、
		// Landscape または Portrait のどちらかのみ.
		auto displayRotation = get_display_rotation(m_nativeOrientation, m_currentOrientation);
		// 表示デバイスの回転が 90° または 270° か判定する.
		if (displayRotation == DXGI_MODE_ROTATION_ROTATE90
			|| displayRotation == DXGI_MODE_ROTATION_ROTATE270) {
			// ならば, 出力領域の幅と高さを入れ替えて, D3D ターゲットの幅と高さに格納する.
			m_d3d_target_width = m_output_height;
			m_d3d_target_height = m_output_width;
		}
		else {
			// そうでなければ, 出力領域の幅と高さをそのまま D3D ターゲットの幅と高さに格納する.
			m_d3d_target_width = m_output_width;
			m_d3d_target_height = m_output_height;
		}

		// DXGI スワップチェーンが初期化されているか判定する.
		if (m_dxgi_swap_chain != nullptr) {
			// DXGI スワップチェーンのバッファサイズを変更し, その結果を得る.
			// バッファの大きさには D3D ターゲットの幅と高さを,
			// バッファの数には 2 (ダブル バッファー) を, 
			// フォーマットには DXGI_FORMAT_B8G8R8A8_UNORM を指定する.
			HRESULT hr = m_dxgi_swap_chain->ResizeBuffers(
				2,
				lround(m_d3d_target_width),
				lround(m_d3d_target_height),
				DXGI_FORMAT_B8G8R8A8_UNORM,
				0
			);

			// 結果が DXGI_ERROR_DEVICE_REMOVED または DXGI_ERROR_DEVICE_RESET か判定する.
			if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
				// すべてのデバイス リソースを再作成し, 現在の状態に再設定する.
				HandleDeviceLost();
				return;
			}
			else {
				winrt::check_hresult(hr);
			}
		}
		else {
			// それ以外の場合は、既存の Direct3D デバイスと同じアダプターを使用して、新規作成する.
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
			// DXGI スワップチェーンの詳細を初期化する.
			// 幅には D3D ターゲットの幅,
			// 高さには D3D ターゲットの高さ,
			// フォーマットには DXGI_FORMAT_B8G8R8A8_UNORM,
			// ステレオには false,
			// マルチサンプリングのピクセル数には 1,
			// マルチサンプリングの品質レベルには 0,
			// バッファの使用方法には DXGI_USAGE_RENDER_TARGET_OUTPUT,
			// バッファの数には 2,
			// フラグには 0 をそれぞれ指定する.
			swapChainDesc.Width = lround(m_d3d_target_width);		// ウィンドウのサイズと一致させます。
			swapChainDesc.Height = lround(m_d3d_target_height);
			swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;				// これは、最も一般的なスワップ チェーンのフォーマットです。
			swapChainDesc.Stereo = false;
			swapChainDesc.SampleDesc.Count = 1;								// マルチサンプリングは使いません。
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = 2;	// 遅延を最小限に抑えるにはダブル バッファーを使用します。
			swapChainDesc.Flags = 0;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			// スケーリングには DXGI_SCALING_STRETCH を指定する.
			// 原因はよくわからないが, たとえバッファサイズが一致していたとしても DXGI_SCALING_NONE を指定するとエラーになる.
			//if (DisplayMetrics::SupportHighResolutions
			// || m_logical_dpi <= DisplayMetrics::DpiThreshold) {
			//	swapChainDesc.Scaling = DXGI_SCALING_NONE;
			//}
			//else {
				swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
			//}
			// D3D ドライバーが D3D_DRIVER_TYPE_HARDWARE か判定する.
			if (m_d3d_driver_type == D3D_DRIVER_TYPE_HARDWARE) {
				// 切り替え効果に DXGI_SWAP_EFFECT_FLIP_DISCARD を格納する.
				// この効果を指定すると, Present1 呼び出し後に残ったバッファは破棄される.
				// 破棄されてないバッファが残っている場合, ハードウェアドライバーはこれを表示してしまう.
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			}
			else {
				// D3D ドライバーが D3D_DRIVER_TYPE_HARDWARE 以外なら,
				// 切り替え効果に DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL を格納する.
				// この効果を指定すると, Present1 呼び出し後に残ったバッファは保持される.
				// 破棄されてないバッファが残っている場合でも, WARP ドライバーは正常に表示する.
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// すべての Windows ストア アプリは、_FLIP_ SwapEffects を使用する必要があります。
			}

			// このシーケンスは、上の Direct3D デバイスを作成する際に使用された DXGI ファクトリを取得する.
			winrt::com_ptr<IDXGIDevice3> dxgiDevice;
			winrt::check_hresult(m_d3dDevice.try_as(dxgiDevice));

			winrt::com_ptr<IDXGIAdapter> dxgiAdapter;
			winrt::check_hresult(dxgiDevice->GetAdapter(dxgiAdapter.put()));

			winrt::com_ptr<IDXGIFactory4> dxgiFactory;
			winrt::check_hresult(dxgiAdapter->GetParent(_uuidof(&dxgiFactory), dxgiFactory.put_void()));

			// XAML 相互運用機能の使用時に, 合成用にスワップ チェーンを作成する必要あり.
			winrt::com_ptr<IDXGISwapChain1> swapChain;
			winrt::check_hresult(
				dxgiFactory->CreateSwapChainForComposition(
					m_d3dDevice.get(),
					&swapChainDesc,
					nullptr,
					swapChain.put()
				)
			);

			winrt::check_hresult(swapChain.try_as(m_dxgi_swap_chain));

			// スワップ チェーンと SwapChainPanel を関連付ける.
			// UI の変更は、UI スレッドにディスパッチして戻す必要あり.
			const auto _{ m_swapChainPanel.Dispatcher().RunAsync(
				CoreDispatcherPriority::High,
				[=]()
				{
					// SwapChainPanel のネイティブ インターフェイスに戻す.
					winrt::com_ptr<ISwapChainPanelNative> scpNative{ nullptr };
					winrt::check_hresult(winrt::get_unknown(m_swapChainPanel)->QueryInterface(__uuidof(scpNative), scpNative.put_void()));
					winrt::check_hresult(scpNative->SetSwapChain(m_dxgi_swap_chain.get()));
				}
			) };
			// DXGI が 1 度に複数のフレームをキュー処理していないことを確認します。これにより、遅延が減少し、
			// アプリケーションが各 VSync の後でのみレンダリングすることが保証され、消費電力が最小限に抑えられます。
			winrt::check_hresult(dxgiDevice->SetMaximumFrameLatency(1));
		}

		// スワップ チェーンの適切な方向を設定し、回転されたスワップ チェーンにレンダリングするための
		// 2D および 3D マトリックス変換を生成します。
		// 2D および 3D 変換の回転角度は異なります。これは座標空間の違いによります。
		// さらに、丸めエラーを回避するために 3D マトリックスが明示的に指定されます。
		set_display_rotation(m_dxgi_swap_chain.get(), m_orientationTransform2D, m_orientationTransform3D, displayRotation, m_logical_width, m_logical_height);

		// スワップ チェーンに逆拡大率を設定します
		DXGI_MATRIX_3X2_F inverseScale{ 0 };
		inverseScale._11 = 1.0f / m_effectiveCompositionScaleX;
		inverseScale._22 = 1.0f / m_effectiveCompositionScaleY;
		winrt::com_ptr<IDXGISwapChain2> spSwapChain2;
		winrt::check_hresult(m_dxgi_swap_chain.try_as(spSwapChain2));//As<IDXGISwapChain2>(&spSwapChain2));		
		winrt::check_hresult(spSwapChain2->SetMatrixTransform(&inverseScale));

		// スワップ チェーンのバック バッファーのレンダリング ターゲット ビューを作成します。
		winrt::com_ptr<ID3D11Texture2D1> backBuffer;
		winrt::check_hresult(
			m_dxgi_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D1), backBuffer.put_void())//IID_PPV_ARGS(&backBuffer))
		);
		// スワップ チェーン バック バッファーに関連付けられた Direct2D ターゲット ビットマップを作成し、
		// それを現在のターゲットとして設定します。
		D2D1_BITMAP_PROPERTIES1 bitmapProperties =
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
				m_logical_dpi,
				m_logical_dpi
			);

		winrt::com_ptr<IDXGISurface2> dxgiBackBuffer;
		winrt::check_hresult(
			m_dxgi_swap_chain->GetBuffer(0, __uuidof(IDXGISurface2), dxgiBackBuffer.put_void())//IID_PPV_ARGS(&dxgiBackBuffer))
		);
		winrt::check_hresult(
			m_d2dContext->CreateBitmapFromDxgiSurface(
				dxgiBackBuffer.get(),
				&bitmapProperties,
				m_d2dTargetBitmap.put()
			)
		);
		m_d2dContext->SetTarget(m_d2dTargetBitmap.get());
		m_d2dContext->SetDpi(m_effectiveDpi, m_effectiveDpi);

		// すべての Windows ストア アプリで、グレースケール テキストのアンチエイリアシングをお勧めします。
		m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	}

	// すべてのデバイス リソースを再作成し、現在の状態に再設定する.
	void SHAPE_DX::HandleDeviceLost()
	{
		m_dxgi_swap_chain = nullptr;

		if (m_deviceNotify != nullptr) {
			m_deviceNotify->OnDeviceLost();
		}

		m_d2dContext->SetDpi(m_logical_dpi, m_logical_dpi);
		CreateWindowSizeDependentResources();

		if (m_deviceNotify != nullptr) {
			m_deviceNotify->OnDeviceRestored();
		}
	}

	// スワップチェーンの内容を画面に表示する.
	void SHAPE_DX::Present()
	{
		DXGI_PRESENT_PARAMETERS param{ 0 };
		HRESULT hr = m_dxgi_swap_chain->Present1(1, 0, &param);
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
			HandleDeviceLost();
		}
		else {
			winrt::check_hresult(hr);
		}
	}

	// デバイスが失われたときと作成されたときに通知を受けるように、DeviceNotify を登録する.
	void SHAPE_DX::RegisterDeviceNotify(IDeviceNotify* deviceNotify)
	{
		m_deviceNotify = deviceNotify;
	}

	// 描画環境に合成倍率を設定する.
	// このメソッドは、CompositionScaleChanged イベントハンドラーの中で呼び出される.
	void SHAPE_DX::SetCompositionScale(float compositionScaleX, float compositionScaleY)
	{
		if (m_compositionScaleX != compositionScaleX || m_compositionScaleY != compositionScaleY) {
			m_compositionScaleX = compositionScaleX;
			m_compositionScaleY = compositionScaleY;
			CreateWindowSizeDependentResources();
		}
	}

	// 描画環境にデバイスの向きを設定する.
	// このメソッドは、OrientationChanged イベントハンドラーの中で呼び出される.
	void SHAPE_DX::SetCurrentOrientation(DisplayOrientations currentOrientation)
	{
		if (m_currentOrientation != currentOrientation) {
			m_currentOrientation = currentOrientation;
			CreateWindowSizeDependentResources();
		}
	}

	// 描画環境に DPI を設定する.
	// このメソッドは、DpiChanged イベントハンドラーの中で呼び出される.
	void SHAPE_DX::SetDpi(float dpi)
	{
		if (dpi == m_logical_dpi) {
			return;
		}
		m_logical_dpi = dpi;
		m_d2dContext->SetDpi(m_logical_dpi, m_logical_dpi);
		CreateWindowSizeDependentResources();
	}

	// 描画環境に表示領域の大きさを設定する.
	// このメソッドは、SizeChanged イベントハンドラーの中で呼び出される.
	void SHAPE_DX::SetLogicalSize2(const D2D1_SIZE_F size)
	{
		if (m_logical_width == size.width && m_logical_height == size.height) {
			return;
		}
		m_logical_width = size.width;
		m_logical_height = size.height;
		CreateWindowSizeDependentResources();
	}

	// 描画環境に XAML スワップチェーンパネルを設定する.
	// このメソッドは, UI コントロールが作成 (または再作成) されたときに呼び出される.
	void SHAPE_DX::SetSwapChainPanel(SwapChainPanel const& xaml_scp)
	{
		using winrt::Windows::Graphics::Display::DisplayInformation;

		// 現在の表示状態 DisplayInfomation を得る.
		// 表示状態をもとに, 表示デバイスの既定の向きと現在の向き, 
		// 論理 DPI を得る. 
		// D2D コンテキストに論理 DPI を格納する.
		// パネルへの保持された参照に指定されたスワップチェーンパネルを保存する.
		// パネルをもとに, 実際的な幅と高さ, 拡大率を得て, 
		// それぞれ論理的な高さと幅, 合成倍率に格納する.
		DisplayInformation di = DisplayInformation::GetForCurrentView();
		m_nativeOrientation = di.NativeOrientation();
		m_currentOrientation = di.CurrentOrientation();
		m_logical_dpi = di.LogicalDpi();
		m_d2dContext->SetDpi(m_logical_dpi, m_logical_dpi);
		m_swapChainPanel = xaml_scp;
		m_compositionScaleX = xaml_scp.CompositionScaleX();
		m_compositionScaleY = xaml_scp.CompositionScaleY();
		m_logical_width = static_cast<FLOAT>(xaml_scp.ActualWidth());
		m_logical_height = static_cast<FLOAT>(xaml_scp.ActualHeight());
		if (m_logical_width > 0.0 && m_logical_height > 0.0) {
			CreateWindowSizeDependentResources();
		}
	}

	// D3D デバイスを構成し, D3D と相互運用される D2D デバイスを作成する.
	SHAPE_DX::SHAPE_DX(void)
	{
		D3D_FEATURE_LEVEL m_d3dFeatureLevel{ D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_1 };

		// D3D11_CREATE_DEVICE_BGRA_SUPPORT を作成フラグに格納する.
		// D2D との互換性を保持するために必要.
		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
		// コンパイルがデバッグビルドなら,
		// D3D デバイスを作成できるか判定し, その結果を得る.
		// ドライバータイプは D3D_DRIVER_TYPE_NULL を, 
		// フラグは D3D11_CREATE_DEVICE_DEBUG を,
		// SDK バージョンは D3D11_SDK_VERSION を,
		// その他の引数は nullptr または 0 を指定する.
		// D3D_DRIVER_TYPE_NULL は, 実際のハードウェアデバイスを作成しない.
		// D3D11_CREATE_DEVICE_DEBUG は, SDK レイヤーの確認.
		// D3D11_SDK_VERSION は, Windows ストアアプリでは常に必要.
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,
			0,
			D3D11_CREATE_DEVICE_DEBUG,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			nullptr,
			nullptr,
			nullptr
		);
		if (SUCCEEDED(hr)) {
			// 作成できるなら,
			// D3D11_CREATE_DEVICE_DEBUG を作成フラグに追加する.
			creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
		}
#endif
		// D3D_FEATURE_LEVEL_9_1 以降を機能レベル配列に格納する.
		constexpr D3D_FEATURE_LEVEL featureLevels[] = {
			// この配列では、順序が保存されることに注意する.
			// アプリケーションの最低限必要な機能レベルをその説明で宣言することを忘れないでください。
			// 特に記載がない限り、すべてのアプリケーションは 9.1 をサポートすることが想定されます。
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};
		// D3D デバイスと D3D コンテキストを作成し結果を得る.
		// 引数として, 作成フラグと機能レベル配列に加え, 
		// ドライバータイプは D3D_DRIVER_TYPE_HARDWARE を,
		// SDK バージョンは D3D11_SDK_VERSION を,
		// その他は 0 または nullptr を指定する.
		winrt::com_ptr<ID3D11Device> device;
		winrt::com_ptr<ID3D11DeviceContext> context;
#if !defined(_DEBUG)
		HRESULT
#endif
		hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			creationFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			device.put(),
			&m_d3dFeatureLevel,
			context.put()
		);
		if (SUCCEEDED(hr)) {
			// 作成できたなら, D3D_DRIVER_TYPE_HARDWARE を D3D ドライバータイプに格納する.
			m_d3d_driver_type = D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE;
		}
		else {
			// 作成できないなら WARP デバイスを作成する.
			// ドライバータイプは D3D_DRIVER_TYPE_WARP を指定する.
			// 作成できないなら, 中断する.
			// 作成したなら, D3D_DRIVER_TYPE_WARP を D3D ドライバータイプに格納する.
			winrt::check_hresult(
				D3D11CreateDevice(
					nullptr,
					D3D_DRIVER_TYPE_WARP,
					0,
					creationFlags,
					featureLevels,
					ARRAYSIZE(featureLevels),
					D3D11_SDK_VERSION,
					device.put(),
					&m_d3dFeatureLevel,
					context.put()
				)
			);
			m_d3d_driver_type = D3D_DRIVER_TYPE_WARP;
		}
		winrt::check_hresult(device.try_as(m_d3dDevice));
		winrt::check_hresult(context.try_as(m_d3dContext));

		// D3D デバイスを DXGI デバイスに格納する.
		winrt::com_ptr<IDXGIDevice3> dxgiDevice;
		winrt::check_hresult(m_d3dDevice.try_as(dxgiDevice));
		// DXGI デバイスをもとに D2D デバイスを作成する.
		m_d2dDevice = nullptr;
		winrt::check_hresult(
			m_d2dFactory->CreateDevice(dxgiDevice.get(), m_d2dDevice.put())
		);
		// D2D デバイスをもとに D2D コンテキストを作成する.
		// オプションは D2D1_DEVICE_CONTEXT_OPTIONS_NONE を指定する.
		m_d2dContext = nullptr;
		winrt::check_hresult(
			m_d2dDevice->CreateDeviceContext(
				D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
				m_d2dContext.put()
			)
		);
		// D2D コンテキストをもとに, 図形の色ブラシ, 部位の色ブラシ,
		// 補助線の色ブラシ, 文字範囲の文字の色ブラシ, 補助線の形式,
		// 描画状態の保存ブロックを作成する.
		m_shape_brush = nullptr;
		winrt::check_hresult(
			m_d2dContext->CreateSolidColorBrush({}, m_shape_brush.put())
		);
		m_range_brush = nullptr;
		winrt::check_hresult(
			m_d2dContext->CreateSolidColorBrush({}, m_range_brush.put())
		);
		m_aux_style = nullptr;
		winrt::check_hresult(
			m_d2dFactory->CreateStrokeStyle(AUX_STYLE, AUX_DASHES, AUX_DASHES_CONT, m_aux_style.put())
		);
		m_state_block = nullptr;
		winrt::check_hresult(
			m_d2dFactory->CreateDrawingStateBlock(m_state_block.put())
		);

	};

	// バッファーを解放できることを, ドライバーに示す.
	// このメソッドは, アプリが停止/中断したときに呼び出される.
	void SHAPE_DX::Trim()
	{
		if (m_d3dDevice == nullptr) {
			return;
		}
		winrt::com_ptr<IDXGIDevice3> dxgiDevice;
		m_d3dDevice.as(dxgiDevice);
		dxgiDevice->Trim();
	}

	// 表示デバイスが有効になった.
	// このメソッドは、DisplayContentsInvalidated イベント用のイベント ハンドラーの中で呼び出されます。
	void SHAPE_DX::ValidateDevice(void)
	{
		//デバイスが作成された後に既定のアダプターが変更された、
		// またはこのデバイスが削除された場合は、D3D デバイスが有効でなくなります。

		// まず、デバイスが作成された時点から、既定のアダプターに関する情報を取得します。

		winrt::com_ptr<IDXGIDevice3> dxgiDevice;
		winrt::check_hresult(m_d3dDevice.try_as(dxgiDevice));

		winrt::com_ptr<IDXGIAdapter> deviceAdapter;
		winrt::check_hresult(dxgiDevice->GetAdapter(deviceAdapter.put()));

		winrt::com_ptr<IDXGIFactory2> deviceFactory;
		winrt::check_hresult(deviceAdapter->GetParent(__uuidof(IDXGIFactory2), deviceFactory.put_void()));//IID_PPV_ARGS(&deviceFactory)));

		winrt::com_ptr<IDXGIAdapter1> previousDefaultAdapter;
		winrt::check_hresult(deviceFactory->EnumAdapters1(0, previousDefaultAdapter.put()));

		DXGI_ADAPTER_DESC1 previousDesc;
		winrt::check_hresult(previousDefaultAdapter->GetDesc1(&previousDesc));

		// 次に、現在の既定のアダプターの情報を取得します。

		winrt::com_ptr<IDXGIFactory4> currentFactory;
		winrt::check_hresult(CreateDXGIFactory1(__uuidof(IDXGIFactory4), currentFactory.put_void()));// IID_PPV_ARGS(&currentFactory)));

		winrt::com_ptr<IDXGIAdapter1> currentDefaultAdapter;
		winrt::check_hresult(currentFactory->EnumAdapters1(0, currentDefaultAdapter.put()));// &currentDefaultAdapter));

		DXGI_ADAPTER_DESC1 currentDesc;
		winrt::check_hresult(currentDefaultAdapter->GetDesc1(&currentDesc));

		// アダプターの LUID が一致しない、またはデバイスで LUID が削除されたとの報告があった場合は、
		// 新しい D3D デバイスを作成する必要があります。

		if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
			previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
			FAILED(m_d3dDevice->GetDeviceRemovedReason()))
		{
			// 古いデバイスに関連したリソースへの参照を解放します。
			dxgiDevice = nullptr;
			deviceAdapter = nullptr;
			deviceFactory = nullptr;
			previousDefaultAdapter = nullptr;

			// すべてのデバイス リソースを再作成し、現在の状態に再設定する.
			HandleDeviceLost();
		}
	}

}