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
	ID2D1RenderTarget* Shape::m_d2d_target = nullptr;	// D2D 描画対象
	winrt::com_ptr<ID2D1DrawingStateBlock> Shape::m_state_block{ nullptr };	// 描画状態を保持するブロック
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style{ nullptr };	// 補助線の形式
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_color_brush{ nullptr };	// 色ブラシ (ターゲット依存)
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_range_brush{ nullptr };	// 選択された文字色のブラシ (ターゲット依存)
	winrt::com_ptr<ID2D1BitmapBrush> Shape::m_d2d_bitmap_brush{ nullptr };	// 背景の画像ブラシ (ターゲット依存)
	winrt::com_ptr<IDWriteFactory> Shape::m_dwrite_factory{ create_dwrite_factory() };
	constexpr double LOCUS_LEN = 6.0;
	float Shape::m_aux_width = 1.0f;	// 補助線の太さ
	bool Shape::m_loc_show = true;	// 部位の表示/非表示.
	float Shape::m_loc_width = LOCUS_LEN;	// 部位の大きさ
	float Shape::m_loc_square_inner = static_cast<float>(0.5 * LOCUS_LEN);	// 部位 (正方形) の内側の辺の半分の長さ
	float Shape::m_loc_square_outer = static_cast<float>(0.5 * (LOCUS_LEN + 4.0));	// 部位 (正方形) の外側の辺の半分の長さ
	float Shape::m_loc_circle_inner = static_cast<float>(sqrt(LOCUS_LEN * LOCUS_LEN / M_PI));	// 部位 (円形) の内側の半径
	float Shape::m_loc_circle_outer = static_cast<float>(sqrt(LOCUS_LEN * LOCUS_LEN / M_PI) + 2.0);	// 部位 (円形) の外側の半径
	float Shape::m_loc_rhombus_inner = static_cast<float>(sqrt(LOCUS_LEN * LOCUS_LEN * 0.5) * 0.5);	// 部位 (ひし型) の中心から内側の頂点までの半分の長さ
	float Shape::m_loc_rhombus_outer = static_cast<float>(sqrt((LOCUS_LEN + 4.0) * (LOCUS_LEN + 4.0) * 0.5) * 0.5);	// 部位 (ひし型) の中心から外側の頂点までの半分の長さ

	// 描画前に必要な変数を格納する.
	void Shape::begin_draw(
		ID2D1RenderTarget* target,	// D2D 描画対象
		const bool located,	// 部位の表示/非表示
		IWICFormatConverter* const background,	// 背景パターンの画像 
		const double scale	// 表示倍率
	) noexcept
	{
		HRESULT hr = S_OK;

		if (m_d2d_target != target) {
			m_d2d_target = target;
			m_d2d_color_brush = nullptr;
			m_d2d_range_brush = nullptr;
			m_d2d_bitmap_brush = nullptr;
		}
		if (target == nullptr) {
			return;
		}

		if (hr == S_OK && m_d2d_color_brush == nullptr) {
			hr = target->CreateSolidColorBrush({},
				Shape::m_d2d_color_brush.put());
		}

		if (hr == S_OK && m_d2d_range_brush == nullptr) {
			hr = target->CreateSolidColorBrush({},
				Shape::m_d2d_range_brush.put());
		}

		if (hr == S_OK && background == nullptr && m_d2d_bitmap_brush != nullptr) {
			m_d2d_bitmap_brush = nullptr;
		}
		else if (hr == S_OK && background != nullptr && m_d2d_bitmap_brush == nullptr) {
			winrt::com_ptr<ID2D1Bitmap> d2d_bitmap;
			if (hr == S_OK) {
				hr = target->CreateBitmapFromWicBitmap(background,
					d2d_bitmap.put());
			}
			if (hr == S_OK) {
				hr = target->CreateBitmapBrush(d2d_bitmap.get(),
					m_d2d_bitmap_brush.put());
			}
			if (hr == S_OK) {
				m_d2d_bitmap_brush->SetExtendModeX(D2D1_EXTEND_MODE_WRAP);
				m_d2d_bitmap_brush->SetExtendModeY(D2D1_EXTEND_MODE_WRAP);
			}
			d2d_bitmap = nullptr;
		}

		if (hr == S_OK && m_state_block == nullptr) {
			ID2D1Factory1* factory = nullptr;
			target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			hr = factory->CreateDrawingStateBlock(
				m_state_block.put());
		}

		if (hr == S_OK && m_aux_style == nullptr) {
			ID2D1Factory1* factory = nullptr;
			target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			hr = factory->CreateStrokeStyle(AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
				m_aux_style.put());
		}

		if (hr == S_OK) {
			m_loc_show = located;
			m_aux_width = static_cast<float>(1.0 / scale);
			const double a_inner = LOCUS_LEN / scale;
			const double a_outer = (LOCUS_LEN + 4.0) / scale;
			m_loc_width = static_cast<float>(LOCUS_LEN / scale);
			m_loc_square_inner = static_cast<float>(0.5 * a_inner);
			m_loc_square_outer = static_cast<float>(0.5 * a_outer);
			const auto r = sqrt(LOCUS_LEN * LOCUS_LEN / M_PI);
			m_loc_circle_inner = static_cast<float>(r / scale);
			m_loc_circle_outer = static_cast<float>((r + 2.0) / scale);
			m_loc_rhombus_inner = static_cast<float>(sqrt(a_inner * a_inner * 0.5) * 0.5);
			m_loc_rhombus_outer = static_cast<float>(sqrt(a_outer * a_outer * 0.5) * 0.5);
		}
	}

}
