//------------------------------
// shape_svg.cpp
// SVG ファイルへの出力
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	// 属性名とシングルバイト文字列をデータライターに SVG として書き込む.
	// value	シングルバイト文字列
	// name	属性名
	void svg_write(const char* value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		std::snprintf(buf, sizeof(buf), "%s=\"%s\" ", name, value);
		svg_write(buf, dt_writer);
	}

	// シングルバイト文字列をデータライターに SVG として書き込む.
	void svg_write(const char* value, DataWriter const& dt_writer)
	{
		for (uint32_t i = 0; value[i] != '\0'; i++) {
			dt_writer.WriteByte(value[i]);
		}
	}

	// 属性名と色をデータライターに SVG として書き込む.
	void svg_write(const D2D1_COLOR_F value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		const uint32_t vr = static_cast<uint32_t>(std::round(value.r * 255.0)) & 0xff;
		const uint32_t vb = static_cast<uint32_t>(std::round(value.b * 255.0)) & 0xff;
		const uint32_t vg = static_cast<uint32_t>(std::round(value.g * 255.0)) & 0xff;
		sprintf_s(buf, "%s=\"#%02x%02x%02x\" ", name, vr, vg, vb);
		svg_write(buf, dt_writer);
		if (is_opaque(value) != true) {
			std::snprintf(buf, sizeof(buf), "%s-opacity=\"%.3f\" ", name, value.a);
			svg_write(buf, dt_writer);
		}
	}

	// 色をデータライターに SVG として書き込む.
	void svg_write(const D2D1_COLOR_F value, DataWriter const& dt_writer)
	{
		char buf[8];
		const uint32_t vr = static_cast<uint32_t>(std::round(value.r * 255.0)) & 0xFF;
		const uint32_t vb = static_cast<uint32_t>(std::round(value.b * 255.0)) & 0xFF;
		const uint32_t vg = static_cast<uint32_t>(std::round(value.g * 255.0)) & 0xFF;
		sprintf_s(buf, "#%02x%02x%02x", vr, vg, vb);
		svg_write(buf, dt_writer);
	}

	// 破線の形式と配置をデータライターに SVG として書き込む.
	void svg_write(const D2D1_DASH_STYLE style, const STROKE_DASH_PATT& patt, const double width, DataWriter const& dt_writer)
	{
		if (width <= FLT_MIN) {
			return;
		}
		const double a[]{
			patt.m_[0],
			patt.m_[1],
			patt.m_[2],
			patt.m_[3]
		};
		char buf[256];
		if (style == D2D1_DASH_STYLE_DASH) {
			sprintf_s(buf, "stroke-dasharray=\"%.0f %.0f\" ", a[0], a[1]);
		}
		else if (style == D2D1_DASH_STYLE_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f\" ", a[2], a[3]);
		}
		else if (style == D2D1_DASH_STYLE_DASH_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3]);
		}
		else if (style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3], a[2], a[3]);
		}
		else {
			return;
		}
		svg_write(buf, dt_writer);
	}

	// 命令と位置をデータライターに SVG として書き込む.
	void svg_write(const D2D1_POINT_2F value, const char* cmd, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s%f %f ", cmd, value.x, value.y);
		svg_write(buf, dt_writer);
	}

	// 属性名と位置をデータライターに SVG として書き込む.
	void svg_write(const D2D1_POINT_2F value, const char* name_x, const char* name_y, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" %s=\"%f\" ", name_x, value.x, name_y, value.y);
		svg_write(buf, dt_writer);
	}

	// 属性名と浮動小数値をデータライターに SVG として書き込む
	void svg_write(const double value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" ", name, value);
		svg_write(buf, dt_writer);
	}

	// 浮動小数をデータライターに書き込む
	void svg_write(const float value, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%f ", value);
		svg_write(buf, dt_writer);
	}

	// 属性名と 32 ビット正整数をデータライターに SVG として書き込む
	void svg_write(const uint32_t value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%u\" ", name, value);
		svg_write(buf, dt_writer);
	}

	// マルチバイト文字列をデータライターに SVG として書き込む.
	// val	マルチバイト文字列
	// v_len	文字列の文字数
	// dt_writer	データライター
	void svg_write(const wchar_t* value, const uint32_t v_len, DataWriter const& dt_writer)
	{
		if (v_len > 0) {
			const auto s_len = WideCharToMultiByte(CP_UTF8, 0, value, v_len, (char*)NULL, 0, NULL, NULL);
			auto s = new char[static_cast<size_t>(s_len) + 1];
			WideCharToMultiByte(CP_UTF8, 0, value, v_len, static_cast<LPSTR>(s), s_len, NULL, NULL);
			s[s_len] = '\0';
			svg_write(s, dt_writer);
			delete[] s;
		}
	}

}