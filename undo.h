#pragma once
//------------------------------
// undo.h
//
// undo.cpp
//
// 元に戻す / やり直す操作
// 図形の追加や削除, 変更は, 「操作」を通じて行われる.
//------------------------------
#include <list>
#include <winrt/Windows.Storage.Streams.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;

	//------------------------------
	// 操作
	//------------------------------
	enum struct UNDO_OP {
		END = -1,	// 操作スタックの終端 (ファイルの読み書きで使用)
		NULLPTR = 0,	// 操作の区切り (ファイルの読み書きで使用)
		ARRANGE,	// 図形の順番の入れ替え
		ARROW_SIZE,	// 矢じりの大きさを操作
		ARROW_STYLE,	// 矢じりの形式を操作
		FILL_COLOR,	// 塗りつぶしの色を操作
		ANCH_POS,	// 図形の部位の位置を操作
		FONT_COLOR,	// 書体の色を操作
		FONT_FAMILY,	// 書体名を操作
		FONT_SIZE,	// 書体の大きさを操作
		FONT_STYLE,	// 書体の字体を操作
		FONT_STRETCH,	// 書体の伸縮を操作
		FONT_WEIGHT,	// 書体の太さを操作
		GRID_BASE,	// 方眼の基準の大さを操作
		GRID_OPAC,	// 方眼の色の濃さを操作
		GRID_SHOW,	// 方眼の表示方法を操作
		GROUP,	// 図形をグループに挿入または削除する操作
		LIST,	// 図形をリストに挿入または削除する操作
		PAGE_COLOR,	// ページの色を操作
		PAGE_SIZE,	// ページの寸法を操作
		SELECT,	// 図形の選択を切り替え
		START_POS,	// 図形の開始位置を操作
		STROKE_COLOR,	// 線枠の色を操作
		STROKE_PATTERN,	// 破線の配置を操作
		STROKE_STYLE,	// 線枠の形式を操作
		STROKE_WIDTH,	// 線枠の太さを操作
		TEXT_CONTENT,	// 文字列を操作
		TEXT_ALIGN_P,	// 段落の整列を操作
		TEXT_ALIGN_T,	// 文字列の整列を操作
		TEXT_LINE,	// 行間を操作
		TEXT_MARGIN,	// 文字列の余白を操作
		TEXT_RANGE,	// 文字範囲を操作
	};

	//------------------------------
	// 操作スタックを処理する関数.
	//------------------------------
	typedef std::list<struct Undo*> U_STACK_T;	// 操作スタック

	//------------------------------
	// 操作のひな型
	//------------------------------
	struct Undo {
		// 参照する図形リスト
		static S_LIST_T* s_shape_list;
		// 参照するページレイアウト
		static ShapeLayout* s_shape_layout;
		// 操作する図形
		Shape* m_shape;

		// 操作を破棄する.
		virtual ~Undo() {}
		// 操作を実行すると値が変わるか調べる.
		virtual bool changed(void) const noexcept { return false; }
		// 操作を実行する.
		virtual void exec(void) {}
		// 操作をデータライターから読み込む.
		virtual void read(DataReader const& /*dt_reader*/) {}
		// 図形を参照しているか調べる.
		virtual bool refer_to(const Shape* s) const noexcept { return m_shape == s; };
		// 参照する図形リストとページ図形を格納する.
		static void set(S_LIST_T* s_list, ShapeLayout* s_page) noexcept;
		// 操作する図形を得る.
		Shape* shape(void) const noexcept { return m_shape; }
		// 操作を作成する.
		Undo(Shape* s) : m_shape(s) {}
		// データライターに書き込む.
		virtual void write(DataWriter const& /*dt_writer*/) {}
	};

	//------------------------------
	// 図形の順番を入れ替える操作
	//------------------------------
	struct UndoArrange2 : Undo {
		// 入れ替え先の図形
		Shape* m_dst_shape;

		// 操作を実行すると値が変わるか調べる.
		bool changed(void) const noexcept { return m_shape != m_dst_shape; }
		// 入れ替える先の図形を得る.
		Shape* dest(void) const noexcept { return m_dst_shape; }
		// 操作を実行する.
		void exec(void);
		// 図形を参照しているか調べる.
		bool refer_to(const Shape* s) const noexcept { return Undo::refer_to(s) || m_dst_shape == s; };
		// 図形の順番を入れ替える.
		UndoArrange2(Shape* s, Shape* t);
		// 操作をデータリーダーから読み込む.
		UndoArrange2(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形をリストに挿入または削除する操作.
	//------------------------------
	struct UndoList : Undo {
		// 挿入フラグ
		bool m_insert;
		// 操作する位置にある図形
		Shape* m_item_pos;

		// 操作を実行すると値が変わるか調べる.
		bool changed(void) const noexcept { return true; }
		// 操作を実行する.
		void exec(void);
		// 操作が挿入か調べる.
		bool is_insert(void) const noexcept { return m_insert; }
		// 図形を参照しているか調べる.
		bool refer_to(const Shape* s) const noexcept { return Undo::refer_to(s) || m_item_pos == s; };
		// 操作する位置にある図形を得る.
		Shape* item_pos(void) const noexcept { return m_item_pos; }
		// 操作をデータリーダーから読み込む.
		UndoList(DataReader const& dt_reader);
		// 図形をリストから取り除く.
		UndoList(Shape* s, const bool dont_exec = false);
		// 図形をリストに挿入する.
		UndoList(Shape* s, Shape* s_pos, const bool dont_exec = false);
		// 操作をデータライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形をグループに追加または削除する操作.
	//------------------------------
	struct UndoListG : UndoList {
		// 操作するグループ
		ShapeGroup* m_shape_group;

		// 操作を実行する.
		void exec(void);
		// 操作するグループの位置を得る.
		//Shape* group_pos(void) { return m_shape_group; }
		// 図形を参照しているか調べる.
		bool refer_to(const Shape* s) const noexcept { return UndoList::refer_to(s) || m_shape_group == s; };
		// 操作をデータリーダーから読み込む.
		UndoListG(DataReader const& dt_reader);
		// 図形をグループから削除する.
		UndoListG(ShapeGroup* g, Shape* s);
		// 図形をグループに追加する.
		UndoListG(ShapeGroup* g, Shape* s, Shape* s_pos);
		// 操作をデータライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形の選択を反転する操作
	//------------------------------
	struct UndoSelect : Undo {
		// 操作を実行すると値が変わるか調べる.
		bool changed(void) const noexcept { return true; }
		// 操作を実行する.
		void exec(void);
		// 操作をデータリーダーから読み込む.
		UndoSelect(DataReader const& dt_reader);
		// 図形の選択を反転する.
		UndoSelect(Shape* s);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形を変形する操作
	//------------------------------
	struct UndoForm : Undo {
		// 変更される図形の部位
		ANCH_WHICH m_anchor;
		// 図形の部位の位置
		D2D1_POINT_2F m_anchor_pos;

		// 操作を実行すると値が変わるか調べる.
		bool changed(void) const noexcept;
		// 操作を実行する.
		void exec(void);
		// 操作をデータリーダーから読み込む.
		UndoForm(DataReader const& dt_reader);
		// 指定した部位の図形の位置を保存する.
		UndoForm(Shape* s, const ANCH_WHICH a);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 操作の種類から値の型を得るテンプレート
	//------------------------------
	template <UNDO_OP U>
	struct U_TYPE {
		using type = int;
	};
	template <> struct U_TYPE<UNDO_OP::ARROW_SIZE> { using type = ARROW_SIZE; };
	template <> struct U_TYPE<UNDO_OP::ARROW_STYLE> { using type = ARROW_STYLE; };
	template <> struct U_TYPE<UNDO_OP::FILL_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::FONT_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::FONT_FAMILY> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_OP::FONT_SIZE> { using type = double; };
	template <> struct U_TYPE<UNDO_OP::FONT_STRETCH> { using type = DWRITE_FONT_STRETCH; };
	template <> struct U_TYPE<UNDO_OP::FONT_STYLE> { using type = DWRITE_FONT_STYLE; };
	template <> struct U_TYPE<UNDO_OP::FONT_WEIGHT> { using type = DWRITE_FONT_WEIGHT; };
	template <> struct U_TYPE<UNDO_OP::GRID_BASE> { using type = double; };
	template <> struct U_TYPE<UNDO_OP::GRID_OPAC> { using type = double; };
	template <> struct U_TYPE<UNDO_OP::GRID_SHOW> { using type = GRID_SHOW; };
	template <> struct U_TYPE<UNDO_OP::TEXT_LINE> { using type = double; };
	template <> struct U_TYPE<UNDO_OP::PAGE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::PAGE_SIZE> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_OP::START_POS> { using type = D2D1_POINT_2F; };
	template <> struct U_TYPE<UNDO_OP::STROKE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::STROKE_PATTERN> { using type = STROKE_PATTERN; };
	template <> struct U_TYPE<UNDO_OP::STROKE_STYLE> { using type = D2D1_DASH_STYLE; };
	template <> struct U_TYPE<UNDO_OP::STROKE_WIDTH> { using type = double; };
	template <> struct U_TYPE<UNDO_OP::TEXT_CONTENT> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_OP::TEXT_ALIGN_P> { using type = DWRITE_PARAGRAPH_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_OP::TEXT_ALIGN_T> { using type = DWRITE_TEXT_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_OP::TEXT_MARGIN> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_OP::TEXT_RANGE> { using type = DWRITE_TEXT_RANGE; };

	//------------------------------
	// 図形の値を保存して変更する操作.
	//------------------------------
	template <UNDO_OP U>
	struct UndoSet : Undo, U_TYPE<U> {
		// 変更される前の値
		U_TYPE<U>::type m_value;

		// 操作を実行すると値が変わるか調べる.
		bool changed(void) const noexcept;
		// 操作を実行する.
		void exec(void);
		// 値を図形から得る.
		static bool GET(Shape* s, U_TYPE<U>::type& value) noexcept;
		// 値を図形に格納する.
		static void SET(Shape* s, const U_TYPE<U>::type& value);
		// データリーダーから操作を読み込んで作成する.
		UndoSet(DataReader const& dt_reader);
		// 図形の値を保存する.
		UndoSet(Shape* s);
		// 図形の値を保存して変更する.
		UndoSet(Shape* s, const U_TYPE<U>::type& value);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

}