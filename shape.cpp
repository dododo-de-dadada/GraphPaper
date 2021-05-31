//------------------------------
// shape.cpp
// 図形のひな型, その他
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	ID2D1Factory3* Shape::s_d2d_factory = nullptr;	// D2D1 ファクトリ
	IDWriteFactory3* Shape::s_dwrite_factory = nullptr;	// DWRITE ファクトリ
#if defined(_DEBUG)
	uint32_t debug_leak_cnt = 0;
	uint32_t debug_shape_cnt = 0;
	uint32_t debug_deleted_cnt = 0;
#endif

	// 文字が '0'...'9' または 'A'...'F', 'a'...'f' か判定する.
	static bool is_hex(const wchar_t w, uint32_t& x) noexcept;

	// 図形の部位 (方形) を表示する.
	// a_pos	部位の位置
	// dx		図形の描画環境
	void anchor_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		D2D1_POINT_2F r_min;
		pt_add(a_pos, -0.5 * dx.m_anchor_len, r_min);
		D2D1_POINT_2F r_max;
		pt_add(r_min, dx.m_anchor_len, r_max);
		const D2D1_RECT_F r{ r_min.x, r_min.y, r_max.x, r_max.y };

		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawRectangle(r, dx.m_shape_brush.get(), 2.0, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->FillRectangle(r, dx.m_shape_brush.get());
	}

	// 図形の部位（円形）を表示する.
	// a_pos	部位の位置
	// dx		図形の描画環境
	void anchor_draw_ellipse(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		const FLOAT rad = static_cast<FLOAT>(dx.m_anchor_len * 0.5 + 1.0);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->FillEllipse({ a_pos, rad, rad }, dx.m_shape_brush.get());
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->FillEllipse({ a_pos, rad - 1.0F, rad - 1.0F }, dx.m_shape_brush.get());
	}

	// 矢じりの返しの位置を求める.
	// a_vec	矢軸ベクトル.
	// a_len	矢軸ベクトルの長さ
	// h_width	矢じりの幅 (返しの間の長さ)
	// h_len	矢じりの長さ (先端から返しまでの軸ベクトル上での長さ)
	// b_vec[2]	計算された矢じりの返しの位置 (先端からのオフセット)
	void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F b_vec[]) noexcept
	{
		if (a_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z{ 0.0f, 0.0f };
			b_vec[0] = Z;
			b_vec[1] = Z;
		}
		else {
			const double hf = h_width * 0.5;	// 矢じりの幅の半分の大きさ
			const double sx = a_vec.x * -h_len;	// 矢軸ベクトルを矢じりの長さ分反転
			const double sy = a_vec.x * hf;
			const double tx = a_vec.y * -h_len;
			const double ty = a_vec.y * hf;
			const double ax = 1.0 / a_len;
			b_vec[0].x = static_cast<FLOAT>((sx - ty) * ax);
			b_vec[0].y = static_cast<FLOAT>((tx + sy) * ax);
			b_vec[1].x = static_cast<FLOAT>((sx + ty) * ax);
			b_vec[1].y = static_cast<FLOAT>((tx - sy) * ax);
		}
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
	//double pt_dot(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept
	//{
	//	return static_cast<double>(a.x) * b.x + static_cast<double>(a.y) * b.y;
	//}

	// 図形の部位が位置 { 0,0 } を含むか判定する.
	// a_pos	部位の位置
	// a_len	部位の一辺の長さ.
	// 戻り値	含む場合 true
	// アンカー長さは 0 より大でなければならない.
	bool pt_in_anch(const D2D1_POINT_2F a_pos, const double a_len) noexcept
	{
		D2D1_POINT_2F a_min;	// 部位の位置を中点とする方形の左上点
		pt_add(a_pos, a_len * -0.5, a_min);
		return a_min.x <= 0.0f && 0.0f <= a_min.x + a_len && a_min.y <= 0.0f && 0.0f <= a_min.y + a_len;
	}

	// 図形の部位が位置を含むか判定する.
	// t_pos	判定する位置
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

	// だ円にが位置を含むか判定する.
	// t_pos	判定する位置
	// c_pos	だ円の中心
	// rad	だ円の径
	// 戻り値	含む場合 true
	bool pt_in_elli(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept
	{
		const double dx = static_cast<double>(t_pos.x) - static_cast<double>(c_pos.x);
		const double dy = static_cast<double>(t_pos.y) - static_cast<double>(c_pos.y);
		const double rxrx = rad_x * rad_x;
		const double ryry = rad_y * rad_y;
		return dx * dx * ryry + dy * dy * rxrx <= rxrx * ryry;
		// 中心点が原点になるよう判定する位置を移動する.
		/*
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
		*/
	}

	// 線分が位置を含むか, 太さも考慮して判定する.
	// t_pos	判定する位置
	// s_pos	線分の始端
	// e_pos	線分の終端
	// s_width	線分の太さ
	// 戻り値	含む場合 true
	bool pt_in_line(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width) noexcept
	{
		D2D1_POINT_2F diff;	// 差分線分のベクトル
		pt_sub(e_pos, s_pos, diff);
		const double abs = pt_abs2(diff);
		if (abs <= FLT_MIN) {
			return equal(t_pos, s_pos);
		}
		// 線分の法線ベクトルを求める.
		// 法線ベクトルの長さは, 線の太さの半分とする.
		// 長さが 0.5 未満の場合は, 0.5 とする.
		pt_mul(diff, max(s_width * 0.5, 0.5) / sqrt(abs), diff);
		const double nx = diff.y;
		const double ny = -diff.x;
		// 線分の両端から, 法線ベクトルの方向, またはその逆の方向にある点を求める.
		// 求めた 4 点からなる四辺形が位置を含むか判定する.
		D2D1_POINT_2F exp_side[4];
		pt_add(s_pos, nx, ny, exp_side[0]);
		pt_add(e_pos, nx, ny, exp_side[1]);
		pt_add(e_pos, -nx, -ny, exp_side[2]);
		pt_add(s_pos, -nx, -ny, exp_side[3]);
		return pt_in_poly(t_pos, 4, exp_side);
	}

	// 多角形が位置を含むか判定する.
	// t_pos	判定する位置
	// v_cnt	頂点の数
	// v_pos	頂点の配列 [v_cnt]
	// 戻り値	含む場合 true
	// 多角形の各辺と, 指定された点を開始点とする水平線が交差する数を求める.
	bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[]) noexcept
	{
		const double tx = t_pos.x;
		const double ty = t_pos.y;
		int i_cnt;	// 交点の数
		int i;

		double px = v_pos[v_cnt - 1].x;
		double py = v_pos[v_cnt - 1].y;
		i_cnt = 0;
		for (i = 0; i < v_cnt; i++) {
			double qx = v_pos[i].x;
			double qy = v_pos[i].y;
			// ルール 1. 上向きの辺. 点が y 軸方向について、始点と終点の間にある (ただし、終点は含まない).
			// ルール 2. 下向きの辺. 点が y 軸方向について、始点と終点の間にある (ただし、始点は含まない).
			if ((py <= ty && qy > ty) || (py > ty && qy <= ty)) {
				// ルール 3. 点を通る水平線が辺と重なる (ルール 1, ルール 2 を確認することで, ルール 3 も確認できている).
				// ルール 4. 辺は点よりも右側にある. ただし, 重ならない.
				// 辺が点と同じ高さになる位置を特定し, その時のxの値と点のxの値を比較する.
				if (tx < px + (ty - py) / (qy - py) * (qx - px)) {
					i_cnt++;
				}
			}
			px = qx;
			py = qy;
		}
		return static_cast<bool>(i_cnt & 1);
	}
	/*
	bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[]) noexcept
	{
		D2D1_POINT_2F p_pos;
		int i_cnt;	// 交点の数
		int i;

		i_cnt = 0;
		for (p_pos = v_pos[v_cnt - 1], i = 0; i < v_cnt; p_pos = v_pos[i++]) {
			// ルール 1. 上向きの辺. 点が y 軸方向について、始点と終点の間にある (ただし、終点は含まない).
			// ルール 2. 下向きの辺. 点が y 軸方向について、始点と終点の間にある (ただし、始点は含まない).
			if ((p_pos.y <= t_pos.y && v_pos[i].y > t_pos.y)
				|| (p_pos.y > t_pos.y && v_pos[i].y <= t_pos.y)) {
				// ルール 3. 点を通る水平線が辺と重なる (ルール 1, ルール 2 を確認することで, ルール 3 も確認できている).
				// ルール 4. 辺は点よりも右側にある. ただし, 重ならない.
				// 辺が点と同じ高さになる位置を特定し, その時のxの値と点のxの値を比較する.
				if (t_pos.x < p_pos.x + (t_pos.y - p_pos.y) / (v_pos[i].y - p_pos.y) * (v_pos[i].x - p_pos.x)) {
					i_cnt++;
				}
			}
		}
		return static_cast<bool>(i_cnt & 1);
	}
	*/

	// 方形が位置を含むか判定する.
	// t_pos	判定する位置
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
		return min_x <= t_pos.x && t_pos.x <= max_x && min_y <= t_pos.y && t_pos.y <= max_y;
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

	// 矢じりの寸法をデータリーダーから読み込む.
	void read(ARROWHEAD_SIZE& value, DataReader const& dt_reader)
	{
		value.m_width = dt_reader.ReadSingle();
		value.m_length = dt_reader.ReadSingle();
		value.m_offset = dt_reader.ReadSingle();
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

	// 文字列範囲をデータリーダーから読み込む.
	void read(DWRITE_TEXT_RANGE& value, DataReader const& dt_reader)
	{
		value.startPosition = dt_reader.ReadUInt32();
		value.length = dt_reader.ReadUInt32();
	}

	// 破線の配置をデータリーダーから読み込む.
	void read(STROKE_DASH_PATT& value, DataReader const& dt_reader)
	{
		value.m_[0] = dt_reader.ReadSingle();
		value.m_[1] = dt_reader.ReadSingle();
		value.m_[2] = dt_reader.ReadSingle();
		value.m_[3] = dt_reader.ReadSingle();
		value.m_[4] = dt_reader.ReadSingle();
		value.m_[5] = dt_reader.ReadSingle();
	}

	// 多角形のツールをデータリーダーから読み込む.
	void read(TOOL_POLY& value, DataReader const& dt_reader)
	{
		value.m_vertex_cnt = dt_reader.ReadUInt32();
		value.m_regular = dt_reader.ReadBoolean();
		value.m_vertex_up = dt_reader.ReadBoolean();
		value.m_closed = dt_reader.ReadBoolean();
		value.m_clockwise = dt_reader.ReadBoolean();
	}

	// 文字列をデータリーダーから読み込む.
	void read(wchar_t*& value, DataReader const& dt_reader)
	{
		const size_t n = dt_reader.ReadUInt32();	// 文字数
		if (n > 0) {
			value = new wchar_t[n + 1];
			if (value != nullptr) {
				for (size_t i = 0; i < n; i++) {
					value[i] = dt_reader.ReadUInt16();
				}
				value[n] = L'\0';
			}
		}
		else {
			value = nullptr;
		}
	}

	// 位置配列をデータリーダーから読み込む.
	void read(std::vector<D2D1_POINT_2F>& value, DataReader const& dt_reader)
	{
		const size_t cnt = dt_reader.ReadUInt32();	// 要素数
		value.resize(cnt);
		for (size_t i = 0; i < cnt; i++) {
			read(value[i], dt_reader);
		}
	}

	// 方眼の強調をデータリーダーから読み込む.
	void read(GRID_EMPH& value, DataReader const& dt_reader)
	{
		value.m_gauge_1 = dt_reader.ReadUInt16();
		value.m_gauge_2 = dt_reader.ReadUInt16();
		if (equal(value, GRID_EMPH_0) || equal(value, GRID_EMPH_2) || equal(value, GRID_EMPH_3)) {
			return;
		}
		value = GRID_EMPH_0;
	}

	//	文字列を複製する.
	//	元の文字列がヌルポインター, または元の文字数が 0 のときは,
	//	ヌルポインターを返す.
	wchar_t* wchar_cpy(const wchar_t* const s)
	{
		const auto s_len = wchar_len(s);
		if (s_len == 0) {
			return nullptr;
		}
		auto t = new wchar_t[static_cast<size_t>(s_len) + 1];
		wcscpy_s(t, static_cast<size_t>(s_len) + 1, s);
		return t;
	}

	// 文字が 0...9 または A...F, a...f か判定する
	static bool is_hex(const wchar_t w, uint32_t& x) noexcept
	{
		if (isdigit(w)) {
			x = w - '0';
		}
		else if (w >= 'a' && w <= 'f') {
			x = w - 'a' + 10;
		}
		else if (w >= 'A' && w <= 'F') {
			x = w - 'A' + 10;
		}
		else {
			return false;
		}
		return true;
	}

	// 文字列を複製する.
	// エスケープ文字列は文字コードに変換する.
	wchar_t* wchar_cpy_esc(const wchar_t* const s)
	{
		const auto s_len = wchar_len(s);
		if (s_len == 0) {
			return nullptr;
		}
		auto t = new wchar_t[static_cast<size_t>(s_len) + 1];
		auto st = 0;
		uint32_t j = 0;
		for (uint32_t i = 0; i < s_len && s[i] != '\0' && j < s_len; i++) {
			if (st == 0) {
				if (s[i] == '\\') {
					st = 1;
				}
				else {
					t[j++] = s[i];
				}
			}
			else if (st == 1) {
				// \0-9
				if (s[i] >= '0' && s[i] <= '8') {
					t[j] = s[i] - '0';
					st = 2;
				}
				// \x
				else if (s[i] == 'x') {
					st = 4;
				}
				// \u
				else if (s[i] == 'u') {
					st = 6;
				}
				// \a
				else if (s[i] == 'a') {
					t[j++] = '\a';
					st = 0;
				}
				// \b
				else if (s[i] == 'b') {
					t[j++] = '\b';
					st = 0;
				}
				// \f
				else if (s[i] == 'f') {
					t[j++] = '\f';
					st = 0;
				}
				// \n
				else if (s[i] == 'n') {
					t[j++] = '\n';
					st = 0;
				}
				// \r
				else if (s[i] == 'r') {
					t[j++] = '\r';
					st = 0;
				}
				// \s
				else if (s[i] == 's') {
					t[j++] = ' ';
					st = 0;
				}
				else if (s[i] == 't') {
					t[j++] = '\t';
					st = 0;
				}
				else if (s[i] == 'v') {
					t[j++] = '\v';
					st = 0;
				}
				else {
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 2) {
				if (s[i] >= '0' && s[i] <= '8') {
					t[j] = t[j] * 8 + s[i] - '0';
					st = 3;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 3) {
				if (s[i] >= '0' && s[i] <= '8') {
					t[j++] = t[j] * 8 + s[i] - '0';
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 4) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(x);
					st = 5;
				}
				else if (s[i] == '\\') {
					st = 1;
				}
				else {
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 5) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j++] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 6) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(x);
					st = 7;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 7) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 8;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 8) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 9;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 9) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j++] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
		}
		t[j] = '\0';
		return t;
	}

	// 文字列の長さ.
	// 引数がヌルポインタの場合, 0 を返す.
	uint32_t wchar_len(const wchar_t* const t) noexcept
	{
		return (t == nullptr || t[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(t));
	}

	// 矢じりの寸法をデータライターに書き込む.
	void write(const ARROWHEAD_SIZE& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_width);
		dt_writer.WriteSingle(value.m_length);
		dt_writer.WriteSingle(value.m_offset);
	}

	// 色をデータライターに書き込む.
	void write(const D2D1_COLOR_F& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.a);
		dt_writer.WriteSingle(value.r);
		dt_writer.WriteSingle(value.g);
		dt_writer.WriteSingle(value.b);
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

	// 文字列範囲をデータライターに書き込む.
	void write(const DWRITE_TEXT_RANGE value, DataWriter const& dt_writer)
	{
		dt_writer.WriteInt32(value.startPosition);
		dt_writer.WriteInt32(value.length);
	}

	// 方眼の形式をデータライターに書き込む.
	void write(const GRID_EMPH value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt16(value.m_gauge_1);
		dt_writer.WriteUInt16(value.m_gauge_2);
	}

	// 破線の配置をデータライターに書き込む.
	void write(const STROKE_DASH_PATT& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_[0]);
		dt_writer.WriteSingle(value.m_[1]);
		dt_writer.WriteSingle(value.m_[2]);
		dt_writer.WriteSingle(value.m_[3]);
		dt_writer.WriteSingle(value.m_[4]);
		dt_writer.WriteSingle(value.m_[5]);
	}

	// 多角形のツールをデータライターに書き込む.
	void write(const TOOL_POLY& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(value.m_vertex_cnt));
		dt_writer.WriteBoolean(value.m_regular);
		dt_writer.WriteBoolean(value.m_vertex_up);
		dt_writer.WriteBoolean(value.m_closed);
		dt_writer.WriteBoolean(value.m_clockwise);
	}

	// 文字列をデータライターに書き込む
	void write(const wchar_t* value, DataWriter const& dt_writer)
	{
		const uint32_t len = wchar_len(value);

		dt_writer.WriteUInt32(len);
		for (uint32_t i = 0; i < len; i++) {
			dt_writer.WriteUInt16(value[i]);
		}
	}

	// 位置配列をデータライターに書き込む
	void write(const std::vector<D2D1_POINT_2F>& value, DataWriter const& dt_writer)
	{
		const size_t n = value.size();

		dt_writer.WriteUInt32(static_cast<uint32_t>(n));
		for (uint32_t i = 0; i < n; i++) {
			write(value[i], dt_writer);
		}
	}

}
