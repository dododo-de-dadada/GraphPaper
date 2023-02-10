#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 方眼を表示する.
	static void page_draw_grid(
		ID2D1RenderTarget* const target,
		ID2D1SolidColorBrush* const brush,
		const float grid_base,
		const D2D1_COLOR_F grid_color,
		const GRID_EMPH grid_emph,
		const D2D1_POINT_2F grid_offset,
		const float page_scale,
		const D2D1_SIZE_F sh_size
	);

	// 曲線の補助線(制御点を結ぶ折れ線)を表示する.
	// target	レンダーターゲット
	// brush	色ブラシ
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapePage::draw_auxiliary_bezi(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// ページの倍率にかかわらず見た目の太さを変えないため, その逆数を線の太さに格納する.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// 線の太さ
		ID2D1StrokeStyle1* const a_style = Shape::m_aux_style.get();
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F e_pos;

		e_pos.x = c_pos.x;
		e_pos.y = p_pos.y;
		brush->SetColor(Shape::s_background_color);
		target->DrawLine(p_pos, e_pos, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawLine(p_pos, e_pos, brush, s_width, a_style);
		s_pos = e_pos;
		e_pos.x = p_pos.x;
		e_pos.y = c_pos.y;
		brush->SetColor(Shape::s_background_color);
		target->DrawLine(s_pos, e_pos, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawLine(s_pos, e_pos, brush, s_width, a_style);
		s_pos = e_pos;
		brush->SetColor(Shape::s_background_color);
		target->DrawLine(s_pos, c_pos, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawLine(s_pos, c_pos, brush, s_width, a_style);
	}

	// だ円の補助線を表示する.
	// target	レンダーターゲット
	// brush	色ブラシ
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapePage::draw_auxiliary_elli(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// ページの倍率にかかわらず見た目の太さを変えないため, その逆数を線の太さに格納する.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// 線の太さ
		//const D2D_UI& d2d = sh.m_d2d;
		D2D1_POINT_2F rect;	// 方形
		D2D1_ELLIPSE elli;		// だ円

		pt_sub(c_pos, p_pos, rect);
		pt_mul(rect, 0.5, rect);
		pt_add(p_pos, rect, elli.point);
		elli.radiusX = rect.x;
		elli.radiusY = rect.y;
		brush->SetColor(Shape::s_background_color);
		target->DrawEllipse(elli, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawEllipse(elli, brush, s_width, Shape::m_aux_style.get());
	}

	// 直線の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapePage::draw_auxiliary_line(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// ページの倍率にかかわらず見た目の太さを変えないため, その逆数を線の太さに格納する.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// 線の太さ
		brush->SetColor(Shape::s_background_color);
		target->DrawLine(p_pos, c_pos, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawLine(p_pos, c_pos, brush, s_width, Shape::m_aux_style.get());
	}

	// 多角形の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	// p_opt	多角形の選択肢
	void ShapePage::draw_auxiliary_poly(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos, const POLY_OPTION& p_opt)
	{
		// ページの倍率にかかわらず見た目の太さを変えないため, その逆数を線の太さに格納する.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// 線の太さ
		D2D1_POINT_2F v_pos[N_GON_MAX];	// 頂点の配列

		D2D1_POINT_2F p_vec;
		pt_sub(c_pos, p_pos, p_vec);
		ShapePolygon::poly_by_bbox(p_pos, p_vec, p_opt, v_pos);
		const auto i_start = (p_opt.m_end_closed ? p_opt.m_vertex_cnt - 1 : 0);
		const auto j_start = (p_opt.m_end_closed ? 0 : 1);
		for (size_t i = i_start, j = j_start; j < p_opt.m_vertex_cnt; i = j++) {
			brush->SetColor(Shape::s_background_color);
			target->DrawLine(v_pos[i], v_pos[j], brush, s_width, nullptr);
			brush->SetColor(Shape::s_foreground_color);
			target->DrawLine(v_pos[i], v_pos[j], brush, s_width, Shape::m_aux_style.get());
		}
	}

	// 方形の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapePage::draw_auxiliary_rect(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// ページの倍率にかかわらず見た目の太さを変えないため, その逆数を線の太さに格納する.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// 線の太さ
		const D2D1_RECT_F rc = {
			p_pos.x, p_pos.y, c_pos.x, c_pos.y
		};
		brush->SetColor(Shape::s_background_color);
		target->DrawRectangle(&rc, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawRectangle(&rc, brush, s_width, Shape::m_aux_style.get());
	}

	// 角丸方形の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	// c_rad	角丸半径
	void ShapePage::draw_auxiliary_rrect(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		// ページの倍率にかかわらず見た目の太さを変えないため, その逆数を線の太さに格納する.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// 線の太さ
		const double cx = c_pos.x;
		const double cy = c_pos.y;
		const double px = p_pos.x;
		const double py = p_pos.y;
		const double qx = cx - px;
		const double qy = cy - py;
		//auto c_rad = m_corner_radius;
		D2D1_POINT_2F c_rad{ m_grid_base + 1.0f, m_grid_base + 1.0f };
		double rx = c_rad.x;
		double ry = c_rad.y;

		if (qx * rx < 0.0f) {
			rx = -rx;
		}
		if (qy * ry < 0.0f) {
			ry = -ry;
		}
		const D2D1_ROUNDED_RECT r_rect = {
			{ p_pos.x, p_pos.y, c_pos.x, c_pos.y },
			static_cast<FLOAT>(rx),
			static_cast<FLOAT>(ry)
		};
		brush->SetColor(Shape::s_background_color);
		target->DrawRoundedRectangle(&r_rect, brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawRoundedRectangle(&r_rect, brush, s_width, Shape::m_aux_style.get());
	}

	void ShapePage::draw_auxiliary_qellipse(
		ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
		const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_page_scale);	// 線の太さ
		D2D1_ARC_SEGMENT arc{
			c_pos,
			D2D1_SIZE_F{ fabsf(c_pos.x - p_pos.x), fabsf(c_pos.y - p_pos.y) },
			0.0f,
			D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE,
			D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL
		};
		ID2D1Factory* factory = Shape::s_d2d_factory;
		winrt::com_ptr<ID2D1PathGeometry> geom;
		winrt::com_ptr<ID2D1GeometrySink> sink;
		winrt::check_hresult(factory->CreatePathGeometry(geom.put()));
		winrt::check_hresult(geom->Open(sink.put()));
		sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
		sink->BeginFigure(p_pos, D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
		sink->AddArc(arc);
		sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
		winrt::check_hresult(sink->Close());
		sink = nullptr;
		brush->SetColor(Shape::s_background_color);
		target->DrawGeometry(geom.get(), brush, s_width, nullptr);
		brush->SetColor(Shape::s_foreground_color);
		target->DrawGeometry(geom.get(), brush, s_width, Shape::m_aux_style.get());
		geom = nullptr;
	}

	// 図形を表示する.
	void ShapePage::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::s_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::s_d2d_color_brush;

		// ページの色で塗りつぶす.
		target->Clear(m_page_color);
		if (m_grid_show == GRID_SHOW::BACK) {
			// 方眼の表示が最背面に表示の場合,
			// 方眼を表示する.
			page_draw_grid(
				target,
				brush,
				m_grid_base,
				m_grid_color,
				m_grid_emph,
				m_grid_offset,
				m_page_scale,
				m_page_size);
		}
		for (auto s : m_shape_list) {
			if (!s->is_deleted()) {
				// 図形を表示する.
				s->draw();
			}
		}
		if (m_grid_show == GRID_SHOW::FRONT) {
			// 方眼の表示が最前面に表示の場合,
			// 方眼を表示する.
			page_draw_grid(
				target,
				brush,
				m_grid_base,
				m_grid_color,
				m_grid_emph,
				m_grid_offset,
				m_page_scale,
				m_page_size);
		}
	}

	// 方眼を表示する.
	// d2d	描画環境
	// g_offset	方眼のずらし量
	static void page_draw_grid(
		ID2D1RenderTarget* const target,
		ID2D1SolidColorBrush* const brush,
		const float grid_base,
		const D2D1_COLOR_F grid_color,
		const GRID_EMPH grid_emph,
		const D2D1_POINT_2F grid_offset,
		const float page_scale,
		const D2D1_SIZE_F page_size
	)
	{
		// 拡大されても 1 ピクセルになるよう拡大率の逆数を線枠の太さに格納する.
		const FLOAT grid_width = static_cast<FLOAT>(1.0 / page_scale);	// 方眼の太さ
		D2D1_POINT_2F h_start, h_end;	// 横の方眼の開始・終了位置
		D2D1_POINT_2F v_start, v_end;	// 縦の方眼の開始・終了位置
		brush->SetColor(grid_color);
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		const auto page_h = page_size.height;
		const auto page_w = page_size.width;
		v_end.y = page_size.height - 1.0f;
		h_end.x = page_size.width - 1.0f;
		const double grid_len = max(grid_base + 1.0, 1.0);

		// 垂直な方眼を表示する.
		float w;
		double x;
		for (uint32_t i = 0; (x = round((grid_len * i + grid_offset.x) / PT_ROUND) * PT_ROUND) < page_w; i++) {
			if (grid_emph.m_gauge_2 != 0 && (i % grid_emph.m_gauge_2) == 0) {
				w = 2.0F * grid_width;
			}
			else if (grid_emph.m_gauge_1 != 0 && (i % grid_emph.m_gauge_1) == 0) {
				w = grid_width;
			}
			else {
				w = 0.5F * grid_width;
			}
			v_start.x = v_end.x = static_cast<FLOAT>(x);
			target->DrawLine(v_start, v_end, brush, w, nullptr);
		}
		// 水平な方眼を表示する.
		double y;
		for (uint32_t i = 0; (y = round((grid_len * i + grid_offset.y) / PT_ROUND) * PT_ROUND) < page_h; i++) {
			if (grid_emph.m_gauge_2 != 0 && (i % grid_emph.m_gauge_2) == 0) {
				w = 2.0F * grid_width;
			}
			else if (grid_emph.m_gauge_1 != 0 && (i % grid_emph.m_gauge_1) == 0) {
				w = grid_width;
			}
			else {
				w = 0.5F * grid_width;
			}
			h_start.y = h_end.y = static_cast<FLOAT>(y);
			target->DrawLine(h_start, h_end, brush, w, nullptr);
		}

	}

	// 矢じるしの寸法を得る.
	bool ShapePage::get_arrow_size(ARROW_SIZE& val) const noexcept
	{
		val = m_arrow_size;
		return true;
	}

	// 矢じるしの形式を得る.
	bool ShapePage::get_arrow_style(ARROW_STYLE& val) const noexcept
	{
		val = m_arrow_style;
		return true;
	}

	// 画像の不透明度を得る.
	bool ShapePage::get_image_opacity(float& val) const noexcept
	{
		val = m_image_opac;
		return true;
	}

	// 角丸半径を得る.
	/*
	bool ShapePage::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_radius;
		return true;
	}
	*/

	// 塗りつぶし色を得る.
	bool ShapePage::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// 書体の色を得る.
	bool ShapePage::get_font_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_font_color;
		return true;
	}

	// 書体名を得る.
	bool ShapePage::get_font_family(wchar_t*& val) const noexcept
	{
		val = m_font_family;
		return true;
	}

	// 書体の大きさを得る.
	bool ShapePage::get_font_size(float& val) const noexcept
	{
		val = m_font_size;
		return true;
	}

	// 書体の幅を得る.
	bool ShapePage::get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept
	{
		val = m_font_stretch;
		return true;
	}

	// 書体の字体を得る.
	bool ShapePage::get_font_style(DWRITE_FONT_STYLE& val) const noexcept
	{
		val = m_font_style;
		return true;
	}

	// 書体の太さを得る.
	bool ShapePage::get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept
	{
		val = m_font_weight;
		return true;
	}

	// 方眼の基準の大きさを得る.
	bool ShapePage::get_grid_base(float& val) const noexcept
	{
		val = m_grid_base;
		return true;
	}

	// 方眼の色を得る.
	bool ShapePage::get_grid_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_grid_color;
		return true;
	}

	// 方眼の強調を得る.
	bool ShapePage::get_grid_emph(GRID_EMPH& val) const noexcept
	{
		val = m_grid_emph;
		return true;
	}

	// 方眼の表示を得る.
	bool ShapePage::get_grid_show(GRID_SHOW& val) const noexcept
	{
		val = m_grid_show;
		return true;
	}

	// 方眼に合わせるを得る.
	bool ShapePage::get_grid_snap(bool& val) const noexcept
	{
		val = m_grid_snap;
		return true;
	}

	// ページの色を得る.
	bool ShapePage::get_page_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_page_color;
		return true;
	}

	// ページ倍率を得る.
	bool ShapePage::get_page_scale(float& val) const noexcept
	{
		val = m_page_scale;
		return true;
	}

	// ページの大きさを得る.
	bool ShapePage::get_page_size(D2D1_SIZE_F& val) const noexcept
	{
		val = m_page_size;
		return true;
	}

	// 端の形式を得る.
	bool ShapePage::get_stroke_cap(CAP_STYLE& val) const noexcept
	{
		val = m_stroke_cap;
		return true;
	}

	// 線枠の色を得る.
	bool ShapePage::get_stroke_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_stroke_color;
		return true;
	}

	// 破線の端の形式を得る.
	bool ShapePage::get_dash_cap(D2D1_CAP_STYLE& val) const noexcept
	{
		val = m_dash_cap;
		return true;
	}

	// 破線の配置を得る.
	bool ShapePage::get_dash_patt(DASH_PATT& val) const noexcept
	{
		val = m_dash_patt;
		return true;
	}

	// 線枠の形式を得る.
	bool ShapePage::get_dash_style(D2D1_DASH_STYLE& val) const noexcept
	{
		val = m_dash_style;
		return true;
	}

	// 線分の結合の尖り制限を得る.
	bool ShapePage::get_join_miter_limit(float& val) const noexcept
	{
		val = m_join_miter_limit;
		return true;
	}

	// 線分の結合を得る.
	bool ShapePage::get_join_style(D2D1_LINE_JOIN& val) const noexcept
	{
		val = m_join_style;
		return true;
	}

	// 線枠の太さを得る.
	bool ShapePage::get_stroke_width(float& val) const noexcept
	{
		val = m_stroke_width;
		return true;
	}

	// 段落の揃えを得る.
	bool ShapePage::get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_vert;
		return true;
	}

	// 文字列のそろえを得る.
	bool ShapePage::get_text_align_horz(DWRITE_TEXT_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_horz;
		return true;
	}

	// 行間を得る.
	bool ShapePage::get_text_line_sp(float& val) const noexcept
	{
		val = m_text_line_sp;
		return true;
	}

	// 文字列の余白を得る.
	bool ShapePage::get_text_padding(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_padding;
		return true;
	}

	// 図形をデータリーダーから読み込む.
	void ShapePage::read(DataReader const& dt_reader)
	{
		constexpr double GRID_BASE_MAX = 127;
		// 方眼の大きさ
		const auto grid_base = dt_reader.ReadSingle();
		if (grid_base >= 0.0f && grid_base <= GRID_BASE_MAX) {
			m_grid_base = grid_base;
		}
		// 方眼の色
		const D2D1_COLOR_F grid_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if ((grid_color.r >= 0.0f && grid_color.r <= 1.0f) &&
			(grid_color.g >= 0.0f && grid_color.g <= 1.0f) &&
			(grid_color.b >= 0.0f && grid_color.b <= 1.0f) &&
			(grid_color.a >= 0.0f && grid_color.a <= 1.0f)) {
			m_grid_color = grid_color;
		}

		// 方眼の強調
		const GRID_EMPH grid_emph{
			dt_reader.ReadUInt32(),
			dt_reader.ReadUInt32()
		};
		if (equal(grid_emph, GRID_EMPH_0) ||
			equal(grid_emph, GRID_EMPH_2) ||
			equal(grid_emph, GRID_EMPH_3)) {
			m_grid_emph = grid_emph;
		}
		// 方眼の表示
		const GRID_SHOW grid_show = static_cast<GRID_SHOW>(dt_reader.ReadUInt32());
		if (m_grid_show == GRID_SHOW::HIDE ||
			m_grid_show == GRID_SHOW::BACK ||
			m_grid_show == GRID_SHOW::FRONT) {
			m_grid_show = grid_show;
		}
		// 方眼に合わせる.
		m_grid_snap = dt_reader.ReadBoolean();
		// ページの色
		const D2D1_COLOR_F page_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (page_color.r >= 0.0f && page_color.r <= 1.0f &&
			page_color.g >= 0.0f && page_color.g <= 1.0f &&
			page_color.b >= 0.0f && page_color.b <= 1.0f &&
			page_color.a >= 0.0f && page_color.a <= 1.0f) {
			m_page_color = page_color;
		}
		// ページの倍率
		const float page_scale = dt_reader.ReadSingle();
		if (page_scale >= 0.25f && page_scale <= 4.0f) {
			m_page_scale = page_scale;
		}
		// ページの大きさ
		const D2D1_SIZE_F page_size{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (page_size.width >= 1.0f && page_size.width <= PAGE_SIZE_MAX &&
			page_size.height >= 1.0f && page_size.height <= PAGE_SIZE_MAX) {
			m_page_size = page_size;
		}
		// 矢じるしの寸法
		const ARROW_SIZE arrow_size{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (arrow_size.m_width >= 0.0f && arrow_size.m_width <= 127.5f &&
			arrow_size.m_length >= 0.0f && arrow_size.m_length <= 127.5f &&
			arrow_size.m_offset >= 0.0f && arrow_size.m_offset <= 127.5f) {
			m_arrow_size = arrow_size;
		}
		// 矢じるしの形式
		const ARROW_STYLE arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadUInt32());
		if (arrow_style == ARROW_STYLE::NONE ||
			arrow_style == ARROW_STYLE::OPENED ||
			arrow_style == ARROW_STYLE::FILLED) {
			m_arrow_style = arrow_style;
		}
		// 角丸半径
		//const D2D1_POINT_2F corner_rad{
		//	dt_reader.ReadSingle(),
		//	dt_reader.ReadSingle()
		//};
		//m_corner_radius = corner_rad;
		// 端の形式
		const CAP_STYLE stroke_cap{
			static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32()),
			static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32())
		};
		if ((stroke_cap.m_start == D2D1_CAP_STYLE_FLAT ||
			stroke_cap.m_start == D2D1_CAP_STYLE_ROUND ||
			stroke_cap.m_start == D2D1_CAP_STYLE_SQUARE ||
			stroke_cap.m_start == D2D1_CAP_STYLE_TRIANGLE) &&
			stroke_cap.m_start == m_stroke_cap.m_end) {
			m_stroke_cap = stroke_cap;
		}
		// 線・枠の色
		const D2D1_COLOR_F stroke_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (stroke_color.r >= 0.0f && stroke_color.r <= 1.0f &&
			stroke_color.g >= 0.0f && stroke_color.g <= 1.0f &&
			stroke_color.b >= 0.0f && stroke_color.b <= 1.0f &&
			stroke_color.a >= 0.0f && stroke_color.a <= 1.0f) {
			m_stroke_color = stroke_color;
		}
		// 線・枠の太さ
		const float stroke_width = dt_reader.ReadSingle();
		if (stroke_width >= 0.0f && stroke_width <= 127.5f) {
			m_stroke_width = stroke_width;
		}
		// 破線の端の形式
		const D2D1_CAP_STYLE dash_cap = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		if (dash_cap == D2D1_CAP_STYLE_FLAT ||
			dash_cap == D2D1_CAP_STYLE_ROUND ||
			dash_cap == D2D1_CAP_STYLE_SQUARE ||
			dash_cap == D2D1_CAP_STYLE_TRIANGLE) {
			m_dash_cap = dash_cap;
		}
		// 破線の配置
		const DASH_PATT dash_patt{
			{
				dt_reader.ReadSingle(), dt_reader.ReadSingle(),
				dt_reader.ReadSingle(), dt_reader.ReadSingle(),
				dt_reader.ReadSingle(), dt_reader.ReadSingle()
			}
		};
		m_dash_patt = dash_patt;
		// 破線の形式
		const D2D1_DASH_STYLE dash_style = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());
		if (dash_style == D2D1_DASH_STYLE_SOLID ||
			dash_style == D2D1_DASH_STYLE_CUSTOM) {
			m_dash_style = dash_style;
		}
		// 線の結合の形状
		const D2D1_LINE_JOIN join_style = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());
		if (join_style == D2D1_LINE_JOIN_BEVEL ||
			join_style == D2D1_LINE_JOIN_MITER ||
			join_style == D2D1_LINE_JOIN_MITER_OR_BEVEL ||
			join_style == D2D1_LINE_JOIN_ROUND) {
			m_join_style = join_style;
		}
		// 線の結合の尖り制限距離
		const float join_miter_limit = dt_reader.ReadSingle();
		if (join_miter_limit >= 1.0f && join_miter_limit <= 128.5f) {
			m_join_miter_limit = join_miter_limit;
		}
		// 塗りつぶし色
		const D2D1_COLOR_F fill_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (fill_color.r >= 0.0f && fill_color.r <= 1.0f &&
			fill_color.g >= 0.0f && fill_color.g <= 1.0f &&
			fill_color.b >= 0.0f && fill_color.b <= 1.0f &&
			fill_color.a >= 0.0f && fill_color.a <= 1.0f) {
			m_fill_color = fill_color;
		}
		// 書体の色
		const D2D1_COLOR_F font_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (font_color.r >= 0.0f && font_color.r <= 1.0f &&
			font_color.g >= 0.0f && font_color.g <= 1.0f &&
			font_color.b >= 0.0f && font_color.b <= 1.0f &&
			font_color.a >= 0.0f && font_color.a <= 1.0f) {
			m_font_color = font_color;
		}
		// 書体名
		const size_t font_family_len = dt_reader.ReadUInt32();
		uint8_t* font_family_data = new uint8_t[2 * (font_family_len + 1)];
		dt_reader.ReadBytes(array_view(font_family_data, font_family_data + 2 * font_family_len));
		m_font_family = reinterpret_cast<wchar_t*>(font_family_data);
		m_font_family[font_family_len] = L'\0';
		ShapeText::is_available_font(m_font_family);

		// 書体の大きさ
		const float font_size = dt_reader.ReadSingle();
		if (font_size >= 1.0f && font_size <= 128.5f) {
			m_font_size = font_size;
		}

		// 書体の幅
		const DWRITE_FONT_STRETCH font_stretch = static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());
		if (font_stretch == DWRITE_FONT_STRETCH_CONDENSED ||
			font_stretch == DWRITE_FONT_STRETCH_EXPANDED ||
			font_stretch == DWRITE_FONT_STRETCH_EXTRA_CONDENSED ||
			font_stretch == DWRITE_FONT_STRETCH_EXTRA_EXPANDED ||
			font_stretch == DWRITE_FONT_STRETCH_NORMAL ||
			font_stretch == DWRITE_FONT_STRETCH_SEMI_CONDENSED ||
			font_stretch == DWRITE_FONT_STRETCH_SEMI_EXPANDED ||
			font_stretch == DWRITE_FONT_STRETCH_ULTRA_CONDENSED ||
			font_stretch == DWRITE_FONT_STRETCH_ULTRA_EXPANDED) {
			m_font_stretch = font_stretch;
		}
		// 書体の字体
		const DWRITE_FONT_STYLE font_style = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());
		if (font_style == DWRITE_FONT_STYLE_ITALIC ||
			font_style == DWRITE_FONT_STYLE_NORMAL ||
			font_style == DWRITE_FONT_STYLE_OBLIQUE) {
			m_font_style = font_style;
		}
		// 書体の太さ
		const DWRITE_FONT_WEIGHT font_weight = static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());
		if (font_weight == DWRITE_FONT_WEIGHT_THIN || 
			font_weight == DWRITE_FONT_WEIGHT_EXTRA_LIGHT ||
			font_weight == DWRITE_FONT_WEIGHT_LIGHT ||
			font_weight == DWRITE_FONT_WEIGHT_SEMI_LIGHT ||
			font_weight == DWRITE_FONT_WEIGHT_NORMAL ||
			font_weight == DWRITE_FONT_WEIGHT_MEDIUM ||
			font_weight == DWRITE_FONT_WEIGHT_DEMI_BOLD ||
			font_weight == DWRITE_FONT_WEIGHT_BOLD ||
			font_weight == DWRITE_FONT_WEIGHT_EXTRA_BOLD ||
			font_weight == DWRITE_FONT_WEIGHT_BLACK ||
			font_weight == DWRITE_FONT_WEIGHT_EXTRA_BLACK) {
			m_font_weight = font_weight;
		}
		// 段落のそろえ
		const DWRITE_PARAGRAPH_ALIGNMENT text_align_vert = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());
		if (text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT_CENTER ||
			text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT_FAR ||
			text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT_NEAR) {
			m_text_align_vert = text_align_vert;
		}
		// 文字列のそろえ
		const DWRITE_TEXT_ALIGNMENT text_align_horz = static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());
		if (text_align_horz == DWRITE_TEXT_ALIGNMENT_CENTER ||
			text_align_horz == DWRITE_TEXT_ALIGNMENT_JUSTIFIED ||
			text_align_horz == DWRITE_TEXT_ALIGNMENT_LEADING ||
			text_align_horz == DWRITE_TEXT_ALIGNMENT_TRAILING) {
			m_text_align_horz = text_align_horz;
		}
		// 行間
		const float text_line_sp = dt_reader.ReadSingle();
		if (text_line_sp >= 0.0f && text_line_sp <= 127.5f) {
			m_text_line_sp = text_line_sp;
		}
		// 文字列の余白
		const D2D1_SIZE_F text_padding{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		if (text_padding.width >= 0.0 && text_padding.width <= 127.5 &&
			text_padding.height >= 0.0 && text_padding.height <= 127.5) {
			m_text_padding = text_padding;
		}
		// 画像の不透明率
		const float image_opac = dt_reader.ReadSingle();
		if (image_opac >= 0.0f && image_opac <= 1.0f) {
			m_image_opac = image_opac;
		}
		ShapeText::is_available_font(m_font_family);
	}

	// 値を矢じるしの寸法に格納する.
	bool ShapePage::set_arrow_size(const ARROW_SIZE& val) noexcept
	{
		if (!equal(m_arrow_size, val)) {
			m_arrow_size = val;
			return true;
		}
		return false;
	}

	// 値を矢じるしの形式に格納する.
	bool ShapePage::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		const auto old_val = m_arrow_style;
		return (m_arrow_style = val) != old_val;
	}

	// 値を画像の不透明度に格納する.
	bool ShapePage::set_image_opacity(const float val) noexcept
	{
		if (!equal(m_image_opac, val)) {
			m_image_opac = val;
			return true;
		}
		return false;
	}

	// 値を角丸半径に格納する.
	/*
	bool ShapePage::set_corner_radius(const D2D1_POINT_2F& val) noexcept
	{
		if (!equal(m_corner_radius, val)) {
			m_corner_radius = val;
			return true;
		}
		return false;
	}
	*/

	// 値を塗りつぶし色に格納する.
	bool ShapePage::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			return true;
		}
		return false;
	}

	// 値を書体の色に格納する.
	bool ShapePage::set_font_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_font_color, val)) {
			m_font_color = val;
			return true;
		}
		return false;
	}

	// 値を書体名に格納する.
	bool ShapePage::set_font_family(wchar_t* const val) noexcept
	{
		if (!equal(m_font_family, val)) {
			m_font_family = val;
			return true;
		}
		return false;
	}

	// 値を書体の大きさに格納する.
	bool ShapePage::set_font_size(const float val) noexcept
	{
		if (!equal(m_font_size, val)) {
			m_font_size = val;
			return true;
		}
		return false;
	}

	// 値を書体の幅に格納する.
	bool ShapePage::set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept
	{
		const auto old_val = m_font_stretch;
		return (m_font_stretch = val) != old_val;
	}

	// 書体の字体に格納する.
	bool ShapePage::set_font_style(const DWRITE_FONT_STYLE val) noexcept
	{
		const auto old_val = m_font_style;
		return (m_font_style = val) != old_val;
	}

	// 値を書体の太さに格納する.
	bool ShapePage::set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept
	{
		const auto old_val = m_font_weight;
		return (m_font_weight = val) != old_val;
	}

	// 値を方眼の基準の大きさに格納する.
	bool ShapePage::set_grid_base(const float val) noexcept
	{
		if (!equal(m_grid_base, val)) {
			m_grid_base = val;
			return true;
		}
		return false;
	}

	// 値を方眼の濃淡に格納する.
	bool ShapePage::set_grid_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_grid_color, val)) {
			m_grid_color = val;
			return true;
		}
		return false;
	}

	// 値を方眼の強調に格納する.
	bool ShapePage::set_grid_emph(const GRID_EMPH& val) noexcept
	{
		if (!equal(m_grid_emph, val)) {
			m_grid_emph = val;
			return true;
		}
		return false;
	}

	// 値を方眼の表示に格納する.
	bool ShapePage::set_grid_show(const GRID_SHOW val) noexcept
	{
		if (m_grid_show != val) {
			m_grid_show = val;
			return true;
		}
		return false;
	}

	// 値を方眼に合わせるに格納する.
	bool ShapePage::set_grid_snap(const bool val) noexcept
	{
		if (m_grid_snap != val) {
			m_grid_snap = val;
			return true;
		}
		return false;
	}

	// 値を, 表示, 方眼, 補助線の各色に格納する
	bool ShapePage::set_page_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_page_color, val)) {
			m_page_color = val;
			m_page_color.a = 1.0f;
			return true;
		}
		return false;
	}

	// 値をページの倍率に格納する.
	bool ShapePage::set_page_scale(const float val) noexcept
	{
		if (!equal(m_page_scale,val)) {
			m_page_scale = val;
			return true;
		}
		return false;
	}

	// 値を表示の大きさに格納する.
	bool ShapePage::set_page_size(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_page_size, val)) {
			m_page_size = val;
			return true;
		}
		return false;
	}

	// 値を端の形式に格納する.
	bool ShapePage::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (!equal(m_stroke_cap, val)) {
			m_stroke_cap = val;
			return true;
		}
		return false;
	}

	// 線枠の色に格納する.
	bool ShapePage::set_stroke_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_stroke_color, val)) {
			m_stroke_color = val;
			return true;
		}
		return false;
	}

	// 破線の端の形式に格納する.
	bool ShapePage::set_dash_cap(const D2D1_CAP_STYLE& val) noexcept
	{
		const auto old_val = m_dash_cap;
		return (m_dash_cap = val) != old_val;
	}

	// 破線の配置に格納する.
	bool ShapePage::set_dash_patt(const DASH_PATT& val) noexcept
	{
		if (!equal(m_dash_patt, val)) {
			m_dash_patt = val;
			return true;
		}
		return false;
	}

	// 線枠の形式に格納する.
	bool ShapePage::set_dash_style(const D2D1_DASH_STYLE val) noexcept
	{
		const auto old_val = m_dash_style;
		return (m_dash_style = val) != old_val;
	}

	// 値を線分の結合の尖り制限に格納する.
	bool ShapePage::set_join_miter_limit(const float& val) noexcept
	{
		if (!equal(m_join_miter_limit, val)) {
			m_join_miter_limit = val;
			return true;
		}
		return false;
	}

	// 値を線分の結合に格納する.
	bool ShapePage::set_join_style(const D2D1_LINE_JOIN& val) noexcept
	{
		const auto old_val = m_join_style;
		return (m_join_style = val) != old_val;
	}

	// 線枠の太さに格納する.
	bool ShapePage::set_stroke_width(const float val) noexcept
	{
		if (!equal(m_stroke_width, val)) {
			m_stroke_width = val;
			return true;
		}
		return false;
	}

	// 値を段落のそろえに格納する.
	bool ShapePage::set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept
	{
		const auto old_val = m_text_align_vert;
		return (m_text_align_vert = val) != old_val;
	}

	// 文字列のそろえに格納する.
	bool ShapePage::set_text_align_horz(const DWRITE_TEXT_ALIGNMENT val) noexcept
	{
		const auto old_val = m_text_align_horz;
		return (m_text_align_horz = val) != old_val;
	}

	// 値を行間に格納する.
	bool ShapePage::set_text_line_sp(const float val) noexcept
	{
		if (!equal(m_text_line_sp, val)) {
			m_text_line_sp = val;
			return true;
		}
		return false;
	}

	// 値を文字列の余白に格納する.
	bool ShapePage::set_text_padding(const D2D1_SIZE_F val) noexcept
	{
		if (!equal(m_text_padding, val)) {
			m_text_padding = val;
			return true;
		}
		return false;
	}

	// 図形の属性値を表示に格納する.
	// s	図形
	void ShapePage::set_attr_to(const Shape* s) noexcept
	{
		s->get_arrow_size(m_arrow_size);
		s->get_arrow_style(m_arrow_style);
		s->get_dash_cap(m_dash_cap);
		s->get_dash_patt(m_dash_patt);
		s->get_dash_style(m_dash_style);
		//s->get_corner_radius(m_corner_radius);
		s->get_fill_color(m_fill_color);
		s->get_font_color(m_font_color);
		s->get_font_family(m_font_family);
		s->get_font_size(m_font_size);
		s->get_font_stretch(m_font_stretch);
		s->get_font_style(m_font_style);
		s->get_font_weight(m_font_weight);
		s->get_grid_base(m_grid_base);
		s->get_grid_color(m_grid_color);
		s->get_grid_emph(m_grid_emph);
		s->get_grid_show(m_grid_show);
		s->get_grid_snap(m_grid_snap);
		s->get_image_opacity(m_image_opac);
		s->get_join_miter_limit(m_join_miter_limit);
		s->get_join_style(m_join_style);
		s->get_page_color(m_page_color);
		s->get_stroke_cap(m_stroke_cap);
		s->get_stroke_color(m_stroke_color);
		s->get_stroke_width(m_stroke_width);
		s->get_text_align_horz(m_text_align_horz);
		s->get_text_align_vert(m_text_align_vert);
		s->get_text_line_sp(m_text_line_sp);
		s->get_text_padding(m_text_padding);
	}

	// データリーダーに書き込む.
	// dt_writer	データリーダー
	void ShapePage::write(DataWriter const& dt_writer)
	{
		// 方眼の基準の大きさ
		dt_writer.WriteSingle(m_grid_base);
		// 方眼の色
		dt_writer.WriteSingle(m_grid_color.r);
		dt_writer.WriteSingle(m_grid_color.g);
		dt_writer.WriteSingle(m_grid_color.b);
		dt_writer.WriteSingle(m_grid_color.a);
		// 方眼の強調
		dt_writer.WriteUInt32(m_grid_emph.m_gauge_1);
		dt_writer.WriteUInt32(m_grid_emph.m_gauge_2);
		// 方眼の表示
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_grid_show));
		// 方眼に合わせる
		dt_writer.WriteBoolean(m_grid_snap);
		// ページの色
		dt_writer.WriteSingle(m_page_color.r);
		dt_writer.WriteSingle(m_page_color.g);
		dt_writer.WriteSingle(m_page_color.b);
		dt_writer.WriteSingle(m_page_color.a);
		// ページの拡大率
		dt_writer.WriteSingle(m_page_scale);
		// ページの大きさ
		dt_writer.WriteSingle(m_page_size.width);
		dt_writer.WriteSingle(m_page_size.height);
		// 矢じるしの大きさ
		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
		// 矢じるしの形式
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
		// 角丸半径
		//dt_writer.WriteSingle(m_corner_radius.x);
		//dt_writer.WriteSingle(m_corner_radius.y);
		// 線の端の形式
		dt_writer.WriteUInt32(m_stroke_cap.m_start);
		dt_writer.WriteUInt32(m_stroke_cap.m_end);
		// 線・枠の色
		dt_writer.WriteSingle(m_stroke_color.r);
		dt_writer.WriteSingle(m_stroke_color.g);
		dt_writer.WriteSingle(m_stroke_color.b);
		dt_writer.WriteSingle(m_stroke_color.a);
		// 線・枠の太さ
		dt_writer.WriteSingle(m_stroke_width);
		// 破線の端の形式
		dt_writer.WriteUInt32(m_dash_cap);
		// 破線の配置
		dt_writer.WriteSingle(m_dash_patt.m_[0]);
		dt_writer.WriteSingle(m_dash_patt.m_[1]);
		dt_writer.WriteSingle(m_dash_patt.m_[2]);
		dt_writer.WriteSingle(m_dash_patt.m_[3]);
		dt_writer.WriteSingle(m_dash_patt.m_[4]);
		dt_writer.WriteSingle(m_dash_patt.m_[5]);
		// 破線の形式
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_dash_style));
		// 線分の結合
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_join_style));
		// 結合の尖り制限
		dt_writer.WriteSingle(m_join_miter_limit);
		// 塗りつぶし色
		dt_writer.WriteSingle(m_fill_color.r);
		dt_writer.WriteSingle(m_fill_color.g);
		dt_writer.WriteSingle(m_fill_color.b);
		dt_writer.WriteSingle(m_fill_color.a);
		// 書体の色
		dt_writer.WriteSingle(m_font_color.r);
		dt_writer.WriteSingle(m_font_color.g);
		dt_writer.WriteSingle(m_font_color.b);
		dt_writer.WriteSingle(m_font_color.a);
		// 書体名
		const uint32_t font_family_len = wchar_len(m_font_family);
		dt_writer.WriteUInt32(font_family_len);
		const uint8_t* font_family_data = reinterpret_cast<const uint8_t*>(m_font_family);
		dt_writer.WriteBytes(array_view(font_family_data, font_family_data + 2 * font_family_len));
		// 書体の大きさ
		dt_writer.WriteSingle(m_font_size);
		// 書体の幅
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_stretch));
		// 書体の字体
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_style));
		// 書体の太さ
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_weight));
		// 段落のそろえ
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_vert));
		// 文字列のそろえ
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_horz));
		// 行間
		dt_writer.WriteSingle(m_text_line_sp);
		// 文字列の余白
		dt_writer.WriteSingle(m_text_padding.width);
		dt_writer.WriteSingle(m_text_padding.height);
		// 画像の不透明率
		dt_writer.WriteSingle(m_image_opac);
	}

}