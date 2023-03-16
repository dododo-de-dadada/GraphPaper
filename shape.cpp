//------------------------------
// shape.cpp
// 図形のひな型, その他
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
#if defined(_DEBUG)
	uint32_t debug_deleted_cnt = 0;
	uint32_t debug_leak_cnt = 0;
	uint32_t debug_shape_cnt = 0;
#endif
	/*
	float Shape::s_anc_len = 6.0f;	// アンカーポイントの大きさ
	D2D1_COLOR_F Shape::s_background_color = COLOR_WHITE;	// 前景色 (アンカーの背景色)
	D2D1_COLOR_F Shape::s_foreground_color = COLOR_BLACK;	// 背景色 (アンカーの前景色)
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style = nullptr;	// 補助線の形式
	ID2D1Factory3* Shape::s_d2d_factory = nullptr;
	ID2D1RenderTarget* Shape::s_d2d_target = nullptr;
	ID2D1SolidColorBrush* Shape::s_d2d_color_brush = nullptr;
	ID2D1SolidColorBrush* Shape::s_d2d_range_brush = nullptr;
	IDWriteFactory* Shape::s_dwrite_factory = nullptr;
	*/
	static winrt::com_ptr<IDWriteFactory3> create_dwrite_factory(void)
	{
		winrt::com_ptr<IDWriteFactory3> dwrite_factory;
		// ファクトリタイプには DWRITE_FACTORY_TYPE_SHARED を指定する.
		winrt::check_hresult(
			DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory3),
				reinterpret_cast<::IUnknown**>(dwrite_factory.put())
			)
		);
		return dwrite_factory;
	};
	ID2D1RenderTarget* Shape::m_d2d_target = nullptr;
	winrt::com_ptr<ID2D1DrawingStateBlock> Shape::m_state_block{ nullptr };	// 描画状態を保持するブロック
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style{ nullptr };	// 補助線の形式
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_color_brush{ nullptr };	// 色ブラシ (ターゲット依存)
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_range_brush{ nullptr };	// 選択された文字色のブラシ (ターゲット依存)
	winrt::com_ptr<ID2D1BitmapBrush> Shape::m_d2d_bitmap_brush{ nullptr };	// 背景の画像ブラシ (ターゲット依存)
	winrt::com_ptr<IDWriteFactory> Shape::m_dwrite_factory{ create_dwrite_factory() };
	constexpr double ANC_LEN = 6.0;
	float Shape::m_aux_width = 1.0f;	// 補助線の太さ
	bool Shape::m_anc_show = true;	// 図形の部位を表示する.
	float Shape::m_anc_width = ANC_LEN;	// 図形の部位の大きさ
	float Shape::m_anc_square_inner = static_cast<FLOAT>(0.5 * ANC_LEN);	// 図形の部位 (正方形) の内側の辺の半分の長さ
	float Shape::m_anc_square_outer = static_cast<FLOAT>(0.5 * (ANC_LEN + 4.0));	// 図形の部位 (正方形) の外側の辺の半分の長さ
	float Shape::m_anc_circle_inner = static_cast<FLOAT>(sqrt(ANC_LEN * ANC_LEN / M_PI));	// 図形の部位 (円形) の内側の半径
	float Shape::m_anc_circle_outer = static_cast<FLOAT>(sqrt(ANC_LEN * ANC_LEN / M_PI) + 2.0);	// 図形の部位 (円形) の外側の半径
	float Shape::m_anc_rhombus_inner = static_cast<FLOAT>(sqrt(ANC_LEN * ANC_LEN * 0.5) * 0.5);	// 図形の部位 (ひし型) の中心から内側の頂点までの半分の長さ
	float Shape::m_anc_rhombus_outer =	// 図形の部位 (ひし型) の中心から外側の頂点までの半分の長さ
		static_cast<FLOAT>(sqrt((ANC_LEN + 4.0) * (ANC_LEN + 4.0) * 0.5) * 0.5);

	void Shape::begin_draw(
		ID2D1RenderTarget* target, const bool anc_show, IWICFormatConverter* const background, 
		const double scale)
	{
		// 描画対象が変更されるなら, ターゲット依存の描画オブジェクトを作成する.
		if (m_d2d_target != target) {
			m_d2d_target = target;

			// レンダーターゲット依存の色ブラシを作成する.
			m_d2d_color_brush = nullptr;
			winrt::check_hresult(
				m_d2d_target->CreateSolidColorBrush({}, Shape::m_d2d_color_brush.put())
			);
			m_d2d_range_brush = nullptr;
			winrt::check_hresult(
				m_d2d_target->CreateSolidColorBrush({}, Shape::m_d2d_range_brush.put())
			);

			// レンダーターゲット依存の画像ブラシを作成する.
			m_d2d_bitmap_brush = nullptr;
			if (background != nullptr) {
				winrt::com_ptr<ID2D1Bitmap> d2d_bitmap;
				winrt::check_hresult(
					target->CreateBitmapFromWicBitmap(background, d2d_bitmap.put())
				);
				winrt::check_hresult(
					target->CreateBitmapBrush(d2d_bitmap.get(), m_d2d_bitmap_brush.put())
				);
				d2d_bitmap = nullptr;
				m_d2d_bitmap_brush->SetExtendModeX(D2D1_EXTEND_MODE_WRAP);
				m_d2d_bitmap_brush->SetExtendModeY(D2D1_EXTEND_MODE_WRAP);
			}
		}

		if (m_state_block == nullptr) {
			ID2D1Factory1* factory = nullptr;
			m_d2d_target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			winrt::check_hresult(
				factory->CreateDrawingStateBlock(m_state_block.put())
			);
		}

		if (m_aux_style == nullptr) {
			ID2D1Factory1* factory = nullptr;
			m_d2d_target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			winrt::check_hresult(
				factory->CreateStrokeStyle(
					AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
					m_aux_style.put())
			);
		}

		m_anc_show = anc_show;
		m_aux_width = static_cast<float>(1.0 / scale);
		const double a_inner = ANC_LEN / scale;
		const double a_outer = (ANC_LEN + 4.0) / scale;
		m_anc_width = static_cast<float>(ANC_LEN / scale);
		m_anc_square_inner = static_cast<float>(0.5 * a_inner);
		m_anc_square_outer = static_cast<float>(0.5 * a_outer);
		const auto r = sqrt(ANC_LEN * ANC_LEN / M_PI);
		m_anc_circle_inner = static_cast<float>(r / scale);
		m_anc_circle_outer = static_cast<float>((r + 2.0) / scale);
		m_anc_rhombus_inner = static_cast<float>(sqrt(a_inner * a_inner * 0.5) * 0.5);
		m_anc_rhombus_outer = static_cast<float>(sqrt(a_outer * a_outer * 0.5) * 0.5);
	}

}
