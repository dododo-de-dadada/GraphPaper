//
// d2d_ui.h
//
// 図形の描画環境
//
// VisualStudio のプロジェクトテンプレートの DirectX 11 アプリに
// 含まれる DeviceResources クラスをもとに,
// 3D 表示やフレームレートに関した処理を削除して,
// 2D 表示に必要な部分を残している.
//
#pragma once
//#define WIN_UI	3
#define WIN_UI	2
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <dxgi1_4.h>
#include <winrt/Windows.Graphics.Display.h>
#if WIN_UI == 3
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#else
#include <winrt/Windows.UI.Xaml.Controls.h>
#endif

namespace winrt::GraphPaper::implementation
{
#if WIN_UI == 3
	using winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel;
#else
	using winrt::Windows::Graphics::Display::DisplayOrientations;
	using winrt::Windows::UI::Xaml::Controls::SwapChainPanel;
#endif
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

		static winrt::com_ptr<ID2D1Factory3> m_d2d_factory;
		winrt::com_ptr<ID2D1DeviceContext2> m_d2d_context{ nullptr };

		// XAML コントロール
		SwapChainPanel m_swap_chain_panel;	// パネルへの保持された参照

		// デバイス属性

		FLOAT m_logical_width = 0.0f;
		FLOAT m_logical_height = 0.0f;
#if WIN_UI == 3
#else
		DisplayOrientations m_nativeOrientation = DisplayOrientations::None;
		DisplayOrientations m_current_orientation = DisplayOrientations::None;
#endif
		float m_logical_dpi = 96.0f;
		float m_composition_scale_x = 1.0f;
		float m_composition_scale_y = 1.0f;
		D3D_DRIVER_TYPE m_d3d_driver_type = D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_NULL;

		// スケーリング用の DPI と合成倍率

		float m_effective_dpi = 0.0f;
		float m_effectiveCompositionScaleX = 1.0f;
		float m_effectiveCompositionScaleY = 1.0f;

		// 通知を受けるためのインターフェイス.

		IDeviceNotify* m_deviceNotify = nullptr;

		// 描画環境を破棄する.
		void Release(void)
		{
			Trim();
		}

		//------------------------------
		// d2d_ui.cpp
		//------------------------------

		// ウィンドウサイズ依存のリソースを初期化する.
		void CreateWindowSizeDependentResources(void);
		// D3D デバイスを構成し, D3D と相互運用される D2D デバイスを作成する.
		D2D_UI(void);
		// すべてのデバイス リソースを再作成し, 現在の状態に再設定する.
		void HandleDeviceLost(void);
		// スワップチェーンの内容を画面に表示する.
		void Present(void);
		// デバイスが失われたときと作成されたときに通知を受けるように、DeviceNotify を登録する.
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		// 描画環境に XAML スワップチェーンパネルを設定する.
		// このメソッドは, UI コントロールが作成 (または再作成) されたときに呼び出される.
		void SetSwapChainPanel(SwapChainPanel const& xamk_scp);
		// 描画環境に表示領域の大きさを設定する.
		// このメソッドは、SizeChanged イベントハンドラーの中で呼び出される.
		void SetLogicalSize2(const D2D1_SIZE_F logicalSize);
		// 描画環境に DPI を設定する.
		// このメソッドは、DpiChanged イベントハンドラーの中で呼び出される.
		void SetDpi(const float dpi);
		// 描画環境にデバイスの向きを設定する.
		// このメソッドは、OrientationChanged イベントハンドラーの中で呼び出される.
		void SetCurrentOrientation(winrt::Windows::Graphics::Display::DisplayOrientations current_orientation);
		// 描画環境に合成倍率を設定する.
		// このメソッドは、CompositionScaleChanged イベントハンドラーの中で呼び出される.
		void SetCompositionScale(float composition_scale_x, float composition_scale_y);
		// バッファーを解放できることを, ドライバーに示す.
		// このメソッドは, アプリが停止/中断したときに呼び出される.
		void Trim();
		// 表示デバイスが有効になった.
		void ValidateDevice(void);
	};
}

