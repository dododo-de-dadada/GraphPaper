//------------------------------
// MainPage_undo.cpp
// 元に戻すとやり直す
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ヌルで区切られる一連の操作を, 操作の組とみなし, その数を組数とする.
	// スタックに積むことができる最大の組数.
	constexpr uint32_t U_MAX_CNT = 32;

	// 操作スタックを消去する.
	static uint32_t undo_clear_stack(U_STACK_T& u_stack);
	// 操作をデータリーダーから読み込む.
	static bool undo_read_op(Undo*& u, DataReader const& dt_reader);
	// 操作をデータリーダーに書き込む.
	static void undo_write_op(Undo* u, DataWriter const& dt_writer);

	// 操作スタックを消去し, 含まれる操作を破棄する.
	// u_stack	操作スタック
	// 戻り値	消去した操作の組数
	static uint32_t undo_clear_stack(U_STACK_T& u_stack)
	{
		uint32_t n = 0;
		for (auto u : u_stack) {
			if (u == nullptr) {
				n++;
				continue;
			}
			delete u;
		}
		u_stack.clear();
		return n;
	}

	// 図形をグループ図形に追加して, その操作をスタックに積む.
	// g	グループ図形
	// s	追加する図形
	// 戻り値	なし
	void MainPage::undo_push_group(ShapeGroup* g, Shape* s)
	{
		m_stack_undo.push_back(new UndoAppendG(g, s));
	}

	// 操作をデータリーダーから読み込む.
	// u	操作
	// dt_reader	データリーダー
	static bool undo_read_op(Undo*& u, DataReader const& dt_reader)
	{
		if (dt_reader.UnconsumedBufferLength() < sizeof(uint32_t)) {
			return false;
		}
		auto u_op = static_cast<UNDO_OP>(dt_reader.ReadUInt32());
		if (u_op == UNDO_OP::END) {
			return false;
		}
		switch (u_op) {
		case UNDO_OP::NULLPTR:
			u = nullptr;
			break;
		case UNDO_OP::ARRANGE:
			u = new UndoArrange2(dt_reader);
			break;
		case UNDO_OP::ARROW_SIZE:
			u = new UndoSet<UNDO_OP::ARROW_SIZE>(dt_reader);
			break;
		case UNDO_OP::ARROW_STYLE:
			u = new UndoSet<UNDO_OP::ARROW_STYLE>(dt_reader);
			break;
		case UNDO_OP::FILL_COLOR:
			u = new UndoSet<UNDO_OP::FILL_COLOR>(dt_reader);
			break;
		case UNDO_OP::FORM:
			u = new UndoForm(dt_reader);
			break;
		case UNDO_OP::FONT_COLOR:
			u = new UndoSet<UNDO_OP::FONT_COLOR>(dt_reader);
			break;
		case UNDO_OP::FONT_FAMILY:
			u = new UndoSet<UNDO_OP::FONT_FAMILY>(dt_reader);
			break;
		case UNDO_OP::FONT_SIZE:
			u = new UndoSet<UNDO_OP::FONT_SIZE>(dt_reader);
			break;
		case UNDO_OP::FONT_STYLE:
			u = new UndoSet<UNDO_OP::FONT_STYLE>(dt_reader);
			break;
		case UNDO_OP::FONT_STRETCH:
			u = new UndoSet<UNDO_OP::FONT_STRETCH>(dt_reader);
			break;
		case UNDO_OP::FONT_WEIGHT:
			u = new UndoSet<UNDO_OP::FONT_WEIGHT>(dt_reader);
			break;
		case UNDO_OP::GRID_LEN:
			u = new UndoSet<UNDO_OP::GRID_LEN>(dt_reader);
			break;
		case UNDO_OP::GRID_OPAC:
			u = new UndoSet<UNDO_OP::GRID_OPAC>(dt_reader);
			break;
		case UNDO_OP::GRID_SHOW:
			u = new UndoSet<UNDO_OP::GRID_SHOW>(dt_reader);
			break;
		case UNDO_OP::GROUP:
			u = new UndoListG(dt_reader);
			break;
		case UNDO_OP::LIST:
			u = new UndoList(dt_reader);
			break;
		case UNDO_OP::TEXT_LINE:
			u = new UndoSet<UNDO_OP::TEXT_LINE>(dt_reader);
			break;
		case UNDO_OP::TEXT_MARGIN:
			u = new UndoSet<UNDO_OP::TEXT_MARGIN>(dt_reader);
			break;
		case UNDO_OP::PAGE_COLOR:
			u = new UndoSet<UNDO_OP::PAGE_COLOR>(dt_reader);
			break;
		case UNDO_OP::PAGE_SIZE:
			u = new UndoSet<UNDO_OP::PAGE_SIZE>(dt_reader);
			break;
		case UNDO_OP::TEXT_ALIGN_P:
			u = new UndoSet<UNDO_OP::TEXT_ALIGN_P>(dt_reader);
			break;
		case UNDO_OP::SELECT:
			u = new UndoSelect(dt_reader);
			break;
		case UNDO_OP::START_POS:
			u = new UndoSet<UNDO_OP::START_POS>(dt_reader);
			break;
		case UNDO_OP::STROKE_COLOR:
			u = new UndoSet<UNDO_OP::STROKE_COLOR>(dt_reader);
			break;
		case UNDO_OP::STROKE_PATTERN:
			u = new UndoSet<UNDO_OP::STROKE_PATTERN>(dt_reader);
			break;
		case UNDO_OP::STROKE_STYLE:
			u = new UndoSet<UNDO_OP::STROKE_STYLE>(dt_reader);
			break;
		case UNDO_OP::STROKE_WIDTH:
			u = new UndoSet<UNDO_OP::STROKE_WIDTH>(dt_reader);
			break;
		case UNDO_OP::TEXT:
			u = new UndoSet<UNDO_OP::TEXT>(dt_reader);
			break;
		case UNDO_OP::TEXT_ALIGN_T:
			u = new UndoSet<UNDO_OP::TEXT_ALIGN_T>(dt_reader);
			break;
		case UNDO_OP::TEXT_RANGE:
			u = new UndoSet<UNDO_OP::TEXT_RANGE>(dt_reader);
			break;
		default:
			throw winrt::hresult_not_implemented();
			break;
		}
		return true;
	}

	// 元に戻す/やり直すメニュー項目の使用の可否を設定する.
	// 操作スタックにヌル操作が含まれている場合, メニュ項目は使用できる.
	// 含まれている場合, ヌル操作によって区切られた一連の操作がスタックに含まれている.
	// そうでない場合, 図形の選択操作など, それだけでは元に戻す必要のない操作だけが含まれている.
	void MainPage::enable_undo_menu(void)
	{
		mfi_undo().IsEnabled(m_stack_ucnt > 0);
		mfi_redo().IsEnabled(m_stack_rcnt > 0);
		//bool enable_undo = false;
		//bool enable_redo = false;
		//if (m_stack_nset > 0) {
		//	for (const auto u : m_stack_undo) {
		//		if (u == nullptr) {
		//			enable_undo = true;
		//			break;
		//		}
		//	}
		//	for (const auto r : m_stack_redo) {
		//		if (r == nullptr) {
		//			enable_redo = true;
		//			break;
		//		}
		//	}
		//}
		//mfi_undo().IsEnabled(enable_undo);
		//mfi_redo().IsEnabled(enable_redo);
	}

	// 編集メニューの「やり直し」が選択された.
	void MainPage::mfi_redo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_stack_rcnt == 0) {
			return;
		}
		while (m_stack_undo.empty() == false) {
			auto u = m_stack_undo.back();
			if (u == nullptr) {
				break;
			}
			m_stack_undo.pop_back();
			undo_exec(u);
		}
		auto flag = false;
		while (m_stack_redo.size() > 0) {
			auto r = m_stack_redo.back();
			m_stack_redo.pop_back();
			m_stack_undo.push_back(r);
			if (r == nullptr) {
				break;
			}
			flag = true;
			undo_exec(r);
		}
		if (flag == false) {
			// フラグがない場合, 中断する.
			return;
		}
		m_stack_rcnt--;
		m_stack_ucnt++;
		// 編集メニュー項目の使用の可否を設定する.
		enable_edit_menu();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		if (m_summary_visible) {
			summary_update();
		}
	}

	// 編集メニューの「元に戻す」が選択された.
	void MainPage::mfi_undo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_stack_ucnt == 0) {
			return;
		}
		auto st = 0;
		while (m_stack_undo.empty() == false) {
			auto u = m_stack_undo.back();
			if (st == 0) {
				m_stack_undo.pop_back();
				if (u != nullptr) {
					undo_exec(u);
				}
				else {
					m_stack_redo.push_back(nullptr);
					st = 1;
				}
			}
			else if (st == 1) {
				if (u != nullptr) {
					m_stack_undo.pop_back();
					undo_exec(u);
					m_stack_redo.push_back(u);
				}
				else {
					break;
				}
			}
		}
		if (st != 0) {
			m_stack_ucnt--;
			m_stack_rcnt++;
		}
		// 編集メニュー項目の使用の可否を設定する.
		enable_edit_menu();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		if (m_summary_visible) {
			summary_update();
		}
	}

	// やり直す操作スタックを消去し, 含まれる操作を破棄する.
	/*
	void MainPage::redo_clear(void)
	{
		//m_stack_nset -= undo_clear_stack(m_stack_redo);
		m_stack_rcnt -= undo_clear_stack(m_stack_redo);
	}
	*/

	// 操作スタックを消去し, 含まれる操作を破棄する.
	void MainPage::undo_clear(void)
	{
		m_stack_push = false;
		//m_stack_nset -= undo_clear_stack(m_stack_redo);
		//m_stack_nset -= undo_clear_stack(m_stack_undo);
		m_stack_rcnt -= undo_clear_stack(m_stack_redo);
		m_stack_ucnt -= undo_clear_stack(m_stack_undo);
#if defined(_DEBUG)
		if (m_stack_rcnt == 0 && m_stack_ucnt == 0) {
			return;
		}
		//if (m_stack_nset == 0) {
		//	return;
		//}
		winrt::Windows::UI::Xaml::Controls::ContentDialog dialog;
		dialog.Title(box_value(L"Undo is not empty."));
		dialog.CloseButtonText(L"Close");
		auto _{ dialog.ShowAsync() };
#endif
		//m_stack_nset = 0;
	}

	// 操作を実行する.
	void MainPage::undo_exec(Undo* u)
	{
		summary_reflect(u);
		u->exec();
		auto const& u_type = typeid(*u);
		if (u_type == typeid(UndoSet<UNDO_OP::ARROW_STYLE>)) {
			// 線枠メニューの「矢じりの種類」に印をつける.
			arrow_style_check_menu(m_page_panel.m_arrow_style);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::GRID_LEN>)) {
			// 方眼の大きさをステータスバーに格納する.
			status_set_grid();
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::GRID_SHOW>)) {
			// ページメニューの「方眼の表示」に印をつける.
			grid_show_check_menu(m_page_panel.m_grid_show);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::FONT_STYLE>)) {
			// 書体メニューの「字体」に印をつける.
			font_style_check_menu(m_page_panel.m_font_style);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::PAGE_SIZE>)) {
			// ページの大きさをステータスバーに格納する.
			status_set_page();
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::STROKE_STYLE>)) {
			// 線枠メニューの「種類」に印をつける.
			stroke_style_check_menu(m_page_panel.m_stroke_style);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::TEXT_ALIGN_T>)) {
			text_align_t_check_menu(m_page_panel.m_text_align_t);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::TEXT_ALIGN_P>)) {
			text_align_p_check_menu(m_page_panel.m_text_align_p);
		}
	}

	// 無効な操作の組をポップする.
	// 戻り値	ポップされた場合 true
	bool MainPage::undo_pop_if_invalid(void)
	{
		while (m_stack_undo.size() > 0) {
			auto u = m_stack_undo.back();
			if (u == nullptr) {
				return true;
			}
			if (u->changed()) {
				break;
			}
			m_stack_undo.pop_back();
		}
		return false;
	}

	// 図形を追加して, その操作をスタックに積む.
	void MainPage::undo_push_append(Shape* s)
	{
		m_stack_undo.push_back(new UndoAppend(s));
	}

	// 図形を入れ替えて, その操作をスタックに積む.
	void MainPage::undo_push_arrange(Shape* s, Shape* t)
	{
		m_stack_undo.push_back(new UndoArrange2(s, t));
	}

	// 図形の部位の位置を変更して, 変更前の値をスタックに積む.
	void MainPage::undo_push_form(Shape* s, const ANCH_WHICH a, const D2D1_POINT_2F a_pos)
	{
		m_stack_undo.push_back(new UndoForm(s, a));
		s->set_pos(a_pos, a);
	}

	// 図形を挿入して, その操作をスタックに積む.
	void MainPage::undo_push_insert(Shape* s, Shape* s_pos)
	{
		m_stack_undo.push_back(new UndoInsert(s, s_pos));
	}

	// 一連の操作の区切としてヌル操作をスタックに積む.
	// やり直す操作スタックは消去される.
	void MainPage::undo_push_null(void)
	{
		// やり直す操作スタックを消去し, 消去された操作の組数を, 操作の組数から引く.
		//m_stack_nset -= undo_clear_stack(m_stack_redo);
		m_stack_rcnt -= undo_clear_stack(m_stack_redo);
		// 元に戻す操作スタックにヌルを積む.
		m_stack_undo.push_back(nullptr);
		// 操作スタックの更新フラグを立てる.
		m_stack_push = true;
		// 操作の組数をインクリメントする.
		//m_stack_nset++;
		m_stack_ucnt++;
		if (m_stack_ucnt <= U_MAX_CNT) {
		//if (m_stack_nset <= U_MAX_CNT) {
			// 操作の組数が最大の組数以下の場合,
			// 終了する.
			return;
		}
		while (m_stack_undo.empty() == false) {
			// 元に戻す操作スタックが空でない場合,
			// 操作スタックの一番底の操作を取り出す.
			const auto u = m_stack_undo.front();
			m_stack_undo.pop_front();
			if (u == nullptr) {
				// 操作がヌルの場合,
				// 中断する.
				break;
			}
		}
		// 操作の組数を 1 減じる.
		//m_stack_nset--;
		m_stack_ucnt--;
	}

	// 図形を差分だけ移動して, 移動前の値をスタックに積む.
	// d_pos	移動させる差分
	void MainPage::undo_push_pos(const D2D1_POINT_2F d_pos)
	{
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() == false) {
				continue;
			}
			undo_push_set<UNDO_OP::START_POS>(s);
			s->move(d_pos);
		}
	}

	// 図形をグループから削除して, その操作をスタックに積む.
	void MainPage::undo_push_remove(Shape* g, Shape* s)
	{
		m_stack_undo.push_back(new UndoRemoveG(g, s));
	}

	// 図形を削除して, その操作をスタックに積む.
	void MainPage::undo_push_remove(Shape* s)
	{
		m_stack_undo.push_back(new UndoRemove(s));
	}

	// 図形の選択を反転して, その操作をスタックに積む.
	// s	選択を反転させる図形.
	void MainPage::undo_push_select(Shape* s)
	{
		auto it_end = m_stack_undo.rend();
		// 逆順で, 操作スタックの反復子を得る.
		// 反復子の操作が終端でない場合, 以下を処理する.
		auto it = m_stack_undo.rbegin();
		if (it != it_end) {
			if (*it == nullptr) {
				// スタックトップの操作がヌルの場合,
				// 反復子を次に進める.
				it++;
			}
			// 中断フラグを消去する.
			bool suspended = false;
			// 反復子の操作が図形の選択操作の場合, 以下を繰り返す.
			while (it != it_end && *it != nullptr && typeid(**it) == typeid(UndoSelect)) {
				if ((*it)->shape() == s) {
					// 操作の図形が指定された図形と一致した場合,
					// フラグを立てて中断する.
					suspended = true;
					break;
				}
				it++;
			}
			if (suspended) {
				// 中断フラグが立っている場合, 
				// 操作スタックを介せずに図形の選択を反転させ, 
				// 操作をスタックから取り除き, 終了する.
				s->set_select(!s->is_selected());
				m_stack_undo.remove(*it);
				return;
			}
		}
		// 図形の選択を反転して, その操作をスタックに積む.
		m_stack_undo.push_back(new UndoSelect(s));
	}

	// 値を図形へ格納して, その操作をスタックに積む.
	template <UNDO_OP U, typename T>
	void MainPage::undo_push_set(Shape* s, T const& val)
	{
		m_stack_undo.push_back(new UndoSet<U>(s, val));
	}

	template void MainPage::undo_push_set<UNDO_OP::GRID_LEN>(Shape* s, double const& val);
	template void MainPage::undo_push_set<UNDO_OP::GRID_OPAC>(Shape* s, double const& val);
	template void MainPage::undo_push_set<UNDO_OP::GRID_SHOW>(Shape* s, GRID_SHOW const& val);
	template void MainPage::undo_push_set<UNDO_OP::PAGE_COLOR>(Shape* s, D2D1_COLOR_F const& val);
	template void MainPage::undo_push_set<UNDO_OP::PAGE_SIZE>(Shape* s, D2D1_SIZE_F const& val);
	template void MainPage::undo_push_set<UNDO_OP::START_POS>(Shape* s);
	template void MainPage::undo_push_set<UNDO_OP::TEXT_RANGE>(Shape* s, DWRITE_TEXT_RANGE const& val);
	template void MainPage::undo_push_set<UNDO_OP::TEXT>(Shape* s, wchar_t* const& val);

	// 図形の値の保存を実行して, その操作をスタックに積む.
	template <UNDO_OP U>
	void MainPage::undo_push_set(Shape* s)
	{
		m_stack_undo.push_back(new UndoSet<U>(s));
	}

	// 値を図形に格納して, その操作をスタックに積む.
	// U	操作の種類.
	// T	格納する型.
	// val	格納する値
	// 格納する型 T は明示しなくても引数の型から推定できる
	template<UNDO_OP U, typename T>
	void MainPage::undo_push_value(T const& val)
	{
		m_stack_undo.push_back(new UndoSet<U>(&m_page_panel, val));
		auto flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() == false) {
				continue;
			}
			T old_val{};
			if (UndoSet<U>::GET(s, old_val) == false) {
				continue;
			}
			if (equal(old_val, val)) {
				continue;
			}
			m_stack_undo.push_back(new UndoSet<U>(s, val));
			flag = true;
		}
		if (flag == false) {
			return;
		}
		undo_push_null();
		// 編集メニュー項目の使用の可否を設定する.
		enable_edit_menu();
		page_draw();
	}

	template void MainPage::undo_push_value<UNDO_OP::ARROW_SIZE>(ARROW_SIZE const& val);
	template void MainPage::undo_push_value<UNDO_OP::ARROW_STYLE>(ARROW_STYLE const& val);
	template void MainPage::undo_push_value<UNDO_OP::FILL_COLOR>(D2D1_COLOR_F const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_COLOR>(D2D1_COLOR_F const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_FAMILY>(wchar_t* const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_SIZE>(double const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_STRETCH>(DWRITE_FONT_STRETCH const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_WEIGHT>(DWRITE_FONT_WEIGHT const& val);
	template void MainPage::undo_push_value<UNDO_OP::STROKE_COLOR>(D2D1_COLOR_F const& val);
	template void MainPage::undo_push_value<UNDO_OP::STROKE_PATTERN>(STROKE_PATTERN const& val);
	template void MainPage::undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE const& val);
	template void MainPage::undo_push_value<UNDO_OP::STROKE_WIDTH>(double const& val);
	template void MainPage::undo_push_value<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT const& val);
	template void MainPage::undo_push_value<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT const& val);
	template void MainPage::undo_push_value<UNDO_OP::TEXT_LINE>(double const& val);
	template void MainPage::undo_push_value<UNDO_OP::TEXT_MARGIN>(D2D1_SIZE_F const& val);

	// 操作スタックをデータリーダーから読み込む.
	void MainPage::undo_read(DataReader const& dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		Undo* r;
		while (undo_read_op(r, dt_reader)) {
			m_stack_redo.push_back(r);
		}
		m_stack_rcnt = dt_reader.ReadUInt32();
		Undo* u;
		while (undo_read_op(u, dt_reader)) {
			m_stack_undo.push_back(u);
		}
		//m_stack_nset = dt_reader.ReadUInt32();
		m_stack_ucnt = dt_reader.ReadUInt32();
		m_stack_push = dt_reader.ReadBoolean();
	}

	// 操作スタックをデータリーダーに書き込む.
	void MainPage::undo_write(DataWriter const& dt_writer)
	{
		for (const auto& r : m_stack_redo) {
			undo_write_op(r, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::END));
		dt_writer.WriteUInt32(m_stack_rcnt);
		for (const auto& u : m_stack_undo) {
			undo_write_op(u, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::END));
		//dt_writer.WriteUInt32(m_stack_nset);
		dt_writer.WriteUInt32(m_stack_ucnt);
		dt_writer.WriteBoolean(m_stack_push);
	}

	// 操作をデータリーダーに書き込む.
	// u	操作
	// dt_writer	データライター
	static void undo_write_op(Undo* u, DataWriter const& dt_writer)
	{
		if (u != nullptr) {
			u->write(dt_writer);
		}
		else {
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::NULLPTR));
		}
	}

}