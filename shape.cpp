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
	winrt::com_ptr<ID2D1DrawingStateBlock> Shape::m_d2d_state_block{ nullptr };
	winrt::com_ptr<ID2D1StrokeStyle1> Shape::m_aux_style{ nullptr };
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_color_brush{ nullptr };
	winrt::com_ptr<ID2D1SolidColorBrush> Shape::m_d2d_range_brush{ nullptr };
	winrt::com_ptr<ID2D1BitmapBrush> Shape::m_d2d_bitmap_brush{ nullptr };
	winrt::com_ptr<IDWriteFactory> Shape::m_dwrite_factory{ create_dwrite_factory() };
	constexpr double a_len = 6.0;
	float Shape::m_aux_width = 1.0f;
	bool Shape::m_anc_show = true;
	float Shape::m_anc_width = a_len;
	float Shape::m_anc_square_inner = static_cast<FLOAT>(0.5 * a_len);
	float Shape::m_anc_square_outer = static_cast<FLOAT>(0.5 * a_len + 2.0);
	float Shape::m_anc_circle_inner = static_cast<FLOAT>(sqrt(a_len * a_len / M_PI));	// 内側の半径
	float Shape::m_anc_circle_outer = static_cast<FLOAT>(sqrt(a_len * a_len / M_PI) + 2.0);	// 外側の半径

	void Shape::begin_draw(ID2D1RenderTarget* target, const bool anc_show, IWICFormatConverter* const background, const double scale)
	{
		// レンダーターゲットが変わるたびに,
		if (m_d2d_target != target) {
			m_d2d_target = target;
			ID2D1Factory* factory;
			m_d2d_target->GetFactory(&factory);
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
			// ファクトリー依存の画像ブラシを作成する.
			m_d2d_state_block = nullptr;
			static_cast<ID2D1Factory1*>(factory)->CreateDrawingStateBlock(m_d2d_state_block.put());
			// ファクトリー依存の補助線を作成する.
			m_aux_style = nullptr;
			winrt::check_hresult(
				static_cast<ID2D1Factory1*>(factory)->CreateStrokeStyle(AUXILIARY_SEG_STYLE,
					AUXILIARY_SEG_DASHES, AUXILIARY_SEG_DASHES_CONT, m_aux_style.put())
			);

		}
		m_anc_show = anc_show;
		constexpr double a_len = 6.0;
		m_aux_width = static_cast<FLOAT>(1.0 / scale);
		m_anc_width = static_cast<FLOAT>(a_len / scale);
		m_anc_square_inner = static_cast<FLOAT>(0.5 * a_len / scale);
		m_anc_square_outer = static_cast<FLOAT>((0.5 * a_len + 2.0) / scale);
		const auto r = sqrt(a_len * a_len / M_PI);
		m_anc_circle_inner = static_cast<FLOAT>(r / scale);	// 内側の半径
		m_anc_circle_outer = static_cast<FLOAT>((r + 2.0) / scale);	// 外側の半径
	}

}
