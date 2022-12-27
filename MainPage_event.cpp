//-------------------------------
// MainPage_event.cpp
// ポインターのイベントハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Devices::Input::PointerDeviceType;
	//using winrt::Windows::Foundation::IAsyncAction;
	//using winrt::Windows::System::VirtualKeyModifiers;
	using winrt::Windows::UI::Core::CoreCursor;
	using winrt::Windows::UI::Core::CoreCursorType;
	//using winrt::Windows::UI::Core::CoreWindow;
	using winrt::Windows::UI::Input::PointerPointProperties;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;
	//using winrt::Windows::UI::Xaml::Controls::SwapChainPanel;
	//using winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs;
	using winrt::Windows::UI::Xaml::UIElement;
	using winrt::Windows::UI::Xaml::Window;

	static auto const& CURS_ARROW = CoreCursor(CoreCursorType::Arrow, 0);	// 矢印カーソル
	static auto const& CURS_CROSS = CoreCursor(CoreCursorType::Cross, 0);	// 十字カーソル
	static auto const& CURS_SIZE_ALL = CoreCursor(CoreCursorType::SizeAll, 0);	// 移動カーソル
	static auto const& CURS_SIZE_NESW = CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);	// 右上左下カーソル
	static auto const& CURS_SIZE_NS = CoreCursor(CoreCursorType::SizeNorthSouth, 0);	// 上下カーソル
	static auto const& CURS_SIZE_NWSE = CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);	// 左上右下カーソル
	static auto const& CURS_SIZE_WE = CoreCursor(CoreCursorType::SizeWestEast, 0);	// 左右カーソル
	static auto const& CURS_WAIT = CoreCursor(CoreCursorType::Wait , 0);	// 左右カーソル

	// 図形が操作スタックに含まれるか判定する.
	static bool event_ustack_include_shape(UNDO_STACK const& ustack, Shape* const s) noexcept;
	// 消去フラグの立つ図形をリストから削除する.
	static void event_slist_reduce(SHAPE_LIST& slist, UNDO_STACK const& ustack, UNDO_STACK const& r_stack) noexcept;
	// マウスホイールの値でスクロールする.
	static bool event_scroll_by_wheel_delta(const ScrollBar& scroll_bar, const int32_t delta, const float scale);
	// 選択された図形の頂点に最も近い方眼を見つけ, 頂点と方眼との差分を求める.
	static bool event_get_vec_nearby_grid(const SHAPE_LIST& slist, const float g_len, D2D1_POINT_2F& g_vec) noexcept;
	// 非選択の図形の頂点の中から, 選択された図形の頂点に最も近い頂点を見つけ, ２点間の差分を求める.
	static bool event_get_vec_nearby_vert(const SHAPE_LIST& slist, const float d_limit, D2D1_POINT_2F& v_vec) noexcept;

	// 操作スタックが図形を参照するか判定する.
	// ustack	操作スタック
	// s	図形
	// 戻り値	参照する場合 true.
	static bool event_ustack_include_shape(UNDO_STACK const& ustack, Shape* const s) noexcept
	{
		for (const auto u : ustack) {
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
	// ただし, 操作スタックで参照されている図形は削除されない.
	// slist	図形リスト
	// ustack	元に戻す操作スタック
	// rstack	やり直す操作スタック
	static void event_slist_reduce(SHAPE_LIST& slist, UNDO_STACK const& ustack, UNDO_STACK const& rstack) noexcept
	{
		// 消去フラグの立つ図形を消去リストに格納する.
		SHAPE_LIST slist_del;	// 消去リスト
		for (const auto t : slist) {
			// 図形の消去フラグがない,
			// または図形が元に戻す操作スタックに含まれる,
			// または図形がやり直し操作スタックに含まれる,　か判定する.
			if (!t->is_deleted() ||
				event_ustack_include_shape(ustack, t) ||
				event_ustack_include_shape(rstack, t)) {
				continue;
			}
			// 上記のいずれでもない図形を消去リストに追加する.
			slist_del.push_back(t);
		}
		// 消去リストに含まれる図形をリストから取り除き, 解放する.
		auto it_beg = slist.begin();
		for (const auto s : slist_del) {
			const auto it = std::find(it_beg, slist.end(), s);
			it_beg = slist.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// 消去リストを消去する.
		slist_del.clear();
	}

	// マウスホイールの値でスクロールする.
	static bool event_scroll_by_wheel_delta(const ScrollBar& scroll_bar, const int32_t delta, const float scale)
	{
		constexpr double DELTA = 32.0;
		double val = scroll_bar.Value();
		double limit = 0.0;
		if (delta < 0 && val < (limit = scroll_bar.Maximum())) {
			val = min(val + DELTA / scale, limit);
		}
		else if (delta > 0 && val > (limit = scroll_bar.Minimum())) {
			val = max(val - DELTA / scale, limit);
		}
		else {
			return false;
		}
		scroll_bar.Value(val);
		return true;
	}

	// 選択された図形に最も近い方眼を見つけ, 図形と方眼との差分を求める.
	// slist	図形リスト
	// g_len	方眼の大きさ
	// g_vec	図形と方眼との差分
	static bool event_get_vec_nearby_grid(const SHAPE_LIST& slist, const float g_len, D2D1_POINT_2F& g_vec) noexcept
	{
		D2D1_POINT_2F v_pos[2 + MAX_N_GON];
		D2D1_POINT_2F g_pos;
		D2D1_POINT_2F g_sub;
		double d_min = FLT_MAX;
		for (const auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			s->get_bound(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, v_pos[0], v_pos[1]);
			const auto v_cnt = s->get_verts(v_pos + 2);
			for (size_t i = 0; i < 2 + v_cnt; i++) {
				pt_round(v_pos[i], g_len, g_pos);
				pt_sub(g_pos, v_pos[i], g_sub);
				const auto g_abs = pt_abs2(g_sub);
				if (g_abs < d_min) {
					g_vec = g_sub;
					d_min = g_abs;
					if (g_abs < FLT_MIN) {
						return true;
					}
				}
			}
		}
		return d_min < FLT_MAX;
	}

	// 非選択の図形の頂点の中から, 選択された図形の頂点に最も近い頂点を見つけ, ２点間の差分を求める.
	// slist	図形リスト
	// d_limit	制限距離 (これ以上離れた頂点は対象としない)
	// v_vec	最も近い頂点間の差分
	// 戻り値	見つかったなら true
	static bool event_get_vec_nearby_vert(const SHAPE_LIST& slist, const float d_limit, D2D1_POINT_2F& v_vec) noexcept
	{
		float dd = d_limit * d_limit;
		bool done = false;
		D2D1_POINT_2F v_pos[MAX_N_GON];
		D2D1_POINT_2F w_pos{};
		D2D1_POINT_2F n_pos{};	// 近傍点
		for (const auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			const auto v_cnt = s->get_verts(v_pos);
			for (size_t i = 0; i < v_cnt; i++) {
				for (const auto t : slist) {
					if (t->is_deleted() || t->is_selected()) {
						continue;
					}
					if (t->get_pos_nearest(v_pos[i], dd, n_pos)) {
						w_pos = v_pos[i];
						done = true;
					}
				}
			}
		}
		if (done) {
			pt_sub(n_pos, w_pos, v_vec);
		}
		return done;
	}

	// ポインターのボタンが上げられた.
	void MainPage::event_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		event_released(sender, args);
	}

	// ポインターが表示のスワップチェーンパネルの中に入った.
	void MainPage::event_entered(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		if (sender != scp_page_panel()) {
			Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
		}
		else {
			event_set_pos_cur(args);
			event_set_curs_style();
			status_bar_set_pos();
		}
	}

	// ポインターが表示のスワップチェーンパネルから出た.
	void MainPage::event_exited(IInspectable const& sender, PointerRoutedEventArgs const&)
	{
		if (sender == scp_page_panel()) {
			auto const& c_win = Window::Current().CoreWindow();
			auto const& p_cur = c_win.PointerCursor();
			if (p_cur.Type() != CURS_ARROW.Type()) {
				c_win.PointerCursor(CURS_ARROW);
			}
		}
	}

	// 図形の作成を終了する.
	// b_pos	囲む領域の開始位置
	// b_vec	終了位置への差分
	void MainPage::event_finish_creating(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec)
	{
		const auto t_draw = m_drawing_tool;
		Shape* s;
		if (t_draw == DRAWING_TOOL::RECT) {
			s = new ShapeRect(b_pos, b_vec, &m_main_page);
		}
		else if (t_draw == DRAWING_TOOL::RRECT) {
			s = new ShapeRRect(b_pos, b_vec, &m_main_page);
		}
		else if (t_draw == DRAWING_TOOL::POLY) {
			s = new ShapePoly(b_pos, b_vec, &m_main_page, m_drawing_poly_opt);
		}
		else if (t_draw == DRAWING_TOOL::ELLI) {
			s = new ShapeElli(b_pos, b_vec, &m_main_page);
		}
		else if (t_draw == DRAWING_TOOL::LINE) {
			s = new ShapeLine(b_pos, b_vec, &m_main_page);
		}
		else if (t_draw == DRAWING_TOOL::BEZI) {
			s = new ShapeBezi(b_pos, b_vec, &m_main_page);
		}
		else if (t_draw == DRAWING_TOOL::RULER) {
			s = new ShapeRuler(b_pos, b_vec, &m_main_page);
		}
		else {
			return;
		}
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		event_slist_reduce(m_main_page.m_shape_list, m_ustack_undo, m_ustack_redo);
		ustack_push_append(s);
		ustack_push_select(s);
		ustack_push_null();
		m_event_shape_prev = s;
		xcvd_is_enabled();
		page_bbox_update(s);
		page_panel_size();
		page_draw();
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_append(s);
			summary_select(s);
		}
	}

	// 文字列図形の作成を終了する.
	IAsyncAction MainPage::event_finish_creating_text_async(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec)
	{
		tx_edit_text().Text(L"");
		ck_text_frame_fit_text().IsChecked(m_text_frame_fit_text);
		if (co_await cd_edit_text_dialog().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit_text().Text().c_str());
			auto s = new ShapeText(b_pos, b_vec, text, &m_main_page);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			if (m_text_frame_fit_text) {
				s->frame_fit(m_main_page.m_grid_snap ? m_main_page.m_grid_base + 1.0f : 0.0f);
			}
			m_text_frame_fit_text = ck_text_frame_fit_text().IsChecked().GetBoolean();
			event_slist_reduce(m_main_page.m_shape_list, m_ustack_undo, m_ustack_redo);
			//unselect_all();
			ustack_push_append(s);
			ustack_push_select(s);
			ustack_push_null();
			m_event_shape_prev = s;
			xcvd_is_enabled();
			page_bbox_update(s);
			page_panel_size();
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_append(s);
				summary_select(s);
			}
		}
		// ポインターの押された状態を初期状態に戻す.
		m_event_state = EVENT_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_anc_pressed = ANC_TYPE::ANC_VIEW;
		page_draw();
	}

	// 押された図形の変形を終了する.
	void MainPage::event_finish_forming(void)
	{
		const auto g_snap = m_main_page.m_grid_snap;
		if (g_snap && m_vert_stick >= FLT_MIN) {
			// 現在の位置と, それを方眼の大きさに丸めた位置と間の距離を求める.
			const auto s_scale = m_main_page.m_page_scale;
			D2D1_POINT_2F g_pos;
			D2D1_POINT_2F g_vec;
			pt_round(m_event_pos_curr, m_main_page.m_grid_base + 1.0, g_pos);
			pt_sub(g_pos, m_event_pos_curr, g_vec);
			float g_len = min(static_cast<float>(sqrt(pt_abs2(g_vec))), m_vert_stick) / s_scale;
			if (slist_find_vertex_closest(m_main_page.m_shape_list, m_event_pos_curr, g_len, g_pos)) {
				// 方眼との距離より近い頂点が見つかったなら, その距離に入れ替える.
				pt_sub(g_pos, m_event_pos_curr, g_vec);
				g_len = static_cast<float>(sqrt(pt_abs2(g_vec))) / s_scale;
			}
			// 近傍の頂点によって位置が変わらなかったか判定する.
			if (!m_event_shape_pressed->set_pos_anc(m_event_pos_curr, m_event_anc_pressed, g_len, m_image_keep_aspect)) {
				// 変わらなかったならば, 方眼に合わせる.
				m_event_shape_pressed->set_pos_anc(g_pos, m_event_anc_pressed, 0.0f, m_image_keep_aspect);
			}
		}
		else if (g_snap) {
			pt_round(m_event_pos_curr, m_main_page.m_grid_base + 1.0, m_event_pos_curr);
			m_event_shape_pressed->set_pos_anc(m_event_pos_curr, m_event_anc_pressed, 0.0f, m_image_keep_aspect);
		}
		else if (m_vert_stick >= FLT_MIN) {
			slist_find_vertex_closest(m_main_page.m_shape_list, m_event_pos_curr, m_vert_stick / m_main_page.m_page_scale, m_event_pos_curr);
			m_event_shape_pressed->set_pos_anc(m_event_pos_curr, m_event_anc_pressed, m_vert_stick / m_main_page.m_page_scale, m_image_keep_aspect);
		}
		if (!ustack_pop_if_invalid()) {
			ustack_push_null();
			page_bbox_update();
			page_panel_size();
			xcvd_is_enabled();
		}
	}

	// 選択された図形の移動を終了する.
	void MainPage::event_finish_moving(void)
	{
		// 方眼に合わせる, かつ頂点に合わせるか判定する.
		if (m_main_page.m_grid_snap && m_vert_stick >= FLT_MIN) {
			D2D1_POINT_2F g_vec{};	// 方眼への差分
			if (event_get_vec_nearby_grid(m_main_page.m_shape_list, m_main_page.m_grid_base + 1.0f, g_vec)) {
				D2D1_POINT_2F v_vec{};	// 頂点への差分
				if (event_get_vec_nearby_vert(m_main_page.m_shape_list, m_vert_stick / m_main_page.m_page_scale, v_vec) && pt_abs2(v_vec) < pt_abs2(g_vec)) {
					g_vec = v_vec;
				}
				slist_move(m_main_page.m_shape_list, g_vec);
			}
		}
		// 方眼に合わせる, かつ頂点に合わせないを判定する.
		else if (m_main_page.m_grid_snap) {
			D2D1_POINT_2F g_vec{};	// 方眼との差分
			if (event_get_vec_nearby_grid(m_main_page.m_shape_list, m_main_page.m_grid_base + 1.0f, g_vec)) {
				slist_move(m_main_page.m_shape_list, g_vec);
			}
		} 
		// 方眼に合わせない, かつ頂点に合わせるか判定する.
		else if (m_vert_stick >= FLT_MIN) {
			D2D1_POINT_2F v_vec{};	// 頂点との差分
			if (event_get_vec_nearby_vert(m_main_page.m_shape_list, m_vert_stick / m_main_page.m_page_scale, v_vec)) {
				slist_move(m_main_page.m_shape_list, v_vec);
			}
		}
		if (!ustack_pop_if_invalid()) {
			ustack_push_null();
			page_bbox_update();
			page_panel_size();
			xcvd_is_enabled();
		}
	}

	// 範囲選択を終了する.
	void MainPage::event_finish_selecting_area(const VirtualKeyModifiers k_mod)
	{
		// 修飾キーがコントロールか判定する.
		if (k_mod == VirtualKeyModifiers::Control) {
			D2D1_POINT_2F area_min;
			D2D1_POINT_2F area_max;
			if (m_event_pos_pressed.x < m_event_pos_curr.x) {
				area_min.x = m_event_pos_pressed.x;
				area_max.x = m_event_pos_curr.x;
			}
			else {
				area_min.x = m_event_pos_curr.x;
				area_max.x = m_event_pos_pressed.x;
			}
			if (m_event_pos_pressed.y < m_event_pos_curr.y) {
				area_min.y = m_event_pos_pressed.y;
				area_max.y = m_event_pos_curr.y;
			}
			else {
				area_min.y = m_event_pos_curr.y;
				area_max.y = m_event_pos_pressed.y;
			}
			if (toggle_area(area_min, area_max)) {
				xcvd_is_enabled();
			}
		}
		// 修飾キーが押されてないか判定する.
		else if (k_mod == VirtualKeyModifiers::None) {
			D2D1_POINT_2F area_min;
			D2D1_POINT_2F area_max;
			if (m_event_pos_pressed.x < m_event_pos_curr.x) {
				area_min.x = m_event_pos_pressed.x;
				area_max.x = m_event_pos_curr.x;
			}
			else {
				area_min.x = m_event_pos_curr.x;
				area_max.x = m_event_pos_pressed.x;
			}
			if (m_event_pos_pressed.y < m_event_pos_curr.y) {
				area_min.y = m_event_pos_pressed.y;
				area_max.y = m_event_pos_curr.y;
			}
			else {
				area_min.y = m_event_pos_curr.y;
				area_max.y = m_event_pos_pressed.y;
			}
			if (select_area(area_min, area_max)) {
				xcvd_is_enabled();
			}
		}
		Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
	}

	// ポインターが動いた.
	void MainPage::event_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// ファイル開くピッカーが返値を戻すまでの排他処理.
		if (!m_mutex_event.try_lock()) {
			Window::Current().CoreWindow().PointerCursor(CURS_WAIT);
			return;
		}
		m_mutex_event.unlock();

		event_set_pos_cur(args);
		status_bar_set_pos();
		// ポインターの押された状態が, 初期状態か判定する.
		if (m_event_state == EVENT_STATE::BEGIN) {
			event_set_curs_style();
		}
		// 状態が. クリックした状態か判定する.
		else if (m_event_state == EVENT_STATE::CLICK) {
			// ポインターの現在位置と押された位置の長さを求め,
			// 長さがクリック判定距離を超えるか判定する.
			D2D1_POINT_2F vec;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, vec);
			if (pt_abs2(vec) > m_event_click_dist) {
				// 初期状態に戻る.
				m_event_state = EVENT_STATE::BEGIN;
				event_set_curs_style();
			}
		}
		// 状態が, 範囲を選択している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_AREA) {
			page_draw();
		}
		// 状態が, 図形を移動している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			// ポインターの現在位置と前回位置の差分を得る.
			D2D1_POINT_2F vec;
			pt_sub(m_event_pos_curr, m_event_pos_prev, vec);
			slist_move(m_main_page.m_shape_list, vec);
			// ポインターの現在位置を前回位置に格納する.
			m_event_pos_prev = m_event_pos_curr;
			page_draw();
		}
		// 状態が, 図形を変形している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_FORM) {
			// ポインターの現在位置を, ポインターが押された図形の, 部位の位置に格納する.
			m_event_shape_pressed->set_pos_anc(m_event_pos_curr, m_event_anc_pressed, 0.0f, m_image_keep_aspect);
			// ポインターの現在位置を前回位置に格納する.
			m_event_pos_prev = m_event_pos_curr;
			page_draw();
		}
		// 状態が, 左ボタンを押している状態, または
		// クリック後に左ボタンを押した状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_LBTN || 
			m_event_state == EVENT_STATE::CLICK_LBTN) {
			// ポインターの現在位置と押された位置との差分を得る.
			D2D1_POINT_2F vec;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, vec);
			// 差分がクリックの判定距離を超えるか判定する.
			if (pt_abs2(vec) > m_event_click_dist) {
				// 作図ツールが選択ツール以外か判定する.
				if (m_drawing_tool != DRAWING_TOOL::SELECT) {
					// 範囲を選択している状態に遷移する.
					m_event_state = EVENT_STATE::PRESS_AREA;
				}
				// 押された図形がヌルか判定する.
				else if (m_event_shape_pressed == nullptr) {
					// 範囲を選択している状態に遷移する.
					m_event_state = EVENT_STATE::PRESS_AREA;
					// 十字カーソルをカーソルに設定する.
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
				}
				// 選択された図形の数が 1 を超える,
				// または押された図形の部位が線枠, 内側, 文字列かを判定する.
				else if (m_list_sel_cnt > 1 ||
					m_event_anc_pressed == ANC_TYPE::ANC_STROKE ||
					m_event_anc_pressed == ANC_TYPE::ANC_FILL ||
					m_event_anc_pressed == ANC_TYPE::ANC_TEXT) {
					// 図形を移動している状態に遷移する.
					m_event_state = EVENT_STATE::PRESS_MOVE;
					// ポインターの現在位置を前回位置に保存する.
					m_event_pos_prev = m_event_pos_curr;
					ustack_push_move(vec);
				}
				// ポインターが押されたのが図形の外部以外か判定する.
				else if (m_event_anc_pressed != ANC_TYPE::ANC_VIEW) {
					// 図形を変形している状態に遷移する.
					// ポインターの現在位置を前回位置に保存する.
					m_event_state = EVENT_STATE::PRESS_FORM;
					m_event_pos_prev = m_event_pos_curr;
					ustack_push_position(m_event_shape_pressed, m_event_anc_pressed);
					m_event_shape_pressed->set_pos_anc(m_event_pos_curr, m_event_anc_pressed, 0.0f, m_image_keep_aspect);
				}
				page_draw();
			}
		}
	}

	// ポインターのボタンが押された.
	// キャプチャの有無にかかわらず, 片方のマウスボタンを押した状態で, もう一方のボタンを押しても, それは通知されない.
	void MainPage::event_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// ファイル開くピッカーが返値を戻すまでの排他処理.
		if (!m_mutex_event.try_lock()) {
			Window::Current().CoreWindow().PointerCursor(CURS_WAIT);
			return;
		}
		m_mutex_event.unlock();

		// ポインターのキャプチャを始める.
		// 引数の値をポンインターの現在位置に格納する.
		// ポインターのイベント発生時間を得る.
		// ポインターのプロパティーを得る.
		const SwapChainPanel& swap_chain_panel = sender.as<SwapChainPanel>();
		swap_chain_panel.CapturePointer(args.Pointer());
		event_set_pos_cur(args);
		const uint64_t t_stamp = args.GetCurrentPoint(swap_chain_panel).Timestamp();
		const PointerPointProperties& p_prop = args.GetCurrentPoint(swap_chain_panel).Properties();
		// ポインターのデバイスタイプを判定する.
		switch (args.GetCurrentPoint(swap_chain_panel).PointerDevice().PointerDeviceType()) {
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
					// イベント発生時刻と前回ポインターが押された時刻の差がクリック判定時間以下か判定する.
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
		// 作図ツールが選択ツールか判定する.
		if (m_drawing_tool == DRAWING_TOOL::SELECT) {
			m_event_anc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, m_event_shape_pressed);
			// 押されたのが図形の外側か判定する.
			if (m_event_anc_pressed == ANC_TYPE::ANC_VIEW) {
				m_event_shape_pressed = nullptr;
				m_event_shape_prev = nullptr;
				// 修飾キーが押されていないならば, すべての図形の選択を解除する.
				// 解除された図形があるか判定する.
				if (args.KeyModifiers() == VirtualKeyModifiers::None && unselect_all()) {
					xcvd_is_enabled();
					page_draw();
				}
			}
			else {
				// 状態が左ボタンが押された状態, または, 右ボタンが押されていてかつ押された図形が選択されてないか判定する.
				if (m_event_state == EVENT_STATE::PRESS_LBTN ||
					(m_event_state == EVENT_STATE::PRESS_RBTN && !m_event_shape_pressed->is_selected())) {
					select_shape(m_event_shape_pressed, args.KeyModifiers());
				}
			}
		}
	}

	// 頂点に合わせるよう, 押された位置と離された位置を調整する.
	// slist	図形リスト
	// box_type	
	static void event_released_snap_to_vertex(const SHAPE_LIST& slist, const bool box_type, const float d_limit, const bool g_snap, const double g_len, D2D1_POINT_2F& p_pos, D2D1_POINT_2F& r_pos)
	{
		// 四隅の位置を得る.
		D2D1_POINT_2F b_pos[4]{
			p_pos, { r_pos.x, p_pos.y }, r_pos, { p_pos.x, r_pos.y },
		};
		// 左上位置に最も近い頂点とその距離を得る.
		double v_abs[4];
		D2D1_POINT_2F v_pos[4];
		if (slist_find_vertex_closest(slist, b_pos[0], d_limit, v_pos[0])) {
			D2D1_POINT_2F v_sub;
			pt_sub(v_pos[0], b_pos[0], v_sub);
			v_abs[0] = pt_abs2(v_sub);
		}
		else {
			v_abs[0] = FLT_MAX;
		}
		// 箱型なら, 右上位置に最も近い頂点とその距離を得る.
		if (box_type && slist_find_vertex_closest(slist, b_pos[1], d_limit, v_pos[1])) {
			D2D1_POINT_2F v_sub;
			pt_sub(v_pos[1], b_pos[1], v_sub);
			v_abs[1] = pt_abs2(v_sub);
		}
		else {
			v_abs[1] = FLT_MAX;
		}
		if (slist_find_vertex_closest(slist, b_pos[2], d_limit, v_pos[2])) {
			D2D1_POINT_2F v_sub;
			pt_sub(v_pos[2], b_pos[2], v_sub);
			v_abs[2] = pt_abs2(v_sub);
		}
		else {
			v_abs[2] = FLT_MAX;
		}
		if (box_type && slist_find_vertex_closest(slist, b_pos[3], d_limit, v_pos[3])) {
			D2D1_POINT_2F v_sub;
			pt_sub(v_pos[3], b_pos[3], v_sub);
			v_abs[3] = pt_abs2(v_sub);
		}
		else {
			v_abs[3] = FLT_MAX;
		}
		double g_abs[2];	// 方眼との距離の自乗
		D2D1_POINT_2F g_pos[2];
		if (g_snap) {
			D2D1_POINT_2F g_sub[2];
			pt_round(p_pos, g_len, g_pos[0]);
			pt_round(r_pos, g_len, g_pos[1]);
			pt_sub(g_pos[0], p_pos, g_sub[0]);
			pt_sub(g_pos[1], r_pos, g_sub[1]);
			g_abs[0] = pt_abs2(g_sub[0]);
			g_abs[1] = pt_abs2(g_sub[1]);
		}
		else {
			g_pos[0] = p_pos;
			g_pos[1] = r_pos;
			g_abs[0] = FLT_MAX;
			g_abs[1] = FLT_MAX;
		}

		if (g_abs[0] <= v_abs[0] && g_abs[0] <= v_abs[3]) {
			p_pos.x = g_pos[0].x;
		}
		else if (v_abs[0] <= g_abs[0] && v_abs[0] <= v_abs[3]) {
			p_pos.x = v_pos[0].x;
		}
		else {
			p_pos.x = v_pos[3].x;
		}

		if (g_abs[0] <= v_abs[0] && g_abs[0] <= v_abs[1]) {
			p_pos.y = g_pos[0].y;
		}
		else if (v_abs[0] <= g_abs[0] && v_abs[0] <= v_abs[1]) {
			p_pos.y = v_pos[0].y;
		}
		else {
			p_pos.y = v_pos[1].y;
		}

		if (g_abs[1] <= v_abs[2] && g_abs[1] <= v_abs[1]) {
			r_pos.x = g_pos[1].x;
		}
		else if (v_abs[2] <= g_abs[1] && v_abs[2] <= v_abs[1]) {
			r_pos.x = v_pos[2].x;
		}
		else {
			r_pos.x = v_pos[1].x;
		}
		if (g_abs[1] <= v_abs[2] && g_abs[1] <= v_abs[3]) {
			r_pos.y = g_pos[1].y;
		}
		else if (v_abs[2] <= g_abs[1] && v_abs[2] <= v_abs[3]) {
			r_pos.y = v_pos[2].y;
		}
		else {
			r_pos.y = v_pos[3].y;
		}

	}

	// ポインターのボタンが上げられた.
	void MainPage::event_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif
		// ファイル開くピッカーが返値を戻すまでの排他処理.
		if (!m_mutex_event.try_lock()) {
			Window::Current().CoreWindow().PointerCursor(CURS_WAIT);
			return;
		}
		m_mutex_event.unlock();

		// ポインターの追跡を停止する.
		auto const& panel = sender.as<SwapChainPanel>();
		panel.ReleasePointerCaptures();
		event_set_pos_cur(args);
		// 状態が, 左ボタンが押された状態か判定する.
		if (m_event_state == EVENT_STATE::PRESS_LBTN) {
			// ボタンが離れた時刻と押された時刻の差が, クリックの判定時間以下か判定する.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			if (t_stamp - m_event_time_pressed <= m_event_click_time) {
				// クリックした状態に遷移する.
				m_event_state = EVENT_STATE::CLICK;
				event_set_curs_style();
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
		// 状態が, 範囲を選択している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_AREA) {
			// 作図ツールが選択ツールか判定する.
			if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				event_finish_selecting_area(args.KeyModifiers());
			}
			else {
				// 作図ツールが選択ツール以外.
				unselect_all();
				// 頂点をくっつける閾値がゼロより大きいか判定する.
				if (m_vert_stick >= FLT_MIN) {
					const bool box_type = (
						m_drawing_tool == DRAWING_TOOL::ELLI ||
						m_drawing_tool == DRAWING_TOOL::POLY ||
						m_drawing_tool == DRAWING_TOOL::RECT ||
						m_drawing_tool == DRAWING_TOOL::RRECT ||
						m_drawing_tool == DRAWING_TOOL::RULER ||
						m_drawing_tool == DRAWING_TOOL::TEXT);
					const float d_lim = m_vert_stick / m_main_page.m_page_scale;
					const double g_len = max(m_main_page.m_grid_base, 0.0) + 1.0;
					event_released_snap_to_vertex(m_main_page.m_shape_list, box_type, d_lim, m_main_page.m_grid_snap, g_len, m_event_pos_pressed, m_event_pos_curr);
				}
				// 方眼に合わせるか判定する.
				else if (m_main_page.m_grid_snap) {
					// 押された位置よ離された位置を方眼の大きさで丸める.
					const double g_len = max(m_main_page.m_grid_base + 1.0, 1.0);
					pt_round(m_event_pos_pressed, g_len, m_event_pos_pressed);
					pt_round(m_event_pos_curr, g_len, m_event_pos_curr);
				}

				// ポインターの現在位置と押された位置の差分を求める.
				D2D1_POINT_2F c_vec;	// 現在位置への差分
				pt_sub(m_event_pos_curr, m_event_pos_pressed, c_vec);
				// 差分の x 値または y 値のいずれかが 1 以上か判定する.
				if (fabs(c_vec.x) >= 1.0f || fabs(c_vec.y) >= 1.0f) {
					if (m_drawing_tool == DRAWING_TOOL::TEXT) {
						event_finish_creating_text_async(m_event_pos_pressed, c_vec);
						return;
					}
					event_finish_creating(m_event_pos_pressed, c_vec);
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
			// コンテンツダイアログやメニューを終了したとき呼び出されることがあった.
			return;
		}
		// 初期状態に戻す.
		m_event_state = EVENT_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_anc_pressed = ANC_TYPE::ANC_VIEW;
		page_draw();
	}

	// ポインターの形状を設定する.
	void MainPage::event_set_curs_style(void)
	{
		// 作図ツールが選択ツール以外か判定する.
		if (m_drawing_tool != DRAWING_TOOL::SELECT) {
			Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
		}
		// 描画の排他制御をロックできないか判定する.
		else if (!m_mutex_draw.try_lock()) {
			Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
		}
		else {
			// 描画の排他制御をロックできたなら, ただちに解除する.
			m_mutex_draw.unlock();
			Shape* s;
			const auto anc = slist_hit_test(m_main_page.m_shape_list, m_event_pos_curr, s);
			if (anc == ANC_TYPE::ANC_VIEW) {
				Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
			}
			else if (m_list_sel_cnt > 1) {
				Window::Current().CoreWindow().PointerCursor(CURS_SIZE_ALL);
			}
			else {
				switch (anc) {
				case ANC_TYPE::ANC_R_NW:
				case ANC_TYPE::ANC_R_NE:
				case ANC_TYPE::ANC_R_SE:
				case ANC_TYPE::ANC_R_SW:
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
					break;
				case ANC_TYPE::ANC_FILL:
				case ANC_TYPE::ANC_STROKE:
				case ANC_TYPE::ANC_TEXT:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_ALL);
					break;
				case ANC_TYPE::ANC_NE:
				case ANC_TYPE::ANC_SW:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_NESW);
					break;
				case ANC_TYPE::ANC_NORTH:
				case ANC_TYPE::ANC_SOUTH:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_NS);
					break;
				case ANC_TYPE::ANC_NW:
				case ANC_TYPE::ANC_SE:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_NWSE);
					break;
				case ANC_TYPE::ANC_WEST:
				case ANC_TYPE::ANC_EAST:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_WE);
					break;
				default:
					// 図形のクラスが, 多角形または曲線であるか判定する.
					if (s != nullptr &&
						(typeid(*s) == typeid(ShapeLine) || typeid(*s) == typeid(ShapePoly) || typeid(*s) == typeid(ShapeBezi))) {
						// 図形の部位が, 頂点の数を超えないか判定する.
						if (anc >= ANC_TYPE::ANC_P0 && anc < ANC_TYPE::ANC_P0 + static_cast<ShapePath*>(s)->m_vec.size() + 1) {
							Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
							break;
						}
					}
					throw winrt::hresult_invalid_argument();
					break;
				}
			}
		}
	}

	// ポインターの現在位置に格納する.
	void MainPage::event_set_pos_cur(PointerRoutedEventArgs const& args)
	{
		// スワップチェーンパネル上でのポインターの位置を表示座標系に変換する.
		D2D1_POINT_2F page_pos;
		pt_add(m_main_min, sb_horz().Value(), sb_vert().Value(), page_pos);
		pt_mul_add(args.GetCurrentPoint(scp_page_panel()).Position(), 1.0 / m_main_page.m_page_scale, page_pos, m_event_pos_curr);
	}

	// コンテキストメニューを表示する.
	void MainPage::event_show_context_menu(void)
	{
		//using winrt::Windows::UI::Xaml::Controls::MenuFlyout;
		using winrt::Windows::UI::Xaml::Controls::MenuFlyoutSeparator;
		// コンテキストメニューを解放する.
		ContextFlyout(nullptr);
		// 押された図形がヌル, または押された図形の部位が外側か判定する.
		if (m_event_shape_pressed == nullptr || m_event_anc_pressed == ANC_TYPE::ANC_VIEW) {
			if (m_menu_page == nullptr) {
				m_menu_page = MenuFlyout();
				for (const auto item : mbi_grid().Items()) {
					m_menu_page.Items().Append(item);
				}
			}
			ContextFlyout(m_menu_page);
		}
		// 押された図形がグループか判定する.
		else if (typeid(*m_event_shape_pressed) == typeid(ShapeGroup)) {
			if (m_menu_ungroup == nullptr) {
				m_menu_ungroup = MenuFlyout();
				for (const auto& item : mbi_edit().Items()) {
					if (item.Name() == L"mfi_ungroup") {
						m_menu_ungroup.Items().Append(item);
					}
				}
			}
			ContextFlyout(m_menu_ungroup);
		}
		else {
			// 押された図形が定規か判定する.
			if (typeid(*m_event_shape_pressed) == typeid(ShapeRuler)) {
				if (m_menu_ruler == nullptr) {
					m_menu_ruler = MenuFlyout();
					m_menu_ruler.Items().Append(mfi_stroke_color());
					m_menu_ruler.Items().Append(mfi_fill_color());
					m_menu_ruler.Items().Append(MenuFlyoutSeparator());
					m_menu_ruler.Items().Append(mfi_font_family());
					m_menu_ruler.Items().Append(mfi_font_size());
				}
				ContextFlyout(m_menu_ruler);
			}
			// 押された図形が画像か判定する.
			else if (typeid(*m_event_shape_pressed) == typeid(ShapeImage)) {
				if (m_menu_image == nullptr) {
					m_menu_image = MenuFlyout();
					for (const auto item : mbi_image().Items()) {
						m_menu_image.Items().Append(item);
					}
				}
				ContextFlyout(m_menu_image);
			}
			// 押された図形の部位が内側か判定する.
			else if (m_event_anc_pressed == ANC_TYPE::ANC_FILL) {
				if (m_menu_fill == nullptr) {
					m_menu_fill = MenuFlyout();
					for (const auto item : mbi_fill().Items()) {
						m_menu_fill.Items().Append(item);
					}
				}
				ContextFlyout(m_menu_fill);
			}
			// 押された図形の部位が文字列か判定する.
			else if (m_event_anc_pressed == ANC_TYPE::ANC_TEXT) {
				if (m_menu_font == nullptr) {
					m_menu_font = MenuFlyout();
					for (const auto item : mbi_font().Items()) {
						m_menu_font.Items().Append(item);
					}
				}
				ContextFlyout(m_menu_font);
			}
			// 押された図形の部位が線枠か判定する.
			else if (m_event_anc_pressed == ANC_TYPE::ANC_STROKE) {
				if (m_menu_stroke == nullptr) {
					m_menu_stroke = MenuFlyout();
					for (const auto item : mbi_stroke().Items()) {
						m_menu_stroke.Items().Append(item);
					}
				}
				ContextFlyout(m_menu_stroke);
			}
			// 押された図形の属性値を表示に格納する.
			m_main_page.set_attr_to(m_event_shape_pressed);
			page_setting_is_checked();
		}
	}

	// ポインターのホイールボタンが操作された.
	void MainPage::event_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// ファイル開くピッカーが返値を戻すまでの排他処理.
		if (!m_mutex_event.try_lock()) {
			Window::Current().CoreWindow().PointerCursor(CURS_WAIT);
			return;
		}
		m_mutex_event.unlock();

		// コントロールキーが押されてるか判定する.
		if (args.KeyModifiers() == VirtualKeyModifiers::Control) {
			// 拡大縮小
			const int32_t delta = args.GetCurrentPoint(scp_page_panel()).Properties().MouseWheelDelta();
			page_zoom_delta(delta);
		}
		// シフトキーが押されてるか判定する.
		else if (args.KeyModifiers() == VirtualKeyModifiers::Shift) {
			// 横スクロール.
			const int32_t delta = args.GetCurrentPoint(scp_page_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_horz(), delta, m_main_page.m_page_scale)) {
				page_draw();
				status_bar_set_pos();
			}
		}
		// 何も押されてないか判定する.
		else if (args.KeyModifiers() == VirtualKeyModifiers::None) {
			// 縦スクロール.
			const int32_t delta = args.GetCurrentPoint(scp_page_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_vert(), delta, m_main_page.m_page_scale)) {
				page_draw();
				status_bar_set_pos();
			}
		}
	}

}