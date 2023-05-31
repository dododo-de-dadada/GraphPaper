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
		SHEET_COLOR,	// 用紙の色の操作
		SHEET_SIZE,	// 用紙の大きさの操作
		SHEET_PAD,	// 用紙の内余白の操作
		POLY_END,	// 多角形の端の操作
		REVERSE_PATH,	// 線の方向を反転する操作
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
		TEXT_WRAP,	// 文字列の折り返しの操作
		//TEXT_RANGE,	// 文字列選択の範囲の操作
		TEXT_SELECT	// 文字列選択の範囲の操作
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
	template <> struct U_TYPE<UNDO_T::SHEET_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::SHEET_SIZE> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_T::SHEET_PAD> { using type = D2D1_RECT_F; };
	template <> struct U_TYPE<UNDO_T::POLY_END> { using type = D2D1_FIGURE_END; };
	template <> struct U_TYPE<UNDO_T::STROKE_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_T::STROKE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::STROKE_WIDTH> { using type = float; };
	template <> struct U_TYPE<UNDO_T::TEXT_ALIGN_P> { using type = DWRITE_PARAGRAPH_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_T::TEXT_ALIGN_T> { using type = DWRITE_TEXT_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_T::TEXT_CONTENT> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_T::TEXT_LINE_SP> { using type = float; };
	template <> struct U_TYPE<UNDO_T::TEXT_PAD> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_T::TEXT_WRAP> { using type = DWRITE_WORD_WRAPPING; };
	//template <> struct U_TYPE<UNDO_T::TEXT_RANGE> { using type = DWRITE_TEXT_RANGE; };

	constexpr auto UNDO_SHAPE_NIL = static_cast<uint32_t>(-2);	// ヌル図形の添え字
	constexpr auto UNDO_SHAPE_SHEET = static_cast<uint32_t>(-1);	// 用紙図形の添え字

	//------------------------------
	// 操作のひな型
	//------------------------------
	struct Undo {
		//static SHAPE_LIST* undo_slist;	// 参照する図形リスト
		static ShapeSheet* undo_sheet;	// 参照する用紙

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
		static void begin(/*SHAPE_LIST* slist,*/ ShapeSheet* page) noexcept
		{
			//undo_slist = slist;
			undo_sheet = page;
		}
		// 操作する図形を得る.
		Shape* shape(void) const noexcept { return m_shape; }
		// 操作を作成する.
		Undo(Shape* s) : m_shape(s) {}
		// データライターに書き込む.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
		// データリーダーから添え字を読み込んで図形を得る.
		static Shape* undo_read_shape(DataReader const& dt_reader)
		{
			Shape* s = static_cast<Shape*>(nullptr);
			const uint32_t i = dt_reader.ReadUInt32();
			if (i == UNDO_SHAPE_SHEET) {
				s = Undo::undo_sheet;
			}
			else if (i == UNDO_SHAPE_NIL) {
				s = nullptr;
			}
			else {
				auto& slist = Undo::undo_sheet->m_shape_list;
				slist_match<const uint32_t, Shape*>(slist, i, s);
			}
			return s;
		}
		// 図形をデータライターに書き込む.
		static void undo_write_shape(
			Shape* const s,	// 書き込まれる図形
			DataWriter const& dt_writer	// データリーダー
		)
		{
			// 図形が用紙図形なら, 用紙を意味する添え字を書き込む.
			if (s == Undo::undo_sheet) {
				dt_writer.WriteUInt32(UNDO_SHAPE_SHEET);
			}
			// 図形がヌルなら, ヌルを意味する添え字を書き込む.
			else if (s == nullptr) {
				dt_writer.WriteUInt32(UNDO_SHAPE_NIL);
			}
			// それ以外なら, リスト中での図形の添え字を書き込む.
			// リスト中に図形がなければ UNDO_SHAPE_NIL が書き込まれる.
			else {
				uint32_t i = UNDO_SHAPE_NIL;
				auto& slist = Undo::undo_sheet->m_shape_list;
				slist_match<Shape* const, uint32_t>(slist, s, i);
				dt_writer.WriteUInt32(i);
			}
		}
	};

	struct UndoReverse : Undo {
		UndoReverse(Shape* s) :
			Undo(s)
		{
			exec();
		}
		UndoReverse(DataReader const& dt_reader) :
			Undo(undo_read_shape(dt_reader))
		{}
		virtual bool changed(void) const noexcept final override
		{
			return true;
		}
		// 元に戻す操作を実行する.
		virtual void exec(void) noexcept final override
		{
			m_shape->reverse_path();
		}
		// データライターに書き込む.
		virtual void write(DataWriter const& dt_writer) const final override
		{
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::REVERSE_PATH));
			undo_write_shape(m_shape, dt_writer);
		}

	};

	// 図形を変形する操作
	struct UndoDeform : Undo {
		uint32_t m_loc;	// 変形される部位
		D2D1_POINT_2F m_pt;	// 変形前の部位の点

		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept final override
		{
			using winrt::GraphPaper::implementation::equal;
			D2D1_POINT_2F p;
			m_shape->get_pos_loc(m_loc, p);
			return !equal(p, m_pt);
		}
		// 元に戻す操作を実行する.
		virtual void exec(void) noexcept final override
		{
			D2D1_POINT_2F pt;
			m_shape->get_pos_loc(m_loc, pt);
			m_shape->set_pos_loc(m_pt, m_loc, 0.0f, false);
			m_pt = pt;
		}
		// データリーダーから操作を読み込む.
		UndoDeform(DataReader const& dt_reader) :
			Undo(undo_read_shape(dt_reader)),
			m_loc(static_cast<LOC_TYPE>(dt_reader.ReadUInt32())),
			m_pt(D2D1_POINT_2F{ dt_reader.ReadSingle(), dt_reader.ReadSingle()})
		{}

		// 指定した部位の点を保存する.
		UndoDeform::UndoDeform(Shape* const s, const uint32_t loc) :
			Undo(s),
			m_loc(loc),
			m_pt([](Shape* const s, const uint32_t loc)->D2D1_POINT_2F {
				D2D1_POINT_2F pt;
				s->get_pos_loc(loc, pt);
				return pt;
			}(s, loc))
		{}

		// 図形の形の操作をデータライターに書き込む.
		void UndoDeform::write(DataWriter const& dt_writer) const final override
		{
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::DEFORM));
			undo_write_shape(m_shape, dt_writer);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_loc));
			dt_writer.WriteSingle(m_pt.x);
			dt_writer.WriteSingle(m_pt.y);
		}

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

	struct UndoTextSelect : Undo {
		int m_start;
		int m_end;
		bool m_is_trail;
		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept override
		{
			return true;
		}
		UndoTextSelect(Shape* const s, const int start, const int end, const bool is_trail) :
			Undo(s)
		{
			m_start = undo_sheet->m_select_start;
			m_end = undo_sheet->m_select_end;
			m_is_trail = undo_sheet->m_select_trail;
			undo_sheet->m_select_start = start;
			undo_sheet->m_select_end = end;
			undo_sheet->m_select_trail = is_trail;
		}
		virtual void exec(void) noexcept final override
		{
			const auto start = undo_sheet->m_select_start;
			const auto end = undo_sheet->m_select_end;
			const auto is_trail = undo_sheet->m_select_trail;
			undo_sheet->m_select_start = m_start;
			undo_sheet->m_select_end = m_end;
			undo_sheet->m_select_trail = m_is_trail;
			m_start = start;
			m_end = end;
			m_is_trail = is_trail;
		}
		UndoTextSelect(DataReader const& dt_reader);
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	struct UndoText2 : Undo {
		uint32_t m_start = 0;
		uint32_t m_end = 0;
		bool m_trail = false;
		wchar_t* m_text = nullptr;	// 保存された文字列

		// 操作を実行すると値が変わるか判定する.
		virtual bool changed(void) const noexcept override
		{
			if (m_start != (m_trail ? m_end + 1 : m_end)) {
				return true;
			}
			return wchar_len(m_text) > 0;
		}
		~UndoText2(void)
		{
			delete[] m_text;
		}

		// 文字列の選択範囲を削除し, そこに指定した文字列を挿入する.
		void edit(Shape* s, const wchar_t* ins_text) noexcept
		{
			wchar_t* old_text = static_cast<ShapeText*>(s)->m_text;
			const auto old_len = wchar_len(old_text);

			// 選択範囲と削除する文字列を保存する.
			m_start = undo_sheet->m_select_start;
			m_end = undo_sheet->m_select_end;
			m_trail = undo_sheet->m_select_trail;
			const auto end = min(m_trail ? m_end + 1 : m_end, old_len);
			const auto start = min(m_start, old_len);
			const auto m = min(start, end);
			const auto n = max(start, end);
			const auto del_len = n - m;	// 削除する文字数.
			if (del_len > 0) {
				m_text = new wchar_t[del_len + 1];
				memcpy(m_text, old_text + m, del_len * 2);
				//for (int i = 0; i < del_len; i++) {
				//	m_text[i] = old_text[m + i];
				//}
				m_text[del_len] = L'\0';
			}
			else {
				m_text = nullptr;
			}

			// 挿入後の文字数が元の文字数を超えるなら, 新しいメモリを確保して, 挿入される位置より前方をコピーする.
			const auto ins_len = wchar_len(ins_text);
			const auto new_len = old_len + ins_len - del_len;
			wchar_t* new_text;
			if (new_len > old_len) {
				new_text = new wchar_t[new_len + 1];
				//for (uint32_t i = 0; i < m; i++) {
				//	new_text[i] = old_text[i];
				//}
				memcpy(new_text, old_text, m * 2);
			}

			// 挿入後の文字数が元の文字数以下なら, 新しいメモリを確保せず, 元の文字列を利用する.
			else {
				new_text = old_text;
			}

			// 挿入する文字列をコピーする.
			//for (uint32_t i = 0; i < ins_len; i++) {
			//	new_text[m + i] = ins_text[i];
			//}
			memcpy(new_text + m, ins_text, ins_len * 2);

			// 元の文字列の選択範囲の終了位置より後方をコピーする.
			//for (uint32_t i = 0; i < old_len - n; i++) {
			//	new_text[m + ins_len + i] = old_text[n + i];
			//}
			memmove(new_text + m + ins_len, old_text + n, (old_len - n) * 2);
			new_text[new_len] = L'\0';

			// 挿入後の文字数が元の文字数を超えるなら, 元の文字列のメモリを解放して, 新しい文字列を格納する.
			if (new_len > old_len) {
				delete[] old_text;
				static_cast<ShapeText*>(s)->m_text = new_text;
			}
			static_cast<ShapeText*>(s)->m_text_len = new_len;

			// 編集後の選択範囲は, 挿入された文字列になる.
			undo_sheet->m_select_start = m;
			undo_sheet->m_select_end = m + ins_len;
			undo_sheet->m_select_trail = false;
			static_cast<ShapeText*>(s)->m_dwrite_text_layout = nullptr;
		}
		// 図形の文字列を編集する.
		UndoText2(Shape* s, const wchar_t* ins_text) :
			Undo(s)
		{
			edit(s, ins_text);
		}
		virtual void exec(void) noexcept final override
		{
			// 保存された文字列は文字列は edit 関数内で確保されたメモリ (またはヌル) で必ず上書きされる.
			// edit 関数後は必要なくなってるので解放する.
			wchar_t* ins_text = m_text;
			edit(m_shape, ins_text);
			delete[] ins_text;
		}
		UndoText2(DataReader const& dt_reader);
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