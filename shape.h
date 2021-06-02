//------------------------------
// shape.h
// shape.cpp	図形のひな型, その他
// shape_bezi.cpp	ベジェ曲線
// shape_dx.cpp	図形の描画環境
// shape_elli.cpp	だ円
// shape_group.cpp	グループ図形
// shape_line.cpp	直線
// shape_list.cpp	図形リスト
// shape_path.cpp	折れ線のひな型
// shape_poly.cpp	多角形
// shape.rect.cpp	方形
// shape_rrect.cpp	角丸方形
// shape_ruler.cpp	定規
// shape_sheet.cpp	用紙
// shape_stroke.cpp	線枠のひな型
// shape_text.cpp	文字列
//------------------------------
#pragma once
#include <list>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include "shape_dx.h"
// +-------------+
// | SHAPE_DX    |
// +-------------+
//
// +-------------+
// | Shape*      |
// +------+------+
//        |
//        +---------------+---------------+
//        |               |               |
// +------+------+ +------+------+ +------+------+
// | ShapeStroke*| | ShapeGroup  | | ShapeSheet  |
// +------+------+ +-------------+ +-------------+
//        |
//        +-------------------------------+
//        |                               |
// +------+------+                        |
// | ShapeLine   |                        |
// +------+------+                        |
//        |                               |
// +------+------+                 +------+------+
// | ShapePath*  |                 | ShapeRect   |
// +------+------+                 +------+------+
//        |                               |
//        +---------------+               +---------------+---------------+---------------+
//        |               |               |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+
// | ShapePoly   | | ShapeBezi   | | ShapeElli   | | ShapeRRect  | | ShapeText   | | ShapeRuler  |
// +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+

// SVG のためのテキスト改行コード
#define SVG_NEW_LINE	"\n"

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::Point;
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::UI::Color;
	using winrt::Windows::UI::Xaml::Media::Brush;

#if defined(_DEBUG)
	extern uint32_t debug_leak_cnt;
	constexpr wchar_t DEBUG_MSG[] = L"Memory leak occurs";
#endif
	constexpr double PT_ROUND = 1.0 / 16.0;

	// アンカー (図形の部位) の種類
	// 折れ線の頂点をあらわすため, enum struct でなく enum を用いる.
	enum ANCH_TYPE {
		ANCH_SHEET,		// 図形の外部 (矢印カーソル)
		ANCH_FILL,		// 図形の内部 (移動カーソル)
		ANCH_STROKE,	// 線枠 (移動カーソル)
		ANCH_TEXT,		// 文字列 (移動カーソル)
		ANCH_NW,		// 方形の左上の頂点 (北西南東カーソル)
		ANCH_SE,		// 方形の右下の頂点 (北西南東カーソル)
		ANCH_NE,		// 方形の右上の頂点 (北東南西カーソル)
		ANCH_SW,		// 方形の左下の頂点 (北東南西カーソル)
		ANCH_NORTH,		// 方形の上辺の中点 (上下カーソル)
		ANCH_SOUTH,		// 方形の下辺の中点 (上下カーソル)
		ANCH_EAST,		// 方形の左辺の中点 (左右カーソル)
		ANCH_WEST,		// 方形の右辺の中点 (左右カーソル)
		ANCH_R_NW,		// 左上の角丸の中心点 (十字カーソル)
		ANCH_R_NE,		// 右上の角丸の中心点 (十字カーソル)
		ANCH_R_SE,		// 右下の角丸の中心点 (十字カーソル)
		ANCH_R_SW,		// 左下の角丸の中心点 (十字カーソル)
		ANCH_P0,	// パスの開始点 (十字カーソル)
	};

	// 方形の中点の配列
	constexpr uint32_t ANCH_MIDDLE[4]{
		ANCH_TYPE::ANCH_SOUTH,
		ANCH_TYPE::ANCH_EAST,
		ANCH_TYPE::ANCH_WEST,
		ANCH_TYPE::ANCH_NORTH
	};

	// 方形の頂点の配列
	constexpr uint32_t ANCH_CORNER[4]{
		ANCH_TYPE::ANCH_SE,
		ANCH_TYPE::ANCH_NE,
		ANCH_TYPE::ANCH_SW,
		ANCH_TYPE::ANCH_NW
	};

	// 矢じりの形式
	enum struct ARROWHEAD_STYLE {
		NONE,	// なし
		OPENED,	// 開いた矢じり
		FILLED	// 閉じた矢じり
	};

	// 方眼の表示
	enum struct GRID_SHOW {
		HIDE,	// 表示なし
		BACK,	// 最背面に表示
		FRONT	// 最前面に表示
	};

	// 矢じりの寸法
	struct ARROWHEAD_SIZE {
		float m_width;		// 返しの幅
		float m_length;		// 先端から付け根までの長さ
		float m_offset;		// 先端のオフセット
	};

	// 線分の単点
	// (SVG は, 始点終点の区別ができない)
	struct CAP_STYLE {
		D2D1_CAP_STYLE m_start;	// 始点
		D2D1_CAP_STYLE m_end;	// 終点
	};

	// 多角形のツール
	struct TOOL_POLY {
		uint32_t m_vertex_cnt;	// 多角形の頂点の数
		bool m_regular;	// 正多角形で作図するか判定
		bool m_vertex_up;	// 頂点を上に作図するか判定
		bool m_closed;	// 辺を閉じて作図するか判定
		bool m_clockwise;	// 頂点を時計回りに作図するか判定する.
	};

	// 方眼の強調
	struct GRID_EMPH {
		uint16_t m_gauge_1;	// 強調する間隔 (その1)
		uint16_t m_gauge_2;	// 強調する間隔 (その2)
	};
	constexpr GRID_EMPH GRID_EMPH_0{ 0, 0 };	// 強調なし
	constexpr GRID_EMPH GRID_EMPH_2{ 2, 0 };	// 2 番目を強調
	constexpr GRID_EMPH GRID_EMPH_3{ 2, 10 };	// 2 番目と 10 番目を強調

	// 破線の配置
	union STROKE_DASH_PATT {
		float m_[6];
	};

	constexpr float COLOR_MAX = 255.0f;	// 色成分の最大値
	constexpr double PT_PER_INCH = 72.0;	// 1 インチあたりのポイント数
	constexpr double MM_PER_INCH = 25.4;	// 1 インチあたりのミリメートル数
	constexpr float GRID_GRAY_DEF = 0.25f;	// 方眼の濃さ
	constexpr float MITER_LIMIT_DEF = 10.0f;	// マイター制限の比率
	constexpr STROKE_DASH_PATT STROKE_DASH_PATT_DEF{ { 4.0F, 3.0F, 1.0F, 3.0F, 1.0F, 3.0F } };
	constexpr ARROWHEAD_SIZE ARROWHEAD_SIZE_DEF{ 7.0, 16.0, 0.0 };
	constexpr TOOL_POLY TOOL_POLY_DEF{ 3, true, true, true, true };
	constexpr float FONT_SIZE_DEF = static_cast<float>(12.0 * 96.0 / 72.0);
	constexpr D2D1_SIZE_F TEXT_MARGIN_DEF{ FONT_SIZE_DEF / 4.0, FONT_SIZE_DEF / 4.0 };
	constexpr float GRID_LEN_DEF = 48.0f;
	constexpr size_t N_GON_MAX = 256;	// 多角形の頂点の最大数 (ヒット判定でスタックを利用するため, オーバーフローしないよう制限する)

	//------------------------------
	// shape.cpp
	//------------------------------

	// 単精度浮動小数が同じか判定する.
	inline bool equal(const float a, const float b) noexcept;
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const double a, const double b) noexcept;
	// 図形の部位 (方形) を表示する.
	void anchor_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx);
	// 図形の部位（円形）を表示する.
	void anchor_draw_ellipse(const D2D1_POINT_2F c_pos, SHAPE_DX& dx);
	// 同値か判定する.
	template<typename T> inline bool equal(const T a, const T b) noexcept { return a == b; };
	// 矢じりの寸法が同じか判定する.
	inline bool equal(const ARROWHEAD_SIZE& a, const ARROWHEAD_SIZE& b) noexcept;
	// 線の単点が同じか判定する.
	inline bool equal(const CAP_STYLE& a, const CAP_STYLE& b) noexcept;
	// 色が同じか判定する.
	inline bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept;
	// 位置が同じか判定する.
	inline bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept;
	// 寸法が同じか判定する.
	inline bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept;
	// 文字範囲が同じか判定する.
	inline bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept;
	// 方眼の強調が同じか判定する.
	inline bool equal(const GRID_EMPH& a, const GRID_EMPH& b) noexcept;
	// 破線の配置が同じか判定する.
	inline bool equal(const STROKE_DASH_PATT& a, const STROKE_DASH_PATT& b) noexcept;
	// ワイド文字列が同じか判定する.
	inline bool equal(const wchar_t* a, const wchar_t* b) noexcept;
	// winrt 文字列が同じか判定する.
	inline bool equal(winrt::hstring const& a, const wchar_t* b) noexcept;
	// 色の成分が同じか判定する.
	inline bool equal_color_comp(const FLOAT a, const FLOAT b) noexcept;
	// 矢じりの返しの位置を求める.
	void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F b_vec[]) noexcept;
	// 色が不透明か判定する.
	inline bool is_opaque(const D2D1_COLOR_F& color) noexcept;
	// ベクトルの長さ (の自乗値) を得る
	inline double pt_abs2(const D2D1_POINT_2F a) noexcept;
	// 位置に位置を足す
	inline void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// 位置にスカラー値を足す
	inline void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// 位置にX軸とY軸の値を足す
	inline void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& c) noexcept;
	// 二点の中点を得る.
	inline void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// 二点で囲まれた方形を得る.
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// 図形の部位が位置 { 0,0 } を含むか判定する.
	bool pt_in_anch(const D2D1_POINT_2F a_pos, const double a_len) noexcept;
	// 位置が図形の部位に含まれるか判定する.
	bool pt_in_anch(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos, const double a_len) noexcept;
	// 位置がだ円に含まれるか判定する.
	bool pt_in_elli(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept;
	// 位置が円に含まれるか判定する.
	inline bool pt_in_circle(const D2D1_POINT_2F t_pos, const double rad) noexcept;
	// 位置が円に含まれるか判定する.
	inline bool pt_in_circle(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad) noexcept;
	// 線分が位置を含むか, 太さも考慮して判定する.
	bool pt_in_line(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept;
	// 多角形が位置を含むか判定する.
	bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t p_cnt, const D2D1_POINT_2F p_pos[]) noexcept;
	// 方形が位置を含むか判定する.
	bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept;
	// 指定した位置を含むよう, 方形を拡大する.
	void pt_inc(const D2D1_POINT_2F a, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept;
	// 二点のそれぞれ大きい値を持つ位置を得る.
	inline void pt_max(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min) noexcept;
	// 位置の符号を逆にする.
	inline void pt_neg(const D2D1_POINT_2F a, D2D1_POINT_2F& b) noexcept;
	// 二点のそれぞれ小さい値を持つ位置を得る.
	inline void pt_min(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min) noexcept;
	// 位置をスカラー倍に丸める.
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& round) noexcept;
	// 位置にスカラー値を掛け, 別の位置を足す.
	inline void pt_mul(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// 位置にスカラー値を掛ける.
	inline void pt_mul(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// 寸法に値を掛ける.
	inline void pt_mul(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept;
	// 点にスカラー値を掛け, 別の位置を足す.
	inline void pt_mul(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// 位置から位置を引く.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& sub) noexcept;
	// 位置から大きさを引く.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& sub) noexcept;
	// 矢じりの寸法を読み込む.
	void read(ARROWHEAD_SIZE& value, DataReader const& dt_reader);
	// 線分の端点を読み込む.
	void read(CAP_STYLE& value, DataReader const& dt_reader);
	// 色をデータリーダーから読み込む.
	void read(D2D1_COLOR_F& value, DataReader const& dt_reader);
	// 位置をデータリーダーから読み込む.
	void read(D2D1_POINT_2F& value, DataReader const& dt_reader);
	// 寸法をデータリーダーから読み込む.
	void read(D2D1_SIZE_F& value, DataReader const& dt_reader);
	// 文字範囲をデータリーダーから読み込む.
	void read(DWRITE_TEXT_RANGE& value, DataReader const& dt_reader);
	// 方眼の形式をデータリーダーから読み込む.
	void read(GRID_EMPH& value, DataReader const& dt_reader);
	// 破線の配置をデータリーダーから読み込む.
	void read(STROKE_DASH_PATT& value, DataReader const& dt_reader);
	// 多角形のツールをデータリーダーから読み込む.
	void read(TOOL_POLY& value, DataReader const& dt_reader);
	// 文字列をデータリーダーから読み込む.
	void read(wchar_t*& value, DataReader const& dt_reader);
	// 位置配列をデータリーダーから読み込む.
	void read(std::vector<D2D1_POINT_2F>& value, DataReader const& dt_reader);
	// 文字列を複製する. 元の文字列がヌルポインター, または元の文字数が 0 のときは, ヌルポインターを返す.
	wchar_t* wchar_cpy(const wchar_t* const s);
	// 文字列を複製する. エスケープ文字列は文字コードに変換する.
	wchar_t* wchar_cpy_esc(const wchar_t* const s);
	// 文字列の長さ. 引数がヌルポインタの場合, 0 を返す.
	uint32_t wchar_len(const wchar_t* const t) noexcept;
	// 矢じりの寸法をデータライターに書き込む.
	void write(const ARROWHEAD_SIZE& value, DataWriter const& dt_writer);
	// 線分の端点をデータライターに書き込む.
	void write(const CAP_STYLE& value, DataWriter const& dt_writer);
	// 色をデータライターに書き込む.
	void write(const D2D1_COLOR_F& value, DataWriter const& dt_writer);
	// 位置をデータライターに書き込む.
	void write(const D2D1_POINT_2F value, DataWriter const& dt_writer);
	// 寸法をデータライターに書き込む.
	void write(const D2D1_SIZE_F value, DataWriter const& dt_writer);
	// 文字列範囲をデータライターに書き込む.
	void write(const DWRITE_TEXT_RANGE value, DataWriter const& dt_writer);
	// 方眼の形式をデータライターに書き込む.
	void write(const GRID_EMPH value, DataWriter const& dt_writer);
	// 破線の配置をデータライターに書き込む.
	void write(const STROKE_DASH_PATT& value, DataWriter const& dt_writer);
	// 多角形のツールをデータライターに書き込む.
	void write(const TOOL_POLY& value, DataWriter const& dt_writer);
	// 文字列をデータライターに書き込む.
	void write(const wchar_t* value, DataWriter const& dt_writer);
	// 位置配列をデータリーダーに書き込む.
	void write(const std::vector<D2D1_POINT_2F>& value, DataWriter const& dt_writer);

	//------------------------------
	// shape_svg.cpp
	// SVG ファイルへの出力
	//------------------------------

	// シングルバイト文字列をデータライターに SVG として書き込む.
	void write_svg(const char* value, DataWriter const& dt_writer);
	// マルチバイト文字列をデータライターに SVG として書き込む.
	void write_svg(const wchar_t* value, const uint32_t v_len, DataWriter const& dt_writer);
	// 属性名とシングルバイト文字列をデータライターに SVG として書き込む.
	void write_svg(const char* value, const char* name, DataWriter const& dt_writer);
	// 命令と位置をデータライターに SVG として書き込む.
	void write_svg(const D2D1_POINT_2F value, const char* cmd, DataWriter const& dt_writer);
	// 属性名と位置をデータライターに SVG として書き込む.
	void write_svg(const D2D1_POINT_2F value, const char* name_x, const char* name_y, DataWriter const& dt_writer);
	// 属性名と色をデータライターに SVG として書き込む.
	void write_svg(const D2D1_COLOR_F value, const char* name, DataWriter const& dt_writer);
	// 色をデータライターに SVG として書き込む.
	void write_svg(const D2D1_COLOR_F value, DataWriter const& dt_writer);
	// 浮動小数をデータライターに書き込む
	void write_svg(const float value, DataWriter const& dt_writer);
	// 属性名と浮動小数値をデータライターに SVG として書き込む
	void write_svg(const double value, const char* name, DataWriter const& dt_writer);
	// 属性名と 32 ビット正整数をデータライターに SVG として書き込む
	void write_svg(const uint32_t value, const char* name, DataWriter const& dt_writer);
	// 破線の形式と配置をデータライターに SVG として書き込む.
	void write_svg(const D2D1_DASH_STYLE style, const STROKE_DASH_PATT& patt, const double width, DataWriter const& dt_writer);

	// 図形のひな型
	struct Shape {
		// D2D ファクトリのキャッシュ
		static ID2D1Factory3* s_d2d_factory;
		// DWRITE ファクトリのキャッシュ
		static IDWriteFactory3* s_dwrite_factory;

		// 図形を破棄する.
		virtual ~Shape() {}
		// 図形を表示する
		virtual void draw(SHAPE_DX& /*dx*/) {}
		// 部位の位置を得る.
		virtual	void get_anch_pos(const uint32_t /*anch*/, D2D1_POINT_2F&/*value*/) const noexcept {}
		// 矢じりの寸法を得る
		virtual bool get_arrow_size(ARROWHEAD_SIZE& /*value*/) const noexcept { return false; }
		// 矢じりの形式を得る.
		virtual bool get_arrow_style(ARROWHEAD_STYLE& /*value*/) const noexcept { return false; }
		// 図形を囲む領域を得る.
		virtual void get_bound(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/, D2D1_POINT_2F& /*b_min*/, D2D1_POINT_2F& /*b_max*/) const noexcept {}
		// 角丸半径を得る.
		virtual bool get_corner_radius(D2D1_POINT_2F& /*value*/) const noexcept { return false; }
		// 塗りつぶし色を得る.
		virtual bool get_fill_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// 書体の色を得る.
		virtual bool get_font_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// 書体名を得る.
		virtual bool get_font_family(wchar_t*& /*value*/) const noexcept { return false; }
		// 書体の大きさを得る.
		virtual bool get_font_size(float& /*value*/) const noexcept { return false; }
		// 書体の横幅を得る.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH& /*value*/) const noexcept { return false; }
		// 書体の字体を得る.
		virtual bool get_font_style(DWRITE_FONT_STYLE& /*value*/) const noexcept { return false; }
		// 書体の太さを得る.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT& /*value*/) const noexcept { return false; }
		// 方眼の基準の大きさを得る.
		virtual bool get_grid_base(float& /*value*/) const noexcept { return false; }
		// 方眼の大きさを得る.
		virtual bool get_grid_gray(float& /*value*/) const noexcept { return false; }
		// 方眼を強調を得る.
		virtual bool get_grid_emph(GRID_EMPH& /*value*/) const noexcept { return false; }
		// 方眼の表示を得る.
		virtual bool get_grid_show(GRID_SHOW& /*value*/) const noexcept { return false; }
		// 方眼にそろえるを得る.
		virtual bool get_grid_snap(bool& /*value*/) const noexcept { return false; }
		// 図形を囲む領域の左上位置を得る.
		virtual void get_min_pos(D2D1_POINT_2F& /*value*/) const noexcept {}
		// 用紙の色を得る.
		virtual bool get_sheet_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// 用紙の拡大率を得る.
		virtual bool get_sheet_scale(float& /*value*/) const noexcept { return false; }
		// 用紙の大きさを得る.
		virtual bool get_sheet_size(D2D1_SIZE_F& /*value*/) const noexcept { return false; }
		// 開始位置を得る.
		virtual bool get_start_pos(D2D1_POINT_2F& /*value*/) const noexcept { return false; }
		// 線の端点を得る.
		virtual bool get_stroke_cap_style(CAP_STYLE& /*value*/) const noexcept { return false; }
		// 線枠の色を得る.
		virtual bool get_stroke_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// 破線の端点を得る.
		virtual bool get_stroke_dash_cap(D2D1_CAP_STYLE& /*value*/) const noexcept { return false; }
		// 破線の配置を得る.
		virtual bool get_stroke_dash_patt(STROKE_DASH_PATT& /*value*/) const noexcept { return false; }
		// 破線の形式を得る.
		virtual bool get_stroke_dash_style(D2D1_DASH_STYLE& /*value*/) const noexcept { return false; }
		// 線のマイター制限の比率を得る.
		virtual bool get_stroke_join_limit(float& /*value*/) const noexcept { return false; }
		// 線のつながりを得る.
		virtual bool get_stroke_join_style(D2D1_LINE_JOIN& /*value*/) const noexcept { return false; }
		// 書体の太さを得る
		virtual bool get_stroke_width(float& /*value*/) const noexcept { return false; }
		// 文字列を得る.
		virtual bool get_text(wchar_t*& /*value*/) const noexcept { return false; }
		// 段落のそろえを得る.
		virtual bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& /*value*/) const noexcept { return false; }
		// 文字列のそろえを得る.
		virtual bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& /*value*/) const noexcept { return false; }
		// 行の高さを得る.
		virtual bool get_text_line(float& /*value*/) const noexcept { return false; }
		// 文字列の周囲の余白を得る.
		virtual bool get_text_margin(D2D1_SIZE_F& /*value*/) const noexcept { return false; }
		// 文字範囲を得る
		virtual bool get_text_range(DWRITE_TEXT_RANGE& /*value*/) const noexcept { return false; }
		// 位置を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept { return ANCH_TYPE::ANCH_SHEET; }
		// 範囲に含まれるか判定する.
		virtual bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept { return false; }
		// 消去フラグを判定する.
		virtual bool is_deleted(void) const noexcept { return false; }
		// 選択フラグを判定する.
		virtual bool is_selected(void) const noexcept { return false; }
		// 差分だけ移動する.
		virtual	bool move(const D2D1_POINT_2F /*value*/) { return false; }
		// 値を矢じりの寸法に格納する.
		virtual bool set_arrow_size(const ARROWHEAD_SIZE& /*value*/) { return false; }
		// 値を矢じりの形式に格納する.
		virtual bool set_arrow_style(const ARROWHEAD_STYLE /*value*/) { return false; }
		// 値を角丸半径に格納する.
		virtual bool set_corner_radius(const D2D1_POINT_2F& /*alue*/) noexcept { return false; }
		// 値を消去フラグに格納する.
		virtual bool set_delete(const bool /*value*/) noexcept { return false; }
		// 値を塗りつぶし色に格納する.
		virtual bool set_fill_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// 値を書体の色に格納する.
		virtual bool set_font_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// 値を書体名に格納する.
		virtual bool set_font_family(wchar_t* const /*value*/) { return false; }
		// 値を書体の大きさに格納する.
		virtual bool set_font_size(const float /*value*/) { return false; }
		// 値を書体の横幅に格納する.
		virtual bool set_font_stretch(const DWRITE_FONT_STRETCH /*value*/) { return false; }
		// 値を書体の字体に格納する.
		virtual bool set_font_style(const DWRITE_FONT_STYLE /*value*/) { return false; }
		// 値を書体の太さに格納する.
		virtual bool set_font_weight(const DWRITE_FONT_WEIGHT /*value*/) { return false; }
		// 値を方眼の大きさに格納する.
		virtual bool set_grid_base(const float /*value*/) noexcept { return false; }
		// 値を方眼の濃淡に格納する.
		virtual bool set_grid_gray(const float /*value*/) noexcept { return false; }
		// 値を方眼の強調に格納する.
		virtual bool set_grid_emph(const GRID_EMPH& /*value*/) noexcept { return false; }
		// 値を方眼の表示に格納する.
		virtual bool set_grid_show(const GRID_SHOW /*value*/) noexcept { return false; }
		// 値を方眼にそろえるに格納する.
		virtual bool set_grid_snap(const bool /*value*/) noexcept { return false; }
		// 値を用紙の色に格納する.
		virtual bool set_sheet_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// 値を用紙の拡大率に格納する.
		virtual bool set_sheet_scale(const float /*value*/) noexcept { return false; }
		// 値を用紙の大きさに格納する.
		virtual bool set_sheet_size(const D2D1_SIZE_F /*value*/) noexcept { return false; }
		// 値を, 部位の位置に格納する.
		virtual bool set_anchor_pos(const D2D1_POINT_2F /*value*/, const uint32_t /*anch*/) { return false; }
		// 値を選択フラグに格納する.
		virtual bool set_select(const bool /*value*/) noexcept { return false; }
		// 値を線の端点に格納する.
		virtual bool set_stroke_cap_style(const CAP_STYLE& /*value*/) { return false; }
		// 値を線枠の色に格納する.
		virtual bool set_stroke_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// 値を破線の端点に格納する.
		virtual bool set_stroke_dash_cap(const D2D1_CAP_STYLE& /*value*/) { return false; }
		// 値を破線の配置に格納する.
		virtual bool set_stroke_dash_patt(const STROKE_DASH_PATT& /*value*/) { return false; }
		// 値を線枠の形式に格納する.
		virtual bool set_stroke_dash_style(const D2D1_DASH_STYLE /*value*/) { return false; }
		// 値をマイター制限の比率に格納する.
		virtual bool set_stroke_join_limit(const float& /*value*/) { return false; }
		// 値を線のつながりに格納する.
		virtual bool set_stroke_join_style(const D2D1_LINE_JOIN& /*value*/) { return false; }
		// 値を書体の太さに格納する.
		virtual bool set_stroke_width(const float /*value*/) noexcept { return false; }
		// 値を文字列に格納する.
		virtual bool set_text(wchar_t* const /*value*/) { return false; }
		// 値を段落のそろえに格納する.
		virtual bool set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT /*value*/) { return false; }
		// 値を文字列のそろえに格納する.
		virtual bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT /*value*/) { return false; }
		// 値を行間に格納する.
		virtual bool set_text_line(const float /*value*/) { return false; }
		// 値を文字列の余白に格納する.
		virtual bool set_text_margin(const D2D1_SIZE_F /*value*/) { return false; }
		// 値を文字範囲に格納する.
		virtual bool set_text_range(const DWRITE_TEXT_RANGE /*value*/) { return false; }
		// 値を始点に格納する. 他の部位の位置も動く.
		virtual bool set_start_pos(const D2D1_POINT_2F /*value*/) { return false; }
		// データライターに書き込む.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
		// データライターに SVG として書き込む.
		virtual void write_svg(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// shape_list.cpp
	// 図形リストに関連した処理
	//------------------------------

	using S_LIST_T = std::list<struct Shape*>;

	// 使用可能な書体名か判定し, 使用できない書体があったならばそれを得る.
	bool s_list_test_font(const S_LIST_T& s_list, wchar_t*& unavailable_font) noexcept;

	// 最後の図形を得る.
	Shape* s_list_back(S_LIST_T const& s_list) noexcept;

	// 図形リストを消去し, 含まれる図形を破棄する.
	void s_list_clear(S_LIST_T& s_list) noexcept;

	// 図形を種類別に数える.
	void s_list_count(const S_LIST_T& s_list, uint32_t& undeleted_cnt, uint32_t& selected_cnt, uint32_t& selected_group_cnt, uint32_t& runlength_cnt, uint32_t& selected_text_cnt, uint32_t& text_cnt, bool& fore_selected, bool& back_selected, bool& prev_selected) noexcept;

	// 先頭から図形まで数える.
	uint32_t s_list_count(S_LIST_T const& s_list, const Shape* s) noexcept;

	// 最初の図形をリストから得る.
	Shape* s_list_front(S_LIST_T const& s_list) noexcept;

	// 図形と用紙を囲む領域を得る.
	void s_list_bound(S_LIST_T const& s_list, const D2D1_SIZE_F sheet_size, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;

	// 図形を囲む領域をリストから得る.
	void s_list_bound(S_LIST_T const& s_list, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;

	// 位置を含む図形とその部位を得る.
	uint32_t s_list_hit_test(S_LIST_T const& s_list, const D2D1_POINT_2F t_pos, const double a_len, Shape*& s) noexcept;

	// 図形を挿入する.
	void s_list_insert(S_LIST_T& s_list, Shape* const s_ins, const Shape* s_at) noexcept;

	// 選択された図形を差分だけ移動する.
	bool s_list_move(S_LIST_T const& s_list, const D2D1_POINT_2F diff) noexcept;

	// 図形のその次の図形を得る.
	Shape* s_list_next(S_LIST_T const& s_list, const Shape* s) noexcept;

	// 図形のその前の図形を得る.
	Shape* s_list_prev(S_LIST_T const& s_list, const Shape* s) noexcept;

	// 図形リストをデータリーダーから読み込む.
	bool s_list_read(S_LIST_T& s_list, DataReader const& dt_reader);

	// 図形をリストから削除し, 削除した図形の次の図形を得る.
	Shape* s_list_remove(S_LIST_T& s_list, const Shape* s) noexcept;

	// 選択された図形のリストを得る.
	template <typename S> void s_list_selected(S_LIST_T const& s_list, S_LIST_T& t_list) noexcept;

	// 図形リストをデータライターに書き込む. REDUCE 場合の消去フラグの立つ図形は無視する.
	template <bool REDUCE> void s_list_write(const S_LIST_T& s_list, DataWriter const& dt_writer);

	// リストの中の図形の順番を得る.
	template <typename S, typename T> bool s_list_match(S_LIST_T const& s_list, S s, T& t);

	// 選択された文字列図形から, それらを改行で連結した文字列を得る.
	winrt::hstring s_list_selected_all_text(S_LIST_T const& s_list) noexcept;

	//------------------------------
	// 用紙
	//------------------------------
	struct ShapeSheet : Shape {

		// 既定の図形属性
		ARROWHEAD_SIZE m_arrow_size{ ARROWHEAD_SIZE_DEF };	// 矢じりの寸法
		ARROWHEAD_STYLE m_arrow_style = ARROWHEAD_STYLE::NONE;	// 矢じりの形式

		D2D1_POINT_2F m_corner_rad{ GRID_LEN_DEF, GRID_LEN_DEF };	// 角丸半径

		D2D1_COLOR_F m_fill_color{ S_WHITE };	// 塗りつぶしの色

		D2D1_COLOR_F m_font_color{ S_BLACK };	// 書体の色 (MainPage のコンストラクタで設定)
		wchar_t* m_font_family = nullptr;	// 書体名
		float m_font_size = FONT_SIZE_DEF;	// 書体の大きさ
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// 書体の伸縮
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// 書体の字体
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ

		CAP_STYLE m_stroke_cap_style{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };	// 線分の端点
		D2D1_COLOR_F m_stroke_color{ S_BLACK };	// 線枠の色 (MainPage のコンストラクタで設定)
		D2D1_CAP_STYLE m_stroke_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 破線の端点
		STROKE_DASH_PATT m_stroke_dash_patt{ STROKE_DASH_PATT_DEF };	// 破線の配置
		D2D1_DASH_STYLE m_stroke_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// 破線の形式
		float m_stroke_join_limit = MITER_LIMIT_DEF;	// 線のつながりのマイター制限の比率
		D2D1_LINE_JOIN m_stroke_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;	// 線枠のつながり
		float m_stroke_width = 1.0;	// 線枠の太さ

		float m_text_line_h = 0.0f;	// 行間 (DIPs 96dpi固定)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// 段落の揃え
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// 文字列の揃え
		D2D1_SIZE_F m_text_margin{ TEXT_MARGIN_DEF };	// 文字列の左右と上下の余白

		// 方眼の属性
		float m_grid_gray = GRID_GRAY_DEF;	// 方眼の濃さ
		float m_grid_base = GRID_LEN_DEF - 1.0f;	// 方眼の基準の大きさ (を -1 した値)
		GRID_SHOW m_grid_show = GRID_SHOW::BACK;	// 方眼の表示
		GRID_EMPH m_grid_emph{ GRID_EMPH_0 };	// 方眼の強調
		bool m_grid_snap = true;	// 方眼に整列

		// 用紙の属性
		D2D1_COLOR_F m_sheet_color{ S_WHITE };	// 背景色 (MainPage のコンストラクタで設定)
		float m_sheet_scale = 1.0f;	// 拡大率
		D2D1_SIZE_F	m_sheet_size{ 0.0f, 0.0f };	// 大きさ (MainPage のコンストラクタで設定)

		//------------------------------
		// shape_sheet.cpp
		//------------------------------

		// 曲線の補助線を表示する.
		void draw_auxiliary_bezi(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// だ円の補助線を表示する.
		void draw_auxiliary_elli(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// 直線の補助線を表示する.
		void draw_auxiliary_line(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// 方形の補助線を表示する.
		void draw_auxiliary_rect(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// 多角形の補助線を表示する.
		void draw_auxiliary_poly(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos, const TOOL_POLY& t_poly);
		// 角丸方形の補助線を表示する.
		void draw_auxiliary_rrect(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// 方眼を表示する,
		void draw_grid(SHAPE_DX const& dx, const D2D1_POINT_2F offset);
		// 矢じりの寸法を得る.
		bool get_arrow_size(ARROWHEAD_SIZE& value) const noexcept;
		// 矢じりの形式を得る.
		bool get_arrow_style(ARROWHEAD_STYLE& value) const noexcept;
		// 方眼の基準の大きさを得る.
		bool get_grid_base(float& value) const noexcept;
		// 方眼の色を得る.
		void get_grid_color(D2D1_COLOR_F& value) const noexcept;
		// 方眼の大きさを得る.
		bool get_grid_gray(float& value) const noexcept;
		// 方眼の強調を得る.
		bool get_grid_emph(GRID_EMPH& value) const noexcept;
		// 方眼の表示の状態を得る.
		bool get_grid_show(GRID_SHOW& value) const noexcept;
		// 方眼にそろえるを得る.
		bool get_grid_snap(bool& value) const noexcept;
		// 用紙の色を得る.
		bool get_sheet_color(D2D1_COLOR_F& value) const noexcept;
		// 用紙の色を得る.
		bool get_sheet_size(D2D1_SIZE_F& value) const noexcept;
		// 用紙の拡大率を得る.
		bool get_sheet_scale(float& value) const noexcept;
		// 角丸半径を得る.
		bool get_corner_radius(D2D1_POINT_2F& value) const noexcept;
		// 塗りつぶし色を得る.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// 書体の色を得る.
		bool get_font_color(D2D1_COLOR_F& value) const noexcept;
		// 書体名を得る.
		bool get_font_family(wchar_t*& value) const noexcept;
		// 書体の大きさを得る.
		bool get_font_size(float& value) const noexcept;
		// 書体の横幅を得る.
		bool get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept;
		// 書体の字体を得る.
		bool get_font_style(DWRITE_FONT_STYLE& value) const noexcept;
		// 書体の太さを得る.
		bool get_font_weight(DWRITE_FONT_WEIGHT& value) const noexcept;
		// 行間を得る.
		bool get_text_line(float& value) const noexcept;
		// 文字列の周囲の余白を得る.
		bool get_text_margin(D2D1_SIZE_F& value) const noexcept;
		// 段落のそろえを得る.
		bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& value) const noexcept;
		// 線の端点を得る.
		bool get_stroke_cap_style(CAP_STYLE& value) const noexcept;
		// 線枠の色を得る.
		bool get_stroke_color(D2D1_COLOR_F& value) const noexcept;
		// 線の端点を得る.
		bool get_stroke_dash_cap(D2D1_CAP_STYLE& value) const noexcept;
		// 破線の配置を得る.
		bool get_stroke_dash_patt(STROKE_DASH_PATT& value) const noexcept;
		// 破線の形式を得る.
		bool get_stroke_dash_style(D2D1_DASH_STYLE& value) const noexcept;
		// 線のマイター制限の比率を得る.
		bool get_stroke_join_limit(float& value) const noexcept;
		// 線のつながりを得る.
		bool get_stroke_join_style(D2D1_LINE_JOIN& value) const noexcept;
		// 書体の太さを得る
		bool get_stroke_width(float& value) const noexcept;
		// 文字列のそろえを得る.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& value) const noexcept;
		// データリーダーから読み込む.
		void read(DataReader const& dt_reader);
		// 図形の属性値を格納する.
		void set_to(Shape* s) noexcept;
		// 値を角丸半径に格納する.
		bool set_corner_radius(const D2D1_POINT_2F& value) noexcept;
		// 値を方眼の基準の大きさに格納する.
		bool set_grid_base(const float value) noexcept;
		// 値を方眼の濃淡に格納する.
		bool set_grid_gray(const float value) noexcept;
		// 値を方眼の強調に格納する.
		bool set_grid_emph(const GRID_EMPH& value) noexcept;
		// 値を方眼の表示に格納する.
		bool set_grid_show(const GRID_SHOW value) noexcept;
		// 値を方眼にそろえるに格納する.
		bool set_grid_snap(const bool value) noexcept;
		// 値を用紙の色に格納する.
		bool set_sheet_color(const D2D1_COLOR_F& value) noexcept;
		// 値を用紙の拡大率に格納する.
		bool set_sheet_scale(const float value) noexcept;
		// 値を用紙の寸法に格納する.
		bool set_sheet_size(const D2D1_SIZE_F value) noexcept;
		// 値を矢じりの寸法に格納する.
		bool set_arrow_size(const ARROWHEAD_SIZE& value);
		// 値を矢じりの形式に格納する.
		bool set_arrow_style(const ARROWHEAD_STYLE value);
		// 値を塗りつぶし色に格納する.
		bool set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// 値を書体の色に格納する.
		bool set_font_color(const D2D1_COLOR_F& value) noexcept;
		// 書体名に格納する.
		bool set_font_family(wchar_t* const value);
		// 書体の大きさに格納する.
		bool set_font_size(const float value);
		// 値を書体の伸縮に格納する.
		bool set_font_stretch(const DWRITE_FONT_STRETCH value);
		// 値を書体の字体に格納する.
		bool set_font_style(const DWRITE_FONT_STYLE value);
		// 値を書体の太さに格納する.
		bool set_font_weight(const DWRITE_FONT_WEIGHT value);
		// 値を行間に格納する.
		bool set_text_line(const float value);
		// 値を文字列の余白に格納する.
		bool set_text_margin(const D2D1_SIZE_F value);
		// 値を段落のそろえに格納する.
		bool set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value);
		// 値を線の端点に格納する.
		bool set_stroke_cap_style(const CAP_STYLE& value);
		// 値を線枠の色に格納する.
		bool set_stroke_color(const D2D1_COLOR_F& value) noexcept;
		// 値を破線の端点に格納する.
		bool set_stroke_dash_cap(const D2D1_CAP_STYLE& value);
		// 値を破線の配置に格納する.
		bool set_stroke_dash_patt(const STROKE_DASH_PATT& value);
		// 値を線枠の形式に格納する.
		bool set_stroke_dash_style(const D2D1_DASH_STYLE value);
		// 値をマイター制限の比率に格納する.
		bool set_stroke_join_limit(const float& value);
		// 値を線のつながりに格納する.
		bool set_stroke_join_style(const D2D1_LINE_JOIN& value);
		// 値を書体の太さに格納する.
		bool set_stroke_width(const float value) noexcept;
		// 値を文字列のそろえに格納する.
		bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT value);
		// データリーダーに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// グループ図形
	//------------------------------
	struct ShapeGroup : Shape {
		S_LIST_T m_list_grouped;	// グループ化された図形のリスト

		// 図形を作成する.
		ShapeGroup(void) {};

		//------------------------------
		// shape_group.cpp
		//------------------------------

		// 図形を破棄する.
		~ShapeGroup(void);
		// 図形を表示する.
		void draw(SHAPE_DX& dx);
		// 図形を囲む領域を得る.
		void get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// 図形を囲む領域の左上位置を得る.
		void get_min_pos(D2D1_POINT_2F& value) const noexcept;
		// 開始位置を得る.
		bool get_start_pos(D2D1_POINT_2F& value) const noexcept;
		// 文字列図形を含むか判定する.
		bool has_text(void) noexcept;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 消去フラグを判定する.
		bool is_deleted(void) const noexcept;
		// 選択フラグを判定する.
		bool is_selected(void) const noexcept;
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F value);
		// 値を消去フラグに格納する.
		bool set_delete(const bool value) noexcept;
		// 値を選択フラグに格納する.
		bool set_select(const bool value) noexcept;
		// 値を始点に格納する. 他の部位の位置も動く.
		bool set_start_pos(const D2D1_POINT_2F value);
		// 図形をデータリーダーから読み込む.
		ShapeGroup(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 線枠のひな型
	//------------------------------
	struct ShapeStroke : Shape {
		bool m_deleted = false;	// 消去フラグ
		bool m_selected = false;	// 選択フラグ
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// 開始位置
		std::vector<D2D1_POINT_2F> m_diff;	// 次の位置への差分

		CAP_STYLE m_stroke_cap_style{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };	// 線分の端点
		D2D1_COLOR_F m_stroke_color{ S_BLACK };	// 線枠の色
		D2D1_CAP_STYLE m_stroke_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 破線の端点
		STROKE_DASH_PATT m_stroke_dash_patt{ STROKE_DASH_PATT_DEF };	// 破線の配置
		D2D1_DASH_STYLE m_stroke_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// 破線の形式
		float m_stroke_join_limit = MITER_LIMIT_DEF;		// 線のつながりのマイター制限の比率
		D2D1_LINE_JOIN m_stroke_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;	// 線のつながり
		float m_stroke_width = 1.0f;	// 線枠の太さ

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_stroke_style{};	// D2D ストロークスタイル
		//winrt::com_ptr<ID2D1StrokeStyle> m_d2d_arrow_style{};	// 矢じりの D2D ストロークスタイル

		//------------------------------
		// shape_stroke.cpp
		//------------------------------

		// 図形を破棄する.
		~ShapeStroke(void);

		// 図形を囲む領域を得る.
		void get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// 図形を囲む領域の左上位置を得る.
		virtual void get_min_pos(D2D1_POINT_2F& value) const noexcept;
		// 部位の位置を得る.
		virtual	void get_anch_pos(const uint32_t /*anch*/, D2D1_POINT_2F& value) const noexcept;
		// 開始位置を得る.
		virtual bool get_start_pos(D2D1_POINT_2F& value) const noexcept;
		// 線の端点を得る.
		bool get_stroke_cap_style(CAP_STYLE& value) const noexcept;
		// 線枠の色を得る.
		bool get_stroke_color(D2D1_COLOR_F& value) const noexcept;
		// 線の端点を得る.
		bool get_stroke_dash_cap(D2D1_CAP_STYLE& value) const noexcept;
		// 破線の配置を得る.
		bool get_stroke_dash_patt(STROKE_DASH_PATT& value) const noexcept;
		// 破線の形式を得る.
		bool get_stroke_dash_style(D2D1_DASH_STYLE& value) const noexcept;
		// 線枠のマイター制限の比率を得る.
		bool get_stroke_join_limit(float& value) const noexcept;
		// 線のつながりを得る.
		bool get_stroke_join_style(D2D1_LINE_JOIN& value) const noexcept;
		// 線枠の太さを得る.
		bool get_stroke_width(float& value) const noexcept;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const double a_len, const size_t d_cnt, const D2D1_POINT_2F diff[], const bool s_close, const bool f_opa) const noexcept;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept;
		// 消去フラグを判定する.
		bool is_deleted(void) const noexcept { return m_deleted; }
		// 選択フラグを判定する.
		bool is_selected(void) const noexcept { return m_selected; }
		// 差分だけ移動する.
		virtual	bool move(const D2D1_POINT_2F value);
		// 値を選択フラグに格納する.
		bool set_select(const bool value) noexcept { if (m_selected != value) { m_selected = value; return true; } return false; }
		// 値を消去フラグに格納する.
		bool set_delete(const bool value) noexcept { if (m_deleted != value) { m_deleted = value;  return true; } return false; }
		// 値を, 部位の位置に格納する.
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// 値を始点に格納する. 他の部位の位置も動く.
		virtual bool set_start_pos(const D2D1_POINT_2F value);
		// 値を線の端点に格納する.
		bool set_stroke_cap_style(const CAP_STYLE& value);
		// 値を線枠の色に格納する.
		bool set_stroke_color(const D2D1_COLOR_F& value) noexcept;
		// 値を破線の端点に格納する.
		bool set_stroke_dash_cap(const D2D1_CAP_STYLE& value);
		// 値を破線の配置に格納する.
		bool set_stroke_dash_patt(const STROKE_DASH_PATT& value);
		// 値を線枠の形式に格納する.
		bool set_stroke_dash_style(const D2D1_DASH_STYLE value);
		// 値をマイター制限の比率に格納する.
		bool set_stroke_join_limit(const float& value);
		// 値を線のつながりに格納する.
		bool set_stroke_join_style(const D2D1_LINE_JOIN& value);
		// 値を線枠の太さに格納する.
		bool set_stroke_width(const float value) noexcept;
		// 図形を作成する.
		ShapeStroke(const size_t d_cnt, const ShapeSheet* s_attr);
		// 図形をデータリーダーから読み込む.
		ShapeStroke(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 矢じりをデータライターに SVG タグとして書き込む.
		void write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, const ARROWHEAD_STYLE h_style, DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 直線
	//------------------------------
	struct ShapeLine : ShapeStroke {
		ARROWHEAD_STYLE m_arrow_style = ARROWHEAD_STYLE::NONE;	// 矢じりの形式
		ARROWHEAD_SIZE m_arrow_size{ ARROWHEAD_SIZE_DEF };	// 矢じりの寸法
		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_arrow_style{ nullptr };	// 矢じりの D2D ストロークスタイル
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_arrow_geom{ nullptr };	// 矢じりの D2D パスジオメトリ

		//------------------------------
		// shape_line.cpp
		//------------------------------

		ShapeLine(const size_t d_cnt, const ShapeSheet* s_attr, const bool closed = false) :
			ShapeStroke(d_cnt, s_attr),
			m_arrow_style(!closed ? s_attr->m_arrow_style : ARROWHEAD_STYLE::NONE),
			m_arrow_size(s_attr->m_arrow_size),
			m_d2d_arrow_geom(nullptr),
			m_d2d_arrow_style(nullptr)
		{}
		// 図形を作成する.
		ShapeLine(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr);
		// 図形をデータリーダーから読み込む.
		ShapeLine(DataReader const& dt_reader);
		// 図形を破棄する.
		~ShapeLine(void);
		// 表示する.
		void draw(SHAPE_DX& dx);
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 矢じりの寸法を得る.
		bool get_arrow_size(ARROWHEAD_SIZE& size) const noexcept;
		// 矢じりの形式を得る.
		bool get_arrow_style(ARROWHEAD_STYLE& value) const noexcept;
		// 値を矢じりの寸法に格納する.
		bool set_arrow_size(const ARROWHEAD_SIZE& value);
		// 値を矢じりの形式に格納する.
		bool set_arrow_style(const ARROWHEAD_STYLE value);
		// 部位の位置を得る.
		//void get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept;
		//	値を, 部位の位置に格納する. 
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// 値を始点に格納する.
		bool set_start_pos(const D2D1_POINT_2F value);
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F value);
		// データリーダーから読み込む.
		//void read(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
		// 
		void write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const;
		bool set_stroke_cap_style(const CAP_STYLE& value);
		bool set_stroke_join_limit(const float& value);
		bool set_stroke_join_style(const D2D1_LINE_JOIN& value);
	};

	//------------------------------
	// 方形
	//------------------------------
	struct ShapeRect : ShapeStroke {
		D2D1_COLOR_F m_fill_color{ S_WHITE };		// 塗りつぶし

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// 図形を作成する.
		ShapeRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr);
		// 図形をデータリーダーから読み込む.
		ShapeRect(DataReader const& dt_reader);
		// 表示する.
		void draw(SHAPE_DX& dx);
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 図形の部位が位置を含むか判定する.
		uint32_t hit_test_anchor(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 塗りつぶしの色を得る.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// 値を塗りつぶしの色に格納する.
		bool set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// 部位の位置を得る.
		void get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept;
		//	値を, 部位の位置に格納する.
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 定規
	// 作成したあとで文字列の属性の変更はできない.
	//------------------------------
	struct ShapeRuler : ShapeRect {
		float m_grid_base = GRID_LEN_DEF - 1.0f;	// 方眼の大きさ (を -1 した値)
		winrt::com_ptr<IDWriteTextFormat> m_dw_text_format{};	// テキストフォーマット

		//------------------------------
		// shape_ruler.cpp
		//------------------------------

		// 図形を破棄する.
		~ShapeRuler(void);
		// 図形を表示する.
		void draw(SHAPE_DX& dx);
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 図形を作成する.
		ShapeRuler(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr);
		// 図形をデータリーダーから読み込む.
		ShapeRuler(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// だ円
	//------------------------------
	struct ShapeElli : ShapeRect {
		// 図形を作成する.
		ShapeElli(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* attr) :
			ShapeRect::ShapeRect(b_pos, b_diff, attr)
		{}
		// 図形をデータリーダーから読み込む.
		ShapeElli(DataReader const& dt_reader) :
			ShapeRect::ShapeRect(dt_reader)
		{}

		//------------------------------
		// shape_elli.cpp
		//------------------------------

		// 図形を表示する.
		void draw(SHAPE_DX& dx);
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 角丸方形
	//------------------------------
	struct ShapeRRect : ShapeRect {
		D2D1_POINT_2F m_corner_rad{ GRID_LEN_DEF, GRID_LEN_DEF };		// 角丸部分の半径

		//------------------------------
		// shape_rrect.cpp
		// 角丸方形
		//------------------------------

		// 図形を作成する.
		ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr);
		// 図形をデータリーダーから読み込む.
		ShapeRRect(DataReader const& dt_reader);
		// 図形を表示する.
		void draw(SHAPE_DX& dx);
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 角丸半径を得る.
		bool get_corner_radius(D2D1_POINT_2F& value) const noexcept;
		// 部位の位置を得る.
		void get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept;
		//	値を, 部位の位置に格納する.
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 折れ線のひな型
	//------------------------------
	struct ShapePath : ShapeLine {
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_path_geom{};	// 折れ線の D2D パスジオメトリ

		//------------------------------
		// shape_path.cpp
		// 折れ線のひな型
		//------------------------------

		// パスジオメトリを作成する.
		virtual void create_path_geometry(ID2D1Factory3* const/*d_factory*/) {}
		// 図形を作成する.
		ShapePath(const size_t d_cnt, const ShapeSheet* attr, const bool closed);
		// 図形をデータリーダーから読み込む.
		ShapePath(DataReader const& dt_reader);
		// 図形を破棄する.
		//~ShapePath(void);
		// 矢じりの寸法を得る
		//bool get_arrow_size(ARROWHEAD_SIZE& value) const noexcept;
		// 矢じりの形式を得る.
		//bool get_arrow_style(ARROWHEAD_STYLE& value) const noexcept;
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F value);
		// 値を, 部位の位置に格納する.
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// 値を矢じりの寸法に格納する.
		bool set_arrow_size(const ARROWHEAD_SIZE& value);
		// 値を矢じりの形式に格納する.
		bool set_arrow_style(const ARROWHEAD_STYLE value);
		// 値を始点に格納する. 他の部位の位置も動く.
		bool set_start_pos(const D2D1_POINT_2F value);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 多角形
	//------------------------------
	struct ShapePoly : ShapePath {
		bool m_end_closed;	// 辺が閉じているか判定
		D2D1_COLOR_F m_fill_color;

		//------------------------------
		// shape_poly.cpp
		//------------------------------

		// 境界方形をみたす多角形を作成する.
		static void create_poly_by_bbox(const D2D1_POINT_2F b_min, const D2D1_POINT_2F b_max, const size_t v_cnt, const bool v_up, const bool v_reg, const bool v_clock, D2D1_POINT_2F v_pos[]) noexcept;
		// パスジオメトリを作成する.
		void create_path_geometry(ID2D1Factory3* const d_factory);
		// 表示する
		void draw(SHAPE_DX& dx);
		// 塗りつぶし色を得る.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 値を矢じりの形式に格納する.
		bool set_arrow_style(const ARROWHEAD_STYLE value);
		// 値を塗りつぶし色に格納する.
		bool set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// 図形を作成する.
		ShapePoly(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr, const TOOL_POLY& poly);
		// 図形をデータリーダーから読み込む.
		ShapePoly(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& /*dt_writer*/) const;
		// データライターに SVG として書き込む.
		void write_svg(DataWriter const& /*dt_writer*/) const;
	};

	//------------------------------
	// 曲線
	//------------------------------
	struct ShapeBezi : ShapePath {

		//------------------------------
		// shape_bezi.cpp
		// ベジェ曲線
		//------------------------------

		// パスジオメトリを作成する.
		void create_path_geometry(ID2D1Factory3* const d_factory);
		// 表示する.
		void draw(SHAPE_DX& dx);
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 図形を作成する.
		ShapeBezi(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr);
		// 図形をデータリーダーから読み込む.
		ShapeBezi(DataReader const& dt_reader);
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 文字列
	//------------------------------
	struct ShapeText : ShapeRect {
		static wchar_t** s_available_fonts;		// 有効な書体名

		DWRITE_TEXT_RANGE m_select_range{ 0, 0 };	// 選択範囲
		D2D1_COLOR_F m_font_color{ S_BLACK };	// 書体の色
		wchar_t* m_font_family = nullptr;	// 書体名
		float m_font_size = FONT_SIZE_DEF;	// 書体の大きさ
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// 書体の伸縮
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// 書体の字体
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ
		wchar_t* m_text = nullptr;	// 文字列
		float m_text_line_h = 0.0f;	// 行間 (DIPs 96dpi固定)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// 段落そろえ
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// 文字揃え
		D2D1_SIZE_F m_text_margin{ TEXT_MARGIN_DEF };	// 文字列のまわりの上下と左右の余白

		winrt::com_ptr<IDWriteTextLayout> m_dw_layout{};	// 文字列を表示するためのレイアウト
		float m_dw_descent = 0.0f;	// ディセント
		UINT32 m_dw_line_cnt = 0;	// 行の計量の要素数
		DWRITE_LINE_METRICS* m_dw_line_metrics = nullptr;	// 行の計量
		UINT32 m_dw_selected_cnt = 0;	// 選択範囲の計量の要素数
		DWRITE_HIT_TEST_METRICS* m_dw_selected_metrics = nullptr;	// 選択範囲の計量
		UINT32 m_dw_test_cnt = 0;	// 位置の計量の要素数
		DWRITE_HIT_TEST_METRICS* m_dw_test_metrics = nullptr;	// 位置の計量

		// 図形を破棄する.
		~ShapeText(void);
		// 領域の大きさを, それが文字列より小さくならないように, 調整する. 
		bool adjust_bbox(const D2D1_SIZE_F& bound = D2D1_SIZE_F{ 0.0F, 0.0F });
		// テキストレイアウトを破棄して作成する.
		void create_text_layout(IDWriteFactory3* d_factory);
		// 計量を破棄して作成する.
		void create_text_metrics(IDWriteFactory3* d_factory);
		// 文末の空白を取り除く.
		void delete_bottom_blank(void) noexcept;
		// 表示する.
		void draw(SHAPE_DX& dx);
		// 選択された文字範囲を塗る.
		void fill_range(SHAPE_DX& dx, const D2D1_POINT_2F t_min);
		// 文字列の枠を表示する.
		void draw_frame(SHAPE_DX& dx, const D2D1_POINT_2F t_min);
		// 有効な書体名から要素を得る.
		static wchar_t* get_available_font(const uint32_t i);
		// 書体の色を得る.
		bool get_font_color(D2D1_COLOR_F& value) const noexcept;
		// 書体名を得る.
		bool get_font_family(wchar_t*& value) const noexcept;
		// 書体の大きさを得る.
		bool get_font_size(float& value) const noexcept;
		// 書体の伸縮を得る.
		bool get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept;
		// 書体の字体を得る.
		bool get_font_style(DWRITE_FONT_STYLE& value) const noexcept;
		// 行間を得る.
		bool get_text_line(float& value) const noexcept;
		// 書体の太さを得る.
		bool get_font_weight(DWRITE_FONT_WEIGHT& value) const noexcept;
		// 文字列の余白を得る.
		bool get_text_margin(D2D1_SIZE_F& value) const noexcept;
		// 段落のそろえを得る.
		bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& value) const noexcept;
		// 文字列を得る.
		bool get_text(wchar_t*& value) const noexcept;
		// 文字列のそろえを得る.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& value) const noexcept;
		// 文字範囲を得る.
		bool get_text_range(DWRITE_TEXT_RANGE& value) const noexcept;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 有効な書体名か判定し, 有効ならもとの書体名を破棄し, 有効な書体名要素へのポインターと置き換える.
		static bool is_available_font(wchar_t*& font);
		// 有効な書体名の配列を破棄する.
		static void release_available_fonts(void);
		// 有効な書体名の配列を設定する.
		static void set_available_fonts(void);
		// 値を書体の色に格納する.
		bool set_font_color(const D2D1_COLOR_F& value) noexcept;
		// 値を書体名に格納する.
		bool set_font_family(wchar_t* const value);
		// 値を書体の大きさに格納する.
		bool set_font_size(const float value);
		// 値を書体の伸縮に格納する.
		bool set_font_stretch(const DWRITE_FONT_STRETCH value);
		// 値を書体の字体に格納する.
		bool set_font_style(const DWRITE_FONT_STYLE value);
		// 値を書体の太さに格納する.
		bool set_font_weight(const DWRITE_FONT_WEIGHT value);
		// 値を行間に格納する.
		bool set_text_line(const float value);
		// 値を文字列の余白に格納する.
		bool set_text_margin(const D2D1_SIZE_F value);
		// 値を段落のそろえに格納する.
		bool set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value);
		// 値を, 部位の位置に格納する.
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// 値を文字列に格納する.
		bool set_text(wchar_t* const value);
		// 値を文字列のそろえに格納する.
		bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT value);
		// 値を文字範囲に格納する.
		bool set_text_range(const DWRITE_TEXT_RANGE value);
		// 図形を作成する.
		ShapeText(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, wchar_t* const text, const ShapeSheet* s_attr);
		// 図形をデータリーダーから読み込む.
		ShapeText(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	// 矢じりの寸法が同じか判定する.
	inline bool equal(const ARROWHEAD_SIZE& a, const ARROWHEAD_SIZE& b) noexcept
	{
		return equal(a.m_width, b.m_width) && equal(a.m_length, b.m_length) && equal(a.m_offset, b.m_offset);
	}

	// 線の端点が同じか判定する.
	inline bool equal(const CAP_STYLE& a, const CAP_STYLE& b) noexcept
	{
		return a.m_start == b.m_start && a.m_end == b.m_end;
	}

	// 色が同じか判定する.
	inline bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept
	{
		return equal_color_comp(a.a, b.a) && equal_color_comp(a.r, b.r) && equal_color_comp(a.g, b.g) && equal_color_comp(a.b, b.b);
	}

	// 位置が同じか判定する.
	inline bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept
	{
		return equal(a.x, b.x) && equal(a.y, b.y);
	}

	// 寸法が同じか判定する.
	inline bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept
	{
		return equal(a.width, b.width) && equal(a.height, b.height);
	}

	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const double a, const double b) noexcept
	{
		return fabs(a - b) <= FLT_EPSILON * fmax(1.0, fmax(fabs(a), fabs(b)));
	}

	// 文字範囲が同じか判定する.
	inline bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept
	{
		return a.startPosition == b.startPosition && a.length == b.length;
	}

	// 単精度浮動小数が同じか判定する.
	inline bool equal(const float a, const float b) noexcept
	{
		return fabs(a - b) <= FLT_EPSILON * fmax(1.0f, fmax(fabs(a), fabs(b)));
	}

	// 方眼の形式が同じか判定する.
	inline bool equal(const GRID_EMPH& a, const GRID_EMPH& b) noexcept
	{
		return a.m_gauge_1 == b.m_gauge_1 && a.m_gauge_2 == b.m_gauge_2;
	}

	// 破線の配置が同じか判定する.
	inline bool equal(const STROKE_DASH_PATT& a, const STROKE_DASH_PATT& b) noexcept
	{
		return equal(a.m_[0], b.m_[0]) && equal(a.m_[1], b.m_[1]) && equal(a.m_[2], b.m_[2]) && equal(a.m_[3], b.m_[3]) && equal(a.m_[4], b.m_[4]) && equal(a.m_[5], b.m_[5]);
	}

	// 文字列が同じか判定する.
	inline bool equal(const wchar_t* a, const wchar_t* b) noexcept
	{
		return a == b || (a != nullptr && b != nullptr && wcscmp(a, b) == 0);
	}

	// 文字列が同じか判定する.
	inline bool equal(winrt::hstring const& a, const wchar_t* b) noexcept
	{
		return a == (b == nullptr ? L"" : b);
	}

	// 色の成分が同じか判定する.
	inline bool equal_color_comp(const FLOAT a, const FLOAT b) noexcept
	{
		return fabs(b - a) * 128.0f < 1.0f;
	}

	// ベクトルの長さ (の自乗値) を得る
	// a	ベクトル
	// 戻り値	長さ (の自乗値) 
	inline double pt_abs2(const D2D1_POINT_2F a) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		return ax * ax + ay * ay;
	}

	// 位置を位置に足す
	inline void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x + b.x;
		c.y = a.y + b.y;
	}

	// スカラー値を位置に足す
	inline void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x + b);
		c.y = static_cast<FLOAT>(a.y + b);
	}

	// 2 つの値を位置に足す
	inline void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& b) noexcept
	{
		b.x = static_cast<FLOAT>(a.x + x);
		b.y = static_cast<FLOAT>(a.y + y);
	}

	// 二点間の中点を求める.
	inline void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>((a.x + b.x) * 0.5);
		c.y = static_cast<FLOAT>((a.y + b.y) * 0.5);
	}

	// 位置が円に含まれるか判定する.
	inline bool pt_in_circle(const D2D1_POINT_2F a, const D2D1_POINT_2F c, const double r) noexcept
	{
		const double dx = static_cast<double>(a.x) - static_cast<double>(c.x);
		const double dy = static_cast<double>(a.y) - static_cast<double>(c.y);
		return dx * dx + dy * dy <= r * r;
	}

	// 位置が円に含まれるか判定する.
	inline bool pt_in_circle(const D2D1_POINT_2F a, const double r) noexcept
	{
		const double dx = a.x;
		const double dy = a.y;
		return dx * dx + dy * dy <= r * r;
	}

	// 二点の位置を比べてそれぞれ大きい値を求める.
	// a	一方の位置
	// b	もう一方の位置
	// c	結果
	inline void pt_max(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x > b.x ? a.x : b.x;
		c.y = a.y > b.y ? a.y : b.y;
	}

	// 二点の位置を比べてそれぞれ小さい値を求める.
	// a	一方の位置
	// b	もう一方の位置
	// c	結果
	inline void pt_min(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x < b.x ? a.x : b.x;
		c.y = a.y < b.y ? a.y : b.y;
	}

	// 位置の符号を反対にする.
	// a	位置
	// b	結果
	inline void pt_neg(const D2D1_POINT_2F a, D2D1_POINT_2F& b) noexcept
	{
		b.x = -a.x;
		b.y = -a.y;
	}

	// 位置をスカラー倍に丸める.
	// a	位置
	// b	スカラー値
	// c	結果
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(std::round(a.x / b) * b);
		c.y = static_cast<FLOAT>(std::round(a.y / b) * b);
	}

	// 位置にスカラーを掛けて, 位置を加える.
	// a	位置
	// b	スカラー値
	// c	加える位置
	// d	結果
	inline void pt_mul(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.x * b + c.x);
		d.y = static_cast<FLOAT>(a.y * b + c.y);
	}

	// 位置にスカラーを掛ける.
	// a	位置
	// b	スカラー値
	// c	結果
	inline void pt_mul(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x * b);
		c.y = static_cast<FLOAT>(a.y * b);
	}

	// 寸法にスカラー値を掛ける.
	// a	寸法
	// b	スカラー値
	// c	結果
	inline void pt_mul(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept
	{
		c.width = static_cast<FLOAT>(a.width * b);
		c.height = static_cast<FLOAT>(a.height * b);
	}

	// 点にスカラーを掛けて, 位置を加える.
	// a	位置
	// b	スカラー値
	// c	加える位置
	// d	結果
	inline void pt_mul(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.X * b + c.x);
		d.y = static_cast<FLOAT>(a.Y * b + c.y);
	}

	// 位置から位置を引く.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.x;
		c.y = a.y - b.y;
	}

	// 位置から大きさを引く.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.width;
		c.y = a.y - b.height;
	}

	// 色が不透明か判定する.
	// a	色
	// 戻り値	不透明ならば true, 透明ならば false.
	inline bool is_opaque(const D2D1_COLOR_F& a) noexcept
	{
		const uint32_t aa = static_cast<uint32_t>(round(a.a * 255.0f));
		return (aa & 0xff) > 0;
	}

}