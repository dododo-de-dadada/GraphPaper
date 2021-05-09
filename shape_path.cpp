//------------------------------
// Shape_path.cpp
// 折れ線のひな型
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 図形を破棄する.
	ShapePath::~ShapePath(void)
	{
		m_poly_geom = nullptr;
	}

	// 図形を囲む領域を得る.
	// b_min	領域の左上位置.
	// b_max	領域の右下位置.
	void ShapePath::get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		const size_t n = m_diff.size();	// 差分の数
		D2D1_POINT_2F e_pos = m_pos;
		pt_inc(e_pos, b_min, b_max);
		for (size_t i = 0; i < n; i++) {
			pt_add(e_pos, m_diff[i], e_pos);
			pt_inc(e_pos, b_min, b_max);
		}
	}

	// 図形を囲む領域の左上位置を得る.
	// value	領域の左上位置
	void ShapePath::get_min_pos(D2D1_POINT_2F& value) const noexcept
	{
		const size_t n = m_diff.size();	// 差分の数
		D2D1_POINT_2F v_pos = m_pos;	// 頂点の位置
		value = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(v_pos, m_diff[i], v_pos);
			pt_min(value, v_pos, value);
		}
	}

	// 指定された部位の位置を得る.
	// anch	図形の部位
	// value	部位の位置
	void ShapePath::get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		if (anch == ANCH_WHICH::ANCH_OUTSIDE || anch == ANCH_WHICH::ANCH_P0) {
			// 図形の部位が「外部」または「開始点」ならば, 開始位置を得る.
			value = m_pos;
		}
		else if (anch > ANCH_WHICH::ANCH_P0) {
			const size_t m = m_diff.size() + 1;		// 頂点の数 (差分の数 + 1)
			if (anch < ANCH_WHICH::ANCH_P0 + m) {
				value = m_pos;
				for (size_t i = 0; i < anch - ANCH_WHICH::ANCH_P0; i++) {
					pt_add(value, m_diff[i], value);
				}
			}
		}
	}

	// 差分だけ移動する.
	// diff	差分
	void ShapePath::move(const D2D1_POINT_2F diff)
	{
		ShapeStroke::move(diff);
		create_path_geometry();
	}

	// 値を, 部位の位置に格納する. 他の部位の位置は動かない. 
	// value	格納する値
	// anch	図形の部位
	void ShapePath::set_anch_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		D2D1_POINT_2F a_pos;
		D2D1_POINT_2F diff;

		if (anch == ANCH_WHICH::ANCH_OUTSIDE) {
			m_pos = value;
		}
		else if (anch == ANCH_WHICH::ANCH_P0) {
			pt_sub(value, m_pos, diff);
			m_pos = value;
			pt_sub(m_diff[0], diff, m_diff[0]);
		}
		else {
			const size_t n = m_diff.size();	// 差分の数
			if (anch == ANCH_WHICH::ANCH_P0 + n) {
				get_anch_pos(anch, a_pos);
				pt_sub(value, a_pos, diff);
				pt_add(m_diff[n - 1], diff, m_diff[n - 1]);
			}
			else if (anch > ANCH_WHICH::ANCH_P0 && anch < ANCH_WHICH::ANCH_P0 + n) {
				get_anch_pos(anch, a_pos);
				pt_sub(value, a_pos, diff);
				const size_t i = anch - ANCH_WHICH::ANCH_P0;
				pt_add(m_diff[i - 1], diff, m_diff[i - 1]);
				pt_sub(m_diff[i], diff, m_diff[i]);
			}
			else {
				return;
			}
		}
		create_path_geometry();
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	// value	格納する値
	void ShapePath::set_start_pos(const D2D1_POINT_2F value)
	{
		ShapeStroke::set_start_pos(value);
		create_path_geometry();
	}

	// 図形を作成する.
	// n	角数
	// attr	属性値
	ShapePath::ShapePath(const uint32_t n, const ShapeSheet* attr) :
		ShapeStroke::ShapeStroke(n, attr)
	{}

	// 図形をデータリーダーから読み込む.
	ShapePath::ShapePath(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_diff[1], dt_reader);
		read(m_diff[2], dt_reader);
	}

	// データライターに書き込む.
	void ShapePath::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeStroke::write(dt_writer);
		write(m_diff[1], dt_writer);
		write(m_diff[2], dt_writer);
	}

}