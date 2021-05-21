//------------------------------
// Shape_poly.cpp
// 多角形
//------------------------------
#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 多角形の辺の法線ベクトルを得る.
	static bool poly_get_nvec(const size_t m, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept;

	// 直行するベクトルを得る.
	static D2D1_POINT_2F poly_pt_orth(const D2D1_POINT_2F vec) { return { -vec.y, vec.x }; }

	// 多角形の辺が位置を含むか判定する.
	static bool poly_test_side(const D2D1_POINT_2F t_pos, const size_t v_cnt, const bool v_end, const D2D1_POINT_2F v_pos[], const D2D1_POINT_2F n_vec[], const double s_width, D2D1_POINT_2F exp_side[]) noexcept;

	// 多角形の角が位置を含むか判定する.
	static bool poly_test_corner(const D2D1_POINT_2F t_pos, const size_t exp_cnt, const bool exp_end, const D2D1_POINT_2F exp_side[], const D2D1_POINT_2F n_vec[], const double ext_len) noexcept;

	//	多角形の各辺の法線ベクトルを得る.
	//	v_cnt	頂点の数
	//	v_pos	頂点の配列 [v_cnt]
	//	n_vec	各辺の法線ベクトルの配列 [v_cnt]
	//	戻り値	法線ベクトルを得たなら true, すべての頂点が重なっていたなら false
	static bool poly_get_nvec(const size_t v_cnt, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept
	{
		// 多角形の各辺の長さと法線ベクトル, 
		// 重複しない頂点の数を求める.
		std::vector<double> s_len(v_cnt);	// 各辺の長さ
		int q_cnt = 1;
		for (size_t i = 0; i < v_cnt; i++) {
			// 次の頂点との差分を求める.
			D2D1_POINT_2F q_sub;
			pt_sub(v_pos[(i + 1) % v_cnt], v_pos[i], q_sub);
			// 差分の長さを求める.
			s_len[i] = sqrt(pt_abs2(q_sub));
			if (s_len[i] > FLT_MIN) {
				// 差分の長さが 0 より大きいなら, 
				// 重複しない頂点の数をインクリメントする.
				q_cnt++;
			}
			// 差分と直行するベクトルを正規化して法線ベクトルに格納する.
			pt_mul(poly_pt_orth(q_sub), 1.0 / s_len[i], n_vec[i]);
		}
		if (q_cnt == 1) {
			// すべての頂点が重なったなら, false を返す.
			return false;
		}
		for (size_t i = 0; i < v_cnt; i++) {
			if (s_len[i] <= FLT_MIN) {
				// 辺の長さが 0 ならば, 隣接する前後の辺の中から
				// 長さが 0 でない辺を探し, それらの法線ベクトルを合成し, 
				// 長さ 0 の辺の法線ベクトルとする.
				size_t prev;
				for (size_t j = 1; s_len[prev = ((i - j) % v_cnt)] < FLT_MIN; j++);
				size_t next;
				for (size_t j = 1; s_len[next = ((i + j) % v_cnt)] < FLT_MIN; j++);
				pt_add(n_vec[prev], n_vec[next], n_vec[i]);
				auto len = sqrt(pt_abs2(n_vec[i]));
				if (len > FLT_MIN) {
					pt_mul(n_vec[i], 1.0 / len, n_vec[i]);
				}
				else {
					// 合成ベクトルがゼロベクトルになるなら,
					// 前方の隣接する辺の法線ベクトルに直交するベクトルを法線ベクトルとする.
					n_vec[i] = poly_pt_orth(n_vec[prev]);
				}
			}
		}
		return true;
	}

	// 多角形の辺が位置を含むか判定する.
	// t_pos	判定する位置
	// v_cnt	頂点の数
	// v_end	辺が閉じているか判定
	// v_pos	頂点の配列 [v_cnt]
	// n_vec	各辺の法線ベクトルの配列 [v_cnt]
	// s_width	辺の太さ
	// exp_side	拡張した辺の配列 [v_cnt × 4]
	static bool poly_test_side(const D2D1_POINT_2F t_pos, const size_t v_cnt, const bool v_end, const D2D1_POINT_2F v_pos[], const D2D1_POINT_2F n_vec[], const double s_width, D2D1_POINT_2F exp_side[]) noexcept
	{
		const auto cnt = (v_end ? v_cnt : v_cnt - 1);
		for (size_t i = 0; i < cnt; i++) {
			// もとの辺の片方の端点を, 法線ベクトルにそって正逆の両方向に移動し, 得られた位置を拡張した辺に格納する.
			D2D1_POINT_2F nor;
			pt_mul(n_vec[i], s_width, nor);
			const auto j = i * 4;
			pt_add(v_pos[i], nor, exp_side[j + 0]);
			pt_sub(v_pos[i], nor, exp_side[j + 1]);
			// もう一方の端点も, 同じようにして拡張した辺を完成させる.
			const auto k = (i + 1) % v_cnt;
			pt_sub(v_pos[k], nor, exp_side[j + 2]);
			pt_add(v_pos[k], nor, exp_side[j + 3]);
			// 位置が拡張した辺に含まれるか判定する.
			if (pt_in_poly(t_pos, 4, exp_side + j)) {
				// 含まれるなら true を返す.
				return true;
			}
		}
		return false;
	}

	// 多角形の角が位置を含むか判定する.
	// t_pos	判定する位置
	// exp_cnt	拡張した辺の数
	// exp_end	拡張した辺が閉じているか判定
	// exp_side	拡張した辺の配列 [exp_cnt × 4]
	// n_vec	各辺の法線ベクトルの配列 [exp_cnt]
	// ext_len	角を超えて延長する長さ
	static bool poly_test_corner(const D2D1_POINT_2F t_pos, const size_t exp_cnt, const bool exp_end, const D2D1_POINT_2F exp_side[], const D2D1_POINT_2F n_vec[], const double ext_len) noexcept
	{
		D2D1_POINT_2F ext_side[4];	// 拡張した辺をさらに延長した辺
		D2D1_POINT_2F ext_vec;	// 拡張した辺に平行なベクトル

		for (size_t i = (exp_end ? 0 : 1), j = (exp_end ? exp_cnt - 1 : 0); i < exp_cnt; j = i++) {
			// ある頂点に隣接する辺について.
			// 拡張した辺の一方の端を, 延長した辺に格納する.
			ext_side[0] = exp_side[4 * j + 3];
			ext_side[1] = exp_side[4 * j + 2];
			// 法線ベクトルと直行するベクトルを,
			// 延長する長さの分だけ倍し,
			// 平行なベクトルに格納する.
			pt_mul(poly_pt_orth(n_vec[j]), ext_len, ext_vec);
			// 格納した位置を平行なベクトルに沿って延長し,
			// 延長した辺に格納する.
			pt_sub(ext_side[1], ext_vec, ext_side[2]);
			pt_sub(ext_side[0], ext_vec, ext_side[3]);
			// 位置が延長した辺に含まれるか判定する.
			if (pt_in_poly(t_pos, 4, ext_side) != true) {
				// 含まれないなら継続する.
				continue;
			}
			// 隣接するもう片方の辺について.
			// 拡張した辺の端を, 延長した辺の端に格納する.
			ext_side[2] = exp_side[4 * i + 1];
			ext_side[3] = exp_side[4 * i + 0];
			// 法線ベクトルと直行するベクトル (先ほどとは逆方向) を得て,
			// 角を延長する長さの分だけ倍し,
			// 平行なベクトルに格納する.
			pt_mul(poly_pt_orth(n_vec[i]), ext_len, ext_vec);
			// 格納した位置を平行なベクトルに沿って延長し,
			// 延長した辺のもう一方の端に格納する.
			pt_add(ext_side[3], ext_vec, ext_side[0]);
			pt_add(ext_side[2], ext_vec, ext_side[1]);
			// 位置が延長した辺に含まれるか判定する.
			if (pt_in_poly(t_pos, 4, ext_side)) {
				// 含まれるなら true を返す.
				return true;
			}
		}
		return false;
	}

	// 領域をもとに多角形を作成する.
	// b_pos	領域の開始位置
	// b_diff	領域の終了位置への差分
	// v_cnt	多角形の頂点の数
	// v_up	頂点を上に作成するか判定
	// v_reg	正多角形を作成するか判定
	// v_clock	時計周りで作成するか判定
	// v_pos	頂点の配列 [v_cnt]
	void ShapePoly::create_poly_by_bbox(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const size_t v_cnt, const bool v_up, const bool v_reg, const bool v_clock, D2D1_POINT_2F v_pos[]) noexcept
	{
		if (v_cnt == 0) {
			return;
		}
		// 原点を中心とする正円をもとに, 多角形の頂点と, その大きさを求める.
		D2D1_POINT_2F v_min{ 0.0, 0.0 };
		D2D1_POINT_2F v_max{ 0.0, 0.0 };
		D2D1_POINT_2F v_diff;
		const double s = v_up ? (M_PI / 2.0) : (M_PI / 2.0 + M_PI / v_cnt);
		for (uint32_t i = 0; i < v_cnt; i++) {
			const double t = 2.0 * M_PI / v_cnt * i;
			const double r = v_clock ? s - t : s + t;
			v_pos[i].x = static_cast<FLOAT>(cos(r));
			v_pos[i].y = static_cast<FLOAT>(-sin(r));
			pt_inc(v_pos[i], v_min, v_max);
		}
		pt_sub(v_max, v_min, v_diff);

		if (pt_abs2(v_diff) > FLT_MIN) {
			const double scale_x = v_reg ? fmin(b_diff.x, b_diff.y) / fmax(v_diff.x, v_diff.y) : b_diff.x / v_diff.x;
			const double scale_y = v_reg ? fmin(b_diff.x, b_diff.y) / fmax(v_diff.x, v_diff.y) : b_diff.y / v_diff.y;
			for (uint32_t i = 0; i < v_cnt; i++) {
				pt_sub(v_pos[i], v_min, v_pos[i]);
				v_pos[i].x = static_cast<FLOAT>(v_pos[i].x * scale_x);
				v_pos[i].y = static_cast<FLOAT>(v_pos[i].y * scale_y);
				pt_add(v_pos[i], b_pos, v_pos[i]);
			}
		}
	}

	// パスジオメトリを作成する.
	void ShapePoly::create_path_geometry(ID2D1Factory3* const d_factory)
	{
		const size_t v_cnt = m_diff.size() + 1;	// 頂点の数 (差分の数 + 1)
		std::vector<D2D1_POINT_2F> v_pos(v_cnt);	// 頂点の配列

		m_path_geom = nullptr;
		v_pos[0] = m_pos;
		for (size_t i = 1; i < v_cnt; i++) {
			pt_add(v_pos[i - 1], m_diff[i - 1], v_pos[i]);
		}
		winrt::com_ptr<ID2D1GeometrySink> sink;
		winrt::check_hresult(d_factory->CreatePathGeometry(m_path_geom.put()));
		m_path_geom->Open(sink.put());
		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		const auto figure_begin = is_opaque(m_fill_color) ? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
		sink->BeginFigure(v_pos[0], figure_begin);
		for (size_t i = 1; i < v_cnt; i++) {
			sink->AddLine(v_pos[i]);
		}
		// Shape 上で始点と終点を重ねたとき,
		// パスに始点を加えないと, LINE_JOINT がへんなことになる.
		//sink->AddLine(v_pos[0]);
		sink->EndFigure(m_end_closed ? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
		sink->Close();
		sink = nullptr;
	}

	// 図形を表示する.
	// dx	図形の描画環境
	void ShapePoly::draw(SHAPE_DX& dx)
	{
		if (is_opaque(m_fill_color)) {
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillGeometry(m_path_geom.get(), dx.m_shape_brush.get(), nullptr);
		}
		if (is_opaque(m_stroke_color)) {
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawGeometry(
				m_path_geom.get(),
				dx.m_shape_brush.get(),
				static_cast<FLOAT>(m_stroke_width),
				m_d2d_stroke_style.get());
		}
		if (is_selected() != true) {
			return;
		}
		D2D1_POINT_2F a_pos { m_pos };	// 図形の部位の位置
		anchor_draw_rect(a_pos, dx);
		const size_t d_cnt = m_diff.size();	// 差分の数
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(a_pos, m_diff[i], a_pos);
			anchor_draw_rect(a_pos, dx);
		}
	}

	// 塗りつぶし色を得る.
	// value	得られた値
	// 戻り値	得られたなら true
	bool ShapePoly::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	uint32_t ShapePoly::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		constexpr D2D1_POINT_2F PZ{ 0.0f, 0.0f };	// 零点
		const size_t v_cnt = m_diff.size() + 1;	// 頂点の数 (差分の数 + 1)
		std::vector<D2D1_POINT_2F> v_pos(v_cnt);	// 頂点の配列
		size_t j = static_cast<size_t>(-1);	// 点を含む頂点の添え字
		// 判定する位置が原点となるよう, 平行移動した多角形の頂点を得る.
		pt_sub(m_pos, t_pos, v_pos[0]);
		if (pt_in_anch(v_pos[0], a_len)) {
			j = 0;
		}
		for (size_t i = 1; i < v_cnt; i++) {
			pt_add(v_pos[i - 1], m_diff[i - 1], v_pos[i]);
			if (pt_in_anch(v_pos[i], a_len)) {
				j = i;
			}
		}
		if (j != -1) {
			const auto anch = ANCH_TYPE::ANCH_P0 + j;
			return static_cast<uint32_t>(anch);
		}
		if (is_opaque(m_stroke_color)) {
			// 太さがちょうど線の太さになるような,　幅をもつ辺を計算する.
			const auto e_width = max(max(m_stroke_width, a_len) * 0.5, 0.5);	// 拡張する幅

			// 各辺の法線ベクトルを得る.
			std::vector<D2D1_POINT_2F> n_vec(v_cnt);	// 法線ベクトル
			poly_get_nvec(v_cnt, v_pos.data(), n_vec.data());

			// 多角形の各辺が位置を含むか判定する.
			std::vector<D2D1_POINT_2F> q_exp(v_cnt * 4);	// 幅をもつ辺
			if (poly_test_side(PZ, v_cnt, m_end_closed, v_pos.data(), n_vec.data(), e_width, q_exp.data())) {
				// 含むなら ANCH_STROKE を返す.
				return ANCH_TYPE::ANCH_STROKE;
			}

			// 多角形の角が位置を含むか判定する.
			// 角を超えて延長する長さは線の太さの 5 倍.
			// こうすれば, D2D の描画と一致する.
			const auto ext_len = m_stroke_width * 5.0;	// 角を超えて延長する長さ
			if (poly_test_corner(PZ, v_cnt, m_end_closed, q_exp.data(), n_vec.data(), ext_len)) {
				// 含むなら ANCH_STROKE を返す.
				return ANCH_TYPE::ANCH_STROKE;
			}
		}
		// 辺が不透明, または位置が辺に含まれていないなら,
		// 塗りつぶし色が不透明か判定する.
		if (is_opaque(m_fill_color)) {
			if (pt_in_poly(PZ, v_cnt, v_pos.data())) {
				// 含まれるなら ANCH_FILL を返す.
				return ANCH_TYPE::ANCH_FILL;
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// 範囲に含まれるか判定する.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapePoly::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		if (!pt_in_rect(m_pos, a_min, a_max)) {
			return false;
		}
		const size_t n = m_diff.size();	// 差分の数
		D2D1_POINT_2F e_pos = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(e_pos, m_diff[i], e_pos);	// 次の位置
			if (!pt_in_rect(e_pos, a_min, a_max)) {
				return false;
			}
		}
		return true;
	}

	// 塗りつぶしの色に格納する.
	void ShapePoly::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		if (equal(m_fill_color, value)) {
			return;
		}
		m_fill_color = value;
		create_path_geometry(s_d2d_factory);
	}

	// 図形を作成する.
	// s_pos	開始位置
	// diff	終了位置への差分
	// attr	属性値
	// v_cnt	頂点の数
	// v_reg	正多角形に作図するか判定
	// v_up	頂点を上に作図するか判定
	// v_end	辺を閉じて作図するか判定
	ShapePoly::ShapePoly(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeSheet* attr, const TOOL_POLY& t_poly) :
		ShapePath::ShapePath(t_poly.m_vertex_cnt - 1, attr),
		m_end_closed(t_poly.m_closed),
		m_fill_color(attr->m_fill_color)
	{
		std::vector<D2D1_POINT_2F> v_pos(t_poly.m_vertex_cnt);	// 頂点の配列
		create_poly_by_bbox(s_pos, diff, t_poly.m_vertex_cnt, t_poly.m_vertex_up, t_poly.m_regular, t_poly.m_clockwise, v_pos.data());

		m_pos = v_pos[0];
		for (size_t i = 1; i < t_poly.m_vertex_cnt; i++) {
			pt_sub(v_pos[i], v_pos[i - 1], m_diff[i - 1]);
		}
		create_path_geometry(s_d2d_factory);
	}

	// 図形をデータリーダーから読み込む.
	// dt_reader	データリーダー
	ShapePoly::ShapePoly(DataReader const& dt_reader) :
		ShapePath::ShapePath(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;
		read(m_end_closed, dt_reader);
		read(m_fill_color, dt_reader);
		create_path_geometry(s_d2d_factory);
	}

	// データライターに書き込む.
	void ShapePoly::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapePath::write(dt_writer);
		write(m_end_closed, dt_writer);
		write(m_fill_color, dt_writer);
	}

	// データライターに SVG タグとして書き込む.
	void ShapePoly::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg(m_pos, "M", dt_writer);
		const size_t d_cnt = m_diff.size();	// 差分の数
		for (size_t i = 0; i < d_cnt; i++) {
			write_svg(m_diff[i], "l", dt_writer);
		}
		write_svg("Z\" ", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		write_svg("/>" SVG_NEW_LINE, dt_writer);
	}

}