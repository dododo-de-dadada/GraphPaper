#pragma once
//------------------------------
// shape.h
// shape.cpp	図形のひな型, その他
// shape_bezier.cpp	ベジェ曲線
// shape_ellipse.cpp	だ円
// shape_group.cpp	グループ
// shape_image.cpp	画像
// shape_line.cpp	直線 (矢じるしつき)
// shape_path.cpp	折れ線のひな型
// shape_pdf.cpp	PDF への書き込み
// shape_poly.cpp	多角形
// shape.rect.cpp	方形
// shape_rrect.cpp	角丸方形
// shape_ruler.cpp	定規
// shape_page.cpp	ページ
// shape_slist.cpp	図形リスト
// shape_stroke.cpp	線枠のひな型
// shape_svg.cpp	SVG への書き込み
// shape_text.cpp	文字列
//------------------------------
#include <list>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <wincodec.h>
#include "d2d_ui.h"
//
// +-------------+
// | Shape*      |
// +------+------+
//        |
//        +---------------+---------------+
//        |               |               |
// +------+------+ +------+------+ +------+------+
// | ShapeSelect*| | ShapeGroup  | | ShapePage   |
// +------+------+ +-------------+ +-------------+
//        |
//        +---------------+
//        |               |
// +------+------+ +------+------+
// | ShapeStroke*| | ShapeImage  |
// +------+------+ +-------------+
//        |
//        +-----------------------------------------------+
//        |                                               |
// +------+------+                                 +------+------+
// | ShapeLine   |                                 | ShapeRect*  |
// +------+------+                                 +------+------+
//        |                                               |
// +------+------+                                        |
// | ShapePath*  |                                        |
// +------+------+                                        |
//        |                                               |
//        +---------------+---------------+               +---------------+---------------+---------------+
//        |               |               |               |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+
// | ShapePolygon| | ShapeBezier | |ShapeQEllipse| | ShapeEllipse| | ShapeRRect  | | ShapeText   | | ShapeRuler  |
// +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+
//
// * 印つきは抽象クラス.

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
	constexpr double PT_ROUND = 1.0 / 16.0;	// 位置を丸めるときの倍数

	// 前方宣言
	struct Shape;
	struct ShapeBezier;
	struct ShapeEllipse;
	struct ShapeGroup;
	struct ShapeImage;
	struct ShapeLine;
	struct ShapePage;
	struct ShapePath;
	struct ShapePolygon;
	struct ShapeRect;
	struct ShapeRRect;
	struct ShapeRuler;
	struct ShapeSelect;
	struct ShapeStroke;
	struct ShapeText;

	constexpr D2D1_COLOR_F ACCENT_COLOR{ 0.0f, 0x78 / 255.0f, 0xD4 / 255.0f, 1.0f };	// 文字範囲の背景色 SystemAccentColor
	constexpr D2D1_COLOR_F COLOR_BLACK{ 0.0f, 0.0f, 0.0f, 1.0f };	// 黒
	constexpr D2D1_COLOR_F COLOR_WHITE{ 1.0f, 1.0f, 1.0f, 1.0f };	// 白
	constexpr D2D1_COLOR_F COLOR_TEXT_RANGE = { 1.0f, 1.0f, 1.0f, 1.0f };	// 文字範囲の文字色

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

	// アンカーポイント (図形の部位)
	// 数の定まっていない多角形の頂点をあらわすため, enum struct でなく enum を用いる.
	// 
	//  NW    N    NE
	//   @----@----@
	//   |         |
	// W @         @ E
	//   |         |
	//   @----@----@
	//  SW    S    SE
	//
	enum ANC_TYPE : uint32_t {
		ANC_PAGE,		// 図形の外部 (矢印カーソル)
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
		ANC_A_CENTER,	// 円弧の中心点
		ANC_A_START,
		ANC_A_END,
		ANC_P0,	// パスの始点 (十字カーソル)
	};

	// 矢じるしの大きさ
	//           |
	//  +--- @   |   @
	//  |     \  |  /   
	// length  \ | /
	//  |       \|/
	//  +---     @   ---+
	//           |      offset
	//           +   -- +
	//        |      |
	//        +------+
	//          width
	struct ARROW_SIZE {
		float m_width;		// 返しの幅
		float m_length;		// 先端から付け根までの長さ
		float m_offset;		// 先端のずらし量
	};
	constexpr ARROW_SIZE ARROW_SIZE_DEFVAL{ 7.0, 16.0, 0.0 };	// 矢じるしの寸法の既定値
	constexpr float ARROW_SIZE_MAX = 127.5f;

	// 矢じるしの形式
	enum struct ARROW_STYLE : uint32_t {
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
	constexpr CAP_STYLE CAP_FLAT{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };
	constexpr CAP_STYLE CAP_ROUND{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND };
	constexpr CAP_STYLE CAP_SQUARE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE };
	constexpr CAP_STYLE CAP_TRIANGLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE };

	// 破線の配置
	union DASH_PATT {
		float m_[6];
	};
	constexpr DASH_PATT DASH_PATT_DEFVAL{ { 4.0F, 3.0F, 1.0F, 3.0F, 1.0F, 3.0F } };	// 破線の配置の既定値

	// 方眼の強調
	struct GRID_EMPH {
		uint32_t m_gauge_1;	// 強調する間隔 (その1)
		uint32_t m_gauge_2;	// 強調する間隔 (その2)
	};
	constexpr GRID_EMPH GRID_EMPH_0{ 0, 0 };	// 強調なし (既定値)
	constexpr GRID_EMPH GRID_EMPH_2{ 2, 0 };	// 2 番目の線を強調
	constexpr GRID_EMPH GRID_EMPH_3{ 2, 10 };	// 2 番目と 10 番目の線を強調

	// 方眼の表示
	enum struct GRID_SHOW : uint32_t {
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
	constexpr D2D1_COLOR_F GRID_COLOR_DEFVAL{	// 方眼の色の既定値
		ACCENT_COLOR.r, ACCENT_COLOR.g, ACCENT_COLOR.b, 192.0f / 255.0f };
	constexpr float GRID_LEN_DEFVAL = 48.0f;	// 方眼の長さの既定値
	constexpr float MITER_LIMIT_DEFVAL = 10.0f;	// 尖り制限の既定値
	constexpr D2D1_SIZE_F TEXT_PADDING_DEFVAL{ FONT_SIZE_DEFVAL / 4.0, FONT_SIZE_DEFVAL / 4.0 };	// 文字列の余白の既定値
	constexpr size_t N_GON_MAX = 256;	// 多角形の頂点の最大数 (ヒット判定でスタックを利用するため, オーバーフローしないよう制限する)
	constexpr float PAGE_SIZE_MAX = 32768.0f;	// 最大のページ大きさ
	constexpr D2D1_SIZE_F PAGE_SIZE_DEFVAL{ 8.0f * 96.0f, 11.0f * 96.0f };	// ページの大きさの既定値 (ピクセル)
	constexpr float FONT_SIZE_MAX = 512.0f;	// 書体の大きさの最大値

	// COM インターフェイス IMemoryBufferByteAccess を初期化
	MIDL_INTERFACE("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")
		IMemoryBufferByteAccess : IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE GetBuffer(
			BYTE * *value,
			UINT32 * capacity
			);
	};

	// 図形の部位（円形）を表示する.
	inline void anc_draw_circle(
		const D2D1_POINT_2F p, ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush);
	// 図形の部位 (方形) を表示する.
	inline void anc_draw_square(
		const D2D1_POINT_2F p, ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush);
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
	// 矢じりの返しの位置を求める.
	inline void get_pos_barbs(
		const D2D1_POINT_2F a, const double a_len, const double width, const double len, 
		D2D1_POINT_2F barb[]) noexcept;
	// 色が不透明か判定する.
	inline bool is_opaque(const D2D1_COLOR_F& color) noexcept;
	// ベクトルの長さ (の自乗値) を得る
	inline double pt_abs2(const D2D1_POINT_2F a) noexcept;
	// 位置に位置を足す
	inline void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// 位置にスカラー値を足す
	inline void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// 位置にX軸とY軸の値を足す
	inline void pt_add(
		const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& c) noexcept;
	// 二点の中点を得る.
	inline void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// 位置が図形の部位に含まれるか判定する.
	inline bool pt_in_anc(
		const D2D1_POINT_2F test, const D2D1_POINT_2F a, const double a_len) noexcept;
	// 位置がだ円に含まれるか判定する.
	inline bool pt_in_ellipse(
		const D2D1_POINT_2F test, const D2D1_POINT_2F center, const double rad_x,
		const double rad_y, const double rot = 0.0) noexcept;
	// 位置が円に含まれるか判定する.
	inline bool pt_in_circle(const D2D1_POINT_2F test, const double radius) noexcept;
	// 位置が円に含まれるか判定する.
	inline bool pt_in_circle(
		const D2D1_POINT_2F test, const D2D1_POINT_2F center, const double radius) noexcept;
	// 多角形が位置を含むか判定する.
	inline bool pt_in_poly(
		const D2D1_POINT_2F test, const size_t p_cnt, const D2D1_POINT_2F p[]) noexcept;
	// 方形が位置を含むか判定する.
	inline bool pt_in_rect(
		const D2D1_POINT_2F test, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb) noexcept;
	// 方形が位置を含むか判定する.
	inline bool pt_in_rect2(
		const D2D1_POINT_2F test, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb) noexcept;
	// 位置をスカラー倍に丸める.
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& round) noexcept;
	// 位置にスカラー値を掛け, 別の位置を足す.
	inline void pt_mul_add(
		const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// 点にスカラー値を掛け, 別の位置を足す.
	inline void pt_mul_add(
		const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
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
	// shape_text.cpp
	//------------------------------

	// wchar_t 型の文字列 (UTF-16) を uint32_t 型の配列 (UTF-32) に変換する. 
	std::vector<uint32_t> conv_utf16_to_utf32(const wchar_t* w, const size_t w_len) noexcept;
	// 字面を得る.
	template <typename T> bool get_font_face(
		T* t, const wchar_t* family, const DWRITE_FONT_WEIGHT weight,
		const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face)
		noexcept;

	//------------------------------
	// shape_rect.cpp
	//------------------------------

	uint32_t rect_hit_test_anc(
		const D2D1_POINT_2F start, const D2D1_POINT_2F vec, const D2D1_POINT_2F test, 
		const double a_len) noexcept;

	//------------------------------
	// shape_slist.cpp
	//------------------------------

	using SHAPE_LIST = std::list<struct Shape*>;

	// 図形リストの中の文字列図形に, 利用できない書体があったならばその書体名を得る.
	bool slist_test_avaiable_font(const SHAPE_LIST& slist, wchar_t*& unavailable_font) noexcept;
	// 最後の図形を得る.
	Shape* slist_back(SHAPE_LIST const& slist) noexcept;
	// 図形リストを消去し, 含まれる図形を破棄する.
	void slist_clear(SHAPE_LIST& slist) noexcept;
	// 図形を種類別に数える.
	void slist_count(
		const SHAPE_LIST& slist, uint32_t& undeleted_cnt, uint32_t& selected_cnt,
		uint32_t& selected_group_cnt, uint32_t& runlength_cnt, uint32_t& selected_text_cnt, 
		uint32_t& text_cnt, uint32_t& selected_image_cnt, uint32_t& selected_arc_cnt,
		bool& fore_selected, bool& back_selected, bool& prev_selected) noexcept;
	// 先頭から図形まで数える.
	uint32_t slist_count(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// 最初の図形をリストから得る.
	Shape* slist_front(SHAPE_LIST const& slist) noexcept;
	// 図形と表示を囲む領域を得る.
	void slist_bound_view(
		SHAPE_LIST const& slist, const D2D1_SIZE_F sh_size, D2D1_POINT_2F& b_lt,
		D2D1_POINT_2F& b_rb) noexcept;
	// すべての図形を囲む領域をリストから得る.
	void slist_bound_all(
		SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept;
	// 選択された図形を囲む領域をリストから得る.
	bool slist_bound_selected(
		SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept;
	// 位置を含む図形とその部位を得る.
	uint32_t slist_hit_test(SHAPE_LIST const& slist, const D2D1_POINT_2F test, Shape*& s) noexcept;
	// 図形を挿入する.
	void slist_insert(SHAPE_LIST& slist, Shape* const s_ins, const Shape* s_at) noexcept;
	// 選択された図形を移動する
	bool slist_move_selected(SHAPE_LIST const& slist, const D2D1_POINT_2F pos) noexcept;
	// 図形のその次の図形を得る.
	Shape* slist_next(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// 図形のその前の図形を得る.
	Shape* slist_prev(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// データリーダーから図形リストを読み込む.
	bool slist_read(SHAPE_LIST& slist, const Shape& page, DataReader const& dt_reader);
	// 図形をリストから削除し, 削除した図形の次の図形を得る.
	Shape* slist_remove(SHAPE_LIST& slist, const Shape* s) noexcept;
	// 選択された図形のリストを得る.
	template <typename T> void slist_get_selected(
		SHAPE_LIST const& slist, SHAPE_LIST& t_list) noexcept;
	// データライターに図形リストを書き込む. REDUCE なら消去された図形は省く.
	template <bool REDUCE> void slist_write(const SHAPE_LIST& slist, DataWriter const& dt_writer);
	// リストの中の図形の順番を得る.
	template <typename S, typename T> bool slist_match(SHAPE_LIST const& slist, S s, T& t);
	// 選択されてない図形の頂点の中から 指定した位置に最も近い頂点を見つける.
	bool slist_find_vertex_closest(
		const SHAPE_LIST& slist, const D2D1_POINT_2F& p, const float d, 
		D2D1_POINT_2F& val) noexcept;

	//------------------------------
	// 図形のひな型
	//------------------------------
	struct Shape {
		static ID2D1RenderTarget* m_d2d_target;
		static winrt::com_ptr<ID2D1DrawingStateBlock> m_d2d_state_block;
		static winrt::com_ptr<ID2D1SolidColorBrush> m_d2d_color_brush;
		static winrt::com_ptr<ID2D1SolidColorBrush> m_d2d_range_brush;
		static winrt::com_ptr<ID2D1BitmapBrush> m_d2d_bitmap_brush;
		static winrt::com_ptr<ID2D1StrokeStyle1> m_aux_style;	// 補助線の形式
		static winrt::com_ptr<IDWriteFactory> m_dwrite_factory;
		static float m_aux_width;	// 補助線の太さ
		static bool m_anc_show;	// 図形の部位を表示する.
		static float m_anc_width;	// 図形の部位の大きさ
		static float m_anc_square_inner;	// 図形の部位 (正方形) の内側の大きさ
		static float m_anc_square_outer;	// 図形の部位 (正方形) の外の大きさ
		static float m_anc_circle_inner;	// 図形の部位 (円形) の内側の大きさ
		static float m_anc_circle_outer;	// 図形の部位 (円形) の外の大きさ

		// 描画環境の設定.
		void begin_draw(ID2D1RenderTarget* const target, const bool anc_show, 
			IWICFormatConverter* const background, const double scale);
		// 図形を破棄する.
		virtual ~Shape(void) noexcept {}	// 派生クラスがあるので必要
		// 図形を表示する.
		virtual void draw(void) = 0;
		// 矢じるしの寸法を得る
		virtual bool get_arrow_size(ARROW_SIZE& /*val*/) const noexcept { return false; }
		// 矢じるしの形式を得る.
		virtual bool get_arrow_style(ARROW_STYLE& /*val*/) const noexcept { return false; }
		// 図形を囲む領域を得る.
		virtual void get_bound(
			const D2D1_POINT_2F /*a_lt*/, const D2D1_POINT_2F /*a_rb*/, D2D1_POINT_2F& /*b_lt*/,
			D2D1_POINT_2F& /*b_rb*/) const noexcept {}
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
		// 書体の幅を得る.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH& /*val*/) const noexcept 
		{ return false; }
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
		// 線分の結合の尖り制限を得る.
		virtual bool get_join_miter_limit(float& /*val*/) const noexcept { return false; }
		// 線分の結合の形式を得る.
		virtual bool get_join_style(D2D1_LINE_JOIN& /*val*/) const noexcept { return false; }
		// 多角形の終端を得る.
		virtual bool get_poly_end(bool& /*val*/) const noexcept { return false; }
		// 近傍の頂点を見つける.
		virtual bool get_pos_nearest(
			const D2D1_POINT_2F /*p*/, float& /*dd*/, D2D1_POINT_2F& /*val*/) const noexcept
		{ return false; }
		// 部位の位置を得る.
		virtual	void get_pos_anc(const uint32_t /*anc*/, D2D1_POINT_2F&/*val*/) const noexcept {}
		// 図形を囲む領域の左上位置を得る.
		virtual void get_bound_lt(D2D1_POINT_2F& /*val*/) const noexcept {}
		// 開始位置を得る.
		virtual bool get_pos_start(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// ページの色を得る.
		virtual bool get_page_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// ページの余白を得る.
		virtual bool get_page_padding(D2D1_RECT_F& /*val*/) const noexcept { return false; }
		// ページ倍率を得る.
		virtual bool get_page_scale(float& /*val*/) const noexcept { return false; }
		// ページの大きさを得る.
		virtual bool get_page_size(D2D1_SIZE_F& /*val*/) const noexcept { return false; }
		// 線枠の色を得る.
		virtual bool get_stroke_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 書体の太さを得る
		virtual bool get_stroke_width(float& /*val*/) const noexcept { return false; }
		// 段落のそろえを得る.
		virtual bool get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT& /*val*/) const noexcept
		{ return false; }
		// 文字列のそろえを得る.
		virtual bool get_text_align_horz(DWRITE_TEXT_ALIGNMENT& /*val*/) const noexcept 
		{ return false; }
		// 文字列を得る.
		virtual bool get_text_content(wchar_t*& /*val*/) const noexcept { return false; }
		// 行間を得る.
		virtual bool get_text_line_sp(float& /*val*/) const noexcept { return false; }
		// 文字列の周囲の余白を得る.
		virtual bool get_text_padding(D2D1_SIZE_F& /*val*/) const noexcept { return false; }
		// 文字範囲を得る
		virtual bool get_text_selected(DWRITE_TEXT_RANGE& /*val*/) const noexcept { return false; }
		// 傾斜度を得る.
		virtual bool get_deg_rotation(float& /*val*/) const noexcept { return false; }
		// 頂点を得る.
		virtual size_t get_verts(D2D1_POINT_2F /*p*/[]) const noexcept { return 0; };
		// 位置を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F /*test*/) const noexcept
		{ return ANC_TYPE::ANC_PAGE; }
		// 範囲に含まれるか判定する.
		virtual bool in_area(const D2D1_POINT_2F /*area_lt*/, const D2D1_POINT_2F /*area_rb*/)
			const noexcept { return false; }
		// 消去されたか判定する.
		virtual bool is_deleted(void) const noexcept { return false; }
		// 選択されてるか判定する.
		virtual bool is_selected(void) const noexcept { return false; }
		// 位置を移動する.
		virtual	bool move(const D2D1_POINT_2F /*pos*/) noexcept { return false; }
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
		// 値を書体の幅に格納する.
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
		// 値を線の結合の尖り制限に格納する.
		virtual bool set_join_miter_limit(const float& /*val*/) noexcept { return false; }
		// 値を線の結合の形式に格納する.
		virtual bool set_join_style(const D2D1_LINE_JOIN& /*val*/) noexcept { return false; }
		// 多角形の終端を得る.
		virtual bool set_poly_end(const bool /*val*/) noexcept { return false; }
		// 値を, 部位の位置に格納する.
		virtual bool set_pos_anc(const D2D1_POINT_2F /*val*/, const uint32_t /*anc*/, 
			const float /*limit*/, const bool /*keep_aspect*/) noexcept { return false; }
		// 値を始点に格納する. 他の部位の位置も動く.
		virtual bool set_pos_start(const D2D1_POINT_2F /*val*/) noexcept { return false; }
		// 値をページの色に格納する.
		virtual bool set_page_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// ページの余白に格納する.
		virtual bool set_page_padding(const D2D1_RECT_F& /*val*/) noexcept { return false; }
		// 値をページ倍率に格納する.
		virtual bool set_page_scale(const float /*val*/) noexcept { return false; }
		// 値をページの大きさに格納する.
		virtual bool set_page_size(const D2D1_SIZE_F /*val*/) noexcept { return false; }
		// 値を傾斜度に格納する.
		virtual bool set_deg_rotation(const float /*val*/) noexcept { return false; }
		// 値を選択されてるか判定に格納する.
		virtual bool set_select(const bool /*val*/) noexcept { return false; }
		// 値を線枠の色に格納する.
		virtual bool set_stroke_color(const D2D1_COLOR_F& /*val*/) noexcept { return false; }
		// 値を書体の太さに格納する.
		virtual bool set_stroke_width(const float /*val*/) noexcept { return false; }
		// 値を段落のそろえに格納する.
		virtual bool set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT /*val*/) noexcept
		{ return false; }
		// 値を文字列のそろえに格納する.
		virtual bool set_text_align_horz(const DWRITE_TEXT_ALIGNMENT /*val*/) noexcept { return false; }
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
		virtual size_t export_pdf(const D2D1_SIZE_F /*page_size*/, DataWriter const& /*dt_writer*/)
		{ return 0; }
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(DataWriter const& /*dt_writer*/) {}
	};

	//------------------------------
	// 選択フラグ
	//------------------------------
	struct ShapeSelect : Shape {
		bool m_is_deleted = false;	// 消去されたか判定
		bool m_is_selected = false;	// 選択されたか判定

		// 図形を表示する.
		virtual void draw(void) = 0;
		// 消去されたか判定する.
		bool is_deleted(void) const noexcept final override { return m_is_deleted; }
		// 選択されてるか判定する.
		bool is_selected(void) const noexcept final override { return m_is_selected; }
		// 値を消去されたか判定に格納する.
		bool set_delete(const bool val) noexcept final override
		{
			if (m_is_deleted != val) {
				m_is_deleted = val; 
				return true; 
			}
			return false;
		}
		// 値を選択されてるか判定に格納する.
		bool set_select(const bool val) noexcept final override
		{
			if (m_is_selected != val) {
				m_is_selected = val;
				return true;
			}
			return false;
		}
		// 図形を作成する.
		ShapeSelect(void) {};	// 派生クラスがあるので必要
		// 図形をデータリーダーから読み込む.
		ShapeSelect(const DataReader& dt_reader) :
			m_is_deleted(dt_reader.ReadBoolean()),
			m_is_selected(dt_reader.ReadBoolean())
			{}
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const
		{
			dt_writer.WriteBoolean(m_is_deleted);
			dt_writer.WriteBoolean(m_is_selected);
		}
	};

	//------------------------------
	// 画像
	//------------------------------
	struct ShapeImage : ShapeSelect {
		static winrt::com_ptr<IWICImagingFactory2> wic_factory;	// WIC ファクトリー

		D2D1_POINT_2F	m_start;	// 始点
		D2D1_SIZE_F	m_view;	// 表示寸法
		D2D1_RECT_F	m_clip;	// 表示されている矩形
		D2D1_SIZE_U	m_orig;	// ビットマップの原寸
		uint8_t* m_bgra = nullptr;	// ビットマップのデータ
		D2D1_SIZE_F	m_ratio{ 1.0, 1.0 };	// 表示寸法と原寸の縦横比
		float m_opac = 1.0f;	// ビットマップの不透明度 (アルファ値と乗算)

		winrt::com_ptr<ID2D1Bitmap1> m_d2d_bitmap{ nullptr };	// D2D ビットマップ

		int m_pdf_image_cnt = 0;	// 画像オブジェクトの計数 (PDF として出力するときのみ使用)

		// 図形を破棄する.
		ShapeImage::~ShapeImage(void)
		{
			if (m_bgra != nullptr) {
				delete m_bgra;
				m_bgra = nullptr;
			}
			if (m_d2d_bitmap != nullptr) {
				m_d2d_bitmap = nullptr;
			}
		} // ~Shape

		//------------------------------
		// shape_image.cpp
		//------------------------------

		// ストリームに格納する.
		template <bool CLIP>
		IAsyncOperation<bool> copy(const winrt::guid enc_id, IRandomAccessStream& ra_stream) const;
		// 図形を表示する.
		void draw(void) final override;
		// 図形を囲む領域を得る.
		void get_bound(
			const D2D1_POINT_2F /*a_lt*/, const D2D1_POINT_2F /*a_rb*/, D2D1_POINT_2F& /*b_lt*/,
			D2D1_POINT_2F& /*b_rb*/) const noexcept final override;
		// 画像の不透明度を得る.
		bool get_image_opacity(float& /*val*/) const noexcept final override;
		// 画素の色を得る.
		bool get_pixcel(const D2D1_POINT_2F p, D2D1_COLOR_F& val) const noexcept;
		// 近傍の頂点を見つける.
		bool get_pos_nearest(
			const D2D1_POINT_2F /*p*/, float& /*dd*/, D2D1_POINT_2F& /*val*/) const noexcept final
			override;
		// 部位の位置を得る.
		void get_pos_anc(const uint32_t /*anc*/, D2D1_POINT_2F&/*val*/) const noexcept final override;
		// 図形を囲む領域の左上位置を得る.
		void get_bound_lt(D2D1_POINT_2F& /*val*/) const noexcept final override;
		// 開始位置を得る.
		bool get_pos_start(D2D1_POINT_2F& /*val*/) const noexcept final override;
		// 頂点を得る.
		size_t get_verts(D2D1_POINT_2F /*p*/[]) const noexcept final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F /*test*/) const noexcept final override;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F /*area_lt*/, const D2D1_POINT_2F /*area_rb*/) const noexcept final override;
		// 位置を移動する.
		bool move(const D2D1_POINT_2F pos) noexcept final override;
		// 原画像に戻す.
		void revert(void) noexcept;
		// 値を画像の不透明度に格納する.
		bool set_image_opacity(const float val) noexcept final override;
		// 値を, 部位の位置に格納する.
		bool set_pos_anc(
			const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect)
			noexcept final override;
		// 値を始点に格納する. 他の部位の位置も動く.
		bool set_pos_start(const D2D1_POINT_2F /*val*/) noexcept final override;
		// 図形を作成する.
		ShapeImage(
			const D2D1_POINT_2F p, const D2D1_SIZE_F page_size, const SoftwareBitmap& bitmap,
			const float opacity);
		// 図形をデータリーダーから読み込む.
		ShapeImage(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;

		//------------------------------
		// shape_export.cpp
		//------------------------------

		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// 図形をデータライターに SVG ファイルとして書き込む.
		winrt::Windows::Foundation::IAsyncAction export_as_svg_async(const DataWriter& dt_writer);
	};

	//------------------------------
	// 表示
	//------------------------------
	struct ShapePage : Shape {
		SHAPE_LIST m_shape_list{};	// 図形リスト

		// 矢じるし
		ARROW_SIZE m_arrow_size{ ARROW_SIZE_DEFVAL };	// 矢じるしの寸法
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// 矢じるしの形式

		// 角丸
		//D2D1_POINT_2F m_corner_radius{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL };	// 角丸半径

		// 塗りつぶし
		D2D1_COLOR_F m_fill_color{ COLOR_WHITE };	// 塗りつぶし色

		// 書体
		D2D1_COLOR_F m_font_color{ COLOR_BLACK };	// 書体の色
		wchar_t* m_font_family = nullptr;	// 書体名 (システムリソースに値が無かった場合)
		float m_font_size = FONT_SIZE_DEFVAL;	// 書体の大きさ (システムリソースに値が無かった場合)
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// 書体の幅 (システムリソースに値が無かった場合)
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// 書体の字体 (システムリソースに値が無かった場合)
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ (システムリソースに値が無かった場合)

		// 線・枠
		D2D1_CAP_STYLE m_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 破線の端の形式
		DASH_PATT m_dash_patt{ DASH_PATT_DEFVAL };	// 破線の配置
		D2D1_DASH_STYLE m_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// 破線の形式
		float m_join_miter_limit = MITER_LIMIT_DEFVAL;	// 線の結合の尖り制限
		D2D1_LINE_JOIN m_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;	// 線の結合の形式
		CAP_STYLE m_stroke_cap{ CAP_FLAT };	// 線の端の形式
		D2D1_COLOR_F m_stroke_color{ COLOR_BLACK };	// 線・枠の色
		float m_stroke_width = 1.0f;	// 線・枠の太さ

		// 文字列
		float m_text_line_sp = 0.0f;	// 行間 (DIPs 96dpi固定)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_vert = 	// 段落の揃え
			DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		DWRITE_TEXT_ALIGNMENT m_text_align_horz = 	// 文字列の揃え
			DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
		D2D1_SIZE_F m_text_padding{ TEXT_PADDING_DEFVAL };	// 文字列の左右と上下の余白

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

		// ページ
		D2D1_COLOR_F m_page_color{ COLOR_WHITE };	// 背景色
		float m_page_scale = 1.0f;	// 拡大率
		D2D1_SIZE_F	m_page_size{ PAGE_SIZE_DEFVAL };	// 大きさ (MainPage のコンストラクタで設定)
		D2D1_RECT_F m_page_padding{ 0.0f, 0.0f, 0.0f, 0.0f };	// ページの内余白

		//------------------------------
		// shape_page.cpp
		//------------------------------

		// 図形を表示する.
		void draw(void);
		// 曲線の補助線を表示する.
		void auxiliary_draw_bezi(
			ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
			const D2D1_POINT_2F start, const D2D1_POINT_2F pos);
		// だ円の補助線を表示する.
		void auxiliary_draw_elli(
			ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
			const D2D1_POINT_2F start, const D2D1_POINT_2F pos);
		// 直線の補助線を表示する.
		void auxiliary_draw_line(
			ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
			const D2D1_POINT_2F start, const D2D1_POINT_2F pos);
		// 方形の補助線を表示する.
		void auxiliary_draw_rect(
			ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
			const D2D1_POINT_2F start, const D2D1_POINT_2F pos);
		// 多角形の補助線を表示する.
		void auxiliary_draw_poly(
			ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
			const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const POLY_OPTION& p_opt);
		// 角丸方形の補助線を表示する.
		void auxiliary_draw_rrect(
			ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush,
			const D2D1_POINT_2F start, const D2D1_POINT_2F pos);
		// 四分円の補助線を表示する.
		void auxiliary_draw_qellipse(
			ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush, 
			const D2D1_POINT_2F start, const D2D1_POINT_2F pos);
		// 矢じるしの寸法を得る.
		bool get_arrow_size(ARROW_SIZE& val) const noexcept final override;
		// 矢じるしの形式を得る.
		bool get_arrow_style(ARROW_STYLE& val) const noexcept final override;
		// 端の形式を得る.
		bool get_stroke_cap(CAP_STYLE& val) const noexcept final override;
		// 角丸半径を得る.
		//bool get_corner_radius(D2D1_POINT_2F& val) const noexcept final override;
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
		// 書体の幅を得る.
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
		// 線の結合の尖り制限を得る.
		bool get_join_miter_limit(float& val) const noexcept final override;
		// 線の結合の形式を得る.
		bool get_join_style(D2D1_LINE_JOIN& val) const noexcept final override;
		// ページの色を得る.
		bool get_page_color(D2D1_COLOR_F& val) const noexcept final override;
		// ページ倍率を得る.
		bool get_page_scale(float& val) const noexcept final override;
		// ページの大きさを得る.
		bool get_page_size(D2D1_SIZE_F& val) const noexcept final override;
		// ページの余白を得る.
		bool get_page_padding(D2D1_RECT_F& val) const noexcept final override
		{
			val = m_page_padding;
			return true;
		}
		// 線枠の色を得る.
		bool get_stroke_color(D2D1_COLOR_F& val) const noexcept final override;
		// 書体の太さを得る
		bool get_stroke_width(float& val) const noexcept final override;
		// 段落のそろえを得る.
		bool get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept final override;
		// 文字列のそろえを得る.
		bool get_text_align_horz(DWRITE_TEXT_ALIGNMENT& val) const noexcept final override;
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
		//bool set_corner_radius(const D2D1_POINT_2F& val) noexcept final override;
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
		// 値を書体の幅に格納する.
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
		// 値を線の結合の尖り制限に格納する.
		bool set_join_miter_limit(const float& val) noexcept final override;
		// 値を線の結合の形式に格納する.
		bool set_join_style(const D2D1_LINE_JOIN& val) noexcept final override;
		// 値をページの色に格納する.
		bool set_page_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値をページの余白に格納する.
		bool set_page_padding(const D2D1_RECT_F& val) noexcept final override
		{
			if (!equal(m_page_padding, val)) {
				m_page_padding = val;
				return true;
			}
			return false;
		}
		// 値をページの倍率に格納する.
		bool set_page_scale(const float val) noexcept final override;
		// 値をページの大きさに格納する.
		bool set_page_size(const D2D1_SIZE_F val) noexcept final override;
		// 値を線枠の色に格納する.
		bool set_stroke_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を書体の太さに格納する.
		bool set_stroke_width(const float val) noexcept final override;
		// 値を段落のそろえに格納する.
		bool set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept final override;
		// 値を文字列のそろえに格納する.
		bool set_text_align_horz(const DWRITE_TEXT_ALIGNMENT val) noexcept final override;
		// 値を行間に格納する.
		bool set_text_line_sp(const float val) noexcept final override;
		// 値を文字列の余白に格納する.
		bool set_text_padding(const D2D1_SIZE_F val) noexcept final override;
		// 図形をデータリーダーに書き込む.
		void write(DataWriter const& dt_writer);
		size_t export_pdf_page(const D2D1_COLOR_F& background, DataWriter const& dt_writer);
		size_t export_pdf_grid(const D2D1_COLOR_F& background, DataWriter const& dt_writer);
		// 図形をデータライターに SVG として書き込む.
		void export_svg(DataWriter const& dt_writer);
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
		void draw(void) final override;
		// 図形を囲む領域を得る.
		void get_bound(
			const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt,
			D2D1_POINT_2F& b_rb) const noexcept final override;
		// 図形を囲む領域の左上位置を得る.
		void get_bound_lt(D2D1_POINT_2F& val) const noexcept final override;
		// 開始位置を得る.
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// 文字列図形を含むか判定する.
		bool has_text(void) noexcept;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F test) const noexcept final override;
		// 範囲に含まれるか判定する.
		bool in_area(
			const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept final 
			override;
		// 消去されているか判定する.
		bool is_deleted(void) const noexcept final override 
		{
			return m_list_grouped.size() == 0 || m_list_grouped.back()->is_deleted();
		}
		// 選択されているか判定する.
		bool is_selected(void) const noexcept final override 
		{
			return m_list_grouped.size() > 0 && m_list_grouped.back()->is_selected();
		}
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F pos) noexcept final override;
		// 値を消去されたか判定に格納する.
		bool set_delete(const bool val) noexcept final override;
		// 値を選択されたか判定に格納する.
		bool set_select(const bool val) noexcept final override;
		// 値を始点に格納する. 他の部位の位置も動く.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept final override;
		// 図形をデータリーダーから読み込む.
		ShapeGroup(const Shape& page, DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(const DataWriter& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		winrt::Windows::Foundation::IAsyncAction export_as_svg_async(const DataWriter& dt_writer);
		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer);
	};

	//------------------------------
	// 線枠のひな型
	//------------------------------
	struct ShapeStroke : ShapeSelect {
		CAP_STYLE m_stroke_cap{ CAP_FLAT };	// 線の端の形式
		D2D1_COLOR_F m_stroke_color{ COLOR_BLACK };	// 線・枠の色
		float m_stroke_width = 1.0f;	// 線・枠の太さ
		D2D1_CAP_STYLE m_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 破線の端の形式
		DASH_PATT m_dash_patt{ DASH_PATT_DEFVAL };	// 破線の配置
		D2D1_DASH_STYLE m_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// 破線の形式
		float m_join_miter_limit = MITER_LIMIT_DEFVAL;		// 線の結合の尖り制限
		D2D1_LINE_JOIN m_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;	// 線の結合の形式

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_stroke_style{};	// D2D ストロークスタイル

		// 図形を破棄する.
		virtual ~ShapeStroke(void)
		{
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
		} // ~Shape

		//------------------------------
		// shape_stroke.cpp
		//------------------------------

		// D2D ストロークスタイルを作成する.
		void create_stroke_style(ID2D1Factory* const factory);
		// 図形を表示する.
		virtual void draw(void) override = 0;
		// 端の形式を得る.
		bool get_stroke_cap(CAP_STYLE& val) const noexcept final override;
		// 破線の端の形式を得る.
		bool get_dash_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// 破線の配置を得る.
		bool get_dash_patt(DASH_PATT& val) const noexcept final override;
		// 破線の形式を得る.
		bool get_dash_style(D2D1_DASH_STYLE& val) const noexcept final override;
		// 線の結合の尖り制限を得る.
		bool get_join_miter_limit(float& val) const noexcept final override;
		// 線の結合の形式を得る.
		bool get_join_style(D2D1_LINE_JOIN& val) const noexcept final override;
		// 線枠の色を得る.
		bool get_stroke_color(D2D1_COLOR_F& val) const noexcept final override;
		// 線枠の太さを得る.
		bool get_stroke_width(float& val) const noexcept final override;
		// 位置を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F test) const noexcept override;
		// 値を端の形式に格納する.
		virtual	bool set_stroke_cap(const CAP_STYLE& val) noexcept override;
		// 値を破線の端の形式に格納する.
		bool set_dash_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// 値を破線の配置に格納する.
		bool set_dash_patt(const DASH_PATT& val) noexcept final override;
		// 値を線枠の形式に格納する.
		bool set_dash_style(const D2D1_DASH_STYLE val) noexcept final override;
		// 値を線の結合の尖り制限に格納する.
		virtual bool set_join_miter_limit(const float& val) noexcept override;
		// 値を線の結合の形式に格納する.
		virtual bool set_join_style(const D2D1_LINE_JOIN& val) noexcept override;
		// 値を線枠の色に格納する.
		bool set_stroke_color(const D2D1_COLOR_F& val) noexcept;
		// 値を線枠の太さに格納する.
		bool set_stroke_width(const float val) noexcept;
		// 図形を作成する.
		ShapeStroke(const Shape* page);
		// 図形をデータリーダーから読み込む.
		ShapeStroke(const Shape& page, DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 矢印つき直線
	//------------------------------
	struct ShapeLine : ShapeStroke {
		D2D1_POINT_2F m_start{ 0.0f, 0.0f };	// 始点
		std::vector<D2D1_POINT_2F> m_pos{};	// 次の点への位置ベクトル
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
		static bool line_get_pos_arrow(
			const D2D1_POINT_2F a_end, const D2D1_POINT_2F a_pos, const ARROW_SIZE& a_size, 
			/*--->*/D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept;

		// 図形を作成する.
		ShapeLine(const Shape* page, const bool e_close) :
			ShapeStroke(page)
		{
			if (e_close) {
				m_arrow_style = ARROW_STYLE::NONE;
			}
			else {
				page->get_arrow_style(m_arrow_style);
			}
			page->get_arrow_size(m_arrow_size);
		}
		// 図形を作成する.
		ShapeLine(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page);
		// データリーダーから図形を読み込む.
		ShapeLine(const Shape& page, DataReader const& dt_reader);
		// 図形を表示する.
		virtual void draw(void) override;
		// 矢じるしの寸法を得る.
		bool get_arrow_size(ARROW_SIZE& size) const noexcept final override;
		// 矢じるしの形式を得る.
		bool get_arrow_style(ARROW_STYLE& val) const noexcept final override;
		// 図形を囲む領域を得る.
		void get_bound(
			const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt,
			D2D1_POINT_2F& b_rb) const noexcept final override;
		// 塗りつぶし色を得る.
		virtual bool get_fill_color(D2D1_COLOR_F& val) const noexcept
		{
			val.a = 0.0f;
			return true;
		}
		// 部位の位置を得る.
		virtual void get_pos_anc(const uint32_t /*anc*/, D2D1_POINT_2F& val) const noexcept override;
		// 図形を囲む領域の左上位置を得る.
		void get_bound_lt(D2D1_POINT_2F& val) const noexcept final override;
		// 開始位置を得る.
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// 近傍の頂点を見つける.
		virtual bool get_pos_nearest(
			const D2D1_POINT_2F p, float& dd, D2D1_POINT_2F& val) const noexcept override;
		// 頂点を得る.
		virtual size_t get_verts(D2D1_POINT_2F p[]) const noexcept override;
		// 位置を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F test) const noexcept override;
		// 範囲に含まれるか判定する.
		virtual bool in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept override;
		// 値を矢じるしの寸法に格納する.
		virtual bool set_arrow_size(const ARROW_SIZE& val) noexcept override;
		// 値を矢じるしの形式に格納する.
		virtual bool set_arrow_style(const ARROW_STYLE val) noexcept override;
		// 値を, 部位の位置に格納する. 
		virtual bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept override;
		// 値を始点に格納する.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept override;
		// 差分だけ移動する.
		virtual bool move(const D2D1_POINT_2F pos) noexcept override;
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに PDF として書き込む.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer);
		// 図形をデータライターに SVG として書き込む.
		void export_svg(DataWriter const& dt_writer);
		// 値を端の形式に格納する.
		bool set_stroke_cap(const CAP_STYLE& val) noexcept final override;
		// 値を線の結合の尖り制限に格納する.
		bool set_join_miter_limit(const float& val) noexcept final override;
		// 値を線の結合の形式に格納する.
		bool set_join_style(const D2D1_LINE_JOIN& val) noexcept final override;
	};

	//------------------------------
	// 方形
	//------------------------------
	struct ShapeRect : ShapeStroke {
		D2D1_POINT_2F m_start{ 0.0f, 0.0f };	// 始点
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// 対角点へのベクトル
		D2D1_COLOR_F m_fill_color{ COLOR_WHITE };		// 塗りつぶし色

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// 図形を作成する.
		ShapeRect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page);
		// データリーダーから図形を読み込む.
		ShapeRect(const Shape& page, DataReader const& dt_reader);
		// 図形を表示する.
		virtual void draw_anc(void);
		// 図形を表示する.
		virtual void draw(void) override;
		// 図形を囲む領域を得る.
		void get_bound(
			const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt,
			D2D1_POINT_2F& b_rb) const noexcept final override;
		// 近傍の頂点を見つける.
		bool get_pos_nearest(
			const D2D1_POINT_2F p, float& dd, D2D1_POINT_2F& val) const noexcept final override;
		// 頂点を得る.
		size_t get_verts(D2D1_POINT_2F p[]) const noexcept final override;
		// 位置を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F test) const noexcept override;
		// 範囲に含まれるか判定する.
		virtual bool in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept override;
		// 塗りつぶし色を得る.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// 値を塗りつぶし色に格納する.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// 部位の位置を得る.
		virtual void get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept override;
		// 図形を囲む領域の左上位置を得る.
		void get_bound_lt(D2D1_POINT_2F& val) const noexcept final override;
		// 開始位置を得る
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F pos) noexcept;
		// 値を, 部位の位置に格納する.
		virtual bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept override;
		// 始点に値を格納する. 他の部位の位置も動く.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept;
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(DataWriter const& dt_writer);
		// 図形をデータライターに PDF として書き込む.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer);
	};

	//------------------------------
	// 定規
	//------------------------------
	struct ShapeRuler : ShapeRect {
		float m_grid_base = GRID_LEN_DEFVAL - 1.0f;	// 方眼の大きさ (を -1 した値)
		wchar_t* m_font_family = nullptr;	// 書体名
		float m_font_size = FONT_SIZE_DEFVAL;	// 書体の大きさ

		winrt::com_ptr<IDWriteTextFormat> m_dwrite_text_format{};	// テキストフォーマット
		int m_pdf_text_cnt = 0;

		// 図形を破棄する.
		ShapeRuler::~ShapeRuler(void)
		{
			if (m_dwrite_text_format != nullptr) {
				m_dwrite_text_format = nullptr;
			}
		} // ~ShapeStroke

		//------------------------------
		// shape_ruler.cpp
		//------------------------------

		// 文字列レイアウトを作成する.
		void create_text_format(void);
		// 図形を表示する.
		void draw(void) final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F test) const noexcept final override;
		// 字面を得る (使用後は Release する).
		bool get_font_face(IDWriteFontFace3*& face) const noexcept;
		// 書体名を得る.
		bool get_font_family(wchar_t*& val) const noexcept final override;
		// 書体の大きさを得る.
		bool get_font_size(float& val) const noexcept final override;
		// 値を書体名に格納する.
		bool set_font_family(wchar_t* const val) noexcept final override;
		// 値を書体の大きさに格納する.
		bool set_font_size(const float val) noexcept final override;
		// 図形を作成する.
		ShapeRuler(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page);
		// 図形をデータリーダーから読み込む.
		ShapeRuler(const Shape& page, DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(const DataWriter& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		void export_svg(const DataWriter& dt_writer);
		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer) final override;
	};

	//------------------------------
	// だ円
	//------------------------------
	struct ShapeEllipse : ShapeRect {
		// 図形を作成する.
		ShapeEllipse(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page) :
			ShapeRect::ShapeRect(start, pos, page)
		{}
		// 図形をデータリーダーから読み込む.
		ShapeEllipse(const Shape& page, DataReader const& dt_reader) :
			ShapeRect::ShapeRect(page, dt_reader)
		{}

		//------------------------------
		// shape_ellipse.cpp
		//------------------------------

		// 図形を表示する.
		void draw(void) final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F test) const noexcept final override;
		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// 図形をデータライターに SVG として書き込む.
		void export_svg(DataWriter const& dt_writer);
	};

	//------------------------------
	// 角丸方形
	//------------------------------
	struct ShapeRRect : ShapeRect {
		D2D1_POINT_2F m_corner_radius{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL };		// 角丸部分の半径

		//------------------------------
		// shape_rrect.cpp
		// 角丸方形
		//------------------------------

		// 図形を表示する.
		void draw(void) final override;
		// 角丸半径を得る.
		bool get_corner_radius(D2D1_POINT_2F& val) const noexcept final override;
		// 部位の位置を得る.
		void get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F test) const noexcept final override;
		//	値を, 部位の位置に格納する.
		bool set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept final override;
		// 図形を作成する.
		ShapeRRect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page);
		// 図形をデータリーダーから読み込む.
		ShapeRRect(const Shape& page, DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// 図形をデータライターに SVG として書き込む.
		void export_svg(DataWriter const& dt_writer);
	};

	//------------------------------
	// 折れ線のひな型
	//------------------------------
	struct ShapePath : ShapeLine {
		D2D1_COLOR_F m_fill_color{ 1.0f, 1.0f, 1.0f, 0.0f };
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_path_geom{ nullptr };	// 折れ線の D2D パスジオメトリ

		// 図形を作成する.
		ShapePath(const Shape* page, const bool e_closed) :
			ShapeLine::ShapeLine(page, e_closed)
		{
			page->get_fill_color(m_fill_color);
		}
		// 図形をデータリーダーから読み込む.
		ShapePath(const Shape& page, const DataReader& dt_reader);

		// 図形を破棄する.
		virtual ~ShapePath(void)
		{
			if (m_d2d_path_geom != nullptr) {
				m_d2d_path_geom = nullptr;
			}
			// ~ShapePath
		}

		//------------------------------
		// shape_path.cpp
		// 折れ線のひな型
		//------------------------------

		// 塗りつぶし色を得る.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F pos) noexcept final override;
		// 値を, 部位の位置に格納する.
		bool set_pos_anc(
			const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect)
			noexcept override;
		// 値を矢じるしの寸法に格納する.
		bool set_arrow_size(const ARROW_SIZE& val) noexcept final override;
		// 値を矢じるしの形式に格納する.
		virtual bool set_arrow_style(const ARROW_STYLE val) noexcept override;
		// 値を塗りつぶし色に格納する.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を始点に格納する. 他の部位の位置も動く.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept override;
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 多角形
	//------------------------------
	struct ShapePolygon : ShapePath {
		bool m_end_closed;	// 辺が閉じているか判定

		//------------------------------
		// shape_poly.cpp
		//------------------------------

		// 矢じりの先端と返しの位置を得る.
		static bool poly_get_pos_arrow(
			const size_t p_cnt, const D2D1_POINT_2F p[], const ARROW_SIZE& a_size, 
			D2D1_POINT_2F& tip, /*--->*/D2D1_POINT_2F barb[]) noexcept;
		// パスジオメトリを作成する.
		//void create_path_geometry(ID2D1Factory3* const factory) final override;
		// 矩形をもとに多角形を作成する.
		static void poly_create_by_box(
			const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const POLY_OPTION& p_opt,
			D2D1_POINT_2F p[]) noexcept;
		// 図形を表示する
		void draw(void) final override;
		// 多角形の終端を得る.
		bool get_poly_end(bool& val) const noexcept final override 
		{ val = m_end_closed; return true; }
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F test) const noexcept final override;
		// 範囲に含まれるか判定する.
		virtual bool in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept override;
		// 値を矢じるしの形式に格納する.
		bool set_arrow_style(const ARROW_STYLE val) noexcept final override;
		// 多角形の終端に格納する.
		bool set_poly_end(const bool val) noexcept final override 
		{
			if (m_end_closed != val) {
				m_end_closed = val;
				return true;
			}
			return false;
		}
		// 図形を作成する.
		ShapePolygon(
			const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page, const POLY_OPTION& p_opt);
		// 図形をデータリーダーから読み込む.
		ShapePolygon(const Shape& page, DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& /*dt_writer*/) const;
		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// 図形をデータライターに SVG として書き込む.
		void export_svg(DataWriter const& dt_writer);
	};

	//------------------------------
	// 曲線
	//------------------------------
	struct ShapeBezier : ShapePath {

		//------------------------------
		// SHAPE_bezier.cpp
		// ベジェ曲線
		//------------------------------

		// 矢じりの返しと先端の位置を得る
		static bool bezi_calc_arrow(const D2D1_POINT_2F start, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_SIZE a_size, D2D1_POINT_2F arrow[3]) noexcept;
		// 図形を表示する.
		void draw(void) final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F test) const noexcept final override;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept final override;
		// 図形を作成する.
		ShapeBezier(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page);
		// 図形をデータリーダーから読み込む.
		ShapeBezier(const Shape& page, DataReader const& dt_reader);
		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer) final override;
		// 図形をデータライターに SVG として書き込む.
		void export_svg(const DataWriter& dt_writer);
		// 図形をデータライターに書き込む.
		void write(const DataWriter& dt_writer) const final override;
	};

	// 四分だ円 (円弧)
	struct ShapeQEllipse : ShapePath {
		D2D1_SIZE_F m_radius{};	// 標準形にしたときの X 軸 Y 軸方向の半径
		float m_deg_rot = 0.0f;	// だ円の傾き
		float m_deg_start = 0.0f;	// 始点の角度
		float m_deg_end = 0.0f;	// 終点の角度
		D2D1_SWEEP_DIRECTION m_sweep_flag = D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE;	// 円弧の方向
		D2D1_ARC_SIZE m_larg_flag = D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL;
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_fill_geom;

		// だ円の中心点を得る.
		bool get_pos_center(D2D1_POINT_2F& val) const noexcept;
		// だ円の始点の角度を得る.
		bool get_deg_start(float& val) const noexcept
		{
			val = m_deg_start;
			return true;
		}
		// だ円の終点の角度を得る.
		bool get_deg_end(float& val) const noexcept
		{
			val = m_deg_end;
			return true;
		}
		// だ円の傾きを得る.
		bool get_deg_rotation(float& val) const noexcept final override
		{
			val = m_deg_rot;
			return true;
		}
		// 値を, 部位の位置に格納する.
		bool set_pos_anc(
			const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect)
			noexcept;
		// 値を始点に格納する. 他の部位の位置も動く.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept final override;
		// だ円の始点の角度に格納する.
		bool set_deg_start(const float val) noexcept
		{
			if (!equal(m_deg_start, val)) {
				m_deg_start = val;
				m_d2d_fill_geom = nullptr;
				m_d2d_path_geom = nullptr;
				m_d2d_arrow_geom = nullptr;
				return true;
			}
			return false;
		}
		// だ円の始点の角度に格納する.
		bool set_deg_end(const float val) noexcept
		{
			if (!equal(m_deg_end, val)) {
				m_deg_end = val;
				m_d2d_fill_geom = nullptr;
				m_d2d_path_geom = nullptr;
				m_d2d_arrow_geom = nullptr;
				return true;
			}
			return false;
		}
		// だ円の傾きに格納する.
		bool set_deg_rotation(const float val) noexcept final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F test) const noexcept final override;
		// 円弧をベジェ曲線で近似する.
		void alternate_bezier(
			const double t, D2D1_POINT_2F& start1, D2D1_BEZIER_SEGMENT& b_seg1,
			D2D1_POINT_2F& start2, D2D1_BEZIER_SEGMENT& b_seg2) const noexcept;
		// 矢じりの返しと先端の位置を得る.
		static bool qellipse_calc_arrow(const D2D1_POINT_2F vec, const D2D1_POINT_2F center, const D2D1_SIZE_F rad, const double rot, const ARROW_SIZE a_size, D2D1_POINT_2F arrow[]);
		// 図形を描く
		void draw(void) final override;
		// 図形をデータライターに SVG として書き込む.
		void export_svg(const DataWriter& dt_writer);
		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer);
		// 図形を作成する.
		ShapeQEllipse(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* page);
		// 図形をデータリーダーから読み込む.
		ShapeQEllipse(const Shape& page, const DataReader& dt_reader);
		// 図形をデータライターに書き込む.
		void write(const DataWriter& dt_writer) const final override;
		size_t get_verts(D2D1_POINT_2F /*p*/[]) const noexcept final override;
	};

	//------------------------------
	// 文字列
	//------------------------------
	struct ShapeText : ShapeRect {
		static wchar_t** s_available_fonts;		// 有効な書体名
		static D2D1_COLOR_F s_text_selected_background;	// 文字範囲の背景色
		static D2D1_COLOR_F s_text_selected_foreground;	// 文字範囲の文字色

		D2D1_COLOR_F m_font_color{ COLOR_BLACK };	// 書体の色
		wchar_t* m_font_family = nullptr;	// 書体名
		float m_font_size = FONT_SIZE_DEFVAL;	// 書体の大きさ
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// 書体の幅
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// 書体の字体
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// 書体の太さ
		wchar_t* m_text = nullptr;	// 文字列
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_vert = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// 段落のそろえ
		DWRITE_TEXT_ALIGNMENT m_text_align_horz = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// 文字のそろえ
		float m_text_line_sp = 0.0f;	// 行間 (DIPs 96dpi固定)
		D2D1_SIZE_F m_text_padding{ TEXT_PADDING_DEFVAL };	// 文字列の上下と左右の余白
		DWRITE_TEXT_RANGE m_text_selected_range{ 0, 0 };	// 選択された文字範囲

		DWRITE_FONT_METRICS m_dwrite_font_metrics{};	// 書体の計量
		DWRITE_LINE_METRICS* m_dwrite_line_metrics = nullptr;	// 行の計量
		UINT32 m_dwrite_selected_cnt = 0;	// 選択された文字範囲の計量の要素数
		DWRITE_HIT_TEST_METRICS* m_dwrite_selected_metrics = nullptr;	// 選択された文字範囲の計量
		UINT32 m_dwrite_test_cnt = 0;	// 位置の計量の要素数
		DWRITE_HIT_TEST_METRICS* m_dwrite_test_metrics = nullptr;	// 位置の計量
		winrt::com_ptr<IDWriteTextLayout> m_dwrite_text_layout{ nullptr };	// 文字列レイアウト

		int m_pdf_text_cnt = 0;	// PDF のフォント番号 (PDF 出力時のみ利用)

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
			if (m_dwrite_text_layout != nullptr) {
				//m_dwrite_text_layout->Release();
				m_dwrite_text_layout = nullptr;
			}
		} // ~ShapeStroke

		//------------------------------
		// shape_text.cpp
		// 文字列図形
		//------------------------------

		// 文字列レイアウトを作成する.
		void create_text_layout(void);
		// 枠を文字列に合わせる.
		bool fit_frame_to_text(const float g_len) noexcept;
		// 図形を表示する.
		void draw(void) final override;
		// 書体の色を得る.
		bool get_font_color(D2D1_COLOR_F& val) const noexcept final override;
		// 字面を得る (使用後は Release する).
		bool get_font_face(IDWriteFontFace3*& face) const noexcept;
		// 書体名を得る.
		bool get_font_family(wchar_t*& val) const noexcept final override;
		// 書体の大きさを得る.
		bool get_font_size(float& val) const noexcept final override;
		// 書体の幅を得る.
		bool get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept final override;
		// 書体の字体を得る.
		bool get_font_style(DWRITE_FONT_STYLE& val) const noexcept final override;
		// 書体の太さを得る.
		bool get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept final override;
		// 段落のそろえを得る.
		bool get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept final override;
		// 文字列のそろえを得る.
		bool get_text_align_horz(DWRITE_TEXT_ALIGNMENT& val) const noexcept final override;
		// 文字列を得る.
		bool get_text_content(wchar_t*& val) const noexcept final override;
		// 行間を得る.
		bool get_text_line_sp(float& val) const noexcept final override;
		// 文字列の余白を得る.
		bool get_text_padding(D2D1_SIZE_F& val) const noexcept final override;
		// 選択された文字範囲を得る.
		bool get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept final override;
		// 位置を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F test) const noexcept final override;
		// 範囲に含まれるか判定する.
		bool in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept final override;
		// 書体名が有効か判定し, 有効なら, 引数の書体名は破棄し, 有効な書体名の配列の要素と置き換える.
		static bool is_available_font(wchar_t*& font) noexcept;
		// 有効な書体名の配列を破棄する.
		static void release_available_fonts(void) noexcept;
		// 計量を破棄する.
		void relese_metrics(void) noexcept;
		// 有効な書体名の配列を設定する.
		static void set_available_fonts(void);
		// 値を書体の色に格納する.
		bool set_font_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を書体名に格納する.
		bool set_font_family(wchar_t* const val) noexcept final override;
		// 値を書体の大きさに格納する.
		bool set_font_size(const float val) noexcept final override;
		// 値を書体の幅に格納する.
		bool set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept final override;
		// 値を書体の字体に格納する.
		bool set_font_style(const DWRITE_FONT_STYLE val) noexcept final override;
		// 値を書体の太さに格納する.
		bool set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept final override;
		// 値を段落のそろえに格納する.
		bool set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept final override;
		// 値を文字列のそろえに格納する.
		bool set_text_align_horz(const DWRITE_TEXT_ALIGNMENT val) noexcept final override;
		// 値を文字列に格納する.
		bool set_text_content(wchar_t* const val) noexcept final override;
		// 値を行間に格納する.
		bool set_text_line_sp(const float val) noexcept final override;
		// 値を文字列の余白に格納する.
		bool set_text_padding(const D2D1_SIZE_F val) noexcept final override;
		// 値を文字範囲に格納する.
		bool set_text_selected(const DWRITE_TEXT_RANGE val) noexcept final override;
		// 図形を作成する.
		ShapeText(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, wchar_t* const text, const Shape* page);
		// 図形をデータリーダーから読み込む.
		ShapeText(const Shape& page, DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// データライターに SVG として書き込む.
		void export_svg(DataWriter const& dt_writer);
	};

	// 図形の部位（円形）を表示する.
	// p	部位の位置
	// target	描画ターゲット
	// brush	色ブラシ
	inline void anc_draw_circle(
		const D2D1_POINT_2F p, ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush)
	{
		const auto c_inner = Shape::m_anc_circle_inner;	// 内側の半径
		const auto c_outer = Shape::m_anc_circle_outer;	// 外側の半径
		const D2D1_ELLIPSE e_outer{	// 外側の円
			p, static_cast<FLOAT>(c_outer), static_cast<FLOAT>(c_outer)
		};
		const D2D1_ELLIPSE e_inner{	// 内側の円
			p, static_cast<FLOAT>(c_inner), static_cast<FLOAT>(c_inner)
		};
		brush->SetColor(COLOR_WHITE);
		target->FillEllipse(e_outer, brush);
		brush->SetColor(COLOR_BLACK);
		target->FillEllipse(e_inner, brush);
	}

	inline uint32_t conv_color_comp(const double c)
	{
		return min(static_cast<uint32_t>(floor(c * 256.0)), 255);
	}

	// 図形の部位 (正方形) を表示する.
	// p	部位の位置
	// target	描画ターゲット
	// brush	色ブラシ
	inline void anc_draw_square(
		const D2D1_POINT_2F p, ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush)
	{
		const auto a_inner = Shape::m_anc_square_inner;
		const auto a_outer = Shape::m_anc_square_outer;
		const D2D1_RECT_F r_inner{
			static_cast<FLOAT>(p.x - a_inner), static_cast<FLOAT>(p.y - a_inner),
			static_cast<FLOAT>(p.x + a_inner), static_cast<FLOAT>(p.y + a_inner)
		};
		const D2D1_RECT_F r_outer{
			static_cast<FLOAT>(p.x - a_outer), static_cast<FLOAT>(p.y - a_outer),
			static_cast<FLOAT>(p.x + a_outer), static_cast<FLOAT>(p.y + a_outer)
		};
		brush->SetColor(COLOR_WHITE);
		target->FillRectangle(r_outer, brush);
		brush->SetColor(COLOR_BLACK);
		target->FillRectangle(r_inner, brush);
	}

	// 矢じるしの寸法が同じか判定する.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept
	{
		return equal(a.m_width, b.m_width) && equal(a.m_length, b.m_length) &&
			equal(a.m_offset, b.m_offset);
	}

	// 端の形式が同じか判定する.
	inline bool equal(const CAP_STYLE& a, const CAP_STYLE& b) noexcept
	{
		return a.m_start == b.m_start && a.m_end == b.m_end;
	}

	// 色が同じか判定する.
	inline bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept
	{
		return equal_color_comp(a.a, b.a) && equal_color_comp(a.r, b.r) && 
			equal_color_comp(a.g, b.g) && equal_color_comp(a.b, b.b);
	}

	// 位置が同じか判定する.
	inline bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept
	{
		return equal(a.x, b.x) && equal(a.y, b.y);
	}

	// 方形が同じか判定する.
	inline bool equal(const D2D1_RECT_F& a, const D2D1_RECT_F& b) noexcept
	{
		return equal(a.left, b.left) && equal(a.top, b.top) && equal(a.right, b.right) &&
			equal(a.bottom, b.bottom);
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
		return equal(a.m_[0], b.m_[0]) && equal(a.m_[1], b.m_[1]) && equal(a.m_[2], b.m_[2]) &&
			equal(a.m_[3], b.m_[3]) && equal(a.m_[4], b.m_[4]) && equal(a.m_[5], b.m_[5]);
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
		return conv_color_comp(a) == conv_color_comp(b);
	}

	// 矢じりの返しの位置を求める.
	// a	矢軸ベクトル.
	// a_len	矢軸ベクトルの長さ
	// width	矢じりの幅 (返しの間の長さ)
	// len	矢じりの長さ (先端から返しまでの軸ベクトル上での長さ)
	// barb[2]	矢じりの返しの位置
	inline void get_pos_barbs(
		const D2D1_POINT_2F a, const double a_len, const double width, const double len,
		D2D1_POINT_2F barbs[]) noexcept
	{
		if (a_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z{ 0.0f, 0.0f };
			barbs[0] = Z;
			barbs[1] = Z;
		}
		else {
			const double bw = width * 0.5;	// 矢じるしの幅の半分の大きさ
			const double sx = a.x * -len;	// 矢軸ベクトルを矢じるしの長さ分反転
			const double sy = a.x * bw;
			const double tx = a.y * -len;
			const double ty = a.y * bw;
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
		//return conv_color_comp(val.a);
		return val.a * 256.0 >= 1.0;
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
	// a	図形の部位の位置
	// a_len	図形の部位の大きさ
	inline bool pt_in_anc(
		const D2D1_POINT_2F test, const D2D1_POINT_2F a, const double a_len) noexcept
	{
		const double dx = static_cast<double>(test.x) - static_cast<double>(a.x);
		const double dy = static_cast<double>(test.y) - static_cast<double>(a.y);
		return -a_len * 0.5 <= dx && dx <= a_len * 0.5 && -a_len * 0.5 <= dy && dy <= a_len * 0.5;
	}

	// 位置が円に含まれるか判定する.
	// center	円の中心点
	// radius	円の半径
	inline bool pt_in_circle(
		const D2D1_POINT_2F test, const D2D1_POINT_2F center, const double radius) noexcept
	{
		const double dx = static_cast<double>(test.x) - static_cast<double>(center.x);
		const double dy = static_cast<double>(test.y) - static_cast<double>(center.y);
		return dx * dx + dy * dy <= radius * radius;
	}

	// 位置が円に含まれるか判定する.
	// tets	判定される位置 (円の中心点を原点とする)
	// radius	円の半径
	inline bool pt_in_circle(const D2D1_POINT_2F test, const double radius) noexcept
	{
		const double tx = test.x;
		const double ty = test.y;
		return tx * tx + ty * ty <= radius * radius;
	}

	// だ円が位置を含むか判定する.
	// test	判定する位置
	// center	だ円の中心
	// rad_x	だ円の径
	// rad_y	だ円の径
	// rot	だ円の傾き (ラジアン)
	// 戻り値	含む場合 true
	inline bool pt_in_ellipse(
		const D2D1_POINT_2F test, const D2D1_POINT_2F center, const double rad_x,
		const double rad_y, const double rot) noexcept
	{
		// 標準形のだ円に合致するよう変換した位置を判定する.
		// だ円の中心を原点とする座標に判定する位置を平行移動.
		const double dx = static_cast<double>(test.x) - static_cast<double>(center.x);
		const double dy = static_cast<double>(test.y) - static_cast<double>(center.y);
		// だ円の傾きに合わせて判定する位置を回転.
		const double c = cos(rot);
		const double s = sin(rot);
		const double tx = c * dx + s * dy;
		const double ty = -s * dx + c * dy;

		const double aa = rad_x * rad_x;
		const double bb = rad_y * rad_y;
		return tx * tx / aa + ty * ty / bb <= 1.0;
	}

	// 多角形が位置を含むか判定する.
	// test	判定する位置
	// p_cnt	頂点の数
	// p	頂点の配列 [v_cnt]
	// 戻り値	含む場合 true
	// 多角形の各辺と, 指定された点を開始点とする水平線が交差する数を求める.
	inline bool pt_in_poly(
		const D2D1_POINT_2F test, const size_t p_cnt, const D2D1_POINT_2F p[]) noexcept
	{
		const double tx = test.x;
		const double ty = test.y;
		int i_cnt;	// 交点の数
		int i;

		double px = p[p_cnt - 1].x;
		double py = p[p_cnt - 1].y;
		i_cnt = 0;
		for (i = 0; i < p_cnt; i++) {
			double qx = p[i].x;
			double qy = p[i].y;
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
	// test	判定する位置
	// r_lt	方形の左上位置
	// r_rb	方形の右下位置
	// 戻り値	含む場合 true
	inline bool pt_in_rect2(
		const D2D1_POINT_2F test, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb) noexcept
	{
		return r_lt.x <= test.x && test.x <= r_rb.x && r_lt.y <= test.y && test.y <= r_rb.y;
	}

	// 方形が位置を含むか判定する.
	// test	判定する位置
	// r_lt	方形のいずれかの頂点
	// r_rb	r_lt に対して対角にある頂点
	// 戻り値	含む場合 true
	inline bool pt_in_rect(
		const D2D1_POINT_2F test, const D2D1_POINT_2F r_lt, const D2D1_POINT_2F r_rb) noexcept
	{
		const double lt_x = r_lt.x < r_rb.x ? r_lt.x : r_rb.x;	// 左上の x
		const double lt_y = r_lt.y < r_rb.y ? r_lt.y : r_rb.y;	// 左上の y
		const double rb_x = r_lt.x < r_rb.x ? r_rb.x : r_lt.x;	// 右下の x
		const double rb_y = r_lt.y < r_rb.y ? r_rb.y : r_lt.y;	// 右下の y
		return lt_x <= test.x && test.x <= rb_x && lt_y <= test.y && test.y <= rb_y;
	}

	// 位置にスカラーを掛けて, 位置を加える.
	// a	位置
	// b	スカラー値
	// c	加える位置
	// d	結果
	inline void pt_mul_add(
		const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
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
	inline void pt_mul_add(
		const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
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
		const size_t len = (s == nullptr || s[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(s));
		if (len > 0) {
			wchar_t* t;
			wcscpy_s(t = new wchar_t[len + 1], len + 1, s);
			return t;
		}
		return nullptr;
	}

}