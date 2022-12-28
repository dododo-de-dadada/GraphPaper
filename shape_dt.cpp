//------------------------------
// shape_dt.cpp
// 読み込み, 書き込み.
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	/*
	// データリーダーから矢じるしの寸法を読み込む.
	void dt_read(ARROW_SIZE& val, DataReader const& dt_reader)
	{
		val.m_width = dt_reader.ReadSingle();
		val.m_length = dt_reader.ReadSingle();
		val.m_offset = dt_reader.ReadSingle();
	}

	// データリーダーから矢じるしの形式を読み込む.
	void dt_read(CAP_STYLE& val, DataReader const& dt_reader)
	{
		const auto start = dt_reader.ReadUInt32();
		if (start != D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT &&
			start != D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND &&
			start != D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE &&
			start != D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
			val.m_start = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;
		}
		else {
			val.m_start = static_cast<D2D1_CAP_STYLE>(start);
		}
		const auto end = dt_reader.ReadUInt32();
		if (end != D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT &&
			end != D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND &&
			end != D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE &&
			end != D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
			val.m_end = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;
		}
		else {
			val.m_end = static_cast<D2D1_CAP_STYLE>(end);
		}
	}

	// データリーダーから色を読み込む.
	void dt_read(D2D1_COLOR_F& val, DataReader const& dt_reader)
	{
		const auto r = dt_reader.ReadSingle();
		const auto g = dt_reader.ReadSingle();
		const auto b = dt_reader.ReadSingle();
		const auto a = dt_reader.ReadSingle();
		val.r = min(max(r, 0.0F), 1.0F);
		val.g = min(max(g, 0.0F), 1.0F);
		val.b = min(max(b, 0.0F), 1.0F);
		val.a = min(max(a, 0.0F), 1.0F);
	}

	// データリーダーから位置を読み込む.
	void dt_read(D2D1_POINT_2F& val, DataReader const& dt_reader)
	{
		val.x = dt_reader.ReadSingle();
		val.y = dt_reader.ReadSingle();
	}

	// データリーダーから寸法を読み込む.
	void dt_read(D2D1_RECT_F& val, DataReader const& dt_reader)
	{
		val.left = dt_reader.ReadSingle();
		val.top = dt_reader.ReadSingle();
		val.right = dt_reader.ReadSingle();
		val.bottom = dt_reader.ReadSingle();
	}

	// データリーダーから寸法を読み込む.
	void dt_read(D2D1_SIZE_F& val, DataReader const& dt_reader)
	{
		val.width = dt_reader.ReadSingle();
		val.height = dt_reader.ReadSingle();
	}

	// データリーダーから寸法を読み込む.
	void dt_read(D2D1_SIZE_U& val, DataReader const& dt_reader)
	{
		val.width = dt_reader.ReadUInt32();
		val.height = dt_reader.ReadUInt32();
	}

	// データリーダーから破線の配置を読み込む.
	void dt_read(DASH_PATT& val, DataReader const& dt_reader)
	{
		val.m_[0] = dt_reader.ReadSingle();
		val.m_[1] = dt_reader.ReadSingle();
		val.m_[2] = dt_reader.ReadSingle();
		val.m_[3] = dt_reader.ReadSingle();
		val.m_[4] = dt_reader.ReadSingle();
		val.m_[5] = dt_reader.ReadSingle();
	}

	// データリーダーから文字列範囲を読み込む.
	void dt_read(DWRITE_TEXT_RANGE& val, DataReader const& dt_reader)
	{
		val.startPosition = dt_reader.ReadUInt32();
		val.length = dt_reader.ReadUInt32();
	}

	// データリーダーから方眼の強調を読み込む.
	void dt_read(GRID_EMPH& val, DataReader const& dt_reader)
	{
		val.m_gauge_1 = dt_reader.ReadUInt16();
		val.m_gauge_2 = dt_reader.ReadUInt16();
		if (equal(val, GRID_EMPH_0) || 
			equal(val, GRID_EMPH_2) || 
			equal(val, GRID_EMPH_3)) {
			return;
		}
		val = GRID_EMPH_0;
	}

	// データリーダーから位置配列を読み込む.
	void dt_read(std::vector<D2D1_POINT_2F>& val, DataReader const& dt_reader)
	{
		const size_t v_cnt = dt_reader.ReadUInt32();	// 要素数
		val.resize(v_cnt);
		for (size_t i = 0; i < v_cnt; i++) {
			val[i].x = dt_reader.ReadSingle();
			val[i].y = dt_reader.ReadSingle();
		}
	}

	// データリーダーから文字列を読み込む.
	void dt_read(wchar_t*& val, DataReader const& dt_reader)
	{
		const size_t len = dt_reader.ReadUInt32();	// 文字数
		uint8_t* data = new uint8_t[2 * (len + 1)];
		dt_reader.ReadBytes(array_view(data, data + 2 * len));
		//for (size_t i = 0; i < len; i++) {
		//	std::swap(data[2 * i], data[2 * i + 1]);
		//}
		val = reinterpret_cast<wchar_t*>(data);
		val[len] = L'\0';
	}
	*/

	// データライターに矢じるしの寸法を書き込む.
	void dt_write(const ARROW_SIZE& val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.m_width);
		dt_writer.WriteSingle(val.m_length);
		dt_writer.WriteSingle(val.m_offset);
	}

	// データライターに端の形式を書き込む.
	void dt_write(const CAP_STYLE& val, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(val.m_start));
		dt_writer.WriteUInt32(static_cast<uint32_t>(val.m_end));
	}

	// データライターに色を書き込む.
	void dt_write(const D2D1_COLOR_F& val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.r);
		dt_writer.WriteSingle(val.g);
		dt_writer.WriteSingle(val.b);
		dt_writer.WriteSingle(val.a);
	}

	// データライターに位置を書き込む.
	void dt_write(const D2D1_POINT_2F val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.x);
		dt_writer.WriteSingle(val.y);
	}

	// データライターに方形を書き込む.
	void dt_write(const D2D1_RECT_F val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.left);
		dt_writer.WriteSingle(val.top);
		dt_writer.WriteSingle(val.right);
		dt_writer.WriteSingle(val.bottom);
	}

	// データライターに寸法を書き込む.
	void dt_write(const D2D1_SIZE_F val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.width);
		dt_writer.WriteSingle(val.height);
	}

	// データライターに寸法を書き込む.
	void dt_write(const D2D1_SIZE_U val, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(val.width);
		dt_writer.WriteUInt32(val.height);
	}

	// データライターに破線の配置を書き込む.
	void dt_write(const DASH_PATT& val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.m_[0]);
		dt_writer.WriteSingle(val.m_[1]);
		dt_writer.WriteSingle(val.m_[2]);
		dt_writer.WriteSingle(val.m_[3]);
		dt_writer.WriteSingle(val.m_[4]);
		dt_writer.WriteSingle(val.m_[5]);
	}

	// データライターに文字列範囲を書き込む.
	void dt_write(const DWRITE_TEXT_RANGE val, DataWriter const& dt_writer)
	{
		dt_writer.WriteInt32(val.startPosition);
		dt_writer.WriteInt32(val.length);
	}

	// データライターに方眼の形式を書き込む.
	void dt_write(const GRID_EMPH val, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(val.m_gauge_1);
		dt_writer.WriteUInt32(val.m_gauge_2);
	}

	// データライターに位置配列を書き込む.
	void dt_write(const std::vector<D2D1_POINT_2F>& val, DataWriter const& dt_writer)
	{
		const size_t n = val.size();

		dt_writer.WriteUInt32(static_cast<uint32_t>(n));
		for (uint32_t i = 0; i < n; i++) {
			dt_writer.WriteSingle(val[i].x);
			dt_writer.WriteSingle(val[i].y);
		}
	}

	// データライターに文字列を書き込む
	void dt_write(const wchar_t* val, DataWriter const& dt_writer)
	{
		const uint32_t len = wchar_len(val);

		dt_writer.WriteUInt32(len);
		const auto data = reinterpret_cast<const uint8_t*>(val);
		dt_writer.WriteBytes(array_view(data, data + 2 * len));
	}

	// データライターに文字列を書き込む
	// 戻り値	書き込んだバイト数.
	size_t dt_write(const char val[], DataWriter const& dt_writer)
	{
		size_t i = 0;
		for (; val[i] != '\0'; i++) {
			dt_writer.WriteByte(val[i]);
		}
		return i;
	}

}