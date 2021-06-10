//-------------------------------
// MainPage_event.cpp
// ポインターのイベントハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Core::CoreCursorType;
	using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;

	static auto const& CC_ARROW = CoreCursor(CoreCursorType::Arrow, 0);	// 矢印カーソル
	static auto const& CC_CROSS = CoreCursor(CoreCursorType::Cross, 0);	// 十字カーソル
	static auto const& CC_SIZE_ALL = CoreCursor(CoreCursorType::SizeAll, 0);	// 移動カーソル
	static auto const& CC_SIZE_NESW = CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);	// 右上左下カーソル
	static auto const& CC_SIZE_NS = CoreCursor(CoreCursorType::SizeNorthSouth, 0);	// 上下カーソル
	static auto const& CC_SIZE_NWSE = CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);	// 左上右下カーソル
	static auto const& CC_SIZE_WE = CoreCursor(CoreCursorType::SizeWestEast, 0);	// 左右カーソル

	// 図形が操作スタックに含まれるか判定する.
	static bool event_if_shape_on_stack(UNDO_STACK const& u_stack, Shape* const s) noexcept;
	// 消去フラグの立つ図形をリストから削除する.
	static void event_reduce_slist(SHAPE_LIST& slist, UNDO_STACK const& u_stack, UNDO_STACK const& r_stack);
	// マウスホイールの値でスクロールする.
	static bool event_scroll_by_wheel_delta(const ScrollBar& sb, const int32_t delta, const float scale);

	// 操作スタックが図形を参照するか判定する.
	// u_stack	操作スタック
	// s	図形
	// 戻り値	参照する場合 true.
	static bool event_if_shape_on_stack(UNDO_STACK const& u_stack, Shape* const s) noexcept
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

	// 消去フラグの立つ図形をリストから削除する.
	static void event_reduce_slist(SHAPE_LIST& slist, UNDO_STACK const& u_stack, UNDO_STACK const& r_stack)
	{
		// 消去フラグの立つ図形を消去リストに格納する.
		SHAPE_LIST list_deleted;
		for (const auto t : slist) {
			// 図形の消去フラグがないか判定する.
			if (!t->is_deleted()) {
				continue;
			}
			// 図形が元に戻す操作スタックに含まれる判定する.
			if (event_if_shape_on_stack(u_stack, t)) {
				continue;
			}
			// 図形がやり直し操作スタックに含まれる判定する.
			if (event_if_shape_on_stack(r_stack, t)) {
				continue;
			}
			// 上記のいずれでもない図形を消去リストに追加する.
			list_deleted.push_back(t);
		}
		// 消去リストに含まれる図形をリストから取り除き, 解放する.
		auto it_begin = slist.begin();
		for (const auto s : list_deleted) {
			const auto it = std::find(it_begin, slist.end(), s);
			it_begin = slist.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// 消去リストを消去する.
		list_deleted.clear();
	}

	// マウスホイールの値でスクロールする.
	static bool event_scroll_by_wheel_delta(const winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar& sb, const int32_t delta, const float scale)
	{
		double value = sb.Value();
		double limit = 0.0;
		if (delta < 0 && value < (limit = sb.Maximum())) {
			value = min(value + 32.0 * scale, limit);
		}
		else if (delta > 0 && value > (limit = sb.Minimum())) {
			value = max(value - 32.0 * scale, limit);
		}
		else {
			return false;
		}
		sb.Value(value);
		return true;
	}

	// ポインターのボタンが上げられた.
	void MainPage::event_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		event_released(sender, args);
	}

	// コンテキストメニューを表示する.
	void MainPage::event_show_context_menu(void)
	{
		// コンテキストメニューを解放する.
		scp_sheet_panel().ContextFlyout(nullptr);
		// 押された図形がヌル, または押された図形の部位が外側か判定する.
		if (m_event_shape_pressed == nullptr || m_event_anch_pressed == ANCH_TYPE::ANCH_SHEET) {
			scp_sheet_panel().ContextFlyout(m_sheet_menu);
		}
		// 押された図形がグループ図形か判定する.
		else if (typeid(*m_event_shape_pressed) == typeid(ShapeGroup)) {
			scp_sheet_panel().ContextFlyout(m_ungroup_menu);
		}
		else if (typeid(*m_event_shape_pressed) == typeid(ShapeRuler)) {
			scp_sheet_panel().ContextFlyout(m_ruler_menu);
		}
		else {
			// 押された図形の属性値を用紙に格納する.
			m_sheet_main.set_attr_to(m_event_shape_pressed);
			sheet_attr_is_checked();
			// 押された図形の部位が内側か判定する.
			if (m_event_anch_pressed == ANCH_TYPE::ANCH_FILL) {
				scp_sheet_panel().ContextFlyout(m_fill_menu);
			}
			// 押された図形の部位が文字列か判定する.
			else if (m_event_anch_pressed == ANCH_TYPE::ANCH_TEXT) {
				scp_sheet_panel().ContextFlyout(m_font_menu);
			}
			// 押された図形の部位が線枠か判定する.
			else if (m_event_anch_pressed == ANCH_TYPE::ANCH_STROKE) {
				scp_sheet_panel().ContextFlyout(m_stroke_menu);
			}
		}
	}

	// ポインターの現在位置を得る.
	void MainPage::event_pos_args(PointerRoutedEventArgs const& args)
	{
		// スワップチェーンパネル上でのポインターの位置を得て, 
		// 用紙座標系に変換し, ポインターの現在位置に格納する.
		D2D1_POINT_2F p_offs;
		pt_add(m_sheet_min, sb_horz().Value(), sb_vert().Value(), p_offs);
		pt_mul(args.GetCurrentPoint(scp_sheet_panel()).Position(), 1.0 / m_sheet_main.m_sheet_scale, p_offs, m_event_pos_curr);
	}

	// ポインターが用紙のスワップチェーンパネルの中に入った.
	void MainPage::event_entered(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		if (sender != scp_sheet_panel()) {
			Window::Current().CoreWindow().PointerCursor(CC_ARROW);
			return;
		}
		event_pos_args(args);
		event_curs_style();
		status_bar_set_curs();
	}

	// ポインターが用紙のスワップチェーンパネルから出た.
	void MainPage::event_exited(IInspectable const& sender, PointerRoutedEventArgs const&)
	{
		if (sender != scp_sheet_panel()) {
			return;
		}
		auto const& c_win = Window::Current().CoreWindow();
		auto const& cur = c_win.PointerCursor();
		if (cur.Type() == CC_ARROW.Type()) {
			return;
		}
		c_win.PointerCursor(CC_ARROW);
	}

	// 図形の作成を終了する.
	void MainPage::event_finish_creating(const D2D1_POINT_2F diff)
	{
		const auto t_draw = m_tool_draw;
		Shape* s;
		if (t_draw == DRAW_TOOL::RECT) {
			s = new ShapeRect(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else if (t_draw == DRAW_TOOL::RRECT) {
			s = new ShapeRRect(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else if (t_draw == DRAW_TOOL::POLY) {
			s = new ShapePoly(m_event_pos_pressed, diff, &m_sheet_main, m_tool_poly);
		}
		else if (t_draw == DRAW_TOOL::ELLI) {
			s = new ShapeElli(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else if (t_draw == DRAW_TOOL::LINE) {
			s = new ShapeLineA(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else if (t_draw == DRAW_TOOL::BEZI) {
			s = new ShapeBezi(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else if (t_draw == DRAW_TOOL::RULER) {
			s = new ShapeRuler(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else {
			return;
		}
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		event_reduce_slist(m_list_shapes, m_stack_undo, m_stack_redo);
		unselect_all();
		undo_push_append(s);
		undo_push_select(s);
		undo_push_null();
		//m_event_shape_summary = 
		m_event_shape_prev = s;
		xcvd_is_enabled();
		sheet_update_bbox(s);
		sheet_panle_size();
		sheet_draw();
		// 図形一覧の排他制御が true か判定する.
		if (m_summary_atomic.load(std::memory_order_acquire)) {
			summary_append(s);
			summary_select(s);
		}
	}

	// 文字列図形の作成を終了する.
	IAsyncAction MainPage::event_finish_creating_text_async(const D2D1_POINT_2F diff)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		tx_edit().Text(L"");
		ck_edit_text_frame().IsChecked(m_edit_text_frame);
		if (co_await cd_edit_text_dialog().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit().Text().c_str());
			auto s = new ShapeText(m_event_pos_pressed, diff, text, &m_sheet_main);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			m_edit_text_frame = ck_edit_text_frame().IsChecked().GetBoolean();
			if (m_edit_text_frame) {
				static_cast<ShapeText*>(s)->adjust_bbox(m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0f : 0.0f);
			}
			event_reduce_slist(m_list_shapes, m_stack_undo, m_stack_redo);
			unselect_all();
			undo_push_append(s);
			undo_push_select(s);
			undo_push_null();
			//m_event_shape_summary =
			m_event_shape_prev = s;
			xcvd_is_enabled();
			sheet_update_bbox(s);
			sheet_panle_size();
			// 図形一覧の排他制御が true か判定する.
			if (m_summary_atomic.load(std::memory_order_acquire)) {
				summary_append(s);
				summary_select(s);
			}
		}
		// ポインターの押された状態を初期状態に戻す.
		m_event_state = EVENT_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
		sheet_draw();
	}

	// 図形の変形を終了する.
	void MainPage::event_finish_forming(void)
	{
		bool g_snap;
		m_sheet_main.get_grid_snap(g_snap);
		if (g_snap) {
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			pt_round(m_event_pos_curr, g_base + 1.0, m_event_pos_curr);
		}
		m_event_shape_pressed->set_anchor_pos(m_event_pos_curr, m_event_anch_pressed);
		if (undo_pop_if_invalid()) {
			return;
		}
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		sheet_update_bbox();
		sheet_panle_size();
	}

	// 図形の移動を終了する.
	void MainPage::event_finish_moving(void)
	{
		bool g_snap;
		m_sheet_main.get_grid_snap(g_snap);
		if (g_snap) {
			D2D1_POINT_2F b_nw{};
			D2D1_POINT_2F b_se{};
			bool flag = false;
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					continue;
				}
				if (s->is_selected() != true) {
					continue;
				}
				if (!flag) {
					flag = true;
					s->get_bound({ FLT_MAX, FLT_MAX }, { -FLT_MAX, -FLT_MAX }, b_nw, b_se);
				}
				else {
					s->get_bound(b_nw, b_se, b_nw, b_se);
				}
			}
			if (flag) {
				float g_base;
				m_sheet_main.get_grid_base(g_base);
				const double g_len = g_base + 1.0;
				D2D1_POINT_2F b_ne{ b_se.x, b_nw.y };
				D2D1_POINT_2F b_sw{ b_nw.x, b_se.y };

				D2D1_POINT_2F g_nw;
				D2D1_POINT_2F g_se;
				D2D1_POINT_2F g_ne;
				D2D1_POINT_2F g_sw;
				pt_round(b_nw, g_len, g_nw);
				pt_round(b_se, g_len, g_se);
				pt_round(b_ne, g_len, g_ne);
				pt_round(b_sw, g_len, g_sw);

				D2D1_POINT_2F d_nw;
				D2D1_POINT_2F d_se;
				D2D1_POINT_2F d_ne;
				D2D1_POINT_2F d_sw;
				pt_sub(g_nw, b_nw, d_nw);
				pt_sub(g_se, b_se, d_se);
				pt_sub(g_ne, b_ne, d_ne);
				pt_sub(g_sw, b_sw, d_sw);

				double a_nw = pt_abs2(d_nw);
				double a_se = pt_abs2(d_se);
				double a_ne = pt_abs2(d_ne);
				double a_sw = pt_abs2(d_sw);
				D2D1_POINT_2F diff;
				if (a_se <= a_nw && a_se <= a_ne && a_nw <= a_sw) {
					diff = d_se;
				}
				else if (a_ne <= a_nw && a_ne <= a_se && a_nw <= a_sw) {
					diff = d_ne;
				}
				else if (a_sw <= a_nw && a_sw <= a_se && a_sw <= a_ne) {
					diff = d_sw;
				}
				else {
					diff = d_nw;
				}
				if (flag != true) {
					flag = true;
				}
				slist_move(m_list_shapes, diff);
			}
		}
		if (undo_pop_if_invalid()) {
			return;
		}
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		sheet_update_bbox();
		sheet_panle_size();
		xcvd_is_enabled();
	}

	// 範囲選択を終了する.
	void MainPage::event_finish_selecting_area(const VirtualKeyModifiers k_mod)
	{
		using winrt::Windows::UI::Xaml::Window;

		auto flag = false;
		if (k_mod == VirtualKeyModifiers::Control) {
			D2D1_POINT_2F a_min;
			D2D1_POINT_2F a_max;
			pt_bound(m_event_pos_pressed, m_event_pos_curr, a_min, a_max);
			flag = toggle_area(a_min, a_max);
		}
		else if (k_mod == VirtualKeyModifiers::None) {
			D2D1_POINT_2F a_min;
			D2D1_POINT_2F a_max;
			pt_bound(m_event_pos_pressed, m_event_pos_curr, a_min, a_max);
			flag = select_area(a_min, a_max);
		}
		if (flag == true) {
			// 編集メニュー項目の使用の可否を設定する.
			xcvd_is_enabled();
		}
		Window::Current().CoreWindow().PointerCursor(CC_ARROW);
	}

	// 状況に応じた形状のカーソルを設定する.
	void MainPage::event_curs_style(void)
	{
		// 作図ツールが選択ツールでないか判定する.
		if (m_tool_draw != DRAW_TOOL::SELECT) {
			Window::Current().CoreWindow().PointerCursor(CC_CROSS);
			return;
		}
		if (!m_dx_mutex.try_lock()) {
			Window::Current().CoreWindow().PointerCursor(CC_ARROW);
			return;
		}
		Shape* s;
		const auto anch = slist_hit_test(m_list_shapes, m_event_pos_curr, s);
		m_dx_mutex.unlock();
		if (anch == ANCH_TYPE::ANCH_SHEET) {
			Window::Current().CoreWindow().PointerCursor(CC_ARROW);
		}
		else if (m_cnt_selected > 1) {
			Window::Current().CoreWindow().PointerCursor(CC_SIZE_ALL);
		}
		else {
			switch (anch) {
			case ANCH_TYPE::ANCH_R_NW:
			case ANCH_TYPE::ANCH_R_NE:
			case ANCH_TYPE::ANCH_R_SE:
			case ANCH_TYPE::ANCH_R_SW:
				Window::Current().CoreWindow().PointerCursor(CC_CROSS);
				break;
			case ANCH_TYPE::ANCH_FILL:
			case ANCH_TYPE::ANCH_STROKE:
			case ANCH_TYPE::ANCH_TEXT:
				Window::Current().CoreWindow().PointerCursor(CC_SIZE_ALL);
				break;
			case ANCH_TYPE::ANCH_NE:
			case ANCH_TYPE::ANCH_SW:
				Window::Current().CoreWindow().PointerCursor(CC_SIZE_NESW);
				break;
			case ANCH_TYPE::ANCH_NORTH:
			case ANCH_TYPE::ANCH_SOUTH:
				Window::Current().CoreWindow().PointerCursor(CC_SIZE_NS);
				break;
			case ANCH_TYPE::ANCH_NW:
			case ANCH_TYPE::ANCH_SE:
				Window::Current().CoreWindow().PointerCursor(CC_SIZE_NWSE);
				break;
			case ANCH_TYPE::ANCH_WEST:
			case ANCH_TYPE::ANCH_EAST:
				Window::Current().CoreWindow().PointerCursor(CC_SIZE_WE);
				break;
			default:
				// 図形のクラスが, 多角形または曲線であるか判定する.
				if (s != nullptr &&
					(typeid(*s) == typeid(ShapeLineA) || typeid(*s) == typeid(ShapePoly) || typeid(*s) == typeid(ShapeBezi))) {
					// 図形の部位が, 頂点の数を超えないか判定する.
					const auto d_cnt = static_cast<ShapePath*>(s)->m_diff.size();
					if (anch >= ANCH_TYPE::ANCH_P0 && anch < ANCH_TYPE::ANCH_P0 + d_cnt + 1) {
						Window::Current().CoreWindow().PointerCursor(CC_CROSS);
						break;
					}
				}
				throw winrt::hresult_invalid_argument();
				break;
			}
		}
	}

	// ポインターが動いた.
	void MainPage::event_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::UI::Xaml::Window;
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		event_pos_args(args);
		status_bar_set_curs();
		// ポインターの押された状態が, 初期状態か判定する.
		if (m_event_state == EVENT_STATE::BEGIN) {
			event_curs_style();
		}
		// 状態が. クリックした状態か判定する.
		else if (m_event_state == EVENT_STATE::CLICK) {
			// ポインターの現在位置と押された位置の長さを求め,
			// 長さがクリック判定距離を超えるか判定する.
			D2D1_POINT_2F diff;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, diff);
			if (pt_abs2(diff) > m_event_click_dist) {
				// 初期状態に戻る.
				m_event_state = EVENT_STATE::BEGIN;
				event_curs_style();
			}
		}
		// 状態が, 範囲を選択している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_AREA) {
			sheet_draw();
		}
		// 状態が, 図形を移動している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			// ポインターの現在位置と前回位置の差分を得る.
			D2D1_POINT_2F diff;
			pt_sub(m_event_pos_curr, m_event_pos_prev, diff);
			slist_move(m_list_shapes, diff);
			// ポインターの現在位置を前回位置に格納する.
			m_event_pos_prev = m_event_pos_curr;
			sheet_draw();
		}
		// 状態が, 図形を変形している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_FORM) {
			// ポインターの現在位置を, ポインターが押された図形の, 部位の位置に格納する.
			m_event_shape_pressed->set_anchor_pos(m_event_pos_curr, m_event_anch_pressed);
			// ポインターの現在位置を前回位置に格納する.
			m_event_pos_prev = m_event_pos_curr;
			sheet_draw();
		}
		// 状態が, 左ボタンを押している状態, またはクリック後に左ボタンを押した状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_LBTN || m_event_state == EVENT_STATE::CLICK_LBTN) {
			// ポインターの現在位置と押された位置との差分を得る.
			D2D1_POINT_2F diff;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, diff);
			// 差分の長さがクリックの判定距離を超えるか判定する.
			if (pt_abs2(diff) > m_event_click_dist) {
				// 作図ツールが選択ツールでないか判定する.
				if (m_tool_draw != DRAW_TOOL::SELECT) {
					// 範囲を選択している状態に遷移する.
					m_event_state = EVENT_STATE::PRESS_AREA;
				}
				// 押された図形がヌルか判定する.
				else if (m_event_shape_pressed == nullptr) {
					// 範囲を選択している状態に遷移する.
					m_event_state = EVENT_STATE::PRESS_AREA;
					// 十字カーソルをカーソルに設定する.
					Window::Current().CoreWindow().PointerCursor(CC_CROSS);
				}
				// 選択された図形の数が 1 を超える,
				// または押された図形の部位が線枠, 内側, 文字列かを判定する.
				else if (m_cnt_selected > 1 ||
					m_event_anch_pressed == ANCH_TYPE::ANCH_STROKE || m_event_anch_pressed == ANCH_TYPE::ANCH_FILL || m_event_anch_pressed == ANCH_TYPE::ANCH_TEXT) {
					// 状態を図形を移動している状態に遷移する.
					m_event_state = EVENT_STATE::PRESS_MOVE;
					// ポインターの現在位置を前回位置に格納する.
					m_event_pos_prev = m_event_pos_curr;
					undo_push_move(diff);
				}
				// ポインターが押された図形の部位が図形の外部でないか判定する
				else if (m_event_anch_pressed != ANCH_TYPE::ANCH_SHEET) {
					// 図形を変形している状態に遷移する.
					m_event_state = EVENT_STATE::PRESS_FORM;
					m_event_pos_prev = m_event_pos_curr;
					undo_push_anchor(m_event_shape_pressed, m_event_anch_pressed);
					m_event_shape_pressed->set_anchor_pos(m_event_pos_curr, m_event_anch_pressed);
				}
				sheet_draw();
			}
		}
	}

	// ポインターのボタンが押された.
	// キャプチャの有無にかかわらず, 片方のマウスボタンを押した状態で, もう一方のボタンを押しても, それは通知されない.
	void MainPage::event_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
		using winrt::Windows::UI::Input::PointerPointProperties;
		using winrt::Windows::Devices::Input::PointerDeviceType;

#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		auto const& panel = sender.as<SwapChainPanel>();
		// ポインターのキャプチャを始める.
		panel.CapturePointer(args.Pointer());
		// ポインターのイベント発生時間を得る.
		auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
		event_pos_args(args);
		// ポインターのプロパティーを得る.
		auto const& p_prop = args.GetCurrentPoint(panel).Properties();
		// ポインターのデバイスタイプを判定する.
		switch (args.GetCurrentPoint(panel).PointerDevice().PointerDeviceType()) {
		// デバイスタイプがマウスの場合
		case PointerDeviceType::Mouse:
			// プロパティーが右ボタン押下か判定する.
			if (p_prop.IsRightButtonPressed()) {
				m_event_state = EVENT_STATE::PRESS_RBTN;
			}
			// プロパティーが左ボタン押下か判定する.
			else if (p_prop.IsLeftButtonPressed()) {
				[[fallthrough]];
		// デバイスタイプがペンまたはタッチの場合
		case PointerDeviceType::Pen:
		case PointerDeviceType::Touch:
				// ポインターの押された状態を判定する.
				switch (m_event_state) {
				// 状態がクリックした状態の場合
				case EVENT_STATE::CLICK:
					if (t_stamp - m_event_time_pressed <= m_event_click_time) {
						m_event_state = EVENT_STATE::CLICK_LBTN;
					}
					else {
						[[fallthrough]];
				// 状態が初期状態の場合
				case EVENT_STATE::BEGIN:
				default:
						m_event_state = EVENT_STATE::PRESS_LBTN;
					}
				}
				break;
			}
			else {
				[[fallthrough]];
		// デバイスタイプがそれ以外の場合
		default:
				m_event_state = EVENT_STATE::BEGIN;
				return;
			}
		}
		m_event_time_pressed = t_stamp;
		m_event_pos_pressed = m_event_pos_curr;
		// 作図ツールが選択ツールでないか判定する.
		if (m_tool_draw != DRAW_TOOL::SELECT) {
			return;
		}
		m_event_anch_pressed = slist_hit_test(m_list_shapes, m_event_pos_pressed, m_event_shape_pressed);
		// 押された図形の部位が, 外側でないか判定する.
		if (m_event_anch_pressed != ANCH_TYPE::ANCH_SHEET) {
			// 状態が左ボタンが押された状態, または右ボタンが押されていてかつ押された図形が選択されてないか判定す.
			if (m_event_state == EVENT_STATE::PRESS_LBTN ||
				(m_event_state == EVENT_STATE::PRESS_RBTN && !m_event_shape_pressed->is_selected())) {
				select_shape(m_event_shape_pressed, args.KeyModifiers());
			}
			return;
		}
		m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
		m_event_shape_pressed = nullptr;
		m_event_shape_prev = nullptr;
		// 修飾キーが押されていないならば, すべての図形の選択を解除し, 解除された図形があるか判定する.
		if (args.KeyModifiers() == VirtualKeyModifiers::None && unselect_all()) {
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// ポインターのボタンが上げられた.
	void MainPage::event_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif
		// ポインターの追跡を停止する.
		auto const& panel = sender.as<SwapChainPanel>();
		panel.ReleasePointerCaptures();
		event_pos_args(args);
		// 状態が, 左ボタンが押された状態か判定する.
		if (m_event_state == EVENT_STATE::PRESS_LBTN) {
			// ボタンが離れた時刻と押された時刻の差が, クリックの判定時間以下か判定する.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			if (t_stamp - m_event_time_pressed <= m_event_click_time) {
				// クリックした状態に遷移する.
				m_event_state = EVENT_STATE::CLICK;
				event_curs_style();
				return;
			}
		}
		// 状態が, クリック後に左ボタンが押した状態か判定する.
		else if (m_event_state == EVENT_STATE::CLICK_LBTN) {
			// ボタンが離された時刻と押された時刻の差分を得る.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			// 差分がクリックの判定時間以下, かつ押された図形が文字列図形か判定する.
			if (t_stamp - m_event_time_pressed <= m_event_click_time &&
				m_event_shape_pressed != nullptr && typeid(*m_event_shape_pressed) == typeid(ShapeText)) {
				edit_text_async(static_cast<ShapeText*>(m_event_shape_pressed));
			}
		}
		// 状態が, 図形を移動している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			event_finish_moving();
		}
		// 状態が, 図形を変形している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_FORM) {
			event_finish_forming();
		}
		// 状態が, 範囲選択している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_AREA) {
			// 作図ツールが選択ツールか判定する.
			if (m_tool_draw == DRAW_TOOL::SELECT) {
				event_finish_selecting_area(args.KeyModifiers());
			}
			else {
				// 選択以外の作図ツールが選択されているならば,
				// 方眼にそろえるか, かつシフトキーが押されていないか判定する.
				bool g_snap;
				m_sheet_main.get_grid_snap(g_snap);
				if (g_snap && args.KeyModifiers() != VirtualKeyModifiers::Shift) {
					// 始点と終点を方眼の大きさで丸める.
					float g_base;
					m_sheet_main.get_grid_base(g_base);
					const double g_len = max(g_base + 1.0, 1.0);
					pt_round(m_event_pos_pressed, g_len, m_event_pos_pressed);
					pt_round(m_event_pos_curr, g_len, m_event_pos_curr);
				}
				// ポインターの現在の位置と押された位置の差分を求める.
				D2D1_POINT_2F diff;
				pt_sub(m_event_pos_curr, m_event_pos_pressed, diff);
				if (fabs(diff.x) >= 1.0f || fabs(diff.y) >= 1.0f) {
					if (m_tool_draw == DRAW_TOOL::TEXT) {
						event_finish_creating_text_async(diff);
						return;
					}
					event_finish_creating(diff);
				}
			}
		}
		// 状態が, 右ボタンを押した状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
			event_show_context_menu();
		}
		// 状態が, 初期状態か判定する.
		else if (m_event_state == EVENT_STATE::BEGIN) {
			// 本来は初期状態でこのハンドラーが呼び出されるはずはないが,
			// コンテンツダイアログを終了したとき呼び出されてしまう.
			return;
		}
		// 初期状態に戻す.
		m_event_state = EVENT_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
		sheet_draw();
	}

	// ポインターのホイールボタンが操作された.
	void MainPage::event_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
		using winrt::Windows::UI::Xaml::UIElement;
		using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;

#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// コントロールキーが押されてるか判定する.
		if (args.KeyModifiers() == VirtualKeyModifiers::Control) {
			const int32_t delta = args.GetCurrentPoint(scp_sheet_panel()).Properties().MouseWheelDelta();
			sheet_zoom_delta(delta);
		}
		// シフトキーが押されてるか判定する.
		else if (args.KeyModifiers() == VirtualKeyModifiers::Shift) {
			// 横スクロール.
			const int32_t delta = args.GetCurrentPoint(scp_sheet_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_horz(), delta, m_sheet_main.m_sheet_scale)) {
				sheet_draw();
				status_bar_set_curs();
			}
		}
		// 何も押されてないか判定する.
		else if (args.KeyModifiers() == VirtualKeyModifiers::None) {
			// 縦スクロール.
			const int32_t delta = args.GetCurrentPoint(scp_sheet_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_vert(), delta, m_sheet_main.m_sheet_scale)) {
				sheet_draw();
				status_bar_set_curs();
			}
		}
	}

}