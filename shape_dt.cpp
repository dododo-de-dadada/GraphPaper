//------------------------------
// shape_dt.cpp
// 読み込み, 書き込み.
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	// データリーダーから矢じるしの寸法を読み込む.
	void dt_read(ARROW_SIZE& value, DataReader const& dt_reader)
	{
		value.m_width = dt_reader.ReadSingle();
		value.m_length = dt_reader.ReadSingle();
		value.m_offset = dt_reader.ReadSingle();
	}

	// データリーダーから矢じるしの形式を読み込む.
	void dt_read(CAP_STYLE& value, DataReader const& dt_reader)
	{
		value.m_start = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		value.m_end = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
	}

	// データリーダーから色を読み込む.
	void dt_read(D2D1_COLOR_F& value, DataReader const& dt_reader)
	{
		value.a = dt_reader.ReadSingle();
		value.r = dt_reader.ReadSingle();
		value.g = dt_reader.ReadSingle();
		value.b = dt_reader.ReadSingle();
		value.a = min(max(value.a, 0.0F), 1.0F);
		value.r = min(max(value.r, 0.0F), 1.0F);
		value.g = min(max(value.g, 0.0F), 1.0F);
		value.b = min(max(value.b, 0.0F), 1.0F);
	}

	// データリーダーから位置を読み込む.
	void dt_read(D2D1_POINT_2F& value, DataReader const& dt_reader)
	{
		value.x = dt_reader.ReadSingle();
		value.y = dt_reader.ReadSingle();
	}

	// データリーダーから寸法を読み込む.
	void dt_read(D2D1_SIZE_F& value, DataReader const& dt_reader)
	{
		value.width = dt_reader.ReadSingle();
		value.height = dt_reader.ReadSingle();
	}

	// データリーダーから寸法を読み込む.
	void dt_read(D2D1_SIZE_U& value, DataReader const& dt_reader)
	{
		value.width = dt_reader.ReadUInt32();
		value.height = dt_reader.ReadUInt32();
	}

	// データリーダーから破線の様式を読み込む.
	void dt_read(DASH_PATT& value, DataReader const& dt_reader)
	{
		value.m_[0] = dt_reader.ReadSingle();
		value.m_[1] = dt_reader.ReadSingle();
		value.m_[2] = dt_reader.ReadSingle();
		value.m_[3] = dt_reader.ReadSingle();
		value.m_[4] = dt_reader.ReadSingle();
		value.m_[5] = dt_reader.ReadSingle();
	}

	// データリーダーから文字列範囲を読み込む.
	void dt_read(DWRITE_TEXT_RANGE& value, DataReader const& dt_reader)
	{
		value.startPosition = dt_reader.ReadUInt32();
		value.length = dt_reader.ReadUInt32();
	}

	// データリーダーから方眼の強調を読み込む.
	void dt_read(GRID_EMPH& value, DataReader const& dt_reader)
	{
		value.m_gauge_1 = dt_reader.ReadUInt16();
		value.m_gauge_2 = dt_reader.ReadUInt16();
		if (equal(value, GRID_EMPH_0) || equal(value, GRID_EMPH_2) || equal(value, GRID_EMPH_3)) {
			return;
		}
		value = GRID_EMPH_0;
	}

	// データリーダーから多角形のツールを読み込む.
	void dt_read(POLY_TOOL& value, DataReader const& dt_reader)
	{
		value.m_vertex_cnt = dt_reader.ReadUInt32();
		value.m_regular = dt_reader.ReadBoolean();
		value.m_vertex_up = dt_reader.ReadBoolean();
		value.m_end_closed = dt_reader.ReadBoolean();
		value.m_clockwise = dt_reader.ReadBoolean();
	}

	// データリーダーから位置配列を読み込む.
	void dt_read(std::vector<D2D1_POINT_2F>& value, DataReader const& dt_reader)
	{
		const size_t v_cnt = dt_reader.ReadUInt32();	// 要素数
		value.resize(v_cnt);
		for (size_t i = 0; i < v_cnt; i++) {
			dt_read(value[i], dt_reader);
		}
	}

	// データリーダーから文字列を読み込む.
	void dt_read(wchar_t*& value, DataReader const& dt_reader)
	{
		const size_t n = dt_reader.ReadUInt32();	// 文字数
		if (n > 0) {
			value = new wchar_t[n + 1];
			if (value != nullptr) {
				for (size_t i = 0; i < n; i++) {
					value[i] = dt_reader.ReadUInt16();
				}
				value[n] = L'\0';
			}
		}
		else {
			value = nullptr;
		}
	}

	// データライターに矢じるしの寸法を書き込む.
	void dt_write(const ARROW_SIZE& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_width);
		dt_writer.WriteSingle(value.m_length);
		dt_writer.WriteSingle(value.m_offset);
	}

	// データライターに端の形式を書き込む.
	void dt_write(const CAP_STYLE& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(value.m_start));
		dt_writer.WriteUInt32(static_cast<uint32_t>(value.m_end));
	}

	// データライターに色を書き込む.
	void dt_write(const D2D1_COLOR_F& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.a);
		dt_writer.WriteSingle(value.r);
		dt_writer.WriteSingle(value.g);
		dt_writer.WriteSingle(value.b);
	}

	// データライターに位置を書き込む.
	void dt_write(const D2D1_POINT_2F value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.x);
		dt_writer.WriteSingle(value.y);
	}

	// データライターに寸法を書き込む.
	void dt_write(const D2D1_SIZE_F value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.width);
		dt_writer.WriteSingle(value.height);
	}

	// データライターに寸法を書き込む.
	void dt_write(const D2D1_SIZE_U value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(value.width);
		dt_writer.WriteUInt32(value.height);
	}

	// データライターに破線の様式を書き込む.
	void dt_write(const DASH_PATT& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_[0]);
		dt_writer.WriteSingle(value.m_[1]);
		dt_writer.WriteSingle(value.m_[2]);
		dt_writer.WriteSingle(value.m_[3]);
		dt_writer.WriteSingle(value.m_[4]);
		dt_writer.WriteSingle(value.m_[5]);
	}

	// データライターに文字列範囲を書き込む.
	void dt_write(const DWRITE_TEXT_RANGE value, DataWriter const& dt_writer)
	{
		dt_writer.WriteInt32(value.startPosition);
		dt_writer.WriteInt32(value.length);
	}

	// データライターに方眼の形式を書き込む.
	void dt_write(const GRID_EMPH value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt16(value.m_gauge_1);
		dt_writer.WriteUInt16(value.m_gauge_2);
	}

	// データライターに多角形のツールを書き込む.
	void dt_write(const POLY_TOOL& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(value.m_vertex_cnt));
		dt_writer.WriteBoolean(value.m_regular);
		dt_writer.WriteBoolean(value.m_vertex_up);
		dt_writer.WriteBoolean(value.m_end_closed);
		dt_writer.WriteBoolean(value.m_clockwise);
	}

	// データライターに位置配列を書き込む.
	void dt_write(const std::vector<D2D1_POINT_2F>& value, DataWriter const& dt_writer)
	{
		const size_t n = value.size();

		dt_writer.WriteUInt32(static_cast<uint32_t>(n));
		for (uint32_t i = 0; i < n; i++) {
			dt_write(value[i], dt_writer);
		}
	}

	// データライターに文字列を書き込む
	void dt_write(const wchar_t* value, DataWriter const& dt_writer)
	{
		const uint32_t len = wchar_len(value);

		dt_writer.WriteUInt32(len);
		for (uint32_t i = 0; i < len; i++) {
			dt_writer.WriteUInt16(value[i]);
		}
	}

	// データライターに SVG として属性名とシングルバイト文字列を書き込む.
	// value	シングルバイト文字列
	// name	属性名
	void dt_write_svg(const char* value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		std::snprintf(buf, sizeof(buf), "%s=\"%s\" ", name, value);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG としてシングルバイト文字列を書き込む.
	void dt_write_svg(const char* value, DataWriter const& dt_writer)
	{
		for (uint32_t i = 0; value[i] != '\0'; i++) {
			dt_writer.WriteByte(value[i]);
		}
	}

	// データライターに SVG として属性名と色を書き込む.
	void dt_write_svg(const D2D1_COLOR_F value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		const uint32_t vr = static_cast<uint32_t>(std::round(value.r * 255.0)) & 0xff;
		const uint32_t vb = static_cast<uint32_t>(std::round(value.b * 255.0)) & 0xff;
		const uint32_t vg = static_cast<uint32_t>(std::round(value.g * 255.0)) & 0xff;
		sprintf_s(buf, "%s=\"#%02x%02x%02x\" ", name, vr, vg, vb);
		dt_write_svg(buf, dt_writer);
		if (is_opaque(value) != true) {
			std::snprintf(buf, sizeof(buf), "%s-opacity=\"%.3f\" ", name, value.a);
			dt_write_svg(buf, dt_writer);
		}
	}

	// データライターに SVG として色を書き込む.
	void dt_write_svg(const D2D1_COLOR_F value, DataWriter const& dt_writer)
	{
		char buf[8];
		const uint32_t vr = static_cast<uint32_t>(std::round(value.r * 255.0)) & 0xFF;
		const uint32_t vb = static_cast<uint32_t>(std::round(value.b * 255.0)) & 0xFF;
		const uint32_t vg = static_cast<uint32_t>(std::round(value.g * 255.0)) & 0xFF;
		sprintf_s(buf, "#%02x%02x%02x", vr, vg, vb);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として破線の形式と様式を書き込む.
	// d_style	線枠の形式
	// d_patt	破線の様式
	// s_width	線枠の太さ
	void dt_write_svg(const D2D1_DASH_STYLE d_style, const DASH_PATT& d_patt, const double s_width, DataWriter const& dt_writer)
	{
		if (s_width < FLT_MIN) {
			return;
		}
		const double a[]{
			d_patt.m_[0],
			d_patt.m_[1],
			d_patt.m_[2],
			d_patt.m_[3]
		};
		char buf[256];
		if (d_style == D2D1_DASH_STYLE_DASH) {
			sprintf_s(buf, "stroke-dasharray=\"%.0f %.0f\" ", a[0], a[1]);
		}
		else if (d_style == D2D1_DASH_STYLE_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f\" ", a[2], a[3]);
		}
		else if (d_style == D2D1_DASH_STYLE_DASH_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3]);
		}
		else if (d_style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3], a[2], a[3]);
		}
		else {
			return;
		}
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として命令と位置を書き込む.
	// value	位置
	// cmd	命令
	void dt_write_svg(const D2D1_POINT_2F value, const char* cmd, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s%f %f ", cmd, value.x, value.y);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として属性名と位置を書き込む.
	// value	位置
	// name_x	x 軸の名前
	// name_y	y 軸の名前
	void dt_write_svg(const D2D1_POINT_2F value, const char* x_name, const char* y_name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" %s=\"%f\" ", x_name, value.x, y_name, value.y);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として属性名と浮動小数値を書き込む
	// value	浮動小数値
	// a_name	属性名
	void dt_write_svg(const double value, const char* a_name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" ", a_name, value);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として浮動小数値を書き込む
	// value	浮動小数値
	void dt_write_svg(const float value, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%f ", value);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG として属性名と 32 ビット正整数を書き込む.
	// value	32 ビット正整数
	// a_name	属性名
	void dt_write_svg(const uint32_t value, const char* a_name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%u\" ", a_name, value);
		dt_write_svg(buf, dt_writer);
	}

	// データライターに SVG としてマルチバイト文字列を書き込む.
	// value	マルチバイト文字列
	// v_len	文字列の文字数
	// dt_writer	データライター
	void dt_write_svg(const wchar_t* value, const uint32_t v_len, DataWriter const& dt_writer)
	{
		if (v_len > 0) {
			const auto s_len = WideCharToMultiByte(CP_UTF8, 0, value, v_len, (char*)NULL, 0, NULL, NULL);
			auto s = new char[static_cast<size_t>(s_len) + 1];
			WideCharToMultiByte(CP_UTF8, 0, value, v_len, static_cast<LPSTR>(s), s_len, NULL, NULL);
			s[s_len] = '\0';
			dt_write_svg(s, dt_writer);
			delete[] s;
		}
	}

}