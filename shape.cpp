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
	ID2D1RenderTarget* Shape::m_d2d_target = nullptr;
	winrt::com_ptr<ID2D1DrawingStateBlock> Shape::m_state_block{ nullptr };	// 描画状態を保持するブロック
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style{ nullptr };	// 補助線の形式
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_color_brush{ nullptr };	// 色ブラシ (ターゲット依存)
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_range_brush{ nullptr };	// 選択された文字色のブラシ (ターゲット依存)
	winrt::com_ptr<ID2D1BitmapBrush> Shape::m_d2d_bitmap_brush{ nullptr };	// 背景の画像ブラシ (ターゲット依存)
	winrt::com_ptr<IDWriteFactory> Shape::m_dwrite_factory{ create_dwrite_factory() };
	constexpr double LOC_LEN = 6.0;
	float Shape::m_aux_width = 1.0f;	// 補助線の太さ
	bool Shape::m_loc_show = true;	// 部位の表示/非表示.
	float Shape::m_loc_width = LOC_LEN;	// 部位の大きさ
	float Shape::m_loc_square_inner = static_cast<FLOAT>(0.5 * LOC_LEN);	// 部位 (正方形) の内側の辺の半分の長さ
	float Shape::m_loc_square_outer = static_cast<FLOAT>(0.5 * (LOC_LEN + 4.0));	// 部位 (正方形) の外側の辺の半分の長さ
	float Shape::m_loc_circle_inner = static_cast<FLOAT>(sqrt(LOC_LEN * LOC_LEN / M_PI));	// 部位 (円形) の内側の半径
	float Shape::m_loc_circle_outer = static_cast<FLOAT>(sqrt(LOC_LEN * LOC_LEN / M_PI) + 2.0);	// 部位 (円形) の外側の半径
	float Shape::m_loc_rhombus_inner = static_cast<FLOAT>(sqrt(LOC_LEN * LOC_LEN * 0.5) * 0.5);	// 部位 (ひし型) の中心から内側の頂点までの半分の長さ
	float Shape::m_loc_rhombus_outer =	// 部位 (ひし型) の中心から外側の頂点までの半分の長さ
		static_cast<FLOAT>(sqrt((LOC_LEN + 4.0) * (LOC_LEN + 4.0) * 0.5) * 0.5);

	// 図形を描画する前に設定する.
	void Shape::begin_draw(
		ID2D1RenderTarget* target,	// D2D 描画対象
		const bool located,	// 部位の表示/非表示
		IWICFormatConverter* const background,	// 背景パターンの画像 
		const double scale	// 表示倍率
	)
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

			// 描画状態を保持するブロックを作成する.
			ID2D1Factory1* factory = nullptr;
			m_d2d_target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			winrt::check_hresult(
				factory->CreateDrawingStateBlock(m_state_block.put())
			);
		}

		if (m_aux_style == nullptr) {

			// 補助線のストローク形式を作成する.
			ID2D1Factory1* factory = nullptr;
			m_d2d_target->GetFactory(reinterpret_cast<ID2D1Factory**>(&factory));
			winrt::check_hresult(
				factory->CreateStrokeStyle(
					AUXILIARY_SEG_STYLE, AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT,
					m_aux_style.put())
			);
		}

		m_loc_show = located;
		m_aux_width = static_cast<float>(1.0 / scale);
		const double a_inner = LOC_LEN / scale;
		const double a_outer = (LOC_LEN + 4.0) / scale;
		m_loc_width = static_cast<float>(LOC_LEN / scale);
		m_loc_square_inner = static_cast<float>(0.5 * a_inner);
		m_loc_square_outer = static_cast<float>(0.5 * a_outer);
		const auto r = sqrt(LOC_LEN * LOC_LEN / M_PI);
		m_loc_circle_inner = static_cast<float>(r / scale);
		m_loc_circle_outer = static_cast<float>((r + 2.0) / scale);
		m_loc_rhombus_inner = static_cast<float>(sqrt(a_inner * a_inner * 0.5) * 0.5);
		m_loc_rhombus_outer = static_cast<float>(sqrt(a_outer * a_outer * 0.5) * 0.5);
	}

}
