//------------------------------
// shape.cpp
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	// 図形が使用する D2D ファクトリへの参照.
	ID2D1Factory3* Shape::s_d2d_factory = nullptr;
	IDWriteFactory3* Shape::s_dwrite_factory = nullptr;	// DWRITE ファクトリ
#if defined(_DEBUG)
	uint32_t debug_leak_cnt = 0;
	uint32_t debug_shape_cnt = 0;
	uint32_t debug_deleted_cnt = 0;
#endif

	// 色の成分が同じか調べる.
	static bool equal_component(const FLOAT a, const FLOAT b) noexcept;

	// UWP のブラシを D2D1_COLOR_F に変換する.
	bool cast_to(const Brush& a, D2D1_COLOR_F& b) noexcept
	{
		using winrt::Windows::UI::Xaml::Media::SolidColorBrush;

		const auto s = a.try_as<SolidColorBrush>();
		if (s == nullptr) {
			return false;
		}
		const auto c = s.Color();
		b.r = static_cast<FLOAT>(static_cast<double>(c.R) / COLOR_MAX);
		b.g = static_cast<FLOAT>(static_cast<double>(c.G) / COLOR_MAX);
		b.b = static_cast<FLOAT>(static_cast<double>(c.B) / COLOR_MAX);
		b.a = static_cast<FLOAT>(static_cast<double>(c.A) / COLOR_MAX);
		return true;
	}

	// UWP のブラシを D2D1_COLOR_F に変換する.
	void cast_to(const Color& a, D2D1_COLOR_F& b) noexcept
	{
		b.r = static_cast<FLOAT>(static_cast<double>(a.R) / COLOR_MAX);
		b.g = static_cast<FLOAT>(static_cast<double>(a.G) / COLOR_MAX);
		b.b = static_cast<FLOAT>(static_cast<double>(a.B) / COLOR_MAX);
		b.a = static_cast<FLOAT>(static_cast<double>(a.A) / COLOR_MAX);
	}

	// 部位の方形を表示する.
	// a_pos	部位の位置
	// dx		図形の描画環境
	void anchor_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		D2D1_POINT_2F r_min;
		pt_add(a_pos, -0.5 * dx.m_anch_len, r_min);
		D2D1_POINT_2F r_max;
		pt_add(r_min, dx.m_anch_len, r_max);
		const D2D1_RECT_F r{ r_min.x, r_min.y, r_max.x, r_max.y };

		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawRectangle(r, dx.m_shape_brush.get(), 2.0, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->FillRectangle(r, dx.m_shape_brush.get());
	}

	// 丸い部位を表示する.
	// a_pos	部位の位置
	// dx		図形の描画環境
	void anchor_draw_rounded(const D2D1_POINT_2F& a_pos, SHAPE_DX& dx)
	{
		const FLOAT rad = static_cast<FLOAT>(dx.m_anch_len * 0.5 + 1.0);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->FillEllipse({ a_pos, rad, rad }, dx.m_shape_brush.get());
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->FillEllipse({ a_pos, rad - 1.0F, rad - 1.0F }, dx.m_shape_brush.get());
	}

	// 矢じりの寸法が同じか調べる.
	bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept
	{
		return equal(a.m_width, b.m_width) && equal(a.m_length, b.m_length) && equal(a.m_offset, b.m_offset);
	}

	// 矢じりの形式が同じか調べる.
	bool equal(const ARROW_STYLE a, const ARROW_STYLE b) noexcept
	{
		return a == b;
	}

	// ブール値が同じか調べる.
	bool equal(const bool a, const bool b) noexcept
	{
		return a == b;
	}

	// 色が同じか調べる.
	bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept
	{
		return equal_component(a.a, b.a)
			&& equal_component(a.r, b.r)
			&& equal_component(a.g, b.g)
			&& equal_component(a.b, b.b);
	}

	// 破線の形式が同じか調べる.
	bool equal(const D2D1_DASH_STYLE a, const D2D1_DASH_STYLE b) noexcept
	{
		return a == b;
	}

	// 位置が同じか調べる.
	bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept
	{
		return equal(a.x, b.x) && equal(a.y, b.y);
	}

	// 寸法が同じか調べる.
	bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept
	{
		return equal(a.width, b.width) && equal(a.height, b.height);
	}

	// 倍精度浮動小数が同じか調べる.
	bool equal(const double a, const double b) noexcept
	{
		return fabs(a - b) <= FLT_EPSILON * fmax(1.0, fmax(fabs(a), fabs(b)));
	}

	// 書体の伸縮が同じか調べる.
	bool equal(const DWRITE_FONT_STRETCH a, const DWRITE_FONT_STRETCH b) noexcept
	{
		return a == b;
	}

	// 書体の字体が同じか調べる.
	bool equal(const DWRITE_FONT_STYLE a, const DWRITE_FONT_STYLE b) noexcept
	{
		return a == b;
	}

	// 書体の太さが同じか調べる.
	bool equal(const DWRITE_FONT_WEIGHT a, const DWRITE_FONT_WEIGHT b) noexcept
	{
		return a == b;
	}

	// 段落の整列が同じか調べる.
	bool equal(const DWRITE_PARAGRAPH_ALIGNMENT a, const DWRITE_PARAGRAPH_ALIGNMENT b) noexcept
	{
		return a == b;
	}

	// 文字列の整列が同じか調べる.
	bool equal(const DWRITE_TEXT_ALIGNMENT a, const DWRITE_TEXT_ALIGNMENT b) noexcept
	{
		return a == b;
	}

	// 文字範囲が同じか調べる.
	bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept
	{
		return a.startPosition == b.startPosition && a.length == b.length;
	}

	// 単精度浮動小数が同じか調べる.
	bool equal(const float a, const float b) noexcept
	{
		return fabs(a - b) <= FLT_EPSILON * fmax(1.0f, fmax(fabs(a), fabs(b)));
	}

	// 方眼の形式が同じか調べる.
	bool equal(const GRID_PATT a, const GRID_PATT b) noexcept
	{
		return a == b;
	}

	// 方眼の表示が同じか調べる.
	bool equal(const GRID_SHOW a, const GRID_SHOW b) noexcept
	{
		return a == b;
	}

	// 破線の配置が同じか調べる.
	bool equal(const STROKE_PATT& a, const STROKE_PATT& b) noexcept
	{
		return equal(a.m_[0], b.m_[0])
			&& equal(a.m_[1], b.m_[1])
			&& equal(a.m_[2], b.m_[2])
			&& equal(a.m_[3], b.m_[3])
			&& equal(a.m_[4], b.m_[4])
			&& equal(a.m_[5], b.m_[5]);
	}

	// 整数が同じか調べる.
	bool equal(const uint32_t a, const uint32_t b) noexcept
	{
		return a == b;
	}

	// 文字列が同じか調べる.
	bool equal(const wchar_t* a, const wchar_t* b) noexcept
	{
		return a == b || (a != nullptr && b != nullptr && wcscmp(a, b) == 0);
	}

	// 文字列が同じか調べる.
	bool equal(winrt::hstring const& a, const wchar_t* b) noexcept
	{
		return a == (b == nullptr ? L"" : b);
	}

	// 色の成分が同じか調べる.
	static bool equal_component(const FLOAT a, const FLOAT b) noexcept
	{
		return fabs(b - a) < 1.0f / 128.0f;
	}

	// 矢じりの軸と寸法をもとに返しの位置を計算する.
	// axis	矢じりの軸ベクトル.
	// axis_len	軸ベクトルの長さ
	// barb_width	矢じりの幅 (返しの両端の長さ)
	// head_len	矢じりの長さ (先端から返しまでの軸ベクトル上での長さ)
	// barbs	計算された矢じりの返しの位置 (先端からのオフセット)
	void get_arrow_barbs(const D2D1_POINT_2F axis, const double axis_len, const double barb_width, const double head_len, D2D1_POINT_2F barbs[]) noexcept
	{
		if (axis_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z = { 0.0f, 0.0f };
			barbs[0] = Z;
			barbs[1] = Z;
		}
		else {
			const double hf = barb_width * 0.5;
			const double sx = axis.x * -head_len;
			const double sy = axis.x * hf;
			const double tx = axis.y * -head_len;
			const double ty = axis.y * hf;
			const double ax = 1.0 / axis_len;
			barbs[0].x = static_cast<FLOAT>((sx - ty) * ax);
			barbs[0].y = static_cast<FLOAT>((tx + sy) * ax);
			barbs[1].x = static_cast<FLOAT>((sx + ty) * ax);
			barbs[1].y = static_cast<FLOAT>((tx - sy) * ax);
		}
	}

	// 色が不透明か調べる.
	// a	調べる色
	// 戻り値	不透明の場合 true
	bool is_opaque(const D2D1_COLOR_F& a) noexcept
	{
		const auto aa = static_cast<uint32_t>(round(a.a * 255.0f)) & 0xff;
		return aa > 0;
	}

	// ベクトルの長さ (の自乗値) を得る
	// a	ベクトル
	// 戻り値	長さ (の自乗値) 
	double pt_abs2(const D2D1_POINT_2F a) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		return ax * ax + ay * ay;
	}

	// 位置を位置に足す
	void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x + b.x;
		c.y = a.y + b.y;
	}

	// スカラー値を位置に足す
	void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		pt_add(a, b, b, c);
	}

	// 2 つの値を位置に足す
	void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& b) noexcept
	{
		b.x = static_cast<FLOAT>(a.x + x);
		b.y = static_cast<FLOAT>(a.y + y);
	}

	// 二点間の中点を得る.
	void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		c.x = static_cast<FLOAT>((ax + b.x) * 0.5);
		c.y = static_cast<FLOAT>((ay + b.y) * 0.5);
	}

	// 二点で囲まれた方形を得る.
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept
	{
		if (a.x < b.x) {
			r_min.x = a.x;
			r_max.x = b.x;
		}
		else {
			r_min.x = b.x;
			r_max.x = a.x;
		}
		if (a.y < b.y) {
			r_min.y = a.y;
			r_max.y = b.y;
		}
		else {
			r_min.y = b.y;
			r_max.y = a.y;
		}
	}

	// 二点の内積を得る.
	double pt_dot(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		return ax * b.x + ay * b.y;
	}

	// 部位が原点を含むか調べる
	// a_pos	部位の位置
	// a_len	部位の一辺の長さ.
	// 戻り値	含む場合 true
	// アンカー長さは 0 より大でなければならない.
	bool pt_in_anch(const D2D1_POINT_2F a_pos, const double a_len) noexcept
	{
		D2D1_POINT_2F a_min;	// 部位の位置を中点とする方形の左上点
		pt_add(a_pos, a_len * -0.5, a_min);
		return a_min.x <= 0.0f && 0.0f <= a_min.x + a_len
			&& a_min.y <= 0.0f && 0.0f <= a_min.y + a_len;
	}

	// 部位が位置を含むか調べる.
	// t_pos	調べる位置
	// a_pos	部位の位置
	// a_len	部位の一辺の長さ.
	// 戻り値	含む場合 true
	// アンカー長さは 0 より大でなければならない.
	bool pt_in_anch(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos, const double a_len) noexcept
	{
		D2D1_POINT_2F a_tran;
		pt_sub(a_pos, t_pos, a_tran);
		return pt_in_anch(a_tran, a_len);
	}

	// だ円にが位置を含むか調べる.
	// t_pos	調べる位置
	// c_pos	だ円の中心
	// rad	だ円の径
	// 戻り値	含む場合 true
	bool pt_in_elli(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept
	{
		// 中心点が原点になるよう調べる位置を移動する.
		const double tx = t_pos.x;
		const double ty = t_pos.y;
		const double cx = c_pos.x;
		const double cy = c_pos.y;
		double px = tx - cx;
		double py = ty - cy;
		const double rx = fabs(rad_x);
		const double ry = fabs(rad_y);
		if (ry <= FLT_MIN) {
			return fabs(py) <= FLT_MIN && fabs(px) <= rx;
		}
		else if (rx <= FLT_MIN) {
			return fabs(py) <= ry && fabs(px) <= FLT_MIN;
		}
		px /= rx;
		py /= ry;
		return px * px + py * py <= 1.0;
	}

	// 線分が位置を含むか, 太さも考慮して調べる.
	// t_pos	調べる位置
	// s_pos	線分の始端
	// e_pos	線分の終端
	// s_width	線分の太さ
	// 戻り値	含む場合 true
	bool pt_in_line(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width) noexcept
	{
		D2D1_POINT_2F d_pos;	// 差分線分のベクトル
		pt_sub(e_pos, s_pos, d_pos);
		const double abs = pt_abs2(d_pos);
		if (abs <= FLT_MIN) {
			return equal(t_pos, s_pos);
		}
		// 線分の法線ベクトルを求める.
		// 法線ベクトルの長さは, 線の太さの半分とする.
		// 長さが 0.5 未満の場合は, 0.5 とする.
		pt_scale(d_pos, max(s_width * 0.5, 0.5) / sqrt(abs), d_pos);
		const double nx = d_pos.y;
		const double ny = -d_pos.x;
		// 線分の両端から, 法線ベクトルの方向, またはその逆の方向にある点を求める.
		// 求めた 4 点からなる四辺形が位置を含むか調べる.
		D2D1_POINT_2F q_pos[4];
		pt_add(s_pos, nx, ny, q_pos[0]);
		pt_add(e_pos, nx, ny, q_pos[1]);
		pt_add(e_pos, -nx, -ny, q_pos[2]);
		pt_add(s_pos, -nx, -ny, q_pos[3]);
		return pt_in_quad(t_pos, q_pos);
	}

	// 四へん形が位置を含むか調べる.
	// t_pos	調べる位置
	// q_pos	四辺形の頂点
	// 戻り値	含む場合 true
	// 四へん形の各辺と, 指定された点を開始点とする水平線が交差する数を求める.
	bool pt_in_quad(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_pos[]) noexcept
	{
		D2D1_POINT_2F p_pos;
		int cnt;
		int i;

		cnt = 0;
		for (p_pos = q_pos[3], i = 0; i < 4; p_pos = q_pos[i++]) {
			// 上向きの辺。点Pがy軸方向について、始点と終点の間にある。ただし、終点は含まない。(ルール1)
			if ((p_pos.y <= t_pos.y && q_pos[i].y > t_pos.y)
				// 下向きの辺。点Pがy軸方向について、始点と終点の間にある。ただし、始点は含まない。(ルール2)
				|| (p_pos.y > t_pos.y&& q_pos[i].y <= t_pos.y)) {
				// ルール1,ルール2を確認することで、ルール3も確認できている。
				// 辺は点pよりも右側にある。ただし、重ならない。(ルール4)
				// 辺が点pと同じ高さになる位置を特定し、その時のxの値と点pのxの値を比較する。
				if (t_pos.x < p_pos.x + (t_pos.y - p_pos.y) / (q_pos[i].y - p_pos.y) * (q_pos[i].x - p_pos.x)) {
					cnt++;
				}
			}
		}
		return static_cast<bool>(cnt & 1);
	}

	// 方形が位置を含むか調べる.
	// t_pos	調べる位置
	// r_min	方形のいずれかの頂点
	// r_max	方形のもう一方の頂点
	// 戻り値	含む場合 true
	bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept
	{
		double min_x;
		double max_x;
		double min_y;
		double max_y;

		if (r_min.x < r_max.x) {
			min_x = r_min.x;
			max_x = r_max.x;
		}
		else {
			min_x = r_max.x;
			max_x = r_min.x;
		}
		if (r_min.y < r_max.y) {
			min_y = r_min.y;
			max_y = r_max.y;
		}
		else {
			min_y = r_max.y;
			max_y = r_min.y;
		}
		return min_x <= t_pos.x && t_pos.x <= max_x
			&& min_y <= t_pos.y && t_pos.y <= max_y;
	}

	// 指定した位置を含むよう, 方形を拡大する.
	// a	含まれる位置
	// r_min	元の方形の左上位置, 得られた左上位置
	// r_max	元の方形の右下位置, 得られた右下位置
	void pt_inc(const D2D1_POINT_2F a, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept
	{
		if (a.x < r_min.x) {
			r_min.x = a.x;
		}
		if (a.x > r_max.x) {
			r_max.x = a.x;
		}
		if (a.y < r_min.y) {
			r_min.y = a.y;
		}
		if (a.y > r_max.y) {
			r_max.y = a.y;
		}
	}

	// 二点の位置を比べてそれぞれ大きい値を得る.
	// a	比べる一方の位置
	// b	比べるもう一方の位置
	// c	得られた位置
	void pt_max(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x > b.x ? a.x : b.x;
		c.y = a.y > b.y ? a.y : b.y;
	}

	// 二点の位置を比べてそれぞれ小さい値を得る.
	// a	比べるられる一方の位置
	// b	もう一方の位置
	// c	得られた位置
	void pt_min(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x < b.x ? a.x : b.x;
		c.y = a.y < b.y ? a.y : b.y;
	}

	// 位置をスカラー倍に丸める.
	// a	丸められる位置
	// b	丸めるスカラー値
	// c	得られた位置
	void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(std::round(a.x / b) * b);
		c.y = static_cast<FLOAT>(std::round(a.y / b) * b);
	}

	// 位置にスカラーを掛けて, 位置を加える.
	// a	掛けられる位置
	// b	掛けるスカラー値
	// c	加える位置
	// d	得られた位置
	void pt_scale(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.x * b + c.x);
		d.y = static_cast<FLOAT>(a.y * b + c.y);
	}

	// 位置にスカラーを掛ける.
	// a	掛けられる位置
	// b	掛けるスカラー値
	// c	得られた位置
	void pt_scale(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x * b);
		c.y = static_cast<FLOAT>(a.y * b);
	}

	// 寸法にスカラー値を掛ける.
	// a	掛けられる寸法
	// b	掛けるスカラー値
	// c	得られた寸法
	void pt_scale(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept
	{
		c.width = static_cast<FLOAT>(a.width * b);
		c.height = static_cast<FLOAT>(a.height * b);
	}

	// 点にスカラーを掛けて, 位置を加える.
	// a	掛けられる位置
	// b	掛けるスカラー値
	// c	加える位置
	// d	得られた位置
	void pt_scale(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.X * b + c.x);
		d.y = static_cast<FLOAT>(a.Y * b + c.y);
	}

	// 位置から位置を引く.
	void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.x;
		c.y = a.y - b.y;
	}

	// 位置から大きさを引く.
	void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.width;
		c.y = a.y - b.height;
	}

	// 矢じりの寸法をデータリーダーから読み込む.
	void read(ARROW_SIZE& value, DataReader const& dt_reader)
	{
		value.m_width = dt_reader.ReadSingle();
		value.m_length = dt_reader.ReadSingle();
		value.m_offset = dt_reader.ReadSingle();
	}

	// 矢じりの形式をデータリーダーから読み込む.
	void read(ARROW_STYLE& value, DataReader const& dt_reader)
	{
		value = static_cast<ARROW_STYLE>(dt_reader.ReadUInt32());
	}

	// ブール値をデータリーダーから読み込む.
	void read(bool& value, DataReader const& dt_reader)
	{
		value = dt_reader.ReadBoolean();
	}

	// 色をデータリーダーから読み込む.
	void read(D2D1_COLOR_F& value, DataReader const& dt_reader)
	{
		value.a = dt_reader.ReadSingle();
		value.r = dt_reader.ReadSingle();
		value.g = dt_reader.ReadSingle();
		value.b = dt_reader.ReadSingle();
		value.a = min(max(value.a, 0.0F), 1.0F);
		value.r = min(max(value.r, 0.0F), 1.0F);
		value.g = min(max(value.g, 0.0F), 1.0F);
		value.b = min(max(value.b, 0.0F), 1.0F);
	}

	// 線枠の形式をデータリーダーから読み込む.
	void read(D2D1_DASH_STYLE& value, DataReader const& dt_reader)
	{
		value = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());
	}

	// 位置をデータリーダーから読み込む.
	void read(D2D1_POINT_2F& value, DataReader const& dt_reader)
	{
		value.x = dt_reader.ReadSingle();
		value.y = dt_reader.ReadSingle();
	}

	// 寸法をデータリーダーから読み込む.
	void read(D2D1_SIZE_F& value, DataReader const& dt_reader)
	{
		value.width = dt_reader.ReadSingle();
		value.height = dt_reader.ReadSingle();
	}

	// 倍精度浮動少数をデータリーダーから読み込む.
	void read(double& value, DataReader const& dt_reader)
	{
		value = dt_reader.ReadDouble();
	}

	// 書体の伸縮をデータリーダーから読み込む.
	void read(DWRITE_FONT_STRETCH& value, DataReader const& dt_reader)
	{
		value = static_cast<DWRITE_FONT_STRETCH>(dt_reader.ReadUInt32());
	}

	// 書体の字体をデータリーダーから読み込む.
	void read(DWRITE_FONT_STYLE& value, DataReader const& dt_reader)
	{
		value = static_cast<DWRITE_FONT_STYLE>(dt_reader.ReadUInt32());
	}

	// 書体の太さをデータリーダーから読み込む.
	void read(DWRITE_FONT_WEIGHT& value, DataReader const& dt_reader)
	{
		value = static_cast<DWRITE_FONT_WEIGHT>(dt_reader.ReadUInt32());
	}

	// 段落の整列をデータリーダーから読み込む.
	void read(DWRITE_PARAGRAPH_ALIGNMENT& value, DataReader const& dt_reader)
	{
		value = static_cast<DWRITE_PARAGRAPH_ALIGNMENT>(dt_reader.ReadUInt32());
	}

	// 文字列の整列をデータリーダーから読み込む.
	void read(DWRITE_TEXT_ALIGNMENT& value, DataReader const& dt_reader)
	{
		value = static_cast<DWRITE_TEXT_ALIGNMENT>(dt_reader.ReadUInt32());
	}

	// 文字列範囲をデータリーダーから読み込む.
	void read(DWRITE_TEXT_RANGE& value, DataReader const& dt_reader)
	{
		value.startPosition = dt_reader.ReadUInt32();
		value.length = dt_reader.ReadUInt32();
	}

	// 破線の配置をデータリーダーから読み込む.
	void read(STROKE_PATT& value, DataReader const& dt_reader)
	{
		value.m_[0] = dt_reader.ReadSingle();
		value.m_[1] = dt_reader.ReadSingle();
		value.m_[2] = dt_reader.ReadSingle();
		value.m_[3] = dt_reader.ReadSingle();
		value.m_[4] = dt_reader.ReadSingle();
		value.m_[5] = dt_reader.ReadSingle();
	}

	// 32 ビット整数をデータリーダーから読み込む.
	void read(uint32_t& value, DataReader const& dt_reader)
	{
		value = dt_reader.ReadUInt32();
	}

	// 文字列をデータリーダーから読み込む.
	void read(wchar_t*& value, DataReader const& dt_reader)
	{
		uint32_t n;	// 文字数

		n = dt_reader.ReadUInt32();
		if (n > 0) {
			value = new wchar_t[static_cast<size_t>(n) + 1];
			if (value != nullptr) {
				for (uint32_t i = 0; i < n; i++) {
					value[i] = dt_reader.ReadUInt16();
				}
				value[n] = L'\0';
			}
		}
		else {
			value = nullptr;
		}
	}

	// 方眼の形式をデータリーダーから読み込む.
	void read(GRID_PATT& value, DataReader const& dt_reader)
	{
		value = static_cast<GRID_PATT>(dt_reader.ReadUInt16());
		if (value == GRID_PATT::PATT_1 || value == GRID_PATT::PATT_2 || value == GRID_PATT::PATT_3) {
			return;
		}
		value = GRID_PATT::PATT_1;
	}

	// 方眼の表示をデータリーダーから読み込む.
	void read(GRID_SHOW& value, DataReader const& dt_reader)
	{
		value = static_cast<GRID_SHOW>(dt_reader.ReadUInt16());
		if (value == GRID_SHOW::BACK || value == GRID_SHOW::FRONT || value == GRID_SHOW::HIDE) {
			return;
		}
		value = GRID_SHOW::BACK;
	}

	// 文字列を複製する.
	// 元の文字列がヌルポインター, または元の文字数が 0 のときは,
	// ヌルポインターを返す.
	wchar_t* wchar_cpy(const wchar_t* const s)
	{
		const auto i = wchar_len(s);
		if (i == 0) {
			return nullptr;
		}
		const auto j = static_cast<size_t>(i) + 1;
		auto t = new wchar_t[j];
		wcscpy_s(t, j, s);
		return t;
	}

	// 文字列の長さ.
	// 引数がヌルポインタの場合, 0 を返す.
	uint32_t wchar_len(const wchar_t* const t) noexcept
	{
		return (t == nullptr || t[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(t));
	}

	// 矢じりの寸法をデータライターに書き込む.
	void write(const ARROW_SIZE& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_width);
		dt_writer.WriteSingle(value.m_length);
		dt_writer.WriteSingle(value.m_offset);
	}

	// 矢じりの形状をデータライターに書き込む.
	void write(const ARROW_STYLE value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// ブール値をデータライターに書き込む.
	void write(const bool value, DataWriter const& dt_writer)
	{
		dt_writer.WriteBoolean(value);
	}

	// 色をデータライターに書き込む.
	void write(const D2D1_COLOR_F& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.a);
		dt_writer.WriteSingle(value.r);
		dt_writer.WriteSingle(value.g);
		dt_writer.WriteSingle(value.b);
	}

	// 破線の形状をデータライターに書き込む.
	void write(const D2D1_DASH_STYLE value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// 位置をデータライターに書き込む.
	void write(const D2D1_POINT_2F value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.x);
		dt_writer.WriteSingle(value.y);
	}

	// 寸法をデータライターに書き込む.
	void write(const D2D1_SIZE_F value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.width);
		dt_writer.WriteSingle(value.height);
	}

	// 倍精度浮動小数をデータライターに書き込む.
	void write(const double value, DataWriter const& dt_writer)
	{
		dt_writer.WriteDouble(value);
	}

	// 書体の伸縮をデータライターに書き込む.
	void write(const DWRITE_FONT_STRETCH value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// 書体の形状をデータライターに書き込む.
	void write(const DWRITE_FONT_STYLE value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// 書体の太さをデータライターに書き込む.
	void write(const DWRITE_FONT_WEIGHT value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// 段落の整列をデータライターに書き込む.
	void write(const DWRITE_PARAGRAPH_ALIGNMENT value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// 文字列の整列をデータライターに書き込む.
	void write(const DWRITE_TEXT_ALIGNMENT value, DataWriter const& dt_writer)
	{
		write(static_cast<uint32_t>(value), dt_writer);
	}

	// 文字列範囲をデータライターに書き込む.
	void write(const DWRITE_TEXT_RANGE value, DataWriter const& dt_writer)
	{
		dt_writer.WriteInt32(value.startPosition);
		dt_writer.WriteInt32(value.length);
	}

	// 方眼の形式をデータライターに書き込む.
	void write(const GRID_PATT value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt16(static_cast<uint16_t>(value));
	}

	// 方眼の表示をデータライターに書き込む.
	void write(const GRID_SHOW value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt16(static_cast<uint16_t>(value));
	}

	// 破線の配置をデータライターに書き込む.
	void write(const STROKE_PATT& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_[0]);
		dt_writer.WriteSingle(value.m_[1]);
		dt_writer.WriteSingle(value.m_[2]);
		dt_writer.WriteSingle(value.m_[3]);
		dt_writer.WriteSingle(value.m_[4]);
		dt_writer.WriteSingle(value.m_[5]);
	}

	// 32 ビット整数をデータライターに書き込む.
	void write(const uint32_t value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(value));
	}

	// 文字列をデータライターに書き込む
	void write(const wchar_t* value, DataWriter const& dt_writer)
	{
		const uint32_t len = wchar_len(value);

		if (len > 0) {
			dt_writer.WriteUInt32(len);
			for (uint32_t i = 0; i < len; i++) {
				dt_writer.WriteUInt16(value[i]);
			}
		}
		else {
			dt_writer.WriteUInt32(0);
		}
	}

	// 属性名とシングルバイト文字列をデータライターに SVG として書き込む.
	// val	シングルバイト文字列
	// attr	属性
	void write_svg(const char* value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		std::snprintf(buf, sizeof(buf), "%s=\"%s\" ", name, value);
		write_svg(buf, dt_writer);
	}

	// シングルバイト文字列をデータライターに SVG として書き込む.
	void write_svg(const char* value, DataWriter const& dt_writer)
	{
		for (uint32_t i = 0; value[i] != '\0'; i++) {
			dt_writer.WriteByte(value[i]);
		}
	}

	// 属性名と色をデータライターに SVG として書き込む.
	void write_svg(const D2D1_COLOR_F value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		const uint32_t vr = static_cast<uint32_t>(std::round(value.r * 255.0)) & 0xff;
		const uint32_t vb = static_cast<uint32_t>(std::round(value.b * 255.0)) & 0xff;
		const uint32_t vg = static_cast<uint32_t>(std::round(value.g * 255.0)) & 0xff;
		sprintf_s(buf, "%s=\"#%02x%02x%02x\" ", name, vr, vg, vb);
		write_svg(buf, dt_writer);
		if (is_opaque(value) == false) {
			std::snprintf(buf, sizeof(buf), "%s-opacity=\"%.3f\" ", name, value.a);
			write_svg(buf, dt_writer);
		}
	}

	// 色をデータライターに SVG として書き込む.
	void write_svg(const D2D1_COLOR_F value, DataWriter const& dt_writer)
	{
		char buf[8];
		const uint32_t vr = static_cast<uint32_t>(std::round(value.r * 255.0)) & 0xff;
		const uint32_t vb = static_cast<uint32_t>(std::round(value.b * 255.0)) & 0xff;
		const uint32_t vg = static_cast<uint32_t>(std::round(value.g * 255.0)) & 0xff;
		sprintf_s(buf, "#%02x%02x%02x", vr, vg, vb);
		write_svg(buf, dt_writer);
	}

	// 破線の形式と配置をデータライターに SVG として書き込む.
	void write_svg(const D2D1_DASH_STYLE style, const STROKE_PATT& patt, const double width, DataWriter const& dt_writer)
	{
		if (width <= FLT_MIN) {
			return;
		}
		const double a[]{
			patt.m_[0] * width,
			patt.m_[1] * width,
			patt.m_[2] * width,
			patt.m_[3] * width
		};
		char buf[256];
		if (style == D2D1_DASH_STYLE_DASH) {
			sprintf_s(buf, "stroke-dasharray=\"%.0f %.0f\" ", a[0], a[1]);
		}
		else if (style == D2D1_DASH_STYLE_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f\" ", a[2], a[3]);
		}
		else if (style == D2D1_DASH_STYLE_DASH_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3]);
		}
		else if (style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
			snprintf(buf, sizeof(buf), "stroke-dasharray=\"%.0f %.0f %.0f %.0f %.0f %.0f\" ", a[0], a[1], a[2], a[3], a[2], a[3]);
		}
		else {
			return;
		}
		write_svg(buf, dt_writer);
	}

	// 命令と位置をデータライターに SVG として書き込む.
	void write_svg(const D2D1_POINT_2F value, const char* cmd, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s%f %f ", cmd, value.x, value.y);
		write_svg(buf, dt_writer);
	}

	// 属性名と位置をデータライターに SVG として書き込む.
	void write_svg(const D2D1_POINT_2F value, const char* name_x, const char* name_y, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" %s=\"%f\" ", name_x, value.x, name_y, value.y);
		write_svg(buf, dt_writer);
	}

	// 属性名と浮動小数値をデータライターに SVG として書き込む
	void write_svg(const double value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%f\" ", name, value);
		write_svg(buf, dt_writer);
	}

	// 浮動小数をデータライターに書き込む
	void write_svg(const float value, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%f ", value);
		write_svg(buf, dt_writer);
	}

	// 属性名と 32 ビット正整数をデータライターに SVG として書き込む
	void write_svg(const uint32_t value, const char* name, DataWriter const& dt_writer)
	{
		char buf[256];
		sprintf_s(buf, "%s=\"%u\" ", name, value);
		write_svg(buf, dt_writer);
	}

	// マルチバイト文字列をデータライターに SVG として書き込む.
	// val	マルチバイト文字列
	// v_len	文字列の文字数
	// dt_writer	データライター
	void write_svg(const wchar_t* value, const uint32_t v_len, DataWriter const& dt_writer)
	{
		if (v_len > 0) {
			const auto s_len = WideCharToMultiByte(CP_UTF8, 0, value, v_len, (char*)NULL, 0, NULL, NULL);
			auto s = new char[static_cast<size_t>(s_len) + 1];
			WideCharToMultiByte(CP_UTF8, 0, value, v_len, static_cast<LPSTR>(s), s_len, NULL, NULL);
			s[s_len] = '\0';
			write_svg(s, dt_writer);
			delete[] s;
		}
	}

}
