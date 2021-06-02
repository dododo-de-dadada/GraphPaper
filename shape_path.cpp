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
	// diff	差分
	bool ShapePath::move(const D2D1_POINT_2F diff)
	{
		if (ShapeStroke::move(diff)) {
			create_path_geometry(s_d2d_factory);
			return true;
		}
		return false;
	}

	// 値を, 部位の位置に格納する. 他の部位の位置は動かない. 
	// value	格納する値
	// anch	図形の部位
	bool ShapePath::set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		if (ShapeStroke::set_anchor_pos(value, anch)) {
			create_path_geometry(s_d2d_factory);
			return true;
		}
		return false;
	}

	// 矢じりの形式に格納する.
	bool ShapePath::set_arrow_size(const ARROWHEAD_SIZE& value)
	{
		if (!equal(m_arrow_size, value)) {
			m_arrow_size = value;
			create_path_geometry(s_d2d_factory);
			return true;
		}
		return false;
	}

	// 矢じりの形式に格納する.
	bool ShapePath::set_arrow_style(const ARROWHEAD_STYLE value)
	{
		if (m_arrow_style != value) {
			m_arrow_style = value;
			create_path_geometry(s_d2d_factory);
			return true;
		}
		return false;
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	// value	格納する値
	bool ShapePath::set_start_pos(const D2D1_POINT_2F value)
	{
		if (ShapeStroke::set_start_pos(value)) {
			create_path_geometry(s_d2d_factory);
			return true;
		}
		return false;
	}

	// 図形を作成する.
	// d_cnt	差分の数
	// s_attr	属性
	ShapePath::ShapePath(const size_t d_cnt, const ShapeSheet* s_attr, const bool closed) :
		ShapeLine::ShapeLine(d_cnt, s_attr, closed)
	{}

	// 図形をデータリーダーから読み込む.
	ShapePath::ShapePath(DataReader const& dt_reader) :
		ShapeLine::ShapeLine(dt_reader)
	{
		//m_arrow_style = static_cast<ARROWHEAD_STYLE>(dt_reader.ReadUInt32());
		//read(m_arrow_size, dt_reader);
		// コンストラクタの中での (デストラクタの中でも) 仮想関数は無意味.
		//create_path_geometry(s_d2d_factory);
	}

	// データライターに書き込む.
	void ShapePath::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeLine::write(dt_writer);
		//dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
		//write(m_arrow_size, dt_writer);
	}

}