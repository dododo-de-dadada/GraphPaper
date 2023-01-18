//------------------------------
// Shape_path.cpp
// 折れ線のひな型
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataWriter;

	// 塗りつぶし色を得る.
	// val	得られた値
	// 戻り値	得られたなら true
	bool ShapePath::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// 差分だけ移動する.
	// d_vec	差分
	bool ShapePath::move(const D2D1_POINT_2F d_vec) noexcept
	{
		if (ShapeLine::move(d_vec)) {
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	// 値を, 部位の位置に格納する. 他の部位の位置は動かない. 
	// val	値
	// anc	図形の部位
	// limit	限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	bool ShapePath::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept
	{
		if (ShapeLine::set_pos_anc(val, anc, limit, keep_aspect)) {
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	// 矢じるしの形式に格納する.
	bool ShapePath::set_arrow_size(const ARROW_SIZE& val) noexcept
	{
		if (!equal(m_arrow_size, val)) {
			m_arrow_size = val;
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	// 矢じるしの形式に格納する.
	bool ShapePath::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		if (m_arrow_style != val) {
			m_arrow_style = val;
			if (m_d2d_path_geom != nullptr) {
				m_d2d_path_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// 塗りつぶしの色に格納する.
	bool ShapePath::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			m_d2d_path_geom = nullptr;
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	// val	格納する値
	bool ShapePath::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		if (ShapeLine::set_pos_start(val)) {
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	ShapePath::ShapePath(const ShapePage& page, const DataReader& dt_reader) :
		ShapeLine::ShapeLine(page, dt_reader)
	{
		const D2D1_COLOR_F fill_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		m_fill_color.r = min(max(fill_color.r, 0.0f), 1.0f);
		m_fill_color.g = min(max(fill_color.g, 0.0f), 1.0f);
		m_fill_color.b = min(max(fill_color.b, 0.0f), 1.0f);
		m_fill_color.a = min(max(fill_color.a, 0.0f), 1.0f);
	}

	// 図形をデータライターに書き込む.
	void ShapePath::write(const DataWriter& dt_writer) const
	{
		ShapeLine::write(dt_writer);
		dt_writer.WriteSingle(m_fill_color.r);
		dt_writer.WriteSingle(m_fill_color.g);
		dt_writer.WriteSingle(m_fill_color.b);
		dt_writer.WriteSingle(m_fill_color.a);
	}

}