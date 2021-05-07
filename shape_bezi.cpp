//------------------------------
// Shape_bezi.cpp
// ベジェ曲線
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// 折れ線の頂点の配列
	constexpr uint32_t ANCH_BEZI[4]{
		ANCH_WHICH::ANCH_P3,//ANCH_R_SE,
		ANCH_WHICH::ANCH_P2,//ANCH_R_SW,
		ANCH_WHICH::ANCH_P1,//ANCH_R_NE
		ANCH_WHICH::ANCH_P0	//ANCH_WHICH::ANCH_R_NW
	};

	// セグメントの区切る助変数の値
	constexpr double T0 = 0.0;
	constexpr double T1 = 1.0 / 3.0;
	constexpr double T2 = 2.0 / 3.0;
	constexpr double T3 = 1.0;

	// シンプソン法の回数
	constexpr uint32_t SIMPSON_N = 30;

	// double 型の値をもつ位置
	// ShapeBase 内でのみ使用する.
	struct BZP {
		double x;
		double y;

		inline double dist(const double a, const double b, const double c, const double d) const noexcept { return (a * x + b * y + c) / d; }
		inline void inc(BZP& r_min, BZP& r_max) const noexcept
		{
			if (x < r_min.x) {
				r_min.x = x;
			}
			if (x > r_max.x) {
				r_max.x = x;
			}
			if (y < r_min.y) {
				r_min.y = y;
			}
			if (y > r_max.y) {
				r_max.y = y;
			}
		}
		inline BZP nextafter(const double d) const noexcept { return { std::nextafter(x, x + d), std::nextafter(y, y + d) }; }
		inline operator D2D1_POINT_2F(void) const noexcept { return { static_cast<FLOAT>(x), static_cast<FLOAT>(y) }; }
		inline BZP operator -(const BZP& q) const noexcept { return { x - q.x, y - q.y }; }
		inline BZP operator -(const D2D1_POINT_2F q) const noexcept { return { x - q.x, y - q.y }; }
		inline BZP operator -(void) const noexcept { return { -x, -y }; }
		inline BZP operator *(const double s) const noexcept { return { x * s, y * s }; }
		inline double operator *(const BZP& q) const noexcept { return x * q.x + y * q.y; }
		inline BZP operator +(const BZP& q) const noexcept { return { x + q.x, y + q.y }; }
		inline BZP operator +(const D2D1_POINT_2F p) const noexcept { return { x + p.x, y + p.y }; }
		inline bool operator <(const BZP& q) const noexcept { return x < q.x && y < q.y; }
		inline BZP operator =(const D2D1_POINT_2F p) noexcept { return { x = p.x, y = p.y }; }
		inline BZP operator =(const double s) noexcept { return { x = s, y = s }; }
		inline bool operator >(const BZP& q) const noexcept { return x > q.x&& y > q.y; }
	};

	// 長さから助変数を得る.
	static double bz_by_length(const BZP bz[4], const double len) noexcept;

	// 曲線上の点を求める.
	static void bz_by_param(const BZP bz[4], const double t, BZP& p) noexcept;

	// 助変数をもとに曲線上の微分値を求める.
	static double bz_derivative(const BZP bz[4], const double t) noexcept;

	// 曲線上の区間の長さを得る.
	static double bz_simpson(const BZP bz[4], const double t_min, const double t_max, const uint32_t sim_n) noexcept;

	// 2 つの助変数が区間0-1の間で正順か調べる.
	static bool bz_test_param(const double t_min, const double t_max) noexcept;

	// 助変数から曲線上の接線ベクトルを求める.
	static void bz_tvec_by_param(const BZP bz[4], const double t, BZP& t_vec) noexcept;

	// 矢じりの両端点を計算する.
	static bool bz_calc_arrowhead(const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_SIZE a_size, D2D1_POINT_2F barbs[3]) noexcept;

	// 曲線のジオメトリシンクに矢じりを追加する.
	static void bz_create_arrow_geometry(ID2D1Factory3* factory, const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_STYLE a_style, const ARROW_SIZE a_size, ID2D1PathGeometry** a_geo);

	// 二点を囲む方形を得る.
	/*
	static void bz_bound(const BZP& p, const BZP& q, BZP r[2])
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
	static void bz_bound(const BZP bz[4], BZP br[2])
	{
		bz_bound(bz[0], bz[1], br);
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

	// 長さから助変数を得る.
	// bz	曲線
	// len	長さ
	// 戻り値	得られた助変数の値
	static double bz_by_length(const BZP bz[4], const double len) noexcept
	{
		double t;	// 助変数
		double d;	// 助変数の変分
		double e;	// 誤差

		/* 区間の中間値 0.5 を助変数に格納する. */
		t = 0.5;
		/* 0.25 を助変数の変分に格納する. */
		/* 助変数の変分は 0.001953125 以上 ? */
		for (d = 0.25; d >= 0.001953125; d *= 0.5) {
			/* 0-助変数の範囲を合成シンプソン公式で積分し, 曲線の長さを求める. */
			/* 求めた長さと指定された長さの差分を誤差に格納する. */
			e = bz_simpson(bz, 0.0, t, SIMPSON_N) - len;
			/* 誤差の絶対は 0.125 より小 ? */
			if (fabs(e) < 0.125) {
				break;
			}
			/* 誤差は 0 より大 ? */
			else if (e > 0.0) {
				/* 変分を助変数から引く. */
				t -= d;
			}
			else {
				/* 変分を助変数に足す. */
				t += d;
			}
		}
		return t;
	}

	// ベジェ曲線上の位置を求める.
	static void bz_by_param(const BZP bz[4], const double t, BZP& p) noexcept
	{
		const double s = 1.0 - t;
		const double ss = s * s;
		const double tt = t * t;
		p = bz[0] * s * ss
			+ bz[1] * 3.0 * ss * t
			+ bz[2] * 3.0 * s * tt
			+ bz[3] * t * tt;
	}

	// 矢じりの両端点を計算する.
	// b_pos	曲線の開始位置
	// b_seg	曲線の制御点
	// a_size	矢じりの寸法
	// barbs[3]	計算された両端点と先端点
	static bool bz_calc_arrowhead(const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_SIZE a_size, D2D1_POINT_2F barbs[3]) noexcept
	{
		BZP pos = {};
		BZP seg[3] = {};
		BZP bz[4] = {};

		pos = b_pos;
		seg[0] = b_seg.point1;
		seg[1] = b_seg.point2;
		seg[2] = b_seg.point3;
		// 座標値による誤差を少なくできる, と思われるので,
		// ベジェ曲線を始点が原点となるように平行移動.
		bz[3] = 0.0;
		bz[2] = seg[0] - pos;
		bz[1] = seg[1] - pos;
		bz[0] = seg[2] - pos;
		auto b_len = bz_simpson(bz, 0.0, 1.0, SIMPSON_N);
		if (b_len > FLT_MIN) {
			// 矢じりの先端のオフセット, または曲線の長さ, 
			// どちらか短い方で, 助変数を得る.
			const auto t = bz_by_length(bz, min(b_len, a_size.m_offset));
			// 助変数をもとに曲線の接線ベクトルを得る.
			BZP t_vec;
			bz_tvec_by_param(bz, t, t_vec);
			// 接線ベクトルを軸とする矢じりの返しの位置を計算する
			get_arrow_barbs(-t_vec,
				/*D2D1_POINT_2F(-t_vec),*/ sqrt(t_vec * t_vec), a_size.m_width, a_size.m_length, barbs);
			// 助変数で曲線上の位置を得る.
			BZP t_pos;	// 終点を原点とする, 矢じりの先端の位置
			bz_by_param(bz, t, t_pos);
			// 曲線上の位置を矢じりの先端とし, 返しの位置も並行移動する.
			pt_add(barbs[0], t_pos.x, t_pos.y, barbs[0]);
			pt_add(barbs[1], t_pos.x, t_pos.y, barbs[1]);
			barbs[2] = t_pos;
			//barbs[2].x = static_cast<FLOAT>(t_pos.x);
			//barbs[2].y = static_cast<FLOAT>(t_pos.y);
			pt_add(barbs[0], b_pos, barbs[0]);
			pt_add(barbs[1], b_pos, barbs[1]);
			pt_add(barbs[2], b_pos, barbs[2]);
			return true;
		}
		return false;
	}

	// 曲線のジオメトリシンクに矢じりを追加する
	// factory	D2D ファクトリ
	// b_pos	曲線の開始位置
	// b_seg	曲線の制御点
	// a_style	矢じりの種別
	// a_size	矢じりの寸法
	// a_geo	矢じりが追加されたジオメトリ
	static void bz_create_arrow_geometry(ID2D1Factory3* factory, const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_STYLE a_style, const ARROW_SIZE a_size, ID2D1PathGeometry** a_geo)
	{
		D2D1_POINT_2F barbs[3];	// 矢じりの返しの端点	
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (bz_calc_arrowhead(b_pos, b_seg, a_size, barbs)) {
			// ジオメトリシンクに追加する.
			winrt::check_hresult(
				factory->CreatePathGeometry(a_geo)
			);
			winrt::check_hresult(
				(*a_geo)->Open(sink.put())
			);
			sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(
				barbs[0],
				a_style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN_HOLLOW
			);
			sink->AddLine(barbs[2]);
			sink->AddLine(barbs[1]);
			sink->EndFigure(
				a_style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END_OPEN
			);
			sink->Close();
			sink = nullptr;
		}
	}

	// 助変数をもとにベジェ曲線上の微分値を求める.
	static double bz_derivative(const BZP bz[4], const double t) noexcept
	{
		BZP t_vec;

		// 助変数をもとにベジェ曲線上の接線ベクトルを求め,
		// その接線ベクトルの長さを返す.
		bz_tvec_by_param(bz, t, t_vec);
		return sqrt(t_vec * t_vec);
	}

	// 方形が分割可能かどうか判定し, その重心点を返す.
	/*
	static bool bz_dividable(const BZP r[2], BZP& mid)
	{
		mid = (r[0] + r[1]) * 0.5;
		return r[0] < mid.nextafter(-1.0) && r[1] > mid.nextafter(1.0);
	}
	*/

	// スタックから取り除く
	/*
	static void bz_pop(std::list<BZP>& stack, BZP bz[4])
	{
		bz[3] = stack.back();
		stack.pop_back();
		bz[2] = stack.back();
		stack.pop_back();
		bz[1] = stack.back();
		stack.pop_back();
		bz[0] = stack.back();
		stack.pop_back();
	}
	*/

	// スタックに積む.
	// i0 = 0	最初に積む位置の添え字
	// i1 = 1	2 番目に積む位置の添え字
	// i2 = 2	3 番目に積む位置の添え字
	// i3 = 3	4 番目に積む位置の添え字
	/*
	static void bz_push(std::list<BZP>& stack, const BZP bz[4], const uint32_t i0 = 0, const uint32_t i1 = 1, const uint32_t i2 = 2, const uint32_t i3 = 3)
	{
		stack.push_back(bz[i0]);
		stack.push_back(bz[i1]);
		stack.push_back(bz[i2]);
		stack.push_back(bz[i3]);
	}
	*/

	// 曲線上の区間の長さを得る.
	static double bz_simpson(const BZP bz[4], const double t_min, const double t_max, const uint32_t sim_n) noexcept
	{
		double t_diff;
		uint32_t n;
		double h;
		double a, b;
		double t;
		double b0, b2;
		double s;

		/* 範囲の上限下限は正順かどうか調べる. */
		/* 正順 ? */
		if (bz_test_param(t_min, t_max)) {
			/* 範囲上限 tMax -範囲下限 tMin を差分 tDiff に格納する. */
			t_diff = t_max - t_min;
			/* 区間の分割数 simN と tDiff を乗算する. */
			/* その結果を切り上げて整数値する. */
			/* 整数値を区間の半数 n に格納する. */
			n = (int)std::ceil(t_diff * (double)sim_n);
			/* tDiff÷2n を階差 h に格納する. */
			h = t_diff / (2.0 * n);
			/* 0 を奇数番目の部分区間の合計値 a に格納する. */
			a = 0.0;
			/* 0 を偶数番目の部分区間の合計値 b に格納する. */
			b = 0.0;
			/* tMin+h を助変数 t に格納する. */
			t = t_min + h;
			/* 1 を添え字 i に格納する. */
			/* i は n より小 ? */
			for (uint32_t i = 1; i < n; i++) {
				/* 2i-1 番目の部分区間の微分値を求め, a に加える. */
				a += bz_derivative(bz, t);
				/* 階差 h を助変数 t に加える. */
				t += h;
				/* 2i 番目の部分区間の微分値を求め, b に加える. */
				b += bz_derivative(bz, t);
				/* 階差 h を助変数 t に加える. */
				t += h;
				/* i をインクリメントする. */
			}
			/* 2n-1 番目の部分区間の微分値を求め, a に加える. */
			a += bz_derivative(bz, t);
			/* 0 番目の部分区間での微分値を求め, b0 に格納する. */
			b0 = bz_derivative(bz, t_min);
			/* 2n 番目の部分区間での微分値を求め, b2 に格納する. */
			b2 = bz_derivative(bz, t_max);
			/* (b0+4×a+2×b+b2)×h/3 を求め, 積分値 s に格納する. */
			s = (b0 + 4.0 * a + 2.0 * b + b2) * h / 3.0f;
		}
		else {
			/* 0 を積分値 s に格納する. */
			s = 0.0;
		}
		/* s を返す. */
		return s;
	}

	// ベジェ曲線と方形が交わるかどうか, 曲線を分割し, 調べる.
	/*
	static bool bz_splitting(const BZP BZ[4], const BZP pr[2])
	{
		BZP bz[10];
		BZP br[2];
		BZP br_mid;
		std::list<BZP> st;

		bz[0] = BZ[0];
		bz[1] = BZ[1];
		bz[2] = BZ[2];
		bz[3] = BZ[3];
		bz_push(st, bz);
		do {
			bz_pop(st, bz);
			bz_bound(bz, br);
			if (pr[0].x <= br[0].x && br[1].x <= pr[1].x
				&& pr[0].y <= br[0].y && br[1].y <= pr[1].y) {
				return true;
			}
			// 方形 br は方形 pr の少なくとも一部と重なる ?
			else if (br[1].x >= pr[0].x && br[1].y >= pr[0].y
				&& br[0].x <= pr[1].x && br[0].y <= pr[1].y) {
				if (bz_dividable(br, br_mid) != true) {
					return true;
				}
				bz[4] = (bz[0] + bz[1]) * 0.5;
				bz[5] = (bz[1] + bz[2]) * 0.5;
				bz[6] = (bz[2] + bz[3]) * 0.5;
				bz[7] = (bz[4] + bz[5]) * 0.5;
				bz[8] = (bz[5] + bz[6]) * 0.5;
				bz[9] = (bz[7] + bz[8]) * 0.5;
				// 分割されたベジェ制御点をスタックにプッシュする.
				bz_push(st, bz, 0, 4, 7, 9);
				bz_push(st, bz, 9, 8, 6, 3);
			}
		} while (st.empty() != true);
		return false;
	}
	*/

	// 2 つの助変数が区間0-1の間で正順か調べる.
	static bool bz_test_param(const double t_min, const double t_max) noexcept
	{
		return -DBL_MIN < t_min
			// 範囲の上限 tMax は 1+DBL_EPSILON より小 ?
			&& t_max < 1.0 + DBL_EPSILON
			// tMin より大きくて最も近い値は tMax より小 ?
			&& std::nextafter(t_min, t_min + 1.0) < t_max;
	}

	// 助変数から曲線上の接線ベクトルを求める.
	// bz	曲線
	// t	助変数
	// t_vec	接線ベクトル
	static void bz_tvec_by_param(const BZP bz[4], const double t, BZP& t_vec) noexcept
	{
		const double a = -3.0 * (1.0 - t) * (1.0 - t);
		const double b = 3.0 * (1.0 - t) * (1.0 - 3.0 * t);
		const double c = 3.0 * t * (2.0 - 3.0 * t);
		const double d = (3.0 * t * t);
		t_vec = bz[0] * a + bz[1] * b + bz[2] * c + bz[3] * d;
	}

	// パスジオメトリを作成する.
	void ShapeBezi::create_path_geometry(void)
	{
		D2D1_BEZIER_SEGMENT b_seg;
		winrt::com_ptr<ID2D1GeometrySink> sink;

		m_poly_geom = nullptr;
		m_arrow_geom = nullptr;
		pt_add(m_pos, m_diff[0], b_seg.point1);
		pt_add(b_seg.point1, m_diff[1], b_seg.point2);
		pt_add(b_seg.point2, m_diff[2], b_seg.point3);
		winrt::check_hresult(
			s_d2d_factory->CreatePathGeometry(m_poly_geom.put())
		);
		m_poly_geom->Open(sink.put());
		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		sink->BeginFigure(m_pos, D2D1_FIGURE_BEGIN_HOLLOW);
		sink->AddBezier(b_seg);
		sink->EndFigure(D2D1_FIGURE_END_OPEN);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			bz_create_arrow_geometry(s_d2d_factory,
				m_pos, b_seg, m_arrow_style, m_arrow_size,
				m_arrow_geom.put());
		}
		sink->Close();
		sink = nullptr;
	}

	// 図形を表示する.
	void ShapeBezi::draw(SHAPE_DX& dx)
	{
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F e_pos;

		if (is_opaque(m_stroke_color)) {
			const auto sw = static_cast<FLOAT>(m_stroke_width);
			auto sb = dx.m_shape_brush.get();
			auto ss = m_d2d_stroke_style.get();
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawGeometry(m_poly_geom.get(), sb, sw, ss);
			if (m_arrow_style != ARROW_STYLE::NONE) {
				auto geo = m_arrow_geom.get();
				dx.m_d2dContext->DrawGeometry(geo, sb, sw, nullptr);
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(geo, sb, nullptr);
				}
			}
		}
		if (is_selected() != true) {
			return;
		}
		D2D1_MATRIX_3X2_F tran;
		dx.m_d2dContext->GetTransform(&tran);
		const auto sw = static_cast<FLOAT>(1.0 / tran.m11);
		//auto sb = dx.m_anch_brush.get();
		//auto ss = dx.m_aux_style.get();

		anchor_draw_rect(m_pos, dx);
		s_pos = m_pos;
		pt_add(s_pos, m_diff[0], e_pos);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		anchor_draw_ellipse(e_pos, dx);

		s_pos = e_pos;
		pt_add(s_pos, m_diff[1], e_pos);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		anchor_draw_ellipse(e_pos, dx);

		s_pos = e_pos;
		pt_add(s_pos, m_diff[2], e_pos);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		anchor_draw_rect(e_pos, dx);
	}

	// 矢じりの寸法を得る.
	bool ShapeBezi::get_arrow_size(ARROW_SIZE& value) const noexcept
	{
		value = m_arrow_size;
		return true;
	}

	// 矢じりの形式を得る.
	bool ShapeBezi::get_arrow_style(ARROW_STYLE& value) const noexcept
	{
		value = m_arrow_style;
		return true;
	}

	// 位置を含むか調べる.
	// t_pos	調べる位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeBezi::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		// 計算精度がなるべく変わらないよう,
		// 調べる位置が原点となるよう平行移動し, 制御点を得る.
		D2D1_POINT_2F a_pos[4];
		pt_sub(m_pos, t_pos, a_pos[3]);
		pt_add(a_pos[3], m_diff[0], a_pos[2]);
		pt_add(a_pos[2], m_diff[1], a_pos[1]);
		pt_add(a_pos[1], m_diff[2], a_pos[0]);
		// 制御点の各部位について,
		for (uint32_t i = 0; i < 4; i++) {
			// 部位が位置を含むか調べる.
			if (pt_in_anch(a_pos[i], a_len)) {
				// 含むならその部位を返す.
				return ANCH_BEZI[i];
			}
		}
		const auto s_width = max(max(m_stroke_width, a_len) * 0.5, 0.5);
		// 最初の制御点の組をプッシュする.
		constexpr auto D_MAX = 52;	// 分割する最大回数
		BZP s_arr[1 + D_MAX * 3] = {};
		int32_t s_cnt = 4;
		s_arr[0] = a_pos[0];
		s_arr[1] = a_pos[1];
		s_arr[2] = a_pos[2];
		s_arr[3] = a_pos[3];
		while (s_cnt >= 4) {
			// スタックが空でないなら, 制御点の組をポップする.
			// 端点は共有なのでピークする.
			BZP b_pos[10];
			b_pos[3] = s_arr[s_cnt - 1];
			b_pos[2] = s_arr[s_cnt - 2];
			b_pos[1] = s_arr[s_cnt - 3];
			b_pos[0] = s_arr[s_cnt - 4];
			s_cnt -= 3;
			// 制御点の組に含まれる各点について,
			// まず, 最初と最後の制御点を含む凸包 b と q を得る.
			// 凸包 b は, 制御点の組が分割できるか調べるため使用する.
			// 凸包 q は, 線枠の太さを反映させ位置を含むか調べるため使用する.
			// 簡単にするため凸包でなく、方形で代用する.
			BZP b_min = b_pos[0];
			BZP b_max = b_pos[3];
			BZP q_min = b_pos[0];
			BZP q_max = b_pos[3];
			for (int i = 0; i < 3; i++) {
				// 制御点と次の制御点の間の差分を得る.
				BZP b_vec = b_pos[i + 1] - b_pos[i];
				//double v = b_vec.abs();
				const auto v = b_vec * b_vec;
				if (v <= FLT_MIN) {
					// 差分の長さが FLT_MIN 未満なら, 
					// 次の制御点は無視して凸包には加えない.
					continue;
				}
				// 次の制御点を凸包 b に加える.
				b_pos[i + 1].inc(b_min, b_max);
				// 差分と直行する法線ベクトルを得る.
				// 制御点と次の制御点を法線にそって正逆の両方向にずらす.
				// 得られた 4 点を凸包 q に加える.
				b_vec = b_vec * (s_width / std::sqrt(v));
				BZP n_vec = { b_vec.y, -b_vec.x };
				BZP q_pos;
				q_pos = b_pos[i] + n_vec;
				q_pos.inc(q_min, q_max);
				q_pos = b_pos[i + 1] + n_vec;
				q_pos.inc(q_min, q_max);
				q_pos = b_pos[i + 1] - n_vec;
				q_pos.inc(q_min, q_max);
				q_pos = b_pos[i] - n_vec;
				q_pos.inc(q_min, q_max);
			}
			if (q_min.x > 0.0 || 0.0 > q_max.x || q_min.y > 0.0 || 0.0 > q_max.y) {
				// 調べる位置が凸包 q に含まれないなら,
				// この制御点の組は位置を含まないので無視する.
				continue;
			}
			BZP d = b_max - b_min;
			if (d.x <= 1.0 && d.y <= 1.0) {
				// 凸包 b の各辺の大きさが 1 以下ならば,
				// 曲線は調べる位置を含むとみなす.
				// (これ以上分割しても結果に影響がない)
				// ANCH_FRAME を返す.
				return ANCH_WHICH::ANCH_FRAME;
			}
			// スタックがオバーフローするなら, スタックに積まれている
			// 他の制御点の組を調べる.
			if (s_cnt + 6 > 1 + D_MAX * 3) {
				continue;
			}
			// 制御点の組を 2 分割する.
			b_pos[4] = (b_pos[0] + b_pos[1]) * 0.5;
			b_pos[5] = (b_pos[1] + b_pos[2]) * 0.5;
			b_pos[6] = (b_pos[2] + b_pos[3]) * 0.5;
			b_pos[7] = (b_pos[4] + b_pos[5]) * 0.5;
			b_pos[8] = (b_pos[5] + b_pos[6]) * 0.5;
			b_pos[9] = (b_pos[7] + b_pos[8]) * 0.5;
			// 一方の組をプッシュする.
			// 始点 (0) はスタックに残っているので, 
			// 残りの 3 つの制御点をプッシュする.
			s_arr[s_cnt] = b_pos[4];
			s_arr[s_cnt + 1] = b_pos[7];
			s_arr[s_cnt + 2] = b_pos[9];
			// もう一方の組をプッシュする.
			// 始点 (9) はプッシュ済みなので,
			// 残りの 3 つの制御点をプッシュする.
			s_arr[s_cnt + 3] = b_pos[8];
			s_arr[s_cnt + 4] = b_pos[6];
			s_arr[s_cnt + 5] = b_pos[3];
			s_cnt += 6;
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// 範囲に含まれるか調べる.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeBezi::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		// 計算精度がなるべく変わらないよう,
		// 範囲の左上が原点となるよう平行移動した制御点を得る.
		const auto w = static_cast<double>(a_max.x) - a_min.x;
		const auto h = static_cast<double>(a_max.y) - a_min.y;
		D2D1_POINT_2F a_pos[4];
		pt_sub(m_pos, a_min, a_pos[0]);
		pt_add(a_pos[0], m_diff[0], a_pos[1]);
		pt_add(a_pos[1], m_diff[1], a_pos[2]);
		pt_add(a_pos[2], m_diff[2], a_pos[3]);
		// 最初の制御点の組をプッシュする.
		constexpr auto D_MAX = 52;	// 分割する最大回数
		BZP s_arr[1 + D_MAX * 3] = {};
		int32_t s_cnt = 4;
		s_arr[0] = a_pos[0];
		s_arr[1] = a_pos[1];
		s_arr[2] = a_pos[2];
		s_arr[3] = a_pos[3];
		while (s_cnt >= 4) {
			// スタックが空でないなら, 制御点の組をポップする.
			// 端点は共有なのでピークする.
			BZP b_pos[10];
			b_pos[3] = s_arr[s_cnt - 1];
			b_pos[2] = s_arr[s_cnt - 2];
			b_pos[1] = s_arr[s_cnt - 3];
			b_pos[0] = s_arr[s_cnt - 4];
			s_cnt -= 3;
			// 始点が範囲の外にあるなら曲線は範囲に含まれない.
			if (b_pos[0].x < 0.0 || w < b_pos[0].x) {
				return false;
			}
			if (b_pos[0].y < 0.0 || h < b_pos[0].y) {
				return false;
			}
			// 終点が範囲の外にあるなら曲線は範囲に含まれない.
			if (b_pos[3].x < 0.0 || w < b_pos[3].x) {
				return false;
			}
			if (b_pos[3].y < 0.0 || h < b_pos[3].y) {
				return false;
			}
			// 他の 2 つの制御点が範囲内なら, 曲線のこの部分は範囲に含まれる.
			// さらに分割する必要はないので, スタックの残りの組について調べる.
			if (0.0 <= b_pos[1].x && b_pos[1].x <= w && 0.0 <= b_pos[1].y && b_pos[1].y <= h) {
				if (0.0 <= b_pos[2].x && b_pos[2].x <= w && 0.0 <= b_pos[2].y && b_pos[2].y <= h) {
					continue;
				}
			}
			// 制御点を含む領域を得る.
			BZP b_min = b_pos[0];
			BZP b_max = b_pos[0];
			b_pos[1].inc(b_min, b_max);
			b_pos[2].inc(b_min, b_max);
			b_pos[3].inc(b_min, b_max);
			BZP d = b_max - b_min;
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
			b_pos[4] = (b_pos[0] + b_pos[1]) * 0.5;
			b_pos[5] = (b_pos[1] + b_pos[2]) * 0.5;
			b_pos[6] = (b_pos[2] + b_pos[3]) * 0.5;
			b_pos[7] = (b_pos[4] + b_pos[5]) * 0.5;
			b_pos[8] = (b_pos[5] + b_pos[6]) * 0.5;
			b_pos[9] = (b_pos[7] + b_pos[8]) * 0.5;
			// 一方の組をプッシュする.
			// 始点 (0) はスタックに残っているので, 
			// 残りの 3 つの制御点をプッシュする.
			s_arr[s_cnt] = b_pos[4];
			s_arr[s_cnt + 1] = b_pos[7];
			s_arr[s_cnt + 2] = b_pos[9];
			// もう一方の組をプッシュする.
			// 始点 (9) はプッシュ済みなので,
			// 残りの 3 つの制御点をプッシュする.
			s_arr[s_cnt + 3] = b_pos[8];
			s_arr[s_cnt + 4] = b_pos[6];
			s_arr[s_cnt + 5] = b_pos[3];
			s_cnt += 6;
		}
		return true;
	}

	// 矢じりの寸法に格納する.
	void ShapeBezi::set_arrow_size(const ARROW_SIZE& value)
	{
		if (equal(m_arrow_size, value)) {
			return;
		}
		m_arrow_size = value;
		create_path_geometry();
	}

	// 矢じりの形式に格納する.
	void ShapeBezi::set_arrow_style(const ARROW_STYLE value)
	{
		if (m_arrow_style == value) {
			return;
		}
		m_arrow_style = value;
		create_path_geometry();
	}

	// 値を始点に格納する. 他の部位の位置も動く.
	void ShapeBezi::set_start_pos(const D2D1_POINT_2F value)
	{
		ShapeStroke::set_start_pos(value);
		create_path_geometry();
	}

	// 図形を作成する.
	ShapeBezi::ShapeBezi(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeSheet* attr) :
		ShapePoly::ShapePoly(3, attr)
	{
		m_pos = s_pos;
		m_diff[0].x = diff.x;
		m_diff[0].y = 0.0f;
		m_diff[1].x = -diff.x;
		m_diff[1].y = diff.y;
		m_diff[2].x = diff.x;
		m_diff[2].y = 0.0f;
		m_arrow_style = attr->m_arrow_style;
		m_arrow_size = attr->m_arrow_size;
		create_path_geometry();
	}

	// 図形をデータリーダーから読み込む.
	ShapeBezi::ShapeBezi(DataReader const& dt_reader) :
		ShapePoly::ShapePoly(dt_reader)
	{
		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadUInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
		create_path_geometry();
	}

	// データライターに書き込む.
	void ShapeBezi::write(DataWriter const& dt_writer) const
	{
		//w.WriteInt32(static_cast<int32_t>(SHAPE_BEZI));
		ShapePoly::write(dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
	}

	// データライターに SVG タグとして書き込む.
	void ShapeBezi::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;
		D2D1_BEZIER_SEGMENT b_seg;

		pt_add(m_pos, m_diff[0], b_seg.point1);
		pt_add(b_seg.point1, m_diff[1], b_seg.point2);
		pt_add(b_seg.point2, m_diff[2], b_seg.point3);
		write_svg("<path d=\"", dt_writer);
		write_svg(m_pos, "M", dt_writer);
		write_svg(b_seg.point1, "C", dt_writer);
		write_svg(b_seg.point2, ",", dt_writer);
		write_svg(b_seg.point3, ",", dt_writer);
		write_svg("\" ", dt_writer);
		write_svg("none", "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[3];
			bz_calc_arrowhead(m_pos, b_seg, m_arrow_size, barbs);
			ShapeStroke::write_svg(barbs, barbs[2], m_arrow_style, dt_writer);
		}
	}

	/*
	static uint32_t clipping(
		const BZP& p, const BZP& q, double* t)
	{
		BZP bz[10];
		BZP br[2];
		BZP pr[2];

		uint32_t t_cnt;
		BZP br_mid;
		BZP pr_next;
		double dist;
		BZP pq;
		BZP pb;
		double a, b, c, d;
		double f0, f1, f2, f3;
		double s;
		double t_min, t_max;
		std::list<BZP> st;

		// 計算精度をなるべく一定にするため, ベジェ制御点 bz を, その始点が原点となるよう平行移動する.
		bz[0] = 0.0;
		bz[1].x = m_diff.x;
		bz[1].y = m_diff.y;
		bz[2].x = bz[1].x + m_diff_1.x;
		bz[2].y = bz[1].y + m_diff_1.y;
		bz[3].x = bz[2].x + m_diff_2.x;
		bz[3].y = bz[2].y + m_diff_2.y;
		// 線分 pq に外接する方形 pr を求める.
		pb.x = p.x - m_pos.x;
		pb.y = p.y - m_pos.y;
		bz_bound(p, q, pr);
		pr[0].x -= m_pos.x;
		pr[0].y -= m_pos.y;
		pr[1].x -= m_pos.x;
		pr[1].y -= m_pos.y;
		pr_next = pr[0].nextafter(1.0);
		pb = p - m_pos;
		pq = q - pb;
		a = pq.y / pq.x;
		b = -1.0;
		c = -a * pb.x + pb.y;
		d = std::sqrt(a * a + b * b);
		// 0 を助変数の個数に格納する.
		t_cnt = 0;
		// ベジェ制御点 b をスタックにプッシュする.
		bz_push(st, bz);
		do {
			bz_pop(st, bz);
			bz_bound(bz, br);
			// 方形 br は方形 pr の少なくとも一部と重なる ?
			if (pr[1].x >= br[0].x && pr[0].x <= br[1].x
				&& pr[1].y >= br[0].y && pr[0].y <= br[1].y) {
				if (bz_dividable(br, br_mid) != true) {
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
						dist = br_mid.dist(a, b, c, d);
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
						f0 = bz[0].dist(a, b, c, d);
						f1 = bz[1].dist(a, b, c, d);
						f2 = bz[2].dist(a, b, c, d);
						f3 = bz[3].dist(a, b, c, d);
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
							bz_push(st, bz, 0, 4, 7, 9);
							// もう一方の組の制御点をスタックにプッシュする.
							bz_push(st, bz, 9, 8, 6, 3);
						}
						// 区間の差分はほぼ 0 ?
						// tMax は tMin の浮動小数で表現可能な次の数より小 ?
						else if (t_max <= std::nextafter(t_min, t_min + 1.0)) {
							// 区間の収斂した値をもちいて曲線上の点 bzP を求める.
							BZP tp;
							bz_by_param(bz, t_min, tp);
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
							bz_push(st, bz, 8, 6, 3, 9);
						}
					}
				}
			}
		} while (st.empty() != true);
		return t_cnt;
	}
	*/

}