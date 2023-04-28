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
	// 操作の種類
	enum struct UNDO_T : uint32_t {
		END = static_cast<uint32_t>(-1),	// 操作スタックの終端 (ファイル読み書きで使用)
		NIL = 0,	// 操作の区切り (ファイル読み書きで使用)
		ARC_DIR,	// 円弧の方向の操作
		ARC_END,	// 円弧の終点の操作
		ARC_ROT,	// 円弧の傾きの操作
		ARC_START,	// 円弧の始点の操作
		ARROW_CAP,	// 矢じるしの返しの形式の操作
		ARROW_JOIN,	// 矢じるしの先端の形式の操作
		ARROW_SIZE,	// 矢じるしの大きさの操作
		ARROW_STYLE,	// 矢じるしの形式の操作
		//DASH_CAP,	// 破線の端の形式の操作
		DASH_PAT,	// 破線の配置の操作
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
		PAGE_PAD,	// ページの内余白の操作
		POLY_END,	// 多角形の端の操作
		STROKE_CAP,	// 端の形式の操作
		STROKE_COLOR,	// 線枠の色の操作
		STROKE_WIDTH,	// 線枠の太さの操作
		TEXT_ALIGN_P,	// 段落の整列の操作
		TEXT_ALIGN_T,	// 文字列の整列の操作
		TEXT_CONTENT,	// 文字列の操作
		TEXT_DEL,	// 文字列の削除の操作
		TEXT_INS,	// 文字列の挿入の操作
		TEXT_LINE_SP,	// 行間の操作
		TEXT_PAD,	// 文字列の余白の操作
		TEXT_RANGE,	// 文字列選択の範囲の操作
	};

	// 操作スタック
	using UNDO_STACK = std::list<struct Undo*>;	// 操作スタック

	// 操作から値の型を得るテンプレート
	template <UNDO_T U> struct U_TYPE { using type = int; };
	template <> struct U_TYPE<UNDO_T::ARC_START> { using type = float; };
	template <> struct U_TYPE<UNDO_T::ARC_DIR> { using type = D2D1_SWEEP_DIRECTION; };
	template <> struct U_TYPE<UNDO_T::ARC_END> { using type = float; };
	template <> struct U_TYPE<UNDO_T::ARC_ROT> { using type = float; };
	template <> struct U_TYPE<UNDO_T::ARROW_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_T::ARROW_JOIN> { using type = D2D1_LINE_JOIN; };
	template <> struct U_TYPE<UNDO_T::ARROW_SIZE> { using type = ARROW_SIZE; };
	template <> struct U_TYPE<UNDO_T::ARROW_STYLE> { using type = ARROW_STYLE; };
	//template <> struct U_TYPE<UNDO_T::DASH_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_T::DASH_PAT> { using type = DASH_PAT; };
	template <> struct U_TYPE<UNDO_T::DASH_STYLE> { using type = D2D1_DASH_STYLE; };
	template <> struct U_TYPE<UNDO_T::FILL_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::FONT_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::FONT_FAMILY> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_T::FONT_SIZE> { using type = float; };
	template <> struct U_TYPE<UNDO_T::FONT_STRETCH> { using type = DWRITE_FONT_STRETCH; };
	template <> struct U_TYPE<UNDO_T::FONT_STYLE> { using type = DWRITE_FONT_STYLE; };
	template <> struct U_TYPE<UNDO_T::FONT_WEIGHT> { using type = DWRITE_FONT_WEIGHT; };
	template <> struct U_TYPE<UNDO_T::GRID_BASE> { using type = float; };
	template <> struct U_TYPE<UNDO_T::GRID_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::GRID_EMPH> { using type = GRID_EMPH; };
	template <> struct U_TYPE<UNDO_T::GRID_SHOW> { using type = GRID_SHOW; };
	template <> struct U_TYPE<UNDO_T::IMAGE_OPAC> { using type = float; };
	template <> struct U_TYPE<UNDO_T::JOIN_LIMIT> { using type = float; };
	template <> struct U_TYPE<UNDO_T::JOIN_STYLE> { using type = D2D1_LINE_JOIN; };
	template <> struct U_TYPE<UNDO_T::MOVE> { using type = D2D1_POINT_2F; };
	template <> struct U_TYPE<UNDO_T::PAGE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::PAGE_SIZE> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_T::PAGE_PAD> { using type = D2D1_RECT_F; };
	template <> struct U_TYPE<UNDO_T::POLY_END> { using type = D2D1_FIGURE_END; };
	template <> struct U_TYPE<UNDO_T::STROKE_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_T::STROKE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::STROKE_WIDTH> { using type = float; };
	template <> struct U_TYPE<UNDO_T::TEXT_ALIGN_P> { using type = DWRITE_PARAGRAPH_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_T::TEXT_ALIGN_T> { using type = DWRITE_TEXT_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_T::TEXT_CONTENT> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_T::TEXT_LINE_SP> { using type = float; };
	template <> struct U_TYPE<UNDO_T::TEXT_PAD> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_T::TEXT_RANGE> { using type = DWRITE_TEXT_RANGE; };

	//------------------------------
	// 操作のひな型
	//------------------------------
	struct Undo {
		static SHAPE_LIST* undo_slist;	// 参照する図形リスト
		static ShapePage* undo_page;	// 参照するページ

		Shape* m_shape;	// 操作する図形

		// 操作を破棄する.
		virtual ~Undo() {}
		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept { return false; }
		// 操作を実行する.
		virtual void exec(void) = 0;
		// 図形を参照しているか判定する.
		virtual bool refer_to(const Shape* s) const noexcept { return m_shape == s; };
		// 操作が参照するための図形リストと表示図形を格納する.
		static void begin(SHAPE_LIST* slist, ShapePage* page) noexcept
		{
			undo_slist = slist;
			undo_page = page;
		}
		// 操作する図形を得る.
		Shape* shape(void) const noexcept { return m_shape; }
		// 操作を作成する.
		Undo(Shape* s) : m_shape(s) {}
		// データライターに書き込む.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
	};

	// 図形を変形する操作
	struct UndoDeform : Undo {
		uint32_t m_loc;	// 変形される部位
		D2D1_POINT_2F m_p;	// 変形前の部位の点

		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept final override
		{
			using winrt::GraphPaper::implementation::equal;
			D2D1_POINT_2F p;
			m_shape->get_pos_loc(m_loc, p);
			return !equal(p, m_p);
		}
		// 元に戻す操作を実行する.
		virtual void exec(void) noexcept final override
		{
			D2D1_POINT_2F p;
			m_shape->get_pos_loc(m_loc, p);
			m_shape->set_pos_loc(m_p, m_loc, 0.0f, false);
			m_p = p;
		}
		// データリーダーから操作を読み込む.
		UndoDeform(DataReader const& dt_reader);
		// 指定した部位の点を保存する.
		UndoDeform(Shape* const s, const uint32_t loc);
		// データライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	//------------------------------
	// 図形の順番を入れ替える操作
	//------------------------------
	struct UndoOrder : Undo {
		Shape* m_dst_shape;	// 入れ替え先の図形

		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept final override { return m_shape != m_dst_shape; }
		// 入れ替える先の図形を得る.
		Shape* const dest(void) const noexcept { return m_dst_shape; }
		// 操作を実行する.
		virtual void exec(void) noexcept override;
		// 図形を参照しているか判定する.
		bool refer_to(const Shape* s) const noexcept final override { return Undo::refer_to(s) || m_dst_shape == s; };
		// 図形の順番を入れ替える.
		UndoOrder(Shape* const s, Shape* const t) : Undo(s), m_dst_shape(t) { UndoOrder::exec(); }
		// データリーダーから操作を読み込む.
		UndoOrder(DataReader const& dt_reader);
		// データライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	//------------------------------
	// 図形の属性値を保存して変更する操作.
	//------------------------------
	template <UNDO_T U>
	struct UndoValue : Undo, U_TYPE<U> {
		U_TYPE<U>::type m_value;	// // 変更される前の値

		~UndoValue() {};
		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept final override
		{
			using winrt::GraphPaper::implementation::equal;
			U_TYPE<U>::type val{};
			return GET(m_shape, val) && !equal(static_cast<const U_TYPE<U>::type>(val), m_value);
		}
		// 操作を実行する.
		virtual void exec(void) noexcept final override;
		// 値を図形から得る.
		static bool GET(const Shape* s, U_TYPE<U>::type& val) noexcept;
		// 値を図形に格納する.
		static void SET(Shape* const s, const U_TYPE<U>::type& val) noexcept;
		// データリーダーから操作を読み込む.
		UndoValue(DataReader const& dt_reader);
		// 図形の値を保存する.
		UndoValue(Shape* s) : Undo(s) { GET(m_shape, m_value); }
		// 図形の値を保存して変更する.
		UndoValue(Shape* s, const U_TYPE<U>::type& val) : UndoValue(s) { SET(m_shape, val); }
		// データライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	//------------------------------
	// 画像の位置と大きさの操作
	//------------------------------
	struct UndoImage : Undo {
		D2D1_POINT_2F m_start;	// 左上点
		D2D1_SIZE_F m_view;	// 表示されている画面上の寸法
		D2D1_RECT_F m_clip;	// 表示されている画像上の矩形
		D2D1_SIZE_F m_ratio;	// 先寸法と元矩形の縦横比
		float m_opac;	// 不透明度

		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept final override;
		// 操作を実行する.
		virtual void exec(void) noexcept final override;
		// データリーダーから操作を読み込む.
		UndoImage(DataReader const& dt_reader);
		// 図形の部位を保存する.
		UndoImage(ShapeImage* const s);
		// データライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	//------------------------------
	// 図形をリストに挿入または削除する操作.
	//------------------------------
	struct UndoList : Undo {
		bool m_insert;	// 挿入フラグ
		Shape* m_shape_at;	// 変更前に, 操作される位置にあった図形

		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept override { return true; }
		// 操作を実行する.
		virtual void exec(void) noexcept override;
		// 操作が挿入か判定する.
		bool is_insert(void) const noexcept { return m_insert; }
		// 図形を参照しているか判定する.
		virtual bool refer_to(const Shape* s) const noexcept override { return Undo::refer_to(s) || m_shape_at == s; };
		// 操作される位置にあった図形を得る.
		Shape* const shape_at(void) const noexcept { return m_shape_at; }
		// データリーダーから操作を読み込む.
		UndoList(DataReader const& dt_reader);
		// 図形をリストから削除する.
		UndoList::UndoList(Shape* const s, const bool dont_exec) :
			Undo(s),
			m_insert(false),
			m_shape_at(static_cast<Shape*>(nullptr))
		{
			if (!dont_exec) {
				exec();
			}
		}
		// 図形をリストに挿入する
		UndoList::UndoList(Shape* const s, Shape* const t, const bool dont_exec) :
			Undo(s),
			m_insert(true),
			m_shape_at(t)
		{
			if (!dont_exec) {
				exec();
			}
		}
		// 操作をデータライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const override;
	};

	//------------------------------
	// 図形をグループに追加または削除する操作.
	//------------------------------
	struct UndoGroup : UndoList {
		ShapeGroup* m_shape_group;	// 操作するグループ

		// 操作を実行する.
		virtual void exec(void) noexcept final override;
		// 図形を参照しているか判定する.
		bool refer_to(const Shape* s) const noexcept final override { return UndoList::refer_to(s) || m_shape_group == s; };
		// データリーダーから操作を読み込む.
		UndoGroup(DataReader const& dt_reader);
		// 図形をグループから削除する.
		UndoGroup(ShapeGroup* const g, Shape* const s) : UndoList(s, true), m_shape_group(g) { exec(); }
		// 図形をグループに追加する.
		UndoGroup(ShapeGroup* const g, Shape* const s, Shape* const s_pos) : UndoList(s, s_pos, true), m_shape_group(g) { exec(); }
		// 操作をデータライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	struct UndoText : Undo {
		bool m_flag;	// 挿入/削除フラグ
		uint32_t m_at;
		uint32_t m_len;
		wchar_t* m_text;

		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept override
		{
			if (m_flag) {
				return wchar_len(m_text) > 0;
			}
			else {
				return m_len > 0;
			}
		}
		~UndoText(void)
		{
			if (m_text != nullptr) {
				delete[] m_text;
			}
		}
		// 文字列を挿入する.
		UndoText(Shape* s, uint32_t ins_at, wchar_t* ins_text) :
			Undo(s)
		{
			wchar_t* old_text;
			s->get_text_content(old_text);
			size_t ins_len = wchar_len(ins_text);
			size_t old_len = static_cast<ShapeText*>(s)->get_text_len();
			size_t new_len = old_len + ins_len;
			const uint32_t at = min(ins_at, old_len);
			wchar_t* new_text = new wchar_t[new_len + 1];
			for (uint32_t i = 0; i < ins_at; i++) {
				new_text[i] = old_text[i];
			}
			for (uint32_t i = ins_at; i < ins_at + ins_len; i++) {
				new_text[i] = ins_text[i - ins_at];
			}
			for (uint32_t i = ins_at + ins_len; i < new_len; i++) {
				new_text[i] = old_text[i - ins_len];
			}
			new_text[new_len] = L'\0';
			s->set_text_content(new_text);

			delete[] old_text;

			// 挿入された位置を削除する位置に, 挿入された文字列の長さを削除する長さに,
			// 挿入された文字列は必要ないので, ヌルポインターを削除する文字列に格納する.
			m_flag = false;
			m_at = ins_at;
			m_len = ins_len;
			m_text = nullptr;
		}

		// 文字列を削除する.
		UndoText(Shape* s, uint32_t del_at, uint32_t del_len) :
			Undo(s)
		{
			wchar_t* old_text;
			static_cast<ShapeText*>(s)->get_text_content(old_text);
			size_t old_len = static_cast<ShapeText*>(s)->get_text_len();
			size_t new_len = old_len - min(old_len, del_len);
			wchar_t* new_text = new wchar_t[new_len + 1];
			for (uint32_t i = 0; i < del_at; i++) {
				new_text[i] = old_text[i];
			}
			for (uint32_t i = del_at; i < new_len; i++) {
				new_text[i] = old_text[i + del_len];
			}
			new_text[new_len] = L'\0';
			s->set_text_content(new_text);

			// 削除された位置を挿入された位置に, 削除された長さを挿入される長さに, 
			// 削除された文字列を挿入される文字列に格納する.
			// 挿入される文字列の長さは分かるので, その長さは必ずしも必要ないが,
			// 削除操作の読み込みをするときにこの値を前提として文字列を読み込む.
			m_flag = true;
			m_at = del_at;
			m_len = del_len;
			m_text = new wchar_t[del_len + 1];
			for (uint32_t i = 0; i < del_len; i++) {
				m_text[i] = old_text[del_at + i];
			}
			m_text[del_len] = L'\0';
			delete[] old_text;
		}
		virtual void exec(void) noexcept final override
		{
			if (m_flag) {
				wchar_t* ins_text = m_text;
				*this = UndoText(m_shape, m_at, ins_text);
				delete[] ins_text;
			}
			else {
				*this = UndoText(m_shape, m_at, m_len);
			}
		}
		UndoText(DataReader const& dt_reader);
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	//------------------------------
	// 図形の選択を反転する操作
	//------------------------------
	struct UndoSelect : Undo {
		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept final override { return true; }
		// 操作を実行する.
		virtual void exec(void) noexcept final override { m_shape->set_select(!m_shape->is_selected()); }
		// 図形の選択を反転する.
		UndoSelect(DataReader const& dt_reader);
		// 図形の選択を反転する.
		UndoSelect(Shape* const s) : Undo(s) { exec(); }
		// データライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	// 文字列の操作を破棄する.
	template <> UndoValue<UNDO_T::TEXT_CONTENT>::~UndoValue() 
	{
		if (m_value != nullptr) {
			delete[] m_value;
			m_value = nullptr;
		}
	}

	// 図形をグループに追加する.
#define UndoAppendG(g, s)	UndoGroup(static_cast<ShapeGroup* const>(g), static_cast<Shape* const>(s), static_cast<Shape* const>(nullptr))
// 図形をリストに追加する.
#define UndoAppend(s)	UndoList(static_cast<Shape* const>(s), static_cast<Shape* const>(nullptr), false)
// 図形をリストに挿入する.
#define UndoInsert(s, p)	UndoList(static_cast<Shape* const>(s), static_cast<Shape* const>(p), false)
// 図形をグループから削除する.
#define UndoRemoveG(g, s)	UndoGroup(static_cast<ShapeGroup* const>(g), static_cast<Shape* const>(s))
// 図形をリストから削除する.
#define UndoRemove(s)	UndoList(static_cast<Shape* const>(s), false)
}