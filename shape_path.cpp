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
		m_path_geom = nullptr;
	}

	// 位置を, 折れ線の図形の部位が含むか判定する.
	// t_pos	判定する位置
	// a_len	部位の大きさ
	// p_cnt	折れ線の頂点の数
	// p_pos	折れ線の頂点
	// 戻り値	位置を含む図形の部位
	/*
	uint32_t ShapePath::hit_test_anchor(const D2D1_POINT_2F t_pos, const double a_len, const size_t p_cnt, D2D1_POINT_2F p_pos[], size_t& k) const noexcept
	{
		//const size_t m = m_diff.size() + 1;	// 頂点の数 (差分の数 + 1)
		size_t j = static_cast<size_t>(-1);	// 点を含む頂点の添え字

		// 判定する位置が原点となるよう平行移動した四へん形の各頂点を得る.
		k = 0;
		pt_sub(m_pos, t_pos, p_pos[k++]);
		if (pt_in_anch(p_pos[0], a_len)) {
			j = 0;
		}
		for (size_t i = 1; i < p_cnt; i++) {
			pt_add(p_pos[k - 1], m_diff[i - 1], p_pos[k]);
			if (pt_in_anch(p_pos[i], a_len)) {
				j = i;
			}
			if (pt_abs2(m_diff[i - 1]) > FLT_MIN) {
				k++;
			}
		}
		if (j != -1) {
			const auto anch = ANCH_TYPE::ANCH_P0 + j;
			return static_cast<uint32_t>(anch);
		}
		return ANCH_TYPE::ANCH_SHEET;
	}
	*/

	// 差分だけ移動する.
	// diff	差分
	void ShapePath::move(const D2D1_POINT_2F diff)
	{
		ShapeStroke::move(diff);
		create_path_geometry(s_d2d_factory);
	}

	// 値を, 部位の位置に格納する. 他の部位の位置は動かない. 
	// value	格納する値
	// anch	図形の部位
	void ShapePath::set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		D2D1_POINT_2F a_pos;
		D2D1_POINT_2F diff;

		if (anch == ANCH_TYPE::ANCH_SHEET) {
			m_pos = value;
		}
		else if (anch == ANCH_TYPE::ANCH_P0) {
			pt_sub(value, m_pos, diff);
			m_pos = value;
			pt_sub(m_diff[0], diff, m_diff[0]);
		}
		else {
			const size_t diff_cnt = m_diff.size();	// 差分の数
			if (anch == ANCH_TYPE::ANCH_P0 + diff_cnt) {
				get_anch_pos(anch, a_pos);
				pt_sub(value, a_pos, diff);
				pt_add(m_diff[diff_cnt - 1], diff, m_diff[diff_cnt - 1]);
			}
			else if (anch > ANCH_TYPE::ANCH_P0 && anch < ANCH_TYPE::ANCH_P0 + diff_cnt) {
				get_anch_pos(anch, a_pos);
				pt_sub(value, a_pos, diff);
				const size_t i = anch - ANCH_TYPE::ANCH_P0;
				pt_add(m_diff[i - 1], diff, m_diff[i - 1]);
				pt_sub(m_diff[i], diff, m_diff[i]);
			}
			else {
				return;
			}
		}
		create_path_geometry(s_d2d_factory);
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	// value	格納する値
	void ShapePath::set_start_pos(const D2D1_POINT_2F value)
	{
		ShapeStroke::set_start_pos(value);
		create_path_geometry(s_d2d_factory);
	}

}