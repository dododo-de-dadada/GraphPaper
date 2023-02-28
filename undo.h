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
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	//------------------------------
	// 操作の識別子
	//------------------------------
	enum struct UNDO_ID : uint32_t {
		END = static_cast<uint32_t>(-1),	// 操作スタックの終端 (ファイル読み書きで使用)
		NIL = 0,	// 操作の区切り (ファイル読み書きで使用)
		ARROW_SIZE,	// 矢じるしの大きさの操作
		ARROW_STYLE,	// 矢じるしの形式の操作
		DASH_CAP,	// 破線の端の形式の操作
		DASH_PATT,	// 破線の配置の操作
		DASH_STYLE,	// 破線の形式の操作
		FILL_COLOR,	// 塗りつぶしの色の操作
		FONT_COLOR,	// 書体の色の操作
		FONT_FAMILY,	// 書体名の操作
		FONT_SIZE,	// 書体の大きさの操作
		FONT_STRETCH,	// 書体の幅の操作
		FONT_STYLE,	// 書体の字体の操作
		FONT_WEIGHT,	// 書体の太さの操作
		GRID_BASE,	// 方眼の基準の大さの操作
		GRID_COLOR,	// 方眼の色の操作
		GRID_EMPH,	// 方眼の形式の操作
		GRID_SHOW,	// 方眼の表示方法の操作
		GROUP,	// グループのリスト操作
		IMAGE,	// 画像の操作 (ファイル読み書きで使用)
		//IMAGE_ASPECT,	// 画像の縦横維持の操作
		IMAGE_OPAC,	// 画像の不透明度の操作
		JOIN_LIMIT,	// 線の結合の尖り制限の操作
		JOIN_STYLE,	// 破の結合の操作
		LIST,	// 図形を挿入または削除する操作
		ORDER,	// 図形の順番の入れ替え
		DEFORM,	// 図形の形 (部位の位置) の操作
		MOVE,	// 図形の移動の操作
		SELECT,	// 図形の選択を切り替え
		PAGE_COLOR,	// ページの色の操作
		PAGE_SIZE,	// ページの大きさの操作
		PAGE_PADD,	// ページの内余白の操作
		POLY_END,	// 多角形の終端の操作
		DEG_START,
		DEG_END,
		DEG_ROT,	// 傾き角度の操作
		STROKE_CAP,	// 端の形式の操作
		STROKE_COLOR,	// 線枠の色の操作
		STROKE_WIDTH,	// 線枠の太さの操作
		TEXT_PAR_ALIGN,	// 段落の整列の操作
		TEXT_ALIGN_T,	// 文字列の整列の操作
		TEXT_CONTENT,	// 文字列の操作
		TEXT_LINE_SP,	// 行間の操作
		TEXT_PADDING,	// 文字列の余白の操作
		TEXT_RANGE,	// 選択された文字範囲の操作
	};

	//------------------------------
	// 操作スタック
	//------------------------------
	using UNDO_STACK = std::list<struct Undo*>;	// 操作スタック

	//------------------------------
	// 操作から値の型を得るテンプレート
	//------------------------------
	template <UNDO_ID U> struct U_TYPE { using type = int; };
	template <> struct U_TYPE<UNDO_ID::ARROW_SIZE> { using type = ARROW_SIZE; };
	template <> struct U_TYPE<UNDO_ID::ARROW_STYLE> { using type = ARROW_STYLE; };
	template <> struct U_TYPE<UNDO_ID::DASH_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_ID::DASH_PATT> { using type = DASH_PATT; };
	template <> struct U_TYPE<UNDO_ID::DASH_STYLE> { using type = D2D1_DASH_STYLE; };
	template <> struct U_TYPE<UNDO_ID::FILL_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_ID::FONT_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_ID::FONT_FAMILY> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_ID::FONT_SIZE> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::FONT_STRETCH> { using type = DWRITE_FONT_STRETCH; };
	template <> struct U_TYPE<UNDO_ID::FONT_STYLE> { using type = DWRITE_FONT_STYLE; };
	template <> struct U_TYPE<UNDO_ID::FONT_WEIGHT> { using type = DWRITE_FONT_WEIGHT; };
	template <> struct U_TYPE<UNDO_ID::GRID_BASE> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::GRID_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_ID::GRID_EMPH> { using type = GRID_EMPH; };
	template <> struct U_TYPE<UNDO_ID::GRID_SHOW> { using type = GRID_SHOW; };
	//template <> struct U_TYPE<UNDO_ID::IMAGE_ASPECT> { using type = bool; };
	template <> struct U_TYPE<UNDO_ID::IMAGE_OPAC> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::JOIN_LIMIT> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::JOIN_STYLE> { using type = D2D1_LINE_JOIN; };
	template <> struct U_TYPE<UNDO_ID::MOVE> { using type = D2D1_POINT_2F; };
	template <> struct U_TYPE<UNDO_ID::PAGE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_ID::PAGE_SIZE> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_ID::PAGE_PADD> { using type = D2D1_RECT_F; };
	template <> struct U_TYPE<UNDO_ID::POLY_END> { using type = bool; };
	template <> struct U_TYPE<UNDO_ID::DEG_START> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::DEG_END> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::DEG_ROT> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::STROKE_CAP> { using type = CAP_STYLE; };
	template <> struct U_TYPE<UNDO_ID::STROKE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_ID::STROKE_WIDTH> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::TEXT_PAR_ALIGN> { using type = DWRITE_PARAGRAPH_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_ID::TEXT_ALIGN_T> { using type = DWRITE_TEXT_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_ID::TEXT_CONTENT> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_ID::TEXT_LINE_SP> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::TEXT_PADDING> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_ID::TEXT_RANGE> { using type = DWRITE_TEXT_RANGE; };

	//------------------------------
	// 操作のひな型
	//------------------------------
	struct Undo {
		Shape* m_shape;	// 操作する図形

		// 操作を破棄する.
		virtual ~Undo() {}
		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept { return false; }
		// 操作を実行する.
		virtual void exec(void) {}
		// 操作をデータライターから読み込む.
		//virtual void read(DataReader const& /*dt_reader*/) {}
		// 図形を参照しているか判定する.
		virtual bool refer_to(const Shape* s) const noexcept { return m_shape == s; };
		// 参照する図形リストと用紙図形を格納する.
		static void set(SHAPE_LIST* slist, ShapePage* page) noexcept;
		// 操作する図形を得る.
		Shape* shape(void) const noexcept { return m_shape; }
		// 操作を作成する.
		Undo(Shape* s) : m_shape(s) {}
		// データライターに書き込む.
		virtual void write(DataWriter const& /*dt_writer*/) {}
	};

	//------------------------------
	// 形の操作
	//------------------------------
	struct UndoForm : Undo {
		uint32_t m_anc;	// 操作される部位
		D2D1_POINT_2F m_start;	// 変形前の部位の位置

		// 操作を実行すると値が変わるか判定する.
		bool changed(void) const noexcept;
		// 操作を実行する.
		void exec(void);
		// データリーダーから操作を読み込む.
		UndoForm(DataReader const& dt_reader);
		// 図形の形を保存する.
		UndoForm(Shape* const s, const uint32_t anc);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形の順番を入れ替える操作
	//------------------------------
	struct UndoOrder : Undo {
		Shape* m_dst_shape;	// 入れ替え先の図形

		// 操作を実行すると値が変わるか判定する.
		bool changed(void) const noexcept { return m_shape != m_dst_shape; }
		// 入れ替える先の図形を得る.
		Shape* const dest(void) const noexcept { return m_dst_shape; }
		// 操作を実行する.
		void exec(void);
		// 図形を参照しているか判定する.
		bool refer_to(const Shape* s) const noexcept final override { return Undo::refer_to(s) || m_dst_shape == s; };
		// 図形の順番を入れ替える.
		UndoOrder(Shape* const s, Shape* const t);
		// データリーダーから操作を読み込む.
		UndoOrder(DataReader const& dt_reader);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形の値を保存して変更する操作.
	//------------------------------
	template <UNDO_ID U>
	struct UndoValue : Undo, U_TYPE<U> {
		U_TYPE<U>::type m_value;	// // 変更される前の値

		~UndoValue() {};
		// 操作を実行すると値が変わるか判定する.
		bool changed(void) const noexcept;
		// 操作を実行する.
		void exec(void);
		// 値を図形から得る.
		static bool GET(const Shape* s, U_TYPE<U>::type& val) noexcept;
		// 値を図形に格納する.
		static void SET(Shape* const s, const U_TYPE<U>::type& val);
		// データリーダーから操作を読み込む.
		UndoValue(DataReader const& dt_reader);
		// 図形の値を保存する.
		UndoValue(Shape* s);
		// 図形の値を保存して変更する.
		UndoValue(Shape* s, const U_TYPE<U>::type& val);
		// データライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 画像の位置と大きさの操作
	//------------------------------
	struct UndoImage : Undo {
		D2D1_POINT_2F m_start;	// 位置
		D2D1_SIZE_F m_view;	// 表示されている画面上の寸法
		D2D1_RECT_F m_clip;	// 表示されている画像上の矩形
		D2D1_SIZE_F m_ratio;	// 先寸法と元矩形の縦横比
		float m_opac;	// 不透明度

		// 操作を実行すると値が変わるか判定する.
		bool changed(void) const noexcept;
		// 操作を実行する.
		void exec(void);
		// データリーダーから操作を読み込む.
		UndoImage(DataReader const& dt_reader);
		// 図形の部位を保存する.
		UndoImage(ShapeImage* const s);
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
		virtual bool refer_to(const Shape* s) const noexcept override { return Undo::refer_to(s) || m_shape_at == s; };
		// 操作される位置にあった図形を得る.
		Shape* const shape_at(void) const noexcept { return m_shape_at; }
		// データリーダーから操作を読み込む.
		UndoList(DataReader const& dt_reader);
		// 図形をリストから取り除く.
		UndoList(Shape* const s, const bool dont_exec = false);
		// 図形をリストに挿入する.
		UndoList(Shape* const s, Shape* const p, const bool dont_exec = false);
		// 操作をデータライターに書き込む.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// 図形をグループに追加または削除する操作.
	//------------------------------
	struct UndoGroup : UndoList {
		ShapeGroup* m_shape_group;	// 操作するグループ

		// 操作を実行する.
		void exec(void);
		// 図形を参照しているか判定する.
		bool refer_to(const Shape* s) const noexcept final override { return UndoList::refer_to(s) || m_shape_group == s; };
		// データリーダーから操作を読み込む.
		UndoGroup(DataReader const& dt_reader);
		// 図形をグループから削除する.
		UndoGroup(ShapeGroup* const g, Shape* const s);
		// 図形をグループに追加する.
		UndoGroup(ShapeGroup* const g, Shape* const s, Shape* const s_pos);
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
	template <> UndoValue<UNDO_ID::TEXT_CONTENT>::~UndoValue() 
	{
		if (m_value != nullptr) {
			delete[] m_value;
			m_value = nullptr;
		}
	}
}