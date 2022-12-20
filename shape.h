#pragma once
//------------------------------
// shape.h
// shape.cpp	図形のひな型, その他
// shape_bezi.cpp	ベジェ曲線
// shape_dt.cpp	読み込み, 書き込み.
// shape_elli.cpp	だ円
// shape_group.cpp	グループ
// shape_image.cpp	画像
// shape_line.cpp	直線 (矢じるしつき)
// shape_path.cpp	折れ線のひな型
// shape_pdf.cpp	PDF への書き込み
// shape_poly.cpp	多角形
// shape.rect.cpp	方形
// shape_rrect.cpp	角丸方形
// shape_ruler.cpp	定規
// shape_sheet.cpp	用紙
// shape_slist.cpp	図形リスト
// shape_stroke.cpp	線枠のひな型
// shape_svg.cpp	SVG への書き込み
// shape_text.cpp	文字列
//------------------------------
#include <list>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include "d2d_ui.h"
//
// +-------------+
// | Shape*      |
// +------+------+
//        |
//        +---------------+---------------+
//        |               |               |
// +------+------+ +------+------+ +------+------+
// | ShapeSelect*| | ShapeGroup  | | ShapeSheet  |
// +------+------+ +-------------+ | .D2D_UI -------------> SwapChainPanel
//        |                        +-------------+
//        +---------------+
//        |               |
// +------+------+ +------+------+
// | ShapeStroke*| | ShapeImage  |
// +------+------+ +-------------+
//        |
//        +-------------------------------+
//        |                               |
// +------+------+                 +------+------+
// | ShapeLine*  |                 | ShapeRect*  |
// +------+------+                 +------+------+
//        |                               |
// +------+------+                        |
// | ShapePath*  |                        |
// +------+------+                        |
//        |                               |
//        +---------------+               +---------------+---------------+---------------+
//        |               |               |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+
// | ShapePoly   | | ShapeBezi   | | ShapeElli   | | ShapeRRect  | | ShapeText   | | ShapeRuler  |
// +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+
//
// * 印つきは抽象クラス.
// ShapeSheet はメンバに D2D_UI をもち, D2D_UI はスワップチェーンパネルへの参照を維持する.

// SVG のためのテキスト改行コード
#define SVG_NEW_LINE	"\n"

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::IAsyncOperation;
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::Foundation::Point;
	using winrt::Windows::Storage::Streams::IRandomAccessStream;
	using winrt::Windows::Graphics::Imaging::SoftwareBitmap;
#if defined(_DEBUG)
	extern uint32_t debug_leak_cnt;
	constexpr wchar_t DEBUG_MSG[] = L"Memory leak occurs";
#endif
	constexpr double PT_ROUND = 1.0 / 16.0;

	// 前方宣言
	struct Shape;
	struct ShapeBezi;
	struct ShapeElli;
	struct ShapeGroup;
	struct ShapeImage;
	struct ShapeLine;
	struct ShapePath;
	struct ShapePoly;
	struct ShapeRect;
	struct ShapeRRect;
	struct ShapeRuler;
	struct ShapeSelect;
	struct ShapeSheet;
	struct ShapeStroke;
	struct ShapeText;

	constexpr D2D1_COLOR_F ACCENT_COLOR{ 0.0f, 0x78 / 255.0f, 0xD4 / 255.0f, 1.0f };	// 文字範囲の背景色 SystemAccentColor

	// 補助線
	constexpr FLOAT AUXILIARY_SEG_DASHES[]{ 4.0f, 4.0f };	// 補助線の破線の配置
	constexpr UINT32 AUXILIARY_SEG_DASHES_CONT = sizeof(AUXILIARY_SEG_DASHES) / sizeof(AUXILIARY_SEG_DASHES[0]);	// 補助線の破線の配置の要素数
	constexpr float AUXILIARY_SEG_OPAC = 0.975f;	// 補助線の不透明度
	constexpr D2D1_STROKE_STYLE_PROPERTIES1 AUXILIARY_SEG_STYLE	// 補助線の線の特性
	{
		D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// startCap
		D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// endCap
		D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND,	// dashCap
		D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL,	// lineJoin
		1.0f,	// miterLimit
		D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM,	// dashStyle
		0.0f,	// dashOffset
		D2D1_STROKE_TRANSFORM_TYPE::D2D1_STROKE_TRANSFORM_TYPE_NORMAL
	};

	// 図形の部位 (アンカーポイント)
	// 数の定まっていない多角形の頂点をあらわすため, enum struct でなく enum を用いる.
	enum ANC_TYPE {
		ANC_SHEET,		// 図形の外部 (矢印カーソル)
		ANC_FILL,		// 図形の内部 (移動カーソル)
		ANC_STROKE,	// 線枠 (移動カーソル)
		ANC_TEXT,		// 文字列 (移動カーソル)
		ANC_NW,		// 方形の左上の頂点 (北西南東カーソル)
		ANC_SE,		// 方形の右下の頂点 (北西南東カーソル)
		ANC_NE,		// 方形の右上の頂点 (北東南西カーソル)
		ANC_SW,		// 方形の左下の頂点 (北東南西カーソル)
		ANC_NORTH,		// 方形の上辺の中点 (上下カーソル)
		ANC_SOUTH,		// 方形の下辺の中点 (上下カーソル)
		ANC_EAST,		// 方形の左辺の中点 (左右カーソル)
		ANC_WEST,		// 方形の右辺の中点 (左右カーソル)
		ANC_R_NW,		// 左上の角丸の中心点 (十字カーソル)
		ANC_R_NE,		// 右上の角丸の中心点 (十字カーソル)
		ANC_R_SE,		// 右下の角丸の中心点 (十字カーソル)
		ANC_R_SW,		// 左下の角丸の中心点 (十字カーソル)
		ANC_P0,	// パスの開始点 (十字カーソル)
	};

	// 矢じるしの寸法
	struct ARROW_SIZE {
		float m_width;		// 返しの幅
		float m_length;		// 先端から付け根までの長さ
		float m_offset;		// 先端のずらし量
	};
	constexpr ARROW_SIZE ARROW_SIZE_DEFVAL{ 7.0, 16.0, 0.0 };	// 矢じるしの寸法の既定値

	// 矢じるしの形式
	enum struct ARROW_STYLE {
		NONE,	// なし
		OPENED,	// 開いた矢じるし
		FILLED	// 閉じた矢じるし
	};

	// 線分の端点
	// (SVG や PDF は, 始点終点の区別ができない)
	struct CAP_STYLE {
		D2D1_CAP_STYLE m_start;	// 始点
		D2D1_CAP_STYLE m_end;	// 終点
	};

	constexpr D2D1_COLOR_F COLOR_BLACK{ 0.0f, 0.0f, 0.0f, 1.0f };	// 黒
	constexpr D2D1_COLOR_F COLOR_WHITE{ 1.0f, 1.0f, 1.0f, 1.0f };	// 白
	constexpr D2D1_COLOR_F COLOR_TEXT_SELECTED = { 1.0f, 1.0f, 1.0f, 1.0f };	// 文字範囲の文字色


	// 破線の配置
	union DASH_PATT {
		float m_[6];
	};
	constexpr DASH_PATT DASH_PATT_DEFVAL{ { 4.0F, 3.0F, 1.0F, 3.0F, 1.0F, 3.0F } };	// 破線の配置の既定値

	// 方眼の強調
	struct GRID_EMPH {
		uint16_t m_gauge_1;	// 強調する間隔 (その1)
		uint16_t m_gauge_2;	// 強調する間隔 (その2)
	};
	constexpr GRID_EMPH GRID_EMPH_0{ 0, 0 };	// 強調なし
	constexpr GRID_EMPH GRID_EMPH_2{ 2, 0 };	// その2を強調
	constexpr GRID_EMPH GRID_EMPH_3{ 2, 10 };	// その2と 10 番目を強調

	// 方眼の表示
	enum struct GRID_SHOW {
		HIDE,	// 表示なし
		BACK,	// 最背面に表示
		FRONT	// 最前面に表示
	};

	// 多角形の作成方法
	struct POLY_OPTION {
		uint32_t m_vertex_cnt;	// 作図する多角形の頂点の数.
		bool m_regular;	// 正多角形で作図する.
		bool m_vertex_up;	// 頂点を上に作図する.
		bool m_end_closed;	// 辺を閉じて作図する.
		bool m_clockwise;	// 頂点を時計回りに作図する.
	};
	constexpr POLY_OPTION POLY_OPTION_DEFVAL{ 3, true, true, true, true };	// 多角形の作成方法の既定値

	constexpr float COLOR_MAX = 255.0f;	// 色成分の最大値
	constexpr double PT_PER_INCH = 72.0;	// 1 インチあたりのポイント数
	constexpr double MM_PER_INCH = 25.4;	// 1 インチあたりのミリメートル数
	constexpr float FONT_SIZE_DEFVAL = static_cast<float>(12.0 * 96.0 / 72.0);	// 書体の大きさの既定値 (システムリソースに値が無かった場合)
	constexpr D2D1_COLOR_F GRID_COLOR_DEFVAL{ ACCENT_COLOR.r, ACCENT_COLOR.g, ACCENT_COLOR.b, 0.5f };	// 方眼の色の既定値
	constexpr float GRID_LEN_DEFVAL = 48.0f;	// 方眼の長さの既定値
	constexpr float MITER_LIMIT_DEFVAL = 10.0f;	// マイター制限比率の既定値
	constexpr D2D1_SIZE_F TEXT_MARGIN_DEFVAL{ FONT_SIZE_DEFVAL / 4.0, FONT_SIZE_DEFVAL / 4.0 };	// 文字列の余白の既定値
	constexpr size_t MAX_N_GON = 256;	// 多角形の頂点の最大数 (ヒット判定でスタックを利用するため, オーバーフローしないよう制限する)

	MIDL_INTERFACE("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")
		IMemoryBufferByteAccess : IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE GetBuffer(
			BYTE * *value,
			UINT32 * capacity
			);
	};

	// 図形の部位（円形）を表示する.
	inline void anc_draw_ellipse(const D2D1_POINT_2F a_pos, const ShapeSheet& sh);
	// 図形の部位 (方形) を表示する.
	inline void anc_draw_rect(const D2D1_POINT_2F a_pos, const ShapeSheet& sh);
	// 単精度浮動小数が同じか判定する.
	inline bool equal(const float a, const float b) noexcept;
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const double a, const double b) noexcept;
	// 同値か判定する.
	template<typename T> inline bool equal(const T a, const T b) noexcept { return a == b; };
	// 矢じるしの寸法が同じか判定する.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept;
	// 線の単点が同じか判定する.
	inline bool equal(const CAP_STYLE& a, const CAP_STYLE& b) noexcept;
	// 色が同じか判定する.
	inline bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept;
	// 位置が同じか判定する.
	inline bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept;
	// 方形が同じか判定する.
	inline bool equal(const D2D1_RECT_F& a, const D2D1_RECT_F& b) noexcept;
	// 寸法が同じか判定する.
	inline bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept;
	// 文字範囲が同じか判定する.
	inline bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept;
	// 方眼の強調が同じか判定する.
	inline bool equal(const GRID_EMPH& a, const GRID_EMPH& b) noexcept;
	// 破線の配置が同じか判定する.
	inline bool equal(const DASH_PATT& a, const DASH_PATT& b) noexcept;
	// ワイド文字列が同じか判定する.
	inline bool equal(const wchar_t* a, const wchar_t* b) noexcept;
	// winrt 文字列が同じか判定する.
	inline bool equal(winrt::hstring const& a, const wchar_t* b) noexcept;
	// 色の成分が同じか判定する.
	inline bool equal_color_comp(const FLOAT a, const FLOAT b) noexcept;
	// 矢じるしの返しの位置を求める.
	inline void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F barbs[]) noexcept;
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
	// 位置が図形の部位に含まれるか判定する.
	inline bool pt_in_anc(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos) noexcept;
	// 位置がだ円に含まれるか判定する.
	inline bool pt_in_ellipse(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept;
	// 位置が円に含まれるか判定する.
	inline bool pt_in_circle(const D2D1_POINT_2F t_vec, const double rad) noexcept;
	// 位置が円に含まれるか判定する.
	inline bool pt_in_circle(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad) noexcept;
	// 多角形が位置を含むか判定する.
	inline bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t p_cnt, const D2D1_POINT_2F p_pos[]) noexcept;
	// 方形が位置を含むか判定する.
	inline bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept;
	// 方形が位置を含むか判定する.
	inline bool pt_in_rect2(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept;
	// 位置をスカラー倍に丸める.
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& round) noexcept;
	// 位置にスカラー値を掛け, 別の位置を足す.
	inline void pt_mul_add(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// 点にスカラー値を掛け, 別の位置を足す.
	inline void pt_mul_add(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// 位置にスカラー値を掛ける.
	inline void pt_mul(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// 寸法に値を掛ける.
	inline void pt_mul(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept;
	// 位置から位置を引く.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// 位置から大きさを引く.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& c) noexcept;
	// 文字列を複製する. 元の文字列がヌルポインター, または元の文字数が 0 のときは, ヌルポインターを返す.
	inline wchar_t* wchar_cpy(const wchar_t* const s) noexcept;
	// 文字列の長さ. 引数がヌルポインタの場合, 0 を返す.
	inline uint32_t wchar_len(const wchar_t* const t) noexcept;

	//------------------------------
	// shape_dt.cpp
	// 読み込み, 書き込み.
	//------------------------------

	// データリーダーから矢じるしの寸法を読み込む.
	void dt_read(ARROW_SIZE& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから端の形式を読み込む.
	void dt_read(CAP_STYLE& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから色を読み込む.
	void dt_read(D2D1_COLOR_F& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから位置を読み込む.
	void dt_read(D2D1_POINT_2F& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから方形を読み込む.
	void dt_read(D2D1_RECT_F& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから寸法を読み込む.
	void dt_read(D2D1_SIZE_F& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから寸法を読み込む.
	void dt_read(D2D1_SIZE_U& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから破線の配置を読み込む.
	void dt_read(DASH_PATT& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから文字範囲を読み込む.
	void dt_read(DWRITE_TEXT_RANGE& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから方眼の形式を読み込む.
	void dt_read(GRID_EMPH& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから位置配列を読み込む.
	void dt_read(std::vector<D2D1_POINT_2F>& val, /*<---*/DataReader const& dt_reader);
	// データリーダーから文字列を読み込む.
	void dt_read(wchar_t*& val, /*<---*/DataReader const& dt_reader);
	// データライターに矢じるしの寸法を書き込む.
	void dt_write(const ARROW_SIZE& val, /*--->*/DataWriter const& dt_writer);
	// データライターに端の形式を書き込む.
	void dt_write(const CAP_STYLE& val, /*--->*/DataWriter const& dt_writer);
	// データライターに色を書き込む.
	void dt_write(const D2D1_COLOR_F& val, /*--->*/DataWriter const& dt_writer);
	// データライターに位置を書き込む.
	void dt_write(const D2D1_POINT_2F val, /*--->*/DataWriter const& dt_writer);
	// データライターに方形を書き込む.
	void dt_write(const D2D1_RECT_F val, /*--->*/DataWriter const& dt_writer);
	// データライターに寸法を書き込む.
	void dt_write(const D2D1_SIZE_F val, /*--->*/DataWriter const& dt_writer);
	// データライターに寸法を書き込む.
	void dt_write(const D2D1_SIZE_U val, /*--->*/DataWriter const& dt_writer);
	// データライターに破線の配置を書き込む.
	void dt_write(const DASH_PATT& val, /*--->*/DataWriter const& dt_writer);
	// データライターに文字列範囲を書き込む.
	void dt_write(const DWRITE_TEXT_RANGE val, /*--->*/DataWriter const& dt_writer);
	// データライターに方眼の形式を書き込む.
	void dt_write(const GRID_EMPH val, /*--->*/DataWriter const& dt_writer);
	// データライターに位置配列を書き込む.
	void dt_write(const std::vector<D2D1_POINT_2F>& val, /*--->*/DataWriter const& dt_writer);
	// データライターに文字列を書き込む.
	void dt_write(const wchar_t* val, /*--->*/DataWriter const& dt_writer);
	// データライターに文字列を書き込む.
	size_t dt_write(const char val[], DataWriter const& dt_writer);

	//-------------------------------
	// shape_svg.cpp
	//-------------------------------

	// データライターに SVG として属性名とシングルバイト文字列を書き込む.
	void svg_dt_write(const char* val, const char* name, /*--->*/DataWriter const& dt_writer);
	// データライターに SVG としてシングルバイト文字列を書き込む.
	void svg_dt_write(const char* val, /*--->*/DataWriter const& dt_writer);
	// データライターに SVG として属性名と色を書き込む.
	void svg_dt_write(const D2D1_COLOR_F val, const char* name, /*--->*/DataWriter const& dt_writer);
	// データライターに SVG として色を書き込む.
	void svg_dt_write(const D2D1_COLOR_F val, /*--->*/DataWriter const& dt_writer);
	// データライターに SVG として破線の形式と配置を書き込む.
	void svg_dt_write(const D2D1_DASH_STYLE style, const DASH_PATT& patt, const double width, /*--->*/DataWriter const& dt_writer);
	// データライターに SVG として命令と位置を書き込む.
	void svg_dt_write(const D2D1_POINT_2F val, const char* cmd, /*--->*/DataWriter const& dt_writer);
	// データライターに SVG として属性名と位置を書き込む.
	void svg_dt_write(const D2D1_POINT_2F val, const char* name_x, const char* name_y, /*--->*/DataWriter const& dt_writer);
	// データライターに SVG として属性名と浮動小数値を書き込む
	void svg_dt_write(const double val, const char* name, /*--->*/DataWriter const& dt_writer);
	// データライターに SVG として浮動小数を書き込む.
	void svg_dt_write(const float val, /*--->*/DataWriter const& dt_writer);
	// データライターに SVG として属性名と 32 ビット正整数を書き込む.
	void svg_dt_write(const uint32_t val, const char* name, /*--->*/DataWriter const& dt_writer);
	// データライターに SVG としてマルチバイト文字列を書き込む.
	void svg_dt_write(const wchar_t val[], const uint32_t v_len, /*--->*/DataWriter const& dt_writer);

	//------------------------------
	// shape_slist.cpp
	// 図形リスト
	//------------------------------

	using SHAPE_LIST = std::list<struct Shape*>;

	// 図形リストの中の文字列図形に, 利用できない書体があったならばその書体名を得る.
	bool slist_test_avaiable_font(const SHAPE_LIST& slist, wchar_t*& unavailable_font) noexcept;
	// 最後の図形を得る.
	Shape* slist_back(SHAPE_LIST const& slist) noexcept;
	// 図形リストを消去し, 含まれる図形を破棄する.
	void slist_clear(SHAPE_LIST& slist) noexcept;
	// 図形を種類別に数える.
	void slist_count(const SHAPE_LIST& slist, uint32_t& undeleted_cnt, uint32_t& selected_cnt, uint32_t& selected_group_cnt, uint32_t& runlength_cnt, uint32_t& selected_text_cnt, uint32_t& text_cnt, uint32_t& selected_image_cnt, bool& fore_selected, bool& back_selected, bool& prev_selected) noexcept;
	// 先頭から図形まで数える.
	uint32_t slist_count(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// 最初の図形をリストから得る.
	Shape* slist_front(SHAPE_LIST const& slist) noexcept;
	// 図形と用紙を囲む領域を得る.
	void slist_bound_sheet(SHAPE_LIST const& slist, const D2D1_SIZE_F sh_size, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// すべての図形を囲む領域をリストから得る.
	void slist_bound_all(SHAPE_LIST const& slist, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// 選択された図形を囲む領域をリストから得る.
	bool slist_bound_selected(SHAPE_LIST const& slist, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// 位置を含む図形とその部位を得る.
	uint32_t slist_hit_test(SHAPE_LIST const& slist, const D2D1_POINT_2F t_pos, Shape*& s) noexcept;
	// 図形を挿入する.
	void slist_insert(SHAPE_LIST& slist, Shape* const s_ins, const Shape* s_at) noexcept;
	// 選択された図形を差分だけ移動する.
	bool slist_move(SHAPE_LIST const& slist, const D2D1_POINT_2F b_vec) noexcept;
	// 図形のその次の図形を得る.
	Shape* slist_next(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// 図形のその前の図形を得る.
	Shape* slist_prev(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// データリーダーから図形リストを読み込む.
	bool slist_read(SHAPE_LIST& slist, DataReader const& dt_reader);
	// 図形をリストから削除し, 削除した図形の次の図形を得る.
	Shape* slist_remove(SHAPE_LIST& slist, const Shape* s) noexcept;
	// 選択された図形のリストを得る.
	template <typename T> void slist_get_selected(SHAPE_LIST const& slist, SHAPE_LIST& t_list) noexcept;
	// データライターに図形リストを書き込む. REDUCE なら消去された図形は省く.
	template <bool REDUCE> void slist_write(const SHAPE_LIST& slist, DataWriter const& dt_writer);
	// リストの中の図形の順番を得る.
	template <typename S, typename T> bool slist_match(SHAPE_LIST const& slist, S s, T& t);
	// 選択されてない図形の頂点の中から 指定した位置に最も近い頂点を見つける.
	bool slist_find_vertex_closest(const SHAPE_LIST& slist, const D2D1_POINT_2F& c_pos, const float dist, D2D1_POINT_2F& val) noexcept;

	//------------------------------
	// 図形のひな型
	//------------------------------
	struct Shape {
		static float s_anc_len;	// 図形の部位の大きさ
		static D2D1_COLOR_F s_background_color;	// 前景色
		static D2D1_COLOR_F s_foreground_color;	// 背景色
		static winrt::com_ptr<ID2D1StrokeStyle1> m_aux_style;	// 補助線の形式

		// 図形を破棄する.
		virtual ~Shape(void) noexcept {}	// 派生クラスがあるので必要
		// 図形を表示する.
		virtual void draw(ShapeSheet const& sh) = 0;
		// 矢じるしの寸法を得る
		virtual bool get_arrow_size(ARROW_SIZE& /*val*/) const noexcept { return false; }
		// 矢じるしの形式を得る.
		virtual bool get_arrow_style(ARROW_STYLE& /*val*/) const noexcept { return false; }
		// 図形を囲む領域を得る.
		virtual void get_bound(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/, D2D1_POINT_2F& /*b_min*/, D2D1_POINT_2F& /*b_max*/) const noexcept {}
		// 端の形式を得る.
		virtual bool get_stroke_cap(CAP_STYLE& /*val*/) const noexcept { return false; }
		// 角丸半径を得る.
		virtual bool get_corner_radius(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// 破線の端の形式を得る.
		virtual bool get_dash_cap(D2D1_CAP_STYLE& /*val*/) const noexcept { return false; }
		// 破線の配置を得る.
		virtual bool get_dash_patt(DASH_PATT& /*val*/) const noexcept { return false; }
		// 破線の形式を得る.
		virtual bool get_dash_style(D2D1_DASH_STYLE& /*val*/) const noexcept { return false; }
		// 塗りつぶし色を得る.
		virtual bool get_fill_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 書体の色を得る.
		virtual bool get_font_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 書体名を得る.
		virtual bool get_font_family(wchar_t*& /*val*/) const noexcept { return false; }
		// 書体の大きさを得る.
		virtual bool get_font_size(float& /*val*/) const noexcept { return false; }
		// 書体の幅の伸縮を得る.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH& /*val*/) const noexcept { return false; }
		// 書体の字体を得る.
		virtual bool get_font_style(DWRITE_FONT_STYLE& /*val*/) const noexcept { return false; }
		// 書体の太さを得る.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT& /*val*/) const noexcept { return false; }
		// 方眼の基準の大きさを得る.
		virtual bool get_grid_base(float& /*val*/) const noexcept { return false; }
		// 方眼の色を得る.
		virtual bool get_grid_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 方眼を強調を得る.
		virtual bool get_grid_emph(GRID_EMPH& /*val*/) const noexcept { return false; }
		// 方眼の表示を得る.
		virtual bool get_grid_show(GRID_SHOW& /*val*/) const noexcept { return false; }
		// 方眼に合わせるを得る.
		virtual bool get_grid_snap(bool& /*val*/) const noexcept { return false; }
		// 画像の不透明度を得る.
		virtual bool get_image_opacity(float& /*val*/) const noexcept { return false; }
		// 線分の結合のマイター制限を得る.
		virtual bool get_join_miter_limit(float& /*val*/) const noexcept { return false; }
		// 線分の結合を得る.
		virtual bool get_join_style(D2D1_LINE_JOIN& /*val*/) const noexcept { return false; }
		// 近傍の頂点を見つける.
		virtual bool get_pos_nearest(const D2D1_POINT_2F /*pos*/, float& /*dd*/, D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// 部位の位置を得る.
		virtual	void get_pos_anc(const uint32_t /*anc*/, D2D1_POINT_2F&/*val*/) const noexcept {}
		// 図形を囲む領域の左上位置を得る.
		virtual void get_pos_min(D2D1_POINT_2F& /*val*/) const noexcept {}
		// 開始位置を得る.
		virtual bool get_pos_start(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// 用紙の色を得る.
		virtual bool get_sheet_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 用紙の拡大率を得る.
		virtual bool get_sheet_scale(float& /*val*/) const noexcept { return false; }
		// 用紙の大きさを得る.
		virtual bool get_sheet_size(D2D1_SIZE_F& /*val*/) const noexcept { return false; }
		// 線枠の色を得る.
		virtual bool get_stroke_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 書体の太さを得る
		virtual bool get_stroke_width(float& /*val*/) const noexcept { return false; }
		// 段落のそろえを得る.
		virtual bool get_text_par_align(DWRITE_PARAGRAPH_ALIGNMENT& /*val*/) const noexcept { return false; }
		// 文字列のそろえを得る.
		virtual bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& /*val*/) const noexcept { return false; }
		// 文字列を得る.
		virtual bool get_text_content(wchar_t*& /*val*/) const noexcept { return false; }
		// 行間を得る.
		virtual bool get_text_line_sp(float& /*val*/) const noexcept { return false; }
		// 文字列の周囲の余白を得る.
		virtual bool get_text_padding(D2D1_SIZE_F& /*val*/) const noexcept { return false; }
		// 文字範囲を得る
		virtual bool get_text_selected(DWRITE_TEXT_RANGE& /*val*/) const noexcept { return false; }
		// 文字範囲を得る
		virtual bool get_font_collection(IDWriteFontCollection** /*val*/) const noexcept { return false; }
		// 頂点を得る.
		virtual size_t get_verts(D2D1_POINT_2F /*v_pos*/[]) const noexcept { return 0; };
		// 位置を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F /*t_pos*/) const noexcept { return ANC_TYPE::ANC_SHEET; }
		// 範囲に含まれるか判定する.
		virtual bool in_area(const D2D1_POINT_2F /*area_min*/, const D2D1_POINT_2F /*area_max*/) const noexcept { return false; }
		// 消去されたか判定する.
		virtual bool is_deleted(void) const noexcept { return false; }
		// 選択されてるか判定する.
		virtual bool is_selected(void) const noexcept { return false; }
		// 差分だけ移動する.
		virtual	bool move(const D2D1_POINT_2F /*val*/) noexcept { return false; }
		// 値を矢じるしの寸法に格納する.
		virtual bool set_arrow_size(const ARROW_SIZE& /*val*/) noexcept { return false; }
		// 値を矢じるしの形式に格納する.
		virtual bool set_arrow_style(const ARROW_STYLE /*val*/) noexcept { return false; }
		// 値を端の形式に格納する.
		virtual bool set_stroke_cap(const CAP_STYLE& /*val*/) noexcept { return false; }
		// 値を角丸半径に格納する.
		virtual bool set_corner_radius(const D2D1_POINT_2F& /*alue*/) noexcept { return false; }
		// 値を破線の端の形式に格納する.
		virtual bool set_dash_cap(const D2D1_CAP_STYLE& /*val*/) noexcept { return false; }
		// 値を破線の配置に格納する.
		virtual bool set_dash_patt(const DASH_PATT& /*val*/) noexcept { return false; }
		// 値を線枠の形式に格納する.
		virtual bool set_dash_style(const D2D1_DASH_STYLE /*val*/) noexcept { return false; }
		// 値を消去されたか判定に格納する.
		virtual bool set_delete(const bool /*val*/) noexcept { return false; }
		// 値を塗りつぶし色に格納する.
		virtual bool set_fill_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// 値を書体の色に格納する.
		virtual bool set_font_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// 値を書体名に格納する.
		virtual bool set_font_family(wchar_t* const /*val*/) noexcept { return false; }
		// 値を書体の大きさに格納する.
		virtual bool set_font_size(const float /*val*/) noexcept { return false; }
		// 値を書体の幅の伸縮に格納する.
		virtual bool set_font_stretch(const DWRITE_FONT_STRETCH /*val*/) noexcept { return false; }
		// 値を書体の字体に格納する.
		virtual bool set_font_style(const DWRITE_FONT_STYLE /*val*/) noexcept { return false; }
		// 値を書体の太さに格納する.
		virtual bool set_font_weight(const DWRITE_FONT_WEIGHT /*val*/) noexcept { return false; }
		// 値を方眼の大きさに格納する.
		virtual bool set_grid_base(const float /*val*/) noexcept { return false; }
		// 値を方眼の色に格納する.
		virtual bool set_grid_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// 値を方眼の強調に格納する.
		virtual bool set_grid_emph(const GRID_EMPH& /*val*/) noexcept { return false; }
		// 値を方眼の表示に格納する.
		virtual bool set_grid_show(const GRID_SHOW /*val*/) noexcept { return false; }
		// 値を方眼に合わせるに格納する.
		virtual bool set_grid_snap(const bool /*val*/) noexcept { return false; }
		// 画像の不透明度を得る.
		virtual bool set_image_opacity(const float /*val*/) noexcept { return false; }
		// 値を線分の結合のマイター制限に格納する.
		virtual bool set_join_miter_limit(const float& /*val*/) noexcept { return false; }
		// 値を線分の結合に格納する.
		virtual bool set_join_style(const D2D1_LINE_JOIN& /*val*/) noexcept { return false; }
		// 値を, 部位の位置に格納する.
		virtual bool set_pos_anc(const D2D1_POINT_2F /*val*/, const uint32_t /*anc*/, const float /*limit*/, const bool /*keep_aspect*/) noexcept { return false; }
		// 値を始点に格納する. 他の部位の位置も動く.
		virtual bool set_pos_start(const D2D1_POINT_2F /*val*/) noexcept { return false; }
		// 値を用紙の色に格納する.
		virtual bool set_sheet_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// 値を用紙の拡大率に格納する.
		virtual bool set_sheet_scale(const float /*val*/) noexcept { return false; }
		// 値を用紙の大きさに格納する.
		virtual bool set_sheet_size(const D2D1_SIZE_F /*val*/) noexcept { return false; }
		// 値を選択されてるか判定に格納する.
		virtual bool set_select(const bool /*val*/) noexcept { return false; }
		// 値を線枠の色に格納する.
		virtual bool set_stroke_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// 値を書体の太さに格納する.
		virtual bool set_stroke_width(const float /*val*/) noexcept { return false; }
		// 値を段落のそろえに格納する.
		virtual bool set_text_par_align(const DWRITE_PARAGRAPH_ALIGNMENT /*val*/) noexcept { return false; }
		// 値を文字列のそろえに格納する.
		virtual bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT /*val*/) noexcept { return false; }
		// 値を文字列に格納する.
		virtual bool set_text_content(wchar_t* const /*val*/) noexcept { return false; }
		// 値を行間に格納する.
		virtual bool set_text_line_sp(const float /*val*/) noexcept { return false; }
		// 値を文字列の余白に格納する.
		virtual bool set_text_padding(const D2D1_SIZE_F /*val*/) noexcept { return false; }
		// 値を文字範囲に格納する.
		virtual bool set_text_selected(const DWRITE_TEXT_RANGE /*val*/) noexcept { return false; }
		// 図形をデータライターに書き込む.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
		// 図形をデータライターに PDF として書き込む.
		virtual size_t pdf_write(const ShapeSheet& /*sheet*/, DataWriter const& /*dt_writer*/) const { return 0; }
		// 図形をデータライターに SVG として書き込む.
		virtual void svg_write(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// 選択フラグ
	//------------------------------
	struct ShapeSelect : Shape {
		bool m_is_deleted = false;	// 消去されたか判定
		bool m_is_selected = false;	// 選択されたか判定

		// 図形を表示する.
		virtual void draw(ShapeSheet const& sh) = 0;
		// 消去されたか判定する.
		bool is_deleted(void) const noexcept final override { return m_is_deleted; }
		// 選択されてるか判定する.
		bool is_selected(void) const noexcept final override { return m_is_selected; }
		// 値を消去されたか判定に格納する.
		bool set_delete(const bool val) noexcept final override { if (m_is_deleted != val) { m_is_deleted = val;  return true; } return false; }
		// 値を選択されてるか判定に格納する.
		bool set_select(const bool val) noexcept final override { if (m_is_selected != val) { m_is_selected = val; return true; } return false; }
		// 図形を作成する.
		ShapeSelect(void) {};	// 派生クラスがあるので必要
		// 図形をデータリーダーから読み込む.
		ShapeSelect(const DataReader& dt_reader) :
			m_is_deleted(dt_reader.ReadBoolean()),
			m_is_selected(dt_reader.ReadBoolean())
			{}
	};

	//------------------------------
	// 画像
	//------------------------------
	struct ShapeImage : ShapeSelect {
		D2D1_POINT_2F m_pos;	// 始点の位置
		D2D1_SIZE_F m_view;	// 表示寸法
		D2D1_RECT_F m_clip;	// 表示されている矩形
		D2D1_SIZE_U m_orig;	// ビットマップの原寸
		uint8_t* m_data = nullptr;	// ビットマップのデータ
		D2D1_SIZE_F m_ratio{ 1.0, 1.0 };	// 表示寸法と原寸の縦横比
		float m_opac = 1.0f;	// ビットマップの不透明度 (アルファ値と乗算)
		winrt::com_ptr<ID2D1Bitmap1> m_d2d_bitmap{ nullptr };	// D2D ビットマップ

		int m_pdf_obj = 0;

		// 図形を破棄する.
		ShapeImage::~ShapeImage(void)
		{
			if (m_data != nullptr) {
				delete m_data;
				m_data = nullptr;
			}
			if (m_d2d_bitmap != nullptr) {
				//m_d2d_bitmap->Release();
				m_d2d_bitmap = nullptr;
			}
		} // ~Shape

		//------------------------------
		// shape_image.cpp
		//------------------------------

		// ストリームに格納する.
		IAsyncOperation<bool> copy_to(const winrt::guid enc_id, IRandomAccessStream& ra_stream);
		// 図形を表示する.
		void draw(ShapeSheet const& sh) final override;
		// 図形を囲む領域を得る.
		void get_bound(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/, D2D1_POINT_2F& /*b_min*/, D2D1_POINT_2F& /*b_max*/) const noexcept final override;
		// 画像の不透明度を得る.
		bool get_image_opacity(float& /*val*/) const noexcept final override;
		// 近傍の頂点を見つける.
		bool get_pos_nearest(const D2D1_POINT_2F /*pos*/, float& /*dd*/, D2D1_POINT_2F& /*val*/) const noexcept final override;
		// 部位の位置を得る.
		void get_pos_anc(const uint32_t /*anc*/, D2D1_POINT_2F&/*val*/) const noexcept final override;
		// 図形を囲む領域の左上位置を得る.
		void get_pos_min(D2D1_POINT_2F& /*val*/) const noexcept final override;
		// 開始位置を得る.
		bool get_pos_start(D2D1_POINT_2F& /*val*/) const noexcept final override;
		// 頂点を得る.
		size_t get_verts(D2D1_POINT_2F /*v_pos*/[]) const noexcept final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F /*t_pos*/) const noexcept final override;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F /*area_min*/, const D2D1_POINT_2F /*area_max*/) const noexcept final override;
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F val) noexcept final override;
		// 原画像に戻す.
		void revert(void) noexcept;
		// 値を画像の不透明度に格納する.
		bool set_image_opacity(const float val) noexcept final override;
		// 値を, 部位の位置に格納する.
		bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept final override;
		// 値を始点に格納する. 他の部位の位置も動く.
		bool set_pos_start(const D2D1_POINT_2F /*val*/) noexcept final override;
		// 図形を作成する.
		ShapeImage(const D2D1_POINT_2F pos, const D2D1_SIZE_F view_size, const SoftwareBitmap& bitmap, const float opacity);
		// 図形をデータリーダーから読み込む.
		ShapeImage(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに PDF として書き込む.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const;
		// 図形をデータライターに SVG ファイルとして書き込む.
		void svg_write(const wchar_t f_name[], DataWriter const& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		void svg_write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 用紙
	//------------------------------
	struct ShapeSheet : Shape {
		SHAPE_LIST m_shape_list;	// 図形リスト

		// 矢じるし
		ARROW_SIZE m_arrow_size{ ARROW_SIZE_DEFVAL };	// 矢じるしの寸法
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// 矢じるしの形式

		// 角丸
		D2D1_POINT_2F m_corner_rad{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL };	// 角丸半径

		// 塗りつぶし
		D2D1_COLOR_F m_fill_color{ COLOR_WHITE };	// 塗りつぶしの色

		// 書体
		D2D1_COLOR_F m_font_color{ COLOR_BLACK };	// 書体の色
		wchar_t* m_font_family = nullptr;	// 書体名 (システムリソースに値が無かった場合)
		float m_font_size = FONT_SIZE_DEFVAL;	// 書体の大きさ (システムリソースに値が無かった場合)
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// 書体の幅の伸縮 (システムリソースに値が無かった場合)
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// 書体の字体 (システムリソースに値が無かった場合)
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ (システムリソースに値が無かった場合)

		// 線枠
		D2D1_CAP_STYLE m_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 破線の端の形式
		DASH_PATT m_dash_patt{ DASH_PATT_DEFVAL };	// 破線の配置
		D2D1_DASH_STYLE m_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// 破線の形式
		float m_join_miter_limit = MITER_LIMIT_DEFVAL;	// 線の結合のマイター制限
		D2D1_LINE_JOIN m_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;	// 線枠の結合
		CAP_STYLE m_stroke_cap{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };	// 端の形式
		D2D1_COLOR_F m_stroke_color{ COLOR_BLACK };	// 線枠の色
		float m_stroke_width = 1.0f;	// 線枠の太さ

		// 文字列
		float m_text_line_sp = 0.0f;	// 行間 (DIPs 96dpi固定)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_par_align = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// 段落の揃え
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// 文字列の揃え
		D2D1_SIZE_F m_text_padding{ TEXT_MARGIN_DEFVAL };	// 文字列の左右と上下の余白

		// 画像
		float m_image_opac = 1.0f;	// 画像の不透明度
		bool m_image_opac_importing = false;	// 画像をインポートするときに不透明度を適用する.

		// 方眼
		float m_grid_base = GRID_LEN_DEFVAL - 1.0f;	// 方眼の基準の大きさ (を -1 した値)
		D2D1_COLOR_F m_grid_color{ GRID_COLOR_DEFVAL };	// 方眼の色
		GRID_EMPH m_grid_emph{ GRID_EMPH_0 };	// 方眼の強調
		D2D1_POINT_2F m_grid_offset{ 0.0f, 0.0f };	// 方眼のオフセット
		GRID_SHOW m_grid_show = GRID_SHOW::BACK;	// 方眼の表示
		bool m_grid_snap = true;	// 方眼に合わせる

		// 用紙
		D2D1_COLOR_F m_sheet_color{ COLOR_WHITE };	// 背景色
		float m_sheet_scale = 1.0f;	// 拡大率
		D2D1_SIZE_F	m_sheet_size{ 0.0f, 0.0f };	// 大きさ (MainPage のコンストラクタで設定)

		D2D_UI m_d2d;	// 描画環境
		winrt::com_ptr<ID2D1DrawingStateBlock1> m_state_block{ nullptr };	// 描画状態の保存ブロック
		winrt::com_ptr<ID2D1SolidColorBrush> m_range_brush{ nullptr };	// 選択された文字範囲の色ブラシ
		winrt::com_ptr<ID2D1SolidColorBrush> m_color_brush{ nullptr };	// 図形の色ブラシ

		//------------------------------
		// shape_sheet.cpp
		//------------------------------

		static constexpr float size_max(void) noexcept { return 32767.0F; }
		// 図形を表示する.
		void draw(ShapeSheet const& sh) final override;
		// 曲線の補助線を表示する.
		void draw_auxiliary_bezi(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// だ円の補助線を表示する.
		void draw_auxiliary_elli(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// 直線の補助線を表示する.
		void draw_auxiliary_line(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// 方形の補助線を表示する.
		void draw_auxiliary_rect(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// 多角形の補助線を表示する.
		void draw_auxiliary_poly(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos, const POLY_OPTION& p_opt);
		// 角丸方形の補助線を表示する.
		void draw_auxiliary_rrect(ShapeSheet const& sh, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// 矢じるしの寸法を得る.
		bool get_arrow_size(ARROW_SIZE& val) const noexcept final override;
		// 矢じるしの形式を得る.
		bool get_arrow_style(ARROW_STYLE& val) const noexcept final override;
		// 端の形式を得る.
		bool get_stroke_cap(CAP_STYLE& val) const noexcept final override;
		// 角丸半径を得る.
		bool get_corner_radius(D2D1_POINT_2F& val) const noexcept final override;
		// 破線の端の形式を得る.
		bool get_dash_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// 破線の配置を得る.
		bool get_dash_patt(DASH_PATT& val) const noexcept final override;
		// 破線の形式を得る.
		bool get_dash_style(D2D1_DASH_STYLE& val) const noexcept final override;
		// 塗りつぶし色を得る.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// 書体の色を得る.
		bool get_font_color(D2D1_COLOR_F& val) const noexcept final override;
		// 書体名を得る.
		bool get_font_family(wchar_t*& val) const noexcept final override;
		// 書体の大きさを得る.
		bool get_font_size(float& val) const noexcept final override;
		// 書体の幅の伸縮を得る.
		bool get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept final override;
		// 書体の字体を得る.
		bool get_font_style(DWRITE_FONT_STYLE& val) const noexcept final override;
		// 書体の太さを得る.
		bool get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept final override;
		// 方眼の基準の大きさを得る.
		bool get_grid_base(float& val) const noexcept final override;
		// 方眼の色を得る.
		bool get_grid_color(D2D1_COLOR_F& val) const noexcept final override;
		// 方眼の強調を得る.
		bool get_grid_emph(GRID_EMPH& val) const noexcept final override;
		// 方眼の表示の状態を得る.
		bool get_grid_show(GRID_SHOW& val) const noexcept final override;
		// 方眼に合わせるを得る.
		bool get_grid_snap(bool& val) const noexcept final override;
		// 画像の不透明度を得る.
		bool get_image_opacity(float& val) const noexcept final override;
		// 線分の結合のマイター制限を得る.
		bool get_join_miter_limit(float& val) const noexcept final override;
		// 線分の結合を得る.
		bool get_join_style(D2D1_LINE_JOIN& val) const noexcept final override;
		// 用紙の色を得る.
		bool get_sheet_color(D2D1_COLOR_F& val) const noexcept final override;
		// 用紙の色を得る.
		bool get_sheet_size(D2D1_SIZE_F& val) const noexcept final override;
		// 用紙の拡大率を得る.
		bool get_sheet_scale(float& val) const noexcept final override;
		// 線枠の色を得る.
		bool get_stroke_color(D2D1_COLOR_F& val) const noexcept final override;
		// 書体の太さを得る
		bool get_stroke_width(float& val) const noexcept final override;
		// 段落のそろえを得る.
		bool get_text_par_align(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept final override;
		// 文字列のそろえを得る.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept final override;
		// 行間を得る.
		bool get_text_line_sp(float& val) const noexcept final override;
		// 文字列の周囲の余白を得る.
		bool get_text_padding(D2D1_SIZE_F& val) const noexcept final override;
		// 図形をデータリーダーから読み込む.
		void read(DataReader const& dt_reader);
		// 値を矢じるしの寸法に格納する.
		bool set_arrow_size(const ARROW_SIZE& val) noexcept final override;
		// 値を矢じるしの形式に格納する.
		bool set_arrow_style(const ARROW_STYLE val) noexcept final override;
		// 図形の属性値を格納する.
		void set_attr_to(const Shape* s) noexcept;
		// 値を画像の不透明度に格納する.
		bool set_image_opacity(const float val) noexcept final override;
		// 値を端の形式に格納する.
		bool set_stroke_cap(const CAP_STYLE& val) noexcept final override;
		// 値を角丸半径に格納する.
		bool set_corner_radius(const D2D1_POINT_2F& val) noexcept final override;
		// 値を破線の端の形式に格納する.
		bool set_dash_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// 値を破線の配置に格納する.
		bool set_dash_patt(const DASH_PATT& val) noexcept final override;
		// 値を線枠の形式に格納する.
		bool set_dash_style(const D2D1_DASH_STYLE val) noexcept final override;
		// 値を塗りつぶし色に格納する.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を書体の色に格納する.
		bool set_font_color(const D2D1_COLOR_F& val) noexcept final override;
		// 書体名に格納する.
		bool set_font_family(wchar_t* const val) noexcept final override;
		// 書体の大きさに格納する.
		bool set_font_size(const float val) noexcept final override;
		// 値を書体の幅の伸縮に格納する.
		bool set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept final override;
		// 値を書体の字体に格納する.
		bool set_font_style(const DWRITE_FONT_STYLE val) noexcept final override;
		// 値を書体の太さに格納する.
		bool set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept final override;
		// 値を方眼の基準の大きさに格納する.
		bool set_grid_base(const float val) noexcept final override;
		// 値を方眼の色に格納する.
		bool set_grid_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を方眼の強調に格納する.
		bool set_grid_emph(const GRID_EMPH& val) noexcept final override;
		// 値を方眼の表示に格納する.
		bool set_grid_show(const GRID_SHOW val) noexcept final override;
		// 値を方眼に合わせるに格納する.
		bool set_grid_snap(const bool val) noexcept final override;
		// 値を線分の結合のマイター制限に格納する.
		bool set_join_miter_limit(const float& val) noexcept final override;
		// 値を線分の結合に格納する.
		bool set_join_style(const D2D1_LINE_JOIN& val) noexcept final override;
		// 値を用紙の色に格納する.
		bool set_sheet_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を用紙の拡大率に格納する.
		bool set_sheet_scale(const float val) noexcept final override;
		// 値を用紙の寸法に格納する.
		bool set_sheet_size(const D2D1_SIZE_F val) noexcept final override;
		// 値を線枠の色に格納する.
		bool set_stroke_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を書体の太さに格納する.
		bool set_stroke_width(const float val) noexcept final override;
		// 値を段落のそろえに格納する.
		bool set_text_par_align(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept final override;
		// 値を文字列のそろえに格納する.
		bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT val) noexcept final override;
		// 値を行間に格納する.
		bool set_text_line_sp(const float val) noexcept final override;
		// 値を文字列の余白に格納する.
		bool set_text_padding(const D2D1_SIZE_F val) noexcept final override;
		// 図形をデータリーダーに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// グループ図形
	//------------------------------
	struct ShapeGroup : Shape {
		SHAPE_LIST m_list_grouped{};	// グループ化された図形のリスト

		// 図形を作成する
		ShapeGroup(void) {}
		// 図形を破棄する
		ShapeGroup::~ShapeGroup(void)
		{
			slist_clear(m_list_grouped);
		} // ~Shape

		//------------------------------
		// shape_group.cpp
		//------------------------------

		// 図形を表示する.
		void draw(ShapeSheet const& sh) final override;
		// 図形を囲む領域を得る.
		void get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept final override;
		// 図形を囲む領域の左上位置を得る.
		void get_pos_min(D2D1_POINT_2F& val) const noexcept final override;
		// 開始位置を得る.
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// 文字列図形を含むか判定する.
		bool has_text(void) noexcept;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept final override;
		// 消去されているか判定する.
		bool is_deleted(void) const noexcept final override { return m_list_grouped.size() == 0 || m_list_grouped.back()->is_deleted(); }
		// 選択されているか判定する.
		bool is_selected(void) const noexcept final override { return m_list_grouped.size() > 0 && m_list_grouped.back()->is_selected(); }
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F val) noexcept final override;
		// 値を消去されたか判定に格納する.
		bool set_delete(const bool val) noexcept final override;
		// 値を選択されたか判定に格納する.
		bool set_select(const bool val) noexcept final override;
		// 値を始点に格納する. 他の部位の位置も動く.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept final override;
		// 図形をデータリーダーから読み込む.
		ShapeGroup(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		void svg_write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 線枠のひな型
	//------------------------------
	struct ShapeStroke : ShapeSelect {
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// 開始位置
		std::vector<D2D1_POINT_2F> m_vec{};	// 次の位置への差分
		CAP_STYLE m_stroke_cap{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };	// 端の形式
		D2D1_COLOR_F m_stroke_color{ COLOR_BLACK };	// 線枠の色
		D2D1_CAP_STYLE m_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 破線の端の形式
		DASH_PATT m_dash_patt{ DASH_PATT_DEFVAL };	// 破線の配置
		D2D1_DASH_STYLE m_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// 破線の形式
		float m_join_miter_limit = MITER_LIMIT_DEFVAL;		// 線の結合のマイター制限の比率
		D2D1_LINE_JOIN m_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;	// 線の結合
		float m_stroke_width = 1.0f;	// 線枠の太さ

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_stroke_style{};	// D2D ストロークスタイル

		// 図形を破棄する.
		virtual ~ShapeStroke(void)
		{
			if (m_d2d_stroke_style != nullptr) {
				//m_d2d_stroke_style->Release();
				m_d2d_stroke_style = nullptr;
			}
		} // ~Shape

		//------------------------------
		// shape_stroke.cpp
		//------------------------------

		// D2D ストロークスタイルを作成する.
		void create_stroke_style(D2D_UI const& d2d);
		// 図形を表示する.
		virtual void draw(ShapeSheet const& sh) override = 0;
		// 図形を囲む領域を得る.
		void get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept final override;
		// 端の形式を得る.
		bool get_stroke_cap(CAP_STYLE& val) const noexcept final override;
		// 破線の端の形式を得る.
		bool get_dash_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// 破線の配置を得る.
		bool get_dash_patt(DASH_PATT& val) const noexcept final override;
		// 破線の形式を得る.
		bool get_dash_style(D2D1_DASH_STYLE& val) const noexcept final override;
		// 線分の結合のマイター制限を得る.
		bool get_join_miter_limit(float& val) const noexcept final override;
		// 線分の結合を得る.
		bool get_join_style(D2D1_LINE_JOIN& val) const noexcept final override;
		// 近傍の頂点を見つける.
		virtual bool get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept override;
		// 図形を囲む領域の左上位置を得る.
		void get_pos_min(D2D1_POINT_2F& val) const noexcept final override;
		// 部位の位置を得る.
		virtual void get_pos_anc(const uint32_t /*anc*/, D2D1_POINT_2F& val) const noexcept override;
		// 開始位置を得る.
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// 線枠の色を得る.
		bool get_stroke_color(D2D1_COLOR_F& val) const noexcept final override;
		// 線枠の太さを得る.
		bool get_stroke_width(float& val) const noexcept final override;
		// 頂点を得る.
		virtual size_t get_verts(D2D1_POINT_2F v_pos[]) const noexcept override;
		// 位置を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept override;
		// 範囲に含まれるか判定する.
		virtual bool in_area(const D2D1_POINT_2F /*area_min*/, const D2D1_POINT_2F /*area_max*/) const noexcept override;
		// 差分だけ移動する.
		virtual	bool move(const D2D1_POINT_2F val) noexcept override;
		// 値を端の形式に格納する.
		virtual	bool set_stroke_cap(const CAP_STYLE& val) noexcept override;
		// 値を破線の端の形式に格納する.
		bool set_dash_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// 値を破線の配置に格納する.
		bool set_dash_patt(const DASH_PATT& val) noexcept final override;
		// 値を線枠の形式に格納する.
		bool set_dash_style(const D2D1_DASH_STYLE val) noexcept final override;
		// 値を線分の結合のマイター制限に格納する.
		virtual bool set_join_miter_limit(const float& val) noexcept override;
		// 値を線分の結合に格納する.
		virtual bool set_join_style(const D2D1_LINE_JOIN& val) noexcept override;
		// 値を, 部位の位置に格納する.
		virtual bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept override;
		// 値を始点に格納する. 他の部位の位置も動く.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept override;
		// 値を線枠の色に格納する.
		bool set_stroke_color(const D2D1_COLOR_F& val) noexcept;
		// 値を線枠の太さに格納する.
		bool set_stroke_width(const float val) noexcept;
		// 図形を作成する.
		ShapeStroke(const ShapeSheet* s_sheet);
		// 図形をデータリーダーから読み込む.
		ShapeStroke(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに PDF として書き込む.
		size_t pdf_write_stroke(DataWriter const& /*dt_writer*/) const;
		// 図形をデータライターに SVG として書き込む.
		void svg_write_stroke(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 矢印つき直線
	//------------------------------
	struct ShapeLine : ShapeStroke {
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// 矢じるしの形式
		ARROW_SIZE m_arrow_size{ ARROW_SIZE_DEFVAL };	// 矢じるしの寸法

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_arrow_style{ nullptr };	// 矢じるしの D2D ストロークスタイル
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_arrow_geom{ nullptr };	// 矢じるしの D2D パスジオメトリ

		// 図形を破棄する.
		virtual ~ShapeLine(void)
		{
			if (m_d2d_arrow_geom != nullptr) {
				//m_d2d_arrow_geom->Release();
				m_d2d_arrow_geom = nullptr;
			}
			if (m_d2d_arrow_style != nullptr) {
				//m_d2d_arrow_style->Release();
				m_d2d_arrow_style = nullptr;
			}
		} // ~ShapeStroke

		//------------------------------
		// shape_line.cpp
		//------------------------------

		// 矢じるしの先端と返しの位置を求める.
		static bool line_get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept;

		// 図形を作成する.
		ShapeLine(const ShapeSheet* s_sheet, const bool a_none = false) :
			ShapeStroke(s_sheet),
			m_arrow_style(a_none ? ARROW_STYLE::NONE : s_sheet->m_arrow_style),
			m_arrow_size(s_sheet->m_arrow_size)
		{}
		// 図形を作成する.
		ShapeLine(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet);
		// データリーダーから図形を読み込む.
		ShapeLine(DataReader const& dt_reader);
		// 図形を表示する.
		virtual void draw(ShapeSheet const& sh) override;
		// 矢じるしの寸法を得る.
		bool get_arrow_size(ARROW_SIZE& size) const noexcept final override;
		// 矢じるしの形式を得る.
		bool get_arrow_style(ARROW_STYLE& val) const noexcept final override;
		// 位置を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept override;
		// 範囲に含まれるか判定する.
		virtual bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept override;
		// 値を矢じるしの寸法に格納する.
		virtual bool set_arrow_size(const ARROW_SIZE& val) noexcept override;
		// 値を矢じるしの形式に格納する.
		virtual bool set_arrow_style(const ARROW_STYLE val) noexcept override;
		// 値を, 部位の位置に格納する. 
		virtual bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept override;
		// 値を始点に格納する.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept override;
		// 差分だけ移動する.
		virtual bool move(const D2D1_POINT_2F val) noexcept override;
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに PDF として書き込む.
		virtual size_t pdf_write(const ShapeSheet& sheet, DataWriter const& /*dt_writer*/) const;
		// 図形をデータライターに SVG として書き込む.
		void svg_write(DataWriter const& dt_writer) const;
		// 矢じりをデータライターに PDF として書き込む.
		size_t pdf_write_barbs(const ShapeSheet& sheet, const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const;
		// 矢じりをデータライターに SVG として書き込む.
		void svg_write_barbs(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const;
		// 値を端の形式に格納する.
		bool set_stroke_cap(const CAP_STYLE& val) noexcept final override;
		// 値を線分の結合のマイター制限に格納する.
		bool set_join_miter_limit(const float& val) noexcept final override;
		// 値を線分の結合に格納する.
		bool set_join_style(const D2D1_LINE_JOIN& val) noexcept final override;
	};

	//------------------------------
	// 方形
	//------------------------------
	struct ShapeRect : ShapeStroke {
		D2D1_COLOR_F m_fill_color{ COLOR_WHITE };		// 塗りつぶし色

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// 図形を作成する.
		ShapeRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet);
		// データリーダーから図形を読み込む.
		ShapeRect(DataReader const& dt_reader);
		// 図形を表示する.
		virtual void draw(ShapeSheet const& sh) override;
		// 近傍の頂点を見つける.
		bool get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept final override;
		// 頂点を得る.
		size_t get_verts(D2D1_POINT_2F v_pos[]) const noexcept final override;
		// 位置を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept override;
		// 図形の部位が位置を含むか判定する.
		uint32_t hit_test_anc(const D2D1_POINT_2F t_pos) const noexcept;
		// 範囲に含まれるか判定する.
		virtual bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept override;
		// 塗りつぶしの色を得る.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// 値を塗りつぶしの色に格納する.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// 部位の位置を得る.
		virtual void get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept override;
		// 値を, 部位の位置に格納する.
		virtual bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept override;
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		virtual void svg_write(DataWriter const& dt_writer) const;
		// 図形をデータライターに PDF として書き込む.
		virtual size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 定規
	// 作成したあとで文字列の属性の変更はできない.
	//------------------------------
	struct ShapeRuler : ShapeRect {
		float m_grid_base = GRID_LEN_DEFVAL - 1.0f;	// 方眼の大きさ (を -1 した値)
		wchar_t* m_font_family = nullptr;	// 書体名
		float m_font_size = FONT_SIZE_DEFVAL;	// 書体の大きさ

		winrt::com_ptr<IDWriteTextFormat> m_dw_text_format{};	// テキストフォーマット

		// 図形を破棄する.
		ShapeRuler::~ShapeRuler(void)
		{
			if (m_dw_text_format != nullptr) {
				//m_dw_text_format->Release();
				m_dw_text_format = nullptr;
			}
		} // ~ShapeStroke

		bool get_font_collection(IDWriteFontCollection** val) const noexcept final override
		{
			winrt::check_hresult(m_dw_text_format->GetFontCollection(val));
			return true;
		}

		//------------------------------
		// shape_ruler.cpp
		//------------------------------

		// 図形を表示する.
		void draw(ShapeSheet const& sh) final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// 書体名を得る.
		bool get_font_family(wchar_t*& val) const noexcept final override;
		// 書体の大きさを得る.
		bool get_font_size(float& val) const noexcept final override;
		// 値を書体名に格納する.
		bool set_font_family(wchar_t* const val) noexcept final override;
		// 値を書体の大きさに格納する.
		bool set_font_size(const float val) noexcept final override;
		// 図形を作成する.
		ShapeRuler(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet);
		// 図形をデータリーダーから読み込む.
		ShapeRuler(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		void svg_write(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// だ円
	//------------------------------
	struct ShapeElli : ShapeRect {
		// 図形を作成する.
		ShapeElli(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet) :
			ShapeRect::ShapeRect(b_pos, b_vec, s_sheet)
		{}
		// 図形をデータリーダーから読み込む.
		ShapeElli(DataReader const& dt_reader) :
			ShapeRect::ShapeRect(dt_reader)
		{}

		//------------------------------
		// shape_elli.cpp
		//------------------------------

		// 図形を表示する.
		void draw(ShapeSheet const& sh) final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// 図形をデータライターに PDF として書き込む.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		void svg_write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 角丸方形
	//------------------------------
	struct ShapeRRect : ShapeRect {
		D2D1_POINT_2F m_corner_rad{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL };		// 角丸部分の半径

		//------------------------------
		// shape_rrect.cpp
		// 角丸方形
		//------------------------------

		// 図形を表示する.
		void draw(ShapeSheet const& sh) final override;
		// 角丸半径を得る.
		bool get_corner_radius(D2D1_POINT_2F& val) const noexcept final override;
		// 部位の位置を得る.
		void get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		//	値を, 部位の位置に格納する.
		bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept final override;
		// 図形を作成する.
		ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet);
		// 図形をデータリーダーから読み込む.
		ShapeRRect(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに PDF として書き込む.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const final override;
		// 図形をデータライターに SVG として書き込む.
		void svg_write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 折れ線のひな型
	//------------------------------
	struct ShapePath : ShapeLine {
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_path_geom{ nullptr };	// 折れ線の D2D パスジオメトリ

		// 図形を作成する.
		ShapePath(const ShapeSheet* s_sheet, const bool s_closed) :
			ShapeLine::ShapeLine(s_sheet, s_closed)
		{}
		// 図形をデータリーダーから読み込む.
		ShapePath(DataReader const& dt_reader) :
			ShapeLine::ShapeLine(dt_reader)
		{}

		// 図形を破棄する.
		virtual ~ShapePath(void)
		{
			if (m_d2d_path_geom != nullptr) {
				//m_d2d_path_geom->Release();
				m_d2d_path_geom = nullptr;
			}
			// ~ShapeLine
		}

		//------------------------------
		// shape_path.cpp
		// 折れ線のひな型
		//------------------------------

		// パスジオメトリを作成する.
		virtual void create_path_geometry(const D2D_UI& /*d2d*/) = 0;
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F val) noexcept final override;
		// 値を, 部位の位置に格納する.
		bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept final override;
		// 値を矢じるしの寸法に格納する.
		bool set_arrow_size(const ARROW_SIZE& val) noexcept final override;
		// 値を矢じるしの形式に格納する.
		virtual bool set_arrow_style(const ARROW_STYLE val) noexcept override;
		// 値を始点に格納する. 他の部位の位置も動く.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept final override;
		// 図形をデータライターに書き込む.
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

		static bool poly_get_arrow_barbs(const size_t v_cnt, const D2D1_POINT_2F v_pos[], const ARROW_SIZE& a_size, D2D1_POINT_2F& h_tip, D2D1_POINT_2F h_barbs[]) noexcept;
		// パスジオメトリを作成する.
		void create_path_geometry(const D2D_UI& d2d) final override;
		// 方形をもとに多角形を作成する.
		static void create_poly_by_bbox(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const POLY_OPTION& p_opt, D2D1_POINT_2F v_pos[]/*, D2D1_POINT_2F& v_vec*/) noexcept;
		// 図形を表示する
		void draw(ShapeSheet const& sh) final override;
		// 塗りつぶし色を得る.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// 範囲に含まれるか判定する.
		virtual bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept override;
		// 値を矢じるしの形式に格納する.
		bool set_arrow_style(const ARROW_STYLE val) noexcept final override;
		// 値を塗りつぶし色に格納する.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// 図形を作成する.
		ShapePoly(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet, const POLY_OPTION& p_opt);
		// 図形をデータリーダーから読み込む.
		ShapePoly(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& /*dt_writer*/) const;
		// 図形をデータライターに PDF として書き込む.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& /*dt_writer*/) const final override;
		// 図形をデータライターに SVG として書き込む.
		void svg_write(DataWriter const& /*dt_writer*/) const;
	};

	//------------------------------
	// 曲線
	//------------------------------
	struct ShapeBezi : ShapePath {

		//------------------------------
		// shape_bezi.cpp
		// ベジェ曲線
		//------------------------------

		static bool bezi_calc_arrow(const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_SIZE a_size, D2D1_POINT_2F barbs[3]) noexcept;
		// パスジオメトリを作成する.
		void create_path_geometry(const D2D_UI& d2d) final override;
		// 図形を表示する.
		void draw(ShapeSheet const& sh) final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept final override;
		// 図形を作成する.
		ShapeBezi(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapeSheet* s_sheet);
		// 図形をデータリーダーから読み込む.
		ShapeBezi(DataReader const& dt_reader);
		// 図形をデータライターに PDF として書き込む.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		void svg_write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 文字列
	//------------------------------
	struct ShapeText : ShapeRect {
		//static ID2D1Factory* s_d2d_factory;	//
		//static ID2D1DeviceContext2* s_d2d_context;	//
		static wchar_t** s_available_fonts;		// 有効な書体名
		static D2D1_COLOR_F s_text_selected_background;	// 文字範囲の背景色
		static D2D1_COLOR_F s_text_selected_foreground;	// 文字範囲の文字色

		D2D1_COLOR_F m_font_color{ COLOR_BLACK };	// 書体の色
		wchar_t* m_font_family = nullptr;	// 書体名
		float m_font_size = FONT_SIZE_DEFVAL;	// 書体の大きさ
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// 書体の幅の伸縮
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// 書体の字体
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ
		wchar_t* m_text = nullptr;	// 文字列
		float m_text_line_sp = 0.0f;	// 行間 (DIPs 96dpi固定)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_par_align = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// 段落のそろえ
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// 文字のそろえ
		D2D1_SIZE_F m_text_padding{ TEXT_MARGIN_DEFVAL };	// 文字列の上下と左右の余白
		DWRITE_TEXT_RANGE m_text_selected_range{ 0, 0 };	// 選択された文字範囲

		DWRITE_FONT_METRICS m_dw_font_metrics{};
		DWRITE_LINE_METRICS* m_dw_line_metrics = nullptr;	// 行の計量
		UINT32 m_dw_selected_cnt = 0;	// 選択された文字範囲の計量の要素数
		DWRITE_HIT_TEST_METRICS* m_dw_selected_metrics = nullptr;	// 選択された文字範囲の計量
		UINT32 m_dw_test_cnt = 0;	// 位置の計量の要素数
		DWRITE_HIT_TEST_METRICS* m_dw_test_metrics = nullptr;	// 位置の計量
		winrt::com_ptr<IDWriteTextLayout> m_dw_text_layout{ nullptr };	// 文字列レイアウト

		int m_pdf_font_num = 0;	// PDF のフォント番号 (PDF 出力時のみ利用)

		// 図形を破棄する.
		~ShapeText(void)
		{
			relese_metrics();

			// 書体名を破棄する.
			if (m_font_family != nullptr) {
				// 有効な書体名に含まれてない書体名なら破棄する.
				if (!is_available_font(m_font_family)) {
					delete[] m_font_family;
				}
				m_font_family = nullptr;
			}
			// 文字列を破棄する.
			if (m_text != nullptr) {
				delete[] m_text;
				m_text = nullptr;
			}

			// 文字列レイアウトを破棄する.
			if (m_dw_text_layout != nullptr) {
				//m_dw_text_layout->Release();
				m_dw_text_layout = nullptr;
			}
		} // ~ShapeStroke

		bool get_font_collection(IDWriteFontCollection** val) const noexcept final override
		{
			winrt::check_hresult(m_dw_text_layout->GetFontCollection(val));
			return true;
		}

		//------------------------------
		// shape_text.cpp
		// 文字列図形
		//------------------------------

		// 枠を文字列に合わせる.
		bool frame_fit(const float g_len) noexcept;
		// 図形を表示する.
		void draw(ShapeSheet const& sh) final override;
		// 書体の色を得る.
		bool get_font_color(D2D1_COLOR_F& val) const noexcept final override;
		// 書体名を得る.
		bool get_font_family(wchar_t*& val) const noexcept final override;
		// 書体の大きさを得る.
		bool get_font_size(float& val) const noexcept final override;
		// 書体の幅の伸縮を得る.
		bool get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept final override;
		// 書体の字体を得る.
		bool get_font_style(DWRITE_FONT_STYLE& val) const noexcept final override;
		// 書体の太さを得る.
		bool get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept final override;
		// 段落のそろえを得る.
		bool get_text_par_align(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept final override;
		// 文字列のそろえを得る.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept final override;
		// 文字列を得る.
		bool get_text_content(wchar_t*& val) const noexcept final override;
		// 行間を得る.
		bool get_text_line_sp(float& val) const noexcept final override;
		// 文字列の余白を得る.
		bool get_text_padding(D2D1_SIZE_F& val) const noexcept final override;
		// 選択された文字範囲を得る.
		bool get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept final override;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept final override;
		// 書体名が有効か判定し, 有効なら, 引数の書体名は破棄し, 有効な書体名の配列の要素と置き換える.
		static bool is_available_font(wchar_t*& font) noexcept;
		// 有効な書体名の配列を破棄する.
		static void release_available_fonts(void) noexcept;
		// 計量を破棄する.
		void relese_metrics(void) noexcept;
		// 有効な書体名の配列を設定する.
		static void set_available_fonts(const D2D_UI& d2d);
		// 値を書体の色に格納する.
		bool set_font_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を書体名に格納する.
		bool set_font_family(wchar_t* const val) noexcept final override;
		// 値を書体の大きさに格納する.
		bool set_font_size(const float val) noexcept final override;
		// 値を書体の幅の伸縮に格納する.
		bool set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept final override;
		// 値を書体の字体に格納する.
		bool set_font_style(const DWRITE_FONT_STYLE val) noexcept final override;
		// 値を書体の太さに格納する.
		bool set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept final override;
		// 値を段落のそろえに格納する.
		bool set_text_par_align(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept final override;
		// 値を文字列のそろえに格納する.
		bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT val) noexcept final override;
		// 値を文字列に格納する.
		bool set_text_content(wchar_t* const val) noexcept final override;
		// 値を行間に格納する.
		bool set_text_line_sp(const float val) noexcept final override;
		// 値を文字列の余白に格納する.
		bool set_text_padding(const D2D1_SIZE_F val) noexcept final override;
		// 値を文字範囲に格納する.
		bool set_text_selected(const DWRITE_TEXT_RANGE val) noexcept final override;
		// 図形を作成する.
		ShapeText(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, wchar_t* const text, const ShapeSheet* s_sheet);
		// 図形をデータリーダーから読み込む.
		ShapeText(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに PDF として書き込む.
		size_t pdf_write(const ShapeSheet& sheet, DataWriter const& dt_writer) const;
		// データライターに SVG として書き込む.
		void svg_write(DataWriter const& dt_writer) const;
	};

	// 図形の部位（円形）を表示する.
	// a_pos	部位の位置
	// sh	表示する用紙
	inline void anc_draw_ellipse(const D2D1_POINT_2F a_pos, const ShapeSheet& sh)
	{
		const FLOAT r = static_cast<FLOAT>(Shape::s_anc_len * 0.5 + 1.0);	// 半径
		sh.m_color_brush->SetColor(Shape::s_background_color);
		sh.m_d2d.m_d2d_context->FillEllipse(D2D1_ELLIPSE{ a_pos, r, r }, sh.m_color_brush.get());
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		sh.m_d2d.m_d2d_context->FillEllipse(D2D1_ELLIPSE{ a_pos, r - 1.0f, r - 1.0f }, sh.m_color_brush.get());
	}

	// 図形の部位 (方形) を表示する.
	// a_pos	部位の位置
	// sh	表示する用紙
	inline void anc_draw_rect(const D2D1_POINT_2F a_pos, const ShapeSheet& sh)
	{
		D2D1_POINT_2F r_min;
		D2D1_POINT_2F r_max;
		pt_add(a_pos, -0.5 * Shape::s_anc_len, r_min);
		pt_add(r_min, Shape::s_anc_len, r_max);
		const D2D1_RECT_F rect{ r_min.x, r_min.y, r_max.x, r_max.y };
		sh.m_color_brush->SetColor(Shape::s_background_color);
		sh.m_d2d.m_d2d_context->DrawRectangle(rect, sh.m_color_brush.get(), 2.0, nullptr);
		sh.m_color_brush->SetColor(Shape::s_foreground_color);
		sh.m_d2d.m_d2d_context->FillRectangle(rect, sh.m_color_brush.get());
	}

	// 矢じるしの寸法が同じか判定する.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept
	{
		return equal(a.m_width, b.m_width) && equal(a.m_length, b.m_length) && equal(a.m_offset, b.m_offset);
	}

	// 端の形式が同じか判定する.
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

	// 方形が同じか判定する.
	inline bool equal(const D2D1_RECT_F& a, const D2D1_RECT_F& b) noexcept
	{
		return equal(a.left, b.left) && equal(a.top, b.top) && equal(a.right, b.right) && equal(a.bottom, b.bottom);
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
	inline bool equal(const DASH_PATT& a, const DASH_PATT& b) noexcept
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

	// 矢じるしの返しの位置を求める.
	// a_vec	矢軸ベクトル.
	// a_len	矢軸ベクトルの長さ
	// h_width	矢じるしの幅 (返しの間の長さ)
	// h_len	矢じるしの長さ (先端から返しまでの軸ベクトル上での長さ)
	// barbs[2]	計算された矢じるしの返しの位置 (先端からのオフセット)
	inline void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F barbs[]) noexcept
	{
		if (a_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z{ 0.0f, 0.0f };
			barbs[0] = Z;
			barbs[1] = Z;
		}
		else {
			const double hf = h_width * 0.5;	// 矢じるしの幅の半分の大きさ
			const double sx = a_vec.x * -h_len;	// 矢軸ベクトルを矢じるしの長さ分反転
			const double sy = a_vec.x * hf;
			const double tx = a_vec.y * -h_len;
			const double ty = a_vec.y * hf;
			const double ax = 1.0 / a_len;
			barbs[0].x = static_cast<FLOAT>((sx - ty) * ax);
			barbs[0].y = static_cast<FLOAT>((tx + sy) * ax);
			barbs[1].x = static_cast<FLOAT>((sx + ty) * ax);
			barbs[1].y = static_cast<FLOAT>((tx - sy) * ax);
		}
	}

	// 色が不透明か判定する.
	// val	色
	// 戻り値	不透明ならば true, 透明ならば false.
	inline bool is_opaque(const D2D1_COLOR_F& val) noexcept
	{
		const uint32_t a = static_cast<uint32_t>(round(val.a * 255.0f));
		return (a & 0xff) > 0;
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

	// 図形の部位が位置を含むか判定する.
	inline bool pt_in_anc(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos) noexcept
	{
		const double a = Shape::s_anc_len * 0.5;
		const double dx = static_cast<double>(t_pos.x) - static_cast<double>(a_pos.x);
		const double dy = static_cast<double>(t_pos.y) - static_cast<double>(a_pos.y);
		return -a <= dx && dx <= a && -a <= dy && dy <= a;
	}

	// 位置が円に含まれるか判定する.
	inline bool pt_in_circle(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double r) noexcept
	{
		const double dx = static_cast<double>(t_pos.x) - static_cast<double>(c_pos.x);
		const double dy = static_cast<double>(t_pos.y) - static_cast<double>(c_pos.y);
		return dx * dx + dy * dy <= r * r;
	}

	// 位置が円に含まれるか判定する.
	inline bool pt_in_circle(const D2D1_POINT_2F t_vec, const double r) noexcept
	{
		const double dx = t_vec.x;
		const double dy = t_vec.y;
		return dx * dx + dy * dy <= r * r;
	}

	// だ円にが位置を含むか判定する.
	// t_pos	判定する位置
	// c_pos	だ円の中心
	// rad	だ円の径
	// 戻り値	含む場合 true
	inline bool pt_in_ellipse(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept
	{
		const double dx = static_cast<double>(t_pos.x) - c_pos.x;
		const double dy = static_cast<double>(t_pos.y) - c_pos.y;
		const double xx = rad_x * rad_x;
		const double yy = rad_y * rad_y;
		return dx * dx * yy + dy * dy * xx <= xx * yy;
	}

	// 多角形が位置を含むか判定する.
	// t_pos	判定する位置
	// v_cnt	頂点の数
	// v_pos	頂点の配列 [v_cnt]
	// 戻り値	含む場合 true
	// 多角形の各辺と, 指定された点を開始点とする水平線が交差する数を求める.
	inline bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[]) noexcept
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

	// 方形が位置を含むか判定する.
	// t_pos	判定する位置
	// r_min	方形の左上位置
	// r_max	方形の右下位置
	// 戻り値	含む場合 true
	inline bool pt_in_rect2(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept
	{
		return r_min.x <= t_pos.x && t_pos.x <= r_max.x && r_min.y <= t_pos.y && t_pos.y <= r_max.y;
	}

	// 方形が位置を含むか判定する.
	// t_pos	判定する位置
	// r_min	方形のいずれかの頂点
	// r_max	r_min に対して対角にある頂点
	// 戻り値	含む場合 true
	inline bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept
	{
		const double min_x = r_min.x < r_max.x ? r_min.x : r_max.x;
		const double max_x = r_min.x < r_max.x ? r_max.x : r_min.x;
		const double min_y = r_min.y < r_max.y ? r_min.y : r_max.y;
		const double max_y = r_min.y < r_max.y ? r_max.y : r_min.y;
		return min_x <= t_pos.x && t_pos.x <= max_x && min_y <= t_pos.y && t_pos.y <= max_y;
	}

	// 位置にスカラーを掛けて, 位置を加える.
	// a	位置
	// b	スカラー値
	// c	加える位置
	// d	結果
	inline void pt_mul_add(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
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
	inline void pt_mul_add(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.X * b + c.x);
		d.y = static_cast<FLOAT>(a.Y * b + c.y);
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

	// 文字列の長さ.
	// 引数がヌルポインタの場合, 0 を返す.
	inline uint32_t wchar_len(const wchar_t* const t) noexcept
	{
		return (t == nullptr || t[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(t));
	}

	// 文字列を複製する.
	// 元の文字列がヌルポインター, または元の文字数が 0 のときは,
	// ヌルポインターを返す.
	inline wchar_t* wchar_cpy(const wchar_t* const s) noexcept
	{
		const size_t s_len = (s == nullptr || s[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(s));
		if (s_len > 0) {
			wchar_t* t;
			wcscpy_s(t = new wchar_t[s_len + 1], s_len + 1, s);
			return t;
		}
		return nullptr;
	}

}