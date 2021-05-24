//
// shape_dx.h
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
	using winrt::Windows::UI::Xaml::Controls::SwapChainPanel;
	using winrt::Windows::Graphics::Display::DisplayOrientations;

	// 白
	constexpr D2D1_COLOR_F S_WHITE{ 1.0f, 1.0f, 1.0f, 1.0f };
	// 黒
	constexpr D2D1_COLOR_F S_BLACK{ 0.0f, 0.0f, 0.0f, 1.0f };
	// 補助線の不透明度
	constexpr float AUX_OPAC = 0.975f;
	// 補助線の破線の配置
	constexpr FLOAT AUX_DASHES[] = { 4.0f, 4.0f };
	// 補助線の破線の配置の要素数
	constexpr UINT32 AUX_DASHES_CONT = sizeof(AUX_DASHES) / sizeof(AUX_DASHES[0]);
	// 補助線の線の特性
	constexpr D2D1_STROKE_STYLE_PROPERTIES1 AUX_STYLE {
		D2D1_CAP_STYLE_FLAT,	// startCap
		D2D1_CAP_STYLE_FLAT,	// endCap
		D2D1_CAP_STYLE_ROUND,	// dashCap
		D2D1_LINE_JOIN_MITER_OR_BEVEL,	// lineJoin
		1.0f,	// miterLimit
		D2D1_DASH_STYLE_CUSTOM,	// dashStyle
		0.0f,	// dashOffset
		D2D1_STROKE_TRANSFORM_TYPE_NORMAL
	};
	// 文字範囲の背景色
	constexpr D2D1_COLOR_F RNG_BACK = { 0.0f, 2.0f / 3.0f, 1.0f, 1.0f };
	// 文字範囲の文字色
	constexpr D2D1_COLOR_F RNG_TEXT = { 1.0f, 1.0f, 1.0f, 1.0f };

	// SHAPE_DX を所有しているアプリケーションが、デバイスが失われたときまたは作成されたときに通知を受けるためのインターフェイスを提供する.
	interface IDeviceNotify {
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	//------------------------------
	// 図形の描画環境
	//------------------------------
	struct SHAPE_DX {

		// D3D オブジェクト

		winrt::com_ptr<ID3D11Device3> m_d3dDevice;
		winrt::com_ptr<ID3D11DeviceContext3> m_d3dContext;

		// DXGI スワップチェーン

		winrt::com_ptr<IDXGISwapChain3> m_dxgi_swap_chain;

		// D2D/DWeite 描画コンポーネント

		static winrt::com_ptr<ID2D1Factory3> m_d2dFactory;
		static winrt::com_ptr<IDWriteFactory3> m_dwriteFactory;
		winrt::com_ptr<ID2D1Device2> m_d2dDevice;
		winrt::com_ptr<ID2D1DeviceContext2> m_d2dContext;
		winrt::com_ptr<ID2D1Bitmap1> m_d2dTargetBitmap;

		// XAML コントロール

		SwapChainPanel m_swapChainPanel{};	// パネルへの保持された参照

		// デバイス属性

		D3D_FEATURE_LEVEL m_d3dFeatureLevel{ D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_1 };
		FLOAT m_d3d_target_width;
		FLOAT m_d3d_target_height;
		FLOAT m_output_width;
		FLOAT m_output_height;
		FLOAT m_logical_width = 0.0f;
		FLOAT m_logical_height = 0.0f;
		DisplayOrientations	m_nativeOrientation = DisplayOrientations::None;
		DisplayOrientations	m_currentOrientation = DisplayOrientations::None;
		float m_logical_dpi = -1.0f;
		float m_compositionScaleX = 1.0f;
		float m_compositionScaleY = 1.0f;
		D3D_DRIVER_TYPE m_d3d_driver_type{ D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_NULL };

		// スケーリング用の DPI と合成倍率

		float m_effectiveDpi = -1.0f;
		float m_effectiveCompositionScaleX;
		float m_effectiveCompositionScaleY;

		// 方向変換用の行列

		D2D1::Matrix3x2F m_orientationTransform2D;
		DirectX::XMFLOAT4X4 m_orientationTransform3D;

		// 通知を受けるためのインターフェイス.

		IDeviceNotify* m_deviceNotify = nullptr;

		// 図形表示用の D2D オブジェクト

		double m_anchor_len = 6.0;	// 部位の方形の大きさ
		winrt::com_ptr<ID2D1StrokeStyle1> m_aux_style;	// 補助線の形式
		D2D1_COLOR_F m_range_background = RNG_BACK;	// 文字範囲の背景色
		D2D1_COLOR_F m_range_foreground = RNG_TEXT;	// 文字範囲の文字色
		D2D1_COLOR_F m_theme_background = S_WHITE;	// 前景色
		D2D1_COLOR_F m_theme_foreground = S_BLACK;	// 背景色
		winrt::com_ptr<ID2D1SolidColorBrush> m_range_brush;	// 文字範囲の文字色ブラシ
		winrt::com_ptr<ID2D1SolidColorBrush> m_shape_brush;	// 図形の色ブラシ
		winrt::com_ptr<ID2D1DrawingStateBlock1> m_state_block;	// 描画状態の保存ブロック

		// 描画環境を破棄する.
		void Release(void)
		{
			Trim();
			m_state_block = nullptr;
			m_shape_brush = nullptr;
			m_range_brush = nullptr;
			m_aux_style = nullptr;
			//m_aux_brush = nullptr;
			//m_anch_brush = nullptr;
			m_swapChainPanel = nullptr;
			m_d2dTargetBitmap = nullptr;
			m_d2dContext = nullptr;
			m_d2dDevice = nullptr;
			m_d3dContext = nullptr;
			m_d3dDevice = nullptr;
		}

		//------------------------------
		// shape_dx.cpp
		//------------------------------

		// ウィンドウサイズ依存のリソースを初期化する.
		void CreateWindowSizeDependentResources();
		// D3D デバイスを構成し, D3D と相互運用される D2D デバイスを作成する.
		SHAPE_DX(void);
		// すべてのデバイス リソースを再作成し, 現在の状態に再設定する.
		void HandleDeviceLost();
		// スワップチェーンの内容を画面に表示する.
		void Present();
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
		void SetDpi(float dpi);
		// 描画環境にデバイスの向きを設定する.
		// このメソッドは、OrientationChanged イベントハンドラーの中で呼び出される.
		void SetCurrentOrientation(DisplayOrientations currentOrientation);
		// 描画環境に合成倍率を設定する.
		// このメソッドは、CompositionScaleChanged イベントハンドラーの中で呼び出される.
		void SetCompositionScale(float compositionScaleX, float compositionScaleY);
		// バッファーを解放できることを, ドライバーに示す.
		// このメソッドは, アプリが停止/中断したときに呼び出される.
		void Trim();
		// 表示デバイスが有効になった.
		void ValidateDevice();
	};
}

