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
		m_d2d_path_geom = nullptr;
		m_d2d_arrow_geom = nullptr;
	}

	// 矢じりの寸法を得る.
	bool ShapePath::get_arrow_size(ARROWHEAD_SIZE& value) const noexcept
	{
		value = m_arrow_size;
		return true;
	}

	// 矢じりの形式を得る.
	bool ShapePath::get_arrow_style(ARROWHEAD_STYLE& value) const noexcept
	{
		value = m_arrow_style;
		return true;
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
		// 図形の部位が頂点以外か判定する.
		const size_t d_cnt = m_diff.size();	// 差分の数
		if (anch < ANCH_TYPE::ANCH_P0 || anch > ANCH_TYPE::ANCH_P0 + d_cnt) {
			return;
		}
		// 図形の部位が始点か判定する.
		if (anch == ANCH_TYPE::ANCH_P0) {
			D2D1_POINT_2F diff;
			pt_sub(value, m_pos, diff);
			pt_round(diff, PT_ROUND, diff);
			pt_add(m_pos, diff, m_pos);
			pt_sub(m_diff[0], diff, m_diff[0]);
		}
		else {
			D2D1_POINT_2F a_pos;
			get_anch_pos(anch, a_pos);
			D2D1_POINT_2F diff;
			pt_sub(value, a_pos, diff);
			pt_round(diff, PT_ROUND, diff);
			const size_t i = anch - ANCH_TYPE::ANCH_P0;
			pt_add(m_diff[i - 1], diff, m_diff[i - 1]);
			// 図形の部位が終点以外か判定する.
			if (anch != ANCH_TYPE::ANCH_P0 + d_cnt) {
				pt_sub(m_diff[i], diff, m_diff[i]);
			}
		}
		create_path_geometry(s_d2d_factory);
	}

	// 矢じりの形式に格納する.
	void ShapePath::set_arrow_size(const ARROWHEAD_SIZE& value)
	{
		if (!equal(m_arrow_size, value)) {
			m_arrow_size = value;
			create_path_geometry(s_d2d_factory);
		}
	}

	// 矢じりの形式に格納する.
	void ShapePath::set_arrow_style(const ARROWHEAD_STYLE value)
	{
		if (m_arrow_style != value) {
			m_arrow_style = value;
			create_path_geometry(s_d2d_factory);
		}
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	// value	格納する値
	void ShapePath::set_start_pos(const D2D1_POINT_2F value)
	{
		ShapeStroke::set_start_pos(value);
		create_path_geometry(s_d2d_factory);
	}

	// 図形を作成する.
	// d_cnt	差分の数
	// s_attr	属性
	ShapePath::ShapePath(const size_t d_cnt, const ShapeSheet* s_attr) :
		ShapeStroke(d_cnt, s_attr),
		m_arrow_style(s_attr->m_arrow_style),
		m_arrow_size(s_attr->m_arrow_size)
	{}

	// 図形をデータリーダーから読み込む.
	ShapePath::ShapePath(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		m_arrow_style = static_cast<ARROWHEAD_STYLE>(dt_reader.ReadUInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
		// コンストラクタの中での (デストラクタの中でも) 仮想関数は無意味.
		//create_path_geometry(s_d2d_factory);
	}

	// データライターに書き込む.
	void ShapePath::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
	}

}