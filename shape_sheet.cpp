//#include <numbers>
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 指定した色と不透明度から反対色を得る.
	//static void get_opposite_color(const D2D1_COLOR_F& src, const double opa, D2D1_COLOR_F& dst) noexcept;

	// 指定した色と不透明度から反対色を得る.
	// src	指定した色
	// opa	指定した不透明度
	// dst	反対色
	/*
	static void get_opposite_color(const D2D1_COLOR_F& src, const double opa, D2D1_COLOR_F& dst) noexcept
	{
		const auto R = src.r;
		const auto G = src.g;
		const auto B = src.b;
		const auto X = max(R, max(G, B)) + min(R, min(G, B));
		const auto Y = 0.29900 * R + 0.58700 * G + 0.11400 * B;
		const auto Cb = -0.168736 * R - 0.331264 * G + 0.5 * B;
		const auto Cr = 0.5 * R - 0.418688 * G - 0.081312 * B;
		//const auto _Y = 1.0 - Y;
		const auto _Cb = Cr;
		const auto _Cr = Cb;
		//const auto _R = _Y + 1.402 * _Cr;
		//const auto _G = _Y - 0.344136 * _Cb - 0.714136 * _Cr;
		//const auto _B = _Y + 1.772 * _Cb;

		const auto _R = (X - R);
		const auto _G = (X - G);
		const auto _B = (X - B);
		const auto _Y = 0.29900 * _R + 0.58700 * _G + 0.11400 * _B;
		if (abs(_Y - Y) > 0.2) {
			dst.r = _R;
			dst.g = _G;
			dst.b = _B;
		}
		else {
			dst.r = Y < 0.5 ? 1.0 : 0.0;
			dst.g = Y < 0.5 ? 1.0 : 0.0;
			dst.b = Y < 0.5 ? 1.0 : 0.0;
		}
		dst.a = opa;
		return;

		dst.r = (src.g <= 0.5f ? 1.0f : 0.0f);
		dst.g = (src.g <= 0.5f ? 1.0f : 0.0f);
		dst.b = (src.g <= 0.5f ? 1.0f : 0.0f);
		dst.a = static_cast<FLOAT>(opa);
	}
	*/

	// 曲線の補助線(制御点を結ぶ折れ線)を表示する.
	// dx	図形の描画環境
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapeSheet::draw_auxiliary_bezi(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		//ID2D1SolidColorBrush* br = dx.m_aux_brush.get();
		//ID2D1StrokeStyle* ss = dx.m_aux_style.get();
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);
		const auto s_brush = dx.m_shape_brush.get();
		const auto a_style = dx.m_aux_style.get();
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F e_pos;

		e_pos.x = c_pos.x;
		e_pos.y = p_pos.y;
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(p_pos, e_pos, s_brush, s_width, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(p_pos, e_pos, s_brush, s_width, a_style);
		s_pos = e_pos;
		e_pos.x = p_pos.x;
		e_pos.y = c_pos.y;
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, s_brush, s_width, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, s_brush, s_width, a_style);
		s_pos = e_pos;
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, c_pos, s_brush, s_width, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, c_pos, s_brush, s_width, a_style);
	}

	// だ円の補助線を表示する.
	// dx	図形の描画環境
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapeSheet::draw_auxiliary_elli(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		//auto br = dx.m_aux_brush.get();
		//auto ss = dx.m_aux_style.get();
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);
		D2D1_POINT_2F rect;	// 方形
		D2D1_ELLIPSE elli;		// だ円

		pt_sub(c_pos, p_pos, rect);
		pt_mul(rect, 0.5, rect);
		pt_add(p_pos, rect, elli.point);
		elli.radiusX = rect.x;
		elli.radiusY = rect.y;
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawEllipse(elli, dx.m_shape_brush.get(), s_width, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawEllipse(elli, dx.m_shape_brush.get(), s_width, dx.m_aux_style.get());
	}

	// 直線の補助線を表示する.
	// dx	図形の描画環境
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapeSheet::draw_auxiliary_line(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		//auto br = dx.m_aux_brush.get();
		//auto ss = dx.m_aux_style.get();
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(p_pos, c_pos, dx.m_shape_brush.get(), s_width, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(p_pos, c_pos, dx.m_shape_brush.get(), s_width, dx.m_aux_style.get());
	}

	// ひし形の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	/*
	void ShapeSheet::draw_auxiliary_quad(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		//auto br = dx.m_aux_brush.get();
		//auto ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_sheet_scale);
		D2D1_POINT_2F m_pos;
		D2D1_POINT_2F q_pos[4];

		pt_avg(p_pos, c_pos, m_pos);
		q_pos[0] = { m_pos.x, p_pos.y };
		q_pos[1] = { c_pos.x, m_pos.y };
		q_pos[2] = { m_pos.x, c_pos.y };
		q_pos[3] = { p_pos.x, m_pos.y };
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(q_pos[0], q_pos[1], dx.m_shape_brush.get(), sw, nullptr);
		dx.m_d2dContext->DrawLine(q_pos[1], q_pos[2], dx.m_shape_brush.get(), sw, nullptr);
		dx.m_d2dContext->DrawLine(q_pos[2], q_pos[3], dx.m_shape_brush.get(), sw, nullptr);
		dx.m_d2dContext->DrawLine(q_pos[3], q_pos[0], dx.m_shape_brush.get(), sw, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(q_pos[0], q_pos[1], dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		dx.m_d2dContext->DrawLine(q_pos[1], q_pos[2], dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		dx.m_d2dContext->DrawLine(q_pos[2], q_pos[3], dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		dx.m_d2dContext->DrawLine(q_pos[3], q_pos[0], dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
	}
	*/

	// 多角形の補助線を表示する.
	// dx	図形の描画環境
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	// t_poly	多角形の作図ツール
	void ShapeSheet::draw_auxiliary_poly(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos, const TOOL_POLY& t_poly)
	{
		// 表示倍率にかかわらず見た目の太さを変えないため, その逆数を線の太さに格納する.
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);	// 線の太さ
		std::vector<D2D1_POINT_2F> v_pos(t_poly.m_vertex_cnt);	// 頂点の配列

		D2D1_POINT_2F diff;
		pt_sub(c_pos, p_pos, diff);
		ShapePoly::create_poly_by_bbox(p_pos, diff, t_poly.m_vertex_cnt, t_poly.m_vertex_up, t_poly.m_regular, t_poly.m_clockwise, v_pos.data());
		const auto i_start = (t_poly.m_closed ? t_poly.m_vertex_cnt - 1 : 0);
		const auto j_start = (t_poly.m_closed ? 0 : 1);
		for (size_t i = i_start, j = j_start; j < t_poly.m_vertex_cnt; i = j++) {
			dx.m_shape_brush->SetColor(dx.m_theme_background);
			dx.m_d2dContext->DrawLine(v_pos[i], v_pos[j], dx.m_shape_brush.get(), s_width, nullptr);
			dx.m_shape_brush->SetColor(dx.m_theme_foreground);
			dx.m_d2dContext->DrawLine(v_pos[i], v_pos[j], dx.m_shape_brush.get(), s_width, dx.m_aux_style.get());
		}
	}

	// 方形の補助線を表示する.
	// dx	図形の描画環境
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapeSheet::draw_auxiliary_rect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);
		const D2D1_RECT_F rc = {
			p_pos.x, p_pos.y, c_pos.x, c_pos.y
		};
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawRectangle(&rc, dx.m_shape_brush.get(), s_width, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawRectangle(&rc, dx.m_shape_brush.get(), s_width, dx.m_aux_style.get());
	}

	// 角丸方形の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	// c_rad	角丸半径
	void ShapeSheet::draw_auxiliary_rrect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		const FLOAT s_width = static_cast<FLOAT>(1.0 / m_sheet_scale);
		const double cx = c_pos.x;
		const double cy = c_pos.y;
		const double px = p_pos.x;
		const double py = p_pos.y;
		const double qx = cx - px;
		const double qy = cy - py;
		auto c_rad = m_corner_rad;
		double rx = c_rad.x;
		double ry = c_rad.y;

		if (qx * rx < 0.0f) {
			rx = -rx;
		}
		if (qy * ry < 0.0f) {
			ry = -ry;
		}
		const D2D1_ROUNDED_RECT rr = {
			{ p_pos.x, p_pos.y, c_pos.x, c_pos.y },
			static_cast<FLOAT>(rx),
			static_cast<FLOAT>(ry)
		};
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawRoundedRectangle(&rr, dx.m_shape_brush.get(), s_width, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawRoundedRectangle(&rr, dx.m_shape_brush.get(), s_width, dx.m_aux_style.get());
	}

	// 方眼を表示する.
	// dx	描画環境
	// g_offset	方眼のずらし量
	void ShapeSheet::draw_grid(SHAPE_DX const& dx, const D2D1_POINT_2F g_offset)
	{
		const double sheet_w = m_sheet_size.width;	// 用紙の幅
		const double sheet_h = m_sheet_size.height;	// 用紙の高さ
		// 拡大されても 1 ピクセルになるよう拡大率の逆数を線枠の太さに格納する.
		const FLOAT grid_w = static_cast<FLOAT>(1.0 / m_sheet_scale);	// 方眼の太さ
		D2D1_POINT_2F h_start, h_end;	// 横の方眼の開始・終了位置
		D2D1_POINT_2F v_start, v_end;	// 縦の方眼の開始・終了位置
		auto const& brush = dx.m_shape_brush.get();

		D2D1_COLOR_F grid_color;
		get_grid_color(grid_color);
		brush->SetColor(grid_color);
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		v_end.y = m_sheet_size.height;
		h_end.x = m_sheet_size.width;
		const double grid_len = max(m_grid_base + 1.0, 1.0);

		// 垂直な方眼を表示する.
		float w;
		double x;
		for (uint32_t i = 0; (x = grid_len * i + g_offset.x) < sheet_w; i++) {
			if (m_grid_emph.m_gauge_2 != 0 && (i % m_grid_emph.m_gauge_2) == 0) {
				w = 2.0F * grid_w;
			}
			else if (m_grid_emph.m_gauge_1 != 0 && (i % m_grid_emph.m_gauge_1) == 0) {
				w = grid_w;
			}
			else {
				w = 0.5F * grid_w;
			}
			v_start.x = v_end.x = static_cast<FLOAT>(x);
			dx.m_d2dContext->DrawLine(v_start, v_end, brush, w, nullptr);
		}
		// 水平な方眼を表示する.
		double y;
		for (uint32_t i = 0; (y = grid_len * i + g_offset.y) < sheet_h; i++) {
			if (m_grid_emph.m_gauge_2 != 0 && (i % m_grid_emph.m_gauge_2) == 0) {
				w = 2.0F * grid_w;
			}
			else if (m_grid_emph.m_gauge_1 != 0 && (i % m_grid_emph.m_gauge_1) == 0) {
				w = grid_w;
			}
			else {
				w = 0.5F * grid_w;
			}
			h_start.y = h_end.y = static_cast<FLOAT>(y);
			dx.m_d2dContext->DrawLine(h_start, h_end, brush, w, nullptr);
		}

	}

	// 部位の色を得る.
	//void ShapeSheet::get_anchor_color(D2D1_COLOR_F& value) const noexcept
	//{
	//	get_opposite_color(m_sheet_color, ANCH_OPAC, value);
	//}

	// 矢じりの寸法を得る.
	bool ShapeSheet::get_arrow_size(ARROWHEAD_SIZE& value) const noexcept
	{
		value = m_arrow_size;
		return true;
	}

	// 矢じりの形式を得る.
	bool ShapeSheet::get_arrow_style(ARROWHEAD_STYLE& value) const noexcept
	{
		value = m_arrow_style;
		return true;
	}

	// 補助線の色を得る.
	//void ShapeSheet::get_auxiliary_color(D2D1_COLOR_F& value) const noexcept
	//{
	//	get_opposite_color(m_sheet_color, AUX_OPAC, value);
	//}

	// 角丸半径を得る.
	bool ShapeSheet::get_corner_radius(D2D1_POINT_2F& value) const noexcept
	{
		value = m_corner_rad;
		return true;
	}

	// 塗りつぶしの色を得る.
	bool ShapeSheet::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// 書体の色を得る.
	bool ShapeSheet::get_font_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_font_color;
		return true;
	}

	// 書体名を得る.
	bool ShapeSheet::get_font_family(wchar_t*& value) const noexcept
	{
		value = m_font_family;
		return true;
	}

	// 書体の大きさを得る.
	bool ShapeSheet::get_font_size(float& value) const noexcept
	{
		value = m_font_size;
		return true;
	}

	// 書体の伸縮を得る.
	bool ShapeSheet::get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept
	{
		value = m_font_stretch;
		return true;
	}

	// 書体の字体を得る.
	bool ShapeSheet::get_font_style(DWRITE_FONT_STYLE& value) const noexcept
	{
		value = m_font_style;
		return true;
	}

	// 書体の太さを得る.
	bool ShapeSheet::get_font_weight(DWRITE_FONT_WEIGHT& value) const noexcept
	{
		value = m_font_weight;
		return true;
	}

	// 方眼の基準の大きさを得る.
	bool ShapeSheet::get_grid_base(float& value) const noexcept
	{
		value = m_grid_base;
		return true;
	}

	// 方眼の濃淡を得る.
	void ShapeSheet::get_grid_color(D2D1_COLOR_F& value) const noexcept
	{
		value.r = 1.0f - m_grid_gray;
		value.g = value.r;
		value.b = value.r;
		value.a = 0.875F;
	}

	// 方眼の濃淡を得る.
	bool ShapeSheet::get_grid_gray(float& value) const noexcept
	{
		value = m_grid_gray;
		return true;
	}

	// 方眼の強調を得る.
	bool ShapeSheet::get_grid_emph(GRID_EMPH& value) const noexcept
	{
		value = m_grid_emph;
		return true;
	}

	// 方眼の表示を得る.
	bool ShapeSheet::get_grid_show(GRID_SHOW& value) const noexcept
	{
		value = m_grid_show;
		return true;
	}

	// 方眼にそろえるを得る.
	bool ShapeSheet::get_grid_snap(bool& value) const noexcept
	{
		value = m_grid_snap;
		return true;
	}

	// 用紙の色を得る.
	bool ShapeSheet::get_sheet_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_sheet_color;
		return true;
	}

	// 用紙の拡大率を得る.
	bool ShapeSheet::get_sheet_scale(float& value) const noexcept
	{
		value = m_sheet_scale;
		return true;
	}

	// 用紙の寸法を得る.
	bool ShapeSheet::get_sheet_size(D2D1_SIZE_F& value) const noexcept
	{
		value = m_sheet_size;
		return true;
	}

	// 線枠の色を得る.
	bool ShapeSheet::get_stroke_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_stroke_color;
		return true;
	}

	// 破線の配置を得る.
	bool ShapeSheet::get_stroke_patt(STROKE_PATT& value) const noexcept
	{
		value = m_stroke_patt;
		return true;
	}

	// 線枠の形式を得る.
	bool ShapeSheet::get_stroke_style(D2D1_DASH_STYLE& value) const noexcept
	{
		value = m_stroke_style;
		return true;
	}

	// 線枠の太さを得る.
	bool ShapeSheet::get_stroke_width(float& value) const noexcept
	{
		value = m_stroke_width;
		return true;
	}

	// 段落の揃えを得る.
	bool ShapeSheet::get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& value) const noexcept
	{
		value = m_text_align_p;
		return true;
	}

	// 文字列のそろえを得る.
	bool ShapeSheet::get_text_align_t(DWRITE_TEXT_ALIGNMENT& value) const noexcept
	{
		value = m_text_align_t;
		return true;
	}

	// 行間を得る.
	bool ShapeSheet::get_text_line(float& value) const noexcept
	{
		value = m_text_line;
		return true;
	}

	// 文字列の余白を得る.
	bool ShapeSheet::get_text_margin(D2D1_SIZE_F& value) const noexcept
	{
		value = m_text_margin;
		return true;
	}

	// データリーダーから読み込む.
	void ShapeSheet::read(DataReader const& dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		D2D1_COLOR_F dummy;
		read(dummy, dt_reader);
		m_grid_base = dt_reader.ReadSingle();
		m_grid_gray = dt_reader.ReadSingle();
		read(m_grid_emph, dt_reader);
		m_grid_show = static_cast<GRID_SHOW>(dt_reader.ReadUInt32());
		m_grid_snap = dt_reader.ReadBoolean();
		read(m_sheet_color, dt_reader);
		m_sheet_scale = dt_reader.ReadSingle();
		read(m_sheet_size, dt_reader);

		read(m_arrow_size, dt_reader);	// 矢じりの寸法
		m_arrow_style = static_cast<ARROWHEAD_STYLE>(dt_reader.ReadUInt32());	// 矢じりの形式
		read(m_corner_rad, dt_reader);	// 角丸半径
		read(m_stroke_color, dt_reader);	// 線・枠の色
		m_stroke_join_style = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());	// 角の形状
		m_stroke_join_limit = dt_reader.ReadSingle();	// 角のマイター制限
		read(m_stroke_patt, dt_reader);	// 破線の配置
		m_stroke_style = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());	// 破線の形式
		m_stroke_width = dt_reader.ReadSingle();	// 線・枠の太さ
		read(m_fill_color, dt_reader);	// 塗りつぶしの色
		read(m_font_color, dt_reader);	// 書体の色
		read(m_font_family, dt_reader);	// 書体名
		m_font_size = dt_reader.ReadSingle();	// 書体の大きさ
		m_font_stretch = static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());	// 書体の伸縮
		m_font_style = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());	// 書体の字体
		m_font_weight = static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());	// 書体の太さ
		m_text_align_p = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());	// 段落のそろえ
		m_text_align_t = static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());	// 文字列のそろえ
		m_text_line = dt_reader.ReadSingle();	// 行間
		read(m_text_margin, dt_reader);	// 文字列の余白

		ShapeText::is_available_font(m_font_family);
	}

	// 値を矢じりの寸法に格納する.
	void ShapeSheet::set_arrow_size(const ARROWHEAD_SIZE& value)
	{
		m_arrow_size = value;
	}

	// 値を矢じりの形式に格納する.
	void ShapeSheet::set_arrow_style(const ARROWHEAD_STYLE value)
	{
		m_arrow_style = value;
	}

	void ShapeSheet::set_corner_radius(const D2D1_POINT_2F& value) noexcept
	{
		m_corner_rad = value;
	}

	// 値を塗りつぶしの色に格納する.
	void ShapeSheet::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		m_fill_color = value;
	}

	// 値を書体の色に格納する.
	void ShapeSheet::set_font_color(const D2D1_COLOR_F& value) noexcept
	{
		m_font_color = value;
	}

	// 値を書体名に格納する.
	void ShapeSheet::set_font_family(wchar_t* const value)
	{
		m_font_family = value;
	}

	// 値を書体の大きさに格納する.
	void ShapeSheet::set_font_size(const float value)
	{
		m_font_size = value;
	}

	// 値を書体の伸縮に格納する.
	void ShapeSheet::set_font_stretch(const DWRITE_FONT_STRETCH value)
	{
		m_font_stretch = value;
	}

	// 書体の字体に格納する.
	void ShapeSheet::set_font_style(const DWRITE_FONT_STYLE value)
	{
		m_font_style = value;
	}

	// 値を書体の太さに格納する.
	void ShapeSheet::set_font_weight(const DWRITE_FONT_WEIGHT value)
	{
		m_font_weight = value;
	}

	// 値を方眼の基準の大きさに格納する.
	void ShapeSheet::set_grid_base(const float value) noexcept
	{
		m_grid_base = value;
	}

	// 値を方眼の濃淡に格納する.
	void ShapeSheet::set_grid_gray(const float value) noexcept
	{
		m_grid_gray = value;
	}

	// 値を方眼の強調に格納する.
	void ShapeSheet::set_grid_emph(const GRID_EMPH& value) noexcept
	{
		m_grid_emph = value;
	}

	// 値を方眼の表示に格納する.
	void ShapeSheet::set_grid_show(const GRID_SHOW value) noexcept
	{
		m_grid_show = value;
	}

	// 値を方眼にそろえるに格納する.
	void ShapeSheet::set_grid_snap(const bool value) noexcept
	{
		m_grid_snap = value;
	}

	// 値を, 用紙, 方眼, 補助線の各色に格納する
	void ShapeSheet::set_sheet_color(const D2D1_COLOR_F& value) noexcept
	{
		m_sheet_color = value;
		m_sheet_color.a = 1.0F;
	}

	// 値を用紙の拡大率に格納する.
	void ShapeSheet::set_sheet_scale(const float value) noexcept
	{
		m_sheet_scale = value;
	}

	// 値を用紙の寸法に格納する.
	void ShapeSheet::set_sheet_size(const D2D1_SIZE_F value) noexcept
	{
		m_sheet_size = value;
	}

	// 線枠の色に格納する.
	void ShapeSheet::set_stroke_color(const D2D1_COLOR_F& value) noexcept
	{
		m_stroke_color = value;
	}

	// 破線の配置に格納する.
	void ShapeSheet::set_stroke_patt(const STROKE_PATT& value)
	{
		m_stroke_patt = value;
	}

	// 線枠の形式に格納する.
	void ShapeSheet::set_stroke_style(const D2D1_DASH_STYLE value)
	{
		m_stroke_style = value;
	}

	// 線枠の太さに格納する.
	void ShapeSheet::set_stroke_width(const float value) noexcept
	{
		m_stroke_width = value;
	}

	// 値を段落のそろえに格納する.
	void ShapeSheet::set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value)
	{
		m_text_align_p = value;
	}

	// 文字列のそろえに格納する.
	void ShapeSheet::set_text_align_t(const DWRITE_TEXT_ALIGNMENT value)
	{
		m_text_align_t = value;
	}

	// 値を行間に格納する.
	void ShapeSheet::set_text_line(const float value)
	{
		m_text_line = value;
	}

	// 値を文字列の余白に格納する.
	void ShapeSheet::set_text_margin(const D2D1_SIZE_F value)
	{
		m_text_margin = value;
	}

	// 図形の属性値を格納する.
	void ShapeSheet::set_to(Shape* s) noexcept
	{
		s->get_arrow_size(m_arrow_size);
		s->get_arrow_style(m_arrow_style);
		s->get_corner_radius(m_corner_rad);
		s->get_fill_color(m_fill_color);
		s->get_font_color(m_font_color);
		s->get_font_family(m_font_family);
		s->get_font_size(m_font_size);
		s->get_font_stretch(m_font_stretch);
		s->get_font_style(m_font_style);
		s->get_font_weight(m_font_weight);
		s->get_grid_base(m_grid_base);
		s->get_grid_gray(m_grid_gray);
		s->get_grid_emph(m_grid_emph);
		s->get_grid_show(m_grid_show);
		s->get_grid_snap(m_grid_snap);
		s->get_sheet_color(m_sheet_color);
		s->get_stroke_color(m_stroke_color);
		s->get_stroke_patt(m_stroke_patt);
		s->get_stroke_style(m_stroke_style);
		s->get_stroke_width(m_stroke_width);
		s->get_text_line(m_text_line);
		s->get_text_align_t(m_text_align_t);
		s->get_text_align_p(m_text_align_p);
		s->get_text_margin(m_text_margin);
	}

	// データリーダーに書き込む.
	void ShapeSheet::write(DataWriter const& dt_writer)
	{
		using winrt::GraphPaper::implementation::write;

		D2D1_COLOR_F dummy;
		write(dummy, dt_writer);
		dt_writer.WriteSingle(m_grid_base);
		dt_writer.WriteSingle(m_grid_gray);
		write(m_grid_emph, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_grid_show));
		dt_writer.WriteBoolean(m_grid_snap);
		write(m_sheet_color, dt_writer);
		dt_writer.WriteSingle(m_sheet_scale);
		write(m_sheet_size, dt_writer);

		write(m_arrow_size, dt_writer);	// 矢じりの寸法
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));	// 矢じりの形式
		write(m_corner_rad, dt_writer);	// 角丸半径
		write(m_stroke_color, dt_writer);	// 線枠の色
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_stroke_join_style));	// 角の形状
		dt_writer.WriteSingle(m_stroke_join_limit);	// 角のマイター制限
		write(m_stroke_patt, dt_writer);	// 破線の配置
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_stroke_style));	// 線枠の形式
		dt_writer.WriteSingle(m_stroke_width);	// 線枠の太さ
		write(m_fill_color, dt_writer);	// 塗りつぶしの色
		write(m_font_color, dt_writer);	// 書体の色
		write(m_font_family, dt_writer);	// 書体名
		dt_writer.WriteSingle(m_font_size);	// 書体の大きさ
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_stretch));	// 書体の伸縮
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_style));	// 書体の字体
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_font_weight));	// 書体の太さ
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_p));	// 段落のそろえ
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_text_align_t));	// 文字列のそろえ
		dt_writer.WriteSingle(m_text_line);	// 行間
		write(m_text_margin, dt_writer);	// 文字列の余白

	}

}