//-------------------------------
// MainPage_pointer.cpp
// ポインターのイベントハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Core::CoreCursorType;

	static auto const& CUR_ARROW = CoreCursor(CoreCursorType::Arrow, 0);	// 矢印カーソル
	static auto const& CUR_CROSS = CoreCursor(CoreCursorType::Cross, 0);	// 十字カーソル
	static auto const& CUR_SIZEALL = CoreCursor(CoreCursorType::SizeAll, 0);	// 移動カーソル
	static auto const& CUR_SIZENESW = CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);	// 右上左下カーソル
	static auto const& CUR_SIZENS = CoreCursor(CoreCursorType::SizeNorthSouth, 0);	// 上下カーソル
	static auto const& CUR_SIZENWSE = CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);	// 左上右下カーソル
	static auto const& CUR_SIZEWE = CoreCursor(CoreCursorType::SizeWestEast, 0);	// 左右カーソル

	// 消去フラグの立つ図形をリストから削除する.
	static void reduce_list(S_LIST_T& s_list, U_STACK_T const& u_stack, U_STACK_T const& r_stack);
	// 図形が操作スタックに含まれるか調べる.
	static bool refer_ro(U_STACK_T const& u_stack, Shape* const s) noexcept;

	// 消去フラグの立つ図形をリストから削除する.
	static void reduce_list(S_LIST_T& s_list, U_STACK_T const& u_stack, U_STACK_T const& r_stack)
	{
		// 消去フラグの立つ図形を消去リストに格納する.
		S_LIST_T list_deleted;
		for (const auto t : s_list) {
			if (t->is_deleted() != true) {
				// 消去フラグがない図形は無視する.
				continue;
			}
			if (refer_ro(u_stack, t)) {
				// 元に戻す操作スタックが参照する図形は無視する.
				continue;
			}
			if (refer_ro(r_stack, t)) {
				// やり直し操作スタックが参照する図形は無視する.
				continue;
			}
			// 上記のいずれでもない図形を消去リストに追加する.
			list_deleted.push_back(t);
		}
		// 消去リストに含まれる図形をリストから取り除き, 解放する.
		auto it_begin = s_list.begin();
		for (const auto s : list_deleted) {
			auto it = std::find(it_begin, s_list.end(), s);
			it_begin = s_list.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// 消去リストを消去する.
		list_deleted.clear();
	}

	// 操作スタックが図形を参照するか調べる.
	// u_stack	操作スタック
	// s	図形
	// 戻り値	参照する場合 true.
	static bool refer_ro(U_STACK_T const& u_stack, Shape* const s) noexcept
	{
		for (const auto u : u_stack) {
			if (u == nullptr) {
				continue;
			}
			if (u->refer_to(s)) {
				return true;
			}

		}
		return false;
	}

	// コンテキストメニューを表示する.
	void MainPage::pointer_context_menu(void)
	{
		scp_page_panel().ContextFlyout(nullptr);
		if (m_pointer_shape == nullptr
			|| m_pointer_anchor == ANCH_WHICH::ANCH_OUTSIDE) {
			// 押された図形がヌル, 
			// または押された部位は外側の場合,
			// ページコンテキストメニューを表示する.
			//scp_page_panel().ContextFlyout(nullptr);
			scp_page_panel().ContextFlyout(m_menu_layout);
		}
		else if (typeid(*m_pointer_shape) == typeid(ShapeGroup)) {
			// 押された図形がグループの場合,
			//scp_page_panel().ContextFlyout(nullptr);
			scp_page_panel().ContextFlyout(m_menu_ungroup);
		}
		else {
			// 押された図形の属性値をページレイアウトに格納する.
			m_page_layout.set_to(m_pointer_shape);
			if (m_pointer_anchor == ANCH_WHICH::ANCH_INSIDE) {
				// 押された図形の部位が内側の場合,
				// 塗りつぶしコンテキストメニューを表示する.
				//scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_fill);
			}
			else if (m_pointer_anchor == ANCH_WHICH::ANCH_TEXT) {
				// 押された図形の部位が文字列の場合,
				// 書体コンテキストメニューを表示する.
				//scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_font);
			}
			else if (m_pointer_anchor == ANCH_WHICH::ANCH_FRAME) {
				// 押された図形の部位が枠上の場合,
				// 線枠コンテキストメニューを表示する.
				//scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_stroke);
			}
		}
	}

	// ポインターの現在位置を得る.
	void MainPage::pointer_cur_pos(PointerRoutedEventArgs const& args)
	{
		// スワップチェーンパネル上でのポインターの位置を得て, 
		// ページ座標系に変換し, ポインターの現在位置に格納する.
		D2D1_POINT_2F p_offs;
		pt_add(page_min(), sb_horz().Value(), sb_vert().Value(), p_offs);
		pt_scale(args.GetCurrentPoint(scp_page_panel()).Position(), 1.0 / m_page_layout.m_page_scale, p_offs, m_pointer_cur);
	}

	// 文字列図形の作成を終了する.
	IAsyncAction MainPage::pointer_finish_creating_text_async(const D2D1_POINT_2F diff)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		tx_edit().Text(L"");
		ck_text_adjust_bound().IsChecked(text_adjust());
		if (co_await cd_edit_text().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit().Text().c_str());
			auto s = new ShapeText(m_pointer_pressed, diff, text, &m_page_layout);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			text_adjust(ck_text_adjust_bound().IsChecked().GetBoolean());
			if (text_adjust()) {
				static_cast<ShapeText*>(s)->adjust_bound();
			}
			reduce_list(m_list_shapes, m_stack_undo, m_stack_redo);
			unselect_all();
			undo_push_append(s);
			undo_push_null();
			m_pointer_shape_summary = m_pointer_shape_prev = s;
			enable_edit_menu();
			page_bound(s);
			page_panle_size();
			if (m_mutex_summary.load(std::memory_order_acquire)) {
			//if (m_summary_visible) {
				summary_append(s);
			}
		}
		// 初期状態に戻す.
		m_pointer_state = STATE_TRAN::BEGIN;
		m_pointer_shape = nullptr;
		m_pointer_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		page_draw();
	}

	// 図形の作成を終了する.
	void MainPage::pointer_finish_creating(const D2D1_POINT_2F diff)
	{
		const auto draw_tool = tool();
		Shape* s;
		if (draw_tool == DRAW_TOOL::RECT) {
			s = new ShapeRect(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::RRCT) {
			s = new ShapeRRect(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::QUAD) {
			s = new ShapeQuad(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::ELLI) {
			s = new ShapeElli(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::LINE) {
			s = new ShapeLine(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::BEZI) {
			s = new ShapeBezi(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::SCALE) {
			s = new ShapeScale(m_pointer_pressed, diff, &m_page_layout);
		}
		else {
			return;
		}
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		reduce_list(m_list_shapes, m_stack_undo, m_stack_redo);
		unselect_all();
		undo_push_append(s);
		undo_push_null();
		m_pointer_shape_summary = m_pointer_shape_prev = s;
		enable_edit_menu();
		page_bound(s);
		page_panle_size();
		page_draw();
		if (m_mutex_summary.load(std::memory_order_acquire)) {
		//if (m_summary_visible) {
			summary_append(s);
		}
	}

	// 図形の変形を終了する.
	void MainPage::pointer_finish_forming(void)
	{
		if (m_page_layout.m_grid_snap) {
			pt_round(m_pointer_cur, m_page_layout.m_grid_base + 1.0, m_pointer_cur);
		}
		m_pointer_shape->set_pos(m_pointer_cur, m_pointer_anchor);
		if (undo_pop_if_invalid()) {
			return;
		}
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		page_bound();
		page_panle_size();
	}

	// 図形の移動を終了する.
	void MainPage::pointer_finish_moving(void)
	{
		if (m_page_layout.m_grid_snap) {
			D2D1_POINT_2F p_min = {};
			bool flag = false;
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					continue;
				}
				if (s->is_selected() != true) {
					continue;
				}
				if (flag != true) {
					flag = true;
					s->get_min_pos(p_min);
					continue;
				}
				D2D1_POINT_2F nw_pos;
				s->get_min_pos(nw_pos);
				pt_min(nw_pos, p_min, p_min);
			}
			if (flag) {
				// 得た左上点を方眼の大きさで丸める.
				// 丸めの前後で生じた差を得る.
				D2D1_POINT_2F g_pos;
				pt_round(p_min, m_page_layout.m_grid_base + 1.0, g_pos);
				D2D1_POINT_2F diff;
				pt_sub(g_pos, p_min, diff);
				s_list_move(m_list_shapes, diff);
			}
		}
		if (undo_pop_if_invalid()) {
			return;
		}
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		page_bound();
		page_panle_size();
		enable_edit_menu();
	}

	// 範囲選択を終了する.
	void MainPage::pointer_finish_selecting_area(const VirtualKeyModifiers k_mod)
	{
		using winrt::Windows::UI::Xaml::Window;

		auto flag = false;
		if (k_mod == VirtualKeyModifiers::Control) {
			D2D1_POINT_2F a_min;
			D2D1_POINT_2F a_max;
			pt_bound(m_pointer_pressed, m_pointer_cur, a_min, a_max);
			flag = toggle_area(a_min, a_max);
		}
		else if (k_mod == VirtualKeyModifiers::None) {
			D2D1_POINT_2F a_min;
			D2D1_POINT_2F a_max;
			pt_bound(m_pointer_pressed, m_pointer_cur, a_min, a_max);
			flag = select_area(a_min, a_max);
		}
		if (flag == true) {
			// 編集メニュー項目の使用の可否を設定する.
			enable_edit_menu();
		}
		Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
	}

	// 状況に応じた形状のカーソルを設定する.
	void MainPage::pointer_set(void)
	{
		if (tool() != DRAW_TOOL::SELECT) {
			Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
			return;
		}
		if (m_mutex_page.try_lock() != true) {
		//if (m_mutex_page.load()) {
			// ロックできない場合
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
			return;
		}
		Shape* s;
		const auto a = s_list_hit_test(m_list_shapes, m_pointer_cur, page_anch_len(), s);
		if (a == ANCH_WHICH::ANCH_OUTSIDE || s->is_selected() != true) {
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
			m_mutex_page.unlock();
			return;
		}
		//if (a != ANCH_WHICH::ANCH_OUTSIDE && s->is_selected()) {
		switch (a) {
		case ANCH_WHICH::ANCH_R_NW:
		case ANCH_WHICH::ANCH_R_NE:
		case ANCH_WHICH::ANCH_R_SE:
		case ANCH_WHICH::ANCH_R_SW:
			Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
			break;
		case ANCH_WHICH::ANCH_INSIDE:
		case ANCH_WHICH::ANCH_FRAME:
		case ANCH_WHICH::ANCH_TEXT:
			Window::Current().CoreWindow().PointerCursor(CUR_SIZEALL);
			break;
		case ANCH_WHICH::ANCH_NE:
		case ANCH_WHICH::ANCH_SW:
			Window::Current().CoreWindow().PointerCursor(CUR_SIZENESW);
			break;
		case ANCH_WHICH::ANCH_NORTH:
		case ANCH_WHICH::ANCH_SOUTH:
			Window::Current().CoreWindow().PointerCursor(CUR_SIZENS);
			break;
		case ANCH_WHICH::ANCH_NW:
		case ANCH_WHICH::ANCH_SE:
			Window::Current().CoreWindow().PointerCursor(CUR_SIZENWSE);
			break;
		case ANCH_WHICH::ANCH_WEST:
		case ANCH_WHICH::ANCH_EAST:
			Window::Current().CoreWindow().PointerCursor(CUR_SIZEWE);
			break;
		}
		m_mutex_page.unlock();
		return;
		//}
		//Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
	}

	// ポインターのボタンが上げられた.
	void MainPage::pointer_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		pointer_released(sender, args);
	}

	// ポインターがページのスワップチェーンパネルの中に入った.
	void MainPage::pointer_entered(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		if (sender != scp_page_panel()) {
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
			return;
		}
		pointer_cur_pos(args);
		pointer_set();
		stbar_set_curs();
	}

	// ポインターがページのスワップチェーンパネルから出た.
	void MainPage::pointer_exited(IInspectable const& sender, PointerRoutedEventArgs const&)
	{
		if (sender != scp_page_panel()) {
			return;
		}
		auto const& c_win = Window::Current().CoreWindow();
		auto const& cur = c_win.PointerCursor();
		if (cur.Type() == CUR_ARROW.Type()) {
			return;
		}
		c_win.PointerCursor(CUR_ARROW);
	}

	// ポインターが動いた.
	void MainPage::pointer_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::UI::Xaml::Window;
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		pointer_cur_pos(args);
		stbar_set_curs();
		if (m_pointer_state == STATE_TRAN::BEGIN) {
			// 状態が初期状態の場合,
			pointer_set();
		}
		else if (m_pointer_state == STATE_TRAN::CLICK) {
			// 状態がクリックした状態の場合,
			// ポインターの現在位置と押された位置の長さを得る.
			D2D1_POINT_2F diff;
			pt_sub(m_pointer_cur, m_pointer_pressed, diff);
			if (pt_abs2(diff) > m_pointer_click_dist) {
				// 長さが閾値を超える場合, 初期状態に戻る.
				m_pointer_state = STATE_TRAN::BEGIN;
				pointer_set();
			}
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_AREA) {
			// 状態が範囲を選択している状態の場合,
			page_draw();
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_MOVE) {
			// 状態が図形を移動している状態の場合,
			// ポインターの現在位置と前回位置の差分を得る.
			D2D1_POINT_2F diff;
			pt_sub(m_pointer_cur, m_pointer_pre, diff);
			s_list_move(m_list_shapes, diff);
			// ポインターの現在位置を前回位置に格納する.
			m_pointer_pre = m_pointer_cur;
			page_draw();
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_FORM) {
			// 状態が図形を変形している状態の場合,
			// ポインターの現在位置を押された図形の部位の位置に格納する.
			m_pointer_shape->set_pos(m_pointer_cur, m_pointer_anchor);
			// ポインターの現在位置を前回位置に格納する.
			m_pointer_pre = m_pointer_cur;
			page_draw();
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_L
			|| m_pointer_state == STATE_TRAN::CLICK_2) {
			// 状態が左ボタンを押している状態,
			// またはクリック後に左ボタンを押している状態の場合,
			// ポインターの現在位置と押された位置の長さを得る.
			D2D1_POINT_2F diff;
			pt_sub(m_pointer_cur, m_pointer_pressed, diff);
			if (pt_abs2(diff) > m_pointer_click_dist) {
				// 長さが閾値を超える場合,
				if (tool() != DRAW_TOOL::SELECT) {
					// 作図ツールが選択ツールでない場合,
					// 範囲を選択している状態に遷移する.
					m_pointer_state = STATE_TRAN::PRESS_AREA;
				}
				else if (m_pointer_shape == nullptr) {
					// 押された図形がない場合,
					// 範囲を選択している状態に遷移する.
					m_pointer_state = STATE_TRAN::PRESS_AREA;
					// 十字カーソルをカーソルに設定する.
					Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
				}
				else if (m_list_selected > 1
					|| m_pointer_anchor == ANCH_WHICH::ANCH_FRAME
					|| m_pointer_anchor == ANCH_WHICH::ANCH_INSIDE
					|| m_pointer_anchor == ANCH_WHICH::ANCH_TEXT) {
					// 選択された図形の数が 1 を超える
					// または押された図形の部位が線枠
					// または押された図形の部位が内側
					// または押された図形の部位が文字列
					m_pointer_state = STATE_TRAN::PRESS_MOVE;
					// ポインターの現在位置を前回位置に格納する.
					m_pointer_pre = m_pointer_cur;
					undo_push_move(diff);
				}
				else if (m_pointer_anchor != ANCH_WHICH::ANCH_OUTSIDE) {
					// 押された図形の部位が図形の外部でない場合,
					// 図形を変形している状態に遷移する.
					m_pointer_state = STATE_TRAN::PRESS_FORM;
					m_pointer_pre = m_pointer_cur;
					undo_push_anchor(m_pointer_shape, m_pointer_anchor);
					m_pointer_shape->set_pos(m_pointer_cur, m_pointer_anchor);
				}
				page_draw();
			}
		}
	}

	// ポインターのボタンが押された.
	// キャプチャの有無にかかわらず, 片方のマウスボタンを押した状態で, もう一方のボタンを押しても, それは通知されない.
	void MainPage::pointer_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
		using winrt::Windows::UI::Input::PointerPointProperties;
		using winrt::Windows::Devices::Input::PointerDeviceType;

#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		auto const& scp = sender.as<SwapChainPanel>();
		// ポインターのキャプチャを始める.
		scp.CapturePointer(args.Pointer());
		// ポインターのイベント発生時間を得る.
		auto t_stamp = args.GetCurrentPoint(scp).Timestamp();
		pointer_cur_pos(args);
		// ポインターのプロパティーを得る.
		auto const& p_prop = args.GetCurrentPoint(scp).Properties();
		switch (args.GetCurrentPoint(scp).PointerDevice().PointerDeviceType()) {
		case PointerDeviceType::Mouse:
			// ポインターのデバイスタイプがマウスの場合
			if (p_prop.IsRightButtonPressed()) {
				// プロパティーが右ボタン押下の場合,
				m_pointer_state = STATE_TRAN::PRESS_R;
			}
			else if (p_prop.IsLeftButtonPressed()) {
				// プロパティーが左ボタン押下の場合,
		case PointerDeviceType::Pen:
		case PointerDeviceType::Touch:
			// ポインターのデバイスタイプがペンまたはタッチの場合
				switch (m_pointer_state) {
				case STATE_TRAN::CLICK:
					// ポインターが押された状態がクリックした状態の場合,
					if (t_stamp - m_pointer_time <= m_pointer_click_time) {
						m_pointer_state = STATE_TRAN::CLICK_2;
					}
					else {
				case STATE_TRAN::BEGIN:
				default:
					// ポインターが押された状態がクリックした状態の場合,
					m_pointer_state = STATE_TRAN::PRESS_L;
					}
				}
				break;
			}
			else {
		default:
				// ポインターのデバイスタイプがそれ以外の場合
				m_pointer_state = STATE_TRAN::BEGIN;
				return;
			}
		}
		m_pointer_time = t_stamp;
		m_pointer_pressed = m_pointer_cur;
		if (tool() != DRAW_TOOL::SELECT) {
			// 作図ツールが選択ツールでない場合,
			return;
		}
		m_pointer_anchor = s_list_hit_test(m_list_shapes, m_pointer_pressed, page_anch_len(), m_pointer_shape);
		if (m_pointer_anchor != ANCH_WHICH::ANCH_OUTSIDE) {
			// 図形とその部位を得た場合,
			if (m_pointer_state == STATE_TRAN::PRESS_L
				|| (m_pointer_state == STATE_TRAN::PRESS_R && m_pointer_shape->is_selected() != true)) {
				m_pointer_shape_summary = m_pointer_shape;
				select_shape(m_pointer_shape, args.KeyModifiers());
			}
			return;
		}
		// ヌルを, ポインターが押された図形と前回ポインターが押された図形, 一覧でポインターが押された図形に格納する.
		// ANCH_OUTSIDE をポインターが押された部位に格納する.
		m_pointer_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		m_pointer_shape = nullptr;
		m_pointer_shape_prev = nullptr;
		m_pointer_shape_summary = nullptr;
		// キー修飾子をハンドラーの引数から得る.
		if (args.KeyModifiers() != VirtualKeyModifiers::None) {
			// キー修飾子が None でない場合
			return;
		}
		if (unselect_all() != true) {
			// 選択が解除された図形がない場合
			return;
		}
		enable_edit_menu();
		page_draw();
	}

	// ポインターのボタンが上げられた.
	void MainPage::pointer_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif
		auto const& scp = sender.as<SwapChainPanel>();
		// ポインターの追跡を停止する.
		scp.ReleasePointerCaptures();
		pointer_cur_pos(args);
		if (m_pointer_state == STATE_TRAN::PRESS_L) {
			// 左ボタンが押された状態の場合,
			// ボタンが離れた時刻と押された時刻の差分を得る.
			const auto t_stamp = args.GetCurrentPoint(scp).Timestamp();
			if (t_stamp - m_pointer_time <= m_pointer_click_time) {
				// 差分がクリックの判定時間以下の場合,
				// クリックした状態に遷移する.
				m_pointer_state = STATE_TRAN::CLICK;
				pointer_set();
				return;
			}
		}
		else if (m_pointer_state == STATE_TRAN::CLICK_2) {
			// クリック後に左ボタンが押した状態の場合,
			// ボタンが離された時刻と押された時刻の差分を得る.
			const auto t_stamp = args.GetCurrentPoint(scp).Timestamp();
			if (t_stamp - m_pointer_time <= m_pointer_click_time) {
				// 差分がクリックの判定時間以下で
				if (m_pointer_shape != nullptr && typeid(*m_pointer_shape) == typeid(ShapeText)) {
					// 押された図形が文字列図形の場合, 
					text_edit_async(static_cast<ShapeText*>(m_pointer_shape));
				}
			}
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_MOVE) {
			// 状態が図形を移動している状態の場合,
			// 図形の移動を終了する.
			pointer_finish_moving();
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_FORM) {
			// 状態が図形を変形している状態の場合,
			// 図形の変形を終了する.
			pointer_finish_forming();
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_AREA) {
			// 状態が範囲選択している状態の場合,
			if (tool() == DRAW_TOOL::SELECT) {
				// 作図ツールが選択ツールの場合,
				pointer_finish_selecting_area(args.KeyModifiers());
			}
			else {
				if (m_page_layout.m_grid_snap) {
					// 方眼に整列の場合, 始点と終点を方眼の大きさで丸める
					double g = max(m_page_layout.m_grid_base + 1.0, 1.0);
					pt_round(m_pointer_pressed, g, m_pointer_pressed);
					pt_round(m_pointer_cur, g, m_pointer_cur);
				}
				// ポインターの現在の位置と押された位置の差分を求める.
				D2D1_POINT_2F diff;
				pt_sub(m_pointer_cur, m_pointer_pressed, diff);
				if (fabs(diff.x) >= 1.0f || fabs(diff.y) >= 1.0f) {
					if (tool() == DRAW_TOOL::TEXT) {
						pointer_finish_creating_text_async(diff);
						return;
					}
					pointer_finish_creating(diff);
				}
			}
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_R) {
			// 状態が右ボタンを押した状態の場合
			pointer_context_menu();
		}
		else if (m_pointer_state == STATE_TRAN::BEGIN) {
			// 状態が初期状態の場合,
			// 本来は初期状態でこのハンドラーが呼び出されるはずはないが,
			// コンテンツダイアログを終了したとき呼び出されてしまう.
			return;
		}
		// 初期状態に戻す.
		m_pointer_state = STATE_TRAN::BEGIN;
		m_pointer_shape = nullptr;
		m_pointer_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		page_draw();
	}

	// ポインターのホイールボタンが操作された.
	void MainPage::pointer_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
		using winrt::Windows::UI::Xaml::UIElement;
		using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;

#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		int32_t delta = args.GetCurrentPoint(scp_page_panel()).Properties().MouseWheelDelta();
		if (args.KeyModifiers() == VirtualKeyModifiers::Control) {
			// コントロールキーが押されていた場合, ページを拡大縮小する.
			if (delta > 0) {
				mfi_zoom_in_clicked(nullptr, nullptr);
			}
			else if (delta < 0) {
				mfi_zoom_out_clicked(nullptr, nullptr);
			}
		}
		else {
			ScrollBar stbar;
			if (args.KeyModifiers() == VirtualKeyModifiers::Shift) {
				stbar = sb_horz();
			}
			else if (args.KeyModifiers() == VirtualKeyModifiers::None) {
				stbar = sb_vert();
			}
			else {
				return;
			}
			// シフトキーが押されていた場合, 横スクロールする.
			double value = stbar.Value();
			double limit = 0.0;
			if (delta < 0 && value < (limit = stbar.Maximum())) {
				value = min(value + 32.0 * m_page_layout.m_page_scale, limit);
			}
			else if (delta > 0 && value > (limit = stbar.Minimum())) {
				value = max(value - 32.0 * m_page_layout.m_page_scale, limit);
			}
			else {
				return;
			}
			stbar.Value(value);
			page_draw();
		}
	}

}