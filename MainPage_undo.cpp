//------------------------------
// MainPage_undo.cpp
// 元に戻すとやり直し操作スタック
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ヌルで区切られる一連の操作を, 操作の組とみなし, その数を組数とする.
	// スタックに積むことができる最大の組数.
	constexpr uint32_t MAX_UCNT = 32;

	// 操作スタックを消去し, 含まれる操作を破棄する.
	static uint32_t undo_clear_stack(UNDO_STACK& ustack);
	// データリーダーから操作を読み込む.
	static bool undo_read_op(Undo*& u, DataReader const& dt_reader);
	// 操作をデータリーダーに書き込む.
	static void undo_write_op(Undo* u, DataWriter const& dt_writer);

	// 操作スタックを消去し, 含まれる操作を破棄する.
	// ustack	操作スタック
	// 戻り値	消去した操作の組数
	static uint32_t undo_clear_stack(UNDO_STACK& ustack)
	{
		uint32_t n = 0;
		for (auto u : ustack) {
			if (u == nullptr) {
				n++;
				continue;
			}
			delete u;
		}
		ustack.clear();
		return n;
	}

	// データリーダーから操作を読み込む.
	// u	操作
	// dt_reader	データリーダー
	static bool undo_read_op(Undo*& u, DataReader const& dt_reader)
	{
		if (dt_reader.UnconsumedBufferLength() < sizeof(uint32_t)) {
			return false;
		}
		auto u_op = static_cast<UNDO_T>(dt_reader.ReadUInt32());
		if (u_op == UNDO_T::END) {
			return false;
		}
		switch (u_op) {
		case UNDO_T::NIL:
			u = nullptr;
			break;
		case UNDO_T::ARC_DIR:
			u = new UndoValue<UNDO_T::ARC_DIR>(dt_reader);
			break;
		case UNDO_T::ARC_END:
			u = new UndoValue<UNDO_T::ARC_END>(dt_reader);
			break;
		case UNDO_T::ARC_ROT:
			u = new UndoValue<UNDO_T::ARC_ROT>(dt_reader);
			break;
		case UNDO_T::ARC_START:
			u = new UndoValue<UNDO_T::ARC_START>(dt_reader);
			break;
		case UNDO_T::DEFORM:
			u = new UndoDeform(dt_reader);
			break;
		case UNDO_T::ORDER:
			u = new UndoOrder(dt_reader);
			break;
		case UNDO_T::ARROW_SIZE:
			u = new UndoValue<UNDO_T::ARROW_SIZE>(dt_reader);
			break;
		case UNDO_T::ARROW_STYLE:
			u = new UndoValue<UNDO_T::ARROW_STYLE>(dt_reader);
			break;
		case UNDO_T::IMAGE:
			u = new UndoImage(dt_reader);
			break;
		case UNDO_T::IMAGE_OPAC:
			u = new UndoValue<UNDO_T::IMAGE_OPAC>(dt_reader);
			break;
		case UNDO_T::FILL_COLOR:
			u = new UndoValue<UNDO_T::FILL_COLOR>(dt_reader);
			break;
		case UNDO_T::FONT_COLOR:
			u = new UndoValue<UNDO_T::FONT_COLOR>(dt_reader);
			break;
		case UNDO_T::FONT_FAMILY:
			u = new UndoValue<UNDO_T::FONT_FAMILY>(dt_reader);
			break;
		case UNDO_T::FONT_SIZE:
			u = new UndoValue<UNDO_T::FONT_SIZE>(dt_reader);
			break;
		case UNDO_T::FONT_STYLE:
			u = new UndoValue<UNDO_T::FONT_STYLE>(dt_reader);
			break;
		case UNDO_T::FONT_STRETCH:
			u = new UndoValue<UNDO_T::FONT_STRETCH>(dt_reader);
			break;
		case UNDO_T::FONT_WEIGHT:
			u = new UndoValue<UNDO_T::FONT_WEIGHT>(dt_reader);
			break;
		case UNDO_T::GRID_BASE:
			u = new UndoValue<UNDO_T::GRID_BASE>(dt_reader);
			break;
		case UNDO_T::GRID_COLOR:
			u = new UndoValue<UNDO_T::GRID_COLOR>(dt_reader);
			break;
		case UNDO_T::GRID_EMPH:
			u = new UndoValue<UNDO_T::GRID_EMPH>(dt_reader);
			break;
		case UNDO_T::GRID_SHOW:
			u = new UndoValue<UNDO_T::GRID_SHOW>(dt_reader);
			break;
		case UNDO_T::GROUP:
			u = new UndoGroup(dt_reader);
			break;
		case UNDO_T::LIST:
			u = new UndoList(dt_reader);
			break;
		case UNDO_T::SELECT:
			u = new UndoSelect(dt_reader);
			break;
		case UNDO_T::SHEET_COLOR:
			u = new UndoValue<UNDO_T::SHEET_COLOR>(dt_reader);
			break;
		case UNDO_T::SHEET_SIZE:
			u = new UndoValue<UNDO_T::SHEET_SIZE>(dt_reader);
			break;
		case UNDO_T::SHEET_PAD:
			u = new UndoValue<UNDO_T::SHEET_PAD>(dt_reader);
			break;
		case UNDO_T::MOVE:
			u = new UndoValue<UNDO_T::MOVE>(dt_reader);
			break;
		case UNDO_T::STROKE_CAP:
			u = new UndoValue<UNDO_T::STROKE_CAP>(dt_reader);
			break;
		case UNDO_T::STROKE_COLOR:
			u = new UndoValue<UNDO_T::STROKE_COLOR>(dt_reader);
			break;
		//case UNDO_T::DASH_CAP:
		//	u = new UndoValue<UNDO_T::DASH_CAP>(dt_reader);
		//	break;
		case UNDO_T::DASH_PAT:
			u = new UndoValue<UNDO_T::DASH_PAT>(dt_reader);
			break;
		case UNDO_T::DASH_STYLE:
			u = new UndoValue<UNDO_T::DASH_STYLE>(dt_reader);
			break;
		case UNDO_T::JOIN_LIMIT:
			u = new UndoValue<UNDO_T::JOIN_LIMIT>(dt_reader);
			break;
		case UNDO_T::JOIN_STYLE:
			u = new UndoValue<UNDO_T::JOIN_STYLE>(dt_reader);
			break;
		case UNDO_T::STROKE_WIDTH:
			u = new UndoValue<UNDO_T::STROKE_WIDTH>(dt_reader);
			break;
		case UNDO_T::TEXT_ALIGN_P:
			u = new UndoValue<UNDO_T::TEXT_ALIGN_P>(dt_reader);
			break;
		case UNDO_T::TEXT_ALIGN_T:
			u = new UndoValue<UNDO_T::TEXT_ALIGN_T>(dt_reader);
			break;
		case UNDO_T::TEXT_CONTENT:
			u = new UndoValue<UNDO_T::TEXT_CONTENT>(dt_reader);
			break;
		case UNDO_T::TEXT_LINE_SP:
			u = new UndoValue<UNDO_T::TEXT_LINE_SP>(dt_reader);
			break;
		case UNDO_T::TEXT_PAD:
			u = new UndoValue<UNDO_T::TEXT_PAD>(dt_reader);
			break;
		case UNDO_T::TEXT_WRAP:
			u = new UndoValue<UNDO_T::TEXT_WRAP>(dt_reader);
			break;
		case UNDO_T::TEXT_SELECT:
			u = new UndoTextSelect(dt_reader);
			break;
		default:
			throw winrt::hresult_invalid_argument();
			break;
		}
		return true;
	}

	// 操作をデータリーダーに書き込む.
	// u	操作
	// dt_writer	出力先
	static void undo_write_op(Undo* u, DataWriter const& dt_writer)
	{
		if (u != nullptr) {
			u->write(dt_writer);
		}
		else {
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::NIL));
		}
	}

	// 編集メニューの「やり直し」が選択された.
	void MainPage::redo_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 最初が空操作ならそれらを取りのぞく.
		while (m_redo_stack.size() > 0 && m_redo_stack.back() == nullptr) {
			m_redo_stack.pop_back();
		}
		bool pushed = false;
		Undo* u;
		while (m_redo_stack.size() > 0 && (u = m_redo_stack.back()) != nullptr) {
			summary_reflect(u);
			undo_exec(u);
			m_redo_stack.pop_back();
			if (!pushed) {
				if (m_undo_stack.size() > 0) {
					m_undo_stack.push_back(nullptr);
				}
				pushed = true;
			}
			m_undo_stack.push_back(u);
		}
		if (pushed) {
			main_bbox_update();
			main_panel_size();
			main_draw();
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_update();
			}
		}
		status_bar_set_pos();
	}

	// 操作スタックを消去し, 含まれる操作を破棄する.
	void MainPage::undo_clear(void)
	{
		//m_ustack_is_changed = false;
		undo_clear_stack(m_redo_stack);
		undo_clear_stack(m_undo_stack);
	}

	// 編集メニューの「元に戻す」が選択された.
	void MainPage::undo_click(IInspectable const&, RoutedEventArgs const&)
	{
		// 最初が空操作ならそれらを取りのぞく.
		while (m_undo_stack.size() > 0 && m_undo_stack.back() == nullptr) {
			m_undo_stack.pop_back();
		}
		bool pushed = false;
		Undo* u;
		while (m_undo_stack.size() > 0 && (u = m_undo_stack.back()) != nullptr) {
			summary_reflect(u);
			undo_exec(u);
			m_undo_stack.pop_back();
			if (!pushed) {
				if (m_redo_stack.size() > 0) {
					m_redo_stack.push_back(nullptr);
				}
				pushed = true;
			}
			m_redo_stack.push_back(u);
		}
		if (pushed) {
			main_bbox_update();
			main_panel_size();
			main_draw();
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_update();
			}
		}
		status_bar_set_pos();
	}

	// 操作を実行する.
	// u	操作
	void MainPage::undo_exec(Undo* u)
	{
		u->exec();
		auto const& u_type = typeid(*u);
		if (u_type == typeid(UndoSelect)) {
			if (u->m_shape->is_selected()) {
				//undo_selected_cnt<true>(u->m_shape);
				m_undo_select_cnt++;
			}
			else {
				//undo_selected_cnt<false>(u->m_shape);
				m_undo_select_cnt--;
			}
#ifdef _DEBUG
			debug_cnt();
#endif
		}
		else if (u_type == typeid(UndoList)) {
			const auto s = u->m_shape;
			// 図形が削除された.
			if (static_cast<UndoList*>(u)->m_insert) {
				// 選択された数は 1 減らす.
				if (s->is_selected()) {
					//undo_selected_cnt<true>(s);
					m_undo_select_cnt++;
				}
				m_undo_undeleted_cnt--;
#ifdef _DEBUG
				debug_cnt();
#endif
			}
			// 図形が追加された.
			else {
				if (s->is_selected()) {
					//undo_selected_cnt<true>(s);
					m_undo_select_cnt++;
				}
				m_undo_undeleted_cnt++;
#ifdef _DEBUG
				debug_cnt();
#endif
			}
		}
		else if (u_type == typeid(UndoValue<UNDO_T::ARROW_STYLE>)) {
			ARROW_STYLE val;
			m_main_sheet.get_arrow_style(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::FONT_STYLE>)) {
			// 書体メニューの「字体」に印をつける.
			DWRITE_FONT_STYLE val;
			m_main_sheet.get_font_style(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::FONT_WEIGHT>)) {
			// 書体メニューの「字体」に印をつける.
			DWRITE_FONT_WEIGHT val;
			m_main_sheet.get_font_weight(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::FONT_STRETCH>)) {
			// 書体メニューの「字体」に印をつける.
			DWRITE_FONT_STRETCH val;
			m_main_sheet.get_font_stretch(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::GRID_BASE>)) {
			// 方眼の大きさをステータスバーに格納する.
			status_bar_set_grid();
		}
		else if (u_type == typeid(UndoValue<UNDO_T::GRID_EMPH>)) {
			// レイアウトメニューの「方眼の強調」に印をつける.
			GRID_EMPH val;
			m_main_sheet.get_grid_emph(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::GRID_SHOW>)) {
			GRID_SHOW val;
			m_main_sheet.get_grid_show(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::SHEET_SIZE>)) {
			// ページの大きさをステータスバーに格納する.
			status_bar_set_sheet();
		}
		else if (u_type == typeid(UndoValue<UNDO_T::STROKE_CAP>)) {
			D2D1_CAP_STYLE val;
			m_main_sheet.get_stroke_cap(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::DASH_STYLE>)) {
			D2D1_DASH_STYLE val;
			m_main_sheet.get_stroke_dash(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::JOIN_STYLE>)) {
			D2D1_LINE_JOIN val;
			m_main_sheet.get_stroke_join(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::STROKE_WIDTH>)) {
			float val;
			m_main_sheet.get_stroke_width(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::TEXT_ALIGN_T>)) {
			DWRITE_TEXT_ALIGNMENT val;
			m_main_sheet.get_text_align_horz(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::TEXT_ALIGN_P>)) {
			DWRITE_PARAGRAPH_ALIGNMENT val;
			m_main_sheet.get_text_align_vert(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::TEXT_WRAP>)) {
			DWRITE_WORD_WRAPPING val;
			m_main_sheet.get_text_wrap(val);
		}
	}

	// 無効な操作をポップする.
	// 戻り値	操作がひとつ以上ポップされたなら true
	bool MainPage::undo_pop_invalid(void)
	{
		while (m_undo_stack.size() > 0) {
			auto u = m_undo_stack.back();
			if (u == nullptr) {
				return true;
			}
			if (u->changed()) {
				break;
			}
			m_undo_stack.pop_back();
		}
		return false;
	}

	// 選択された (あるいは全ての) 図形の位置をスタックに保存してから差分だけ移動する.
	// to	移動先へのベクトル
	// any_shape	図形すべての場合 true, 選択された図形のみの場合 false
	void MainPage::undo_push_move(const D2D1_POINT_2F to, const bool any_shape)
	{
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted() || (!any_shape && !s->is_selected())) {
				continue;
			}
			undo_push_set<UNDO_T::MOVE>(s);
			s->move(to);
		}
	}

	// 一連の操作の区切としてヌル操作をスタックに積む.
	// やり直し操作スタックは消去される.
	void MainPage::undo_push_null(void)
	{		
		// やり直し操作スタックを消去し, 消去された操作の組数を, 操作の組数から引く.
		undo_clear_stack(m_redo_stack);
		if (m_undo_stack.size() > 0 && m_undo_stack.back() != nullptr) {
			m_undo_stack.push_back(nullptr);
		}
		if (m_undo_stack.size() > 1024) {
			// 1024 個を超える操作は問答無用でスタックの先頭から取りのぞいて削除する.
			while (m_undo_stack.size() > 1024) {
				Undo* u = m_undo_stack.front();
				m_undo_stack.pop_front();
				delete u;
			}
			// 空以外の操作をスタックの先頭から取りのぞいて削除する.
			Undo* u;
			while (!m_undo_stack.empty() && (u = m_undo_stack.front()) != nullptr) {
				m_undo_stack.pop_front();
				delete u;
			}
			// 空操作を取り除いて削除する.
			if (!m_undo_stack.empty()) {
				m_undo_stack.pop_front();
			}
		}
		// true をスタックが更新されたか判定に格納する.
		//m_ustack_is_changed = true;
	}


	//template<bool I> void MainPage::undo_selected_cnt(Shape* s)
	//{
	//	if constexpr (I) m_undo_select_cnt++; else m_undo_select_cnt--;
	//	if (s->exist_cap()) {
	//		if constexpr (I) m_undo_selected_exist_cap++; else m_undo_selected_exist_cap--;
	//	}
	//	if (typeid(*s) == typeid(ShapeGroup)) {
	//		if constexpr (I) m_undo_selected_group++; else m_undo_selected_group--;
	//	}
	//	else if (typeid(*s) == typeid(ShapeText)) {
	//		if constexpr (I) m_undo_selected_text++; else m_undo_selected_text--;
	//	}
	//	else if (typeid(*s) == typeid(ShapeLine)) {
	//		if constexpr (I) m_undo_selected_line++; else m_undo_selected_line--;
	//	}
	//	else if (typeid(*s) == typeid(ShapeImage)) {
	//		if constexpr (I) m_undo_selected_image++; else m_undo_selected_image--;
	//	}
	//	else if (typeid(*s) == typeid(ShapeRuler)) {
	//		if constexpr (I) m_undo_selected_ruler++; else m_undo_selected_ruler--;
	//	}
	//	else if (typeid(*s) == typeid(ShapeArc)) {
	//		if constexpr (I) m_undo_selected_arc++; else m_undo_selected_arc--;
	//	}
	//	else if (typeid(*s) == typeid(ShapePoly)) {
	//		if (s->exist_cap()) {
	//			if constexpr (I) m_undo_selected_polyline++; else m_undo_selected_polyline--;
	//		}
	//		else {
	//			if constexpr (I) m_undo_selected_polygon++; else m_undo_selected_polygon--;
	//		}
	//	}
	//}

	// 図形の選択を反転して, その操作をスタックに積む.
	// s	選択を反転させる図形.
	void MainPage::undo_push_select(Shape* s)
	{
		const auto it_end = m_undo_stack.rend();
		Undo* u;
		for (auto it = m_undo_stack.rbegin();
			it != it_end && (u = *it) != nullptr && typeid(*u) == typeid(UndoSelect); it++) {
			if (u->shape() == s) {
				// 操作の図形が指定された図形と一致した場合,
				// 操作スタックを介せずに図形の選択を反転させ, 
				// 以前の操作をスタックから取り除き, 終了する.
				if (s->is_selected()) {
					s->set_select(false);
					//undo_selected_cnt<false>(s);
					m_undo_select_cnt--;
				}
				else {
					s->set_select(true);
					//undo_selected_cnt<true>(s);
					m_undo_select_cnt++;
				}
#ifdef _DEBUG
				debug_cnt();
#endif
				m_undo_stack.remove(u);
				return;
			}
		}
		// 図形の選択を反転して, その操作をスタックに積む.
		m_undo_stack.push_back(new UndoSelect(s));
		if (s->is_selected()) {
			//undo_selected_cnt<true>(s);
			m_undo_select_cnt++;
		}
		else {
			//undo_selected_cnt<false>(s);
			m_undo_select_cnt--;
		}
#ifdef _DEBUG
		debug_cnt();
#endif
	}

	// 値を指定する図形へ格納し, その操作をスタックに積む.
	// U	操作の種類
	// T	値の型
	// s	図形
	// val	値
	template <UNDO_T U, typename T> void MainPage::undo_push_set(Shape* const s, T const& val)
	{
		// 図形がその値を持たない, または同値なら中断する.
		T old_val;
		if (!UndoValue<U>::GET(s, old_val) || equal(val, old_val)) {
			return;
		}
		// それ以外なら, 値を格納してその操作をスタックに積む.
		m_undo_stack.push_back(new UndoValue<U>(s, val));
	}

	template void MainPage::menu_is_checked<UNDO_T::NIL>(void);

	template<UNDO_T U> void MainPage::menu_is_checked(void)
	{
		if constexpr (U == UNDO_T::DASH_STYLE || U == UNDO_T::NIL) {
			if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID) {
				rmfi_popup_stroke_dash_solid().IsChecked(true);
				rmfi_menu_stroke_dash_solid().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(false);
				mfi_menu_stroke_dash_pat().IsEnabled(false);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
				rmfi_popup_stroke_dash_dash().IsChecked(true);
				rmfi_menu_stroke_dash_dash().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
				rmfi_popup_stroke_dash_dot().IsChecked(true);
				rmfi_menu_stroke_dash_dot().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
				rmfi_popup_stroke_dash_dash_dot().IsChecked(true);
				rmfi_menu_stroke_dash_dash_dot().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
				rmfi_popup_stroke_dash_dash_dot_dot().IsChecked(true);
				rmfi_menu_stroke_dash_dash_dot_dot().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
		}
		else if constexpr (U == UNDO_T::STROKE_WIDTH || U == UNDO_T::NIL) {
			if (equal(m_main_sheet.m_stroke_width, 0.0f)) {
				rmfi_popup_stroke_width_0px().IsChecked(true);
				rmfi_menu_stroke_width_0px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 1.0f)) {
				rmfi_popup_stroke_width_1px().IsChecked(true);
				rmfi_menu_stroke_width_1px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 2.0f)) {
				rmfi_popup_stroke_width_2px().IsChecked(true);
				rmfi_menu_stroke_width_2px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 3.0f)) {
				rmfi_popup_stroke_width_3px().IsChecked(true);
				rmfi_menu_stroke_width_3px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 4.0f)) {
				rmfi_popup_stroke_width_4px().IsChecked(true);
				rmfi_menu_stroke_width_4px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 8.0f)) {
				rmfi_popup_stroke_width_8px().IsChecked(true);
				rmfi_menu_stroke_width_8px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 12.0f)) {
				rmfi_popup_stroke_width_12px().IsChecked(true);
				rmfi_menu_stroke_width_12px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 16.0f)) {
				rmfi_popup_stroke_width_16px().IsChecked(true);
				rmfi_menu_stroke_width_16px().IsChecked(true);
			}
			else {
				rmfi_popup_stroke_width_other().IsChecked(true);
				rmfi_menu_stroke_width_other().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::STROKE_CAP || U == UNDO_T::NIL) {
			if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT) {
				rmfi_popup_stroke_cap_flat().IsChecked(true);
				rmfi_menu_stroke_cap_flat().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
				rmfi_popup_stroke_cap_square().IsChecked(true);
				rmfi_menu_stroke_cap_square().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				rmfi_popup_stroke_cap_round().IsChecked(true);
				rmfi_menu_stroke_cap_round().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				rmfi_popup_stroke_cap_triangle().IsChecked(true);
				rmfi_menu_stroke_cap_triangle().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::ARROW_STYLE || U == UNDO_T::NIL) {
			if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_NONE) {
				rmfi_popup_stroke_arrow_none().IsChecked(true);
				rmfi_menu_stroke_arrow_none().IsChecked(true);
				mfi_popup_stroke_arrow_size().IsEnabled(false);
				mfi_menu_stroke_arrow_size().IsEnabled(false);
			}
			else if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_OPENED) {
				rmfi_popup_stroke_arrow_opened().IsChecked(true);
				rmfi_menu_stroke_arrow_opened().IsChecked(true);
				mfi_popup_stroke_arrow_size().IsEnabled(true);
				mfi_menu_stroke_arrow_size().IsEnabled(true);
			}
			else if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_FILLED) {
				rmfi_popup_stroke_arrow_filled().IsChecked(true);
				rmfi_menu_stroke_arrow_filled().IsChecked(true);
				mfi_popup_stroke_arrow_size().IsEnabled(true);
				mfi_menu_stroke_arrow_size().IsEnabled(true);
			}
		}
		else if constexpr (U == UNDO_T::JOIN_STYLE || U == UNDO_T::NIL) {
			if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				rmfi_popup_stroke_join_bevel().IsChecked(true);
				rmfi_menu_stroke_join_bevel().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
				rmfi_popup_stroke_join_miter().IsChecked(true);
				rmfi_menu_stroke_join_miter().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				rmfi_popup_stroke_join_miter_or_bevel().IsChecked(true);
				rmfi_menu_stroke_join_miter_or_bevel().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				rmfi_popup_stroke_join_round().IsChecked(true);
				rmfi_menu_stroke_join_round().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::FONT_STRETCH || U == UNDO_T::NIL) {
			if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED) {
				rmfi_popup_font_stretch_ultra_condensed().IsChecked(true);
				rmfi_menu_font_stretch_ultra_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED) {
				rmfi_popup_font_stretch_extra_condensed().IsChecked(true);
				rmfi_menu_font_stretch_extra_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED) {
				rmfi_popup_font_stretch_condensed().IsChecked(true);
				rmfi_menu_font_stretch_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED) {
				rmfi_popup_font_stretch_semi_condensed().IsChecked(true);
				rmfi_menu_font_stretch_semi_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL) {
				rmfi_popup_font_stretch_normal().IsChecked(true);
				rmfi_menu_font_stretch_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED) {
				rmfi_popup_font_stretch_semi_expanded().IsChecked(true);
				rmfi_menu_font_stretch_semi_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED) {
				rmfi_popup_font_stretch_expanded().IsChecked(true);
				rmfi_menu_font_stretch_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED) {
				rmfi_popup_font_stretch_extra_expanded().IsChecked(true);
				rmfi_menu_font_stretch_extra_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED) {
				rmfi_popup_font_stretch_ultra_expanded().IsChecked(true);
				rmfi_menu_font_stretch_ultra_expanded().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::FONT_STYLE || U == UNDO_T::NIL) {
			if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC) {
				rmfi_popup_font_style_italic().IsChecked(true);
				rmfi_menu_font_style_italic().IsChecked(true);
			}
			else if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL) {
				rmfi_popup_font_style_normal().IsChecked(true);
				rmfi_menu_font_style_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE) {
				rmfi_popup_font_style_oblique().IsChecked(true);
				rmfi_menu_font_style_oblique().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::FONT_WEIGHT || U == UNDO_T::NIL) {
			if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_THIN) {
				rmfi_popup_font_weight_thin().IsChecked(true);
				rmfi_menu_font_weight_thin().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_LIGHT) {
				rmfi_popup_font_weight_extra_light().IsChecked(true);
				rmfi_menu_font_weight_extra_light().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_LIGHT) {
				rmfi_popup_font_weight_light().IsChecked(true);
				rmfi_menu_font_weight_light().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL) {
				rmfi_popup_font_weight_normal().IsChecked(true);
				rmfi_menu_font_weight_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_MEDIUM) {
				rmfi_popup_font_weight_medium().IsChecked(true);
				rmfi_menu_font_weight_medium().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_BOLD) {
				rmfi_popup_font_weight_semi_bold().IsChecked(true);
				rmfi_menu_font_weight_semi_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BOLD) {
				rmfi_popup_font_weight_bold().IsChecked(true);
				rmfi_menu_font_weight_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BOLD) {
				rmfi_popup_font_weight_extra_bold().IsChecked(true);
				rmfi_menu_font_weight_extra_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BLACK) {
				rmfi_popup_font_weight_black().IsChecked(true);
				rmfi_menu_font_weight_black().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BLACK) {
				rmfi_popup_font_weight_extra_black().IsChecked(true);
				rmfi_menu_font_weight_extra_black().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::TEXT_ALIGN_T || U == UNDO_T::NIL) {
			if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING) {
				rmfi_popup_text_align_left().IsChecked(true);
				rmfi_menu_text_align_left().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING) {
				rmfi_popup_text_align_right().IsChecked(true);
				rmfi_menu_text_align_right().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER) {
				rmfi_popup_text_align_center().IsChecked(true);
				rmfi_menu_text_align_center().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED) {
				rmfi_popup_text_align_just().IsChecked(true);
				rmfi_menu_text_align_just().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::TEXT_ALIGN_P || U == UNDO_T::NIL) {
			if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR) {
				rmfi_popup_text_align_top().IsChecked(true);
				rmfi_menu_text_align_top().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR) {
				rmfi_popup_text_align_bot().IsChecked(true);
				rmfi_menu_text_align_bot().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER) {
				rmfi_popup_text_align_mid().IsChecked(true);
				rmfi_menu_text_align_mid().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::TEXT_WRAP || U == UNDO_T::NIL) {
			if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP) {
				rmfi_popup_text_wrap().IsChecked(true);
				rmfi_menu_text_wrap().IsChecked(true);
			}
			else if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_NO_WRAP) {
				rmfi_popup_text_no_wrap().IsChecked(true);
				rmfi_menu_text_no_wrap().IsChecked(true);
			}
			else if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_CHARACTER) {
				rmfi_popup_text_wrap_char().IsChecked(true);
				rmfi_menu_text_wrap_char().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::GRID_EMPH || U == UNDO_T::NIL) {
			if (m_main_sheet.m_grid_emph.m_gauge_1 == 0 && m_main_sheet.m_grid_emph.m_gauge_2 == 0) {
				rmfi_popup_grid_emph_1().IsChecked(true);
				rmfi_menu_grid_emph_1().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_emph.m_gauge_1 != 0 && m_main_sheet.m_grid_emph.m_gauge_2 == 0) {
				rmfi_popup_grid_emph_2().IsChecked(true);
				rmfi_menu_grid_emph_2().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_emph.m_gauge_1 != 0 && m_main_sheet.m_grid_emph.m_gauge_2 != 0) {
				rmfi_popup_grid_emph_3().IsChecked(true);
				rmfi_menu_grid_emph_3().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::GRID_SHOW || U == UNDO_T::NIL) {
			if (m_main_sheet.m_grid_show == GRID_SHOW::BACK) {
				rmfi_popup_grid_show_back().IsChecked(true);
				rmfi_menu_grid_show_back().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_show == GRID_SHOW::FRONT) {
				rmfi_popup_grid_show_front().IsChecked(true);
				rmfi_menu_grid_show_front().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_show == GRID_SHOW::HIDE) {
				rmfi_popup_grid_show_hide().IsChecked(true);
				rmfi_menu_grid_show_hide().IsChecked(true);
			}
		}
	}
	// 値を選択された図形に格納して, その操作をスタックに積む.
	// 戻り値	格納される前の値と異なっており, 値が格納されたら true.
	// U	操作の種類
	// T	値の型
	// val	値
	template<UNDO_T U, typename T> bool MainPage::undo_push_set(const T& val)
	{
		// メイン用紙に属性ありなら, メイン用紙の属性も変更する.
		T sheet_val;
		if (UndoValue<U>::GET(&m_main_sheet, sheet_val) && !equal(sheet_val, val)) {
			m_undo_stack.push_back(new UndoValue<U>(&m_main_sheet, val));
			menu_is_checked<U>();
		}
		auto changed = false;	// 値が変わった図形があるか.
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			undo_push_set<U>(s, val);
			changed = true;
		}
		return changed;
	}

	// 図形の値の保存し, その操作をスタックに積む.
	// U	操作の種類.
	// s	図形
	template <UNDO_T U> void MainPage::undo_push_set(Shape* const s)
	{
		m_undo_stack.push_back(new UndoValue<U>(s));
	}

	template bool MainPage::undo_push_set<UNDO_T::ARC_DIR>(D2D1_SWEEP_DIRECTION const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARC_END>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARC_ROT>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARC_START>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARROW_CAP>(D2D1_CAP_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARROW_JOIN>(D2D1_LINE_JOIN const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARROW_SIZE>(ARROW_SIZE const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARROW_STYLE>(ARROW_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::IMAGE_OPAC>(float const& val);
	//template bool MainPage::undo_push_set<UNDO_T::DASH_CAP>(D2D1_CAP_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::DASH_PAT>(DASH_PAT const& val);
	template bool MainPage::undo_push_set<UNDO_T::DASH_STYLE>(D2D1_DASH_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::FILL_COLOR>(D2D1_COLOR_F const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_COLOR>(D2D1_COLOR_F const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_FAMILY>(wchar_t* const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_SIZE>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_STRETCH>(DWRITE_FONT_STRETCH const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_STYLE>(DWRITE_FONT_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_WEIGHT>(DWRITE_FONT_WEIGHT const& val);
	template bool MainPage::undo_push_set<UNDO_T::JOIN_LIMIT>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::JOIN_STYLE>(D2D1_LINE_JOIN const& val);
	template bool MainPage::undo_push_set<UNDO_T::POLY_END>(D2D1_FIGURE_END const& val);
	template bool MainPage::undo_push_set<UNDO_T::STROKE_CAP>(D2D1_CAP_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::STROKE_COLOR>(D2D1_COLOR_F const& val);
	template bool MainPage::undo_push_set<UNDO_T::STROKE_WIDTH>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT const& val);
	template bool MainPage::undo_push_set<UNDO_T::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT const& val);
	template bool MainPage::undo_push_set<UNDO_T::TEXT_LINE_SP>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::TEXT_PAD>(D2D1_SIZE_F const& val);
	template bool MainPage::undo_push_set<UNDO_T::TEXT_WRAP>(DWRITE_WORD_WRAPPING const& val);

	template void MainPage::undo_push_set<UNDO_T::GRID_BASE>(Shape* const s, float const& val);
	template void MainPage::undo_push_set<UNDO_T::GRID_EMPH>(Shape* const s, GRID_EMPH const& val);
	template void MainPage::undo_push_set<UNDO_T::GRID_COLOR>(Shape* const s, D2D1_COLOR_F const& val);
	template void MainPage::undo_push_set<UNDO_T::GRID_SHOW>(Shape* const s, GRID_SHOW const& val);
	template void MainPage::undo_push_set<UNDO_T::SHEET_COLOR>(Shape* const s, D2D1_COLOR_F const& val);
	template void MainPage::undo_push_set<UNDO_T::SHEET_SIZE>(Shape* const s, D2D1_SIZE_F const& val);
	template void MainPage::undo_push_set<UNDO_T::SHEET_PAD>(Shape* const s, D2D1_RECT_F const& val);
	template void MainPage::undo_push_set<UNDO_T::TEXT_CONTENT>(Shape* const s, wchar_t* const& val);

	template void MainPage::undo_push_set<UNDO_T::MOVE>(Shape* const s);
	template void MainPage::undo_push_set<UNDO_T::IMAGE_OPAC>(Shape* const s);

	// データリーダーから操作スタックを読み込む.
	// dt_reader	データリーダー
	void MainPage::undo_read_stack(DataReader const& dt_reader)
	{
		Undo* r;
		while (undo_read_op(r, dt_reader)) {
			m_redo_stack.push_back(r);
		}
		Undo* u;
		while (undo_read_op(u, dt_reader)) {
			m_undo_stack.push_back(u);
		}
		//m_ustack_is_changed = dt_reader.ReadBoolean();
	}

	// 操作スタックをデータライターに書き込む.
	// dt_writer	データライター
	void MainPage::undo_write_stack(DataWriter const& dt_writer)
	{
		for (const auto& r : m_redo_stack) {
			undo_write_op(r, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::END));
		for (const auto& u : m_undo_stack) {
			undo_write_op(u, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::END));
		//dt_writer.WriteBoolean(m_ustack_is_changed);
	}

}