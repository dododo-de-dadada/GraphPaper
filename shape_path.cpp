//------------------------------
// Shape_path.cpp
// 折れ線のひな型
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 差分だけ移動する.
	// d_vec	差分
	bool ShapePath::move(const D2D1_POINT_2F d_vec)
	{
		if (ShapeStroke::move(d_vec)) {
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			//create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// 値を, 部位の位置に格納する. 他の部位の位置は動かない. 
	// value	値
	// anch	図形の部位
	// limit	限界距離 (他の頂点との距離がこの値未満になるなら, その頂点に位置に合わせる)
	bool ShapePath::set_pos_anch(const D2D1_POINT_2F value, const uint32_t anch, const float limit, const bool keep_aspect) noexcept
	{
		if (ShapeStroke::set_pos_anch(value, anch, limit, keep_aspect)) {
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			//create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// 矢じるしの形式に格納する.
	bool ShapePath::set_arrow_size(const ARROW_SIZE& value)
	{
		if (!equal(m_arrow_size, value)) {
			m_arrow_size = value;
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			//create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// 矢じるしの形式に格納する.
	bool ShapePath::set_arrow_style(const ARROW_STYLE value)
	{
		if (m_arrow_style != value) {
			m_arrow_style = value;
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			//create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	// value	格納する値
	bool ShapePath::set_pos_start(const D2D1_POINT_2F value)
	{
		if (ShapeStroke::set_pos_start(value)) {
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			//create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// データライターに書き込む.
	void ShapePath::write(DataWriter const& dt_writer) const
	{
		ShapeLineA::write(dt_writer);
	}

}