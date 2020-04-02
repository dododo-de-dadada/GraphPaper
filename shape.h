//------------------------------
// shape.h
// shape.cpp
// shape_bezi.cpp	ベジェ曲線
// shape_dx.cpp	図形の描画環境
// shape_elli.cpp	だ円
// shape_group.cpp	グループ図形
// shape_line.cpp	直線
// shape_list.cpp	図形リスト
// shape_layout.cpp	レイアウト
// shape_quad.cpp	四へん形
// shape.rect.cpp	方形
// shape_rrect.cpp	角丸方形
// shape_stroke.cpp	線枠, 折れ線のひな型
// shape_text.cpp	文字列図形
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
// | ShapeStroke*| | ShapeGroup  | | ShapeLayout |
// +------+------+ +-------------+ +-------------+
//        |
//        +---------------+---------------+
//        |               |               |
// +------+------+ +------+------+ +------+------+
// | ShapePoly*  | | ShapeLine   | | ShapeRect   |
// +------+------+ +-------------+ +------+------+
//        |                               |
//        +---------------+               +---------------+---------------+---------------+
//        |               |               |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+
// | ShapeQuad   | | ShapeBezi   | | ShapeElli   | | ShapeRRect  | | ShapeText   | | ShapeScale  |
// +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+

// SVG のためのテキスト改行コード
#define SVG_NL	"\n"

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::Point;
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::UI::Color;
	using winrt::Windows::UI::Xaml::Media::Brush;

#if defined(_DEBUG)
	extern uint32_t debug_leak_cnt;
#endif

	// 図形の部位 (アンカー)
	enum struct ANCH_WHICH {
		ANCH_OUTSIDE,		// 図形の外部
		ANCH_INSIDE,		// 図形の内部
		ANCH_FRAME,		// 線枠 (移動カーソル)
		ANCH_TEXT,		// 文字列
		ANCH_NW,		// 方形の左上の頂点 (北西南東カーソル)
		ANCH_SE,		// 方形の右下の頂点 (北西南東カーソル)
		ANCH_NE,		// 方形の右上の頂点 (北東南西カーソル)
		ANCH_SW,		// 方形の左下の頂点 (北東南西カーソル)
		ANCH_NORTH,		// 方形の上辺の中点 (上下カーソル)
		ANCH_SOUTH,		// 方形の下辺の中点 (上下カーソル)
		ANCH_EAST,		// 方形の左辺の中点 (左右カーソル)
		ANCH_WEST,		// 方形の右辺の中点 (左右カーソル)
		ANCH_R_NW,		// 角丸の左上の中心点 (十字カーソル)
		ANCH_R_NE,		// 角丸の右上の中心点 (十字カーソル)
		ANCH_R_SE,		// 角丸の右下の中心点 (十字カーソル)
		ANCH_R_SW,		// 角丸の左下の中心点 (十字カーソル)
	};

	// 方形の中点の配列
	constexpr ANCH_WHICH ANCH_MIDDLE[4]{
		ANCH_WHICH::ANCH_SOUTH,
		ANCH_WHICH::ANCH_EAST,
		ANCH_WHICH::ANCH_WEST,
		ANCH_WHICH::ANCH_NORTH
	};

	// 方形の頂点の配列
	constexpr ANCH_WHICH ANCH_CORNER[4]{
		ANCH_WHICH::ANCH_SE,
		ANCH_WHICH::ANCH_NE,
		ANCH_WHICH::ANCH_SW,
		ANCH_WHICH::ANCH_NW
	};

	// 矢じりの形式
	enum struct ARROW_STYLE {
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

	enum struct GRID_PATT {
		PATT_1,
		PATT_2,
		PATT_3
	};

	// 図形の種類
	// ファイルへの書き込みで使用する.
	enum SHAPE_TYPE {
		SHAPE_NULL,		// ヌル
		SHAPE_BEZI,		// 曲線
		SHAPE_ELLI,		// だ円
		SHAPE_LINE,		// 線分
		SHAPE_QUAD,		// 四辺形
		SHAPE_RECT,		// 方形
		SHAPE_RRECT,	// 角丸方形
		SHAPE_TEXT,		// 文字列
		SHAPE_GROUP,	// グループ
		SHAPE_SCALE		// 目盛り
	};

	// 矢じりの寸法
	struct ARROW_SIZE {
		float m_width = 7.0f;		// 返しの幅
		float m_length = 16.0f;		// 付け根から先端までの長さ
		float m_offset = 0.0f;		// 先端のオフセット
	};

	// 破線の配置
	union STROKE_PATT {
		float m_[6] = { 4.0F, 3.0F, 1.0F, 3.0F, 1.0F, 3.0F };
	};

	enum struct WCHAR_CPY {
		EXACT,
		ESC_CHR,
		CHR_ESC
	};

	// 色成分の最大値
	constexpr double COLOR_MAX = 255.0;
	// 1 インチあたりのポイント数
	constexpr double PT_PER_INCH = 72.0;
	// 1 インチあたりのミリメートル数
	constexpr double MM_PER_INCH = 25.4;
	// 方眼線の濃さ
	constexpr float GRID_GRAY = 0.25f;

	//------------------------------
	// shape.cpp
	//------------------------------

	// 部位 (方形) を表示する.
	void anchor_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx);
	// 部位 (円) を表示する.
	void anchor_draw_rounded(const D2D1_POINT_2F& c_pos, SHAPE_DX& dx);
	// 矢じりの寸法が同じか調べる.
	bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept;
	// 矢じりの形式が同じか調べる.
	bool equal(const ARROW_STYLE a, const ARROW_STYLE b) noexcept;
	// ブール値が同じか調べる.
	bool equal(const bool a, const bool b) noexcept;
	// 色が同じか調べる.
	bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept;
	// 破線の形式が同じか調べる.
	bool equal(const D2D1_DASH_STYLE a, const D2D1_DASH_STYLE b) noexcept;
	// 位置が同じか調べる.
	bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept;
	// 寸法が同じか調べる.
	bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept;
	// 倍精度浮動小数が同じか調べる.
	bool equal(const double a, const double b) noexcept;
	// 書体の幅が同じか調べる.
	bool equal(const DWRITE_FONT_STRETCH a, const DWRITE_FONT_STRETCH b) noexcept;
	// 書体の字体が同じか調べる.
	bool equal(const DWRITE_FONT_STYLE a, const DWRITE_FONT_STYLE b) noexcept;
	// 書体の太さが同じか調べる.
	bool equal(const DWRITE_FONT_WEIGHT a, const DWRITE_FONT_WEIGHT b) noexcept;
	// 段落のそろえが同じか調べる.
	bool equal(const DWRITE_PARAGRAPH_ALIGNMENT a, const DWRITE_PARAGRAPH_ALIGNMENT b) noexcept;
	// 文字列のそろえが同じか調べる.
	bool equal(const DWRITE_TEXT_ALIGNMENT a, const DWRITE_TEXT_ALIGNMENT b) noexcept;
	// 文字範囲が同じか調べる.
	bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept;
	// 単精度浮動小数が同じか調べる.
	bool equal(const float a, const float b) noexcept;
	// 方眼の形式が同じか調べる.
	bool equal(const GRID_PATT a, const GRID_PATT b) noexcept;
	// 方眼線の表示が同じか調べる.
	bool equal(const GRID_SHOW a, const GRID_SHOW b) noexcept;
	// 破線の配置が同じか調べる.
	bool equal(const STROKE_PATT& a, const STROKE_PATT& b) noexcept;
	// 32 ビット整数が同じか調べる.
	bool equal(const uint32_t a, const uint32_t b) noexcept;
	// ワイド文字列が同じか調べる.
	bool equal(const wchar_t* a, const wchar_t* b) noexcept;
	// winrt 文字列が同じか調べる.
	bool equal(winrt::hstring const& a, const wchar_t* b) noexcept;
	// 矢じりの軸と寸法をもとに返しの位置を計算する.
	void get_arrow_barbs(const D2D1_POINT_2F axis, const double axis_len, const double barbWidth, const double barbLen, D2D1_POINT_2F barbs[]) noexcept;
	// 色が不透明か調べる.
	bool is_opaque(const D2D1_COLOR_F& color) noexcept;
	// ベクトルの長さ (の自乗値) を得る
	double pt_abs2(const D2D1_POINT_2F a) noexcept;
	// 位置に位置を足す
	void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// 位置にスカラー値を足す
	void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// 位置にX軸とY軸の値を足す
	void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& c) noexcept;
	// 二点の中点を得る.
	void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// 二点で囲まれた方形を得る.
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// 二点の内積を得る.
	double pt_dot(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept;
	// 部位が原点を含むか調べる.
	bool pt_in_anch(const D2D1_POINT_2F a_pos, const double a_len) noexcept;
	// 部位が位置を含むか調べる.
	bool pt_in_anch(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos, const double a_len) noexcept;
	// だ円が位置を含むか調べる.
	bool pt_in_elli(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept;
	// 線分が位置を含むか, 太さも考慮して調べる.
	bool pt_in_line(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width) noexcept;
	// 四へん形が位置を含むか調べる.
	bool pt_in_quad(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_pos[]) noexcept;
	// 方形が位置を含むか調べる.
	bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept;
	// 指定した位置を含むよう, 方形を拡大する.
	void pt_inc(const D2D1_POINT_2F a, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept;
	// 二点を比べてそれぞれの大きい値を持つ位置を得る.
	void pt_max(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min) noexcept;
	// 二点を比べてそれぞれの小さい値を持つ位置を得る.
	void pt_min(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min) noexcept;
	// 位置をスカラー倍に丸める.
	void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& round) noexcept;
	// 位置にスカラー値を掛け, 別の位置を足す.
	void pt_scale(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& scale) noexcept;
	// 位置にスカラー値を掛ける.
	void pt_scale(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& scale) noexcept;
	// 寸法に値を掛ける.
	void pt_scale(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& scale) noexcept;
	// 点にスカラー値を掛け, 別の位置を足す.
	void pt_scale(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// 位置から位置を引く.
	void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& sub) noexcept;
	// 位置から大きさを引く.
	void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& sub) noexcept;
	// 矢じりの寸法を読み込む.
	void read(ARROW_SIZE& value, DataReader const& dt_reader);
	// 矢じりの形式をデータリーダーから読み込む.
	void read(ARROW_STYLE& value, DataReader const& dt_reader);
	// ブール値をデータリーダーから読み込む.
	void read(bool& value, DataReader const& dt_reader);
	// 倍精度浮動小数をデータリーダーから読み込む.
	void read(double& value, DataReader const& dt_reader);
	// 色をデータリーダーから読み込む.
	void read(D2D1_COLOR_F& value, DataReader const& dt_reader);
	// 破線の形式をデータリーダーから読み込む.
	void read(D2D1_DASH_STYLE& value, DataReader const& dt_reader);
	// 位置をデータリーダーから読み込む.
	void read(D2D1_POINT_2F& value, DataReader const& dt_reader);
	// 寸法をデータリーダーから読み込む.
	void read(D2D1_SIZE_F& value, DataReader const& dt_reader);
	// 書体の字体をデータリーダーから読み込む.
	void read(DWRITE_FONT_STYLE& value, DataReader const& dt_reader);
	// 書体の太さをデータリーダーから読み込む.
	void read(DWRITE_FONT_WEIGHT& value, DataReader const& dt_reader);
	// 書体の伸縮をデータリーダーから読み込む.
	void read(DWRITE_FONT_STRETCH& value, DataReader const& dt_reader);
	// 段落のそろえをデータリーダーから読み込む.
	void read(DWRITE_PARAGRAPH_ALIGNMENT& value, DataReader const& dt_reader);
	// 文字列のそろえをデータリーダーから読み込む.
	void read(DWRITE_TEXT_ALIGNMENT& value, DataReader const& dt_reader);
	// 文字範囲をデータリーダーから読み込む.
	void read(DWRITE_TEXT_RANGE& value, DataReader const& dt_reader);
	// 方眼の形式をデータリーダーから読み込む.
	void read(GRID_PATT& value, DataReader const& dt_reader);
	// 方眼の表示をデータリーダーから読み込む.
	void read(GRID_SHOW& value, DataReader const& dt_reader);
	// 破線の配置をデータリーダーから読み込む.
	void read(STROKE_PATT& value, DataReader const& dt_reader);
	// 32 ビット整数をデータリーダーから読み込む.
	void read(uint32_t& value, DataReader const& dt_reader);
	// 文字列をデータリーダーから読み込む.
	void read(wchar_t*& value, DataReader const& dt_reader);
	// 文字列を複製する. 元の文字列がヌルポインター, または元の文字数が 0 のときは, ヌルポインターを返す.
	wchar_t* wchar_cpy(const wchar_t* const s, const bool exact = true);
	// 文字列の長さ. 引数がヌルポインタの場合, 0 を返す.
	uint32_t wchar_len(const wchar_t* const t) noexcept;
	// 矢じりの寸法をデータライターに書き込む.
	void write(const ARROW_SIZE& value, DataWriter const& dt_writer);
	// 矢じりの形式をデータライターに書き込む.
	void write(const ARROW_STYLE value, DataWriter const& dt_writer);
	// ブール値をデータライターに書き込む.
	void write(const bool value, DataWriter const& dt_writer);
	// 色をデータライターに書き込む.
	void write(const D2D1_COLOR_F& value, DataWriter const& dt_writer);
	// 破線の形式をデータライターに書き込む.
	void write(const D2D1_DASH_STYLE value, DataWriter const& dt_writer);
	// 位置をデータライターに書き込む.
	void write(const D2D1_POINT_2F value, DataWriter const& dt_writer);
	// 寸法をデータライターに書き込む.
	void write(const D2D1_SIZE_F value, DataWriter const& dt_writer);
	// 倍精度浮動小数をデータライターに書き込む.
	void write(const double value, DataWriter const& dt_writer);
	// 書体の字体をデータライターに書き込む.
	void write(const DWRITE_FONT_STYLE value, DataWriter const& dt_writer);
	// 書体の伸縮をデータライターに書き込む.
	void write(const DWRITE_FONT_STRETCH value, DataWriter const& dt_writer);
	// 書体の太さをデータライターに書き込む.
	void write(const DWRITE_FONT_WEIGHT value, DataWriter const& dt_writer);
	// 段落のそろえをデータライターに書き込む.
	void write(const DWRITE_PARAGRAPH_ALIGNMENT value, DataWriter const& dt_writer);
	// 文字列のそろえをデータライターに書き込む.
	void write(const DWRITE_TEXT_ALIGNMENT value, DataWriter const& dt_writer);
	// 文字列範囲をデータライターに書き込む.
	void write(const DWRITE_TEXT_RANGE value, DataWriter const& dt_writer);
	// 方眼の配置をデータライターに書き込む.
	void write(const GRID_PATT value, DataWriter const& dt_writer);
	// 方眼の表示をデータライターに書き込む.
	void write(const GRID_SHOW value, DataWriter const& dt_writer);
	// 破線の配置をデータライターに書き込む.
	void write(const STROKE_PATT& value, DataWriter const& dt_writer);
	// 32 ビット整数をデータライターに書き込む.
	void write(const uint32_t value, DataWriter const& dt_writer);
	// 文字列をデータライターに書き込む.
	void write(const wchar_t* value, DataWriter const& dt_writer);
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
	void write_svg(const D2D1_DASH_STYLE style, const STROKE_PATT& patt, const double width, DataWriter const& dt_writer);

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
		// 矢じりの寸法を得る
		virtual bool get_arrow_size(ARROW_SIZE& /*val*/) const noexcept { return false; }
		// 矢じりの形式を得る.
		virtual bool get_arrow_style(ARROW_STYLE& /*val*/) const noexcept { return false; }
		// 図形を囲む領域を得る.
		virtual void get_bound(D2D1_POINT_2F& /*b_min*/, D2D1_POINT_2F& /*b_max*/) const noexcept {}
		// 角丸半径を得る.
		virtual bool get_corner_radius(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// 塗りつぶし色を得る.
		virtual bool get_fill_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 書体の色を得る.
		virtual bool get_font_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 書体名を得る.
		virtual bool get_font_family(wchar_t*& /*val*/) const noexcept { return false; }
		// 書体の大きさを得る.
		virtual bool get_font_size(double& /*val*/) const noexcept { return false; }
		// 書体の横幅を得る.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH& /*val*/) const noexcept { return false; }
		// 書体の字体を得る.
		virtual bool get_font_style(DWRITE_FONT_STYLE& /*val*/) const noexcept { return false; }
		// 書体の太さを得る.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT& /*val*/) const noexcept { return false; }
		// 方眼の基準の大きさを得る.
		virtual bool get_grid_base(double& /*val*/) const noexcept { return false; }
		// 方眼の大きさを得る.
		virtual bool get_grid_gray(double& /*val*/) const noexcept { return false; }
		// 方眼の形式を得る.
		virtual bool get_grid_patt(GRID_PATT& /*val*/) const noexcept { return false; }
		// 方眼の表示を得る.
		virtual bool get_grid_show(GRID_SHOW& /*val*/) const noexcept { return false; }
		// 方眼の表示を得る.
		virtual bool get_grid_snap(bool& /*val*/) const noexcept { return false; }
		// 図形を囲む領域の左上位置を得る.
		virtual void get_min_pos(D2D1_POINT_2F& /*val*/) const noexcept {}
		// ページの色を得る.
		virtual bool get_page_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// ページの拡大率を得る.
		virtual bool get_page_scale(double& /*val*/) const noexcept { return false; }
		// ページの大きさを得る.
		virtual bool get_page_size(D2D1_SIZE_F& /*val*/) const noexcept { return false; }
		// 指定された部位の位置を得る.
		virtual	void get_pos(const ANCH_WHICH /*a*/, D2D1_POINT_2F&/*val*/) const noexcept {}
		// 始点を得る
		virtual bool get_start_pos(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// 線枠の色を得る.
		virtual bool get_stroke_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 破線の配置を得る.
		virtual bool get_stroke_patt(STROKE_PATT& /*val*/) const noexcept { return false; }
		// 破線の形式を得る.
		virtual bool get_stroke_style(D2D1_DASH_STYLE& /*val*/) const noexcept { return false; }
		// 書体の太さを得る
		virtual bool get_stroke_width(double& /*val*/) const noexcept { return false; }
		// 文字列を得る.
		virtual bool get_text(wchar_t*& /*val*/) const noexcept { return false; }
		// 段落のそろえを得る.
		virtual bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& /*val*/) const noexcept { return false; }
		// 文字列のそろえを得る.
		virtual bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& /*val*/) const noexcept { return false; }
		// 行の高さを得る.
		virtual bool get_text_line_height(double& /*val*/) const noexcept { return false; }
		// 文字列の周囲の余白を得る.
		virtual bool get_text_margin(D2D1_SIZE_F& /*val*/) const noexcept { return false; }
		// 文字範囲を得る
		virtual bool get_text_range(DWRITE_TEXT_RANGE& /*val*/) const noexcept { return false; }
		// 位置を含むか調べる.
		virtual ANCH_WHICH hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept { return ANCH_WHICH::ANCH_OUTSIDE; }
		// 範囲に含まれるか調べる.
		virtual bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept { return false; }
		// 消去フラグを調べる.
		virtual bool is_deleted(void) const noexcept { return false; }
		// 選択フラグを調べる.
		virtual bool is_selected(void) const noexcept { return false; }
		// 差分だけ移動する.
		virtual	void move(const D2D1_POINT_2F /*d*/) {}
		// 値を矢じりの寸法に格納する.
		virtual void set_arrow_size(const ARROW_SIZE& /*val*/) {}
		// 値を矢じりの形式に格納する.
		virtual void set_arrow_style(const ARROW_STYLE /*val*/) {}
		// 値を消去フラグに格納する.
		virtual void set_delete(const bool /*val*/) noexcept {}
		// 値を塗りつぶし色に格納する.
		virtual void set_fill_color(const D2D1_COLOR_F& /*val*/) noexcept {}
		// 値を書体の色に格納する.
		virtual void set_font_color(const D2D1_COLOR_F& /*val*/) noexcept {}
		// 値を書体名に格納する.
		virtual void set_font_family(wchar_t* const /*val*/) {}
		// 値を書体の大きさに格納する.
		virtual void set_font_size(const double /*val*/) {}
		// 値を書体の横幅に格納する.
		virtual void set_font_stretch(const DWRITE_FONT_STRETCH /*val*/) {}
		// 値を書体の字体に格納する.
		virtual void set_font_style(const DWRITE_FONT_STYLE /*val*/) {}
		// 値を書体の太さに格納する.
		virtual void set_font_weight(const DWRITE_FONT_WEIGHT /*val*/) {}
		// 値を方眼の大きさに格納する.
		virtual void set_grid_base(const double /*val*/) noexcept {}
		// 値を方眼の濃淡に格納する.
		virtual void set_grid_gray(const double /*val*/) noexcept {}
		// 値を方眼の形式に格納する.
		virtual void set_grid_patt(const GRID_PATT /*val*/) noexcept {}
		// 値を方眼の表示に格納する.
		virtual void set_grid_show(const GRID_SHOW /*val*/) noexcept {}
		// 値を方眼への揃えに格納する.
		virtual void set_grid_snap(const bool /*val*/) noexcept {}
		// 値をページの色に格納する.
		virtual void set_page_color(const D2D1_COLOR_F& /*val*/) noexcept {}
		// 値をページの拡大率に格納する.
		virtual void set_page_scale(const double /*val*/) noexcept {}
		// 値をページの大きさに格納する.
		virtual void set_page_size(const D2D1_SIZE_F /*val*/) noexcept {}
		// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
		virtual void set_pos(const D2D1_POINT_2F /*val*/, const ANCH_WHICH /*a*/) {}
		// 値を選択フラグに格納する.
		virtual void set_select(const bool /*val*/) noexcept {}
		// 値を線枠の色に格納する.
		virtual void set_stroke_color(const D2D1_COLOR_F& /*val*/) noexcept {}
		// 値を破線の配置に格納する.
		virtual void set_stroke_patt(const STROKE_PATT& /*val*/) {}
		// 値を線枠の形式に格納する.
		virtual void set_stroke_style(const D2D1_DASH_STYLE /*val*/) {}
		// 値を書体の太さに格納する.
		virtual void set_stroke_width(const double /*val*/) noexcept {}
		// 値を文字列に格納する.
		virtual void set_text(wchar_t* const /*val*/) {}
		// 値を段落のそろえに格納する.
		virtual void set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT /*val*/) {}
		// 値を文字列のそろえに格納する.
		virtual void set_text_align_t(const DWRITE_TEXT_ALIGNMENT /*val*/) {}
		// 値を行間に格納する.
		virtual void set_text_line_height(const double /*val*/) {}
		// 値を文字列の余白に格納する.
		virtual void set_text_margin(const D2D1_SIZE_F /*val*/) {}
		// 値を文字範囲に格納する.
		virtual void set_text_range(const DWRITE_TEXT_RANGE /*val*/) {}
		// 値を始点に格納する. 他の部位の位置も動く.
		virtual void set_start_pos(const D2D1_POINT_2F /*val*/) {}
		// データライターに書き込む.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
		// データライターに SVG として書き込む.
		virtual void write_svg(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// shape_list.cpp
	// 図形リストに関連した処理
	//------------------------------

	typedef std::list<struct Shape*>	S_LIST_T;
	// 最後の図形をリストから得る.
	Shape* s_list_back(S_LIST_T const& s_list) noexcept;
	// 図形リストを消去し, 含まれる図形を破棄する.
	void s_list_clear(S_LIST_T& s_list) noexcept;
	// 図形のリスト上での位置を得る.
	uint32_t s_list_distance(S_LIST_T const& s_list, const Shape* s) noexcept;
	// 最初の図形をリストから得る.
	Shape* s_list_front(S_LIST_T const& s_list) noexcept;
	// 図形全体の領域をリストから得る.
	void s_list_bound(S_LIST_T const& s_list, const D2D1_SIZE_F p_size, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// 図形全体の領域をリストから得る.
	void s_list_bound(S_LIST_T const& s_list, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// 位置を含む図形とその部位をリストから得る.
	ANCH_WHICH s_list_hit_test(S_LIST_T const& s_list, const D2D1_POINT_2F t_pos, const double a_len, Shape*& s) noexcept;
	// 図形をリストに挿入する.
	void s_list_insert(S_LIST_T& s_list, Shape* s_ins, const Shape* s_pos) noexcept;
	// 選択フラグの立つすべての図形を差分だけ移動する.
	void s_list_move(S_LIST_T const& s_list, const D2D1_POINT_2F d_pos) noexcept;
	// 次の図形をリストから得る.
	Shape* s_list_next(S_LIST_T const& s_list, const Shape* s) noexcept;
	// 前の図形をリストから得る.
	Shape* s_list_prev(S_LIST_T const& s_list, const Shape* s) noexcept;
	// 図形リストをデータリーダーから読み込む.
	bool s_list_read(S_LIST_T& s_list, DataReader const& dt_reader);
	// 図形をリストから削除し, 削除した図形の次の図形を得る.
	Shape* s_list_remove(S_LIST_T& s_list, const Shape* s) noexcept;
	// 選択された図形のリストを得る.
	template <typename S> void s_list_selected(S_LIST_T const& s_list, S_LIST_T& sel_list) noexcept;
	// 図形リストをデータライターに書き込む. REDUCE 場合の消去フラグの立つ図形は無視する.
	template <bool REDUCE> void s_list_write(const S_LIST_T& s_list, DataWriter const& dt_writer);
	// リストの中の図形の順番を得る.
	template <typename S, typename T> bool s_list_match(S_LIST_T const& s_list, S s, T& t);
	// 選択された図形から, それらを全て合わせた文字列を得る.
	winrt::hstring s_list_text_selected_all(S_LIST_T const& s_list) noexcept;

	//------------------------------
	// レイアウト
	//------------------------------
	struct ShapeLayout : Shape {

		// 図形の属性
		ARROW_SIZE m_arrow_size{ 7.0f, 16.0f, 0.0f };	// 矢じりの寸法
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// 矢じりの形式
		D2D1_POINT_2F m_corner_rad{ 0.0f, 0.0f };	// 角丸半径
		D2D1_COLOR_F m_fill_color = S_WHITE;	// 塗りつぶしの色
		D2D1_COLOR_F m_font_color = S_BLACK;	// 書体の色 (MainPage のコンストラクタで設定)
		wchar_t* m_font_family = nullptr;	// 書体名
		double m_font_size = 12.0 * 96.0 / 72.0;	// 書体の大きさ
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH_UNDEFINED;	// 書体の伸縮
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// 書体の字体
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ
		D2D1_COLOR_F m_stroke_color = S_BLACK;	// 線枠の色 (MainPage のコンストラクタで設定)
		STROKE_PATT m_stroke_patt{ 4.0f, 3.0f, 1.0f, 3.0f };	// 破線の配置
		D2D1_DASH_STYLE m_stroke_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// 破線の形式
		double m_stroke_width = 1.0;	// 線枠の太さ
		double m_text_line = 0.0;	// 行間 (DIPs 96dpi固定)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// 段落の揃え
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// 文字列の揃え
		D2D1_SIZE_F m_text_margin{ 4.0f, 4.0f };	// 文字列の左右と上下の余白

		// 方眼の属性
		double m_grid_base = 0.0;	// 方眼の基準の大きさ (を -1 した値)
		double m_grid_gray = GRID_GRAY;	// 方眼線の濃さ
		GRID_SHOW m_grid_show = GRID_SHOW::BACK;	// 方眼線の表示
		GRID_PATT m_grid_patt = GRID_PATT::PATT_1;	// 方眼の形式
		bool m_grid_snap = true;	// 方眼に整列

		// ページの属性
		D2D1_COLOR_F m_page_color = S_WHITE;	// 背景色 (MainPage のコンストラクタで設定)
		double m_page_scale = 1.0;	// ページの拡大率
		D2D1_SIZE_F	m_page_size{ 8.27f * 96.0f, 11.69f * 96.0f };	// ページの大きさ (MainPage のコンストラクタで設定)

		//------------------------------
		// shape_layout.cpp
		//------------------------------

		// 曲線の補助線を表示する.
		void draw_auxiliary_bezi(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// だ円の補助線を表示する.
		void draw_auxiliary_elli(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// 直線の補助線を表示する.
		void draw_auxiliary_line(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// 方形の補助線を表示する.
		void draw_auxiliary_rect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// 四辺形の補助線を表示する.
		void draw_auxiliary_quad(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// 角丸方形の補助線を表示する.
		void draw_auxiliary_rrect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// 方眼線を表示する,
		void draw_grid(SHAPE_DX const& dx, const D2D1_POINT_2F offset);
		// 部位の色を得る.
		//void get_anchor_color(D2D1_COLOR_F& value) const noexcept;
		// 矢じりの寸法を得る.
		bool get_arrow_size(ARROW_SIZE& value) const noexcept;
		// 矢じりの形式を得る.
		bool get_arrow_style(ARROW_STYLE& value) const noexcept;
		// 方眼の基準の大きさを得る.
		bool get_grid_base(double& value) const noexcept;
		// 方眼線の色を得る.
		void get_grid_color(D2D1_COLOR_F& value) const noexcept;
		// 方眼の大きさを得る.
		bool get_grid_gray(double& value) const noexcept;
		// 方眼の形式を得る.
		bool get_grid_patt(GRID_PATT& value) const noexcept;
		// 方眼の表示の状態を得る.
		bool get_grid_show(GRID_SHOW& value) const noexcept;
		// 方眼へのそろえを得る.
		bool get_grid_snap(bool& value) const noexcept;
		// ページの色を得る.
		bool get_page_color(D2D1_COLOR_F& value) const noexcept;
		// ページの色を得る.
		bool get_page_size(D2D1_SIZE_F& value) const noexcept;
		// ページの拡大率を得る.
		bool get_page_scale(double& value) const noexcept;
		// 角丸半径を得る.
		bool get_corner_radius(D2D1_POINT_2F& value) const noexcept;
		// 塗りつぶし色を得る.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// 書体の色を得る.
		bool get_font_color(D2D1_COLOR_F& value) const noexcept;
		// 書体名を得る.
		bool get_font_family(wchar_t*& value) const noexcept;
		// 書体の大きさを得る.
		bool get_font_size(double& value) const noexcept;
		// 書体の横幅を得る.
		bool get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept;
		// 書体の字体を得る.
		bool get_font_style(DWRITE_FONT_STYLE& value) const noexcept;
		// 書体の太さを得る.
		bool get_font_weight(DWRITE_FONT_WEIGHT& value) const noexcept;
		// 行間を得る.
		bool get_text_line_height(double& value) const noexcept;
		// 文字列の周囲の余白を得る.
		bool get_text_margin(D2D1_SIZE_F& value) const noexcept;
		// 段落のそろえを得る.
		bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& value) const noexcept;
		// 線枠の色を得る.
		bool get_stroke_color(D2D1_COLOR_F& value) const noexcept;
		// 破線の配置を得る.
		bool get_stroke_patt(STROKE_PATT& value) const noexcept;
		// 破線の形式を得る.
		bool get_stroke_style(D2D1_DASH_STYLE& value) const noexcept;
		// 書体の太さを得る
		bool get_stroke_width(double& value) const noexcept;
		// 文字列のそろえを得る.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& value) const noexcept;
		// データリーダーから読み込む.
		void read(DataReader const& dt_reader);
		// 図形の属性値を格納する.
		void set_to(Shape* s) noexcept;
		// 値を方眼の基準の大きさに格納する.
		void set_grid_base(const double value) noexcept;
		// 値を方眼の濃淡に格納する.
		void set_grid_gray(const double value) noexcept;
		// 値を方眼の表示に格納する.
		void set_grid_patt(const GRID_PATT value) noexcept;
		// 値を方眼の表示に格納する.
		void set_grid_show(const GRID_SHOW value) noexcept;
		// 値を方眼へのそろえに格納する.
		void set_grid_snap(const bool value) noexcept;
		// 値をページの色に格納する.
		void set_page_color(const D2D1_COLOR_F& value) noexcept;
		// 値をページの寸法に格納する.
		void set_page_size(const D2D1_SIZE_F value) noexcept;
		// 値をページの拡大率に格納する.
		void set_page_scale(const double value) noexcept;
		// 値を矢じりの寸法に格納する.
		void set_arrow_size(const ARROW_SIZE& value);
		// 値を矢じりの形式に格納する.
		void set_arrow_style(const ARROW_STYLE value);
		// 値を塗りつぶし色に格納する.
		void set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// 値を書体の色に格納する.
		void set_font_color(const D2D1_COLOR_F& value) noexcept;
		// 書体名に格納する.
		void set_font_family(wchar_t* const value);
		// 書体の大きさに格納する.
		void set_font_size(const double value);
		// 値を書体の伸縮に格納する.
		void set_font_stretch(const DWRITE_FONT_STRETCH value);
		// 値を書体の字体に格納する.
		void set_font_style(const DWRITE_FONT_STYLE value);
		// 値を書体の太さに格納する.
		void set_font_weight(const DWRITE_FONT_WEIGHT value);
		// 値を行間に格納する.
		void set_text_line_height(const double value);
		// 値を文字列の余白に格納する.
		void set_text_margin(const D2D1_SIZE_F value);
		// 値を段落のそろえに格納する.
		void set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value);
		// 値を線枠の色に格納する.
		void set_stroke_color(const D2D1_COLOR_F& value) noexcept;
		// 値を破線の配置に格納する.
		void set_stroke_patt(const STROKE_PATT& value);
		// 値を線枠の形式に格納する.
		void set_stroke_style(const D2D1_DASH_STYLE value);
		// 値を書体の太さに格納する.
		void set_stroke_width(const double value) noexcept;
		// 値を文字列のそろえに格納する.
		void set_text_align_t(const DWRITE_TEXT_ALIGNMENT value);
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
		void get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// 図形を囲む領域の左上位置を得る.
		void get_min_pos(D2D1_POINT_2F& value) const noexcept;
		// 始点を得る
		bool get_start_pos(D2D1_POINT_2F& value) const noexcept;
		// 文字列図形を含むか調べる.
		bool has_text(void) noexcept;
		// 位置を含むか調べる.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか調べる.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 消去フラグを調べる.
		bool is_deleted(void) const noexcept;
		// 選択フラグを調べる.
		bool is_selected(void) const noexcept;
		// 差分だけ移動する
		void move(const D2D1_POINT_2F d_pos);
		// 値を消去フラグに格納する.
		void set_delete(const bool value) noexcept;
		// 値を選択フラグに格納する.
		void set_select(const bool value) noexcept;
		// 値を始点に格納する. 他の部位の位置も動く.
		void set_start_pos(const D2D1_POINT_2F value);
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
		bool m_selected = true;	// 選択フラグ
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// 開始位置
		D2D1_POINT_2F m_diff{ 0.0f, 0.0f };	// 終了位置への差分
		D2D1_COLOR_F m_stroke_color;	// 線枠の色
		STROKE_PATT m_stroke_patt{};	// 破線の配置
		D2D1_DASH_STYLE m_stroke_style;	// 破線の形式
		double m_stroke_width;	// 線枠の太さ
		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_stroke_style{};	// D2D ストロークスタイル

		//------------------------------
		// shape_stroke.cpp
		//------------------------------

		// 図形を破棄する.
		~ShapeStroke(void);

		// 図形を囲む領域を得る.
		void get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// 図形を囲む領域の左上位置を得る.
		virtual void get_min_pos(D2D1_POINT_2F& value) const noexcept;
		// 指定された部位の位置を得る.
		virtual	void get_pos(const ANCH_WHICH /*a*/, D2D1_POINT_2F& value) const noexcept;
		// 始点を得る
		virtual bool get_start_pos(D2D1_POINT_2F& value) const noexcept;
		// 線枠の色を得る.
		bool get_stroke_color(D2D1_COLOR_F& value) const noexcept;
		// 破線の配置を得る.
		bool get_stroke_patt(STROKE_PATT& value) const noexcept;
		// 破線の形式を得る.
		bool get_stroke_style(D2D1_DASH_STYLE& value) const noexcept;
		// 線枠の太さを得る.
		bool get_stroke_width(double& value) const noexcept;
		// 位置を含むか調べる.
		ANCH_WHICH hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept;
		// 範囲に含まれるか調べる.
		bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept;
		// 消去フラグを調べる.
		bool is_deleted(void) const noexcept { return m_deleted; }
		// 選択フラグを調べる.
		bool is_selected(void) const noexcept { return m_selected; }
		// 差分だけ移動する.
		virtual	void move(const D2D1_POINT_2F d_pos);
		// 値を選択フラグに格納する.
		void set_select(const bool value) noexcept { m_selected = value; }
		// 値を消去フラグに格納する.
		void set_delete(const bool value) noexcept { m_deleted = value; }
		// 値を始点に格納する. 他の部位の位置も動く.
		virtual void set_start_pos(const D2D1_POINT_2F value);
		// 値を線枠の色に格納する.
		void set_stroke_color(const D2D1_COLOR_F& value) noexcept;
		// 値を破線の配置に格納する.
		void set_stroke_patt(const STROKE_PATT& value);
		// 値を線枠の形式に格納する.
		void set_stroke_style(const D2D1_DASH_STYLE value);
		// 値を線枠の太さに格納する.
		void set_stroke_width(const double width) noexcept;
		// 図形を作成する.
		ShapeStroke(const ShapeLayout* attr);
		// 図形をデータリーダーから読み込む.
		ShapeStroke(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 矢じりをデータライターに SVG タグとして書き込む.
		void write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, const ARROW_STYLE a_style, DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 直線
	//------------------------------
	struct ShapeLine : ShapeStroke {
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// 矢じりの形式
		ARROW_SIZE m_arrow_size{};	// 矢じりの寸法
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_arrow_geometry{};	// 矢じりの D2D パスジオメトリ

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// 図形を作成する.
		ShapeLine(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, const ShapeLayout* attr);
		// 図形をデータリーダーから読み込む.
		ShapeLine(DataReader const& dt_reader);
		// 図形を破棄する.
		~ShapeLine(void);
		// 表示する.
		void draw(SHAPE_DX& dx);
		// 位置を含むか調べる.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか調べる.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 矢じりの寸法を得る.
		bool get_arrow_size(ARROW_SIZE& size) const noexcept;
		// 矢じりの形式を得る.
		bool get_arrow_style(ARROW_STYLE& value) const noexcept;
		// 値を矢じりの寸法に格納する.
		void set_arrow_size(const ARROW_SIZE& value);
		// 値を矢じりの形式に格納する.
		void set_arrow_style(const ARROW_STYLE value);
		// 指定された部位の位置を得る.
		void get_pos(const ANCH_WHICH a, D2D1_POINT_2F& value) const noexcept;
		// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
		void set_pos(const D2D1_POINT_2F value, const ANCH_WHICH a);
		// 値を始点に格納する. 他の部位の位置も動く.
		void set_start_pos(const D2D1_POINT_2F value);
		// 差分だけ移動する.
		void move(const D2D1_POINT_2F d_pos);
		// データリーダーから読み込む.
		void read(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 方形
	//------------------------------
	struct ShapeRect : ShapeStroke {
		D2D1_COLOR_F m_fill_color;		// 塗りつぶし

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// 図形を作成する.
		ShapeRect(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, const ShapeLayout* s_attr);
		// 図形をデータリーダーから読み込む.
		ShapeRect(DataReader const& dt_reader);
		// 表示する.
		void draw(SHAPE_DX& dx);
		// 位置を含むか調べる.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 位置を含むか調べる.
		ANCH_WHICH hit_test_anchor(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか調べる.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 塗りつぶしの色を得る.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// 値を塗りつぶしの色に格納する.
		void set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// 指定された部位の位置を得る.
		void get_pos(const ANCH_WHICH a, D2D1_POINT_2F& value) const noexcept;
		// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
		void set_pos(const D2D1_POINT_2F value, const ANCH_WHICH a);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 目盛り
	// 作成したあとで文字列の属性の変更はできない.
	//------------------------------
	struct ShapeScale : ShapeRect {
		double m_grid_base;	// 方眼の大きさ (を -1 した値)
		winrt::com_ptr<IDWriteTextFormat> m_dw_text_format{};	// テキストフォーマット

		//------------------------------
		// shape_scale.cpp
		//------------------------------

		// 図形を破棄する.
		~ShapeScale(void);
		// 図形を表示する.
		void draw(SHAPE_DX& dx);
		// 位置を含むか調べる.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 図形を作成する.
		ShapeScale(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, const ShapeLayout* attr);
		// 図形をデータリーダーから読み込む.
		ShapeScale(DataReader const& dt_reader);
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
		ShapeElli(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, const ShapeLayout* attr) :
			ShapeRect::ShapeRect(s_pos, d_pos, attr)
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
		// 位置を含むか調べる.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 角丸方形
	//------------------------------
	struct ShapeRRect : ShapeRect {
		D2D1_POINT_2F m_corner_rad;		// 角丸部分の半径

		//------------------------------
		// shape_rrect.cpp
		// 角丸方形
		//------------------------------

		// 図形を作成する.
		ShapeRRect(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, const ShapeLayout* attr);
		// 図形をデータリーダーから読み込む.
		ShapeRRect(DataReader const& dt_reader);
		// 図形を表示する.
		void draw(SHAPE_DX& dx);
		// 位置を含むか調べる.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 角丸半径を得る.
		bool get_corner_radius(D2D1_POINT_2F& value) const noexcept;
		// 指定された部位の位置を得る.
		void get_pos(const ANCH_WHICH a, D2D1_POINT_2F& value) const noexcept;
		// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
		void set_pos(const D2D1_POINT_2F value, const ANCH_WHICH a);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 折れ線のひな型
	//------------------------------
	struct ShapePoly : ShapeStroke {
		D2D1_POINT_2F m_diff_1{ 0.0f, 0.0f };	// 3 番目の頂点への差分
		D2D1_POINT_2F m_diff_2{ 0.0f, 0.0f };	// 4 番目の頂点への差分
		winrt::com_ptr<ID2D1PathGeometry> m_poly_geom{};	// 四辺形のパスジオメトリ

		//------------------------------
		// shape_stroke.cpp
		// 折れ線のひな型
		//------------------------------

		// パスジオメトリを作成する.
		virtual void create_path_geometry(void) {}
		// 図形を作成する.
		ShapePoly(const ShapeLayout* attr);
		// 図形をデータリーダーから読み込む.
		ShapePoly(DataReader const& dt_reader);
		// 図形を破棄する.
		~ShapePoly(void);
		// 図形を囲む領域の左上位置を得る.
		void get_min_pos(D2D1_POINT_2F& value) const noexcept;
		// 図形を囲む領域を得る.
		void get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// 差分だけ移動する.
		void move(const D2D1_POINT_2F d_pos);
		// 指定された部位の位置を得る.
		void get_pos(const ANCH_WHICH a, D2D1_POINT_2F& value) const noexcept;
		// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
		void set_pos(const D2D1_POINT_2F value, const ANCH_WHICH a);
		// 値を始点に格納する. 他の部位の位置も動く.
		void set_start_pos(const D2D1_POINT_2F value);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 四へん形
	//------------------------------
	struct ShapeQuad : ShapePoly {
		D2D1_COLOR_F m_fill_color;

		//------------------------------
		// shape_quad.cpp
		//------------------------------

		// パスジオメトリを作成する.
		void create_path_geometry(void);
		// 表示する
		void draw(SHAPE_DX& dx);
		// 塗りつぶし色を得る.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// 位置を含むか調べる.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか調べる.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 値を塗りつぶし色に格納する.
		void set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// 図形を作成する.
		ShapeQuad(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, const ShapeLayout* attr);
		// 図形をデータリーダーから読み込む.
		ShapeQuad(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& /*dt_writer*/) const;
		// データライターに SVG として書き込む.
		void write_svg(DataWriter const& /*dt_writer*/) const;
	};

	//------------------------------
	// 曲線
	//------------------------------
	struct ShapeBezi : ShapePoly {
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// 矢じりの形式
		ARROW_SIZE m_arrow_size{};	// 矢じりの寸法
		winrt::com_ptr<ID2D1PathGeometry> m_arrow_geom{};	// 矢じりの D2D パスジオメトリ

		~ShapeBezi(void)
		{
			m_arrow_geom = nullptr;
		}

		//------------------------------
		// shape_bezi.cpp
		// ベジェ曲線
		//------------------------------

		// パスジオメトリを作成する.
		void create_path_geometry(void);
		// 表示する.
		void draw(SHAPE_DX& dx);
		// 矢じりの寸法を得る
		bool get_arrow_size(ARROW_SIZE& value) const noexcept;
		// 矢じりの形式を得る.
		bool get_arrow_style(ARROW_STYLE& value) const noexcept;
		// 位置を含むか調べる.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか調べる.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 値を矢じりの寸法に格納する.
		void set_arrow_size(const ARROW_SIZE& value);
		// 値を矢じりの形式に格納する.
		void set_arrow_style(const ARROW_STYLE value);
		// 値を始点に格納する. 他の部位の位置も動く.
		void set_start_pos(const D2D1_POINT_2F value);
		// 図形を作成する.
		ShapeBezi(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, const ShapeLayout* attr);
		// 図形をデータリーダーから読み込む.
		ShapeBezi(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 文字列
	//------------------------------
	struct ShapeText : ShapeRect {
		static wchar_t** s_available_fonts;		// 有効な書体名

		DWRITE_TEXT_RANGE m_sel_range{ 0, 0 };	// 文字範囲
		D2D1_COLOR_F m_font_color = S_BLACK;	// 書体の色
		wchar_t* m_font_family = nullptr;	// 書体名
		double m_font_size = 0.0;	// 書体の大きさ
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH_UNDEFINED;	// 書体の伸縮
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE_NORMAL;	// 書体の字体
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ
		double m_text_line = 0.0;	// 行間 (DIPs 96dpi固定)
		wchar_t* m_text = nullptr;	// 文字列
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// 段落そろえ
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT_LEADING;	// 文字揃え
		D2D1_SIZE_F m_text_margin{ 4.0f, 4.0f };	// 文字列のまわりの上下と左右の余白

		winrt::com_ptr<IDWriteTextLayout> m_dw_text_layout{};	// 文字列を表示するためのレイアウト
		double m_dw_descent = 0.0f;
		UINT32 m_dw_line_cnt = 0;	// 行の計量の要素数
		DWRITE_LINE_METRICS* m_dw_line_metrics = nullptr;	// 行の計量
		UINT32 m_dw_range_cnt = 0;	// 範囲の計量の要素数
		DWRITE_HIT_TEST_METRICS* m_dw_range_metrics = nullptr;	// 範囲の計量
		UINT32 m_dw_test_cnt = 0;	// 位置の計量の要素数
		DWRITE_HIT_TEST_METRICS* m_dw_test_metrics = nullptr;	// 位置の計量

		// 図形を破棄する.
		~ShapeText(void);
		// 大きさを文字列に合わせる.
		bool adjust_bound(const D2D1_SIZE_F& max_size = D2D1_SIZE_F{ 0.0F, 0.0F });
		// テキストレイアウトから計量の配列を得る.
		void create_test_metrics(void);
		// テキストレイアウトを破棄して作成する.
		void create_text_layout(void);
		// 計量を破棄して作成する.
		void create_text_metrics(void);
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
		bool get_font_size(double& value) const noexcept;
		// 書体の伸縮を得る.
		bool get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept;
		// 書体の字体を得る.
		bool get_font_style(DWRITE_FONT_STYLE& value) const noexcept;
		// 行間を得る.
		bool get_text_line_height(double& value) const noexcept;
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
		// 位置を含むか調べる.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// 範囲に含まれるか調べる.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// 有効な書体名か調べ, 有効なら, もともとの書体名を破棄し, 有効な書体名要素へのポインターと置き換える.
		static bool is_available_font(wchar_t*& font);
		// 有効な書体名の配列を破棄する.
		static void release_available_fonts(void);
		// 有効な書体名の配列を設定する.
		static void set_available_fonts(void);
		// 値を書体の色に格納する.
		void set_font_color(const D2D1_COLOR_F& value) noexcept;
		// 値を書体名に格納する.
		void set_font_family(wchar_t* const value);
		// 値を書体の大きさに格納する.
		void set_font_size(const double value);
		// 値を書体の伸縮に格納する.
		void set_font_stretch(const DWRITE_FONT_STRETCH value);
		// 値を書体の字体に格納する.
		void set_font_style(const DWRITE_FONT_STYLE value);
		// 値を書体の太さに格納する.
		void set_font_weight(const DWRITE_FONT_WEIGHT value);
		// 値を行間に格納する.
		void set_text_line_height(const double value);
		// 値を文字列の余白に格納する.
		void set_text_margin(const D2D1_SIZE_F value);
		// 値を段落のそろえに格納する.
		void set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value);
		// 値を指定した部位の位置に格納する. 他の部位の位置は動かない. 
		void set_pos(const D2D1_POINT_2F value, const ANCH_WHICH a);
		// 値を文字列に格納する.
		void set_text(wchar_t* const value);
		// 値を文字列のそろえに格納する.
		void set_text_align_t(const DWRITE_TEXT_ALIGNMENT value);
		// 値を文字範囲に格納する.
		void set_text_range(const DWRITE_TEXT_RANGE value);
		// 図形を作成する.
		ShapeText(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, wchar_t* const text, const ShapeLayout* attr);
		// 図形をデータリーダーから読み込む.
		ShapeText(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに SVG タグとして書き込む.
		void write_svg(DataWriter const& dt_writer) const;
	};

}