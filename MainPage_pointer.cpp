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
		S_LIST_T d_list;
		for (const auto t : s_list) {
			if (t->is_deleted() == false) {
				// 消去フラグがない図形は無視する.
				continue;
			}
			if (refer_ro(u_stack, t)) {
				// 元に戻す操作スタックが参照する図形は無視する.
				continue;
			}
			if (refer_ro(r_stack, t)) {
				// やり直す操作スタックが参照する図形は無視する.
				continue;
			}
			// 上記のいずれでもない図形を消去リストに追加する.
			d_list.push_back(t);
		}
		// 消去リストに含まれる図形をリストから取り除き, 解放する.
		auto it_begin = s_list.begin();
		for (const auto s : d_list) {
			auto it = std::find(it_begin, s_list.end(), s);
			it_begin = s_list.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// 消去リストを消去する.
		d_list.clear();

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

	// 範囲選択を終了する.
	void MainPage::finish_area_select(const VirtualKeyModifiers k_mod)
	{
		using winrt::Windows::UI::Xaml::Window;

		auto flag = false;
		if (k_mod == VirtualKeyModifiers::Control) {
			D2D1_POINT_2F a_min;
			D2D1_POINT_2F a_max;
			pt_bound(m_press_pos, m_curr_pos, a_min, a_max);
			flag = toggle_area(a_min, a_max);
		}
		else if (k_mod == VirtualKeyModifiers::None) {
			D2D1_POINT_2F a_min;
			D2D1_POINT_2F a_max;
			pt_bound(m_press_pos, m_curr_pos, a_min, a_max);
			flag = select_area(a_min, a_max);
		}
		if (flag == true) {
			// 編集メニュー項目の使用の可否を設定する.
			enable_edit_menu();
		}
		Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
	}

	// 図形の作成を終了する.
	void MainPage::finish_create_shape(void)
	{
		if (m_page_panel.m_grid_snap) {
			// 方眼に整列の場合, 始点と終点を方眼の大きさで丸める
			double g = max(m_page_panel.m_grid_base + 1.0, 1.0);
			pt_round(m_press_pos, g, m_press_pos);
			pt_round(m_curr_pos, g, m_curr_pos);
		}
		// ポインターの現在の位置と押された位置の差分を求める.
		D2D1_POINT_2F d_pos;
		pt_sub(m_curr_pos, m_press_pos, d_pos);
		if (fabs(d_pos.x) >= 1.0f || fabs(d_pos.y) >= 1.0f) {
			if (m_draw_tool == DRAW_TOOL::TEXT) {
				static winrt::event_token primary_token;
				static winrt::event_token closed_token;

				tx_edit().Text(L"");
				primary_token = cd_edit_text().PrimaryButtonClick(
					[this](auto, auto) {
						D2D1_POINT_2F d_pos;

						pt_sub(m_curr_pos, m_press_pos, d_pos);
						auto text = wchar_cpy(tx_edit().Text().c_str());
						auto s = new ShapeText(m_press_pos, d_pos, text, &m_page_panel);
#if defined(_DEBUG)
						debug_leak_cnt++;
#endif
						if (ck_ignore_blank().IsChecked().GetBoolean()) {
							s->delete_bottom_blank();
						}
						reduce_list(m_list_shapes, m_stack_undo, m_stack_redo);
						unselect_all();
						undo_push_append(s);
						m_press_shape_prev = m_press_shape_summary = s;
						// 一連の操作の区切としてヌル操作をスタックに積む.
						undo_push_null();
						// 編集メニュー項目の使用の可否を設定する.
						enable_edit_menu();
						s->get_bound(m_page_min, m_page_max);
						set_page_panle_size();
						if (m_summary_visible) {
							summary_append(s);
						}
					}
				);
				closed_token = cd_edit_text().Closed(
					[this](auto, auto)
					{
						cd_edit_text().PrimaryButtonClick(primary_token);
						cd_edit_text().Closed(closed_token);
						m_press_state = STATE_TRAN::BEGIN;
						m_press_shape = nullptr;
						m_press_anchor = ANCH_WHICH::ANCH_OUTSIDE;
						m_press_shape_prev = nullptr;
						page_draw();
					}
				);
				auto _{ cd_edit_text().ShowAsync() };
				// 画面に範囲を表示したままにするために中断する.
				return;
			}
			Shape* s;
			if (m_draw_tool == DRAW_TOOL::RECT) {
				s = new ShapeRect(m_press_pos, d_pos, &m_page_panel);
			}
			else if (m_draw_tool == DRAW_TOOL::RRECT) {
				s = new ShapeRRect(m_press_pos, d_pos, &m_page_panel);
			}
			else if (m_draw_tool == DRAW_TOOL::QUAD) {
				s = new ShapeQuad(m_press_pos, d_pos, &m_page_panel);
			}
			else if (m_draw_tool == DRAW_TOOL::ELLI) {
				s = new ShapeElli(m_press_pos, d_pos, &m_page_panel);
			}
			else if (m_draw_tool == DRAW_TOOL::LINE) {
				s = new ShapeLine(m_press_pos, d_pos, &m_page_panel);
			}
			else if (m_draw_tool == DRAW_TOOL::BEZI) {
				s = new ShapeBezi(m_press_pos, d_pos, &m_page_panel);
			}
			else if (m_draw_tool == DRAW_TOOL::SCALE) {
				s = new ShapeScale(m_press_pos, d_pos, &m_page_panel);
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
			// 一連の操作の区切としてヌル操作をスタックに積む.
			undo_push_null();
			m_press_shape_summary = m_press_shape_prev = s;
			// 編集メニュー項目の使用の可否を設定する.
			enable_edit_menu();
			s->get_bound(m_page_min, m_page_max);
			set_page_panle_size();
			page_draw();
			if (m_summary_visible) {
				summary_append(s);
			}
		}
	}

	// 図形の移動を終了する.
	void MainPage::finish_move_shape(void)
	{
		if (m_page_panel.m_grid_snap) {
			// 選択された図形を囲む方形の左上点を得る.
			D2D1_POINT_2F p_min = {};
			bool flag = true;
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					continue;
				}
				if (s->is_selected() == false) {
					continue;
				}
				if (flag == true) {
					flag = false;
					s->get_min_pos(p_min);
					continue;
				}
				D2D1_POINT_2F nw_pos;
				s->get_min_pos(nw_pos);
				pt_min(nw_pos, p_min, p_min);
			}
			if (flag == false) {
				// 得た左上点を方眼の大きさで丸める.
				// 丸めの前後で生じた差を得る.
				D2D1_POINT_2F g_pos;
				pt_round(p_min, m_page_panel.m_grid_base + 1.0, g_pos);
				D2D1_POINT_2F d_pos;
				pt_sub(g_pos, p_min, d_pos);
				s_list_move(m_list_shapes, d_pos);
			}
		}
		if (undo_pop_if_invalid()) {
			return;
		}
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		// 編集メニュー項目の使用の可否を設定する.
		enable_edit_menu();
	}

	// 図形の変形を終了する.
	void MainPage::finish_form_shape(void)
	{
		if (m_page_panel.m_grid_snap) {
			pt_round(m_curr_pos, m_page_panel.m_grid_base + 1.0, m_curr_pos);
		}
		m_press_shape->set_pos(m_curr_pos, m_press_anchor);
		if (undo_pop_if_invalid()) {
			return;
		}
		// 一連の操作の区切としてヌル操作をスタックに積む.
		undo_push_null();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
	}

	// ポインターのボタンが上げられた.
	void MainPage::scp_pointer_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		scp_pointer_released(sender, args);
	}

	// ポインターがページのパネルの中に入った.
	void MainPage::scp_pointer_entered(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		if (sender != scp_page_panel()) {
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
			return;
		}
		// スワップチェーンパネル上でのポインターの位置を得て, ページ座標に変換し,
		// ポインターの現在位置に格納する.
		D2D1_POINT_2F p_offs;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), p_offs);
		const auto ui_pos{ args.GetCurrentPoint(scp_page_panel()).Position() };
		pt_scale(ui_pos, 1.0 / m_page_panel.m_page_scale, p_offs, m_curr_pos);
		set_pointer();
		// ポインターの位置をステータスバーに格納する.
		status_set_curs();
	}

	// ポインターがページのパネルから出た.
	void MainPage::scp_pointer_exited(IInspectable const& sender, PointerRoutedEventArgs const&)
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
	void MainPage::scp_pointer_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::UI::Xaml::Window;
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// スワップチェーンパネル上でのポインターの位置を得て, ページ座標に変換し,
		// ポインターの現在位置に格納する.
		D2D1_POINT_2F p_offs;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), p_offs);
		const auto ui_pos{ args.GetCurrentPoint(scp_page_panel()).Position() };
		pt_scale(ui_pos, 1.0 / m_page_panel.m_page_scale, p_offs, m_curr_pos);

		// ポインターの位置をステータスバーに格納する.
		status_set_curs();
		if (m_press_state == STATE_TRAN::BEGIN) {
			// 状態が初期状態の場合,
			// 状況に応じた形状のカーソルを設定する.
			set_pointer();
		}
		else if (m_press_state == STATE_TRAN::CLICK) {
			// 状態がクリックした状態の場合,
			// ポインターの現在位置と押された位置の長さを得る.
			D2D1_POINT_2F d_pos;
			pt_sub(m_curr_pos, m_press_pos, d_pos);
			if (pt_abs2(d_pos) > m_click_dist) {
				// 長さが閾値を超える場合, 初期状態に戻る.
				m_press_state = STATE_TRAN::BEGIN;
				set_pointer();
			}
		}
		else if (m_press_state == STATE_TRAN::PRESS_AREA) {
			// 状態が範囲選択している状態の場合,
			// ページと図形を表示する.
			page_draw();
		}
		else if (m_press_state == STATE_TRAN::PRESS_MOVE) {
			// 状態が図形を移動している状態の場合,
			// ポインターの現在位置と前回位置の差分を得る.
			// 選択フラグの立つすべての図形を差分だけ移動する.
			D2D1_POINT_2F d_pos;
			pt_sub(m_curr_pos, m_prev_pos, d_pos);
			s_list_move(m_list_shapes, d_pos);
			// ポインターの現在位置を前回位置に格納する.
			m_prev_pos = m_curr_pos;
			// ページと図形を表示する.
			page_draw();
		}
		else if (m_press_state == STATE_TRAN::PRESS_FORM) {
			// 状態が図形を変形している状態の場合,
			// ポインターの現在位置を押された図形の部位の位置に格納する.
			m_press_shape->set_pos(m_curr_pos, m_press_anchor);
			// ポインターの現在位置を前回位置に格納する.
			m_prev_pos = m_curr_pos;
			// ページと図形を表示する.
			page_draw();
		}
		else if (m_press_state == STATE_TRAN::PRESS_L
			|| m_press_state == STATE_TRAN::CLICK_2) {
			// 状態が左ボタンを押している状態,
			// またはクリック後に左ボタンを押している状態の場合,
			// ポインターの現在位置と押された位置の長さを得る.
			D2D1_POINT_2F d_pos;
			pt_sub(m_curr_pos, m_press_pos, d_pos);
			if (pt_abs2(d_pos) > m_click_dist) {
				// 長さが閾値を超える場合,
				if (m_draw_tool != DRAW_TOOL::SELECT) {
					// 作図ツールが選択ツールでない場合,
					// 範囲を選択している状態に遷移する.
					m_press_state = STATE_TRAN::PRESS_AREA;
				}
				else if (m_press_shape == nullptr) {
					// 押された図形がヌルの場合,
					// 範囲を選択している状態に遷移する.
					m_press_state = STATE_TRAN::PRESS_AREA;
					// 十字カーソルをカーソルに設定する.
					Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
				}
				else if (m_list_selected > 1
					|| m_press_anchor == ANCH_WHICH::ANCH_FRAME
					|| m_press_anchor == ANCH_WHICH::ANCH_INSIDE
					|| m_press_anchor == ANCH_WHICH::ANCH_TEXT) {
					// 選択された図形の数が 1 を超える
					// または押された図形の部位が線枠
					// または押された図形の部位が内側
					// または押された図形の部位が文字列
					m_press_state = STATE_TRAN::PRESS_MOVE;
					// ポインターの現在位置を前回位置に格納する.
					m_prev_pos = m_curr_pos;
					// 図形の位置をスタックに積み, 差分だけ移動する.
					undo_push_pos(d_pos);
				}
				else if (m_press_anchor != ANCH_WHICH::ANCH_OUTSIDE) {
					m_press_state = STATE_TRAN::PRESS_FORM;
					// ポインターの現在位置を前回位置に格納する.
					m_prev_pos = m_curr_pos;
					// 図形の変形前の位置をスタックに保存して, 図形を変形する.
					undo_push_form(m_press_shape, m_press_anchor, m_curr_pos);
				}
				page_draw();
			}
		}
	}

	// ポインターのボタンが押された.
	void MainPage::scp_pointer_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
		using winrt::Windows::UI::Input::PointerPointProperties;

#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// スワップチェーンパネルのポインターのプロパティーを得る.
		auto const& p_prop = args.GetCurrentPoint(scp_page_panel()).Properties();
		// ポインターのキャプチャを始める.
		scp_page_panel().CapturePointer(args.Pointer());
		// イベント発生時間を得る.
		auto t_stamp = args.GetCurrentPoint(scp_page_panel()).Timestamp();
		// スワップチェーンパネル上でのポインターの位置を得て, 
		// ページ座標系に変換し, ポインターの現在位置に格納する.
		D2D1_POINT_2F p_offs;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), p_offs);
		const auto ui_pos{ args.GetCurrentPoint(scp_page_panel()).Position() };
		pt_scale(ui_pos, 1.0 / m_page_panel.m_page_scale, p_offs, m_curr_pos);
		switch (m_press_state) {
		case STATE_TRAN::CLICK:
			// 状態がクリックした状態の場合,
			// イベント発生時間と前回押された時間との時間差を得る.
			if (t_stamp - m_press_time <= m_click_time) {
				if (p_prop.IsLeftButtonPressed()) {
					// 時間差が閾値以下,
					// かつプロパティーが左ボタンの押下の場合,
					// クリック直後に左ボタンを押した状態に遷移する.
					m_press_state = STATE_TRAN::CLICK_2;
					break;
				}
			}
			// ブレークなし.
		case STATE_TRAN::BEGIN:
			// 状態が初期状態の場合, 
			// スワップチェーンパネルのポインターのプロパティーを得る.
			if (p_prop.IsLeftButtonPressed()) {
				// プロパティーが左ボタンの押下の場合,
				// 左ボタンが押された状態に遷移する.
				m_press_state = STATE_TRAN::PRESS_L;
				break;
			}
			else if (p_prop.IsRightButtonPressed()) {
				// プロパティーが右ボタンの押下の場合,
				// 右ボタンが押された状態に遷移する.
				m_press_state = STATE_TRAN::PRESS_R;
				break;
			}
			// ブレークなし.
		default:
			// それ以外の場合, 状態を初期状態に戻して終了する.
			m_press_state = STATE_TRAN::BEGIN;
			return;
		}
		// イベント発生時間をポインターが押された時間に格納する.
		// イベント発生位置をポインターが押された位置に格納する.
		m_press_time = t_stamp;
		m_press_pos = m_curr_pos;
		if (m_draw_tool != DRAW_TOOL::SELECT) {
			// 作図ツールが選択ツールでない場合,
			// 終了する.
			return;
		}
		// ポインターが押された位置を含む図形とその部位をリストから得る.
		m_press_anchor = s_list_hit_test(m_list_shapes, m_press_pos, m_page_dx.m_anch_len, m_press_shape);
		if (m_press_anchor != ANCH_WHICH::ANCH_OUTSIDE) {
			// 図形とその部位を得た場合,
			// 得た値をポインターが押された図形と部位に格納する.
			// 得た図形を一覧でポインターが押された図形に格納する.
			m_press_shape_summary = m_press_shape;
			// 得た図形を選択する.
			select_shape(m_press_shape, args.KeyModifiers());
			// 終了する.
			return;
		}
		// ヌルを, ポインターが押された図形と前回ポインターが押された図形, 一覧でポインターが押された図形に格納する.
		// ANCH_OUTSIDE をポインターが押された部位に格納する.
		m_press_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		m_press_shape = nullptr;
		m_press_shape_prev = nullptr;
		m_press_shape_summary = nullptr;
		// キー修飾子をハンドラーの引数から得る.
		if (args.KeyModifiers() != VirtualKeyModifiers::None) {
			// キー修飾子が None でない場合, 終了する.
			return;
		}
		// すべての図形の選択を解除する.
		if (unselect_all() == false) {
			// 選択が解除された図形がない場合, 終了する.
			return;
		}
		// やり直す操作スタックを消去し, 含まれる操作を破棄する.
		//redo_clear();
		// 編集メニュー項目の使用の可否を設定する.
		enable_edit_menu();
		// ページと図形を表示する.
		page_draw();
	}

	// コンテキストメニューを表示する.
	void MainPage::show_context_menu(void)
	{
		if (m_press_shape == nullptr
			|| m_press_anchor == ANCH_WHICH::ANCH_OUTSIDE) {
			// 押された図形がヌル, 
			// または押された部位は外側の場合,
			// ページコンテキストメニューを表示する.
			scp_page_panel().ContextFlyout(nullptr);
			scp_page_panel().ContextFlyout(m_menu_layout);
		}
		else if (typeid(*m_press_shape) == typeid(ShapeGroup)) {
			// 押された図形がグループの場合,
			scp_page_panel().ContextFlyout(nullptr);
			scp_page_panel().ContextFlyout(m_menu_ungroup);
		}
		else {
			// 押された図形の属性値をページのパネルに格納する.
			m_page_panel.set_to_shape(m_press_shape);
			if (m_press_anchor == ANCH_WHICH::ANCH_INSIDE) {
				// 押された図形の部位が内側の場合,
				// 塗りつぶしコンテキストメニューを表示する.
				scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_fill);
			}
			else if (m_press_anchor == ANCH_WHICH::ANCH_TEXT) {
				// 押された図形の部位が文字列の場合,
				// 書体コンテキストメニューを表示する.
				scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_font);
			}
			else if (m_press_anchor == ANCH_WHICH::ANCH_FRAME) {
				// 押された図形の部位が枠上の場合,
				// 線枠コンテキストメニューを表示する.
				scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_stroke);
			}
		}
	}

	// ポインターのボタンが上げられた.
	void MainPage::scp_pointer_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif
		// ポインターの追跡を停止する.
		scp_page_panel().ReleasePointerCaptures();
		// スワップチェーンパネル上でのポインターの位置を得て, ページ座標に変換し,
		// ポインターの現在位置に格納する.
		D2D1_POINT_2F p_offs;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), p_offs);
		const auto ui_pos{ args.GetCurrentPoint(scp_page_panel()).Position() };
		pt_scale(ui_pos, 1.0 / m_page_panel.m_page_scale, p_offs, m_curr_pos);
		if (m_press_state == STATE_TRAN::PRESS_L) {
			// 左ボタンが押された状態の場合,
			// ボタンが離れた時刻と押された時刻の差分を得る.
			const auto t_stamp = args.GetCurrentPoint(scp_page_panel()).Timestamp();
			if (t_stamp - m_press_time <= m_click_time) {
				// 差分がクリックの判定時間以下の場合,
				// クリックした状態に遷移する.
				// 状況に応じた形状のカーソルを設定する.
				m_press_state = STATE_TRAN::CLICK;
				set_pointer();
				return;
			}
		}
		else if (m_press_state == STATE_TRAN::CLICK_2) {
			// クリック後に左ボタンが押した状態の場合,
			// ボタンが離された時刻と押された時刻の差分を得る.
			const auto t_stamp = args.GetCurrentPoint(scp_page_panel()).Timestamp();
			if (t_stamp - m_press_time <= m_click_time) {
				// 差分がクリックの判定時間以下で
				if (m_press_shape != nullptr && typeid(*m_press_shape) == typeid(ShapeText)) {
					// 押された図形が文字列図形の場合, 
					// 文字列編集ダイアログを表示する.
					text_edit_in(static_cast<ShapeText*>(m_press_shape));
				}
			}
		}
		else if (m_press_state == STATE_TRAN::PRESS_MOVE) {
			// 状態が図形を移動している状態の場合,
			// 図形の移動を終了する.
			finish_move_shape();
		}
		else if (m_press_state == STATE_TRAN::PRESS_FORM) {
			// 状態が図形を変形している状態の場合,
			// 図形の変形を終了する.
			finish_form_shape();
		}
		else if (m_press_state == STATE_TRAN::PRESS_AREA) {
			// 状態が範囲選択している状態の場合,
			if (m_draw_tool == DRAW_TOOL::SELECT) {
				// 作図ツールが選択ツールの場合,
				// 範囲選択を終了する.
				finish_area_select(args.KeyModifiers());
			}
			else {
				// 図形の作成を終了する.
				finish_create_shape();
			}
		}
		else if (m_press_state == STATE_TRAN::PRESS_R) {
			// 状態が右ボタンを押した状態の場合
			show_context_menu();
		}
		// 初期状態に戻す.
		m_press_state = STATE_TRAN::BEGIN;
		m_press_shape = nullptr;
		m_press_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		page_draw();
	}

	// ポインターのホイールボタンが操作された.
	void MainPage::scp_pointer_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
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
			// コントロールキーが押されていた場合, パネルを拡大縮小する.
			if (delta > 0) {
				mfi_zoom_in_clicked(nullptr, nullptr);
			}
			else if (delta < 0) {
				mfi_zoom_out_clicked(nullptr, nullptr);
			}
		}
		else {
			ScrollBar s_bar;
			if (args.KeyModifiers() == VirtualKeyModifiers::Shift) {
				s_bar = sb_horz();
			}
			else if (args.KeyModifiers() == VirtualKeyModifiers::None) {
				s_bar = sb_vert();
			}
			else {
				return;
			}
			// シフトキーが押されていた場合, 横スクロールする.
			double value = s_bar.Value();
			double limit = 0.0;
			if (delta < 0 && value < (limit = s_bar.Maximum())) {
				value = min(value + 32.0 * m_page_panel.m_page_scale, limit);
			}
			else if (delta > 0 && value > (limit = s_bar.Minimum())) {
				value = max(value - 32.0 * m_page_panel.m_page_scale, limit);
			}
			else {
				return;
			}
			s_bar.Value(value);
			page_draw();
		}
	}

	// 状況に応じた形状のカーソルを設定する.
	void MainPage::set_pointer(void)
	{
		if (m_draw_tool != DRAW_TOOL::SELECT) {
			Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
			return;
		}
		Shape* s;
		const auto a = s_list_hit_test(m_list_shapes, m_curr_pos, m_page_dx.m_anch_len, s);
		if (a == ANCH_WHICH::ANCH_OUTSIDE || s->is_selected() == false) {
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
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
			return;
		//}
		//Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
	}

}