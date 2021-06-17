//------------------------------
// MainPage_undo.cpp
// 元に戻すとやり直し
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

// 図形をグループに追加する.
#define UndoAppendG(g, s)	UndoListGroup(static_cast<ShapeGroup* const>(g), static_cast<Shape* const>(s), static_cast<Shape* const>(nullptr))
// 図形をリストに追加する.
#define UndoAppend(s)	UndoList(static_cast<Shape* const>(s), static_cast<Shape* const>(nullptr))
// 図形をリストに挿入する.
#define UndoInsert(s, p)	UndoList(static_cast<Shape* const>(s), static_cast<Shape* const>(p))
// 図形をグループから取り除く.
#define UndoRemoveG(g, s)	UndoListGroup(static_cast<ShapeGroup* const>(g), static_cast<Shape* const>(s))
// 図形をリストから取り除く.
#define UndoRemove(s)	UndoList(static_cast<Shape* const>(s))

namespace winrt::GraphPaper::implementation
{
	// ヌルで区切られる一連の操作を, 操作の組とみなし, その数を組数とする.
	// スタックに積むことができる最大の組数.
	constexpr uint32_t MAX_UCNT = 32;

	// 操作スタックを消去し, 含まれる操作を破棄する.
	static uint32_t undo_clear_stack(UNDO_STACK& u_stack);
	// データリーダーから操作を読み込む.
	static bool undo_read_op(Undo*& u, DataReader const& dt_reader);
	// 操作をデータリーダーに書き込む.
	static void undo_write_op(Undo* u, DataWriter const& dt_writer);

	// 操作スタックを消去し, 含まれる操作を破棄する.
	// u_stack	操作スタック
	// 戻り値	消去した操作の組数
	static uint32_t undo_clear_stack(UNDO_STACK& u_stack)
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

	// データリーダーから操作を読み込む.
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
		case UNDO_OP::ANCH_POS:
			u = new UndoAnchor(dt_reader);
			break;
		case UNDO_OP::ARRANGE:
			u = new UndoArrange2(dt_reader);
			break;
		case UNDO_OP::ARROW_SIZE:
			u = new UndoAttr<UNDO_OP::ARROW_SIZE>(dt_reader);
			break;
		case UNDO_OP::ARROW_STYLE:
			u = new UndoAttr<UNDO_OP::ARROW_STYLE>(dt_reader);
			break;
		case UNDO_OP::FILL_COLOR:
			u = new UndoAttr<UNDO_OP::FILL_COLOR>(dt_reader);
			break;
		case UNDO_OP::FONT_COLOR:
			u = new UndoAttr<UNDO_OP::FONT_COLOR>(dt_reader);
			break;
		case UNDO_OP::FONT_FAMILY:
			u = new UndoAttr<UNDO_OP::FONT_FAMILY>(dt_reader);
			break;
		case UNDO_OP::FONT_SIZE:
			u = new UndoAttr<UNDO_OP::FONT_SIZE>(dt_reader);
			break;
		case UNDO_OP::FONT_STYLE:
			u = new UndoAttr<UNDO_OP::FONT_STYLE>(dt_reader);
			break;
		case UNDO_OP::FONT_STRETCH:
			u = new UndoAttr<UNDO_OP::FONT_STRETCH>(dt_reader);
			break;
		case UNDO_OP::FONT_WEIGHT:
			u = new UndoAttr<UNDO_OP::FONT_WEIGHT>(dt_reader);
			break;
		case UNDO_OP::GRID_BASE:
			u = new UndoAttr<UNDO_OP::GRID_BASE>(dt_reader);
			break;
		case UNDO_OP::GRID_GRAY:
			u = new UndoAttr<UNDO_OP::GRID_GRAY>(dt_reader);
			break;
		case UNDO_OP::GRID_EMPH:
			u = new UndoAttr<UNDO_OP::GRID_EMPH>(dt_reader);
			break;
		case UNDO_OP::GRID_SHOW:
			u = new UndoAttr<UNDO_OP::GRID_SHOW>(dt_reader);
			break;
		case UNDO_OP::GROUP:
			u = new UndoListGroup(dt_reader);
			break;
		case UNDO_OP::LIST:
			u = new UndoList(dt_reader);
			break;
		case UNDO_OP::SELECT:
			u = new UndoSelect(dt_reader);
			break;
		case UNDO_OP::SHEET_COLOR:
			u = new UndoAttr<UNDO_OP::SHEET_COLOR>(dt_reader);
			break;
		case UNDO_OP::SHEET_SIZE:
			u = new UndoAttr<UNDO_OP::SHEET_SIZE>(dt_reader);
			break;
		case UNDO_OP::START_POS:
			u = new UndoAttr<UNDO_OP::START_POS>(dt_reader);
			break;
		case UNDO_OP::STROKE_CAP_STYLE:
			u = new UndoAttr<UNDO_OP::STROKE_CAP_STYLE>(dt_reader);
			break;
		case UNDO_OP::STROKE_COLOR:
			u = new UndoAttr<UNDO_OP::STROKE_COLOR>(dt_reader);
			break;
		case UNDO_OP::STROKE_DASH_CAP:
			u = new UndoAttr<UNDO_OP::STROKE_DASH_CAP>(dt_reader);
			break;
		case UNDO_OP::STROKE_DASH_PATT:
			u = new UndoAttr<UNDO_OP::STROKE_DASH_PATT>(dt_reader);
			break;
		case UNDO_OP::STROKE_DASH_STYLE:
			u = new UndoAttr<UNDO_OP::STROKE_DASH_STYLE>(dt_reader);
			break;
		case UNDO_OP::STROKE_JOIN_LIMIT:
			u = new UndoAttr<UNDO_OP::STROKE_JOIN_LIMIT>(dt_reader);
			break;
		case UNDO_OP::STROKE_JOIN_STYLE:
			u = new UndoAttr<UNDO_OP::STROKE_JOIN_STYLE>(dt_reader);
			break;
		case UNDO_OP::STROKE_WIDTH:
			u = new UndoAttr<UNDO_OP::STROKE_WIDTH>(dt_reader);
			break;
		case UNDO_OP::TEXT_ALIGN_P:
			u = new UndoAttr<UNDO_OP::TEXT_ALIGN_P>(dt_reader);
			break;
		case UNDO_OP::TEXT_ALIGN_T:
			u = new UndoAttr<UNDO_OP::TEXT_ALIGN_T>(dt_reader);
			break;
		case UNDO_OP::TEXT_CONTENT:
			u = new UndoAttr<UNDO_OP::TEXT_CONTENT>(dt_reader);
			break;
		case UNDO_OP::TEXT_LINE_H:
			u = new UndoAttr<UNDO_OP::TEXT_LINE_H>(dt_reader);
			break;
		case UNDO_OP::TEXT_MARGIN:
			u = new UndoAttr<UNDO_OP::TEXT_MARGIN>(dt_reader);
			break;
		case UNDO_OP::TEXT_RANGE:
			u = new UndoAttr<UNDO_OP::TEXT_RANGE>(dt_reader);
			break;
		default:
			throw winrt::hresult_invalid_argument();
			break;
		}
		return true;
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

	// 編集メニューの「やり直し」が選択された.
	void MainPage::redo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_stack_rcnt == 0) {
			return;
		}
		auto flag = false;
		// 元に戻す操作スタックからヌルで区切られていない (選択などの) 操作を取り除く.
		while (m_stack_undo.empty() != true) {
			auto u = m_stack_undo.back();
			if (u == nullptr) {
				break;
			}
			m_stack_undo.pop_back();
			undo_exec(u);
			flag = true;
		}
		// やり直し操作スタックから操作を取り出し, 実行して, 元に戻す操作に積む.
		// 操作がヌルでないあいだこれを繰り返す.
		while (m_stack_redo.size() > 0) {
			auto r = m_stack_redo.back();
			m_stack_redo.pop_back();
			m_stack_undo.push_back(r);
			if (r == nullptr) {
				// 実行された操作があった場合, スタックの組数を変更する.
				m_stack_rcnt--;
				m_stack_ucnt++;
				break;
			}
			undo_exec(r);
			flag = true;
		}
		if (flag != true) {
			return;
		}
		// メニューや表示を更新する.
		xcvd_is_enabled();
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			//if (m_summary_visible) {
			summary_update();
		}
	}

	// 操作スタックを消去し, 含まれる操作を破棄する.
	void MainPage::undo_clear(void)
	{
		m_stack_updt = false;
		m_stack_rcnt -= undo_clear_stack(m_stack_redo);
		m_stack_ucnt -= undo_clear_stack(m_stack_undo);
#if defined(_DEBUG)
		if (m_stack_rcnt == 0 && m_stack_ucnt == 0) {
			return;
		}
		using winrt::Windows::UI::Xaml::Controls::ContentDialog;
		ContentDialog dialog;
		dialog.Title(box_value(L"Undo is not empty."));
		dialog.CloseButtonText(L"Close");
		auto _{ dialog.ShowAsync() };
#endif
	}

	// 編集メニューの「元に戻す」が選択された.
	void MainPage::undo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_stack_ucnt == 0) {
			return;
		}
		auto st = 0;
		while (m_stack_undo.empty() != true) {
			auto u = m_stack_undo.back();
			if (st == 0) {
				m_stack_undo.pop_back();
				if (u != nullptr) {
					undo_exec(u);
				}
				else {
					st = 1;
				}
				continue;
			}
			if (u == nullptr) {
				break;
			}
			m_stack_undo.pop_back();
			undo_exec(u);
			if (st == 1) {
				m_stack_redo.push_back(nullptr);
				st = 2;
			}
			m_stack_redo.push_back(u);
		}
		if (st == 2) {
			// 実行された操作があった場合, スタックの組数を変更する.
			m_stack_ucnt--;
			m_stack_rcnt++;
		}
		xcvd_is_enabled();
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			//if (m_summary_visible) {
			summary_update();
		}
	}

	// 操作を実行する.
	void MainPage::undo_exec(Undo* u)
	{
		summary_reflect(u);
		u->exec();
		auto const& u_type = typeid(*u);
		if (u_type == typeid(UndoAttr<UNDO_OP::ARROW_STYLE>)) {
			// 線枠メニューの「矢じるしの種類」に印をつける.
			ARROW_STYLE value;
			m_sheet_main.get_arrow_style(value);
			arrow_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::GRID_BASE>)) {
			// 方眼の大きさをステータスバーに格納する.
			status_set_grid();
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::GRID_EMPH>)) {
			// 用紙メニューの「方眼の強調」に印をつける.
			GRID_EMPH value;
			m_sheet_main.get_grid_emph(value);
			grid_emph_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::GRID_SHOW>)) {
			GRID_SHOW value;
			m_sheet_main.get_grid_show(value);
			grid_show_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::FONT_STYLE>)) {
			// 書体メニューの「字体」に印をつける.
			DWRITE_FONT_STYLE value;
			m_sheet_main.get_font_style(value);
			font_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::SHEET_SIZE>)) {
			// 用紙の大きさをステータスバーに格納する.
			status_set_sheet();
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::STROKE_CAP_STYLE>)) {
			CAP_STYLE value;
			m_sheet_main.get_stroke_cap_style(value);
			cap_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::STROKE_DASH_CAP>)) {
			D2D1_CAP_STYLE value;
			m_sheet_main.get_stroke_dash_cap(value);
			//cap_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::STROKE_DASH_STYLE>)) {
			D2D1_DASH_STYLE value;
			m_sheet_main.get_stroke_dash_style(value);
			stroke_dash_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::STROKE_JOIN_STYLE>)) {
			D2D1_LINE_JOIN value;
			m_sheet_main.get_stroke_join_style(value);
			join_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::TEXT_ALIGN_T>)) {
			DWRITE_TEXT_ALIGNMENT value;
			m_sheet_main.get_text_align_t(value);
			text_align_t_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::TEXT_ALIGN_P>)) {
			DWRITE_PARAGRAPH_ALIGNMENT value;
			m_sheet_main.get_text_align_p(value);
			text_align_p_is_checked(value);
		}
	}

	// 元に戻す/やり直しメニュー項目の使用の可否を設定する.
	void MainPage::undo_menu_enable(void)
	{
		mfi_undo().IsEnabled(m_stack_ucnt > 0);
		mfi_redo().IsEnabled(m_stack_rcnt > 0);
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

	// 図形の部位の位置をスタックに保存する.
	void MainPage::undo_push_anch(Shape* const s, const uint32_t anch)
	{
		m_stack_undo.push_back(new UndoAnchor(s, anch));
	}

	// 図形を追加して, その操作をスタックに積む.
	void MainPage::undo_push_append(Shape* s)
	{
		m_stack_undo.push_back(new UndoAppend(s));
	}

	// 図形をグループ図形に追加して, その操作をスタックに積む.
	// g	グループ図形
	// s	追加する図形
	// 戻り値	なし
	void MainPage::undo_push_append(ShapeGroup* g, Shape* s)
	{
		m_stack_undo.push_back(new UndoAppendG(g, s));
	}

	// 図形を入れ替えて, その操作をスタックに積む.
	void MainPage::undo_push_arrange(Shape* s, Shape* t)
	{
		m_stack_undo.push_back(new UndoArrange2(s, t));
	}

	// 図形を挿入して, その操作をスタックに積む.
	void MainPage::undo_push_insert(Shape* s, Shape* s_pos)
	{
		m_stack_undo.push_back(new UndoInsert(s, s_pos));
	}

	// 図形の位置をスタックに保存してから差分だけ移動する.
	// d_vec	移動させる差分
	// all	すべての図形の場合 true, 選択された図形の場合 false
	void MainPage::undo_push_move(const D2D1_POINT_2F d_vec, const bool all)
	{
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (all != true && s->is_selected() != true) {
				continue;
			}
			undo_push_set<UNDO_OP::START_POS>(s);
			s->move(d_vec);
		}
	}

	// 一連の操作の区切としてヌル操作をスタックに積む.
	// やり直し操作スタックは消去される.
	void MainPage::undo_push_null(void)
	{
		// やり直し操作スタックを消去し, 消去された操作の組数を, 操作の組数から引く.
		m_stack_rcnt -= undo_clear_stack(m_stack_redo);
		// 元に戻す操作スタックにヌルを積む.
		m_stack_undo.push_back(nullptr);
		// 操作スタックの更新フラグを立てる.
		m_stack_updt = true;
		// 操作の組数をインクリメントする.
		//m_stack_nset++;
		m_stack_ucnt++;
		if (m_stack_ucnt <= MAX_UCNT) {
		//if (m_stack_nset <= MAX_UCNT) {
			// 操作の組数が最大の組数以下の場合,
			// 終了する.
			return;
		}
		while (m_stack_undo.empty() != true) {
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
				//it++;
			}
else {
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
}
		// 図形の選択を反転して, その操作をスタックに積む.
		m_stack_undo.push_back(new UndoSelect(s));
	}

	// 値を図形へ格納して, その操作をスタックに積む.
	// ただし, 図形がその値を持たない場合, またはすでに同値の場合は何もしない.
	// s	図形
	// value	値
	// 戻り値	なし
	template <UNDO_OP U, typename T> void MainPage::undo_push_set(Shape* const s, T const& value)
	{
		// 図形がその値を持たない場合, またはすでに同値の場合,
		T t_value;
		if (UndoAttr<U>::GET(s, t_value) != true || equal(t_value, value)) {
			// 終了する.
			return;
		}
		m_stack_undo.push_back(new UndoAttr<U>(s, value));
	}

	// 値を選択された図形に格納して, その操作をスタックに積む.
	// U	操作の種類.
	// T	格納する型.
	// value	格納する値
	// 格納する型 T は明示しなくても引数の型から推定できる
	template<UNDO_OP U, typename T> bool MainPage::undo_push_set(T const& value)
	{
		m_stack_undo.push_back(new UndoAttr<U>(&m_sheet_main, value));
		auto flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() != true) {
				continue;
			}
			undo_push_set<U>(s, value);
			flag = true;
		}
		return flag;
	}

	// 図形の値の保存を実行して, その操作をスタックに積む.
	template <UNDO_OP U> void MainPage::undo_push_set(Shape* const s)
	{
		m_stack_undo.push_back(new UndoAttr<U>(s));
	}

	template bool MainPage::undo_push_set<UNDO_OP::ARROW_SIZE>(ARROW_SIZE const& value);
	template bool MainPage::undo_push_set<UNDO_OP::ARROW_STYLE>(ARROW_STYLE const& value);
	template bool MainPage::undo_push_set<UNDO_OP::FILL_COLOR>(D2D1_COLOR_F const& value);
	template bool MainPage::undo_push_set<UNDO_OP::FONT_COLOR>(D2D1_COLOR_F const& value);
	template bool MainPage::undo_push_set<UNDO_OP::FONT_FAMILY>(wchar_t* const& value);
	template bool MainPage::undo_push_set<UNDO_OP::FONT_SIZE>(float const& value);
	template bool MainPage::undo_push_set<UNDO_OP::FONT_STRETCH>(DWRITE_FONT_STRETCH const& value);
	template bool MainPage::undo_push_set<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE const& value);
	template bool MainPage::undo_push_set<UNDO_OP::FONT_WEIGHT>(DWRITE_FONT_WEIGHT const& value);
	template void MainPage::undo_push_set<UNDO_OP::GRID_BASE>(Shape* const s, float const& value);
	template void MainPage::undo_push_set<UNDO_OP::GRID_GRAY>(Shape* const s, float const& value);
	template void MainPage::undo_push_set<UNDO_OP::GRID_EMPH>(Shape* const s, GRID_EMPH const& value);
	template void MainPage::undo_push_set<UNDO_OP::GRID_SHOW>(Shape* const s, GRID_SHOW const& value);
	template void MainPage::undo_push_set<UNDO_OP::SHEET_COLOR>(Shape* const s, D2D1_COLOR_F const& value);
	template void MainPage::undo_push_set<UNDO_OP::SHEET_SIZE>(Shape* const s, D2D1_SIZE_F const& value);
	template void MainPage::undo_push_set<UNDO_OP::START_POS>(Shape* const s);
	template bool MainPage::undo_push_set<UNDO_OP::STROKE_CAP_STYLE>(CAP_STYLE const& value);
	template bool MainPage::undo_push_set<UNDO_OP::STROKE_COLOR>(D2D1_COLOR_F const& value);
	template bool MainPage::undo_push_set<UNDO_OP::STROKE_DASH_CAP>(D2D1_CAP_STYLE const& value);
	template bool MainPage::undo_push_set<UNDO_OP::STROKE_DASH_PATT>(STROKE_DASH_PATT const& value);
	template bool MainPage::undo_push_set<UNDO_OP::STROKE_DASH_STYLE>(D2D1_DASH_STYLE const& value);
	template bool MainPage::undo_push_set<UNDO_OP::STROKE_JOIN_LIMIT>(float const& value);
	template bool MainPage::undo_push_set<UNDO_OP::STROKE_JOIN_STYLE>(D2D1_LINE_JOIN const& value);
	template bool MainPage::undo_push_set<UNDO_OP::STROKE_WIDTH>(float const& value);
	template bool MainPage::undo_push_set<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT const& value);
	template bool MainPage::undo_push_set<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT const& value);
	template void MainPage::undo_push_set<UNDO_OP::TEXT_CONTENT>(Shape* const s, wchar_t* const& value);
	template bool MainPage::undo_push_set<UNDO_OP::TEXT_LINE_H>(float const& value);
	template bool MainPage::undo_push_set<UNDO_OP::TEXT_MARGIN>(D2D1_SIZE_F const& value);

	// 文字範囲の値を図形に格納して, その操作をスタックに積む.
	// スタックの一番上の操作の組の中に、すでに文字範囲の操作が積まれている場合,
	// その値が書き換えられる.
	// そうでない場合, 新たな操作としてスタックに積む.
	// s	操作する図形
	// value	文字範囲の値
	// 戻り値	なし
	template<> void MainPage::undo_push_set<UNDO_OP::TEXT_RANGE, DWRITE_TEXT_RANGE>(Shape* const s, DWRITE_TEXT_RANGE const& value)
	{
		auto flag = false;
		// 元に戻す操作スタックの各操作について
		for (auto it = m_stack_undo.rbegin(); it != m_stack_undo.rend(); it++) {
			const auto u = *it;
			if (u == nullptr) {
				// 操作がヌルの場合,
				break;
			}
			else if (typeid(*u) != typeid(UndoAttr<UNDO_OP::TEXT_RANGE>)) {
				// 操作が文字範囲の選択する操作でない場合,
				if (typeid(*u) != typeid(UndoSelect)) {
					// 操作が図形の選択を反転する操作でない場合,
					// 中断する.
					break;
				}
			}
			// 操作が文字範囲の選択する操作の場合,
			else if (u->m_shape == s) {
				// 操作する図形が引数の図形と同じ場合,
				// 図形を直接操作してスタックには積まないようにする.
				s->set_text_range(value);
				flag = true;
				break;
			}
		}
		if (flag != true) {
			m_stack_undo.push_back(new UndoAttr<UNDO_OP::TEXT_RANGE>(s, value));
		}

	}

	// データリーダーから操作スタックを読み込む.
	void MainPage::undo_read(DataReader const& dt_reader)
	{
		Undo* r;
		while (undo_read_op(r, dt_reader)) {
			m_stack_redo.push_back(r);
		}
		m_stack_rcnt = dt_reader.ReadUInt32();
		Undo* u;
		while (undo_read_op(u, dt_reader)) {
			m_stack_undo.push_back(u);
		}
		m_stack_ucnt = dt_reader.ReadUInt32();
		m_stack_updt = dt_reader.ReadBoolean();
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
		dt_writer.WriteUInt32(m_stack_ucnt);
		dt_writer.WriteBoolean(m_stack_updt);
	}

}