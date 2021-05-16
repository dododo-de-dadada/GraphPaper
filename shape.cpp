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

	// 矢じりの軸と寸法をもとに返しの位置を計算する.
	// axis_vec	矢じりの軸ベクトル.
	// axis_len	軸ベクトルの長さ
	// barb_width	矢じりの幅 (返しの両端の長さ)
	// head_len	矢じりの長さ (先端から返しまでの軸ベクトル上での長さ)
	// barbs	計算された矢じりの返しの位置 (先端からのオフセット)
	void get_arrow_barbs(const D2D1_POINT_2F axis_vec, const double axis_len, const double barb_width, const double head_len, D2D1_POINT_2F barbs[]) noexcept
	{
		if (axis_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z = { 0.0f, 0.0f };
			barbs[0] = Z;
			barbs[1] = Z;
		}
		else {
			const double hf = barb_width * 0.5;	// 矢じりの幅の半分の大きさ
			const double sx = axis_vec.x * -head_len;	// 矢じり軸ベクトルを矢じりの長さ分反転
			const double sy = axis_vec.x * hf;
			const double tx = axis_vec.y * -head_len;
			const double ty = axis_vec.y * hf;
			const double ax = 1.0 / axis_len;
			barbs[0].x = static_cast<FLOAT>((sx - ty) * ax);
			barbs[0].y = static_cast<FLOAT>((tx + sy) * ax);
			barbs[1].x = static_cast<FLOAT>((sx + ty) * ax);
			barbs[1].y = static_cast<FLOAT>((tx - sy) * ax);
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
		// 中心点が原点になるよう判定する位置を移動する.
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
	// n	頂点の数
	// v_pos	頂点の配列
	// 戻り値	含む場合 true
	// 多角形の各辺と, 指定された点を開始点とする水平線が交差する数を求める.
	bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t n, const D2D1_POINT_2F v_pos[]) noexcept
	{
		D2D1_POINT_2F p_pos;
		int cnt;
		int i;

		cnt = 0;
		for (p_pos = v_pos[n - 1], i = 0; i < n; p_pos = v_pos[i++]) {
			// 上向きの辺。点Pがy軸方向について、始点と終点の間にある。ただし、終点は含まない。(ルール1)
			if ((p_pos.y <= t_pos.y && v_pos[i].y > t_pos.y)
				// 下向きの辺。点Pがy軸方向について、始点と終点の間にある。ただし、始点は含まない。(ルール2)
				|| (p_pos.y > t_pos.y && v_pos[i].y <= t_pos.y)) {
				// ルール1, ルール2を確認することで, ルール3も確認できている。
				// 辺は点 p よりも右側にある. ただし, 重ならない。(ルール4)
				// 辺が点 p と同じ高さになる位置を特定し, その時のxの値と点pのxの値を比較する。
				if (t_pos.x < p_pos.x + (t_pos.y - p_pos.y) / (v_pos[i].y - p_pos.y) * (v_pos[i].x - p_pos.x)) {
					cnt++;
				}
			}
		}
		return static_cast<bool>(cnt & 1);
	}

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
		const size_t n = dt_reader.ReadUInt32();	// 要素数
		value.resize(n);
		for (size_t i = 0; i < n; i++) {
			read(value[i], dt_reader);
		}
	}

	// 方眼の強調をデータリーダーから読み込む.
	void read(GRID_EMPH& value, DataReader const& dt_reader)
	{
		value = static_cast<GRID_EMPH>(dt_reader.ReadUInt16());
		if (value == GRID_EMPH::EMPH_0 || value == GRID_EMPH::EMPH_2 || value == GRID_EMPH::EMPH_3) {
			return;
		}
		value = GRID_EMPH::EMPH_0;
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
	void write(const GRID_EMPH value, DataWriter const& dt_writer)
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

	// 属性名とシングルバイト文字列をデータライターに SVG として書き込む.
	// value	シングルバイト文字列
	// name	属性名
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
		if (is_opaque(value) != true) {
			std::snprintf(buf, sizeof(buf), "%s-opacity=\"%.3f\" ", name, value.a);
			write_svg(buf, dt_writer);
		}
	}

	// 色をデータライターに SVG として書き込む.
	void write_svg(const D2D1_COLOR_F value, DataWriter const& dt_writer)
	{
		char buf[8];
		const uint32_t vr = static_cast<uint32_t>(std::round(value.r * 255.0)) & 0xFF;
		const uint32_t vb = static_cast<uint32_t>(std::round(value.b * 255.0)) & 0xFF;
		const uint32_t vg = static_cast<uint32_t>(std::round(value.g * 255.0)) & 0xFF;
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
