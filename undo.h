#pragma once
//------------------------------
// undo.h
//
// 元に戻す / やり直し操作
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
	// 操作スタック
	//------------------------------
	using UNDO_STACK = std::list<struct Undo*>;	// 操作スタック

	//------------------------------
	// 操作
	//------------------------------
	enum struct UNDO_OP {
		END = -1,	// 操作スタックの終端 (ファイル読み書きで使用)
		NULLPTR = 0,	// 操作の区切り (ファイル読み書きで使用)
		ARRANGE,	// 図形の順番の入れ替え
		ARROW_SIZE,	// 矢じるしの大きさの操作
		ARROW_STYLE,	// 矢じるしの形式の操作
		BITMAP,	// 画像の操作 (ファイル読み書きで使用)
		//BM_KEEP,	// 画像の縦横維持の操作
		BM_OPAC,	// 画像の不透明度の操作
		CAP_STYLE,	// 端の形式の操作
		DASH_CAP,	// 破線の端の形式の操作
		DASH_PATT,	// 破線の様式の操作
		DASH_STYLE,	// 破線の形式の操作
		FILL_COLOR,	// 塗りつぶしの色の操作
		FONT_COLOR,	// 書体の色の操作
		FONT_FAMILY,	// 書体名の操作
		FONT_SIZE,	// 書体の大きさの操作
		FONT_STYLE,	// 書体の字体の操作
		FONT_STRETCH,	// 書体の伸縮の操作
		FONT_WEIGHT,	// 書体の太さの操作
		GRID_BASE,	// 方眼の基準の大さの操作
		GRID_COLOR,	// 方眼の色の操作
		GRID_EMPH,	// 方眼の形式の操作
		GRID_SHOW,	// 方眼の表示方法の操作
		GROUP,	// 図形をグループに挿入または削除する操作
		JOIN_LIMIT,	// 線のマイター制限の操作
		JOIN_STYLE,	// 破のつなぎの操作
		LIST,	// 図形をリストに挿入または削除する操作
		POS_ANCH,	// 図形の部位の位置の操作
		POS_START,	// 図形の開始位置の操作
		SELECT,	// 図形の選択を切り替え
		SHEET_COLOR,	// 用紙の色の操作
		SHEET_SIZE,	// 用紙の寸法の操作
		STROKE_COLOR,	// 線枠の色の操作
		STROKE_WIDTH,	// 線枠の太さの操作
		TEXT_CONTENT,	// 文字列の操作
		TEXT_ALIGN_P,	// 段落の整列の操作
		TEXT_ALIGN_T,	// 文字列の整列の操作
		TEXT_LINE_SP,	// 行間の操作
		TEXT_MARGIN,	// 文字列の余白の操作
		TEXT_RANGE,	// 文字範囲の操作
	};

	//------------------------------
	// 操作から値の型を得るテンプレート
	//------------------------------
	template <UNDO_OP U> struct U_TYPE { using type = int; };
	template <> struct U_TYPE<UNDO_OP::ARROW_SIZE> { using type = ARROW_SIZE; };
	template <> struct U_TYPE<UNDO_OP::ARROW_STYLE> { using type = ARROW_STYLE; };
	//template <> struct U_TYPE<UNDO_OP::BM_KEEP> { using type = bool; };
	template <> struct U_TYPE<UNDO_OP::BM_OPAC> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::CAP_STYLE> { using type = CAP_STYLE; };
	template <> struct U_TYPE<UNDO_OP::DASH_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_OP::DASH_PATT> { using type = DASH_PATT; };
	template <> struct U_TYPE<UNDO_OP::DASH_STYLE> { using type = D2D1_DASH_STYLE; };
	template <> struct U_TYPE<UNDO_OP::FILL_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::FONT_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::FONT_FAMILY> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_OP::FONT_SIZE> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::FONT_STRETCH> { using type = DWRITE_FONT_STRETCH; };
	template <> struct U_TYPE<UNDO_OP::FONT_STYLE> { using type = DWRITE_FONT_STYLE; };
	template <> struct U_TYPE<UNDO_OP::FONT_WEIGHT> { using type = DWRITE_FONT_WEIGHT; };
	template <> struct U_TYPE<UNDO_OP::GRID_BASE> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::GRID_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::GRID_EMPH> { using type = GRID_EMPH; };
	template <> struct U_TYPE<UNDO_OP::GRID_SHOW> { using type = GRID_SHOW; };
	template <> struct U_TYPE<UNDO_OP::JOIN_LIMIT> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::JOIN_STYLE> { using type = D2D1_LINE_JOIN; };
	template <> struct U_TYPE<UNDO_OP::POS_START> { using type = D2D1_POINT_2F; };
	template <> struct U_TYPE<UNDO_OP::SHEET_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::SHEET_SIZE> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_OP::STROKE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::STROKE_WIDTH> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::TEXT_CONTENT> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_OP::TEXT_ALIGN_P> { using type = DWRITE_PARAGRAPH_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_OP::TEXT_ALIGN_T> { using type = DWRITE_TEXT_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_OP::TEXT_LINE_SP> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::TEXT_MARGIN> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_OP::TEXT_RANGE> { using type = DWRITE_TEXT_RANGE; };

	//------------------------------
	// 操作のひな型
	//------------------------------
	struct Undo {
		// 参照する図形リスト
		static SHAPE_LIST* s_shape_list;
		// 参照する図形シート
		static ShapeSheet* s_shape_sheet;
		// 操作する図形
		Shape* m_shape;

		// 操作を破棄する.
		virtual ~Undo() {}
		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept { return false; }
		// 操作を実行する.
		virtual void exec(void) {}
		// 操作をデータライターから読み込む.
		virtual void read(DataReader const& /*dt_reader*/) {}
		// 図形を参照しているか判定する.
		virtual bool refer_to(const Shape* s) const noexcept { return m_shape == s; };
		// 参照する図形リストと用紙図形を格納する.
		static void set(SHAPE_LIST* slist, ShapeSheet* s_sheet) noexcept;
		// 操作する図形を得る.
		Shape* shape(void) const noexcept { return m_shape; }
		// 操作を作成する.
		Undo(Shape* s) : m_shape(s) {}
		// データライターに書き込む.
		virtual void write(DataWriter const& /*dt_writer*/) {}
	};

	//------------------------------
	// 図形の部位の操作
	//------------------------------
	struct UndoAnchor : Undo {
		uint32_t m_anch;	// 操作される図形の部位
		D2D1_POINT_2F m_pos;	// 変更前の, 図形の部位の位置

		// 操作を実行すると値が変わるか判定する.
		bool changed(void) const noexcept;
		// 操作を実行する.
		void exec(void);
		// データリーダーから操作を読み込む.
		UndoAnchor(DataReader const& dt_reader);
		// 図形の部位を保存する.
		UndoAnchor(Shape* const s, const uint32_t anch);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形の順番を入れ替える操作
	//------------------------------
	struct UndoArrange2 : Undo {
		Shape* m_dst_shape;	// 入れ替え先の図形

		// 操作を実行すると値が変わるか判定する.
		bool changed(void) const noexcept { return m_shape != m_dst_shape; }
		// 入れ替える先の図形を得る.
		Shape* const dest(void) const noexcept { return m_dst_shape; }
		// 操作を実行する.
		void exec(void);
		// 図形を参照しているか判定する.
		bool refer_to(const Shape* s) const noexcept { return Undo::refer_to(s) || m_dst_shape == s; };
		// 図形の順番を入れ替える.
		UndoArrange2(Shape* const s, Shape* const t);
		// データリーダーから操作を読み込む.
		UndoArrange2(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形の値を保存して変更する操作.
	//------------------------------
	template <UNDO_OP U>
	struct UndoAttr : Undo, U_TYPE<U> {
		U_TYPE<U>::type m_value;	// // 変更される前の値

		~UndoAttr() {};
		// 操作を実行すると値が変わるか判定する.
		bool changed(void) const noexcept;
		// 操作を実行する.
		void exec(void);
		// 値を図形から得る.
		static bool GET(const Shape* s, U_TYPE<U>::type& value) noexcept;
		// 値を図形に格納する.
		static void SET(Shape* const s, const U_TYPE<U>::type& value);
		// データリーダーから操作を読み込む.
		UndoAttr(DataReader const& dt_reader);
		// 図形の値を保存する.
		UndoAttr(Shape* s);
		// 図形の値を保存して変更する.
		UndoAttr(Shape* s, const U_TYPE<U>::type& value);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 画像の操作
	//------------------------------
	struct UndoBitmap : Undo {
		D2D1_POINT_2F m_pos;	// 位置
		D2D1_SIZE_F m_view;	// 先寸法
		D2D1_RECT_F m_rect;	// 元矩形
		D2D1_SIZE_F m_ratio;	// 先寸法と元矩形の縦横比

		// 操作を実行すると値が変わるか判定する.
		bool changed(void) const noexcept;
		// 操作を実行する.
		void exec(void);
		// データリーダーから操作を読み込む.
		UndoBitmap(DataReader const& dt_reader);
		// 図形の部位を保存する.
		UndoBitmap(ShapeImage* const s);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形をリストに挿入または削除する操作.
	//------------------------------
	struct UndoList : Undo {
		bool m_insert;	// 挿入フラグ
		Shape* m_shape_at;	// 変更前に, 操作される位置にあった図形

		// 操作を実行すると値が変わるか判定する.
		bool changed(void) const noexcept { return true; }
		// 操作を実行する.
		void exec(void);
		// 操作が挿入か判定する.
		bool is_insert(void) const noexcept { return m_insert; }
		// 図形を参照しているか判定する.
		bool refer_to(const Shape* s) const noexcept { return Undo::refer_to(s) || m_shape_at == s; };
		// 操作される位置にあった図形を得る.
		Shape* const shape_at(void) const noexcept { return m_shape_at; }
		// データリーダーから操作を読み込む.
		UndoList(DataReader const& dt_reader);
		// 図形をリストから取り除く.
		UndoList(Shape* const s, const bool dont_exec = false);
		// 図形をリストに挿入する.
		UndoList(Shape* const s, Shape* const s_pos, const bool dont_exec = false);
		// 操作をデータライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形をグループに追加または削除する操作.
	//------------------------------
	struct UndoListGroup : UndoList {
		// 操作するグループ
		ShapeGroup* m_shape_group;

		// 操作を実行する.
		void exec(void);
		// 図形を参照しているか判定する.
		bool refer_to(const Shape* s) const noexcept { return UndoList::refer_to(s) || m_shape_group == s; };
		// データリーダーから操作を読み込む.
		UndoListGroup(DataReader const& dt_reader);
		// 図形をグループから削除する.
		UndoListGroup(ShapeGroup* const g, Shape* const s);
		// 図形をグループに追加する.
		UndoListGroup(ShapeGroup* const g, Shape* const s, Shape* const s_pos);
		// 操作をデータライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形の選択を反転する操作
	//------------------------------
	struct UndoSelect : Undo {
		// 操作を実行すると値が変わるか判定する.
		bool changed(void) const noexcept { return true; }
		// 操作を実行する.
		void exec(void);
		// データリーダーから操作を読み込む.
		UndoSelect(DataReader const& dt_reader);
		// 図形の選択を反転する.
		UndoSelect(Shape* const s);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	// 文字列の操作を破棄する.
	template <> UndoAttr<UNDO_OP::TEXT_CONTENT>::~UndoAttr() 
	{
		if (m_value != nullptr) {
			delete[] m_value;
			m_value = nullptr;
		}
	}
}