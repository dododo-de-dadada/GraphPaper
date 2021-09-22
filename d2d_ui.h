//
// d2d_ui.h
//
// 図形の描画環境
//
// VisualStudio のプロジェクトテンプレートの DirectX 11 アプリに
// 含まれる DeviceResources クラスをもとに,
// 3D 表示やフレームレートに関した処理を削除して, 2D 表示に
// 必要な部分を残している.
// 図形を表示するため, 色ブラシなど D2D オブジェクトが
// 追加されている.
//
#pragma once
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <dxgi1_4.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.UI.Xaml.Controls.h>

namespace winrt::GraphPaper::implementation
{
	// D2D_UI を所有しているアプリケーションが、デバイスが失われたときまたは作成されたときに通知を受けるためのインターフェイスを提供する.
	interface IDeviceNotify {
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	//------------------------------
	// D2D
	//------------------------------
	struct D2D_UI {

		// D3D オブジェクト

		winrt::com_ptr<ID3D11Device3> m_d3d_device{ nullptr };
		winrt::com_ptr<ID3D11DeviceContext3> m_d3d_context{ nullptr };

		// DXGI スワップチェーン

		winrt::com_ptr<IDXGISwapChain3> m_dxgi_swap_chain{ nullptr };

		// D2D/DWeite 描画コンポーネント

		static winrt::com_ptr<ID2D1Factory3> m_d2d_fanctory;
		static winrt::com_ptr<IDWriteFactory3> m_dwrite_factory;
		//winrt::com_ptr<ID2D1Device2> m_d2dDevice;
		winrt::com_ptr<ID2D1DeviceContext2> m_d2d_context{ nullptr };
		//winrt::com_ptr<ID2D1Bitmap1> d2d_target_bitmap;

		// XAML コントロール

		//SwapChainPanel m_swapChainPanel{};	// パネルへの保持された参照

		// デバイス属性

		//FLOAT m_d3d_target_width;
		//FLOAT m_d3d_target_height;
		//FLOAT m_output_width;
		//FLOAT m_output_height;
		FLOAT m_logical_width = 0.0f;
		FLOAT m_logical_height = 0.0f;
		winrt::Windows::Graphics::Display::DisplayOrientations m_nativeOrientation = winrt::Windows::Graphics::Display::DisplayOrientations::None;
		winrt::Windows::Graphics::Display::DisplayOrientations m_currentOrientation = winrt::Windows::Graphics::Display::DisplayOrientations::None;
		float m_logical_dpi = -1.0f;
		float m_compositionScaleX = 1.0f;
		float m_compositionScaleY = 1.0f;
		D3D_DRIVER_TYPE m_d3d_driver_type = D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_NULL;

		// スケーリング用の DPI と合成倍率

		float m_effectiveDpi = -1.0f;
		float m_effectiveCompositionScaleX;
		float m_effectiveCompositionScaleY;

		// 方向変換用の行列

		//D2D1::Matrix3x2F m_orientationTransform2D;
		//DirectX::XMFLOAT4X4 m_orientationTransform3D;

		// 通知を受けるためのインターフェイス.

		IDeviceNotify* m_deviceNotify = nullptr;

		// 図形表示用の D2D オブジェクト
		winrt::com_ptr<ID2D1DrawingStateBlock1> m_state_block{ nullptr };	// 描画状態の保存ブロック
		winrt::com_ptr<ID2D1SolidColorBrush> m_range_brush{ nullptr };	// 文字範囲の文字色ブラシ
		winrt::com_ptr<ID2D1SolidColorBrush> m_solid_color_brush{ nullptr };	// 図形の色ブラシ

		// 描画環境を破棄する.
		void Release(void)
		{
			Trim();
			m_state_block = nullptr;
			m_solid_color_brush = nullptr;
			m_range_brush = nullptr;
			//m_aux_style = nullptr;
			//m_aux_brush = nullptr;
			//m_anch_brush = nullptr;
			//m_swapChainPanel = nullptr;
			//d2d_target_bitmap = nullptr;
			m_d2d_context = nullptr;
			//m_d2dDevice = nullptr;
			m_d3d_context = nullptr;
			m_d3d_device = nullptr;
		}

		//------------------------------
		// shape_dx.cpp
		//------------------------------

		// ウィンドウサイズ依存のリソースを初期化する.
		void CreateWindowSizeDependentResources(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&);
		// D3D デバイスを構成し, D3D と相互運用される D2D デバイスを作成する.
		D2D_UI(void);
		// すべてのデバイス リソースを再作成し, 現在の状態に再設定する.
		void HandleDeviceLost(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&);
		// スワップチェーンの内容を画面に表示する.
		void Present(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&);
		// デバイスが失われたときと作成されたときに通知を受けるように、DeviceNotify を登録する.
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		// 描画環境に XAML スワップチェーンパネルを設定する.
		// このメソッドは, UI コントロールが作成 (または再作成) されたときに呼び出される.
		void SetSwapChainPanel(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& xamk_scp);
		// 描画環境に表示領域の大きさを設定する.
		// このメソッドは、SizeChanged イベントハンドラーの中で呼び出される.
		void SetLogicalSize2(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&, const D2D1_SIZE_F logicalSize);
		// 描画環境に DPI を設定する.
		// このメソッドは、DpiChanged イベントハンドラーの中で呼び出される.
		void SetDpi(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const&, const float dpi);
		// 描画環境にデバイスの向きを設定する.
		// このメソッドは、OrientationChanged イベントハンドラーの中で呼び出される.
		void SetCurrentOrientation(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& swap_chain_panel, winrt::Windows::Graphics::Display::DisplayOrientations currentOrientation);
		// 描画環境に合成倍率を設定する.
		// このメソッドは、CompositionScaleChanged イベントハンドラーの中で呼び出される.
		void SetCompositionScale(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& swap_chain_panel, float compositionScaleX, float compositionScaleY);
		// バッファーを解放できることを, ドライバーに示す.
		// このメソッドは, アプリが停止/中断したときに呼び出される.
		void Trim();
		// 表示デバイスが有効になった.
		void ValidateDevice(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& swap_chain_panel);
	};
}

