#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 折れ線の頂点の配列
	constexpr ANCH_WHICH ANCH_QUAD[4]{
		ANCH_WHICH::ANCH_R_SE,
		ANCH_WHICH::ANCH_R_SW,
		ANCH_WHICH::ANCH_R_NE,
		ANCH_WHICH::ANCH_R_NW
	};

	// 四へん形の各辺の法線ベクトルを得る.
	static bool qd_get_nor(const D2D1_POINT_2F q_pos[4], D2D1_POINT_2F n_vec[4]) noexcept;
	// 直行するベクトルを得る.
	static D2D1_POINT_2F qd_orth(const D2D1_POINT_2F v) { return { -v.y, v.x }; }
	// 四へん形の辺が位置を含むか調べる.
	static bool qd_test_expanded(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_pos[], const D2D1_POINT_2F n_vec[], const double exp, D2D1_POINT_2F q_exp[][4]) noexcept;
	// 四へん形の角が位置を含むか調べる.
	static bool qd_test_extended(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F exp[4][4], const D2D1_POINT_2F n_vec[4], const double ext) noexcept;

	// 四へん形の各辺の法線ベクトルを得る.
	// q_pos	四辺形
	// q_nor	得られた各辺の法線ベクトル.
	// 戻り値	法線ベクトルを得たなら true, すべての頂点が重なっていたなら false
	static bool qd_get_nor(const D2D1_POINT_2F q_pos[4], D2D1_POINT_2F q_nor[4]) noexcept
	{
		// 四辺形の各辺の長さと法線ベクトル, 
		// 重複しない頂点の数を求める.
		double q_len[4];
		int q_cnt = 1;
		for (int i = 0; i < 4; i++) {
			// 次の頂点との差分を求める.
			D2D1_POINT_2F q_sub;
			pt_sub(q_pos[(i + 1) & 3], q_pos[i], q_sub);
			// 差分の長さを求める.
			q_len[i] = sqrt(pt_abs2(q_sub));
			if (q_len[i] > FLT_MIN) {
				// 差分の長さが 0 より大きいなら, 
				// 重複しない頂点の数をインクリメントする.
				q_cnt++;
			}
			// 差分と直行するベクトルを正規化して法線ベクトルに格納する.
			pt_scale(qd_orth(q_sub), 1.0 / q_len[i], q_nor[i]);
		}
		if (q_cnt == 1) {
			// すべての頂点が重なったなら, false を返す.
			return false;
		}
		for (int i = 0; i < 4; i++) {
			if (q_len[i] <= FLT_MIN) {
				// 辺の長さが 0 ならば,
				// 辺に隣接する前後の辺の中から
				// 長さが 0 でない辺を探し,
				// それらの法線ベクトルを合成し, 
				// 長さ 0 の辺の法線ベクトルとする.
				int prev;
				for (int j = 1; q_len[prev = ((i - j) & 3)] < FLT_MIN; j++);
				int next;
				for (int j = 1; q_len[next = ((i + j) & 3)] < FLT_MIN; j++);
				pt_add(q_nor[prev], q_nor[next], q_nor[i]);
				auto len = sqrt(pt_abs2(q_nor[i]));
				if (len > FLT_MIN) {
					pt_scale(q_nor[i], 1.0 / len, q_nor[i]);
					continue;
				}
				// 合成ベクトルがゼロベクトルになるなら,
				// 直交するベクトルを法線ベクトルとする.
				q_nor[i] = qd_orth(q_nor[prev]);
			}
		}
		return true;
	}

	// 四へん形の辺が位置を含むか調べる.
	// t_pos	調べる位置
	// q_pos	四へん形
	// q_nor	四へん形の各辺の法線ベクトル
	// exp	辺の太さ
	// q_exp	太さ分拡張した各辺
	static bool qd_test_expanded(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_pos[], const D2D1_POINT_2F q_nor[], const double exp, D2D1_POINT_2F q_exp[][4]) noexcept
	{
		for (int i = 0; i < 4; i++) {
			// 辺の片方の端点を法線ベクトルにそって
			// 太さの半分だけ移動した位置を
			// 拡張した辺に格納する.
			D2D1_POINT_2F nor;
			pt_scale(q_nor[i], exp, nor);
			pt_add(q_pos[i], nor, q_exp[i][0]);
			// 逆方向にも移動し, 拡張した辺に格納する.
			pt_sub(q_pos[i], nor, q_exp[i][1]);
			// 辺のもう一方の端点を法線ベクトルにそって
			// 太さの半分だけ移動した位置を
			// 拡張した辺に格納する.
			const auto j = (i + 1) & 3;
			pt_sub(q_pos[j], nor, q_exp[i][2]);
			// 逆方向にも移動し, 拡張した辺に格納する.
			pt_add(q_pos[j], nor, q_exp[i][3]);
			// 位置が拡張した辺に含まれるか調べる.
			if (pt_in_quad(t_pos, q_exp[i])) {
				// 含まれるなら true を返す.
				return true;
			}
		}
		return false;
	}

	// 四へん形の角が位置を含むか調べる.
	// t_pos	調べる位置
	// q_exp	四へん形の拡張された各辺
	// q_nor	四へん形の各辺の法線ベクトル
	// ext	角を延長する長さ
	static bool qd_test_extended(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_exp[4][4], const D2D1_POINT_2F q_nor[4], const double ext) noexcept
	{
		for (int i = 0, j = 3; i < 4; j = i++) {
			D2D1_POINT_2F q_ext[4];	// 延長した辺
			D2D1_POINT_2F vec;	// 平行なベクトル
			// ある頂点に隣接する辺について.
			// 拡張した辺の端を, 延長した辺の端に格納する.
			q_ext[0] = q_exp[j][3];
			q_ext[1] = q_exp[j][2];
			// 法線ベクトルと直行するベクトルを,
			// 延長する長さの分だけ倍し,
			// 平行なベクトルに格納する.
			pt_scale(qd_orth(q_nor[j]), ext, vec);
			// 格納した位置を平行なベクトルに沿って延長し,
			// 延長した辺のもう一方の端に格納する.
			pt_sub(q_ext[1], vec, q_ext[2]);
			pt_sub(q_ext[0], vec, q_ext[3]);
			// 位置が延長した辺に含まれるか調べる.
			if (pt_in_quad(t_pos, q_ext) != true) {
				// 含まれないなら継続する.
				continue;
			}
			// 隣接するもう片方の辺について.
			// 拡張した辺の端を, 延長した辺の端に格納する.
			q_ext[2] = q_exp[i][1];
			q_ext[3] = q_exp[i][0];
			// 法線ベクトルと直行するベクトル (先ほどとは逆方向) を得て,
			// 角を延長する長さの分だけ倍し,
			// 平行なベクトルに格納する.
			pt_scale(qd_orth(q_nor[i]), ext, vec);
			// 格納した位置を平行なベクトルに沿って延長し,
			// 延長した辺のもう一方の端に格納する.
			pt_add(q_ext[3], vec, q_ext[0]);
			pt_add(q_ext[2], vec, q_ext[1]);
			// 位置が延長した辺に含まれるか調べる.
			if (pt_in_quad(t_pos, q_ext)) {
				// 含まれるなら true を返す.
				return true;
			}
		}
		return false;
	}

	// パスジオメトリを作成する.
	void ShapeQuad::create_path_geometry(void)
	{
		D2D1_POINT_2F q_pos[4];

		m_poly_geom = nullptr;
		q_pos[0] = m_pos;
		pt_add(q_pos[0], m_diff, q_pos[1]);
		pt_add(q_pos[1], m_diff_1, q_pos[2]);
		pt_add(q_pos[2], m_diff_2, q_pos[3]);
		winrt::com_ptr<ID2D1GeometrySink> sink;
		winrt::check_hresult(
			s_d2d_factory->CreatePathGeometry(m_poly_geom.put())
		);
		m_poly_geom->Open(sink.put());
		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		const auto figure_begin = is_opaque(m_fill_color)
			? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
			: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
		sink->BeginFigure(q_pos[0], figure_begin);
		sink->AddLine(q_pos[1]);
		sink->AddLine(q_pos[2]);
		sink->AddLine(q_pos[3]);
		// Shape 上で始点と終点を重ねたとき,
		// パスに始点を加えないと, LINE_JOINT がへんなことになる.
		sink->AddLine(q_pos[0]);
		sink->EndFigure(D2D1_FIGURE_END_CLOSED);
		sink->Close();
		sink = nullptr;
	}

	// 図形を表示する.
	// dx	図形の描画環境
	void ShapeQuad::draw(SHAPE_DX& dx)
	{
		if (is_opaque(m_fill_color)) {
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillGeometry(m_poly_geom.get(), dx.m_shape_brush.get(), nullptr);
		}
		if (is_opaque(m_stroke_color)) {
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawGeometry(
				m_poly_geom.get(),
				dx.m_shape_brush.get(),
				static_cast<FLOAT>(m_stroke_width),
				m_d2d_stroke_style.get());
		}
		if (is_selected() != true) {
			return;
		}
		anchor_draw_rect(m_pos, dx);
		D2D1_POINT_2F a_pos;
		pt_add(m_pos, m_diff, a_pos);
		anchor_draw_rect(a_pos, dx);
		pt_add(a_pos, m_diff_1, a_pos);
		anchor_draw_rect(a_pos, dx);
		pt_add(a_pos, m_diff_2, a_pos);
		anchor_draw_rect(a_pos, dx);
	}

	// 塗りつぶし色を得る.
	// val	得られた値
	// 戻り値	得られたなら true
	bool ShapeQuad::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// 位置を含むか調べる.
	// t_pos	調べる位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	ANCH_WHICH ShapeQuad::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		constexpr D2D1_POINT_2F ZP{ 0.0f, 0.0f };
		// 調べる位置が原点となるよう平行移動した四へん形の各頂点を得る.
		D2D1_POINT_2F q_pos[4];
		pt_sub(m_pos, t_pos, q_pos[3]);
		pt_add(q_pos[3], m_diff, q_pos[2]);
		pt_add(q_pos[2], m_diff_1, q_pos[1]);
		pt_add(q_pos[1], m_diff_2, q_pos[0]);
		for (int i = 0; i < 4; i++) {
			// 位置が, 四へん形の各部位 (頂点) に含まれるか調べる.
			if (pt_in_anch(q_pos[i], a_len)) {
				// 含まれるなら, 該当する部位を返す.
				return ANCH_QUAD[i];
			}
		}
		if (is_opaque(m_stroke_color)) {
			// 辺が不透明なら,
			// 線の太さをもとに, 辺を拡張する太さを計算する.
			const auto exp = max(max(m_stroke_width, a_len) * 0.5, 0.5);
			// 各辺の法線ベクトルを得る.
			D2D1_POINT_2F q_nor[4];
			if (qd_get_nor(q_pos, q_nor) != true) {
				// 法線ベクトルがない (すべての頂点が重なっている) なら,
				// 位置が, 拡張する太さを半径とする円に含まれるか調べる.
				if (pt_abs2(q_pos[0]) <= exp * exp) {
					// 含まれるなら ANCH_FRAME を返す.
					return ANCH_WHICH::ANCH_FRAME;
				}
				// そうでなければ ANCH_NONE を返す.
				return ANCH_WHICH::ANCH_OUTSIDE;
			}
			// 四辺形の辺が位置を含むか調べる.
			D2D1_POINT_2F q_exp[4][4];
			if (qd_test_expanded(ZP, q_pos, q_nor, exp, q_exp)) {
				// 含むなら ANCH_FRAME を返す.
				return ANCH_WHICH::ANCH_FRAME;
			}
			// 四辺形の角が位置を含むか調べる.
			// 角を延長する長さは辺の太さの 5 倍.
			// こうすれば, D2D の描画と一致する.
			const auto ext = m_stroke_width * 5.0;
			if (qd_test_extended(ZP, q_exp, q_nor, ext)) {
				// 含むなら ANCH_FRAME を返す.
				return ANCH_WHICH::ANCH_FRAME;
			}
		}
		// 辺が不透明, または位置が辺に含まれていないなら,
		// 塗りつぶし色が不透明か調べる.
		if (is_opaque(m_fill_color)) {
			// 不透明なら, 位置が四へん形に含まれるか調べる.
			if (pt_in_quad(ZP, q_pos)) {
				// 含まれるなら ANCH_INSIDE を返す.
				return ANCH_WHICH::ANCH_INSIDE;
			}
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// 範囲に含まれるか調べる.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeQuad::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F e_pos;	// 次の位置
		if (pt_in_rect(m_pos, a_min, a_max)) {
			pt_add(m_pos, m_diff, e_pos);
			if (pt_in_rect(e_pos, a_min, a_max)) {
				pt_add(e_pos, m_diff_1, e_pos);
				if (pt_in_rect(e_pos, a_min, a_max)) {
					pt_add(e_pos, m_diff_2, e_pos);
					return pt_in_rect(e_pos, a_min, a_max);
				}
			}
		}
		return false;
	}

	// 塗りつぶしの色に格納する.
	void ShapeQuad::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		if (equal(m_fill_color, value)) {
			return;
		}
		m_fill_color = value;
		create_path_geometry();
	}

	// 図形を作成する.
	// s_pos	開始位置
	// diff	終了位置への差分
	// attr	既定の属性値
	ShapeQuad::ShapeQuad(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeLayout* attr) :
		ShapePoly::ShapePoly(attr)
	{
		m_pos.x = static_cast<FLOAT>(s_pos.x + 0.5 * diff.x);
		m_pos.y = s_pos.y;
		pt_scale(diff, 0.5, m_diff);
		m_diff_1.x = -m_diff.x;
		m_diff_1.y = m_diff.y;
		m_diff_2.x = m_diff_1.x;
		m_diff_2.y = -m_diff.y;
		m_fill_color = attr->m_fill_color;
		create_path_geometry();
		D2D1_POINT_2F q_pos[4];
		q_pos[0] = { 0.0f, 0.0f };
		q_pos[1] = m_diff;
		q_pos[2] = m_diff_1;
		q_pos[3] = m_diff_2;
	}

	// 図形をデータリーダーから読み込む.
	// dt_reader	データリーダー
	ShapeQuad::ShapeQuad(DataReader const& dt_reader) :
		ShapePoly::ShapePoly(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_fill_color, dt_reader);
		create_path_geometry();
	}

	// データライターに書き込む.
	void ShapeQuad::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapePoly::write(dt_writer);
		write(m_fill_color, dt_writer);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeQuad::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg(m_pos, "M", dt_writer);
		write_svg(m_diff, "l", dt_writer);
		write_svg(m_diff_1, "l", dt_writer);
		write_svg(m_diff_2, "l", dt_writer);
		write_svg("Z\" ", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
	}

}