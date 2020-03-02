#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 指定した色と不透明度から反対色を得る.
	static void get_opposite_color(const D2D1_COLOR_F& src, const double opa, D2D1_COLOR_F& dst) noexcept;

	// 指定した色と不透明度から反対色を得る.
	// src	指定した色
	// opa	指定した不透明度
	// dst	反対色
	static void get_opposite_color(const D2D1_COLOR_F& src, const double opa, D2D1_COLOR_F& dst) noexcept
	{
		dst.r = (src.r <= 0.5f ? 1.0f : 0.0f);
		dst.g = (src.g <= 0.5f ? 1.0f : 0.0f);
		dst.b = (src.b <= 0.5f ? 1.0f : 0.0f);
		dst.a = static_cast<FLOAT>(opa);
	}

	// 曲線の補助線(制御点を結ぶ折れ線)を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapePanel::TOOL_auxiliary_bezi(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		ID2D1Brush* br = dx.m_aux_brush.get();
		ID2D1StrokeStyle* ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F e_pos;

		e_pos.x = c_pos.x;
		e_pos.y = p_pos.y;
		dx.m_d2dContext->DrawLine(p_pos, e_pos, br, sw, ss);
		s_pos = e_pos;
		e_pos.x = p_pos.x;
		e_pos.y = c_pos.y;
		dx.m_d2dContext->DrawLine(s_pos, e_pos, br, sw, ss);
		s_pos = e_pos;
		dx.m_d2dContext->DrawLine(s_pos, c_pos, br, sw, ss);
	}

	// だ円の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapePanel::TOOL_auxiliary_elli(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		D2D1_POINT_2F r;	// 方形
		D2D1_ELLIPSE e;		// だ円

		pt_sub(c_pos, p_pos, r);
		pt_scale(r, 0.5, r);
		pt_add(p_pos, r, e.point);
		e.radiusX = r.x;
		e.radiusY = r.y;
		dx.m_d2dContext->DrawEllipse(e, br, sw, ss);
	}

	// 直線の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapePanel::TOOL_auxiliary_line(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		dx.m_d2dContext->DrawLine(p_pos, c_pos, br, sw, ss);
	}

	// ひし形の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapePanel::TOOL_auxiliary_quad(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		D2D1_POINT_2F m_pos;
		D2D1_POINT_2F q_pos[4];

		pt_avg(p_pos, c_pos, m_pos);
		q_pos[0] = { m_pos.x, p_pos.y };
		q_pos[1] = { c_pos.x, m_pos.y };
		q_pos[2] = { m_pos.x, c_pos.y };
		q_pos[3] = { p_pos.x, m_pos.y };
		dx.m_d2dContext->DrawLine(q_pos[0], q_pos[1], br, sw, ss);
		dx.m_d2dContext->DrawLine(q_pos[1], q_pos[2], br, sw, ss);
		dx.m_d2dContext->DrawLine(q_pos[2], q_pos[3], br, sw, ss);
		dx.m_d2dContext->DrawLine(q_pos[3], q_pos[0], br, sw, ss);
	}

	// 方形の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	void ShapePanel::TOOL_auxiliary_rect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		const D2D1_RECT_F rc = {
			p_pos.x, p_pos.y, c_pos.x, c_pos.y
		};
		dx.m_d2dContext->DrawRectangle(&rc, br, sw, ss);
	}

	// 角丸方形の補助線を表示する.
	// p_pos	ポインターが押された位置
	// c_pos	ポインターの現在位置
	// c_rad	角丸半径
	void ShapePanel::TOOL_auxiliary_rrect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos)
	{
		auto br = dx.m_aux_brush.get();
		auto ss = dx.m_aux_style.get();
		auto c_rad = m_corner_rad;
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);
		const double cx = c_pos.x;
		const double cy = c_pos.y;
		const double px = p_pos.x;
		const double py = p_pos.y;
		const double qx = cx - px;
		const double qy = cy - py;
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
		dx.m_d2dContext->DrawRoundedRectangle(&rr, br, sw, ss);
	}

	// 方眼線を表示する.
	// offset	方眼線のずらし量
	void ShapePanel::TOOL_grid_line(SHAPE_DX const& dx, const D2D1_POINT_2F offset)
	{
		const double pw = m_page_size.width;	// ページの大きさ
		const double ph = m_page_size.height;
		// 拡大されても 1 ピクセルになるよう拡大率の逆数を線枠の太さに格納する.
		const FLOAT sw = static_cast<FLOAT>(1.0 / m_page_scale);	// 方眼線の太さ
		D2D1_POINT_2F h_start, h_end;	// 横の方眼線の開始・終了位置
		D2D1_POINT_2F v_start, v_end;	// 縦の方眼線の開始・終了位置
		auto br = dx.m_shape_brush.get();

		m_grid_color.a = static_cast<FLOAT>(m_grid_opac);
		br->SetColor(m_grid_color);
		v_start.y = 0.0f;
		h_start.x = 0.0f;
		v_end.y = m_page_size.height;
		h_end.x = m_page_size.width;
		const double g_len = max(m_grid_len + 1.0, 1.0);
		// 垂直な方眼線を表示する.
		double x;
		for (uint32_t i = 0; (x = g_len * i + offset.x) < pw; i++) {
			v_start.x = v_end.x = static_cast<FLOAT>(x);
			dx.m_d2dContext->DrawLine(v_start, v_end, br, sw, nullptr);
		}
		// 水平な方眼線を表示する.
		double y;
		for (uint32_t i = 0; (y = g_len * i + offset.y) < ph; i++) {
			h_start.y = h_end.y = static_cast<FLOAT>(y);
			dx.m_d2dContext->DrawLine(h_start, h_end, br, sw, nullptr);
		}
	}

	// 矢じりの寸法を得る.
	bool ShapePanel::get_arrow_size(ARROW_SIZE& val) const noexcept
	{
		val = m_arrow_size;
		return true;
	}

	// 矢じりの形式を得る.
	bool ShapePanel::get_arrow_style(ARROW_STYLE& val) const noexcept
	{
		val = m_arrow_style;
		return true;
	}

	// 角丸半径を得る.
	bool ShapePanel::get_corner_radius(D2D1_POINT_2F& val) const noexcept
	{
		val = m_corner_rad;
		return true;
	}

	// 塗りつぶしの色を得る.
	bool ShapePanel::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// 書体の色を得る.
	bool ShapePanel::get_font_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_font_color;
		return true;
	}

	// 書体名を得る.
	bool ShapePanel::get_font_family(wchar_t*& val) const noexcept
	{
		val = m_font_family;
		return true;
	}

	// 書体の大きさを得る.
	bool ShapePanel::get_font_size(double& val) const noexcept
	{
		val = m_font_size;
		return true;
	}

	// 書体の伸縮を得る.
	bool ShapePanel::get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept
	{
		val = m_font_stretch;
		return true;
	}

	// 書体の字体を得る.
	bool ShapePanel::get_font_style(DWRITE_FONT_STYLE& val) const noexcept
	{
		val = m_font_style;
		return true;
	}

	// 書体の太さを得る.
	bool ShapePanel::get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept
	{
		val = m_font_weight;
		return true;
	}

	//	方眼の大きさを得る.
	bool ShapePanel::get_grid_len(double& val) const noexcept
	{
		val = m_grid_len;
		return true;
	}

	// 方眼線の不透明度を得る.
	bool ShapePanel::get_grid_opac(double& val) const noexcept
	{
		val = m_grid_opac;
		return true;
	}

	// 方眼線の表示を得る.
	bool ShapePanel::get_grid_show(GRID_SHOW& val) const noexcept
	{
		val = m_grid_show;
		return true;
	}

	// 方眼へのそろえを得る.
	bool ShapePanel::get_grid_snap(bool& val) const noexcept
	{
		val = m_grid_snap;
		return true;
	}

	// ページの色を得る.
	bool ShapePanel::get_page_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_page_color;
		return true;
	}

	// ページの拡大率を得る.
	bool ShapePanel::get_page_scale(double& val) const noexcept
	{
		val = m_page_scale;
		return true;
	}

	// ページの寸法を得る.
	bool ShapePanel::get_page_size(D2D1_SIZE_F& val) const noexcept
	{
		val = m_page_size;
		return true;
	}

	// 線枠の色を得る.
	bool ShapePanel::get_stroke_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_stroke_color;
		return true;
	}

	// 破線の配置を得る.
	bool ShapePanel::get_stroke_pattern(STROKE_PATTERN& val) const noexcept
	{
		val = m_stroke_pattern;
		return true;
	}

	// 線枠の形式を得る.
	bool ShapePanel::get_stroke_style(D2D1_DASH_STYLE& val) const noexcept
	{
		val = m_stroke_style;
		return true;
	}

	// 線枠の太さを得る.
	bool ShapePanel::get_stroke_width(double& val) const noexcept
	{
		val = m_stroke_width;
		return true;
	}

	// 段落の揃えを得る.
	bool ShapePanel::get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_p;
		return true;
	}

	// 文字列のそろえを得る.
	bool ShapePanel::get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept
	{
		val = m_text_align_t;
		return true;
	}

	// 行間を得る.
	bool ShapePanel::get_text_line(double& val) const noexcept
	{
		val = m_text_line;
		return true;
	}

	// 文字列の余白を得る.
	bool ShapePanel::get_text_margin(D2D1_SIZE_F& val) const noexcept
	{
		val = m_text_mar;
		return true;
	}

	// データリーダーから読み込む.
	void ShapePanel::read(DataReader const& dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_grid_color, dt_reader);
		m_grid_len = dt_reader.ReadDouble();
		m_grid_opac = dt_reader.ReadDouble();
		read(m_grid_show, dt_reader);
		m_grid_snap = dt_reader.ReadBoolean();
		read(m_page_color, dt_reader);
		m_page_scale = dt_reader.ReadDouble();
		read(m_page_size, dt_reader);
uint32_t dummy;
read(dummy, dt_reader);

		read(m_arrow_size, dt_reader);	// 矢じりの寸法
		read(m_arrow_style, dt_reader);	// 矢じりの形式
		read(m_corner_rad, dt_reader);	// 角丸半径
		read(m_stroke_color, dt_reader);	// 線・枠の色
		read(m_stroke_pattern, dt_reader);	// 破線の配置
		read(m_stroke_style, dt_reader);	// 破線の形式
		read(m_stroke_width, dt_reader);	// 線・枠の太さ
		read(m_fill_color, dt_reader);	// 塗りつぶしの色
		read(m_font_color, dt_reader);	// 書体の色
		read(m_font_family, dt_reader);	// 書体名
		read(m_font_size, dt_reader);	// 書体の大きさ
		read(m_font_stretch, dt_reader);	// 書体の伸縮
		read(m_font_style, dt_reader);	// 書体の字体
		read(m_font_weight, dt_reader);	// 書体の太さ
		read(m_text_align_p, dt_reader);	// 段落のそろえ
		read(m_text_align_t, dt_reader);	// 文字列のそろえ
		read(m_text_line, dt_reader);	// 行間
		read(m_text_mar, dt_reader);	// 文字列の余白

		ShapeText::is_available_font(m_font_family);
	}

	// 値を矢じりの寸法に格納する.
	void ShapePanel::set_arrow_size(const ARROW_SIZE& val)
	{
		m_arrow_size = val;
	}

	// 値を矢じりの形式に格納する.
	void ShapePanel::set_arrow_style(const ARROW_STYLE val)
	{
		m_arrow_style = val;
	}

	// 値を塗りつぶしの色に格納する.
	void ShapePanel::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		m_fill_color = val;
	}

	// 値を書体の色に格納する.
	void ShapePanel::set_font_color(const D2D1_COLOR_F& val) noexcept
	{
		m_font_color = val;
	}

	// 値を書体名に格納する.
	void ShapePanel::set_font_family(wchar_t* const val)
	{
		m_font_family = val;
	}

	// 値を書体の大きさに格納する.
	void ShapePanel::set_font_size(const double val)
	{
		m_font_size = val;
	}

	// 値を書体の伸縮に格納する.
	void ShapePanel::set_font_stretch(const DWRITE_FONT_STRETCH val)
	{
		m_font_stretch = val;
	}

	// 書体の字体に格納する.
	void ShapePanel::set_font_style(const DWRITE_FONT_STYLE val)
	{
		m_font_style = val;
	}

	// 値を書体の太さに格納する.
	void ShapePanel::set_font_weight(const DWRITE_FONT_WEIGHT val)
	{
		m_font_weight = val;
	}

	// 値を方眼の大きさに格納する.
	void ShapePanel::set_grid_len(const double val) noexcept
	{
		m_grid_len = val;
	}

	// 値を方眼線の不透明度に格納する.
	void ShapePanel::set_grid_opac(const double val) noexcept
	{
		m_grid_opac = val;
	}

	// 値を方眼線の表示に格納する.
	void ShapePanel::set_grid_show(const GRID_SHOW val) noexcept
	{
		m_grid_show = val;
	}

	// 値を方眼へのそろえに格納する.
	void ShapePanel::set_grid_snap(const bool val) noexcept
	{
		m_grid_snap = val;
	}

	// 値をページ, 方眼, 補助線の色に格納する
	void ShapePanel::set_page_color(const D2D1_COLOR_F& val) noexcept
	{
		m_page_color = val;
		get_opposite_color(val, m_grid_opac, m_grid_color);
		get_opposite_color(val, ANCH_OPAC, m_anch_color);
		get_opposite_color(val, AUX_OPAC, m_aux_color);
	}

	// 値をページの拡大率に格納する.
	void ShapePanel::set_page_scale(const double val) noexcept
	{
		m_page_scale = val;
	}

	// 値をページの寸法に格納する.
	void ShapePanel::set_page_size(const D2D1_SIZE_F val) noexcept
	{
		m_page_size = val;
	}

	// 線枠の色に格納する.
	void ShapePanel::set_stroke_color(const D2D1_COLOR_F& val) noexcept
	{
		m_stroke_color = val;
	}

	// 破線の配置に格納する.
	void ShapePanel::set_stroke_pattern(const STROKE_PATTERN& val)
	{
		m_stroke_pattern = val;
	}

	// 線枠の形式に格納する.
	void ShapePanel::set_stroke_style(const D2D1_DASH_STYLE val)
	{
		m_stroke_style = val;
	}

	// 線枠の太さに格納する.
	void ShapePanel::set_stroke_width(const double val) noexcept
	{
		m_stroke_width = val;
	}

	// 値を段落のそろえに格納する.
	void ShapePanel::set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT val)
	{
		m_text_align_p = val;
	}

	// 文字列のそろえに格納する.
	void ShapePanel::set_text_align_t(const DWRITE_TEXT_ALIGNMENT val)
	{
		m_text_align_t = val;
	}

	// 値を行間に格納する.
	void ShapePanel::set_text_line(const double val)
	{
		m_text_line = val;
	}

	// 値を文字列の余白に格納する.
	void ShapePanel::set_text_margin(const D2D1_SIZE_F val)
	{
		m_text_mar = val;
	}

	// 図形の属性値を格納する.
	void ShapePanel::set_to_shape(Shape* s) noexcept
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
		s->get_grid_opac(m_grid_opac);
		s->get_grid_len(m_grid_len);
		s->get_grid_show(m_grid_show);
		s->get_grid_snap(m_grid_snap);
		s->get_text_line(m_text_line);
		D2D1_COLOR_F color;
		if (s->get_page_color(color)) {
			set_page_color(color);
		}
		s->get_stroke_color(m_stroke_color);
		s->get_stroke_pattern(m_stroke_pattern);
		s->get_stroke_style(m_stroke_style);
		s->get_stroke_width(m_stroke_width);
		s->get_text_align_t(m_text_align_t);
		s->get_text_align_p(m_text_align_p);
		s->get_text_margin(m_text_mar);
	}

	// データリーダーに書き込む.
	void ShapePanel::write(DataWriter const& dt_writer)
	{
		using winrt::GraphPaper::implementation::write;

		write(m_grid_color, dt_writer);
		dt_writer.WriteDouble(m_grid_len);
		dt_writer.WriteDouble(m_grid_opac);
		write(m_grid_show, dt_writer);
		dt_writer.WriteBoolean(m_grid_snap);
		write(m_page_color, dt_writer);
		dt_writer.WriteDouble(m_page_scale);
		write(m_page_size, dt_writer);
uint32_t dummy = 0;
write(dummy, dt_writer);

		write(m_arrow_size, dt_writer);	// 矢じりの寸法
		write(m_arrow_style, dt_writer);	// 矢じりの形式
		write(m_corner_rad, dt_writer);	// 角丸半径
		write(m_stroke_color, dt_writer);	// 線枠の色
		write(m_stroke_pattern, dt_writer);	// 破線の配置
		write(m_stroke_style, dt_writer);	// 線枠の形式
		write(m_stroke_width, dt_writer);	// 線枠の太さ
		write(m_fill_color, dt_writer);	// 塗りつぶしの色
		write(m_font_color, dt_writer);	// 書体の色
		write(m_font_family, dt_writer);	// 書体名
		write(m_font_size, dt_writer);	// 書体の大きさ
		write(m_font_stretch, dt_writer);	// 書体の伸縮
		write(m_font_style, dt_writer);	// 書体の字体
		write(m_font_weight, dt_writer);	// 書体の太さ
		write(m_text_align_p, dt_writer);	// 段落のそろえ
		write(m_text_align_t, dt_writer);	// 文字列のそろえ
		write(m_text_line, dt_writer);	// 行間
		write(m_text_mar, dt_writer);	// 文字列の余白

	}
}