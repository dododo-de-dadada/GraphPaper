//------------------------------
// Shape_path.cpp
// 折れ線のひな型
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 近傍の頂点を見つける.
	// pos	ある位置
	// dd	近傍とみなす距離 (の二乗値), これより離れた頂点は近傍とはみなさない.
	// val	ある位置の近傍にある頂点
	// 戻り値	見つかったら true
	bool ShapePath::get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool done = false;
		D2D1_POINT_2F r;
		pt_sub(m_start, p, r);
		float r_abs = static_cast<float>(pt_abs2(r));
		if (r_abs < dd) {
			dd = r_abs;
			val = m_start;
			if (!done) {
				done = true;
			}
		}
		D2D1_POINT_2F q{ m_start };	// 次の点
		for (const D2D1_POINT_2F pos : m_pos) {
			pt_add(q, pos, q);
			pt_sub(q, p, r);
			r_abs = static_cast<float>(pt_abs2(r));
			if (r_abs < dd) {
				dd = r_abs;
				val = q;
				if (!done) {
					done = true;
				}
			}
		}
		return done;
	}

	// 値を, 指定した部位の点に格納する.
	bool ShapePath::set_pos_loc(
		const D2D1_POINT_2F val,	// 値
		const uint32_t loc,	// 部位
		const float snap_point,	// 
		const bool /*keep_aspect*/
	) noexcept
	{
		bool flag = false;
		// 変更する頂点がどの頂点か判定する.
		const size_t d_cnt = m_pos.size();	// 差分の数
		if (loc >= LOC_TYPE::LOC_P0 && loc <= LOC_TYPE::LOC_P0 + d_cnt) {
			D2D1_POINT_2F p[N_GON_MAX];	// 頂点の位置
			const size_t l_cnt = loc - LOC_TYPE::LOC_P0;	// 変更する点の添え字
			// 変更する頂点までの, 各頂点の位置を得る.
			p[0] = m_start;
			for (size_t i = 0; i < l_cnt; i++) {
				pt_add(p[i], m_pos[i], p[i + 1]);
			}
			// 値から変更前の位置を引き, 変更する差分を得る.
			D2D1_POINT_2F d;
			pt_sub(val, p[l_cnt], d);
			pt_round(d, PT_ROUND, d);
			// 差分の長さがゼロより大きいか判定する.
			if (pt_abs2(d) >= FLT_MIN) {
				// 変更する頂点が最初の頂点か判定する.
				if (l_cnt == 0) {
					// 最初の頂点の位置に変更分を加える.
					pt_add(m_start, d, m_start);
				}
				else {
					// 頂点の直前の差分に変更分を加える.
					pt_add(m_pos[l_cnt - 1], d, m_pos[l_cnt - 1]);
				}
				// 変更するのが最後の頂点以外か判定する.
				if (l_cnt < d_cnt) {
					// 次の頂点が動かないように,
					// 変更する頂点の次の頂点への差分から変更分を引く.
					pt_sub(m_pos[l_cnt], d, m_pos[l_cnt]);
				}
				if (!flag) {
					flag = true;
				}
			}
			// 限界距離がゼロでないか判定する.
			if (snap_point >= FLT_MIN) {
				// 残りの頂点の位置を得る.
				for (size_t i = l_cnt; i < d_cnt; i++) {
					pt_add(p[i], m_pos[i], p[i + 1]);
				}
				const double dd = static_cast<double>(snap_point) * static_cast<double>(snap_point);
				for (size_t i = 0; i < d_cnt + 1; i++) {
					// 頂点が, 変更する頂点か判定する.
					if (i == l_cnt) {
						continue;
					}
					// 頂点と変更する頂点との距離が限界距離以上か判定する.
					pt_sub(p[i], p[l_cnt], d);
					if (pt_abs2(d) >= dd) {
						continue;
					}
					// 変更するのが最初の頂点か判定する.
					if (l_cnt == 0) {
						pt_add(m_start, d, m_start);
					}
					else {
						pt_add(m_pos[l_cnt - 1], d, m_pos[l_cnt - 1]);
					}
					// 変更するのが最後の頂点以外か判定する.
					if (l_cnt < d_cnt) {
						pt_sub(m_pos[l_cnt], d, m_pos[l_cnt]);
					}
					if (!flag) {
						flag = true;
					}
					break;
				}
			}
		}
		if (flag) {
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
		}
		return flag;
	}

	// 頂点を得る.
	size_t ShapePath::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		const size_t p_cnt = m_pos.size();
		p[0] = m_start;
		for (size_t i = 0; i < p_cnt; i++) {
			p[i + 1].x = p[i].x + m_pos[i].x;
			p[i + 1].y = p[i].y + m_pos[i].y;
		}
		return p_cnt + 1;
	}

	// 境界矩形を得る.
	void ShapePath::get_bbox(
		const D2D1_POINT_2F a_lt,	// a_lt	元の領域の左上位置.
		const D2D1_POINT_2F a_rb,	// a_rb	元の領域の右下位置.
		D2D1_POINT_2F& b_lt,	// b_lt	囲む領域の左上位置.
		D2D1_POINT_2F& b_rb	// b_rb	囲む領域の右下位置.
	) const noexcept
	{
		b_lt.x = m_start.x < a_lt.x ? m_start.x : a_lt.x;
		b_lt.y = m_start.y < a_lt.y ? m_start.y : a_lt.y;
		b_rb.x = m_start.x > a_rb.x ? m_start.x : a_rb.x;
		b_rb.y = m_start.y > a_rb.y ? m_start.y : a_rb.y;
		const size_t d_cnt = m_pos.size();	// 差分の数
		D2D1_POINT_2F pos = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(pos, m_pos[i], pos);
			if (pos.x < b_lt.x) {
				b_lt.x = pos.x;
			}
			if (pos.x > b_rb.x) {
				b_rb.x = pos.x;
			}
			if (pos.y < b_lt.y) {
				b_lt.y = pos.y;
			}
			if (pos.y > b_rb.y) {
				b_rb.y = pos.y;
			}
		}
	}

	// 境界矩形の左上位置を得る.
	void ShapePath::get_bbox_lt(D2D1_POINT_2F& val) const noexcept
	{
		const size_t p_cnt = m_pos.size();	// 位置の数
		D2D1_POINT_2F p = m_start;	// 頂点
		D2D1_POINT_2F lt;	// 左上位置
		lt = m_start;
		for (size_t i = 0; i < p_cnt; i++) {
			pt_add(p, m_pos[i], p);
			if (lt.x > p.x) {
				lt.x = p.x;
			}
			if (lt.y > p.y) {
				lt.y = p.y;
			}
		}
		val = lt;
	}


	// 指定した部位の点を得る.
	void ShapePath::get_pos_loc(
		const uint32_t loc,	// 部位
		D2D1_POINT_2F& val	// 得られた値
	) const noexcept
	{
		// 図形の部位が「図形の外部」または「始点」ならば, 始点を得る.
		if (loc == LOC_TYPE::LOC_PAGE || loc == LOC_TYPE::LOC_P0) {
			val = m_start;
		}
		else if (loc > LOC_TYPE::LOC_P0) {
			const size_t  l_cnt = loc - LOC_TYPE::LOC_P0;
			if (l_cnt < m_pos.size() + 1) {
				val = m_start;
				for (size_t i = 0; i < l_cnt; i++) {
					pt_add(val, m_pos[i], val);
				}
			}
		}
	}

	// 塗りつぶし色を得る.
	// val	得られた値
	// 戻り値	得られたなら true
	bool ShapePath::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// 位置を移動する.
	// pos	位置ベクトル
	bool ShapePath::move(const D2D1_POINT_2F pos) noexcept
	{
		D2D1_POINT_2F start;
		pt_add(m_start, pos, start);
		if (set_pos_start(start)) {
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	// 塗りつぶし色に格納する.
	bool ShapePath::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			return true;
		}
		return false;
	}

	// 始点に値を格納する. 他の部位の位置も動く.
	// val	格納する値
	bool ShapePath::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F p;
		pt_round(val, PT_ROUND, p);
		if (!equal(m_start, p)) {
			m_start = p;
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	ShapePath::ShapePath(const DataReader& dt_reader) :
		ShapeArrow(dt_reader)
	{
		m_start.x = dt_reader.ReadSingle();
		m_start.y = dt_reader.ReadSingle();
		const size_t vec_cnt = dt_reader.ReadUInt32();	// 要素数
		m_pos.resize(vec_cnt);
		for (size_t i = 0; i < vec_cnt; i++) {
			m_pos[i].x = dt_reader.ReadSingle();
			m_pos[i].y = dt_reader.ReadSingle();
		}
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
		ShapeArrow::write(dt_writer);
		// 始点
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);
		// 次の点への位置ベクトル
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_pos.size()));
		for (const D2D1_POINT_2F vec : m_pos) {
			dt_writer.WriteSingle(vec.x);
			dt_writer.WriteSingle(vec.y);
		}
		// 塗りつぶし色
		dt_writer.WriteSingle(m_fill_color.r);
		dt_writer.WriteSingle(m_fill_color.g);
		dt_writer.WriteSingle(m_fill_color.b);
		dt_writer.WriteSingle(m_fill_color.a);
	}

}