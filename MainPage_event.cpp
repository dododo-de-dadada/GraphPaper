//-------------------------------
// MainPage_event.cpp
// ポインターのイベントハンドラー
//-------------------------------
#include "pch.h"
#include "MainPage.h"
#include "resource.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Devices::Input::PointerDeviceType;
	using winrt::Windows::UI::Core::CoreCursor;
	using winrt::Windows::UI::Core::CoreCursorType;
	using winrt::Windows::UI::Input::PointerPointProperties;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;
	using winrt::Windows::UI::Xaml::Window;
	using winrt::Windows::UI::Xaml::Controls::MenuFlyoutSeparator;
	using winrt::Windows::UI::ViewManagement::UISettings;
	using winrt::Windows::UI::Input::PointerUpdateKind;

	static auto const& CURS_IBEAM = CoreCursor(CoreCursorType::IBeam, 0);	// 文字列カーソル
	static auto const& CURS_ARROW = CoreCursor(CoreCursorType::Arrow, 0);	// 矢印カーソル
	static auto const& CURS_CROSS = CoreCursor(CoreCursorType::Cross, 0);	// 十字カーソル
	static auto const& CURS_SIZE_ALL = CoreCursor(CoreCursorType::SizeAll, 0);	// 移動カーソル
	static auto const& CURS_SIZE_NESW = CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);	// 右上左下カーソル
	static auto const& CURS_SIZE_NS = CoreCursor(CoreCursorType::SizeNorthSouth, 0);	// 上下カーソル
	static auto const& CURS_SIZE_NWSE = CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);	// 左上右下カーソル
	static auto const& CURS_SIZE_WE = CoreCursor(CoreCursorType::SizeWestEast, 0);	// 左右カーソル
	static auto const& CURS_EYEDROPPER1 = CoreCursor(CoreCursorType::Custom, IDC_CURSOR1);	// スポイトカーソル
	static auto const& CURS_EYEDROPPER2 = CoreCursor(CoreCursorType::Custom, IDC_CURSOR2);	// スポイトカーソル

	// 押された点と離された点を調整する.
	static void event_snap_point(const SHAPE_LIST& slist, const bool boxed, const double interval, const bool g_snap, const double g_len, D2D1_POINT_2F& pressed, D2D1_POINT_2F& released);
	// 最も近い方眼を得る.
	static bool event_get_nearest_grid(const SHAPE_LIST& slist, const float g_len, D2D1_POINT_2F& g_pos) noexcept;
	// 最も近い点を得る.
	static bool event_get_nearest_point(const SHAPE_LIST& slist, const double interval, D2D1_POINT_2F& v_pos) noexcept;
	// 図形が操作スタックに含まれるか判定する.
	static bool event_undo_contain_shape(const UNDO_STACK& ustack, const Shape* s) noexcept;
	// 図形リストを整理する.
	static void event_reduce_slist(SHAPE_LIST& slist, const UNDO_STACK& ustack, const UNDO_STACK& r_stack) noexcept;
	// マウスホイールの値でスクロールする.
	static bool event_scroll_by_wheel_delta(const ScrollBar& scroll_bar, const int32_t delta, const float scale);

	// 点をそろえる.
	static void event_snap_point(
		const SHAPE_LIST& slist,	// 図形リスト
		const bool is_box,	// 調整の対象を, 図形を囲む矩形とするなら true, 図形の頂点を対象とするなら false 
		const double interval,	// 制限距離
		const bool g_snap,	// 方眼にそろえる.
		const double g_len,	// 方眼の大きさ
		D2D1_POINT_2F& pressed,	// 押された位置
		D2D1_POINT_2F& released	// 離された位置
	)
	{
		D2D1_POINT_2F box[4]{	// 押された位置と離された位置で囲まれた方形の頂点
			pressed, { released.x, pressed.y }, released, { pressed.x, released.y },
		};
		double p_abs[4];	// 位置と頂点との距離 (の自乗).
		D2D1_POINT_2F p[4];	// 頂点の位置

		// 左上位置に最も近い頂点とその距離を得る.
		if (slist_find_vertex_closest(slist, box[0], interval, p[0])) {
			D2D1_POINT_2F d;
			pt_sub(p[0], box[0], d);
			p_abs[0] = pt_abs2(d);
		}
		else {
			p_abs[0] = FLT_MAX;
		}

		// 調整の対象が矩形なら, 右上位置に最も近い頂点とその距離を得る.
		if (is_box && slist_find_vertex_closest(slist, box[1], interval, p[1])) {
			D2D1_POINT_2F d;
			pt_sub(p[1], box[1], d);
			p_abs[1] = pt_abs2(d);
		}
		else {
			p_abs[1] = FLT_MAX;
		}

		// 右下位置に最も近い頂点とその距離を得る.
		if (slist_find_vertex_closest(slist, box[2], interval, p[2])) {
			D2D1_POINT_2F d;
			pt_sub(p[2], box[2], d);
			p_abs[2] = pt_abs2(d);
		}
		else {
			p_abs[2] = FLT_MAX;
		}

		// 調整の対象が矩形なら, 左下位置に最も近い頂点とその距離を得る.
		if (is_box && slist_find_vertex_closest(slist, box[3], interval, p[3])) {
			D2D1_POINT_2F d;
			pt_sub(p[3] , box[3], d);
			p_abs[3] = pt_abs2(d);
		}
		else {
			p_abs[3] = FLT_MAX;
		}

		// 方眼にそろえる場合,
		// 押された位置と離された位置に, 最も近い格子の位置を得る.
		double g_abs[2];	// 方眼の格子との距離 (の自乗)
		D2D1_POINT_2F g[2];
		if (g_snap) {
			D2D1_POINT_2F d[2];
			pt_round(pressed, g_len, g[0]);
			pt_round(released, g_len, g[1]);
			pt_sub(g[0], pressed, d[0]);
			pt_sub(g[1], released, d[1]);
			g_abs[0] = pt_abs2(d[0]);
			g_abs[1] = pt_abs2(d[1]);
		}

		// 方眼にそろえない場合,
		// 押された位置と離された位置を, 最も近い格子の位置に格納し,
		// その距離は最大値とする.
		else {
			g[0] = pressed;
			g[1] = released;
			g_abs[0] = FLT_MAX;
			g_abs[1] = FLT_MAX;
		}

		if (g_abs[0] <= p_abs[0] && g_abs[0] <= p_abs[3]) {
			pressed.x = g[0].x;
		}
		else if (p_abs[0] <= g_abs[0] && p_abs[0] <= p_abs[3]) {
			pressed.x = p[0].x;
		}
		else {
			pressed.x = p[3].x;
		}

		if (g_abs[0] <= p_abs[0] && g_abs[0] <= p_abs[1]) {
			pressed.y = g[0].y;
		}
		else if (p_abs[0] <= g_abs[0] && p_abs[0] <= p_abs[1]) {
			pressed.y = p[0].y;
		}
		else {
			pressed.y = p[1].y;
		}

		if (g_abs[1] <= p_abs[2] && g_abs[1] <= p_abs[1]) {
			released.x = g[1].x;
		}
		else if (p_abs[2] <= g_abs[1] && p_abs[2] <= p_abs[1]) {
			released.x = p[2].x;
		}
		else {
			released.x = p[1].x;
		}
		if (g_abs[1] <= p_abs[2] && g_abs[1] <= p_abs[3]) {
			released.y = g[1].y;
		}
		else if (p_abs[2] <= g_abs[1] && p_abs[2] <= p_abs[3]) {
			released.y = p[2].y;
		}
		else {
			released.y = p[3].y;
		}
	}

	// 最も近い方眼への位置ベクトルを求める.
	static bool event_get_nearest_grid(
		const SHAPE_LIST& slist,	// 図形リスト
		const float g_len,	// 方眼の大きさ
		D2D1_POINT_2F& g_pos	// 図形から方眼への位置ベクトル
	) noexcept
	{
		D2D1_POINT_2F p[2 + N_GON_MAX];
		D2D1_POINT_2F g;	// 方眼の大きさで丸めた位置
		D2D1_POINT_2F d;	// 丸めた位置と元の位置の差分
		double d_min = FLT_MAX;	// 最も短い距離
		for (const auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			// 境界矩形の左上位置, 右下位置を得る.
			s->get_bbox(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, p[0], p[1]);
			const auto p_cnt = s->get_verts(p + 2);
			for (size_t i = 0; i < 2 + p_cnt; i++) {
				pt_round(p[i], g_len, g);
				pt_sub(g, p[i], d);
				const auto g_abs = pt_abs2(d);	// 丸めた位置と元の位置の距離の二乗
				if (g_abs < d_min) {
					g_pos = d;
					d_min = g_abs;
					if (g_abs <= FLT_MIN) {
						return true;
					}
				}
			}
		}
		return d_min < FLT_MAX;
	}

	// 最も近い頂点を得る.
	// 戻り値	見つかったなら true
	static bool event_get_nearest_point(
		const SHAPE_LIST& slist,	// 図形リスト
		const double interval,	// 間隔 (これ以上離れた点は無視する)
		D2D1_POINT_2F& pos	// 最も近い頂点への位置ベクトル
	) noexcept
	{
		double dd = interval * interval;
		bool flag = false;
		D2D1_POINT_2F p[N_GON_MAX];
		D2D1_POINT_2F q{};
		D2D1_POINT_2F n{};	// 近傍点
		for (const auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			const auto v_cnt = s->get_verts(p);
			for (size_t i = 0; i < v_cnt; i++) {
				for (const auto t : slist) {
					if (t->is_deleted() || t->is_selected()) {
						continue;
					}
					if (t->get_pos_nearest(p[i], dd, n)) {
						q = p[i];
						flag = true;
					}
				}
			}
		}
		if (flag) {
			pt_sub(n, q, pos);
		}
		return flag;
	}

	// マウスホイールの値でスクロールする.
	static bool event_scroll_by_wheel_delta(
		const ScrollBar& scroll_bar,	// スクロールバー
		const int32_t delta,	// マウスホイールのデルタ値
		const float scale	// ページの倍率
	)
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

	// 図形リストを整理する.
	static void event_reduce_slist(
		SHAPE_LIST& slist,	// 図形リスト
		const UNDO_STACK& ustack,	// 元に戻す操作スタック
		const UNDO_STACK& rstack	// やり直す操作スタック
	) noexcept
	{
		// 消去フラグの立つ図形を消去リストに格納する.
		SHAPE_LIST delete_list;	// 消去リスト
		for (const auto t : slist) {
			// 図形の消去フラグがない,
			// または図形が元に戻す操作スタックに含まれる,
			// または図形がやり直し操作スタックに含まれる,　か判定する.
			if (!t->is_deleted() || event_undo_contain_shape(ustack, t) || event_undo_contain_shape(rstack, t)) {
				continue;
			}
			// 上記のいずれでもない図形を消去リストに追加する.
			delete_list.push_back(t);
		}
		// 消去リストに含まれる図形をリストから取り除き, 解放する.
		auto begin = slist.begin();
		for (const auto s : delete_list) {
			const auto it = std::find(begin, slist.end(), s);
			begin = slist.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// 消去リストを消去する.
		delete_list.clear();
	}

	// 操作スタックが図形を含むか判定する.
	// 戻り値	含む場合 true.
	static bool event_undo_contain_shape(
		const UNDO_STACK& ustack,	// 操作スタック
		const Shape* s	// 図形
	) noexcept
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

	//------------------------------
	// ポインターのボタンが上げられた.
	//------------------------------
	void MainPage::event_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		event_released(sender, args);
	}

	//------------------------------
	// ポインターがスワップチェーンパネルの中に入った.
	//------------------------------
	void MainPage::event_entered(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		if (sender != scp_main_panel()) {
			Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
		}
		else {
			event_set_position(args);
			event_set_cursor();
			status_bar_set_pos();
		}
	}

	//------------------------------
	// ポインターがスワップチェーンパネルから出た.
	//------------------------------
	void MainPage::event_exited(IInspectable const& sender, PointerRoutedEventArgs const&)
	{
		if (sender == scp_main_panel()) {
			auto const& w = Window::Current().CoreWindow();
			auto const& c = w.PointerCursor();
			if (c.Type() != CURS_ARROW.Type()) {
				w.PointerCursor(CURS_ARROW);
			}
		}
	}

	//------------------------------
	// 色を検出する.
	//------------------------------
	void MainPage::event_eyedropper_detect(
		const Shape* s,	// 押された図形
		const uint32_t loc	// 押された部位
	)
	{
		if (loc == LOC_TYPE::LOC_PAGE) {
			m_eyedropper_color = m_main_page.m_page_color;
			m_eyedropper_filled = true;
			Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
		}
		else if (s != nullptr) {
			if (loc == LOC_TYPE::LOC_FILL) {
				if (typeid(*s) == typeid(ShapeImage)) {
					if (static_cast<const ShapeImage*>(s)->get_pixcel(m_event_pos_pressed, m_eyedropper_color)) {
						m_main_page.m_fill_color = m_eyedropper_color;
						m_eyedropper_filled = true;
					}
				}
				else {
					s->get_fill_color(m_eyedropper_color);
					m_main_page.m_fill_color = m_eyedropper_color;
					m_eyedropper_filled = true;
				}
				Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
			}
			else if (loc == LOC_TYPE::LOC_TEXT) {
				s->get_font_color(m_eyedropper_color);
				m_main_page.m_font_color = m_eyedropper_color;
				m_eyedropper_filled = true;
				Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
			}
			else if (loc == LOC_TYPE::LOC_STROKE) {
				s->get_stroke_color(m_eyedropper_color);
				m_main_page.m_stroke_color = m_eyedropper_color;
				m_eyedropper_filled = true;
				Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
			}
		}
	}

	//------------------------------
	// 図形の作成を終了する.
	// start	始点
	// pos	対角点への位置ベクトル
	//------------------------------
	void MainPage::event_finish_creating(const D2D1_POINT_2F start, const D2D1_POINT_2F pos)
	{
		const auto d_tool = m_drawing_tool;
		Shape* s;
		if (d_tool == DRAWING_TOOL::RECT) {
			s = new ShapeRect(start, pos, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::RRECT) {
			s = new ShapeRRect(start, pos, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::POLY) {
			const auto poly_opt = m_drawing_poly_opt;
			s = new ShapePoly(start, pos, &m_main_page, poly_opt);
		}
		else if (d_tool == DRAWING_TOOL::ELLIPSE) {
			s = new ShapeEllipse(start, pos, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::LINE) {
			s = new ShapeLine(start, pos, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::BEZIER) {
			s = new ShapeBezier(start, pos, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::RULER) {
			s = new ShapeRuler(start, pos, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::ARC) {
			s = new ShapeArc(start, pos, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::TEXT) {
			if (m_edit_text_shape != nullptr) {
				m_edit_context.NotifyFocusLeave();
				undo_push_text_unselect(m_edit_text_shape);
			}
			s = new ShapeText(start, pos, nullptr, &m_main_page);
			m_edit_text_shape = static_cast<ShapeText*>(s);
			m_edit_text_shape->create_text_layout();
			m_edit_context.NotifyFocusEnter();
		}
		else {
			return;
		}
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		event_reduce_slist(m_main_page.m_shape_list, m_ustack_undo, m_ustack_redo);
		undo_push_null();
		unselect_shape_all();
		undo_push_append(s);
		undo_push_select(s);
		//undo_menu_is_enabled();
		xcvd_menu_is_enabled();
		main_bbox_update(s);
		main_panel_size();
		m_event_shape_last = s;
		main_draw();
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_append(s);
			summary_select(s);
		}
		status_bar_set_pos();
	}

	//------------------------------
	// 文字列図形の作成を終了する.
	// start	始点
	// pos	対角点への位置ベクトル
	//------------------------------
	/*
	IAsyncAction MainPage::event_finish_creating_text_async(const D2D1_POINT_2F start, const D2D1_POINT_2F pos)
	{
		const auto fit_text = m_fit_text_frame;
		tx_edit_text().Text(L"");
		ck_fit_text_frame().IsChecked(fit_text);
		if (co_await cd_edit_text_dialog().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit_text().Text().c_str());
			auto s = new ShapeText(start, pos, text, &m_main_page);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			if (fit_text) {
				s->fit_frame_to_text(m_snap_grid ? m_main_page.m_grid_base + 1.0f : 0.0f);
			}
			m_fit_text_frame = ck_fit_text_frame().IsChecked().GetBoolean();
			event_reduce_slist(m_main_page.m_shape_list, m_ustack_undo, m_ustack_redo);
			undo_push_null();
			unselect_shape_all();
			undo_push_append(s);
			undo_push_select(s);
			//undo_menu_is_enabled();
			xcvd_menu_is_enabled();
			m_event_shape_last = s;
			main_bbox_update(s);
			main_panel_size();
			// 一覧が表示されてるか判定する.
			if (summary_is_visible()) {
				summary_append(s);
				summary_select(s);
			}
		}
		// ポインターの押された状態を初期状態に戻す.
		m_event_state = EVENT_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_loc_pressed = LOC_TYPE::LOC_PAGE;
		main_draw();
		status_bar_set_pos();
	}
	*/
	//------------------------------
	// 図形の変形を終了する.
	//------------------------------
	void MainPage::event_finish_deforming(const VirtualKeyModifiers key_mod)
	{
		//const auto g_snap = m_main_page.m_snap_grid;
		//const auto g_snap = m_snap_grid;
		if (m_snap_grid && m_snap_point >= FLT_MIN) {
			// 現在の位置と, それを方眼の大きさに丸めた位置と間の距離を求める.
			D2D1_POINT_2F p{ m_event_pos_curr };
			D2D1_POINT_2F q;
			D2D1_POINT_2F r;
			m_event_shape_pressed->get_pos_loc(m_event_loc_pressed, p);
			pt_round(p, m_main_page.m_grid_base + 1.0, q);
			pt_sub(q, p, r);
			const double s = sqrt(pt_abs2(r));
			double grid_dist = min(s, static_cast<double>(m_snap_point)) / m_main_scale;
			// 方眼との距離より近い頂点が見つかったなら, その距離に入れ替える.
			if (slist_find_vertex_closest(m_main_page.m_shape_list, p, grid_dist, q)) {
				pt_sub(q, p, r);
				grid_dist = sqrt(pt_abs2(r)) / m_main_scale;
			}
			// 近傍の頂点によって点が変わらなかったか判定する.
			const auto keep_aspect = ((key_mod & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			if (!m_event_shape_pressed->set_pos_loc(p, m_event_loc_pressed, grid_dist, keep_aspect)) {
				// 変わらなかったならば, 方眼に合わせる.
				m_event_shape_pressed->set_pos_loc(q, m_event_loc_pressed, 0.0f, keep_aspect);
			}
		}
		else if (m_snap_grid) {
			D2D1_POINT_2F p{ m_event_pos_curr };
			m_event_shape_pressed->get_pos_loc(m_event_loc_pressed, p);
			const auto keep_aspect = ((key_mod & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			pt_round(p, m_main_page.m_grid_base + 1.0, p);
			m_event_shape_pressed->set_pos_loc(p, m_event_loc_pressed, 0.0f, keep_aspect);
		}
		else if (m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{ m_event_pos_curr };
			m_event_shape_pressed->get_pos_loc(m_event_loc_pressed, p);
			const auto keep_aspect = ((key_mod & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			slist_find_vertex_closest(m_main_page.m_shape_list, p, m_snap_point / m_main_scale, p);
			m_event_shape_pressed->set_pos_loc(p, m_event_loc_pressed, m_snap_point / m_main_scale, keep_aspect);
		}
		//undo_push_null();
		//if (!undo_pop_invalid()) {
		//	//undo_menu_is_enabled();
		//	main_bbox_update();
		//	main_panel_size();
		//}
	}

	//------------------------------
	// 図形の移動を終了する.
	//------------------------------
	void MainPage::event_finish_moving(void)
	{
		// 方眼にくっつける, かつ頂点にくっつける.
		if (m_snap_grid && m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{};	// 方眼との差分
			if (event_get_nearest_grid(m_main_page.m_shape_list, m_main_page.m_grid_base + 1.0f, p)) {
				D2D1_POINT_2F q{};	// 頂点との差分
				if (event_get_nearest_point(m_main_page.m_shape_list, m_snap_point / m_main_scale, q) && pt_abs2(q) < pt_abs2(p)) {
					// 方眼と頂点のどちらか短い方の距離を, 差分に得る.
					p = q;
				}
				// 得られた差分の分だけ, 選択された図形を移動する.
				slist_move_selected(m_main_page.m_shape_list, p);
			}
		}
		// 方眼にくっつける
		else if (m_snap_grid) {
			D2D1_POINT_2F p{};	// 方眼との差分
			if (event_get_nearest_grid(m_main_page.m_shape_list, m_main_page.m_grid_base + 1.0f, p)) {
				slist_move_selected(m_main_page.m_shape_list, p);
			}
		}
		// 頂点にくっつける
		else if (m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{};	// 頂点との差分
			if (event_get_nearest_point(m_main_page.m_shape_list, m_snap_point / m_main_scale, p)) {
				slist_move_selected(m_main_page.m_shape_list, p);
			}
		}
	}

	//------------------------------
	// 矩形選択を終了する.
	// key_mod	修飾キー
	//------------------------------
	void MainPage::event_finish_rect_selection(const VirtualKeyModifiers key_mod)
	{
		// 修飾キーがコントロールか判定する.
		if ((key_mod & VirtualKeyModifiers::Control) == VirtualKeyModifiers::Control) {
			D2D1_POINT_2F lt{};
			D2D1_POINT_2F rb{};
			if (m_event_pos_pressed.x < m_event_pos_curr.x) {
				lt.x = m_event_pos_pressed.x;
				rb.x = m_event_pos_curr.x;
			}
			else {
				lt.x = m_event_pos_curr.x;
				rb.x = m_event_pos_pressed.x;
			}
			if (m_event_pos_pressed.y < m_event_pos_curr.y) {
				lt.y = m_event_pos_pressed.y;
				rb.y = m_event_pos_curr.y;
			}
			else {
				lt.y = m_event_pos_curr.y;
				rb.y = m_event_pos_pressed.y;
			}
			if (toggle_shape_inside(lt, rb)) {
				xcvd_menu_is_enabled();
			}
		}
		// 修飾キーが押されてないか判定する.
		else if (key_mod == VirtualKeyModifiers::None) {
			D2D1_POINT_2F lt{};
			D2D1_POINT_2F rb{};
			if (m_event_pos_pressed.x < m_event_pos_curr.x) {
				lt.x = m_event_pos_pressed.x;
				rb.x = m_event_pos_curr.x;
			}
			else {
				lt.x = m_event_pos_curr.x;
				rb.x = m_event_pos_pressed.x;
			}
			if (m_event_pos_pressed.y < m_event_pos_curr.y) {
				lt.y = m_event_pos_pressed.y;
				rb.y = m_event_pos_curr.y;
			}
			else {
				lt.y = m_event_pos_curr.y;
				rb.y = m_event_pos_pressed.y;
			}
			if (select_shape_inside(lt, rb)) {
				xcvd_menu_is_enabled();
			}
		}
		Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
	}

	//------------------------------
	// ポインターが動いた.
	//------------------------------
	void MainPage::event_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_main_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// ピッカーが返値を戻すまで, イベント処理をさせないための排他.
		if (!m_mutex_event.try_lock()) {
			wait_cursor_show();
			return;
		}
		m_mutex_event.unlock();

		event_set_position(args);
		status_bar_set_pos();

		// 状態が, 初期状態か判定する.
		if (m_event_state == EVENT_STATE::BEGIN) {
			event_set_cursor();
		}
		// 状態が. クリックした状態か判定する.
		else if (m_event_state == EVENT_STATE::CLICK) {
			// ポインターの現在点と押された点の間の長さを求め, 長さがクリック判定距離を超えたら初期状態に戻る.
			const auto raw_dpi = DisplayInformation::GetForCurrentView().RawDpiX();
			const auto log_dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			const D2D1_POINT_2F p{
				m_event_pos_curr.x - m_event_pos_pressed.x, m_event_pos_curr.y - m_event_pos_pressed.y
			};
			if (pt_abs2(p) * m_main_scale > m_event_click_dist * raw_dpi / log_dpi) {
				m_event_state = EVENT_STATE::BEGIN;
				event_set_cursor();
			}
		}
		// 状態が, 矩形選択している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_RECT) {
			main_draw();
		}
		// 状態が, 図形を移動している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			// ポインターの現在位置と前回位置の差分だけ, 選択された図形を移動する.
			//D2D1_POINT_2F p;
			//pt_sub(m_event_pos_curr, m_event_pos_prev, p);
			const D2D1_POINT_2F p{
				m_event_pos_curr.x - m_event_pos_prev.x, m_event_pos_curr.y - m_event_pos_prev.y
			};
			slist_move_selected(m_main_page.m_shape_list, p);
			// ポインターの現在位置を前回位置に格納する.
			m_event_pos_prev = m_event_pos_curr;
			main_draw();
		}
		// 状態が, 図形を変形している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_DEFORM) {
			// ポインターの現在位置を, 図形の部位の点に格納する.
			const auto keep_aspect = ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			m_event_shape_pressed->set_pos_loc(m_event_pos_curr, m_event_loc_pressed, 0.0f, keep_aspect);
			// ポインターの現在位置を前回位置に格納する.
			m_event_pos_prev = m_event_pos_curr;
			main_draw();
		}
		// 状態が, 文字列を選択している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_TEXT) {
			bool trail;
			const auto end = m_edit_text_shape->get_text_pos(m_event_pos_curr, trail);
			const auto start = m_main_page.m_select_start;
			undo_push_text_select(m_edit_text_shape, start, end, trail);
			xcvd_menu_is_enabled();
			main_draw();
		}
		// 状態が, 左ボタンを押している状態, または
		// クリック後に左ボタンを押した状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_LBTN || m_event_state == EVENT_STATE::CLICK_LBTN) {
			const auto raw_dpi = DisplayInformation::GetForCurrentView().RawDpiX();
			const auto log_dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			// ポインターの現在位置と押された位置の間の長さが, クリックの判定距離を超えるか判定する.
			//D2D1_POINT_2F p;
			//pt_sub(m_event_pos_curr, m_event_pos_pressed, p);
			const D2D1_POINT_2F p{
				m_event_pos_curr.x - m_event_pos_pressed.x, m_event_pos_curr.y - m_event_pos_pressed.y
			};
			if (pt_abs2(p) * m_main_scale > m_event_click_dist) {
				// 作図ツールが図形作成なら, 矩形選択している状態に遷移する.
				if (m_drawing_tool == DRAWING_TOOL::ARC ||
					m_drawing_tool == DRAWING_TOOL::BEZIER ||
					m_drawing_tool == DRAWING_TOOL::ELLIPSE ||
					m_drawing_tool == DRAWING_TOOL::LINE ||
					m_drawing_tool == DRAWING_TOOL::POLY ||
					m_drawing_tool == DRAWING_TOOL::RECT ||
					m_drawing_tool == DRAWING_TOOL::RRECT ||
					m_drawing_tool == DRAWING_TOOL::RULER ||
					m_drawing_tool == DRAWING_TOOL::TEXT) {
					m_event_state = EVENT_STATE::PRESS_RECT;
				}
				// 押された図形がヌルなら, 矩形選択している状態に遷移する.
				else if (m_drawing_tool == DRAWING_TOOL::SELECT && m_event_shape_pressed == nullptr) {
					m_event_state = EVENT_STATE::PRESS_RECT;
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
				}
				// 選択された図形の数が 1 を超える,
				// または押された図形の部位が線枠, 内側.
				else if (m_drawing_tool == DRAWING_TOOL::SELECT &&
					(m_list_sel_cnt > 1 || m_event_loc_pressed == LOC_TYPE::LOC_STROKE || m_event_loc_pressed == LOC_TYPE::LOC_FILL)) {
					// 図形を移動している状態に遷移する.
					m_event_state = EVENT_STATE::PRESS_MOVE;
					m_event_pos_prev = m_event_pos_curr;
					undo_push_null();
					undo_push_move(p);
				}
				else if (m_drawing_tool == DRAWING_TOOL::SELECT && m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
					m_event_state = EVENT_STATE::PRESS_TEXT;
					bool trail;
					const auto end = m_edit_text_shape->get_text_pos(m_event_pos_curr, trail);
					const auto start = m_main_page.m_select_start;
					undo_push_text_select(m_edit_text_shape, start, end, trail);
				}
				// ポインターが押されたのが図形の外部以外か判定する.
				else if (m_drawing_tool == DRAWING_TOOL::SELECT && m_event_loc_pressed != LOC_TYPE::LOC_PAGE) {
					// 図形を変形している状態に遷移する.
					if (m_event_shape_pressed->is_selected()) {
						m_event_state = EVENT_STATE::PRESS_DEFORM;
						m_event_pos_prev = m_event_pos_curr;
						undo_push_null();
						undo_push_position(m_event_shape_pressed, m_event_loc_pressed);
						const auto keep_aspect = ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
						m_event_shape_pressed->set_pos_loc(m_event_pos_curr, m_event_loc_pressed, 0.0f, keep_aspect);
					}
				}
				xcvd_menu_is_enabled();
				main_draw();
			}
		}
	}

	//------------------------------
	// ポインターのボタンが押された.
	// キャプチャの有無にかかわらず, 片方のマウスボタンを押した状態で, もう一方のボタンを押しても, それは通知されない.
	//------------------------------
	void MainPage::event_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_main_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// ピッカーが返値を戻すまで, イベント処理をさせないための排他.
		if (!m_mutex_event.try_lock()) {
			wait_cursor_show();
			return;
		}
		m_mutex_event.unlock();

		Focus(winrt::Windows::UI::Xaml::FocusState::Programmatic);

		const SwapChainPanel& swap_chain_panel{
			sender.as<SwapChainPanel>()
		};

		// ポインターのキャプチャを始める.
		swap_chain_panel.CapturePointer(args.Pointer());
		const uint64_t t_stamp = args.GetCurrentPoint(swap_chain_panel).Timestamp();	// イベント発生時間
		const PointerPointProperties& p_prop = args.GetCurrentPoint(swap_chain_panel).Properties();	// ポインターの拡張情報
		event_set_position(args);

		// ポインターのデバイスタイプを判定する.
		const auto c_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
		switch (args.GetCurrentPoint(swap_chain_panel).PointerDevice().PointerDeviceType()) {
		// デバイスタイプがマウスの場合
		case PointerDeviceType::Mouse:
			// 拡張情報が右ボタン押下か判定する.
			if (p_prop.IsRightButtonPressed()) {
				m_event_state = EVENT_STATE::PRESS_RBTN;
			}
			// 拡張情報が左ボタン押下か判定する.
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
					if (t_stamp - m_event_time_pressed <= c_time) {
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

		if (scp_main_panel().ContextFlyout() != nullptr) {
			scp_main_panel().ContextFlyout().Hide();
			scp_main_panel().ContextFlyout(nullptr);
		}

		// 押された図形があるなら, その属性をページに反映し, メニューバーを更新する
		if (m_event_loc_pressed != LOC_TYPE::LOC_PAGE) {
			m_main_page.set_attr_to(m_event_shape_pressed);
			stroke_dash_is_checked(m_main_page.m_stroke_dash);
			stroke_width_is_checked(m_main_page.m_stroke_width);
			stroke_cap_is_checked(m_main_page.m_stroke_cap);
			stroke_join_is_checked(m_main_page.m_stroke_join);
			stroke_arrow_is_checked(m_main_page.m_arrow_style);
			font_weight_is_checked(m_main_page.m_font_weight);
			font_stretch_is_checked(m_main_page.m_font_stretch);
			font_style_is_checked(m_main_page.m_font_style);
			text_align_horz_is_checked(m_main_page.m_text_align_horz);
			text_align_vert_is_checked(m_main_page.m_text_align_vert);
			grid_emph_is_checked(m_main_page.m_grid_emph);
			grid_show_is_checked(m_main_page.m_grid_show);
		}

		bool changed = false;
		if (m_event_state == EVENT_STATE::PRESS_LBTN) {
			if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				if ((args.KeyModifiers() & VirtualKeyModifiers::Control) == VirtualKeyModifiers::Control) {
					// 制御キー押下でのマウスボタン押下は, 押された図形が選択されていたなら外し, 選択されていなかったなら付ける.
					// ただし, 押された図形の部位が, 枠線や塗りつぶし以外の部位だったならば, 制御キー押下での変形を行なう.
					m_event_shape_pressed = nullptr;
					m_event_loc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, true, m_event_shape_pressed);
					if (m_event_loc_pressed == LOC_TYPE::LOC_PAGE) {
						/////
						// 編集中の文字列図形があるならフォーカスを取り消す.
						if (m_edit_text_shape != nullptr) {
							m_edit_context.NotifyFocusLeave();
							undo_push_text_unselect(m_edit_text_shape);
							m_edit_text_shape = nullptr;
							changed = true;
						}
					}
					else if (m_event_loc_pressed == LOC_TYPE::LOC_STROKE || m_event_loc_pressed == LOC_TYPE::LOC_FILL) {
						if (!m_event_shape_pressed->is_selected()) {
							undo_push_select(m_event_shape_pressed);
							m_event_shape_last = m_event_shape_pressed;
							if (summary_is_visible()) {
								summary_select(m_event_shape_pressed);
							}
						}
						else {
							undo_push_select(m_event_shape_pressed);
							if (summary_is_visible()) {
								summary_unselect(m_event_shape_pressed);
							}
						}
						/////
						// 編集中の文字列図形があるならフォーカスを取り消す.
						if (m_edit_text_shape != nullptr) {
							m_edit_context.NotifyFocusLeave();
							undo_push_text_unselect(m_edit_text_shape);
							m_edit_text_shape = nullptr;
						}
						changed = true;
					}
					else if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
						if (!m_event_shape_pressed->is_selected()) {
							undo_push_select(m_event_shape_pressed);
							m_event_shape_last = m_event_shape_pressed;
							if (summary_is_visible()) {
								summary_select(m_event_shape_pressed);
							}
						}
						else {
							undo_push_select(m_event_shape_pressed);
							if (summary_is_visible()) {
								summary_unselect(m_event_shape_pressed);
							}
						}
						/////
						// 編集中の文字列図形があるならフォーカスを取り消す.
						if (m_edit_text_shape != nullptr) {
							m_edit_context.NotifyFocusLeave();
							undo_push_text_unselect(m_edit_text_shape);
							m_edit_text_shape = nullptr;
						}
						changed = true;
					}
					else {
						////
						// 編集中の文字列図形があるならフォーカスを取り消す.
						if (m_edit_text_shape != nullptr) {
							m_edit_context.NotifyFocusLeave();
							undo_push_text_unselect(m_edit_text_shape);
							m_edit_text_shape = nullptr;
							changed = true;
						}
					}
				}
				else if ((args.KeyModifiers() & VirtualKeyModifiers::Shift) == VirtualKeyModifiers::Shift) {
					m_event_shape_pressed = nullptr;
					m_event_loc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, false, m_event_shape_pressed);
					if (m_event_loc_pressed == LOC_TYPE::LOC_PAGE) {
						////
						// 編集中の文字列図形があるならフォーカスを取り消す.
						if (m_edit_text_shape != nullptr) {
							m_edit_context.NotifyFocusLeave();
							undo_push_text_unselect(m_edit_text_shape);
							m_edit_text_shape = nullptr;
							changed = true;
						}
					}
					else if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
						////
						// 押された図形は編集中の図形
						if (m_edit_text_shape == m_event_shape_pressed) {
							//押された点からキャレット位置を設定し, 文字列を選択する.
							const ShapeText* t = m_edit_text_shape;
							bool trail;
							const auto end = t->get_text_pos(m_event_pos_curr, trail);
							undo_push_text_select(m_edit_text_shape, m_main_page.m_select_start, end, trail);
							changed = true;
						}
						// 押された図形は編集中の図形以外
						else if (m_edit_text_shape != m_event_shape_pressed) {
							if (select_shape_range(m_event_shape_last, m_event_shape_pressed)) {
								xcvd_menu_is_enabled();
								changed = true;
							}
							// 現在に編集中の文字列があれば中断し, 押された文字列を編集中の図形にする.
							if (m_edit_text_shape != nullptr) {
								m_edit_context.NotifyFocusLeave();
								undo_push_text_unselect(m_edit_text_shape);
								m_edit_text_shape = nullptr;
								changed = true;
							}
						}
					}
					else {
						// 最後に押された図形があるなら, 指定した範囲の図形を選択, 範囲外の図形の選択を外す.
						if (m_event_shape_last != nullptr) {
							if (select_shape_range(m_event_shape_last, m_event_shape_pressed)) {
								changed = true;
							}
						}
						else {
							if (!m_event_shape_pressed->is_selected()) {
								undo_push_select(m_event_shape_pressed);
								changed = true;
							}

						}
						////
						// 現在に編集中の文字列があれば中断する.
						if (m_edit_text_shape != nullptr) {
							m_edit_context.NotifyFocusLeave();
							undo_push_text_unselect(m_edit_text_shape);
							m_edit_text_shape = nullptr;
							changed = true;
						}
					}
				}
				else {
					m_event_shape_pressed = nullptr;
					m_event_loc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, false, m_event_shape_pressed);
					// 押された図形はない.
					if (m_event_loc_pressed == LOC_TYPE::LOC_PAGE) {
						if (unselect_shape_all()) {
							m_event_shape_last = nullptr;
							////
							if (m_edit_text_shape != nullptr) {
								m_edit_context.NotifyFocusLeave();
								undo_push_text_unselect(m_edit_text_shape);
								m_edit_text_shape = nullptr;
							}
							changed = true;
						}
					}
					// 押された図形は文字列.
					else if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
						bool take_over = !m_event_shape_pressed->is_selected();
						if (take_over) {
							m_event_shape_last = m_event_shape_pressed;
							unselect_shape_all();
							undo_push_select(m_event_shape_pressed);
							if (summary_is_visible()) {
								summary_select(m_event_shape_pressed);
							}
							changed = true;
						}
						// 押された図形がすでに選択ずみなら
						else {
							m_event_shape_last = m_event_shape_pressed;
						}
						if (m_list_sel_cnt > 1) {
							if (m_edit_text_shape != nullptr) {
								m_edit_context.NotifyFocusLeave();
								undo_push_text_unselect(m_edit_text_shape);
								m_edit_text_shape = nullptr;
								changed = true;
							}
						}
						else {
							///////
							if (m_edit_text_shape != m_event_shape_pressed) {
								if (m_edit_text_shape != nullptr) {
									m_edit_context.NotifyFocusLeave();
									undo_push_text_unselect(m_edit_text_shape);
								}
								m_edit_text_shape = static_cast<ShapeText*>(m_event_shape_pressed);
							}
							bool trail;
							const auto end = m_edit_text_shape->get_text_pos(m_event_pos_curr, trail);
							const auto start = trail ? end + 1 : end;
							undo_push_text_select(m_edit_text_shape, start, end, trail);
							m_edit_context.NotifyFocusEnter();
							changed = true;
						}
					}
					// 押された図形は文字列以外.
					else {
						bool take_over = !m_event_shape_pressed->is_selected();
						if (take_over) {
							m_event_shape_last = m_event_shape_pressed;
							unselect_shape_all();
							undo_push_select(m_event_shape_pressed);
							if (summary_is_visible()) {
								summary_select(m_event_shape_pressed);
							}
							changed = true;
						}
						// 押された図形がすでに選択ずみなら
						else {
							m_event_shape_last = m_event_shape_pressed;
						}
						////
						if (m_edit_text_shape != nullptr) {
							m_edit_context.NotifyFocusLeave();
							undo_push_text_unselect(m_edit_text_shape);
							m_edit_text_shape = nullptr;
							changed = true;
						}
					}
				}
			}
			else if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
			}
			else {

			}
		}
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
		}
		if (changed) {
			//undo_menu_is_enabled();
			xcvd_menu_is_enabled();
			main_draw();
		}
		/*
		if (m_event_state == EVENT_STATE::PRESS_LBTN) {
			if (args.KeyModifiers() == VirtualKeyModifiers::Control) {
				if (m_edit_text_shape != nullptr) {
					m_edit_context.NotifyFocusLeave();
					m_edit_text_shape = nullptr;
				}
				m_event_loc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, true, m_event_shape_pressed);
				// 押された部位が, 図形の線枠または塗りつぶしなら, その図形の選択を反転する.
				// 逆にいうなら, コントロール押下のときでも頂点は制御点は移動できるので, 図形が選択されたままする必要がある.
				if (m_event_loc_pressed == LOC_TYPE::LOC_STROKE || m_event_loc_pressed == LOC_TYPE::LOC_FILL) {
					undo_push_select(m_event_shape_pressed);
					xcvd_menu_is_enabled();
					main_draw();
					// 一覧が表示されてるか判定する.
					if (summary_is_visible()) {
						if (m_event_shape_pressed->is_selected()) {
							summary_select(m_event_shape_pressed);
						}
						else {
							summary_unselect(m_event_shape_pressed);
						}
					}
					m_event_shape_last = m_event_shape_pressed;
				}
			}
			else if (args.KeyModifiers() == VirtualKeyModifiers::Shift) {
				m_event_loc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, false, m_event_shape_pressed);
				if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
					if (m_edit_text_shape == m_event_shape_pressed) {
						bool trail;
						const auto end = m_edit_text_shape->get_text_pos(m_event_pos_curr, trail);
						const auto start = m_edit_text_shape->m_select_start;
						undo_push_text_select(m_edit_text_shape, start, end, trail);
					}
					else {
						if (m_edit_text_shape != nullptr) {
							m_edit_context.NotifyFocusLeave();
							m_edit_text_shape = nullptr;
						}
						m_edit_text_shape = static_cast<ShapeText*>(m_event_shape_pressed);
						bool trail;
						const auto end = m_edit_text_shape->get_text_pos(m_event_pos_curr, trail);
						const auto start = trail ? end + 1 : end;
						undo_push_text_select(m_edit_text_shape, start, end, trail);
					}
				}
				else {
					if (m_edit_text_shape != nullptr) {
						m_edit_context.NotifyFocusLeave();
						m_edit_text_shape = nullptr;
					}
					// 範囲の中の図形は選択して, それ以外の図形の選択をはずす.
					if (select_shape_range(m_event_shape_pressed, m_event_shape_last)) {
						xcvd_menu_is_enabled();
						main_draw();
					}
				}
			}
			else if (args.KeyModifiers() == VirtualKeyModifiers::None) {
				m_event_loc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, false, m_event_shape_pressed);
				if (m_drawing_tool == DRAWING_TOOL::SELECT || m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
					if (m_event_shape_last != m_event_shape_pressed) {
						// 押された図形の属性をページに反映する.
						m_main_page.set_attr_to(m_event_shape_pressed);
						// メニューバーを更新する
						stroke_dash_is_checked(m_main_page.m_stroke_dash);
						stroke_width_is_checked(m_main_page.m_stroke_width);
						stroke_cap_is_checked(m_main_page.m_stroke_cap);
						stroke_join_is_checked(m_main_page.m_stroke_join);
						stroke_arrow_is_checked(m_main_page.m_arrow_style);
						font_weight_is_checked(m_main_page.m_font_weight);
						font_stretch_is_checked(m_main_page.m_font_stretch);
						font_style_is_checked(m_main_page.m_font_style);
						text_align_horz_is_checked(m_main_page.m_text_align_horz);
						text_align_vert_is_checked(m_main_page.m_text_align_vert);
						grid_emph_is_checked(m_main_page.m_grid_emph);
						grid_show_is_checked(m_main_page.m_grid_show);
					}
				}
				if (m_drawing_tool == DRAWING_TOOL::SELECT) {
					m_event_shape_last = m_event_shape_pressed;
					if (m_event_shape_pressed == nullptr) {
						unselect_shape_all();
					}
					else if (!m_event_shape_pressed->is_selected()) {
						bool sele_changed = false;	// 図形の選択が変更されたか判定
						for (auto s : m_main_page.m_shape_list) {
							if (s->is_deleted()) {
								continue;
							}
							// 図形が押された図形でかつ選択されてないなら, 図形を選択する.
							if (s == m_event_shape_pressed && !s->is_selected()) {
								undo_push_select(s);
								sele_changed = true;
							}
							// 図形が押された図形以外でかつ選択されるなら, 図形の選択をはずす.
							else if (s != m_event_shape_pressed && s->is_selected()) {
								undo_push_select(s);
								sele_changed = true;
							}
						}
						if (sele_changed) {
							// 一覧が表示されてるなら一覧も変更する.
							if (summary_is_visible()) {
								summary_select(m_event_shape_pressed);
							}
							xcvd_menu_is_enabled();
						}
					}
					// 押された部位が文字列だったなら.
					if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
						m_event_state = EVENT_STATE::PRESS_TEXT;
						// 押された図形が編集対象の図形以外なら,
						bool edit_change = false;
						if (m_edit_text_shape == nullptr || m_edit_text_shape != static_cast<ShapeText*>(m_event_shape_pressed)) {
							// 編集対象の図形があるならフォーカスをはずす.
							//if (m_edit_text_shape != nullptr) {
							//	m_edit_context.NotifyFocusLeave();
							//}
							//m_edit_context.NotifyFocusEnter();
							// 押された図形をあらたな編集対象の図形とする.
							m_edit_text_shape = static_cast<ShapeText*>(m_event_shape_pressed);
							bool trail;
							const auto end = m_edit_text_shape->get_text_pos(m_event_pos_curr, trail);
							const auto start = trail ? end + 1 : end;
							undo_push_text_select(m_edit_text_shape, start, end, trail);
						}
						// 押された図形が編集対象の図形なら,
						else {
							bool trail;
							const auto end = m_edit_text_shape->get_text_pos(m_event_pos_curr, trail);
							const auto start = trail ? end + 1 : end;
							undo_push_text_select(m_edit_text_shape, start, end, trail);
						}
					}
					// 押された部位が文字列以外なら.
					else {
						// 編集対象の図形があるならフォーカスをはずす.
						if (m_edit_text_shape != nullptr) {
							m_edit_context.NotifyFocusLeave();
							m_edit_text_shape = nullptr;
						}
					}
					main_draw();
					xcvd_menu_is_enabled();
					main_draw();
				}
				else {
					if (m_edit_text_shape != nullptr) {
						m_edit_context.NotifyFocusLeave();
						m_edit_text_shape = nullptr;
					}
				}
			}
			else {
				m_event_state = EVENT_STATE::BEGIN;
				m_event_loc_pressed = LOC_TYPE::LOC_PAGE;
				if (m_edit_text_shape != nullptr) {
					m_edit_context.NotifyFocusLeave();
					m_edit_text_shape = nullptr;
				}
				m_event_shape_pressed = nullptr;
			}
		}
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
			m_main_page.set_attr_to(m_event_shape_pressed);
			// メニューバーを更新する
			stroke_dash_is_checked(m_main_page.m_stroke_dash);
			stroke_width_is_checked(m_main_page.m_stroke_width);
			stroke_cap_is_checked(m_main_page.m_stroke_cap);
			stroke_join_is_checked(m_main_page.m_stroke_join);
			stroke_arrow_is_checked(m_main_page.m_arrow_style);
			font_weight_is_checked(m_main_page.m_font_weight);
			font_stretch_is_checked(m_main_page.m_font_stretch);
			font_style_is_checked(m_main_page.m_font_style);
			text_align_horz_is_checked(m_main_page.m_text_align_horz);
			text_align_vert_is_checked(m_main_page.m_text_align_vert);
			grid_emph_is_checked(m_main_page.m_grid_emph);
			grid_show_is_checked(m_main_page.m_grid_show);
		}
		else if (m_event_state == EVENT_STATE::CLICK_LBTN) {

		}
		*/
		/*
		// 作図ツールが選択ツールか判定する.
		if (m_drawing_tool == DRAWING_TOOL::SELECT || m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
			m_event_loc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, m_event_shape_pressed);
			m_main_page.set_attr_to(m_event_shape_pressed);
			// メニューバーを更新する
			stroke_dash_is_checked(m_main_page.m_stroke_dash);
			stroke_width_is_checked(m_main_page.m_stroke_width);
			stroke_cap_is_checked(m_main_page.m_stroke_cap);
			stroke_join_is_checked(m_main_page.m_stroke_join);
			stroke_arrow_is_checked(m_main_page.m_arrow_style);
			font_weight_is_checked(m_main_page.m_font_weight);
			font_stretch_is_checked(m_main_page.m_font_stretch);
			font_style_is_checked(m_main_page.m_font_style);
			text_align_horz_is_checked(m_main_page.m_text_align_horz);
			text_align_vert_is_checked(m_main_page.m_text_align_vert);
			grid_emph_is_checked(m_main_page.m_grid_emph);
			grid_show_is_checked(m_main_page.m_grid_show);
			if (m_event_state == EVENT_STATE::PRESS_LBTN) {
				if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
					m_event_state = EVENT_STATE::PRESS_TEXT;
					if (m_edit_text_shape == nullptr || m_edit_text_shape != static_cast<ShapeText*>(m_event_shape_pressed)) {
						//if (m_edit_text_shape != nullptr) {
						//	m_edit_context.NotifyFocusLeave();
						//}
						m_edit_text_shape = static_cast<ShapeText*>(m_event_shape_pressed);
						bool trail;
						const auto end = m_edit_text_shape->get_text_pos(m_event_pos_curr, trail);
						const auto start = trail ? end + 1 : end;
						undo_push_text_select(m_edit_text_shape, start, end, trail);
						main_draw();
					}
					else {
						bool trail;
						const auto end = m_edit_text_shape->get_text_pos(m_event_pos_curr, trail);
						const auto start = trail ? end + 1 : end;
						undo_push_text_select(m_edit_text_shape, start, end, trail);
						main_draw();
					}
				}
				else {
					// 押された部位が文字列以外なら編集終了
					if (m_edit_text_shape != nullptr) {
						m_edit_context.NotifyFocusLeave();
						m_edit_text_shape = nullptr;
						main_draw();
					}
				}
				// 押されたのが図形の外側か判定する.
				if (m_event_loc_pressed == LOC_TYPE::LOC_PAGE) {
					m_event_shape_pressed = nullptr;
					m_event_shape_last = nullptr;
					// 修飾キーが押されていないならば, すべての図形の選択を解除する.
					// 解除された図形があるか判定する.
					if (args.KeyModifiers() == VirtualKeyModifiers::None && unselect_shape_all()) {
						xcvd_menu_is_enabled();
						main_draw();
					}
				}
				else {
					select_shape(m_event_shape_pressed, args.KeyModifiers());
				}
			}
			else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
				// 右ボタン押下なら何もしない
			}
			else {
				// 状態が左ボタンが押された状態, または, 右ボタンが押されていてかつ押された図形が選択されてないか判定する.
				if (m_event_state == EVENT_STATE::PRESS_LBTN ||
					(m_event_state == EVENT_STATE::PRESS_RBTN && !m_event_shape_pressed->is_selected())) {
					if (scp_main_panel().ContextFlyout() != nullptr) {
						scp_main_panel().ContextFlyout().Hide();
						scp_main_panel().ContextFlyout(nullptr);
					}
					select_shape(m_event_shape_pressed, args.KeyModifiers());
					// 編集中の文字列があるなら, 空にしてフォーカスをはずす.
					if (m_edit_text_shape != nullptr && (m_edit_text_shape->is_deleted() || !m_edit_text_shape->is_selected())) {
						m_edit_text_shape = nullptr;
						m_edit_context.NotifyFocusLeave();
						InputPane::GetForCurrentView().TryHide();
					}
				}
			}
		}
		*/
	}

	//------------------------------
	// ポインターのボタンが上げられた.
	//------------------------------
	void MainPage::event_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
//#if defined(_DEBUG)
		if (sender != scp_main_panel()) {
			return;
		}
//#endif
		// ピッカーが返値を戻すまで, イベント処理をさせないための排他.
		if (!m_mutex_event.try_lock()) {
			wait_cursor_show();
			return;
		}
		m_mutex_event.unlock();

		const auto& panel = sender.as<SwapChainPanel>();

		// ポインターの追跡を停止する.
		panel.ReleasePointerCaptures();
		event_set_position(args);
		// 状態が, 左ボタンが押された状態か判定する.
		if (m_event_state == EVENT_STATE::PRESS_LBTN) {
			// ボタンが離れた時刻と押された時刻の差が, クリックの判定時間以下か判定する.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			const auto c_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
			if (t_stamp - m_event_time_pressed <= c_time) {
				// クリックした状態に遷移する.
				m_event_state = EVENT_STATE::CLICK;
				event_set_cursor();
			}
			// クリックの確定
			if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
				event_eyedropper_detect(m_event_shape_pressed, m_event_loc_pressed);
				status_bar_set_draw();
			}
			return;
		}
		// 状態が, クリック後に左ボタンが押した状態か判定する.
		else if (m_event_state == EVENT_STATE::CLICK_LBTN) {
			// ボタンが離された時刻と押された時刻の差分を得る.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			const auto c_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
			// 差分がクリックの判定時間以下, かつ押された図形が文字列図形か判定する.
			if (t_stamp - m_event_time_pressed <= c_time && m_event_shape_pressed != nullptr) {
				// ダブルクリックの確定.
				//if (typeid(*m_event_shape_pressed) == typeid(ShapeText)) {
				//	meth_text_edit_click_async(nullptr, nullptr);
				//}
				//else if (typeid(*m_event_shape_pressed) == typeid(ShapeArc)) {
				//	meth_arc_click_async(nullptr, nullptr);
				//}
			}
		}
		// 状態が, 文字列を押している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_TEXT) {
			bool trail;
			const auto end = m_edit_text_shape->get_text_pos(m_event_pos_curr, trail);
			const auto start = m_main_page.m_select_start;
			undo_push_text_select(m_edit_text_shape, start, end, trail);
		}
		// 状態が, 図形を移動している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			if ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control) {
				event_finish_moving();
			}
			//undo_push_null();
			if (!undo_pop_invalid()) {
				//undo_menu_is_enabled();
				main_bbox_update();
				main_panel_size();
			}
		}
		// 状態が, 図形を変形している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_DEFORM) {
			if ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control) {
				event_finish_deforming(args.KeyModifiers());
			}
			undo_push_null();
			if (!undo_pop_invalid()) {
				//undo_menu_is_enabled();
				main_bbox_update();
				main_panel_size();
			}
		}
		// 状態が, 矩形選択している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_RECT) {
			// 作図ツールが選択ツールか判定する.
			if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				event_finish_rect_selection(args.KeyModifiers());
			}
			// 作図ツールが選択ツール以外.
			else {
				// 制御キーが押されてないなら, ポインター点を頂点または方眼に合わせる.
				if ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control) {
					// 点と点をくっつける閾値がゼロより大きいなら, ほかの頂点に合わせる.
					if (m_snap_point >= FLT_MIN) {
						const bool boxed = (
							m_drawing_tool == DRAWING_TOOL::ELLIPSE ||
							m_drawing_tool == DRAWING_TOOL::POLY ||
							m_drawing_tool == DRAWING_TOOL::RECT ||
							m_drawing_tool == DRAWING_TOOL::RRECT ||
							m_drawing_tool == DRAWING_TOOL::RULER ||
							m_drawing_tool == DRAWING_TOOL::TEXT
							);
						const float interval = m_snap_point / m_main_scale;
						const double g_len = max(m_main_page.m_grid_base, 0.0) + 1.0;
						event_snap_point(m_main_page.m_shape_list, boxed, interval, m_snap_grid, g_len, m_event_pos_pressed, m_event_pos_curr);
					}
					// 方眼に合わせるか判定する.
					else if (m_snap_grid) {
						// 押された位置よ離された位置を方眼の大きさで丸める.
						const double g_len = max(m_main_page.m_grid_base + 1.0, 1.0);
						pt_round(m_event_pos_pressed, g_len, m_event_pos_pressed);
						pt_round(m_event_pos_curr, g_len, m_event_pos_curr);
					}
				}
				// ポインターの現在位置と押された位置の差分の x 値または y 値が 1 以上か判定する.
				const D2D1_POINT_2F p{
					m_event_pos_curr.x - m_event_pos_pressed.x,
					m_event_pos_curr.y - m_event_pos_pressed.y
				};
				if (fabs(p.x) >= 1.0f || fabs(p.y) >= 1.0f) {
					event_finish_creating(m_event_pos_pressed, p);
				}
				else {
					unselect_shape_all();
				}
			}
		}
		// 状態が, 右ボタンを押した状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
			if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
				Shape* s;
				const uint32_t loc = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, false, s);
				if (loc == LOC_TYPE::LOC_PAGE) {
					undo_push_null();
					undo_push_set<UNDO_T::PAGE_COLOR>(&m_main_page, m_eyedropper_color);
					//undo_menu_is_enabled();
				}
				else if (s != nullptr) {
					if (m_event_loc_pressed == LOC_TYPE::LOC_FILL) {
						undo_push_null();
						undo_push_set<UNDO_T::FILL_COLOR>(s, m_eyedropper_color);
						//undo_menu_is_enabled();
					}
					else if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
						undo_push_null();
						undo_push_set<UNDO_T::FONT_COLOR>(s, m_eyedropper_color);
						//undo_menu_is_enabled();
					}
					else if (m_event_loc_pressed == LOC_TYPE::LOC_STROKE) {
						undo_push_null();
						undo_push_set<UNDO_T::STROKE_COLOR>(s, m_eyedropper_color);
						//undo_menu_is_enabled();
					}
				}
			}
			else {
				m_event_loc_r_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, false, m_event_shape_r_pressed);
				// ポップアップ (フライアウト) が表示されているならそれを解放する.
				if (scp_main_panel().ContextFlyout() != nullptr) {
					scp_main_panel().ContextFlyout().Hide();
					scp_main_panel().ContextFlyout(nullptr);
				}
				// ポップアップメニューを表示する.
				scp_main_panel().ContextFlyout(mf_popup_menu());
			}
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
		m_event_loc_pressed = LOC_TYPE::LOC_PAGE;
		main_draw();
	}

	//------------------------------
	// カーソルを設定する.
	//------------------------------
	void MainPage::event_set_cursor(void)
	{
		// 作図ツールが色抽出.
		if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
			const CoreCursor& curs = m_eyedropper_filled ? CURS_EYEDROPPER2 : CURS_EYEDROPPER1;
			Window::Current().CoreWindow().PointerCursor(curs);
		}
		// 作図ツールが選択ツール以外かつ状態が右ボタン押下でない.
		else if (m_drawing_tool != DRAWING_TOOL::SELECT && m_event_state != EVENT_STATE::PRESS_RBTN) {
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
			const auto loc = slist_hit_test(m_main_page.m_shape_list, m_event_pos_curr, false, s);
			if (loc == LOC_TYPE::LOC_PAGE) {
				Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
			}
			else if (m_list_sel_cnt > 1) {
				Window::Current().CoreWindow().PointerCursor(CURS_SIZE_ALL);
			}
			else if (loc == LOC_TYPE::LOC_TEXT) {
				Window::Current().CoreWindow().PointerCursor(CURS_IBEAM);
			}
			else {
				switch (loc) {
				case LOC_TYPE::LOC_A_START:
				case LOC_TYPE::LOC_A_END:
				case LOC_TYPE::LOC_A_AXIS_X:
				case LOC_TYPE::LOC_A_AXIS_Y:
				case LOC_TYPE::LOC_R_NW:
				case LOC_TYPE::LOC_R_NE:
				case LOC_TYPE::LOC_R_SE:
				case LOC_TYPE::LOC_R_SW:
				case LOC_TYPE::LOC_START:
				case LOC_TYPE::LOC_END:
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
					break;
				case LOC_TYPE::LOC_A_CENTER:
				case LOC_TYPE::LOC_FILL:
				case LOC_TYPE::LOC_STROKE:
				case LOC_TYPE::LOC_TEXT:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_ALL);
					break;
				case LOC_TYPE::LOC_NE:
				case LOC_TYPE::LOC_SW:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_NESW);
					break;
				case LOC_TYPE::LOC_NORTH:
				case LOC_TYPE::LOC_SOUTH:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_NS);
					break;
				case LOC_TYPE::LOC_NW:
				case LOC_TYPE::LOC_SE:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_NWSE);
					break;
				case LOC_TYPE::LOC_WEST:
				case LOC_TYPE::LOC_EAST:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_WE);
					break;
				default:
					// 図形のクラスが, 多角形または曲線であるか判定する.
					if (s != nullptr) {
						if (typeid(*s) == typeid(ShapeLine) ||
							typeid(*s) == typeid(ShapePoly) ||
							typeid(*s) == typeid(ShapeBezier) ||
							typeid(*s) == typeid(ShapeArc)) {
							// 図形の部位が, 頂点の数を超えないか判定する.
							if (loc >= LOC_TYPE::LOC_P0 && loc < LOC_TYPE::LOC_P0 + static_cast<ShapePath*>(s)->m_pos.size() + 1) {
								Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
								break;
							}
						}
					}
					throw winrt::hresult_invalid_argument();
					break;
				}
			}
		}
	}

	//------------------------------
	// ポインターの現在位置に格納する.
	//------------------------------
	void MainPage::event_set_position(PointerRoutedEventArgs const& args)
	{
		const auto scale = m_main_scale;
		// 境界ボックスの左上点にスクロールの値を加え, 表示されている左上点を得る.
		D2D1_POINT_2F q;
		pt_add(m_main_bbox_lt, sb_horz().Value(), sb_vert().Value(), q);
		// 引数として渡された点に,
		// 拡大率の逆数を乗じ, 表示されている左上点を加えた点を得る.
		// 得られた点を, ポインターの現在位置に格納する.
		const auto p{ args.GetCurrentPoint(scp_main_panel()).Position() };
		pt_mul_add(D2D1_POINT_2F{ p.X, p.Y }, 1.0 / scale, q, m_event_pos_curr);
	}

	//------------------------------
	// ポインターのホイールボタンが操作された.
	//------------------------------
	void MainPage::event_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_main_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// ピッカーが返値を戻すまで, イベント処理をさせないための排他.
		if (!m_mutex_event.try_lock()) {
			wait_cursor_show();
			return;
		}
		m_mutex_event.unlock();

		// 制御キーが押されてるか判定する.
		const auto mod = args.KeyModifiers();
		if ((mod & VirtualKeyModifiers::Control) == VirtualKeyModifiers::Control) {
			// 拡大縮小
			const int32_t w_delta = args.GetCurrentPoint(scp_main_panel()).Properties().MouseWheelDelta();
			if (w_delta > 0 && m_main_scale < 16.f / 1.1f - FLT_MIN) {
			//if (w_delta > 0 && m_main_page.m_page_scale < 16.f / 1.1f - FLT_MIN) {
				//m_main_page.m_page_scale *= 1.1f;
				//zoom_is_cheched(m_main_page.m_page_scale);
				m_main_scale *= 1.1f;
				page_zoom_is_checked(m_main_scale);
				main_panel_size();
				main_draw();
				status_bar_set_pos();
				status_bar_set_zoom();
			}
			else if (w_delta < 0 && m_main_scale > 0.25f * 1.1f + FLT_MIN) {
			//else if (w_delta < 0 && m_main_page.m_page_scale > 0.25f * 1.1f + FLT_MIN) {
				//m_main_page.m_page_scale /= 1.1f;
				//zoom_is_cheched(m_main_page.m_page_scale);
				m_main_scale /= 1.1f;
				page_zoom_is_checked(m_main_scale);
				main_panel_size();
				main_draw();
				status_bar_set_pos();
				status_bar_set_zoom();
			}
		}
		// シフトキーが押されてるか判定する.
		else if ((mod & VirtualKeyModifiers::Shift) == VirtualKeyModifiers::Shift) {
			// 横スクロール.
			const int32_t w_delta = args.GetCurrentPoint(scp_main_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_horz(), w_delta, m_main_scale)) {
				main_draw();
				status_bar_set_pos();
			}
		}
		// 何も押されてないか判定する.
		else if (mod == VirtualKeyModifiers::None) {
			// 縦スクロール.
			const int32_t w_delta = args.GetCurrentPoint(scp_main_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_vert(), w_delta, m_main_scale)) {
				main_draw();
				status_bar_set_pos();
			}
		}
	}

}