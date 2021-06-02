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
	static auto const& CUR_SIZE_ALL = CoreCursor(CoreCursorType::SizeAll, 0);	// 移動カーソル
	static auto const& CUR_SIZE_NESW = CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);	// 右上左下カーソル
	static auto const& CUR_SIZE_NS = CoreCursor(CoreCursorType::SizeNorthSouth, 0);	// 上下カーソル
	static auto const& CUR_SIZE_NWSE = CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);	// 左上右下カーソル
	static auto const& CUR_SIZE_WE = CoreCursor(CoreCursorType::SizeWestEast, 0);	// 左右カーソル

	// 消去フラグの立つ図形をリストから削除する.
	static void reduce_list(S_LIST_T& s_list, U_STACK_T const& u_stack, U_STACK_T const& r_stack);
	// 図形が操作スタックに含まれるか判定する.
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

	// 操作スタックが図形を参照するか判定する.
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

	// ポインターのボタンが上げられた.
	void MainPage::pointer_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		pointer_released(sender, args);
	}

	// コンテキストメニューを表示する.
	void MainPage::pointer_context_menu(void)
	{
		scp_sheet_panel().ContextFlyout(nullptr);
		if (m_pointer_shape == nullptr || m_pointer_anchor == ANCH_TYPE::ANCH_SHEET) {
			// 押された図形がヌル, 
			// または押された部位は外側の場合,
			// 用紙コンテキストメニューを表示する.
			//scp_sheet_panel().ContextFlyout(nullptr);
			scp_sheet_panel().ContextFlyout(m_menu_sheet);
		}
		else if (typeid(*m_pointer_shape) == typeid(ShapeGroup)) {
			// 押された図形がグループの場合,
			//scp_sheet_panel().ContextFlyout(nullptr);
			scp_sheet_panel().ContextFlyout(m_menu_ungroup);
		}
		else {
			// 押された図形の属性値を用紙レイアウトに格納する.
			m_sheet_main.set_to(m_pointer_shape);
			if (m_pointer_anchor == ANCH_TYPE::ANCH_FILL) {
				// 押された図形の部位が内側の場合,
				// 塗りつぶしコンテキストメニューを表示する.
				//scp_sheet_panel().ContextFlyout(nullptr);
				scp_sheet_panel().ContextFlyout(m_menu_fill);
			}
			else if (m_pointer_anchor == ANCH_TYPE::ANCH_TEXT) {
				// 押された図形の部位が文字列の場合,
				// 書体コンテキストメニューを表示する.
				//scp_sheet_panel().ContextFlyout(nullptr);
				scp_sheet_panel().ContextFlyout(m_menu_font);
			}
			else if (m_pointer_anchor == ANCH_TYPE::ANCH_STROKE) {
				// 押された図形の部位が枠上の場合,
				// 線枠コンテキストメニューを表示する.
				scp_sheet_panel().ContextFlyout(m_menu_stroke);
			}
		}
	}

	// ポインターの現在位置を得る.
	void MainPage::pointer_cur_pos(PointerRoutedEventArgs const& args)
	{
		// スワップチェーンパネル上でのポインターの位置を得て, 
		// 用紙座標系に変換し, ポインターの現在位置に格納する.
		D2D1_POINT_2F p_offs;
		pt_add(sheet_min(), sb_horz().Value(), sb_vert().Value(), p_offs);
		pt_mul(args.GetCurrentPoint(scp_sheet_panel()).Position(), 1.0 / m_sheet_main.m_sheet_scale, p_offs, m_pointer_cur);
	}

	// ポインターが用紙のスワップチェーンパネルの中に入った.
	void MainPage::pointer_entered(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		if (sender != scp_sheet_panel()) {
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
			return;
		}
		pointer_cur_pos(args);
		pointer_set();
		sbar_set_curs();
	}

	// ポインターが用紙のスワップチェーンパネルから出た.
	void MainPage::pointer_exited(IInspectable const& sender, PointerRoutedEventArgs const&)
	{
		if (sender != scp_sheet_panel()) {
			return;
		}
		auto const& c_win = Window::Current().CoreWindow();
		auto const& cur = c_win.PointerCursor();
		if (cur.Type() == CUR_ARROW.Type()) {
			return;
		}
		c_win.PointerCursor(CUR_ARROW);
	}

	// 図形の作成を終了する.
	void MainPage::pointer_finish_creating(const D2D1_POINT_2F diff)
	{
		const auto tool = tool_draw();
		Shape* s;
		if (tool == TOOL_DRAW::RECT) {
			s = new ShapeRect(m_pointer_pressed, diff, &m_sheet_main);
		}
		else if (tool == TOOL_DRAW::RRECT) {
			s = new ShapeRRect(m_pointer_pressed, diff, &m_sheet_main);
		}
		else if (tool == TOOL_DRAW::POLY) {
			s = new ShapePoly(m_pointer_pressed, diff, &m_sheet_main, tool_poly());
		}
		else if (tool == TOOL_DRAW::ELLI) {
			s = new ShapeElli(m_pointer_pressed, diff, &m_sheet_main);
		}
		else if (tool == TOOL_DRAW::LINE) {
			s = new ShapeLine(m_pointer_pressed, diff, &m_sheet_main);
		}
		else if (tool == TOOL_DRAW::BEZI) {
			s = new ShapeBezi(m_pointer_pressed, diff, &m_sheet_main);
		}
		else if (tool == TOOL_DRAW::RULER) {
			s = new ShapeRuler(m_pointer_pressed, diff, &m_sheet_main);
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
		undo_push_select(s);
		undo_push_null();
		m_pointer_shape_smry = m_pointer_shape_prev = s;
		edit_menu_enable();
		sheet_update_bbox(s);
		sheet_panle_size();
		sheet_draw();
		// 図形一覧の排他制御が true か判定する.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			smry_append(s);
			smry_select(s);
		}
	}

	// 文字列図形の作成を終了する.
	IAsyncAction MainPage::pointer_finish_creating_text_async(const D2D1_POINT_2F diff)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		tx_edit().Text(L"");
		ck_text_adjust_bbox().IsChecked(text_adjust());
		if (co_await cd_edit_text().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit().Text().c_str());
			auto s = new ShapeText(m_pointer_pressed, diff, text, &m_sheet_main);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			text_adjust(ck_text_adjust_bbox().IsChecked().GetBoolean());
			if (text_adjust()) {
				static_cast<ShapeText*>(s)->adjust_bbox();
			}
			reduce_list(m_list_shapes, m_stack_undo, m_stack_redo);
			unselect_all();
			undo_push_append(s);
			undo_push_select(s);
			undo_push_null();
			m_pointer_shape_smry = m_pointer_shape_prev = s;
			edit_menu_enable();
			sheet_update_bbox(s);
			sheet_panle_size();
			// 図形一覧の排他制御が true か判定する.
			if (m_smry_atomic.load(std::memory_order_acquire)) {
				smry_append(s);
				smry_select(s);
			}
		}
		// ポインターの押された状態を初期状態に戻す.
		m_pointer_state = PBTN_STATE::BEGIN;
		m_pointer_shape = nullptr;
		m_pointer_anchor = ANCH_TYPE::ANCH_SHEET;
		sheet_draw();
	}

	// 図形の変形を終了する.
	void MainPage::pointer_finish_forming(void)
	{
		bool g_snap;
		m_sheet_main.get_grid_snap(g_snap);
		if (g_snap) {
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			pt_round(m_pointer_cur, g_base + 1.0, m_pointer_cur);
		}
		m_pointer_shape->set_anchor_pos(m_pointer_cur, m_pointer_anchor);
		if (undo_pop_if_invalid()) {
			return;
		}
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		sheet_update_bbox();
		sheet_panle_size();
	}

	// 図形の移動を終了する.
	void MainPage::pointer_finish_moving(void)
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
				s_list_move(m_list_shapes, diff);
			}
		}
		if (undo_pop_if_invalid()) {
			return;
		}
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		sheet_update_bbox();
		sheet_panle_size();
		edit_menu_enable();
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
			edit_menu_enable();
		}
		Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
	}

	// 状況に応じた形状のカーソルを設定する.
	void MainPage::pointer_set(void)
	{
		if (tool_draw() != TOOL_DRAW::SELECT) {
			// 作図ツールが選択ツールでない場合
			Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
			return;
		}
		if (m_dx_mutex.try_lock() != true) {
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
			return;
		}
		Shape* s;
		const auto anch = s_list_hit_test(m_list_shapes, m_pointer_cur, s);
		m_dx_mutex.unlock();
		if (anch == ANCH_TYPE::ANCH_SHEET) {
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
		}
		else if (m_cnt_selected > 1) {
			Window::Current().CoreWindow().PointerCursor(CUR_SIZE_ALL);
		}
		else {
			switch (anch) {
			case ANCH_TYPE::ANCH_R_NW:
			case ANCH_TYPE::ANCH_R_NE:
			case ANCH_TYPE::ANCH_R_SE:
			case ANCH_TYPE::ANCH_R_SW:
				Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
				break;
			case ANCH_TYPE::ANCH_FILL:
			case ANCH_TYPE::ANCH_STROKE:
			case ANCH_TYPE::ANCH_TEXT:
				Window::Current().CoreWindow().PointerCursor(CUR_SIZE_ALL);
				break;
			case ANCH_TYPE::ANCH_NE:
			case ANCH_TYPE::ANCH_SW:
				Window::Current().CoreWindow().PointerCursor(CUR_SIZE_NESW);
				break;
			case ANCH_TYPE::ANCH_NORTH:
			case ANCH_TYPE::ANCH_SOUTH:
				Window::Current().CoreWindow().PointerCursor(CUR_SIZE_NS);
				break;
			case ANCH_TYPE::ANCH_NW:
			case ANCH_TYPE::ANCH_SE:
				Window::Current().CoreWindow().PointerCursor(CUR_SIZE_NWSE);
				break;
			case ANCH_TYPE::ANCH_WEST:
			case ANCH_TYPE::ANCH_EAST:
				Window::Current().CoreWindow().PointerCursor(CUR_SIZE_WE);
				break;
			default:
				// 図形のクラスが, 多角形または曲線であるか判定する.
				if (s != nullptr &&
					(typeid(*s) == typeid(ShapeLine) || typeid(*s) == typeid(ShapePoly) || typeid(*s) == typeid(ShapeBezi))) {
					// 図形の部位が, 頂点の数を超えないか判定する.
					const auto d_cnt = static_cast<ShapePath*>(s)->m_diff.size();
					if (anch >= ANCH_TYPE::ANCH_P0 && anch < ANCH_TYPE::ANCH_P0 + d_cnt + 1) {
						Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
						break;
					}
				}
				throw winrt::hresult_invalid_argument();
				break;
			}
		}
	}

	// ポインターが動いた.
	void MainPage::pointer_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::UI::Xaml::Window;
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		pointer_cur_pos(args);
		sbar_set_curs();
		// ポインターの押された状態が初期状態か判定する.
		if (m_pointer_state == PBTN_STATE::BEGIN) {
			pointer_set();
		}
		// ポインターの押された状態がクリックした状態か判定する.
		else if (m_pointer_state == PBTN_STATE::CLICK) {
			// ポインターの現在位置と押された位置の長さを得る.
			D2D1_POINT_2F diff;
			pt_sub(m_pointer_cur, m_pointer_pressed, diff);
			if (pt_abs2(diff) > m_pointer_click_dist) {
				// 長さが閾値を超える場合, 初期状態に戻る.
				m_pointer_state = PBTN_STATE::BEGIN;
				pointer_set();
			}
		}
		// ポインターの押された状態が範囲を選択している状態か判定する.
		else if (m_pointer_state == PBTN_STATE::PRESS_AREA) {
			sheet_draw();
		}
		// ポインターの押された状態が図形を移動している状態か判定する.
		else if (m_pointer_state == PBTN_STATE::PRESS_MOVE) {
			// ポインターの現在位置と前回位置の差分を得る.
			D2D1_POINT_2F diff;
			pt_sub(m_pointer_cur, m_pointer_pre, diff);
			s_list_move(m_list_shapes, diff);
			// ポインターの現在位置を前回位置に格納する.
			m_pointer_pre = m_pointer_cur;
			sheet_draw();
		}
		// ポインターの押された状態が図形を変形している状態か判定する.
		else if (m_pointer_state == PBTN_STATE::PRESS_FORM) {
			// ポインターの現在位置を, ポインターが押された図形の, 部位の位置に格納する.
			m_pointer_shape->set_anchor_pos(m_pointer_cur, m_pointer_anchor);
			// ポインターの現在位置を前回位置に格納する.
			m_pointer_pre = m_pointer_cur;
			sheet_draw();
		}
		// ポインターの押された状態が左ボタンを押している状態, またはクリック後に左ボタンを押した状態か判定する.
		else if (m_pointer_state == PBTN_STATE::PRESS_LBTN || m_pointer_state == PBTN_STATE::CLICK_LBTN) {
			// ポインターの現在位置と押された位置との差分を得る.
			D2D1_POINT_2F diff;
			pt_sub(m_pointer_cur, m_pointer_pressed, diff);
			// 差分の長さがクリックの判定距離を超えるか判定する.
			if (pt_abs2(diff) > m_pointer_click_dist) {
				// 作図ツールが選択ツールでないか判定する.
				if (tool_draw() != TOOL_DRAW::SELECT) {
					// 範囲を選択している状態に遷移する.
					m_pointer_state = PBTN_STATE::PRESS_AREA;
				}
				// 押された図形がヌルか判定する.
				else if (m_pointer_shape == nullptr) {
					// 範囲を選択している状態に遷移する.
					m_pointer_state = PBTN_STATE::PRESS_AREA;
					// 十字カーソルをカーソルに設定する.
					Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
				}
				// 選択された図形の数が 1 を超える,
				// または押された図形の部位が線枠, 内側, 文字列かを判定する.
				else if (m_cnt_selected > 1 ||
					m_pointer_anchor == ANCH_TYPE::ANCH_STROKE || m_pointer_anchor == ANCH_TYPE::ANCH_FILL || m_pointer_anchor == ANCH_TYPE::ANCH_TEXT) {
					// 状態を図形を移動している状態に遷移する.
					m_pointer_state = PBTN_STATE::PRESS_MOVE;
					// ポインターの現在位置を前回位置に格納する.
					m_pointer_pre = m_pointer_cur;
					undo_push_move(diff);
				}
				// ポインターが押された図形の部位が図形の外部でないか判定する
				else if (m_pointer_anchor != ANCH_TYPE::ANCH_SHEET) {
					// 図形を変形している状態に遷移する.
					m_pointer_state = PBTN_STATE::PRESS_FORM;
					m_pointer_pre = m_pointer_cur;
					undo_push_anchor(m_pointer_shape, m_pointer_anchor);
					m_pointer_shape->set_anchor_pos(m_pointer_cur, m_pointer_anchor);
				}
				sheet_draw();
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
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		auto const& panel = sender.as<SwapChainPanel>();
		// ポインターのキャプチャを始める.
		panel.CapturePointer(args.Pointer());
		// ポインターのイベント発生時間を得る.
		auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
		pointer_cur_pos(args);
		// ポインターのプロパティーを得る.
		auto const& p_prop = args.GetCurrentPoint(panel).Properties();
		// ポインターのデバイスタイプを判定する.
		switch (args.GetCurrentPoint(panel).PointerDevice().PointerDeviceType()) {
		// デバイスタイプがマウスの場合
		case PointerDeviceType::Mouse:
			// プロパティーが右ボタン押下か判定する.
			if (p_prop.IsRightButtonPressed()) {
				m_pointer_state = PBTN_STATE::PRESS_RBTN;
			}
			// プロパティーが左ボタン押下か判定する.
			else if (p_prop.IsLeftButtonPressed()) {
				[[fallthrough]];
		// デバイスタイプがペンまたはタッチの場合
		case PointerDeviceType::Pen:
		case PointerDeviceType::Touch:
				// ポインターの押された状態を判定する.
				switch (m_pointer_state) {
				// 押された状態がクリックした状態の場合
				case PBTN_STATE::CLICK:
					if (t_stamp - m_pointer_time <= m_pointer_click_time) {
						m_pointer_state = PBTN_STATE::CLICK_LBTN;
					}
					else {
						[[fallthrough]];
				// 押された状態が初期状態の場合
				case PBTN_STATE::BEGIN:
				default:
						m_pointer_state = PBTN_STATE::PRESS_LBTN;
					}
				}
				break;
			}
			else {
				[[fallthrough]];
		// デバイスタイプがそれ以外の場合
		default:
				m_pointer_state = PBTN_STATE::BEGIN;
				return;
			}
		}
		m_pointer_time = t_stamp;
		m_pointer_pressed = m_pointer_cur;
		// 作図ツールが選択ツールでないか判定する.
		if (tool_draw() != TOOL_DRAW::SELECT) {
			return;
		}
		m_pointer_anchor = s_list_hit_test(m_list_shapes, m_pointer_pressed, m_pointer_shape);
		if (m_pointer_anchor != ANCH_TYPE::ANCH_SHEET) {
			if (m_pointer_state == PBTN_STATE::PRESS_LBTN
				|| (m_pointer_state == PBTN_STATE::PRESS_RBTN && m_pointer_shape->is_selected() != true)) {
				m_pointer_shape_smry = m_pointer_shape;
				select_shape(m_pointer_shape, args.KeyModifiers());
			}
			return;
		}
		// ヌルを, ポインターが押された図形と前回ポインターが押された図形, 一覧でポインターが押された図形に格納する.
		// ANCH_SHEET をポインターが押された部位に格納する.
		m_pointer_anchor = ANCH_TYPE::ANCH_SHEET;
		m_pointer_shape = nullptr;
		m_pointer_shape_prev = nullptr;
		m_pointer_shape_smry = nullptr;
		// キー修飾子をハンドラーの引数から得る.
		if (args.KeyModifiers() != VirtualKeyModifiers::None) {
			// キー修飾子が None でない場合
			return;
		}
		// 選択が解除された図形がない場合
		if (!unselect_all()) {
			return;
		}
		edit_menu_enable();
		sheet_draw();
	}

	// ポインターのボタンが上げられた.
	void MainPage::pointer_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif
		auto const& panel = sender.as<SwapChainPanel>();
		// ポインターの追跡を停止する.
		panel.ReleasePointerCaptures();
		pointer_cur_pos(args);
		// 左ボタンが押された状態か判定する.
		if (m_pointer_state == PBTN_STATE::PRESS_LBTN) {
			// ボタンが離れた時刻と押された時刻の差分を得る.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			// 差分がクリックの判定時間以下か判定する.
			if (t_stamp - m_pointer_time <= m_pointer_click_time) {
				// クリックした状態に遷移する.
				m_pointer_state = PBTN_STATE::CLICK;
				pointer_set();
				return;
			}
		}
		// クリック後に左ボタンが押した状態か判定する.
		else if (m_pointer_state == PBTN_STATE::CLICK_LBTN) {
			// ボタンが離された時刻と押された時刻の差分を得る.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			// 差分がクリックの判定時間以下か判定する.
			if (t_stamp - m_pointer_time <= m_pointer_click_time) {
				// 押された図形が文字列図形か判定する. 
				if (m_pointer_shape != nullptr && typeid(*m_pointer_shape) == typeid(ShapeText)) {
					text_edit_async(static_cast<ShapeText*>(m_pointer_shape));
				}
			}
		}
		else if (m_pointer_state == PBTN_STATE::PRESS_MOVE) {
			// 状態が図形を移動している状態の場合,
			// 図形の移動を終了する.
			pointer_finish_moving();
		}
		else if (m_pointer_state == PBTN_STATE::PRESS_FORM) {
			// 状態が図形を変形している状態の場合,
			// 図形の変形を終了する.
			pointer_finish_forming();
		}
		else if (m_pointer_state == PBTN_STATE::PRESS_AREA) {
			// 状態が範囲選択している状態の場合,
			if (tool_draw() == TOOL_DRAW::SELECT) {
				// 作図ツールが選択ツールの場合,
				pointer_finish_selecting_area(args.KeyModifiers());
			}
			else {
				bool g_snap;
				m_sheet_main.get_grid_snap(g_snap);
				if (g_snap) {
					// 方眼に整列の場合, 始点と終点を方眼の大きさで丸める
					if (args.KeyModifiers() != VirtualKeyModifiers::Shift) {
						float g_base;
						m_sheet_main.get_grid_base(g_base);
						const double g_len = max(g_base + 1.0, 1.0);
						pt_round(m_pointer_pressed, g_len, m_pointer_pressed);
						pt_round(m_pointer_cur, g_len, m_pointer_cur);
					}
				}
				// ポインターの現在の位置と押された位置の差分を求める.
				D2D1_POINT_2F diff;
				pt_sub(m_pointer_cur, m_pointer_pressed, diff);
				if (fabs(diff.x) >= 1.0f || fabs(diff.y) >= 1.0f) {
					if (tool_draw() == TOOL_DRAW::TEXT) {
						pointer_finish_creating_text_async(diff);
						return;
					}
					pointer_finish_creating(diff);
				}
			}
		}
		else if (m_pointer_state == PBTN_STATE::PRESS_RBTN) {
			// 状態が右ボタンを押した状態の場合
			pointer_context_menu();
		}
		else if (m_pointer_state == PBTN_STATE::BEGIN) {
			// 状態が初期状態の場合,
			// 本来は初期状態でこのハンドラーが呼び出されるはずはないが,
			// コンテンツダイアログを終了したとき呼び出されてしまう.
			return;
		}
		// 初期状態に戻す.
		m_pointer_state = PBTN_STATE::BEGIN;
		m_pointer_shape = nullptr;
		m_pointer_anchor = ANCH_TYPE::ANCH_SHEET;
		sheet_draw();
	}

	// ポインターのホイールボタンが操作された.
	void MainPage::pointer_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
		using winrt::Windows::UI::Xaml::UIElement;
		using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;

#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		int32_t delta = args.GetCurrentPoint(scp_sheet_panel()).Properties().MouseWheelDelta();
		if (args.KeyModifiers() == VirtualKeyModifiers::Control) {
			// コントロールキーが押されていた場合, 用紙を拡大縮小する.
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
				value = min(value + 32.0 * m_sheet_main.m_sheet_scale, limit);
			}
			else if (delta > 0 && value > (limit = stbar.Minimum())) {
				value = max(value - 32.0 * m_sheet_main.m_sheet_scale, limit);
			}
			else {
				return;
			}
			stbar.Value(value);
			sheet_draw();
		}
	}

}