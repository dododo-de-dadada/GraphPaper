//------------------------------
// shape_bezier.cpp
// ベジェ曲線
//------------------------------
#include "pch.h"
#include "shape.h"
#include "shape_bezier.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// セグメントを区切る助変数の値
	constexpr double T0 = 0.0;	// 区間の開始
	constexpr double T1 = 1.0 / 3.0;	// 1 番目の区切り
	constexpr double T2 = 2.0 / 3.0;	// 2 番目の区切り
	constexpr double T3 = 1.0;	// 区間の終端

	// 曲線の矢じるしのジオメトリを作成する.
	static HRESULT bezi_create_arrow_geom(ID2D1Factory3* const factory, const D2D1_POINT_2F start, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_STYLE a_style, const ARROW_SIZE a_size, ID2D1PathGeometry** a_geo) noexcept;

	// 点の配列をもとにそれらをすべて含む凸包を求める.
	static void bezi_get_convex(const uint32_t p_cnt, const POINT_2D p[], uint32_t& c_cnt, POINT_2D c[]);

	// 曲線の端が点を含むか判定する.
	template<D2D1_CAP_STYLE S>
	static bool bezi_hit_test_cap(const D2D1_POINT_2F& t, const D2D1_POINT_2F c[4], const D2D1_POINT_2F s[3], const double e_width);

	// 点が凸包に含まれるか判定する.
	static bool bezi_in_convex(const double tx, const double ty, const size_t c_cnt, const POINT_2D c[]) noexcept;

	// 曲線上の助変数をもとに位置を求める.
	static inline void bezi_point_by_param(const POINT_2D c[4], const double t, POINT_2D& p) noexcept;

	//------------------------------
	// 矢じりの返しと先端の点を得る
	//------------------------------
	bool ShapeBezier::bezi_get_pos_arrow(
		const D2D1_POINT_2F b_start, const D2D1_BEZIER_SEGMENT& b_seg,	// 曲線の始点と制御点
		const ARROW_SIZE a_size,	// 矢じるしの寸法
		D2D1_POINT_2F arrow[3]	// 矢じりの返しと先端の点
	) noexcept
	{
		POINT_2D seg[3]{};
		POINT_2D c[4]{};	// 制御点

		// 制御点を配列に格納する.
		seg[0] = b_seg.point1;
		seg[1] = b_seg.point2;
		seg[2] = b_seg.point3;

		// 座標値による誤差を少なくできる, と思われるので,
		// ベジェ曲線を始点が原点となるように平行移動.
		c[3] = 0.0;
		c[2] = seg[0] - b_start;
		c[1] = seg[1] - b_start;
		c[0] = seg[2] - b_start;
		auto b_len = bezi_len_by_param(c, 0.0, 1.0, SIMPSON_CNT);
		if (b_len >= FLT_MIN) {

			// 矢じるしの先端の位置と, 曲線の長さの, どちらか短い方で, 助変数を求める.
			const auto t = bezi_param_by_len(c, min(b_len, a_size.m_offset));

			// 助変数をもとに曲線の接線ベクトルを得る.
			POINT_2D v;
			bezi_tvec_by_param(c, t, v);

			// 矢じるしの返しの位置を計算する
			get_barbs(-v, sqrt(v * v), a_size.m_width, a_size.m_length, arrow);

			// 助変数で曲線上の位置を得る.
			POINT_2D tip;	// 終点を原点とする, 矢じるしの先端の位置
			bezi_point_by_param(c, t, tip);

			// 曲線上の位置を矢じるしの先端とし, 返しの位置も並行移動する.
			pt_add(arrow[0], tip.x, tip.y, arrow[0]);
			pt_add(arrow[1], tip.x, tip.y, arrow[1]);
			arrow[2] = tip;
			pt_add(arrow[0], b_start, arrow[0]);
			pt_add(arrow[1], b_start, arrow[1]);
			pt_add(arrow[2], b_start, arrow[2]);
			return true;
		}
		return false;
	}

	//------------------------------
	// 曲線の矢じるしのジオメトリを作成する.
	//------------------------------
	static HRESULT bezi_create_arrow_geom(
		ID2D1Factory3* const factory,	// D2D ファクトリ
		const D2D1_POINT_2F b_start, const D2D1_BEZIER_SEGMENT& b_seg,	// 曲線の始点と制御点
		const ARROW_STYLE a_style,	// 矢じるしの種別
		const ARROW_SIZE a_size,	// 矢じるしの寸法
		ID2D1PathGeometry** a_geom	// 矢じるしのジオメトリ
	) noexcept
	{
		D2D1_POINT_2F barb[3];	// 矢じるしの返しの端点	
		winrt::com_ptr<ID2D1GeometrySink> sink;
		HRESULT hr = S_OK;
		if (!ShapeBezier::bezi_get_pos_arrow(b_start, b_seg, a_size, barb)) {
			hr = E_FAIL;
		}
		if (hr == S_OK) {
			// ジオメトリシンクに追加する.
			hr = factory->CreatePathGeometry(a_geom);
		}
		if (hr == S_OK) {
			hr = (*a_geom)->Open(sink.put());
		}
		if (hr == S_OK) {
			const D2D1_FIGURE_BEGIN f_begin = a_style == ARROW_STYLE::ARROW_FILLED ?
				D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
				D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
			const D2D1_FIGURE_END f_end = a_style == ARROW_STYLE::ARROW_FILLED ?
				D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED :
				D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN;
			sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(barb[0], f_begin);
			sink->AddLine(barb[2]);
			sink->AddLine(barb[1]);
			sink->EndFigure(f_end);
		}
		if (hr == S_OK) {
			hr = sink->Close();
		}
		sink = nullptr;
		return hr;
	}

	//------------------------------
	// 点の配列をもとにそれらをすべて含む凸包を求める.
	// ギフト包装法をもちいる.
	// p_cnt	点の数
	// p	点の配列
	// c_cnt	凸包の頂点の数
	// c	凸包の頂点の配列
	//------------------------------
	static void bezi_get_convex(
		const uint32_t p_cnt, const POINT_2D p[], uint32_t& c_cnt, POINT_2D c[])
	{
		// e のうち, y 値が最も小さい点の集合から, x 値が最も小さい点の添え字 k を得る.
		uint32_t k = 0;
		double ex = p[0].x;
		double ey = p[0].y;
		for (uint32_t i = 1; i < p_cnt; i++) {
			if (p[i].y < ey || (p[i].y == ey && p[i].x < ex)) {
				ex = p[i].x;
				ey = p[i].y;
				k = i;
			}
		}
		// p[k] を a に格納する.
		POINT_2D a = p[k];
		c_cnt = 0;
		do {
			// c に a を追加する.
			c[c_cnt++] = a;
			POINT_2D b = p[0];
			for (uint32_t i = 1; i < p_cnt; i++) {
				const POINT_2D d = p[i];
				if (b == a) {
					b = d;
				}
				else {
					const POINT_2D ab = b - a;
					const POINT_2D ad = d - a;
					const double v = ab.opro(ad);
					if (v > 0.0 || (fabs(v) < FLT_MIN && ad * ad > ab * ab)) {
						b = d;
					}
				}
			}
			a = b;
		} while (c_cnt < p_cnt && a != c[0]);
	}

	//------------------------------
	// 曲線の端が点を含むか判定する.
	// 戻り値	含むなら true
	//------------------------------
	template<D2D1_CAP_STYLE S> static bool bezi_hit_test_cap(
		const D2D1_POINT_2F& t,	// 判定される点	
		const D2D1_POINT_2F c[4],	// 凸包 (四辺形) の頂点の配列
		const D2D1_POINT_2F s[3],	// 凸包の辺のベクトル
		const double ew	// 凸包の辺の半分の太さ
	)
	{
		size_t i;
		for (i = 0; i < 3; i++) {
			const double s_abs = pt_abs2(s[i]);
			if (s_abs >= FLT_MIN) {
				D2D1_POINT_2F s_dir;	// 辺の方向ベクトル
				pt_mul(s[i], -ew / sqrt(s_abs), s_dir);
				D2D1_POINT_2F s_orth{ s_dir.y, -s_dir.x };	// 辺の直交ベクトル
				D2D1_POINT_2F q[4];	// 端点の四辺形.
				pt_add(c[i], s_orth, q[0]);
				pt_sub(c[i], s_orth, q[1]);
				if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
					pt_add(q[1], s_dir, q[2]);
					pt_add(q[0], s_dir, q[3]);
					if (pt_in_poly(t, 4, q)) {
						return true;
					}
				}
				else if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
					pt_add(q[i], s_dir, q[2]);
					if (pt_in_poly(t, 3, q)) {
						return true;
					}
				}
				break;
			}
		}
		if (i == 3) {
			D2D1_POINT_2F q[4]{};
			if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
				pt_add(c[0], -ew, q[0]);
				pt_add(c[0], ew, -ew, q[1]);
				pt_add(c[0], ew, q[2]);
				pt_add(c[0], -ew, ew, q[3]);
				if (pt_in_poly(t, 4, q)) {
					return true;
				}
			}
			else if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				pt_add(c[0], 0.0, -ew, q[0]);
				pt_add(c[0], -ew, 0.0, q[1]);
				pt_add(c[0], 0.0, ew, q[2]);
				pt_add(c[0], ew, 0.0, q[3]);
				if (pt_in_poly(t, 4, q)) {
					return true;
				}
			}
		}
		else {
			for (size_t j = 3; j > 0; j--) {
				const double s_abs = pt_abs2(s[j - 1]);
				if (s_abs >= FLT_MIN) {
					D2D1_POINT_2F s_dir;	// 辺の方向ベクトル
					pt_mul(s[j - 1], ew / sqrt(s_abs), s_dir);
					D2D1_POINT_2F s_orth{ s_dir.y, -s_dir.x };	// 辺の直交ベクトル
					D2D1_POINT_2F q[4];
					pt_add(c[j], s_orth, q[0]);
					pt_sub(c[j], s_orth, q[1]);
					if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
						pt_add(q[1], s_dir, q[2]);
						pt_add(q[0], s_dir, q[3]);
						if (pt_in_poly(t, 4, q)) {
							return true;
						}
					}
					else if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
						pt_add(c[j], s_dir, q[2]);
						if (pt_in_poly(t, 3, q)) {
							return true;
						}
					}
					break;
				}
			}
		}
		return false;
	}

	//------------------------------
	// 点が凸包に含まれるか判定する.
	// 交差数判定を用いる.
	// tx, ty	点
	// c_cnt	凸包の頂点の数
	// c	凸包の頂点の配列
	// 戻り値	含まれるなら true を, 含まれないなら false を返す.
	//------------------------------
	static bool bezi_in_convex(const double tx, const double ty, const size_t c_cnt, const POINT_2D c[]) noexcept
	{
		int k = 0;	// 点をとおる水平線が凸包の辺と交差する回数.
		for (size_t i = c_cnt - 1, j = 0; j < c_cnt; i = j++) {
			// ルール 1. 上向きの辺. 点が垂直方向について, 辺の始点と終点の間にある. ただし、終点は含まない.
			// ルール 2. 下向きの辺. 点が垂直方向について, 辺の始点と終点の間にある. ただし、始点は含まない.
			// ルール 3. 点をとおる水平線と辺が水平でない.
			// ルール 1. ルール 2 を確認することで, ルール 3 も確認できている.
			if ((c[i].y <= ty && c[j].y > ty) || (c[i].y > ty && c[j].y <= ty)) {
				// ルール 4. 辺は点よりも右側にある. ただし, 重ならない.
				// 辺が点と同じ高さになる位置を特定し, その時の水平方向の値と点のその値とを比較する.
				if (tx < c[i].x + (ty - c[i].y) / (c[j].y - c[i].y) * (c[j].x - c[i].x)) {
					// ルール 1 またはルール 2, かつルール 4 を満たすなら, 点をとおる水平線は凸包と交差する.
					k++;
				}
			}
		}
		// 交差する回数が奇数か判定する.
		// 奇数ならば, 点は凸包に含まるので true を返す.
		return static_cast<bool>(k & 1);
	}

	//------------------------------
	// 曲線上の助変数をもとに位置を求める.
	// c	制御点
	// t	助変数
	// p	求まった位置
	//------------------------------
	static inline void bezi_point_by_param(const POINT_2D c[4], const double t, POINT_2D& p) noexcept
	{
		const double s = 1.0 - t;
		const double ss = s * s;
		const double tt = t * t;
		p = c[0] * s * ss + c[1] * 3.0 * ss * t + c[2] * 3.0 * s * tt + c[3] * t * tt;
	}

	//------------------------------
	// 図形を表示する.
	//------------------------------
	void ShapeBezier::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = SHAPE::m_d2d_target;
		ID2D1SolidColorBrush* const brush = SHAPE::m_d2d_color_brush.get();
		ID2D1Factory* factory;
		m_d2d_target->GetFactory(&factory);
		bool b_flag = false;
		D2D1_BEZIER_SEGMENT b_seg{};
		HRESULT hr = S_OK;
		const bool exist_stroke = (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color));
		if (exist_stroke && m_d2d_stroke_style == nullptr) {
			if (hr == S_OK) {
				hr = create_stroke_style(factory);
			}
		}
		if (exist_stroke && m_arrow_style != ARROW_STYLE::ARROW_NONE && m_d2d_arrow_stroke == nullptr) {
			if (hr == S_OK) {
				hr = create_arrow_stroke();
			}
		}
		if ((exist_stroke || is_opaque(m_fill_color)) && m_d2d_path_geom == nullptr) {
			if (!b_flag) {
				pt_add(m_start, m_lineto[0], b_seg.point1);
				pt_add(b_seg.point1, m_lineto[1], b_seg.point2);
				pt_add(b_seg.point2, m_lineto[2], b_seg.point3);
				b_flag = true;
			}
			if (hr == S_OK) {
				hr = factory->CreatePathGeometry(m_d2d_path_geom.put());
			}
			winrt::com_ptr<ID2D1GeometrySink> sink;
			if (hr == S_OK) {
				hr = m_d2d_path_geom->Open(sink.put());
			}
			if (hr == S_OK) {
				sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
				const auto f_begin = (is_opaque(m_fill_color) ?
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
				sink->BeginFigure(m_start, f_begin);
				sink->AddBezier(b_seg);
				sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
				hr = sink->Close();
			}
			sink = nullptr;
		}
		if (exist_stroke && m_arrow_style != ARROW_STYLE::ARROW_NONE && m_d2d_arrow_geom == nullptr) {
			if (!b_flag) {
				b_seg.point1.x = m_start.x + m_lineto[0].x;
				b_seg.point1.y = m_start.y + m_lineto[0].y;
				b_seg.point2.x = b_seg.point1.x + m_lineto[1].x;
				b_seg.point2.y = b_seg.point1.y + m_lineto[1].y;
				b_seg.point3.x = b_seg.point2.x + m_lineto[2].x;
				b_seg.point3.y = b_seg.point2.y + m_lineto[2].y;
				b_flag = true;
			}
			hr = bezi_create_arrow_geom(static_cast<ID2D1Factory3*>(factory), m_start, b_seg, m_arrow_style, m_arrow_size,
				m_d2d_arrow_geom.put());
		}
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillGeometry(m_d2d_path_geom.get(), brush, nullptr);
		}
		if (exist_stroke) {
			brush->SetColor(m_stroke_color);
			target->DrawGeometry(m_d2d_path_geom.get(), brush, m_stroke_width, m_d2d_stroke_style.get());
			if (m_arrow_style == ARROW_STYLE::ARROW_FILLED) {
				target->FillGeometry(m_d2d_arrow_geom.get(), brush, nullptr);
				target->DrawGeometry(m_d2d_arrow_geom.get(), brush, m_stroke_width, m_d2d_arrow_stroke.get());
			}
			else if (m_arrow_style == ARROW_STYLE::ARROW_OPENED) {
				target->DrawGeometry(m_d2d_arrow_geom.get(), brush, m_stroke_width, m_d2d_arrow_stroke.get());
			}
		}
		if (m_hit_show && is_selected()) {
			if (!b_flag) {
				b_seg.point1.x = m_start.x + m_lineto[0].x;
				b_seg.point1.y = m_start.y + m_lineto[0].y;
				b_seg.point2.x = b_seg.point1.x + m_lineto[1].x;
				b_seg.point2.y = b_seg.point1.y + m_lineto[1].y;
				b_seg.point3.x = b_seg.point2.x + m_lineto[2].x;
				b_seg.point3.y = b_seg.point2.y + m_lineto[2].y;
				b_flag = true;
			}
			// 補助線を描く
			if (m_stroke_width >= SHAPE::m_hit_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawGeometry(m_d2d_path_geom.get(), brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawGeometry(m_d2d_path_geom.get(), brush, m_aux_width, m_aux_style.get());
			}
			// 制御点への補助線を描く.
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(m_start, b_seg.point1, brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(m_start, b_seg.point1, brush, m_aux_width, m_aux_style.get());
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(b_seg.point1, b_seg.point2, brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(b_seg.point1, b_seg.point2, brush, m_aux_width, m_aux_style.get());
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(b_seg.point2, b_seg.point3, brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(b_seg.point2, b_seg.point3, brush, m_aux_width, m_aux_style.get());
			// 図形の部位を描く.
			hit_draw_circle(b_seg.point2, target, brush);
			hit_draw_circle(b_seg.point1, target, brush);
			hit_draw_square(m_start, target, brush);
			hit_draw_square(b_seg.point3, target, brush);
		}
	}

	// test_pt	判定される点
	// b_pt	制御点
	// b_to	次の制御点へのベクトル
	uint32_t ShapeBezier::hit_test(const D2D1_POINT_2F test_pt, const D2D1_POINT_2F bezi_pt[4], const D2D1_POINT_2F bezi_to[3], const D2D1_CAP_STYLE stroke_cap, const bool fill_opa, const double ew) noexcept
	{
		bool fill_test = false;	// 位置が塗りつぶしに含まれるか判定
		if (equal(stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND)) {
			if (pt_in_circle(test_pt, bezi_pt[0], ew)) {
				return HIT_TYPE::HIT_STROKE;
			}
			if (pt_in_circle(test_pt, bezi_pt[3], ew)) {
				return HIT_TYPE::HIT_STROKE;
			}
		}
		else if (equal(stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE)) {
			if (bezi_hit_test_cap<D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE>(test_pt, bezi_pt, bezi_to, ew)) {
				return HIT_TYPE::HIT_STROKE;
			}
		}
		else if (equal(stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE)) {
			if (bezi_hit_test_cap<D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE>(test_pt, bezi_pt, bezi_to, ew)) {
				return HIT_TYPE::HIT_STROKE;
			}
		}
		// 最初の制御点の組をプッシュする.
		// ４つの点のうち端点は, 次につまれる組と共有するので, 1 + 3 * D_MAX 個の配列を確保する.
		constexpr int32_t D_MAX = 64;	// 分割する深さの最大値
		POINT_2D s[1 + D_MAX * 3]{};	// 制御点のスタック
		int32_t s_cnt = 0;	// スタックに積まれた点の数
		s[0] = bezi_pt[0];
		s[1] = bezi_pt[1];
		s[2] = bezi_pt[2];
		s[3] = bezi_pt[3];
		s_cnt += 4;
		// スタックに 一組以上の制御点が残っているか判定する.
		while (s_cnt >= 4) {
			// 制御点をポップする.
			POINT_2D c[10];
			c[3] = s[s_cnt - 1];
			c[2] = s[s_cnt - 2];
			c[1] = s[s_cnt - 3];
			c[0] = s[s_cnt - 4];
			// 端点は共有なのでピークする;
			s_cnt -= 4 - 1;
			// 制御点の組から凸包 c0 を得る (実際は方形で代用する).
			// 制御点の組から, 重複するものを除いた点の集合を得る.
			POINT_2D c0_lt = c[0];	// 凸包 c0 (を含む方形の左上点)
			POINT_2D c0_rb = c[0];	// 凸包 c0 (を含む方形の右下点)
			POINT_2D d[4];	// 重複しない点の集合.
			uint32_t d_cnt = 0;	// 重複しない点の集合の要素数
			d[d_cnt++] = c[0];
			for (uint32_t i = 1; i < 4; i++) {
				if (d[d_cnt - 1] != c[i]) {
					d[d_cnt++] = c[i];
					c[i].exp(c0_lt, c0_rb);
				}
			}
			// 重複しない点の集合の要素数が 2 未満か判定する.
			if (d_cnt < 2) {
				// 制御点の組は 1 点に集まっているので, 中断する.
				continue;
			}

			// 拡張・延長された線分を得る.
			//   e[i][0]             e[i][1]
			//        + - - - - - - - - - +
			//        |           orth|
			//   d[i] +---------------+---> dir
			//        |           d[i+1]
			//        + - - - - - - - - - +
			//   e[i][3]             e[i][2]
			POINT_2D e[3 * 4];	// 拡張・延長された線分の配列
			for (uint32_t i = 0, j = 0; i < d_cnt - 1; i++, j += 4) {
				auto e_dir = d[i + 1] - d[i];	// 次の頂点への位置ベクトル
				e_dir = e_dir * (ew / std::sqrt(e_dir * e_dir));
				const POINT_2D e_orth{ e_dir.y, -e_dir.x };	// 線分の直交ベクトル

				// 法線ベクトルにそって正逆の方向に線分を拡張する.
				e[j + 0] = d[i] + e_orth;
				e[j + 1] = d[i + 1] + e_orth;
				e[j + 2] = d[i + 1] - e_orth;
				e[j + 3] = d[i] - e_orth;
				if (i > 0) {
					// 最初の制御点以外は, 線分ベクトルの方向に延長する.
					e[j + 0] = e[j + 0] - e_dir;
					e[j + 3] = e[j + 3] - e_dir;
				}
				if (i + 1 < d_cnt - 1) {
					// 最後の制御点以外は, 線分ベクトルの逆方向に延長する.
					e[j + 1] = e[j + 1] + e_dir;
					e[j + 2] = e[j + 2] + e_dir;
				}
			}
			// 拡張・延長された線分から, 凸包 c1 を得る.
			uint32_t c1_cnt;
			POINT_2D c1[3 * 4];
			bezi_get_convex((d_cnt - 1) * 4, e, c1_cnt, c1);
			// 点が凸包 c1 に含まれないか判定する.
			if (!bezi_in_convex(test_pt.x, test_pt.y, c1_cnt, c1)) {
				// これ以上この制御点の組を分割する必要はない.
				// スタックに残った他の制御点の組を試す.
				continue;
			}

			// 凸包 c0 の大きさが 1 以下か判定する.
			POINT_2D c0 = c0_rb - c0_lt;
			if (c0.x <= 1.0 && c0.y <= 1.0) {
				// 現在の制御点の組 (凸包 c0) をこれ以上分割する必要はない.
				// 凸包 c1 は判定される点を含んでいるので, 図形の部位を返す.
				return HIT_TYPE::HIT_STROKE;
			}

			// スタックがオバーフローするか判定する.
			if (s_cnt + 6 > 1 + D_MAX * 3) {
				// 現在の制御点の組 (凸包 c0) をこれ以上分割することはできない.
				// 凸包 c1は判定される点を含んでいるので, 図形の部位を返す.
				return HIT_TYPE::HIT_STROKE;
			}

			// 制御点の組を 2 分割する.
			// c[0,1,2,3] の中点を c[4,5,6] に, c[4,5,6] の中点を c[7,8] に, c[7,8] の中点を c[9] に格納する.
			// 分割した制御点の組はそれぞれ c[0,4,7,9] と c[9,8,6,3] になる.
			c[4] = (c[0] + c[1]) * 0.5;
			c[5] = (c[1] + c[2]) * 0.5;
			c[6] = (c[2] + c[3]) * 0.5;
			c[7] = (c[4] + c[5]) * 0.5;
			c[8] = (c[5] + c[6]) * 0.5;
			c[9] = (c[7] + c[8]) * 0.5;
			if (fill_opa && !fill_test) {
				// 分割された凸包のあいだにできた三角形は, 塗りつぶしの領域.
				// この領域に点が含まれるか, 分割するたびに判定する.
				// ただし 1 度でも含まれるなら, それ以上の判定は必要ない.
				const POINT_2D tri[3]{
					c[0], c[9], c[3]
				};
				fill_test = bezi_in_convex(test_pt.x, test_pt.y, 3, tri);
			}
			// 一方の組をプッシュする.
			// 始点 (0) はスタックに残っているので, 
			// 残りの 3 つの制御点をプッシュする.
			s[s_cnt] = c[4];
			s[s_cnt + 1] = c[7];
			s[s_cnt + 2] = c[9];
			// もう一方の組をプッシュする.
			// 始点 (9) はプッシュ済みなので,
			// 残りの 3 つの制御点をプッシュする.
			s[s_cnt + 3] = c[8];
			s[s_cnt + 4] = c[6];
			s[s_cnt + 5] = c[3];
			s_cnt += 6;
		}
		if (fill_opa && fill_test) {
			return HIT_TYPE::HIT_FILL;
		}
		return HIT_TYPE::HIT_SHEET;

	}

	// 図形が点を含むか判定する.
	// 戻り値	点を含む部位
	uint32_t ShapeBezier::hit_test(const D2D1_POINT_2F test_pt, const bool/*ctrl_key*/) const noexcept
	{
		const auto fill_opa = is_opaque(m_fill_color);
		bool fill_test = false;	// 位置が塗りつぶしに含まれるか判定
		const auto ew = max(max(static_cast<double>(m_stroke_width), m_hit_width) * 0.5, 0.5);	// 線枠の太さの半分の値
		const D2D1_POINT_2F t_pt{
			test_pt.x - m_start.x, test_pt.y - m_start.y
		};
		//pt_sub(pt, m_start, tp);
		// 判定される点によって精度が落ちないよう, 曲線の始点が原点となるよう平行移動し, 制御点を得る.
		D2D1_POINT_2F cp[4]{
			{ 0.0f, 0.0f },
			{ m_lineto[0] },
			{ m_lineto[0].x + m_lineto[1].x, m_lineto[0].y + m_lineto[1].y },
			{ m_lineto[0].x + m_lineto[1].x + m_lineto[2].x, m_lineto[0].y + m_lineto[1].y + m_lineto[2].y }
		};
		//cp[0].x = cp[0].y = 0.0;
		//cp[1].x = cp[0].x + m_lineto[0].x;
		//cp[1].y = cp[0].y + m_lineto[0].y;
		//cp[2].x = cp[1].x + m_lineto[1].x;
		//cp[2].y = cp[1].y + m_lineto[1].y;
		//cp[3].x = cp[2].x + m_lineto[2].x;
		//cp[3].y = cp[2].y + m_lineto[2].y;
		if (hit_text_pt(t_pt, cp[3], m_hit_width)) {
			return HIT_TYPE::HIT_P0 + 3;
		}
		if (hit_text_pt(t_pt, cp[2], m_hit_width)) {
			return HIT_TYPE::HIT_P0 + 2;
		}
		if (hit_text_pt(t_pt, cp[1], m_hit_width)) {
			return HIT_TYPE::HIT_P0 + 1;
		}
		if (hit_text_pt(t_pt, cp[0], m_hit_width)) {
			return HIT_TYPE::HIT_P0 + 0;
		}
		if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND)) {
			if (pt_in_circle(t_pt.x, t_pt.y, ew)) {
				return HIT_TYPE::HIT_STROKE;
			}
			if (pt_in_circle(t_pt, cp[3], ew)) {
				return HIT_TYPE::HIT_STROKE;
			}
		}
		else if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE)) {
			if (bezi_hit_test_cap<D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE>(t_pt, cp, m_lineto.data(), ew)) {
				return HIT_TYPE::HIT_STROKE;
			}
		}
		else if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE)) {
			if (bezi_hit_test_cap<D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE>(t_pt, cp, m_lineto.data(), ew)) {
				return HIT_TYPE::HIT_STROKE;
			}
		}
		// 最初の制御点の組をプッシュする.
		// ４つの点のうち端点は, 次につまれる組と共有するので, 1 + 3 * D_MAX 個の配列を確保する.
		constexpr int32_t D_MAX = 64;	// 分割する深さの最大値
		POINT_2D s[1 + D_MAX * 3] {};	// 制御点のスタック
		int32_t s_cnt = 0;	// スタックに積まれた点の数
		s[0] = cp[0];
		s[1] = cp[1];
		s[2] = cp[2];
		s[3] = cp[3];
		s_cnt += 4;
		// スタックに 一組以上の制御点が残っているか判定する.
		while (s_cnt >= 4) {
			// 制御点をポップする.
			POINT_2D c[10];
			c[3] = s[s_cnt - 1];
			c[2] = s[s_cnt - 2];
			c[1] = s[s_cnt - 3];
			c[0] = s[s_cnt - 4];
			// 端点は共有なのでピークする;
			s_cnt -= 4 - 1;
			// 制御点の組から凸包 c0 を得る (実際は方形で代用する).
			// 制御点の組から, 重複するものを除いた点の集合を得る.
			POINT_2D c0_lt = c[0];	// 凸包 c0 (を含む方形の左上点)
			POINT_2D c0_rb = c[0];	// 凸包 c0 (を含む方形の右下点)
			POINT_2D d[4];	// 重複しない点の集合.
			uint32_t d_cnt = 0;	// 重複しない点の集合の要素数
			d[d_cnt++] = c[0];
			for (uint32_t i = 1; i < 4; i++) {
				if (d[d_cnt - 1] != c[i]) {
					d[d_cnt++] = c[i];
					c[i].exp(c0_lt, c0_rb);
				}
			}
			// 重複しない点の集合の要素数が 2 未満か判定する.
			if (d_cnt < 2) {
				// 制御点の組は 1 点に集まっているので, 中断する.
				continue;
			}

			// 拡張・延長された線分を得る.
			//   e[i][0]             e[i][1]
			//        + - - - - - - - - - +
			//        |           orth|
			//   d[i] +---------------+---> dir
			//        |           d[i+1]
			//        + - - - - - - - - - +
			//   e[i][3]             e[i][2]
			POINT_2D e[3 * 4];	// 拡張・延長された線分の配列
			for (uint32_t i = 0, j = 0; i < d_cnt - 1; i++, j += 4) {
				auto e_dir = d[i + 1] - d[i];	// 次の頂点への位置ベクトル
				e_dir = e_dir * (ew / std::sqrt(e_dir * e_dir));
				const POINT_2D e_orth{ e_dir.y, -e_dir.x };	// 線分の直交ベクトル

				// 法線ベクトルにそって正逆の方向に線分を拡張する.
				e[j + 0] = d[i] + e_orth;
				e[j + 1] = d[i + 1] + e_orth;
				e[j + 2] = d[i + 1] - e_orth;
				e[j + 3] = d[i] - e_orth;
				if (i > 0) {
					// 最初の制御点以外は, 線分ベクトルの方向に延長する.
					e[j + 0] = e[j + 0] - e_dir;
					e[j + 3] = e[j + 3] - e_dir;
				}
				if (i + 1 < d_cnt - 1) {
					// 最後の制御点以外は, 線分ベクトルの逆方向に延長する.
					e[j + 1] = e[j + 1] + e_dir;
					e[j + 2] = e[j + 2] + e_dir;
				}
			}
			// 拡張・延長された線分から, 凸包 c1 を得る.
			uint32_t c1_cnt;
			POINT_2D c1[3 * 4];
			bezi_get_convex((d_cnt - 1) * 4, e, c1_cnt, c1);
			// 点が凸包 c1 に含まれないか判定する.
			if (!bezi_in_convex(t_pt.x, t_pt.y, c1_cnt, c1)) {
				// これ以上この制御点の組を分割する必要はない.
				// スタックに残った他の制御点の組を試す.
				continue;
			}

			// 凸包 c0 の大きさが 1 以下か判定する.
			POINT_2D c0 = c0_rb - c0_lt;
			if (c0.x <= 1.0 && c0.y <= 1.0) {
				// 現在の制御点の組 (凸包 c0) をこれ以上分割する必要はない.
				// 凸包 c1 は判定される点を含んでいるので, 図形の部位を返す.
				return HIT_TYPE::HIT_STROKE;
			}

			// スタックがオバーフローするか判定する.
			if (s_cnt + 6 > 1 + D_MAX * 3) {
				// 現在の制御点の組 (凸包 c0) をこれ以上分割することはできない.
				// 凸包 c1は判定される点を含んでいるので, 図形の部位を返す.
				return HIT_TYPE::HIT_STROKE;
			}

			// 制御点の組を 2 分割する.
			// c[0,1,2,3] の中点を c[4,5,6] に, c[4,5,6] の中点を c[7,8] に, c[7,8] の中点を c[9] に格納する.
			// 分割した制御点の組はそれぞれ c[0,4,7,9] と c[9,8,6,3] になる.
			c[4] = (c[0] + c[1]) * 0.5;
			c[5] = (c[1] + c[2]) * 0.5;
			c[6] = (c[2] + c[3]) * 0.5;
			c[7] = (c[4] + c[5]) * 0.5;
			c[8] = (c[5] + c[6]) * 0.5;
			c[9] = (c[7] + c[8]) * 0.5;
			if (fill_opa && !fill_test) {
				// 分割された凸包のあいだにできた三角形は, 塗りつぶしの領域.
				// この領域に点が含まれるか, 分割するたびに判定する.
				// ただし 1 度でも含まれるなら, それ以上の判定は必要ない.
				const POINT_2D tri[3]{ 
					c[0], c[9], c[3] 
				};
				fill_test = bezi_in_convex(t_pt.x, t_pt.y, 3, tri);
			}
			// 一方の組をプッシュする.
			// 始点 (0) はスタックに残っているので, 
			// 残りの 3 つの制御点をプッシュする.
			s[s_cnt] = c[4];
			s[s_cnt + 1] = c[7];
			s[s_cnt + 2] = c[9];
			// もう一方の組をプッシュする.
			// 始点 (9) はプッシュ済みなので,
			// 残りの 3 つの制御点をプッシュする.
			s[s_cnt + 3] = c[8];
			s[s_cnt + 4] = c[6];
			s[s_cnt + 5] = c[3];
			s_cnt += 6;
		}
		if (fill_opa && fill_test) {
			return HIT_TYPE::HIT_FILL;
		}
		return HIT_TYPE::HIT_SHEET;
	}

	//------------------------------
	// 矩形に含まれるか判定する.
	// 線の太さは考慮されない.
	// lt	範囲の左上位置
	// rb	範囲の右下位置
	// 戻り値	含まれるなら true
	//------------------------------
	bool ShapeBezier::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		// 計算精度がなるべく変わらないよう,
		// 範囲の左上が原点となるよう平行移動した制御点を得る.
		const double w = static_cast<double>(rb.x) - lt.x;
		const double h = static_cast<double>(rb.y) - lt.y;
		D2D1_POINT_2F cp[4];
		pt_sub(m_start, lt, cp[0]);
		pt_add(cp[0], m_lineto[0], cp[1]);
		pt_add(cp[1], m_lineto[1], cp[2]);
		pt_add(cp[2], m_lineto[2], cp[3]);
		// 最初の制御点の組をプッシュする.
		constexpr auto D_MAX = 52;	// 分割する最大回数
		POINT_2D s_arr[1 + D_MAX * 3] = {};
		int32_t s_cnt = 4;
		s_arr[0] = cp[0];
		s_arr[1] = cp[1];
		s_arr[2] = cp[2];
		s_arr[3] = cp[3];
		while (s_cnt >= 4) {
			// スタックが空でないなら, 制御点の組をポップする.
			// 端点は共有なのでピークする.
			POINT_2D c[10];
			c[3] = s_arr[s_cnt - 1];
			c[2] = s_arr[s_cnt - 2];
			c[1] = s_arr[s_cnt - 3];
			c[0] = s_arr[s_cnt - 4];
			s_cnt -= 3;
			// 始点が範囲の外にあるなら曲線は範囲に含まれない.
			if (c[0].x < 0.0 || w < c[0].x) {
				return false;
			}
			if (c[0].y < 0.0 || h < c[0].y) {
				return false;
			}
			// 終点が範囲の外にあるなら曲線は範囲に含まれない.
			if (c[3].x < 0.0 || w < c[3].x) {
				return false;
			}
			if (c[3].y < 0.0 || h < c[3].y) {
				return false;
			}
			// 他の 2 つの制御点が範囲内なら, 曲線のこの部分は範囲に含まれる.
			// さらに分割する必要はないので, スタックの残りの組について判定する.
			if (0.0 <= c[1].x && c[1].x <= w && 0.0 <= c[1].y && c[1].y <= h) {
				if (0.0 <= c[2].x && c[2].x <= w && 0.0 <= c[2].y && c[2].y <= h) {
					continue;
				}
			}
			// 制御点を含む領域を得る.
			POINT_2D b_lt = c[0];
			POINT_2D b_rb = c[0];
			c[1].exp(b_lt, b_rb);
			c[2].exp(b_lt, b_rb);
			c[3].exp(b_lt, b_rb);
			POINT_2D d = b_rb - b_lt;
			if (d.x <= 1.0 && d.y <= 1.0) {
				// 領域の各辺の大きさが 1 以下ならば, 
				// これ以上分割する必要はない.
				// 制御点の少なくとも 1 つが範囲に含まれてないのだから false を返す.
				return false;
			}
			if (s_cnt + 6 > 1 + D_MAX * 3) {
				// スタックオバーフローならこれ以上分割できない.
				// 制御点の少なくとも 1 つが範囲に含まれていないのなら false を返す.
				return false;
			}
			// 制御点の組を 2 分割する.
			c[4] = (c[0] + c[1]) * 0.5;
			c[5] = (c[1] + c[2]) * 0.5;
			c[6] = (c[2] + c[3]) * 0.5;
			c[7] = (c[4] + c[5]) * 0.5;
			c[8] = (c[5] + c[6]) * 0.5;
			c[9] = (c[7] + c[8]) * 0.5;
			// 一方の組をプッシュする.
			// 始点 (0) はスタックに残っているので, 
			// 残りの 3 つの制御点をプッシュする.
			s_arr[s_cnt] = c[4];
			s_arr[s_cnt + 1] = c[7];
			s_arr[s_cnt + 2] = c[9];
			// もう一方の組をプッシュする.
			// 始点 (9) はプッシュ済みなので,
			// 残りの 3 つの制御点をプッシュする.
			s_arr[s_cnt + 3] = c[8];
			s_arr[s_cnt + 4] = c[6];
			s_arr[s_cnt + 5] = c[3];
			s_cnt += 6;
		}
		return true;
	}

	// 図形を作成する.
	// start	始点
	// end_to	終点への位置ベクトル
	// prop	属性
	ShapeBezier::ShapeBezier(const D2D1_POINT_2F start, const D2D1_POINT_2F end_to, const SHAPE* prop) :
		SHAPE_PATH::SHAPE_PATH(prop, false)
	{
		m_start = start;
		m_lineto.resize(3);
		m_lineto.shrink_to_fit();
		m_lineto[0] = D2D1_POINT_2F{ end_to.x , 0.0f };
		m_lineto[1] = D2D1_POINT_2F{ -end_to.x , end_to.y };
		m_lineto[2] = D2D1_POINT_2F{ end_to.x , 0.0f };
	}

	//------------------------------
	// データリーダーから図形を読み込む.
	// dt_reader	データリーダー
	//------------------------------
	ShapeBezier::ShapeBezier(DataReader const& dt_reader) :
		SHAPE_PATH::SHAPE_PATH(dt_reader)
	{}

	void ShapeBezier::write(const DataWriter& dt_writer) const
	{
		SHAPE_PATH::write(dt_writer);
	}

	/*
	static uint32_t clipping(const POINT_2D& p, const POINT_2D& q, double* t)
	{
		POINT_2D bz[10];
		POINT_2D br[2];
		POINT_2D pr[2];

		uint32_t t_cnt;
		POINT_2D br_mid;
		POINT_2D pr_next;
		double dist;
		POINT_2D pq;
		POINT_2D pb;
		double a, b, c, d;
		double f0, f1, f2, f3;
		double s;
		double t_min, t_max;
		std::list<POINT_2D> st;

		// 計算精度をなるべく一定にするため, ベジェ制御点 bz を, その始点が原点となるよう平行移動する.
		bz[0] = 0.0;
		bz[1].x = m_pos.x;
		bz[1].y = m_pos.y;
		bz[2].x = bz[1].x + m_pos_1.x;
		bz[2].y = bz[1].y + m_pos_1.y;
		bz[3].x = bz[2].x + m_pos_2.x;
		bz[3].y = bz[2].y + m_pos_2.y;
		// 線分 pq に外接する方形 pr を求める.
		pb.x = p.x - m_start.x;
		pb.y = p.y - m_start.y;
		bezi_bound(p, q, pr);
		pr[0].x -= m_start.x;
		pr[0].y -= m_start.y;
		pr[1].x -= m_start.x;
		pr[1].y -= m_start.y;
		pr_next = pr[0].nextafter(1.0);
		pb = p - m_start;
		pq = q - pb;
		a = pq.y / pq.x;
		b = -1.0;
		c = -a * pb.x + pb.y;
		d = std::sqrt(a * a + b * b);
		// 0 を助変数の個数に格納する.
		t_cnt = 0;
		// ベジェ制御点 b をスタックにプッシュする.
		bezi_push(st, bz);
		do {
			bezi_pop(st, bz);
			bezi_bound(bz, br);
			// 方形 br は方形 pr の少なくとも一部と重なる ?
			if (pr[1].x >= br[0].x && pr[0].x <= br[1].x
				&& pr[1].y >= br[0].y && pr[0].y <= br[1].y) {
				if (!bezi_dividable(br, br_mid)) {
					// 線分 pq は y 軸にほぼ平行 ?
					if (pr[1].x < pr_next.x) {
						dist = br_mid.x - p.x;
					}
					// 線分 pq は x 軸にほぼ平行 ?
					else if (pr[1].y < pr_next.y) {
						// 中点と直線 pq の Y 成分の差分を長さに格納する.
						dist = br_mid.y - p.y;
					}
					else {
						// 中点と直線 pq の長さを求める.
						dist = bezi_shortest_dist(br_mid, a, b, c, d);
					}
					if (dist < 1.0) {
						s = nearest(pb, pq, br_mid);
						if (s >= 0.0 && s <= 1.0) {
							if (t != nullptr) {
								t[t_cnt] = s;
							}
							t_cnt++;
						}
					}
				}
				else {
					// 線分 pq は y 軸にほぼ平行 ?
					if (pr[1].x < pr_next.x) {
						f0 = bz[0].x - pb.x;
						f1 = bz[1].x - pb.x;
						f2 = bz[2].x - pb.x;
						f3 = bz[3].x - pb.x;
					}
					// 線分 pq は x 軸にほぼ平行 ?
					else if (pr[1].y < pr_next.y) {
						// 中点と直線 pq の Y 成分の差分を長さに格納する.
						f0 = bz[0].y - pb.y;
						f1 = bz[1].y - pb.y;
						f2 = bz[2].y - pb.y;
						f3 = bz[3].y - pb.y;
					}
					else {
						f0 = bezi_shortest_dist(bz[0], a, b, c, d);
						f1 = bezi_shortest_dist(bz[1], a, b, c, d);
						f2 = bezi_shortest_dist(bz[2], a, b, c, d);
						f3 = bezi_shortest_dist(bz[3], a, b, c, d);
					}
					// 凸包 f の少なくとも 1 つは 0 以下 ?
					// (変換された凸包は t 軸と交わる ?)
					if ((f0 <= 0.0
						|| f1 <= 0.0
						|| f2 <= 0.0
						|| f3 <= 0.0)
						// 凸包 f の少なくとも 1 つは 0以上 ?
						&& (f0 >= 0.0
							|| f1 >= 0.0
							|| f2 >= 0.0
							|| f3 >= 0.0)) {
						// 1 を区間の最小値 tMin に, 0 を最大値 tMax に格納する.
						t_min = T3;
						t_max = T0;
						// 凸包と t 軸の交点を求め, 区間 tMin,tMax を更新.
						segment(T0, f0, T1, f1, t_min, t_max);
						segment(T0, f0, T2, f2, t_min, t_max);
						segment(T0, f0, T3, f3, t_min, t_max);
						segment(T1, f1, T2, f2, t_min, t_max);
						segment(T1, f1, T3, f3, t_min, t_max);
						segment(T2, f2, T3, f3, t_min, t_max);
						// 区間の差分は 0.5 以上 ?
						if (t_max - t_min >= 0.5f) {
							// 曲線を半分に分割し, 2 組の制御点を求める.
							bz[4] = (bz[0] + bz[1]) * 0.5;
							bz[5] = (bz[1] + bz[2]) * 0.5;
							bz[6] = (bz[2] + bz[3]) * 0.5;
							bz[7] = (bz[4] + bz[5]) * 0.5;
							bz[8] = (bz[5] + bz[6]) * 0.5;
							bz[9] = (bz[7] + bz[8]) * 0.5;
							// 一方の組の制御点をスタックにプッシュする.
							bezi_push(st, bz, 0, 4, 7, 9);
							// もう一方の組の制御点をスタックにプッシュする.
							bezi_push(st, bz, 9, 8, 6, 3);
						}
						// 区間の差分はほぼ 0 ?
						// tMax は tMin の浮動小数で表現可能な次の数より小 ?
						else if (t_max <= std::nextafter(t_min, t_min + 1.0)) {
							// 区間の収斂した値をもちいて曲線上の点 bzP を求める.
							POINT_2D tp;
							bezi_point_by_param(bz, t_min, tp);
							/* 直線 pq 上にあって, 点 bzP に最も近い点の助変数を得る.
							s = nearest(pb, pq, tp);
							// 助変数は 0 以上 1 以下 ?
							// (点は線分上にある ?)
							if (s >= 0.0 && s <= 1.0) {
								// 配列 t は NULL でない ?
								if (t != nullptr) {
									// 助変数を配列の tCnt 番目に格納する.
									t[t_cnt] = s;
								}
								// 助変数の個数 tCnt をインクリメントする.
								t_cnt++;
							}
						}
						/* 最大値 tMax はほぼ 0 より大 ?
						else if (t_max > DBL_MIN) {
							// 最大値 tMax 以下の制御点を求める.
							bz[4] = bz[0] + (bz[1] - bz[0]) * t_max;
							bz[5] = bz[1] + (bz[2] - bz[1]) * t_max;
							bz[6] = bz[2] + (bz[3] - bz[2]) * t_max;
							bz[7] = bz[4] + (bz[5] - bz[4]) * t_max;
							bz[8] = bz[5] + (bz[6] - bz[5]) * t_max;
							bz[9] = bz[7] + (bz[8] - bz[7]) * t_max;
							// 最小値 tMin を最大値 tMax で除して補正する.
							t_min /= t_max;
							// 補正した tMin 以上の制御点を求める.
							bz[1] = bz[0] + (bz[4] - bz[0]) * t_max;
							bz[2] = bz[4] + (bz[7] - bz[4]) * t_max;
							bz[3] = bz[7] + (bz[9] - bz[7]) * t_max;
							bz[5] = bz[1] + (bz[2] - bz[1]) * t_max;
							bz[6] = bz[2] + (bz[3] - bz[2]) * t_max;
							bz[8] = bz[5] + (bz[6] - bz[5]) * t_max;
							// 求めた制御点をスタックにプッシュする.
							bezi_push(st, bz, 8, 6, 3, 9);
						}
					}
				}
			}
		} while (!st.empty());
		return t_cnt;
	}
	*/

	// 点と直線の間の最短距離を求める.
	/*
	static double bezi_shortest_dist(const POINT_2D& p, const double a, const double b, const double c, const double d) noexcept
	{
		return (a * p.x + b * p.y + c) / d;
	}
	*/

	// 二点を囲む方形を得る.
	/*
	static void bezi_bound(const POINT_2D& p, const POINT_2D& q, POINT_2D r[2])
	{
		if (p.x < q.x) {
			r[0].x = p.x;
			r[1].x = q.x;
		}
		else {
			r[0].x = q.x;
			r[1].x = p.x;
		}
		if (p.y < q.y) {
			r[0].y = p.y;
			r[1].y = q.y;
		}
		else {
			r[0].y = q.y;
			r[1].y = p.y;
		}
	}
	*/

	// 四つの点を囲む方形を得る.
	/*
	static void bezi_bound(const POINT_2D bz[4], POINT_2D br[2])
	{
		bezi_bound(bz[0], bz[1], br);
		br[0].x = min(br[0].x, bz[2].x);
		br[0].y = min(br[0].y, bz[2].y);
		br[0].x = min(br[0].x, bz[3].x);
		br[0].y = min(br[0].y, bz[3].y);
		br[1].x = max(br[1].x, bz[2].x);
		br[1].y = max(br[1].y, bz[2].y);
		br[1].x = max(br[1].x, bz[3].x);
		br[1].y = max(br[1].y, bz[3].y);
	}
	*/

}