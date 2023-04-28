#pragma once
//------------------------------
// shape.h
// shape.cpp	図形のひな型
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
#include "shape_pt.h"
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
// | ShapeArrow* |                                 | ShapeRect   |
// +------+------+                                 +------+------+
//        |                                               |
//        +---------------+                               |
//        |               |                               |
// +------+------+ +------+------+                        |
// | ShapePath*  | | ShapeLine   |                        |
// +------+------+ +------+------+                        |
//        |                                               |
//        +---------------+---------------+               +---------------+---------------+---------------+
//        |               |               |               |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+
// | ShapePoly   | | ShapeBezier | | ShapeArc    | | ShapeEllipse| | ShapeRRect  | | ShapeText   | | ShapeRuler  |
// +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+
//
// * 印つきは draw=0

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::IAsyncAction;
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

	constexpr D2D1_COLOR_F COLOR_ACCENT{ 0.0f, 0x78 / 255.0f, 0xD4 / 255.0f, 1.0f };	// 文字範囲の背景色 SystemAccentColor で上書き
	constexpr D2D1_COLOR_F COLOR_BLACK{ 0.0f, 0.0f, 0.0f, 1.0f };	// 黒
	constexpr D2D1_COLOR_F COLOR_WHITE{ 1.0f, 1.0f, 1.0f, 1.0f };	// 白
	constexpr D2D1_COLOR_F COLOR_TEXT_RANGE{ 1.0f, 1.0f, 1.0f, 1.0f };	// 文字範囲の文字色

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

	// 部位
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
	enum LOC_TYPE : uint32_t {
		LOC_PAGE,		// 図形の外部 (矢印カーソル)
		LOC_FILL,		// 図形の内部 (移動カーソル)
		LOC_STROKE,	// 線枠 (移動カーソル)
		LOC_TEXT,		// 文字列 (移動カーソル)
		LOC_NW,		// 方形の左上の頂点 (北西南東カーソル)
		LOC_SE,		// 方形の右下の頂点 (北西南東カーソル)
		LOC_NE,		// 方形の右上の頂点 (北東南西カーソル)
		LOC_SW,		// 方形の左下の頂点 (北東南西カーソル)
		LOC_NORTH,		// 方形の上辺の中点 (上下カーソル)
		LOC_SOUTH,		// 方形の下辺の中点 (上下カーソル)
		LOC_EAST,		// 方形の左辺の中点 (左右カーソル)
		LOC_WEST,		// 方形の右辺の中点 (左右カーソル)
		LOC_R_NW,		// 左上の角丸の中心点 (十字カーソル)
		LOC_R_NE,		// 右上の角丸の中心点 (十字カーソル)
		LOC_R_SE,		// 右下の角丸の中心点 (十字カーソル)
		LOC_R_SW,		// 左下の角丸の中心点 (十字カーソル)
		LOC_A_CENTER,	// 円弧の中心点
		LOC_A_START,	// 円弧の始点
		LOC_A_END,	// 円弧の終点
		LOC_START,	// 線分の始点
		LOC_END,	// 線分の終点
		LOC_P0,	// パスの始点 (十字カーソル)
	};

	// 矢じるしの大きさ
	struct ARROW_SIZE {
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
		float m_width;		// 返しの幅
		float m_length;		// 先端から返しまでの長さ
		float m_offset;		// 先端の位置
	};
	constexpr ARROW_SIZE ARROW_SIZE_DEFVAL{ 7.0, 16.0, 0.0 };	// 矢じるしの大きさの既定値
	constexpr float ARROW_SIZE_MAX = 127.5f;	// 矢じるしの各大きさの最大値

	// 矢じるしの形式
	enum struct ARROW_STYLE : uint32_t {
		ARROW_NONE,	// 矢じるしなし
		ARROW_OPENED,	// 開いた矢じるし
		ARROW_FILLED	// 閉じた矢じるし
	};

	// 破線の配置
	union DASH_PAT {
		float m_[6];
	};
	constexpr DASH_PAT DASH_PAT_DEFVAL{ { 4.0F, 3.0F, 1.0F, 3.0F, 1.0F, 3.0F } };	// 破線の配置の既定値

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
		COLOR_ACCENT.r, COLOR_ACCENT.g, COLOR_ACCENT.b, 192.0f / 255.0f
	};
	constexpr float GRID_LEN_DEFVAL = 48.0f;	// 方眼の長さの既定値
	constexpr float JOIN_MITER_LIMIT_DEFVAL = 10.0f;	// 尖り制限の既定値
	constexpr D2D1_SIZE_F TEXT_PAD_DEFVAL{ FONT_SIZE_DEFVAL / 4.0, FONT_SIZE_DEFVAL / 4.0 };	// 文字列の余白の既定値
	constexpr size_t N_GON_MAX = 256;	// 多角形の頂点の最大数 (ヒット判定でスタックを利用するため, オーバーフローしないよう制限する)
	constexpr float PAGE_SIZE_MAX = 32768.0f;	// 最大のページ大きさ
	constexpr D2D1_SIZE_F PAGE_SIZE_DEFVAL{ 8.0f * 96.0f, 11.0f * 96.0f };	// ページの大きさの既定値
	constexpr float FONT_SIZE_MAX = 512.0f;	// 書体の大きさの最大値
	constexpr D2D1_LINE_JOIN JOIN_STYLE_DEFVAL = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;

	// COM インターフェイス IMemoryBufferByteAccess を初期化
	MIDL_INTERFACE("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d")
		IMemoryBufferByteAccess : IUnknown
	{
		virtual HRESULT STDMETHODCALLTYPE GetBuffer(
			BYTE * *value,
			UINT32 * capacity
			);
	};

	// 部位（円形）を表示する.
	inline void loc_draw_circle(const D2D1_POINT_2F p, ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush) noexcept;
	// 部位（ひし型）を表示する.
	inline void loc_draw_rhombus(const D2D1_POINT_2F p, ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush) noexcept;
	// 部位 (方形) を表示する.
	inline void loc_draw_square(const D2D1_POINT_2F p, ID2D1RenderTarget* const target, ID2D1SolidColorBrush* const brush) noexcept;
	// 部位が点を含むか判定する.
	inline bool loc_hit_test(const D2D1_POINT_2F t, const D2D1_POINT_2F loc, const double len) noexcept;
	// 実数 (0.0...1.0) の色成分を整数 (0...255) に変換する.
	inline uint32_t conv_color_comp(const double c) noexcept;
	// 32ビット整数が同じか判定する.
	inline bool equal(const uint32_t a, const uint32_t b) noexcept { return a == b; };
	// 単精度浮動小数が同じか判定する.
	inline bool equal(const float a, const float b) noexcept;
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const double a, const double b) noexcept;
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const D2D1_CAP_STYLE a, const D2D1_CAP_STYLE b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const D2D1_LINE_JOIN a, const D2D1_LINE_JOIN b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const DWRITE_FONT_STRETCH a, const DWRITE_FONT_STRETCH b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const DWRITE_FONT_STYLE a, const DWRITE_FONT_STYLE b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const DWRITE_FONT_WEIGHT a, const DWRITE_FONT_WEIGHT b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const DWRITE_PARAGRAPH_ALIGNMENT a, const DWRITE_PARAGRAPH_ALIGNMENT b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const DWRITE_TEXT_ALIGNMENT a, const DWRITE_TEXT_ALIGNMENT b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const D2D1_SWEEP_DIRECTION a, const D2D1_SWEEP_DIRECTION b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const D2D1_DASH_STYLE a, const D2D1_DASH_STYLE b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	//inline bool equal(const GRID_EMPH a, const GRID_EMPH b) noexcept { return a.m_gauge_1 == b.m_gauge_1 && a.m_gauge_2 == b.m_gauge_2; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const GRID_SHOW a, const GRID_SHOW b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const D2D1_FIGURE_END a, const D2D1_FIGURE_END b) noexcept { return a == b; };
	// 倍精度浮動小数が同じか判定する.
	inline bool equal(const ARROW_STYLE a, const ARROW_STYLE b) noexcept { return a == b; };
	// 同値か判定する.
	//template<typename T> inline bool equal(const T a, const T b) noexcept { return a == b; };
	// 矢じるしの大きさが同じか判定する.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept;
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
	inline bool equal(const GRID_EMPH a, const GRID_EMPH b) noexcept;
	// 破線の配置が同じか判定する.
	inline bool equal(const DASH_PAT& a, const DASH_PAT& b) noexcept;
	// ワイド文字列が同じか判定する.
	inline bool equal(const wchar_t* a, const wchar_t* b) noexcept;
	// winrt 文字列が同じか判定する.
	inline bool equal(winrt::hstring const& a, const wchar_t* b) noexcept;
	// 色の成分が同じか判定する.
	inline bool equal_color_comp(const FLOAT a, const FLOAT b) noexcept;
	// 矢じりの返しの位置を求める.
	inline void get_pos_barbs(const D2D1_POINT_2F a, const double a_len, const double width, const double len, D2D1_POINT_2F barb[]) noexcept;
	// 色が不透明か判定する.
	inline bool is_opaque(const D2D1_COLOR_F& color) noexcept;
	// 文字列を複製する. 元の文字列がヌルポインター, または元の文字数が 0 のときは, ヌルポインターを返す.
	inline wchar_t* wchar_cpy(const wchar_t* const s) noexcept;
	// 文字列の長さ. 引数がヌルポインタの場合, 0 を返す.
	inline uint32_t wchar_len(const wchar_t* const t) noexcept;

	//------------------------------
	// shape_text.cpp
	//------------------------------

	// wchar_t 型の文字列 (UTF-16) を uint32_t 型の配列 (UTF-32) に変換する. 
	std::vector<uint32_t> text_utf16_to_utf32(const wchar_t* w, const size_t w_len) noexcept;
	// 字面を得る.
	template <typename T> bool text_get_font_face(T* src, const wchar_t* family, const DWRITE_FONT_WEIGHT weight, const DWRITE_FONT_STRETCH stretch, const DWRITE_FONT_STYLE style, IDWriteFontFace3*& face) noexcept;

	//------------------------------
	// shape_rect.cpp
	//------------------------------

	// 方形の頂点と中間点のうち, どの部位が点を含むか判定する.
	uint32_t rect_loc_hit_test(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const D2D1_POINT_2F t, const double a_len) noexcept;

	//------------------------------
	// shape_slist.cpp
	//------------------------------

	using SHAPE_LIST = std::list<struct Shape*>;

	// リスト中の最後の図形を得る.
	Shape* slist_back(SHAPE_LIST const& slist) noexcept;
	// リスト中の図形の境界矩形を得る.
	void slist_bbox_shape(SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept;
	// リスト中の選択された図形の境界矩形を得る.
	bool slist_bbox_selected(SHAPE_LIST const& slist, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) noexcept;
	// リスト中の文字列図形に, 利用できない書体があったならばその書体名を得る.
	bool slist_check_avaiable_font(const SHAPE_LIST& slist, wchar_t*& unavailable_font) noexcept;
	// 図形リストを消去し, 含まれる図形を破棄する.
	void slist_clear(SHAPE_LIST& slist) noexcept;
	// 図形を種類別に数える.
	void slist_count(
		const SHAPE_LIST& slist, uint32_t& undeleted_cnt, uint32_t& selected_cnt,
		uint32_t& selected_group_cnt, uint32_t& runlength_cnt, uint32_t& selected_text_cnt, 
		uint32_t& text_cnt, uint32_t& selected_image_cnt, uint32_t& selected_arc_cnt,
		uint32_t& selected_poly_open_cnt, uint32_t& selected_poly_close_cnt, bool& fore_selected,
		bool& back_selected, bool& prev_selected) noexcept;
	// 先頭から図形まで数える.
	uint32_t slist_count(SHAPE_LIST const& slist, const Shape* s) noexcept;
	// 最初の図形をリストから得る.
	Shape* slist_front(SHAPE_LIST const& slist) noexcept;
	// リスト中の図形が点を含むか判定する.
	uint32_t slist_hit_test(SHAPE_LIST const& slist, const D2D1_POINT_2F t, Shape*& s) noexcept;
	// リストに図形を挿入する.
	void slist_insert(SHAPE_LIST& slist, Shape* const s_ins, const Shape* s_at) noexcept;
	// リスト中の選択された図形を移動する
	bool slist_move_selected(SHAPE_LIST const& slist, const D2D1_POINT_2F pos) noexcept;
	// リスト中の図形のその次の図形を得る.
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
	// 選択されてない図形の頂点の中から 指定した点に最も近い点を見つける.
	bool slist_find_vertex_closest(const SHAPE_LIST& slist, const D2D1_POINT_2F& p, const double d, D2D1_POINT_2F& val) noexcept;

	//------------------------------
	// 図形のひな型
	//------------------------------
	struct Shape {
		static winrt::com_ptr<IDWriteFactory> m_dwrite_factory;	// DWrite ファクトリ
		static ID2D1RenderTarget* m_d2d_target;	// 描画対象
		static winrt::com_ptr<ID2D1DrawingStateBlock> m_state_block;	// 描画状態を保持するブロック
		static winrt::com_ptr<ID2D1SolidColorBrush> m_d2d_color_brush;	// 色ブラシ (ターゲット依存)
		static winrt::com_ptr<ID2D1SolidColorBrush> m_d2d_range_brush;	// 選択された文字色のブラシ (ターゲット依存)
		static winrt::com_ptr<ID2D1BitmapBrush> m_d2d_bitmap_brush;	// 背景の画像ブラシ (ターゲット依存)
		static winrt::com_ptr<ID2D1StrokeStyle1> m_aux_style;	// 補助線の形式
		static float m_aux_width;	// 補助線の太さ
		static bool m_loc_show;	// 部位を表示/非表示
		static float m_loc_width;	// 部位の大きさ
		static float m_loc_square_inner;	// 図形の部位 (正方形) の内側の辺の半分の長さ
		static float m_loc_square_outer;	// 図形の部位 (正方形) の外側の辺の半分の長さ
		static float m_loc_circle_inner;	// 図形の部位 (円形) の内側の半径
		static float m_loc_circle_outer;	// 図形の部位 (円形) の外側の半径
		static float m_loc_rhombus_inner;	// 図形の部位 (ひし型) の中心から内側の頂点までの半分の長さ
		static float m_loc_rhombus_outer;	// 図形の部位 (ひし型) の中心から外側の頂点までの半分の長さ

		// 図形を破棄する.
		virtual ~Shape(void) noexcept {}	// 派生クラスのデストラクタを呼ぶために仮想化が必要.
		// 描画前に必要な変数を格納する.
		void begin_draw(ID2D1RenderTarget* const target, const bool located, IWICFormatConverter* const background, const double scale) noexcept;
		// 図形を表示する.
		virtual void draw(void) noexcept = 0;
		// 図形をデータライターに PDF として書き込む.
		virtual size_t export_pdf(const D2D1_SIZE_F/*page_size*/, DataWriter const&/*dt_writer*/) { return 0; }
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(DataWriter const& /*dt_writer*/) noexcept {}
		// 円弧の方向を得る
		virtual bool get_arc_dir(D2D1_SWEEP_DIRECTION&/*val*/) const noexcept { return false; }
		// 円弧の終点の角度を得る.
		virtual bool get_arc_end(float&/*val*/) const noexcept { return false; }
		// 傾斜度を得る.
		virtual bool get_arc_rot(float&/*val*/) const noexcept { return false; }
		// 円弧の始点の角度を得る.
		virtual bool get_arc_start(float&/*val*/) const noexcept { return false; }
		// 矢じるしの寸法を得る
		virtual bool get_arrow_size(ARROW_SIZE&/*val*/) const noexcept { return false; }
		// 矢じるしの形式を得る.
		virtual bool get_arrow_style(ARROW_STYLE&/*val*/) const noexcept { return false; }
		// 矢じるしの返しの形式を得る
		virtual bool get_arrow_cap(D2D1_CAP_STYLE&/*val*/) const noexcept { return false; }
		// 矢じるしの先端の形式を得る.
		virtual bool get_arrow_join(D2D1_LINE_JOIN&/*val*/) const noexcept { return false; }
		// 境界矩形を得る.
		virtual void get_bbox(const D2D1_POINT_2F/*a_lt*/, const D2D1_POINT_2F/*a_rb*/, D2D1_POINT_2F&/*b_lt*/, D2D1_POINT_2F&/*b_rb*/) const noexcept {}
		// 境界矩形の左上点を得る.
		virtual void get_bbox_lt(D2D1_POINT_2F&/*val*/) const noexcept {}
		// 角丸半径を得る.
		virtual bool get_corner_radius(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// 破線の端の形式を得る.
		//virtual bool get_dash_cap(D2D1_CAP_STYLE& /*val*/) const noexcept { return false; }
		// 破線の配置を得る.
		virtual bool get_stroke_dash_pat(DASH_PAT& /*val*/) const noexcept { return false; }
		// 破線の形式を得る.
		virtual bool get_stroke_dash(D2D1_DASH_STYLE& /*val*/) const noexcept { return false; }
		// 塗りつぶし色を得る.
		virtual bool get_fill_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 書体の色を得る.
		virtual bool get_font_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// 書体名を得る.
		virtual bool get_font_family(wchar_t*& /*val*/) const noexcept { return false; }
		// 書体の大きさを得る.
		virtual bool get_font_size(float&/*val*/) const noexcept { return false; }
		// 書体の幅を得る.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH&/*val*/) const noexcept { return false; }
		// 書体の字体を得る.
		virtual bool get_font_style(DWRITE_FONT_STYLE&/*val*/) const noexcept { return false; }
		// 書体の太さを得る.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT&/*val*/) const noexcept { return false; }
		// 方眼の基準の大きさを得る.
		virtual bool get_grid_base(float&/*val*/) const noexcept { return false; }
		// 方眼の色を得る.
		virtual bool get_grid_color(D2D1_COLOR_F&/*val*/) const noexcept { return false; }
		// 方眼を強調を得る.
		virtual bool get_grid_emph(GRID_EMPH&/*val*/) const noexcept { return false; }
		// 方眼の表示を得る.
		virtual bool get_grid_show(GRID_SHOW&/*val*/) const noexcept { return false; }
		// 画像の不透明度を得る.
		virtual bool get_image_opacity(float&/*val*/) const noexcept { return false; }
		// 線分の結合の尖り制限を得る.
		virtual bool get_stroke_join_limit(float&/*val*/) const noexcept { return false; }
		// 線分の結合の形式を得る.
		virtual bool get_stroke_join(D2D1_LINE_JOIN&/*val*/) const noexcept { return false; }
		// 多角形の終端の形式を得る.
		virtual bool get_poly_end(D2D1_FIGURE_END& /*val*/) const noexcept { return false; }
		// 近傍の頂点を見つける.
		virtual bool get_pos_nearest(const D2D1_POINT_2F/*p*/, double&/*dd*/, D2D1_POINT_2F&/*val*/) const noexcept { return false; }
		// 指定した部位の点を得る.
		virtual	void get_pos_loc(const uint32_t/*loc*/, D2D1_POINT_2F&/*val*/) const noexcept {}
		// 始点を得る.
		virtual bool get_pos_start(D2D1_POINT_2F&/*val*/) const noexcept { return false; }
		// ページの色を得る.
		virtual bool get_page_color(D2D1_COLOR_F&/*val*/) const noexcept { return false; }
		// ページの余白を得る.
		virtual bool get_page_margin(D2D1_RECT_F&/*val*/) const noexcept { return false; }
		// ページの大きさを得る.
		virtual bool get_page_size(D2D1_SIZE_F&/*val*/) const noexcept { return false; }
		// 端の形式を得る.
		//virtual bool get_stroke_cap(CAP_STYLE& /*val*/) const noexcept { return false; }
		virtual bool get_stroke_cap(D2D1_CAP_STYLE& /*val*/) const noexcept { return false; }
		// 線枠の色を得る.
		virtual bool get_stroke_color(D2D1_COLOR_F&/*val*/) const noexcept { return false; }
		// 書体の太さを得る
		virtual bool get_stroke_width(float&/*val*/) const noexcept { return false; }
		// 段落のそろえを得る.
		virtual bool get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT&/*val*/) const noexcept { return false; }
		// 文字列のそろえを得る.
		virtual bool get_text_align_horz(DWRITE_TEXT_ALIGNMENT&/*val*/) const noexcept { return false; }
		// 文字列を得る.
		virtual bool get_text_content(wchar_t*&/*val*/) const noexcept { return false; }
		// 行間を得る.
		virtual bool get_text_line_sp(float&/*val*/) const noexcept { return false; }
		// 文字列の周囲の余白を得る.
		virtual bool get_text_pad(D2D1_SIZE_F&/*val*/) const noexcept { return false; }
		// 文字範囲を得る
		virtual bool get_text_selected(DWRITE_TEXT_RANGE&/*val*/) const noexcept { return false; }
		// 頂点を得る.
		virtual size_t get_verts(D2D1_POINT_2F/*p*/[]) const noexcept { return 0; };
		// 図形が点を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F/*t*/) const noexcept { return LOC_TYPE::LOC_PAGE; }
		// 矩形に含まれるか判定する.
		virtual bool is_inside(const D2D1_POINT_2F/*lt*/, const D2D1_POINT_2F/*rb*/) const noexcept { return false; }
		// 消去されたか判定する.
		virtual bool is_deleted(void) const noexcept { return false; }
		// 選択されてるか判定する.
		virtual bool is_selected(void) const noexcept { return false; }
		// 線の終端があるか判定する.
		virtual bool exist_cap(void) const noexcept { return false; }
		// 線の連結があるか判定する.
		virtual bool exist_join(void) const noexcept { return false; }
		// 図形を移動する.
		virtual	bool move(const D2D1_POINT_2F /*pos*/) noexcept { return false; }
		// 値を円弧の始点の角度に格納する.
		virtual bool set_arc_start(const float/* val*/) noexcept { return false; }
		// 値を円弧の終点の角度に格納する.
		virtual bool set_arc_end(const float/* val*/) noexcept { return false; }
		// 値を円弧の角度に格納する.
		virtual bool set_arc_rot(const float/*val*/) noexcept { return false; }
		// 値を矢じるしの寸法に格納する.
		virtual bool set_arrow_size(const ARROW_SIZE&/*val*/) noexcept { return false; }
		// 値を矢じるしの形式に格納する.
		virtual bool set_arrow_style(const ARROW_STYLE/*val*/) noexcept { return false; }
		// 値を矢じるしの返しの形式に格納する.
		virtual bool set_arrow_cap(const D2D1_CAP_STYLE/*val*/) noexcept { return false; }
		// 値を矢じるしの先端の形式に格納する.
		virtual bool set_arrow_join(const D2D1_LINE_JOIN/*val*/) noexcept { return false; }
		// 値を端の形式に格納する.
		virtual bool set_stroke_cap(const D2D1_CAP_STYLE&/*val*/) noexcept { return false; }
		// 値を角丸半径に格納する.
		virtual bool set_corner_radius(const D2D1_POINT_2F&/*alue*/) noexcept { return false; }
		// 値を破線の端の形式に格納する.
		//virtual bool set_dash_cap(const D2D1_CAP_STYLE&/*val*/) noexcept { return false; }
		// 値を破線の配置に格納する.
		virtual bool set_stroke_dash_pat(const DASH_PAT&/*val*/) noexcept { return false; }
		// 値を線枠の形式に格納する.
		virtual bool set_stroke_dash(const D2D1_DASH_STYLE/*val*/) noexcept { return false; }
		// 値を消去されたか判定に格納する.
		virtual bool set_delete(const bool/*val*/) noexcept { return false; }
		// 値を塗りつぶし色に格納する.
		virtual bool set_fill_color(const D2D1_COLOR_F&/*val*/) noexcept { return false; }
		// 値を書体の色に格納する.
		virtual bool set_font_color(const D2D1_COLOR_F&/*val*/) noexcept { return false; }
		// 値を書体名に格納する.
		virtual bool set_font_family(wchar_t* const/*val*/) noexcept { return false; }
		// 値を書体の大きさに格納する.
		virtual bool set_font_size(const float/*val*/) noexcept { return false; }
		// 値を書体の幅に格納する.
		virtual bool set_font_stretch(const DWRITE_FONT_STRETCH/*val*/) noexcept { return false; }
		// 値を書体の字体に格納する.
		virtual bool set_font_style(const DWRITE_FONT_STYLE/*val*/) noexcept { return false; }
		// 値を書体の太さに格納する.
		virtual bool set_font_weight(const DWRITE_FONT_WEIGHT/*val*/) noexcept { return false; }
		// 値を方眼の大きさに格納する.
		virtual bool set_grid_base(const float/*val*/) noexcept { return false; }
		// 値を方眼の色に格納する.
		virtual bool set_grid_color(const D2D1_COLOR_F&/*val*/) noexcept { return false; }
		// 値を方眼の強調に格納する.
		virtual bool set_grid_emph(const GRID_EMPH&/*val*/) noexcept { return false; }
		// 値を方眼の表示に格納する.
		virtual bool set_grid_show(const GRID_SHOW/*val*/) noexcept { return false; }
		// 値を方眼に合わせるに格納する.
		//virtual bool set_snap_grid(const bool/*val*/) noexcept { return false; }
		// 画像の不透明度を得る.
		virtual bool set_image_opacity(const float/*val*/) noexcept { return false; }
		// 値を線の結合の尖り制限に格納する.
		virtual bool set_stroke_join_limit(const float&/*val*/) noexcept { return false; }
		// 値を線の結合の形式に格納する.
		virtual bool set_stroke_join(const D2D1_LINE_JOIN&/*val*/) noexcept { return false; }
		// 多角形の終端を得る.
		virtual bool set_poly_end(const D2D1_FIGURE_END/*val*/) noexcept { return false; }
		// 値を, 指定した部位の点に格納する.
		virtual bool set_pos_loc(const D2D1_POINT_2F/*val*/, const uint32_t/*anc*/, const float/*snap_point*/, const bool/*keep_aspect*/) noexcept { return false; }
		// 値を始点に格納する. 他の部位の点も動く.
		virtual bool set_pos_start(const D2D1_POINT_2F/*val*/) noexcept { return false; }
		// 値をページの色に格納する.
		virtual bool set_page_color(const D2D1_COLOR_F&/*val*/) noexcept { return false; }
		// ページの余白に格納する.
		virtual bool set_page_margin(const D2D1_RECT_F&/*val*/) noexcept { return false; }
		// 値をページ倍率に格納する.
		//virtual bool set_page_scale(const float/*val*/) noexcept { return false; }
		// 値をページの大きさに格納する.
		virtual bool set_page_size(const D2D1_SIZE_F/*val*/) noexcept { return false; }
		// 値を選択されてるか判定に格納する.
		virtual bool set_select(const bool/*val*/) noexcept { return false; }
		// 値を線枠の色に格納する.
		virtual bool set_stroke_color(const D2D1_COLOR_F&/*val*/) noexcept { return false; }
		// 値を書体の太さに格納する.
		virtual bool set_stroke_width(const float/*val*/) noexcept { return false; }
		// 値を円弧の方向に格納する.
		virtual bool set_arc_dir(const D2D1_SWEEP_DIRECTION/*val*/) noexcept { return false; }
		// 値を段落のそろえに格納する.
		virtual bool set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT/*val*/) noexcept
		{ return false; }
		// 値を文字列のそろえに格納する.
		virtual bool set_text_align_horz(const DWRITE_TEXT_ALIGNMENT/*val*/) noexcept { return false; }
		// 値を文字列に格納する.
		virtual bool set_text_content(wchar_t* const/*val*/) noexcept { return false; }
		// 値を行間に格納する.
		virtual bool set_text_line_sp(const float/*val*/) noexcept { return false; }
		// 値を文字列の余白に格納する.
		virtual bool set_text_pad(const D2D1_SIZE_F/*val*/) noexcept { return false; }
		// 値を文字範囲に格納する.
		virtual bool set_text_selected(const DWRITE_TEXT_RANGE/*val*/) noexcept { return false; }
		// 図形をデータライターに書き込む.
		virtual void write(DataWriter const&/*dt_writer*/) const {}
	};

	//------------------------------
	// 選択フラグ
	//------------------------------
	struct ShapeSelect : Shape {
		bool m_is_deleted = false;	// 消去されたか判定
		bool m_is_selected = false;	// 選択されたか判定

		// 図形を表示する.
		virtual void draw(void) noexcept = 0;
		// 消去されたか判定する.
		virtual bool is_deleted(void) const noexcept final override { return m_is_deleted; }
		// 選択されてるか判定する.
		virtual bool is_selected(void) const noexcept final override { return m_is_selected; }
		// 値を消去されたか判定に格納する.
		virtual bool set_delete(const bool val) noexcept final override
		{
			if (m_is_deleted != val) {
				m_is_deleted = val; 
				return true; 
			}
			return false;
		}
		// 値を選択されてるか判定に格納する.
		virtual bool set_select(const bool val) noexcept final override
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
		ShapeSelect(const DataReader& dt_reader)
		{
			m_is_deleted = dt_reader.ReadBoolean();
			m_is_selected = dt_reader.ReadBoolean();
		}
		// 図形をデータライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const override
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

		D2D1_POINT_2F m_start;	// 始点
		D2D1_SIZE_F m_view;	// 表示寸法
		D2D1_RECT_F m_clip;	// 表示されている矩形
		D2D1_SIZE_U m_orig;	// ビットマップの原寸
		uint8_t* m_bgra = nullptr;	// ビットマップのデータ
		D2D1_SIZE_F m_ratio{ 1.0, 1.0 };	// 表示寸法と原寸の縦横比
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
		virtual void draw(void) noexcept final override;
		// 境界矩形を得る.
		virtual void get_bbox(const D2D1_POINT_2F /*a_lt*/, const D2D1_POINT_2F /*a_rb*/, D2D1_POINT_2F& /*b_lt*/, D2D1_POINT_2F& /*b_rb*/) const noexcept final override;
		// 境界矩形の左上点を得る.
		virtual void get_bbox_lt(D2D1_POINT_2F& /*val*/) const noexcept final override;
		// 画像の不透明度を得る.
		virtual bool get_image_opacity(float& /*val*/) const noexcept final override;
		// 画素の色を得る.
		bool get_pixcel(const D2D1_POINT_2F p, D2D1_COLOR_F& val) const noexcept;
		// 近傍の頂点を見つける.
		virtual bool get_pos_nearest(const D2D1_POINT_2F /*p*/, double& /*dd*/, D2D1_POINT_2F& /*val*/) const noexcept final override;
		// 指定した部位の点を得る.
		virtual void get_pos_loc(const uint32_t /*loc*/, D2D1_POINT_2F&/*val*/) const noexcept final override;
		// 開始点を得る.
		virtual bool get_pos_start(D2D1_POINT_2F& /*val*/) const noexcept final override;
		// 頂点を得る.
		virtual size_t get_verts(D2D1_POINT_2F /*p*/[]) const noexcept final override;
		// 図形が点を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F /*t*/) const noexcept final override;
		// 矩形に含まれるか判定する.
		virtual bool is_inside(const D2D1_POINT_2F/*lt*/, const D2D1_POINT_2F/*rb*/) const noexcept final override;
		// 図形を移動する.
		virtual bool move(const D2D1_POINT_2F pos) noexcept final override;
		// 原画像に戻す.
		void revert(void) noexcept;
		// 値を画像の不透明度に格納する.
		virtual bool set_image_opacity(const float val) noexcept final override;
		// 値を, 指定した部位の点に格納する.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept final override;
		// 値を始点に格納する. 他の部位の位置も動く.
		virtual bool set_pos_start(const D2D1_POINT_2F /*val*/) noexcept final override;
		// 図形を作成する.
		ShapeImage(const D2D1_POINT_2F p, const D2D1_SIZE_F page_size, const SoftwareBitmap& bitmap, const float opacity);
		// 図形をデータリーダーから読み込む.
		ShapeImage(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const override;

		//------------------------------
		// shape_pdf.cpp
		//------------------------------

		// 図形をデータライターに PDF として書き込む.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;

		//------------------------------
		// shape_svg.cpp
		//------------------------------

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
		ARROW_STYLE m_arrow_style = ARROW_STYLE::ARROW_NONE;	// 矢じるしの形式
		D2D1_CAP_STYLE m_arrow_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 矢じるしの返しの形式
		D2D1_LINE_JOIN m_arrow_join = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL;	// 矢じるしの先端の形式
		float m_arrow_join_limit = JOIN_MITER_LIMIT_DEFVAL;	// 矢じるしの尖り制限

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
		//D2D1_CAP_STYLE m_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 破線の端の形式
		DASH_PAT m_dash_pat{ DASH_PAT_DEFVAL };	// 破線の配置
		D2D1_DASH_STYLE m_stroke_dash = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// 破線の形式
		float m_stroke_join_limit = JOIN_MITER_LIMIT_DEFVAL;	// 線の結合の尖り制限
		D2D1_LINE_JOIN m_stroke_join = JOIN_STYLE_DEFVAL;	// 線の結合の形式
		//CAP_STYLE m_stroke_cap{ CAP_STYLE_FLAT };	// 線の端の形式
		D2D1_CAP_STYLE m_stroke_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 線の端の形式
		D2D1_COLOR_F m_stroke_color{ COLOR_BLACK };	// 線・枠の色
		float m_stroke_width = 1.0f;	// 線・枠の太さ

		// 文字列
		float m_text_line_sp = 0.0f;	// 行間 (DIPs 96dpi固定)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_vert = 	// 段落の揃え
			DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
		DWRITE_TEXT_ALIGNMENT m_text_align_horz = 	// 文字列の揃え
			DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;
		D2D1_SIZE_F m_text_pad{ TEXT_PAD_DEFVAL };	// 文字列の左右と上下の余白

		// 画像
		float m_image_opac = 1.0f;	// 画像の不透明度

		// 方眼
		float m_grid_base = GRID_LEN_DEFVAL - 1.0f;	// 方眼の基準の大きさ (を -1 した値)
		D2D1_COLOR_F m_grid_color{ GRID_COLOR_DEFVAL };	// 方眼の色
		GRID_EMPH m_grid_emph{ GRID_EMPH_0 };	// 方眼の強調
		D2D1_POINT_2F m_grid_offset{ 0.0f, 0.0f };	// 方眼のオフセット
		GRID_SHOW m_grid_show = GRID_SHOW::BACK;	// 方眼の表示
		//bool m_snap_grid = true;	// 方眼に合わせる

		// ページ
		D2D1_COLOR_F m_page_color{ COLOR_WHITE };	// 背景色
		D2D1_SIZE_F	m_page_size{ PAGE_SIZE_DEFVAL };	// 大きさ (MainPage のコンストラクタで設定)
		D2D1_RECT_F m_page_margin{ 0.0f, 0.0f, 0.0f, 0.0f };	// ページの内余白

		// 図形リストの最後の図形を得る.
		Shape* back() const noexcept
		{
			return m_shape_list.back();
		}

		//------------------------------
		// shape_page.cpp
		//------------------------------

		// 曲線の補助線を表示する.
		void auxiliary_draw_bezi(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// だ円の補助線を表示する.
		void auxiliary_draw_elli(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// 直線の補助線を表示する.
		void auxiliary_draw_line(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// 方形の補助線を表示する.
		void auxiliary_draw_rect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// 多角形の補助線を表示する.
		void auxiliary_draw_poly(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const POLY_OPTION& p_opt) noexcept;
		// 角丸方形の補助線を表示する.
		void auxiliary_draw_rrect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// 四分円の補助線を表示する.
		void auxiliary_draw_arc(const D2D1_POINT_2F start, const D2D1_POINT_2F pos) noexcept;
		// 図形を表示する.
		virtual void draw(void) noexcept final override;
		// 矢じるしの寸法を得る.
		virtual bool get_arrow_size(ARROW_SIZE& val) const noexcept final override;
		// 矢じるしの形式を得る.
		virtual bool get_arrow_style(ARROW_STYLE& val) const noexcept final override;
		// 矢じるしの返しの形式を得る.
		virtual bool get_arrow_cap(D2D1_CAP_STYLE& val) const noexcept final override
		{
			val = m_arrow_cap;
			return true;
		}
		// 矢じるしの先端の形式を得る.
		virtual bool get_arrow_join(D2D1_LINE_JOIN& val) const noexcept final override
		{
			val = m_arrow_join;
			return true;
		}
		// 端の形式を得る.
		//virtual bool get_stroke_cap(CAP_STYLE& val) const noexcept final override;
		virtual bool get_stroke_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// 破線の端の形式を得る.
		//virtual bool get_dash_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// 破線の配置を得る.
		virtual bool get_stroke_dash_pat(DASH_PAT& val) const noexcept final override;
		// 破線の形式を得る.
		virtual bool get_stroke_dash(D2D1_DASH_STYLE& val) const noexcept final override;
		// 塗りつぶし色を得る.
		virtual bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// 書体の色を得る.
		virtual bool get_font_color(D2D1_COLOR_F& val) const noexcept final override;
		// 書体名を得る.
		virtual bool get_font_family(wchar_t*& val) const noexcept final override;
		// 書体の大きさを得る.
		virtual bool get_font_size(float& val) const noexcept final override;
		// 書体の幅を得る.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept final override;
		// 書体の字体を得る.
		virtual bool get_font_style(DWRITE_FONT_STYLE& val) const noexcept final override;
		// 書体の太さを得る.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept final override;
		// 方眼の基準の大きさを得る.
		virtual bool get_grid_base(float& val) const noexcept final override;
		// 方眼の色を得る.
		virtual bool get_grid_color(D2D1_COLOR_F& val) const noexcept final override;
		// 方眼の強調を得る.
		virtual bool get_grid_emph(GRID_EMPH& val) const noexcept final override;
		// 方眼の表示の状態を得る.
		virtual bool get_grid_show(GRID_SHOW& val) const noexcept final override;
		// 方眼に合わせるを得る.
		//virtual bool get_snap_grid(bool& val) const noexcept final override;
		// 画像の不透明度を得る.
		virtual bool get_image_opacity(float& val) const noexcept final override;
		// 線の結合の尖り制限を得る.
		virtual bool get_stroke_join_limit(float& val) const noexcept final override;
		// 線の結合の形式を得る.
		virtual bool get_stroke_join(D2D1_LINE_JOIN& val) const noexcept final override;
		// ページの色を得る.
		virtual bool get_page_color(D2D1_COLOR_F& val) const noexcept final override;
		// ページの大きさを得る.
		virtual bool get_page_size(D2D1_SIZE_F& val) const noexcept final override;
		// ページの余白を得る.
		virtual bool get_page_margin(D2D1_RECT_F& val) const noexcept final override
		{
			val = m_page_margin;
			return true;
		}
		// 線枠の色を得る.
		virtual bool get_stroke_color(D2D1_COLOR_F& val) const noexcept final override;
		// 書体の太さを得る
		virtual bool get_stroke_width(float& val) const noexcept final override;
		// 段落のそろえを得る.
		virtual bool get_text_align_vert(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept final override;
		// 文字列のそろえを得る.
		virtual bool get_text_align_horz(DWRITE_TEXT_ALIGNMENT& val) const noexcept final override;
		// 行間を得る.
		virtual bool get_text_line_sp(float& val) const noexcept final override;
		// 文字列の周囲の余白を得る.
		virtual bool get_text_pad(D2D1_SIZE_F& val) const noexcept final override;
		// 図形をデータリーダーから読み込む.
		void read(DataReader const& dt_reader);
		// 値を矢じるしの返しの形式に格納する.
		virtual bool set_arrow_cap(const D2D1_CAP_STYLE val) noexcept final override;
		// 値を矢じるしの先端の形式に格納する.
		virtual bool set_arrow_join(const D2D1_LINE_JOIN val) noexcept final override;
		// 値を矢じるしの寸法に格納する.
		virtual bool set_arrow_size(const ARROW_SIZE& val) noexcept final override;
		// 値を矢じるしの形式に格納する.
		virtual bool set_arrow_style(const ARROW_STYLE val) noexcept final override;
		// 指定した図形から属性値を格納する.
		void set_attr_to(const Shape* s) noexcept;
		// 値を画像の不透明度に格納する.
		virtual bool set_image_opacity(const float val) noexcept final override;
		// 値を端の形式に格納する.
		//virtual bool set_stroke_cap(const CAP_STYLE& val) noexcept final override;
		virtual bool set_stroke_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// 値を破線の端の形式に格納する.
		//virtual bool set_dash_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// 値を破線の配置に格納する.
		virtual bool set_stroke_dash_pat(const DASH_PAT& val) noexcept final override;
		// 値を線枠の形式に格納する.
		virtual bool set_stroke_dash(const D2D1_DASH_STYLE val) noexcept final override;
		// 値を塗りつぶし色に格納する.
		virtual bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を書体の色に格納する.
		virtual bool set_font_color(const D2D1_COLOR_F& val) noexcept final override;
		// 書体名に格納する.
		virtual bool set_font_family(wchar_t* const val) noexcept final override;
		// 書体の大きさに格納する.
		virtual bool set_font_size(const float val) noexcept final override;
		// 値を書体の幅に格納する.
		virtual bool set_font_stretch(const DWRITE_FONT_STRETCH val) noexcept final override;
		// 値を書体の字体に格納する.
		virtual bool set_font_style(const DWRITE_FONT_STYLE val) noexcept final override;
		// 値を書体の太さに格納する.
		virtual bool set_font_weight(const DWRITE_FONT_WEIGHT val) noexcept final override;
		// 値を方眼の基準の大きさに格納する.
		virtual bool set_grid_base(const float val) noexcept final override;
		// 値を方眼の色に格納する.
		virtual bool set_grid_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を方眼の強調に格納する.
		virtual bool set_grid_emph(const GRID_EMPH& val) noexcept final override;
		// 値を方眼の表示に格納する.
		virtual bool set_grid_show(const GRID_SHOW val) noexcept final override;
		// 値を方眼に合わせるに格納する.
		//virtual bool set_snap_grid(const bool val) noexcept final override;
		// 値を線の結合の尖り制限に格納する.
		virtual bool set_stroke_join_limit(const float& val) noexcept final override;
		// 値を線の結合の形式に格納する.
		virtual bool set_stroke_join(const D2D1_LINE_JOIN& val) noexcept final override;
		// 値をページの色に格納する.
		virtual bool set_page_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値をページの余白に格納する.
		virtual bool set_page_margin(const D2D1_RECT_F& val) noexcept final override
		{
			if (!equal(m_page_margin, val)) {
				m_page_margin = val;
				return true;
			}
			return false;
		}
		// 値をページの倍率に格納する.
		//virtual bool set_page_scale(const float val) noexcept final override;
		// 値をページの大きさに格納する.
		virtual bool set_page_size(const D2D1_SIZE_F val) noexcept final override;
		// 値を線枠の色に格納する.
		virtual bool set_stroke_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を書体の太さに格納する.
		virtual bool set_stroke_width(const float val) noexcept final override;
		// 値を段落のそろえに格納する.
		virtual bool set_text_align_vert(const DWRITE_PARAGRAPH_ALIGNMENT val) noexcept final override;
		// 値を文字列のそろえに格納する.
		virtual bool set_text_align_horz(const DWRITE_TEXT_ALIGNMENT val) noexcept final override;
		// 値を行間に格納する.
		virtual bool set_text_line_sp(const float val) noexcept final override;
		// 値を文字列の余白に格納する.
		virtual bool set_text_pad(const D2D1_SIZE_F val) noexcept final override;
		// 図形をデータリーダーに書き込む.
		virtual void write(DataWriter const& dt_writer) const final override;
		size_t export_pdf_page(const D2D1_COLOR_F& background, DataWriter const& dt_writer);
		size_t export_pdf_grid(const D2D1_COLOR_F& background, DataWriter const& dt_writer);
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
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
		virtual void draw(void) noexcept final override;
		// 境界矩形を得る.
		virtual void get_bbox(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept final override;
		// 境界矩形の左上点を得る.
		virtual void get_bbox_lt(D2D1_POINT_2F& val) const noexcept final override;
		// 開始点を得る.
		virtual bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// 文字列図形を含むか判定する.
		bool has_text(void) noexcept;
		// 図形が点を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// 矩形に含まれるか判定する.
		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept final override;
		// 消去されているか判定する.
		virtual bool is_deleted(void) const noexcept final override
		{
			return m_list_grouped.size() == 0 || m_list_grouped.back()->is_deleted();
		}
		// 選択されているか判定する.
		virtual bool is_selected(void) const noexcept final override
		{
			return m_list_grouped.size() > 0 && m_list_grouped.back()->is_selected();
		}
		// 差分だけ移動する.
		virtual bool move(const D2D1_POINT_2F pos) noexcept final override;
		// 値を消去されたか判定に格納する.
		virtual bool set_delete(const bool val) noexcept final override;
		// 値を選択されたか判定に格納する.
		virtual bool set_select(const bool val) noexcept final override;
		// 値を始点に格納する. 他の部位の位置も動く.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept final override;
		// 図形をデータリーダーから読み込む.
		ShapeGroup(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		virtual void write(const DataWriter& dt_writer) const final override;
		// 図形をデータライターに SVG として書き込む.
		IAsyncAction export_as_svg_async(const DataWriter& dt_writer);
		// 図形をデータライターに PDF として書き込む.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer) final override;
	};

	//------------------------------
	// 線枠のひな型
	//------------------------------
	struct ShapeStroke : ShapeSelect {
		//CAP_STYLE m_stroke_cap{ CAP_STYLE_FLAT };	// 線の端の形式
		D2D1_CAP_STYLE m_stroke_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 線の端の形式
		D2D1_COLOR_F m_stroke_color{ COLOR_BLACK };	// 線・枠の色
		float m_stroke_width = 1.0f;	// 線・枠の太さ
		DASH_PAT m_dash_pat{ DASH_PAT_DEFVAL };	// 破線の配置
		D2D1_DASH_STYLE m_stroke_dash = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// 破線の形式
		float m_stroke_join_limit = JOIN_MITER_LIMIT_DEFVAL;		// 線の結合の尖り制限
		D2D1_LINE_JOIN m_stroke_join = JOIN_STYLE_DEFVAL;	// 線の結合の形式

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_stroke_style{};	// D2D ストロークスタイル

		// 図形を破棄する.
		virtual ~ShapeStroke(void)
		{
			m_d2d_stroke_style = nullptr;
		} // ~Shape

		//------------------------------
		// shape_stroke.cpp
		//------------------------------

		// D2D ストロークスタイルを作成する.
		HRESULT create_stroke_style(ID2D1Factory* const factory) noexcept;
		// 図形を表示する.
		virtual void draw(void) noexcept override = 0;
		// 端の形式を得る.
		//bool get_stroke_cap(CAP_STYLE& val) const noexcept final override;
		bool get_stroke_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// 破線の端の形式を得る.
		//bool get_dash_cap(D2D1_CAP_STYLE& val) const noexcept final override;
		// 破線の配置を得る.
		bool get_stroke_dash_pat(DASH_PAT& val) const noexcept final override;
		// 破線の形式を得る.
		bool get_stroke_dash(D2D1_DASH_STYLE& val) const noexcept final override;
		// 線の結合の尖り制限を得る.
		bool get_stroke_join_limit(float& val) const noexcept final override;
		// 線の結合の形式を得る.
		bool get_stroke_join(D2D1_LINE_JOIN& val) const noexcept final override;
		// 線枠の色を得る.
		bool get_stroke_color(D2D1_COLOR_F& val) const noexcept final override;
		// 線枠の太さを得る.
		bool get_stroke_width(float& val) const noexcept final override;
		// 図形が点を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept override;
		// 値を端の形式に格納する.
		//virtual bool set_stroke_cap(const CAP_STYLE& val) noexcept override;
		virtual bool set_stroke_cap(const D2D1_CAP_STYLE& val) noexcept override;
		// 値を破線の端の形式に格納する.
		//bool set_dash_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// 値を破線の配置に格納する.
		bool set_stroke_dash_pat(const DASH_PAT& val) noexcept final override;
		// 値を線枠の形式に格納する.
		bool set_stroke_dash(const D2D1_DASH_STYLE val) noexcept final override;
		// 値を線の結合の尖り制限に格納する.
		virtual bool set_stroke_join_limit(const float& val) noexcept override;
		// 値を線の結合の形式に格納する.
		virtual bool set_stroke_join(const D2D1_LINE_JOIN& val) noexcept override;
		// 値を線枠の色に格納する.
		bool set_stroke_color(const D2D1_COLOR_F& val) noexcept;
		// 値を線枠の太さに格納する.
		bool set_stroke_width(const float val) noexcept;
		// 図形を作成する.
		ShapeStroke(const Shape* prop);
		// 図形をデータリーダーから読み込む.
		ShapeStroke(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 方形
	//------------------------------
	struct ShapeOblong : ShapeStroke {
		D2D1_POINT_2F m_start{ 0.0f, 0.0f };	// 始点
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// 対角点へのベクトル
		D2D1_COLOR_F m_fill_color{ COLOR_WHITE };		// 塗りつぶし色

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// 図形を作成する.
		ShapeOblong(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop);
		// データリーダーから図形を読み込む.
		ShapeOblong(DataReader const& dt_reader);
		// 部位を表示する.
		void draw_loc(void) noexcept;
		// 図形を表示する.
		virtual void draw(void) noexcept override;
		// 境界矩形を得る.
		void get_bbox(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept final override;
		// 近傍の点を見つける.
		bool get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept final override;
		// 頂点を得る.
		size_t get_verts(D2D1_POINT_2F p[]) const noexcept final override;
		// 図形が点を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept override;
		// 矩形に含まれるか判定する.
		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept override;
		// 塗りつぶし色を得る.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// 値を塗りつぶし色に格納する.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// 指定した部位の点を得る.
		virtual void get_pos_loc(const uint32_t loc, D2D1_POINT_2F& val) const noexcept override;
		// 境界矩形の左上点を得る.
		void get_bbox_lt(D2D1_POINT_2F& val) const noexcept final override;
		// 開始点を得る
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F pos) noexcept;
		// 値を, 指定した部位の点に格納する.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept override;
		// 始点に値を格納する. 他の部位の位置も動く.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept;
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(DataWriter const& dt_writer) noexcept override;
		// 図形をデータライターに PDF として書き込む.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) override;
	};

	struct ShapeRect : ShapeOblong {
		ShapeRect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop) :
			ShapeOblong(start, pos, prop)
		{}
		ShapeRect(DataReader const& dt_reader) :
			ShapeOblong(dt_reader)
		{}
		// 線の連結があるか判定する.
		virtual bool exist_join(void) const noexcept final override
		{
			return true;
		}

	};

	//------------------------------
	// 定規
	//------------------------------
	struct ShapeRuler : ShapeOblong {
		float m_grid_base = GRID_LEN_DEFVAL - 1.0f;	// 方眼の大きさ (を -1 した値)
		wchar_t* m_font_family = nullptr;	// 書体名
		float m_font_size = FONT_SIZE_DEFVAL;	// 書体の大きさ

		winrt::com_ptr<IDWriteTextFormat> m_dwrite_text_format{};	// テキストフォーマット
		int m_pdf_text_cnt = 0;

		// 図形を破棄する.
		ShapeRuler::~ShapeRuler(void)
		{
			m_dwrite_text_format = nullptr;
		} // ~ShapeStroke

		//------------------------------
		// shape_ruler.cpp
		//------------------------------

		// 文字列レイアウトを作成する.
		HRESULT create_text_format(void) noexcept;
		// 図形を表示する.
		virtual void draw(void) noexcept final override;
		// 図形が点を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
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
		ShapeRuler(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop);
		// 図形をデータリーダーから読み込む.
		ShapeRuler(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(const DataWriter& dt_writer) const;
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(const DataWriter& dt_writer) noexcept final override;
		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer) final override;
	};

	//------------------------------
	// だ円
	//------------------------------
	struct ShapeEllipse : ShapeOblong {
		// 図形を作成する.
		ShapeEllipse(const D2D1_POINT_2F start,	// 始点
			const D2D1_POINT_2F pos,	// 終点への位置ベクトル
			const Shape* prop	// 属性
		) :
			ShapeOblong::ShapeOblong(start, pos, prop)
		{}
		// 図形をデータリーダーから読み込む.
		ShapeEllipse(DataReader const& dt_reader) :
			ShapeOblong::ShapeOblong(dt_reader)
		{}

		//------------------------------
		// shape_ellipse.cpp
		//------------------------------

		// 図形を表示する.
		virtual void draw(void) noexcept final override;
		// 図形が点を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
	};

	//------------------------------
	// 角丸方形
	//------------------------------
	struct ShapeRRect : ShapeOblong {
		D2D1_POINT_2F m_corner_radius{ GRID_LEN_DEFVAL, GRID_LEN_DEFVAL };		// 角丸部分の半径

		//------------------------------
		// shape_rrect.cpp
		// 角丸方形
		//------------------------------

		// 図形を表示する.
		virtual void draw(void) noexcept final override;
		// 角丸半径を得る.
		bool get_corner_radius(D2D1_POINT_2F& val) const noexcept final override;
		// 指定した部位の点を得る.
		virtual void get_pos_loc(const uint32_t loc, D2D1_POINT_2F& val) const noexcept final override;
		// 図形が点を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// 値を, 指定した部位の点に格納する.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept final override;
		// 図形を作成する.
		ShapeRRect(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop);
		// 図形をデータリーダーから読み込む.
		ShapeRRect(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに PDF として書き込む.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
	};

	//------------------------------
	// 矢じるし
	//------------------------------
	struct ShapeArrow : ShapeStroke {
		ARROW_STYLE m_arrow_style = ARROW_STYLE::ARROW_NONE;	// 矢じるしの形式
		ARROW_SIZE m_arrow_size{ ARROW_SIZE_DEFVAL };	// 矢じるしの寸法
		D2D1_CAP_STYLE m_arrow_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// 矢じるしの返しの形式
		D2D1_LINE_JOIN m_arrow_join = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL;	// 矢じるしの先端の形式
		float m_arrow_join_limit = JOIN_MITER_LIMIT_DEFVAL;	// 矢じるしの尖り制限

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_arrow_stroke{ nullptr };	// 矢じるしの D2D ストロークスタイル
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_arrow_geom{ nullptr };	// 矢じるしの D2D パスジオメトリ

		// 線の終端があるか判定する.
		virtual bool exist_cap(void) const noexcept override { return true; }
		// 矢じるしのストロークを作成する.
		HRESULT create_arrow_stroke(void) noexcept
		{
			m_d2d_arrow_stroke = nullptr;
			ID2D1Factory* factory = nullptr;
			Shape::m_d2d_target->GetFactory(&factory);

			// 矢じるしはかならずｊとする.
			const D2D1_STROKE_STYLE_PROPERTIES s_prop{
				m_arrow_cap,	// startCap
				m_arrow_cap,	// endCap
				D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// dashCap
				m_arrow_join,	// lineJoin
				static_cast<FLOAT>(m_stroke_join_limit),	// miterLimit
				D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
				0.0f	// dashOffset
			};
			return factory->CreateStrokeStyle(s_prop, nullptr, 0, m_d2d_arrow_stroke.put());
		}
		// 図形を表示する.
		virtual void draw(void) noexcept = 0;

		virtual bool get_arrow_size(ARROW_SIZE& val) const noexcept final override
		{
			val = m_arrow_size;
			return true;
		}

		virtual bool get_arrow_style(ARROW_STYLE& val) const noexcept final override
		{
			val = m_arrow_style;
			return true;
		}
		virtual bool get_arrow_cap(D2D1_CAP_STYLE& val) const noexcept final override
		{
			val = m_arrow_cap;
			return true;
		}
		virtual bool get_arrow_join(D2D1_LINE_JOIN& val) const noexcept final override
		{
			val = m_arrow_join;
			return true;
		}

		// 図形を破棄する.
		virtual ~ShapeArrow(void)
		{
			m_d2d_arrow_geom = nullptr;
			m_d2d_arrow_stroke = nullptr;
		} // ~ShapeStroke

		// 値を矢じるしの寸法に格納する.
		bool set_arrow_size(const ARROW_SIZE& val) noexcept final override
		{
			if (!equal(m_arrow_size, val)) {
				m_arrow_size = val;
				m_d2d_arrow_geom = nullptr;
				return true;
			}
			return false;
		}

		// 値を矢じるしの形式に格納する.
		bool set_arrow_style(const ARROW_STYLE val) noexcept override
		{
			if (m_arrow_style != val) {
				m_arrow_style = val;
				m_d2d_arrow_geom = nullptr;
				m_d2d_arrow_stroke = nullptr;
				return true;
			}
			return false;
		}

		// 値を矢じるしの返しの形式に格納する.
		bool set_arrow_cap(const D2D1_CAP_STYLE val) noexcept override
		{
			if (m_arrow_cap != val) {
				m_arrow_cap = val;
				m_d2d_arrow_stroke = nullptr;
				return true;
			}
			return false;
		}

		// 値を矢じるしの先端の形式に格納する.
		bool set_arrow_join(const D2D1_LINE_JOIN val) noexcept override
		{
			if (m_arrow_join != val) {
				m_arrow_join = val;
				m_d2d_arrow_stroke = nullptr;
				return true;
			}
			return false;
		}

		ShapeArrow(const Shape* prop) :
			ShapeStroke(prop),
			m_arrow_style([prop]() {
				ARROW_STYLE a_style;
				prop->get_arrow_style(a_style);
				return a_style;
				}()),
			m_arrow_size([prop]() {
				ARROW_SIZE a_size;
				prop->get_arrow_size(a_size);
				return a_size;
			}()),
			m_arrow_cap([prop]() {
				D2D1_CAP_STYLE a_cap;
				prop->get_arrow_cap(a_cap);
				return a_cap;
			}()),
			m_arrow_join([prop]() {
				D2D1_LINE_JOIN a_join;
				prop->get_arrow_join(a_join);
				return a_join;
			}())
			{}

		ShapeArrow(const DataReader& dt_reader) :
			ShapeStroke(dt_reader)
		{
			const ARROW_STYLE a_style = static_cast<ARROW_STYLE>(dt_reader.ReadUInt32());
			const ARROW_SIZE a_size{
				dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle()
			};
			const D2D1_CAP_STYLE a_cap = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
			const D2D1_LINE_JOIN a_join = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());

			if (a_style == ARROW_STYLE::ARROW_NONE || a_style == ARROW_STYLE::ARROW_OPENED || a_style == ARROW_STYLE::ARROW_FILLED) {
				m_arrow_style = a_style;
			}
			if (a_size.m_width >= 0.0f &&
				a_size.m_length >= 0.0f &&
				a_size.m_offset >= 0.0f) {
				m_arrow_size = a_size;
			}
			if (a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT ||
				a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND ||
				a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE ||
				a_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				m_arrow_cap = a_cap;
			}
			if (a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL ||
				a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER ||
				a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL ||
				a_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				m_arrow_join = a_join;
			}
		}

		void write(DataWriter const& dt_writer) const
		{
			ShapeStroke::write(dt_writer);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
			dt_writer.WriteSingle(m_arrow_size.m_width);
			dt_writer.WriteSingle(m_arrow_size.m_length);
			dt_writer.WriteSingle(m_arrow_size.m_offset);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_cap));
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_join));
		}

	};

	struct ShapeLine : ShapeArrow {
		D2D1_POINT_2F m_start{ 0.0f, 0.0f };	// 始点
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// 次の点への位置ベクトル

		//------------------------------
		// shape_line.cpp
		//------------------------------

		// 矢じるしの先端と返しの位置を求める.
		static bool line_get_pos_arrow(const D2D1_POINT_2F a_end, const D2D1_POINT_2F a_pos, const ARROW_SIZE& a_size, /*--->*/D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept;
		// 図形を作成する.
		ShapeLine(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prpp);
		// データリーダーから図形を読み込む.
		ShapeLine(DataReader const& dt_reader);
		// 図形を表示する.
		virtual void draw(void) noexcept override;
		// 境界矩形を得る.
		virtual void get_bbox(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept override;
		// 指定した部位の点を得る.
		virtual void get_pos_loc(const uint32_t /*loc*/, D2D1_POINT_2F& val) const noexcept override;
		// 境界矩形の左上点を得る.
		virtual void get_bbox_lt(D2D1_POINT_2F& val) const noexcept override;
		// 開始点を得る.
		bool get_pos_start(D2D1_POINT_2F& val) const noexcept final override;
		// 近傍の頂点を見つける.
		virtual bool get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept override;
		// 点を得る.
		virtual size_t get_verts(D2D1_POINT_2F p[]) const noexcept override;
		// 図形が点を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept override;
		// 矩形に含まれるか判定する.
		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept override;
		// 値を, 指定した部位の点に格納する.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept override;
		// 値を始点に格納する.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept override;
		// 差分だけ移動する.
		virtual bool move(const D2D1_POINT_2F pos) noexcept override;
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// 図形をデータライターに PDF として書き込む.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer);
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
		// 値を端の形式に格納する.
		//bool set_stroke_cap(const CAP_STYLE& val) noexcept final override;
		bool set_stroke_cap(const D2D1_CAP_STYLE& val) noexcept final override;
		// 値を線の結合の尖り制限に格納する.
		bool set_stroke_join_limit(const float& val) noexcept final override;
		// 値を線の結合の形式に格納する.
		bool set_stroke_join(const D2D1_LINE_JOIN& val) noexcept final override;
	};

	//------------------------------
	// 折れ線のひな型
	//------------------------------
	struct ShapePath : ShapeArrow {
		D2D1_POINT_2F m_start{ 0.0f, 0.0f };	// 始点
		std::vector<D2D1_POINT_2F> m_pos{};	// 次の点への位置ベクトル
		D2D1_COLOR_F m_fill_color{ 1.0f, 1.0f, 1.0f, 0.0f };

		winrt::com_ptr<ID2D1PathGeometry> m_d2d_path_geom{ nullptr };	// 折れ線の D2D パスジオメトリ

		// 図形を表示する.
		virtual void draw(void) noexcept = 0;
		// 近傍の頂点を見つける.
		virtual bool get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept override;
		// 値を, 指定した部位の点に格納する.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept override;
		// 頂点を得る.
		virtual size_t get_verts(D2D1_POINT_2F p[]) const noexcept override;
		// 境界矩形を得る.
		void get_bbox(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept final override;
		// 境界矩形の左上点を得る.
		void get_bbox_lt(D2D1_POINT_2F& val) const noexcept final override;
		// 指定した部位の点を得る.
		virtual void get_pos_loc(const uint32_t /*loc*/, D2D1_POINT_2F& val) const noexcept override;
		// 図形を作成する.
		ShapePath(
			const Shape* prop,
			const bool e_closed) :
			ShapeArrow::ShapeArrow(prop)
		{
			prop->get_fill_color(m_fill_color);
			if (e_closed) {
				set_arrow_style(ARROW_STYLE::ARROW_NONE);
			}
		}

		// 図形を破棄する.
		virtual ~ShapePath(void)
		{
			if (m_d2d_path_geom != nullptr) {
				m_d2d_path_geom = nullptr;
			}
		}	// ~ShapePath

		//------------------------------
		// shape_path.cpp
		// 折れ線のひな型
		//------------------------------

		// 塗りつぶし色を得る.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept final override;
		// 差分だけ移動する.
		bool move(const D2D1_POINT_2F pos) noexcept final override;
		// 値を塗りつぶし色に格納する.
		bool set_fill_color(const D2D1_COLOR_F& val) noexcept final override;
		// 値を始点に格納する. 他の部位の位置も動く.
		bool set_pos_start(const D2D1_POINT_2F val) noexcept override;
		// 図形をデータリーダーから読み込む.
		ShapePath(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// 多角形
	//------------------------------
	struct ShapePoly : ShapePath {
		D2D1_FIGURE_END m_end = D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED;	// 終端の状態
		// 線の連結があるか判定する.
		virtual bool exist_join(void) const noexcept final override
		{
			return true;
		}
		// 線の終端があるか判定する.
		virtual bool exist_cap(void) const noexcept final override
		{
			return m_end == D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN;
		}
		//------------------------------
		// shape_poly.cpp
		//------------------------------

		// 矢じりの先端と返しの位置を得る.
		static bool poly_get_pos_arrow(const size_t p_cnt, const D2D1_POINT_2F p[], const ARROW_SIZE& a_size, D2D1_POINT_2F barb[], D2D1_POINT_2F& tip) noexcept;
		// 矩形をもとに多角形を作成する.
		static void poly_create_by_box(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const POLY_OPTION& p_opt, D2D1_POINT_2F p[]) noexcept;
		// 図形を表示する
		virtual void draw(void) noexcept final override;
		// 辺が閉じているかを得る.
		virtual bool get_poly_end(D2D1_FIGURE_END& val) const noexcept final override
		{
			val = m_end;
			return true;
		}
		// 図形が点を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// 矩形に含まれるか判定する.
		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept override;
		// 値を矢じるしの形式に格納する.
		virtual bool set_arrow_style(const ARROW_STYLE val) noexcept final override;
		// 値を辺が閉じているかに格納する.
		virtual bool set_poly_end(const D2D1_FIGURE_END val) noexcept final override
		{
			if (m_end != val) {
				// 多角形が閉じているのに矢じるしがついているならば, 矢じるしを外す.
				if (val == D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED && m_arrow_style != ARROW_STYLE::ARROW_NONE) {
					m_arrow_style = ARROW_STYLE::ARROW_NONE;
				}
				m_end = val;
				m_d2d_arrow_geom = nullptr;
				m_d2d_path_geom = nullptr;
				return true;
			}
			return false;
		}
		// 図形を作成する.
		ShapePoly(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop, const POLY_OPTION& p_opt);
		// 図形をデータリーダーから読み込む.
		ShapePoly(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		virtual void write(DataWriter const& /*dt_writer*/) const;
		// 図形をデータライターに PDF として書き込む.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
	};

	//------------------------------
	// 曲線
	//------------------------------
	struct ShapeBezier : ShapePath {

		//------------------------------
		// SHAPE_bezier.cpp
		// ベジェ曲線
		//------------------------------

		// 矢じりの返しと先端の点を得る
		static bool bezi_get_pos_arrow(const D2D1_POINT_2F start, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_SIZE a_size, D2D1_POINT_2F arrow[3]) noexcept;
		// 図形を表示する.
		virtual void draw(void) noexcept final override;
		// 図形が点を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// 矩形に含まれるか判定する.
		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept final override;
		// 図形を作成する.
		ShapeBezier(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop);
		// 図形をデータリーダーから読み込む.
		ShapeBezier(DataReader const& dt_reader);
		// 図形をデータライターに PDF として書き込む.
		virtual size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer) final override;
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(const DataWriter& dt_writer) noexcept final override;
		// 図形をデータライターに書き込む.
		virtual void write(const DataWriter& dt_writer) const final override;
	};

	// 円弧
	struct ShapeArc : ShapePath {
		// ラディアンなら精度が不足しそうなので「度」で保持する.
		D2D1_SIZE_F m_radius{ 0.0f, 0.0f };	// 標準形にしたときの X 軸 Y 軸方向の半径
		float m_angle_rot = 0.0f;	// 円弧の傾き (度)
		float m_angle_start = 0.0f;	// 円弧の始点 (度)
		float m_angle_end = 0.0f;	// 円弧の終点 (度)
		D2D1_SWEEP_DIRECTION m_sweep_dir = D2D1_SWEEP_DIRECTION::D2D1_SWEEP_DIRECTION_CLOCKWISE;	// 円弧の描画方向
		D2D1_ARC_SIZE m_larg_flag = D2D1_ARC_SIZE::D2D1_ARC_SIZE_SMALL;	// 円弧の大きさ

		winrt::com_ptr<ID2D1PathGeometry> m_d2d_fill_geom{ nullptr };	// 塗りつぶしジオメトリ

		virtual bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept final override
		{
			D2D1_POINT_2F p[5];
			get_verts(p);
			return pt_in_rect(p[0], lt, rb) && pt_in_rect(p[1], lt, rb) && pt_in_rect(p[2], lt, rb) && pt_in_rect(p[3], lt, rb) && pt_in_rect(p[4], lt, rb);
		}

		virtual bool get_arc_dir(D2D1_SWEEP_DIRECTION& val) const noexcept final override
		{
			val = m_sweep_dir;
			return true;
		}
		// 円弧の始点の角度を得る.
		virtual bool get_arc_start(float& val) const noexcept final override
		{
			val = m_angle_start;
			return true;
		}
		// 円弧の終点の角度を得る.
		virtual bool get_arc_end(float& val) const noexcept final override
		{
			val = m_angle_end;
			return true;
		}
		// 円弧の傾きを得る.
		virtual bool get_arc_rot(float& val) const noexcept final override
		{
			val = m_angle_rot;
			return true;
		}

		//------------------------------
		// SHAPE_arc.cpp
		// 円弧
		//------------------------------

		// 指定した部位の点を得る.
		virtual void get_pos_loc(const uint32_t loc, D2D1_POINT_2F& val) const noexcept final override;
		// 値を, 指定した部位の点に格納する.
		virtual bool set_pos_loc(const D2D1_POINT_2F val, const uint32_t loc, const float snap_point, const bool keep_aspect) noexcept final override;
		// 値を始点に格納する. 他の部位の位置も動く.
		virtual bool set_pos_start(const D2D1_POINT_2F val) noexcept final override;
		// 値を円弧の始点の角度に格納する.
		virtual bool set_arc_start(const float val) noexcept final override;
		// 値を円弧の終点の角度に格納する.
		virtual bool set_arc_end(const float val) noexcept final override;
		// 値を円弧の傾きに格納する.
		virtual bool set_arc_rot(const float val) noexcept final override;
		// 値を円弧を描く方向に格納する.
		virtual bool set_arc_dir(const D2D1_SWEEP_DIRECTION val) noexcept final override;
		// 図形が点を含むか判定する.
		virtual uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// 円弧をベジェ曲線で近似する.
		void alter_bezier(D2D1_POINT_2F& start, D2D1_BEZIER_SEGMENT& b_seg) const noexcept;
		// 矢じりの返しと先端の位置を得る.
		static bool arc_get_pos_arrow(const D2D1_POINT_2F pos, const D2D1_POINT_2F ctr, const D2D1_SIZE_F rad, const double deg_start, const double deg_end, const double deg_rot, const ARROW_SIZE a_size, D2D1_POINT_2F arrow[]);
		// 図形を描く
		virtual void draw(void) noexcept final override;
		// 図形をデータライターに SVG として書き込む.
		virtual void export_svg(const DataWriter& dt_writer) noexcept final override;
		// 図形をデータライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, const DataWriter& dt_writer);
		// 図形を作成する.
		ShapeArc(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, const Shape* prop);
		// 図形をデータリーダーから読み込む.
		ShapeArc(const DataReader& dt_reader);
		// 図形をデータライターに書き込む.
		void write(const DataWriter& dt_writer) const final override;
		// 頂点を得る.
		size_t get_verts(D2D1_POINT_2F p[]) const noexcept final override;
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
		D2D1_SIZE_F m_text_pad{ TEXT_PAD_DEFVAL };	// 文字列の上下と左右の余白
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
				m_dwrite_text_layout = nullptr;
			}
		} // ~ShapeStroke

		//------------------------------
		// shape_text.cpp
		// 文字列図形
		//------------------------------

		// 文字列レイアウトを作成する.
		void create_text_layout(void) noexcept;
		// 枠を文字列に合わせる.
		bool fit_frame_to_text(const float g_len) noexcept;
		float get_frame_width(void) const noexcept { return fabsf(m_pos.x); }
		float get_frame_height(void) const noexcept { return fabsf(m_pos.y); }
		// 図形を表示する.
		virtual void draw(void) noexcept final override;
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
		bool get_text_pad(D2D1_SIZE_F& val) const noexcept final override;
		// 選択された文字範囲を得る.
		bool get_text_selected(DWRITE_TEXT_RANGE& val) const noexcept final override;
		int get_text_len(void) const noexcept
		{
			return wchar_len(m_text);
		}
		// 指定した点の
		int get_text_pos(const D2D1_POINT_2F p) const noexcept
		{
			// 図形の開始点を原点とする.
			const double px = p.x - (m_pos.x >= 0.0f ? m_start.x : m_start.x + m_pos.x);
			const double py = p.y - (m_pos.y >= 0.0f ? m_start.y : m_start.y + m_pos.y);
			if (px >= 0.0 && px < m_pos.x && py >= 0.0 && py <= m_pos.y) {
				const int t_cnt = m_dwrite_test_cnt;
				const int t_len = static_cast<int>(wchar_len(m_text));
				DWRITE_HIT_TEST_METRICS h;
				FLOAT x, y;
				for (int i = 0; i < t_cnt; i++) {
					const auto t_start = m_dwrite_test_metrics[i].textPosition;	// 行頭の文字の位置
					const auto t_end = t_start + m_dwrite_test_metrics[i].length;	// 行末の文字の位置
					const auto t_bot = m_dwrite_test_metrics[i].top + m_dwrite_test_metrics[i].height;
					// 点が i 行目の文字列に含まれているなら,
					if (py < t_bot) {
						// 行頭から行末の各文字について繰り返す.
						for (uint32_t j = t_start; j < t_end; j++) {
							m_dwrite_text_layout->HitTestTextPosition(j, false, &x, &y, &h);
							if (px <= x + h.width * 0.5) {
								return j;
							}
						}
						// 行末を超えているなら行末の文字の位置, 超えてないなら行頭の文字の位置を返す.
						if (px >= m_dwrite_test_metrics[i].left + m_dwrite_test_metrics[i].width) {
							return t_end;
						}
						return t_start;
					}
				}
				//for (int i = 0; i < t_len; i++) {
				//	m_dwrite_text_layout->HitTestTextPosition(i, false, &x, &y, &h);
				//	if (py <= y + h.height) {
				//		if (px <= x + h.width * 0.5) {
				//			return i;
				//		}
				//	}
				//}
				return t_len;
			}
			return -1;
		}
		// 図形が点を含むか判定する.
		uint32_t hit_test(const D2D1_POINT_2F t) const noexcept final override;
		// 矩形に含まれるか判定する.
		bool is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept final override;
		// 書体名が有効か判定し, 有効なら, 引数の書体名は破棄し, 有効な書体名の配列の要素と置き換える.
		static bool is_available_font(wchar_t*& font) noexcept;
		// 有効な書体名の配列を破棄する.
		static void release_available_fonts(void) noexcept;
		// 計量を破棄する.
		void relese_metrics(void) noexcept;
		// 有効な書体名の配列を設定する.
		static void set_available_fonts(void) noexcept;
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
		bool set_text_pad(const D2D1_SIZE_F val) noexcept final override;
		// 値を文字範囲に格納する.
		bool set_text_selected(const DWRITE_TEXT_RANGE val) noexcept final override;
		// 図形を作成する.
		ShapeText(const D2D1_POINT_2F start, const D2D1_POINT_2F pos, wchar_t* const text, const Shape* prop);
		// 図形をデータリーダーから読み込む.
		ShapeText(DataReader const& dt_reader);
		// 図形をデータライターに書き込む.
		void write(DataWriter const& dt_writer) const;
		// データライターに PDF として書き込む.
		size_t export_pdf(const D2D1_SIZE_F page_size, DataWriter const& dt_writer) final override;
		// データライターに SVG として書き込む.
		virtual void export_svg(DataWriter const& dt_writer) noexcept final override;
	};

	// 部位 (円形) を表示する.
	// 円形は, 図形の補助的な変形に関わる部位を意味する.
	inline void loc_draw_circle(
		const D2D1_POINT_2F p,	// 部位の点
		ID2D1RenderTarget* const target,	// 描画ターゲット
		ID2D1SolidColorBrush* const brush	// 色ブラシ
	) noexcept
	{
		const auto c_inner = Shape::m_loc_circle_inner;	// 内側の半径
		const auto c_outer = Shape::m_loc_circle_outer;	// 外側の半径
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

	// 部位 (ひし形) を表示する.
	// ひし形は, 図形の移動は行なうが, 変形には関わらない補助的な部位を意味する.
	inline void loc_draw_rhombus(
		const D2D1_POINT_2F p,	// 部位の点
		ID2D1RenderTarget* const target,	// 描画ターゲット
		ID2D1SolidColorBrush* const brush	// 色ブラシ
	) noexcept
	{
		// パスジオメトリを使わず, 斜めの太い線分で代用する.
		const double w_outer = 2.0 * Shape::m_loc_square_outer;
		const double w_inner = 2.0 * Shape::m_loc_square_inner;
		const double d_outer = Shape::m_loc_rhombus_outer;
		const double d_inner = Shape::m_loc_rhombus_inner;
		const D2D1_POINT_2F q_inner{
			static_cast<FLOAT>(p.x - d_inner), static_cast<FLOAT>(p.y - d_inner)
		};
		const D2D1_POINT_2F r_inner{
			static_cast<FLOAT>(p.x + d_inner), static_cast<FLOAT>(p.y + d_inner)
		};
		const D2D1_POINT_2F q_outer{
			static_cast<FLOAT>(p.x - d_outer), static_cast<FLOAT>(p.y - d_outer)
		};
		const D2D1_POINT_2F r_outer{
			static_cast<FLOAT>(p.x + d_outer), static_cast<FLOAT>(p.y + d_outer)
		};
		brush->SetColor(COLOR_WHITE);
		target->DrawLine(q_outer, r_outer, brush, static_cast<FLOAT>(w_outer));
		brush->SetColor(COLOR_BLACK);
		target->DrawLine(q_inner, r_inner, brush, static_cast<FLOAT>(w_inner));
	}

	// 部位 (正方形) を表示する.
	// 正方形は, 図形の変形に関わる部位を意味する.
	// p	部位の点
	// target	描画ターゲット
	// brush	色ブラシ
	inline void loc_draw_square(
		const D2D1_POINT_2F p,
		ID2D1RenderTarget* const target,
		ID2D1SolidColorBrush* const brush
	) noexcept
	{
		const double a_inner = Shape::m_loc_square_inner;
		const double a_outer = Shape::m_loc_square_outer;
		const D2D1_RECT_F s_inner{
			static_cast<FLOAT>(p.x - a_inner), static_cast<FLOAT>(p.y - a_inner),
			static_cast<FLOAT>(p.x + a_inner), static_cast<FLOAT>(p.y + a_inner)
		};
		const D2D1_RECT_F s_outer{
			static_cast<FLOAT>(p.x - a_outer), static_cast<FLOAT>(p.y - a_outer),
			static_cast<FLOAT>(p.x + a_outer), static_cast<FLOAT>(p.y + a_outer)
		};
		brush->SetColor(COLOR_WHITE);
		target->FillRectangle(s_outer, brush);
		brush->SetColor(COLOR_BLACK);
		target->FillRectangle(s_inner, brush);
	}

	// 部位が点を含むか判定する.
	// t	判定される点
	// loc	部位の点
	// len	部位の大きさ
	inline bool loc_hit_test(const D2D1_POINT_2F t, const D2D1_POINT_2F loc, const double len) noexcept
	{
		const double x = static_cast<double>(t.x) - static_cast<double>(loc.x);
		const double y = static_cast<double>(t.y) - static_cast<double>(loc.y);
		return -len * 0.5 <= x && x <= len * 0.5 && -len * 0.5 <= y && y <= len * 0.5;
	}

	// 実数 (0.0...1.0) の色成分を整数 (0...255) に変換する.
	inline uint32_t conv_color_comp(const double c) noexcept
	{
		return min(static_cast<uint32_t>(floor(c * 256.0)), 255);
	}

	// 矢じるしの大きさが同じか判定する.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept
	{
		return equal(a.m_width, b.m_width) && equal(a.m_length, b.m_length) && equal(a.m_offset, b.m_offset);
	}

	// 線の端点が同じか判定する.
	//inline bool equal(const CAP_STYLE& a, const CAP_STYLE& b) noexcept
	//{
	//	return a.m_start == b.m_start && a.m_end == b.m_end;
	//}

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
	inline bool equal(const GRID_EMPH a, const GRID_EMPH b) noexcept
	{
		return a.m_gauge_1 == b.m_gauge_1 && a.m_gauge_2 == b.m_gauge_2;
	}

	// 破線の配置が同じか判定する.
	inline bool equal(const DASH_PAT& a, const DASH_PAT& b) noexcept
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
		return conv_color_comp(a) == conv_color_comp(b);
	}

	// 矢じりの返しの点を求める.
	inline void get_pos_barbs(
		const D2D1_POINT_2F a,	// 矢軸ベクトル (ハズから先端方向への位置ベクトル)
		const double a_len,	// 矢軸ベクトルの長さ
		const double width,	// 矢じりの幅 (返しの間の長さ)
		const double len,	// 矢じりの長さ (先端から返しまでの長さ)
		D2D1_POINT_2F barb[]	// 矢じりの返しの位置
	) noexcept
	{
		if (a_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z{ 0.0f, 0.0f };
			barb[0] = Z;
			barb[1] = Z;
		}
		else {
			const double bw = width * 0.5;	// 矢じるしの幅の半分の大きさ
			const double sx = a.x * -len;	// 矢軸ベクトルを矢じるしの長さ分反転
			const double sy = a.x * bw;
			const double tx = a.y * -len;
			const double ty = a.y * bw;
			const double ax = 1.0 / a_len;
			barb[0].x = static_cast<FLOAT>((sx - ty) * ax);
			barb[0].y = static_cast<FLOAT>((tx + sy) * ax);
			barb[1].x = static_cast<FLOAT>((sx + ty) * ax);
			barb[1].y = static_cast<FLOAT>((tx - sy) * ax);
		}
	}

	// 色が不透明か判定する.
	// val	色
	// 戻り値	不透明ならば true, 透明ならば false.
	inline bool is_opaque(const D2D1_COLOR_F& val) noexcept
	{
		return val.a * 256.0 >= 1.0;
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
		const size_t len = ((s == nullptr || s[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(s)));
		if (len > 0) {
			wchar_t* t;
			wcscpy_s(t = new wchar_t[len + 1], len + 1, s);
			return t;
		}
		return nullptr;
	}

}