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
	using winrt::Windows::UI::Xaml::FocusState;

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
	static bool event_undo_contain_shape(const UNDO_STACK& ustack, const SHAPE* s) noexcept;
	// 図形リストを整理する.
	static void event_reduce_slist(SHAPE_LIST& slist, const UNDO_STACK& ustack, const UNDO_STACK& r_stack) noexcept;
	// マウスホイールの値でスクロールする.
	static bool event_scroll_by_wheel_delta(const ScrollBar& scroll_bar, const int32_t delta, const float scale);

	// 点をそろえる.
	// slist	図形リスト
	// is_box	調整の対象を, 図形を囲む矩形とするなら true, 図形の頂点を対象とするなら false 
	// interval	制限距離
	// g_snap	方眼にそろえる.
	// g_len	方眼の大きさ
	// pressed	押された位置
	// released	離された位置
	static void event_snap_point(const SHAPE_LIST& slist, const bool is_box, const double interval, const bool g_snap, const double g_len, D2D1_POINT_2F& pressed, D2D1_POINT_2F& released)
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
	// slist	図形リスト
	// g_len	方眼の大きさ
	// to_grid	図形から方眼への位置ベクトル
	static bool event_get_nearest_grid(const SHAPE_LIST& slist, const float g_len, D2D1_POINT_2F& to_grid) noexcept
	{
		D2D1_POINT_2F p[2 + N_GON_MAX];
		D2D1_POINT_2F g;	// 方眼の大きさで丸めた位置
		D2D1_POINT_2F to;	// 丸めた位置と元の位置の差分
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
				pt_sub(g, p[i], to);
				const auto g_abs = pt_abs2(to);	// 丸めた位置と元の位置の距離の二乗
				if (g_abs < d_min) {
					to_grid = to;
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
	// slist	図形リスト
	// interval	間隔 (これ以上離れた点は無視する)
	// to_pt	最も近い頂点への位置ベクトル
	// 戻り値	見つかったなら true
	static bool event_get_nearest_point(
		const SHAPE_LIST& slist,	// 図形リスト
		const double interval,	// 間隔 (これ以上離れた点は無視する)
		D2D1_POINT_2F& to_pt	// 最も近い頂点への位置ベクトル
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
			pt_sub(n, q, to_pt);
		}
		return flag;
	}

	// マウスホイールの値でスクロールする.
	// scroll_bar	スクロールバー
	// t delta	マウスホイールのデルタ値
	// scale	用紙の倍率
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

	// 図形リストを整理する.
	// slist	図形リスト
	// ustack	元に戻す操作スタック
	// rstack	やり直す操作スタック
	static void event_reduce_slist(SHAPE_LIST& slist, const UNDO_STACK& ustack, const UNDO_STACK& rstack) noexcept
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
	// ustack	操作スタック
	// s	図形
	// 戻り値	含む場合 true.
	static bool event_undo_contain_shape(const UNDO_STACK& ustack, const SHAPE* s) noexcept
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
			status_bar_set_pointer();
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
		const SHAPE* s,	// 押された図形
		const uint32_t hit	// 押された判定部位
	)
	{
		if (hit == HIT_TYPE::HIT_SHEET) {
			m_eyedropper_color = m_main_sheet.m_sheet_color;
			m_eyedropper_filled = true;
			Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
		}
		else if (s != nullptr) {
			if (hit == HIT_TYPE::HIT_FILL) {
				if (typeid(*s) == typeid(SHAPE_IMAGE)) {
					if (static_cast<const SHAPE_IMAGE*>(s)->get_pixcel(m_event_point_pressed, m_eyedropper_color)) {
						m_main_sheet.m_fill_color = m_eyedropper_color;
						m_eyedropper_filled = true;
					}
				}
				else {
					s->get_fill_color(m_eyedropper_color);
					m_main_sheet.m_fill_color = m_eyedropper_color;
					m_eyedropper_filled = true;
				}
				Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
			}
			else if (hit == HIT_TYPE::HIT_TEXT) {
				s->get_font_color(m_eyedropper_color);
				m_main_sheet.m_font_color = m_eyedropper_color;
				m_eyedropper_filled = true;
				Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
			}
			else if (hit == HIT_TYPE::HIT_STROKE) {
				s->get_stroke_color(m_eyedropper_color);
				m_main_sheet.m_stroke_color = m_eyedropper_color;
				m_eyedropper_filled = true;
				Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
			}
		}
	}

	//------------------------------
	// 図形の作成を終了する.
	// start	始点
	// end_to	対角点への位置ベクトル
	//------------------------------
	void MainPage::event_finish_creating(const D2D1_POINT_2F start, const D2D1_POINT_2F lineto)
	{
		const auto drawing_tool = m_tool;
		SHAPE* s;
		if (drawing_tool == DRAWING_TOOL::RECT) {
			s = new ShapeRect(start, lineto, &m_main_sheet);
		}
		else if (drawing_tool == DRAWING_TOOL::RRECT) {
			s = new ShapeRRect(start, lineto, &m_main_sheet);
		}
		else if (drawing_tool == DRAWING_TOOL::POLY) {
			const auto poly_opt = m_tool_polygon;
			s = new ShapePoly(start, lineto, &m_main_sheet, poly_opt);
		}
		else if (drawing_tool == DRAWING_TOOL::ELLIPSE) {
			s = new ShapeEllipse(start, lineto, &m_main_sheet);
		}
		else if (drawing_tool == DRAWING_TOOL::LINE) {
			s = new ShapeLine(start, lineto, &m_main_sheet);
		}
		else if (drawing_tool == DRAWING_TOOL::BEZIER) {
			s = new ShapeBezier(start, lineto, &m_main_sheet);
		}
		else if (drawing_tool == DRAWING_TOOL::RULER) {
			s = new ShapeRuler(start, lineto, &m_main_sheet);
		}
		else if (drawing_tool == DRAWING_TOOL::ARC) {
			s = new SHAPE_ARC(start, lineto, &m_main_sheet);
		}
		else if (drawing_tool == DRAWING_TOOL::TEXT) {
			s = new ShapeText(start, lineto, nullptr, &m_main_sheet);
		}
		else {
			return;
		}
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		event_reduce_slist(m_main_sheet.m_shape_list, m_undo_stack, m_redo_stack);
		undo_push_null();
		undo_push_append(s);
		select_shape(s);
		//unselect_all_shape();
		//undo_push_toggle(s);
		if (drawing_tool == DRAWING_TOOL::TEXT) {
			if (m_core_text_focused != nullptr) {
				m_core_text.NotifyFocusLeave();
				undo_push_text_unselect(m_core_text_focused);
			}
			m_core_text_focused = static_cast<ShapeText*>(s);
			m_core_text_focused->create_text_layout();
			m_core_text.NotifyFocusEnter();
		}
		main_bbox_update(s);
		main_panel_size();
		m_event_shape_pressed = s;
		m_event_shape_last = s;
		menu_is_enable();
		main_sheet_draw();
		// 一覧が表示されてるか判定する.
		if (summary_is_visible()) {
			summary_append(s);
			summary_select(s);
		}
		status_bar_set_pointer();
	}

	//------------------------------
	// 図形の変形を終了する.
	//------------------------------
	void MainPage::event_adjust_after_deforming(const VirtualKeyModifiers key_mod)
	{
		//const auto g_snap = m_main_sheet.m_snap_grid;
		//const auto g_snap = m_snap_grid;
		if (m_snap_grid && m_snap_point >= FLT_MIN) {
			// 現在の位置と, それを方眼の大きさに丸めた位置と間の距離を求める.
			D2D1_POINT_2F p{ m_event_point_curr };
			D2D1_POINT_2F q;
			D2D1_POINT_2F r;
			m_event_shape_pressed->get_pt_hit(m_event_hit_pressed, p);
			pt_round(p, m_main_sheet.m_grid_base + 1.0, q);
			pt_sub(q, p, r);
			const double s = sqrt(pt_abs2(r));
			double grid_dist = min(s, static_cast<double>(m_snap_point)) / m_main_scale;
			// 方眼との距離より近い頂点が見つかったなら, その距離に入れ替える.
			if (slist_find_vertex_closest(m_main_sheet.m_shape_list, p, grid_dist, q)) {
				pt_sub(q, p, r);
				grid_dist = sqrt(pt_abs2(r)) / m_main_scale;
			}
			// 近傍の頂点によって点が変わらなかったか判定する.
			const auto keep_aspect = ((key_mod & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			if (!m_event_shape_pressed->set_pt_hit(p, m_event_hit_pressed, static_cast<float>(grid_dist), keep_aspect)) {
				// 変わらなかったならば, 方眼に合わせる.
				m_event_shape_pressed->set_pt_hit(q, m_event_hit_pressed, 0.0f, keep_aspect);
			}
		}
		else if (m_snap_grid) {
			D2D1_POINT_2F p{ m_event_point_curr };
			m_event_shape_pressed->get_pt_hit(m_event_hit_pressed, p);
			const auto keep_aspect = ((key_mod & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			pt_round(p, m_main_sheet.m_grid_base + 1.0, p);
			m_event_shape_pressed->set_pt_hit(p, m_event_hit_pressed, 0.0f, keep_aspect);
		}
		else if (m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{ m_event_point_curr };
			m_event_shape_pressed->get_pt_hit(m_event_hit_pressed, p);
			const auto keep_aspect = ((key_mod & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			slist_find_vertex_closest(m_main_sheet.m_shape_list, p, m_snap_point / m_main_scale, p);
			m_event_shape_pressed->set_pt_hit(p, m_event_hit_pressed, m_snap_point / m_main_scale, keep_aspect);
		}
	}

	//------------------------------
	// 図形の移動を終了する.
	//------------------------------
	void MainPage::event_adjust_after_moving(void)
	{
		// 方眼にくっつける, かつ頂点にくっつける.
		if (m_snap_grid && m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{};	// 方眼との差分
			if (event_get_nearest_grid(m_main_sheet.m_shape_list, m_main_sheet.m_grid_base + 1.0f, p)) {
				D2D1_POINT_2F q{};	// 頂点との差分
				if (event_get_nearest_point(m_main_sheet.m_shape_list, m_snap_point / m_main_scale, q) && pt_abs2(q) < pt_abs2(p)) {
					// 方眼と頂点のどちらか短い方の距離を, 差分に得る.
					p = q;
				}
				// 得られた差分の分だけ, 選択された図形を移動する.
				slist_move_selected(m_main_sheet.m_shape_list, p);
			}
		}
		// 方眼にくっつける
		else if (m_snap_grid) {
			D2D1_POINT_2F p{};	// 方眼との差分
			if (event_get_nearest_grid(m_main_sheet.m_shape_list, m_main_sheet.m_grid_base + 1.0f, p)) {
				slist_move_selected(m_main_sheet.m_shape_list, p);
			}
		}
		// 頂点にくっつける
		else if (m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{};	// 頂点との差分
			if (event_get_nearest_point(m_main_sheet.m_shape_list, m_snap_point / m_main_scale, p)) {
				slist_move_selected(m_main_sheet.m_shape_list, p);
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
			if (m_event_point_pressed.x < m_event_point_curr.x) {
				lt.x = m_event_point_pressed.x;
				rb.x = m_event_point_curr.x;
			}
			else {
				lt.x = m_event_point_curr.x;
				rb.x = m_event_point_pressed.x;
			}
			if (m_event_point_pressed.y < m_event_point_curr.y) {
				lt.y = m_event_point_pressed.y;
				rb.y = m_event_point_curr.y;
			}
			else {
				lt.y = m_event_point_curr.y;
				rb.y = m_event_point_pressed.y;
			}
			if (toggle_inside_shape(lt, rb)) {
			}
		}
		// 修飾キーが押されてないか判定する.
		else if (key_mod == VirtualKeyModifiers::None) {
			D2D1_POINT_2F lt{};
			D2D1_POINT_2F rb{};
			if (m_event_point_pressed.x < m_event_point_curr.x) {
				lt.x = m_event_point_pressed.x;
				rb.x = m_event_point_curr.x;
			}
			else {
				lt.x = m_event_point_curr.x;
				rb.x = m_event_point_pressed.x;
			}
			if (m_event_point_pressed.y < m_event_point_curr.y) {
				lt.y = m_event_point_pressed.y;
				rb.y = m_event_point_curr.y;
			}
			else {
				lt.y = m_event_point_curr.y;
				rb.y = m_event_point_pressed.y;
			}
			if (select_inside_shape(lt, rb)) {
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
		status_bar_set_pointer();

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
				m_event_point_curr.x - m_event_point_pressed.x, m_event_point_curr.y - m_event_point_pressed.y
			};
			if (pt_abs2(p) * m_main_scale > m_event_click_dist * raw_dpi / log_dpi) {
				m_event_state = EVENT_STATE::BEGIN;
				event_set_cursor();
			}
		}
		// 状態が, 矩形選択している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_RECT) {
			main_sheet_draw();
		}
		// 状態が, 図形を移動している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			// ポインターの現在位置と前回位置の差分だけ, 選択された図形を移動する.
			const D2D1_POINT_2F p{
				m_event_point_curr.x - m_event_point_prev.x, m_event_point_curr.y - m_event_point_prev.y
			};
			slist_move_selected(m_main_sheet.m_shape_list, p);
			m_event_point_prev = m_event_point_curr;
			main_sheet_draw();
		}
		// 状態が, 図形を変形している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_DEFORM) {
			// ポインターの現在位置を, 図形の判定部位の座標に格納する.
			const auto keep_aspect = ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			m_event_shape_pressed->set_pt_hit(m_event_point_curr, m_event_hit_pressed, 0.0f, keep_aspect);
			m_event_point_prev = m_event_point_curr;
			main_sheet_draw();
		}
		// 状態が, 文字列を選択している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_TEXT) {
			core_text_pressed<true>();
		}
		// 状態が, 左ボタンを押している状態, または
		// クリック後に左ボタンを押した状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_LBTN || m_event_state == EVENT_STATE::CLICK_LBTN) {
			// ポインターの現在位置と押された位置の間の長さが, クリックの判定距離を超えるか判定する.
			const D2D1_POINT_2F p{
				m_event_point_curr.x - m_event_point_pressed.x, m_event_point_curr.y - m_event_point_pressed.y
			};
			if (pt_abs2(p) * m_main_scale > m_event_click_dist) {
				// 作図ツールが図形作成なら, 矩形選択している状態に遷移する.
				if (m_tool == DRAWING_TOOL::ARC ||
					m_tool == DRAWING_TOOL::BEZIER ||
					m_tool == DRAWING_TOOL::ELLIPSE ||
					m_tool == DRAWING_TOOL::LINE ||
					m_tool == DRAWING_TOOL::POLY ||
					m_tool == DRAWING_TOOL::RECT ||
					m_tool == DRAWING_TOOL::RRECT ||
					m_tool == DRAWING_TOOL::RULER ||
					m_tool == DRAWING_TOOL::TEXT) {
					m_event_state = EVENT_STATE::PRESS_RECT;
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
				}
				// 押された図形がヌルなら, 矩形選択している状態に遷移する.
				else if (m_tool == DRAWING_TOOL::SELECT && m_event_shape_pressed == nullptr) {
					m_event_state = EVENT_STATE::PRESS_RECT;
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
				}
				// 選択された図形の数が 1 を超える,
				// または押された図形の判定部位が線枠, 内側のときは, 図形を移動している状態に遷移する.
				else if (m_tool == DRAWING_TOOL::SELECT &&
					(m_event_hit_pressed == HIT_TYPE::HIT_STROKE || m_event_hit_pressed == HIT_TYPE::HIT_FILL || m_undo_select_cnt > 1)) {
					m_event_state = EVENT_STATE::PRESS_MOVE;
					m_event_point_prev = m_event_point_curr;
					undo_push_null();
					undo_push_move<false>(p);
					main_sheet_draw();
				}
				// 押された図形の判定部位が文字列なら, 文字列の選択をしている状態に遷移する.
				else if (m_tool == DRAWING_TOOL::SELECT && m_event_hit_pressed == HIT_TYPE::HIT_TEXT) {
					// 編集中の文字列がない, または押された図形が編集中の文字列以外の場合.
					if (m_core_text_focused == nullptr || m_core_text_focused != m_event_shape_pressed) {
						m_event_state = EVENT_STATE::PRESS_MOVE;
						m_event_point_prev = m_event_point_curr;
						undo_push_null();
						undo_push_position(m_event_shape_pressed, m_event_hit_pressed);
						const auto keep_aspect = ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
						m_event_shape_pressed->set_pt_hit(m_event_point_curr, m_event_hit_pressed, 0.0f, keep_aspect);

						//////
						if (m_core_text_focused != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_focused);
							m_core_text_focused = nullptr;
						}
						main_sheet_draw();
					}
					// 押された図形が編集中の文字列の場合.
					else if (m_core_text_focused != nullptr && m_core_text_focused == m_event_shape_pressed) {
						m_event_state = EVENT_STATE::PRESS_TEXT;
						core_text_pressed<true>();
					}
				}
				// ポインターが押されたのが図形の外部以外なら, 図形を変形している状態に遷移する.
				else if (m_tool == DRAWING_TOOL::SELECT && m_event_hit_pressed != HIT_TYPE::HIT_SHEET) {
					if (m_event_shape_pressed->is_selected()) {
						m_event_state = EVENT_STATE::PRESS_DEFORM;
						m_event_point_prev = m_event_point_curr;
						undo_push_null();
						undo_push_position(m_event_shape_pressed, m_event_hit_pressed);
						const auto keep_aspect = ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
						m_event_shape_pressed->set_pt_hit(m_event_point_curr, m_event_hit_pressed, 0.0f, keep_aspect);
						main_sheet_draw();
					}
				}
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

		Focus(FocusState::Programmatic);

		const SwapChainPanel& swap_chain_panel{
			sender.as<SwapChainPanel>()
		};

		if (scp_main_panel().ContextFlyout() != nullptr) {
			scp_main_panel().ContextFlyout().Hide();
			scp_main_panel().ContextFlyout(nullptr);
		}

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
		m_event_point_pressed = m_event_point_curr;

		wchar_t buf[512];
		wchar_t buf_x[256];
		wchar_t buf_y[256];
		conv_len_to_str<false>(m_len_unit, m_event_point_pressed.x, m_main_d2d.m_logical_dpi, m_main_sheet.m_grid_base + 1.0, buf_x);
		conv_len_to_str<false>(m_len_unit, m_event_point_pressed.y, m_main_d2d.m_logical_dpi, m_main_sheet.m_grid_base + 1.0, buf_y);
		swprintf_s(buf, L"%s %s", buf_x, buf_y);
		status_bar_tool_value().Text(buf);

		bool changed = false;
		if (m_event_state == EVENT_STATE::PRESS_LBTN) {
			if (m_tool == DRAWING_TOOL::SELECT) {
				if ((args.KeyModifiers() & VirtualKeyModifiers::Control) == VirtualKeyModifiers::Control) {
					m_event_shape_pressed = nullptr;
					m_event_hit_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_point_pressed, true, m_event_shape_pressed);
					// 制御キー押下では, 押された図形が選択されていたなら外し, 選択されていなかったなら付ける.
					// ただし, 押された図形の判定部位が, 枠線や塗り以外の判定部位だったならば, 変形を行なうため, 選択された状態を変えない.
					if (m_event_hit_pressed == HIT_TYPE::HIT_SHEET) {
					}
					else if (m_event_hit_pressed == HIT_TYPE::HIT_STROKE || m_event_hit_pressed == HIT_TYPE::HIT_FILL || m_event_hit_pressed == HIT_TYPE::HIT_TEXT) {
						undo_push_toggle(m_event_shape_pressed);
						if (m_event_shape_pressed->is_selected()) {
							m_main_sheet.set_attr_to(m_event_shape_pressed);
							m_event_shape_last = m_event_shape_pressed;
						}
						if (summary_is_visible()) {
							if (m_event_shape_pressed->is_selected()) {
								summary_select(m_event_shape_pressed);
							}
							else {
								summary_unselect(m_event_shape_pressed);
							}
						}
						changed = true;
					}
					if (m_event_hit_pressed == HIT_TYPE::HIT_TEXT && m_undo_select_cnt == 1) {
						if (m_core_text_focused != m_event_shape_pressed) {
							if (m_core_text_focused != nullptr) {
								m_core_text.NotifyFocusLeave();
								undo_push_text_unselect(m_core_text_focused);
							}
							m_core_text_focused = static_cast<ShapeText*>(m_event_shape_pressed);
						}
						else {
							if (m_core_text_comp) {
								m_core_text.NotifyFocusLeave();
							}
						}
						bool trail;
						const auto end = m_core_text_focused->get_text_pos(m_event_point_curr, trail);
						const auto start = trail ? end + 1 : end;
						undo_push_text_select(m_core_text_focused, start, end, trail);
						changed = true;
					}
					/////
					// 編集中の文字列図形があるならフォーカスを取り消す.
					if (m_core_text_focused != nullptr) {
						m_core_text.NotifyFocusLeave();
						undo_push_text_unselect(m_core_text_focused);
						m_core_text_focused = nullptr;
						changed = true;
					}
				}
				// シフトキー押下では, 図形リストの中で, 直前に押された図形から, たった今押された図形までの間に並んでいる図形を, 範囲選択する.
				// ただし, 編集中の文字列がある場合は, 直前に押されたキャレット位置から, たった今押された位置の間に並んでいる文字列を, 範囲選択する.
				else if ((args.KeyModifiers() & VirtualKeyModifiers::Shift) == VirtualKeyModifiers::Shift) {
					m_event_shape_pressed = nullptr;
					m_event_hit_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_point_pressed, false, m_event_shape_pressed);
					if (m_event_hit_pressed == HIT_TYPE::HIT_SHEET) {
					}
					// 最後に押された図形があるなら, 指定した範囲の図形を選択, 範囲外の図形の選択を外す.
					else {
						if (m_event_shape_last == nullptr) {
							m_event_shape_last = m_event_shape_pressed;
						}
						if (select_range_shape(m_event_shape_last, m_event_shape_pressed)) {
							changed = true;
						}
					}
					if (m_event_hit_pressed == HIT_TYPE::HIT_TEXT && m_undo_select_cnt == 1) {
						if (m_core_text_focused != m_event_shape_pressed) {
							if (m_core_text_focused != nullptr) {
								m_core_text.NotifyFocusLeave();
								undo_push_text_unselect(m_core_text_focused);
							}
							m_core_text_focused = static_cast<ShapeText*>(m_event_shape_pressed);
							bool trail;
							const auto end = m_core_text_focused->get_text_pos(m_event_point_curr, trail);
							const auto start = (trail ? end + 1 : end);
							undo_push_text_select(m_core_text_focused, start, end, trail);
							m_core_text.NotifyFocusEnter();
						}
						else {
							if (m_core_text_comp) {
								m_core_text.NotifyFocusLeave();
							}
							bool trail;
							const auto end = m_core_text_focused->get_text_pos(m_event_point_curr, trail);
							const auto start = m_main_sheet.m_core_text_range.m_start;
							undo_push_text_select(m_core_text_focused, start, end, trail);
							m_core_text.NotifyFocusEnter();
						}
						changed = true;
					}
					else {
						// 現在に編集中の文字列があれば中断し, 押された文字列を編集中の図形にする.
						if (m_core_text_focused != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_focused);
							m_core_text_focused = nullptr;
							changed = true;
						}
					}
				}
				// 修飾キーがない. 
				else if (args.KeyModifiers() == VirtualKeyModifiers::None) {
					m_event_shape_pressed = nullptr;
					m_event_hit_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_point_pressed, false, m_event_shape_pressed);
					// 押された図形がない.
					if (m_event_hit_pressed == HIT_TYPE::HIT_SHEET || m_event_shape_pressed == nullptr) {
						if (unselect_all_shape()) {
							m_event_shape_last = nullptr;
							changed = true;
						}
					}
					// 押された図形が選択されてない.
					else if (!m_event_shape_pressed->is_selected()) {
						m_main_sheet.set_attr_to(m_event_shape_pressed);
						m_event_shape_last = m_event_shape_pressed;
						select_shape(m_event_shape_pressed);
						//unselect_all_shape();
						//undo_push_toggle(m_event_shape_pressed);
						if (summary_is_visible()) {
							summary_select(m_event_shape_pressed);
						}
						changed = true;
					}
					// 押されたのが文字列で, かつ選択された図形がひとつだけ.
					if (m_event_hit_pressed == HIT_TYPE::HIT_TEXT && m_undo_select_cnt == 1) {
						// 押された図形がフォーカス中の文字列図形でない.
						if (m_core_text_focused != m_event_shape_pressed) {
							// 
							if (m_core_text_focused != nullptr) {
								m_core_text.NotifyFocusLeave();
								undo_push_text_unselect(m_core_text_focused);
							}
							m_core_text_focused = static_cast<ShapeText*>(m_event_shape_pressed);
						}
						else {
							if (m_core_text_comp) {
								m_core_text.NotifyFocusLeave();
							}
						}
						bool trail;
						const auto end = m_core_text_focused->get_text_pos(m_event_point_curr, trail);
						const auto start = trail ? end + 1 : end;
						undo_push_text_select(m_core_text_focused, start, end, trail);
						m_core_text.NotifyFocusEnter();
						changed = true;
					}
					else {
						if (m_core_text_focused != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_focused);
							m_core_text_focused = nullptr;
							changed = true;
						}
					}
				}
			}
			else if (m_tool == DRAWING_TOOL::POINTER) {
				m_event_shape_pressed = nullptr;
				m_event_hit_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_point_pressed, false, m_event_shape_pressed);
			}
			else if (m_tool == DRAWING_TOOL::EYEDROPPER) {
				m_event_shape_pressed = nullptr;
				m_event_hit_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_point_pressed, false, m_event_shape_pressed);
			}
			else {
				m_event_shape_pressed = nullptr;
				m_event_hit_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_point_pressed, false, m_event_shape_pressed);
				if (m_core_text_focused != nullptr) {
					m_core_text.NotifyFocusLeave();
					undo_push_text_unselect(m_core_text_focused);
					m_core_text_focused = nullptr;
					changed = true;
				}
			}
		}
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
			m_event_shape_pressed = nullptr;
			m_event_hit_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_point_pressed, false, m_event_shape_pressed);
			if (m_event_shape_pressed != nullptr && !m_event_shape_pressed->is_selected()) {
				m_event_shape_last = m_event_shape_pressed;
				select_shape(m_event_shape_pressed);
				changed = true;
			}
			//if (unselect_all_shape()) {
			//	changed = true;
			//}
			//if (m_event_shape_pressed != nullptr) {
			//	undo_push_toggle(m_event_shape_pressed);
			//	changed = true;
			//}
		}
		if (changed) {
			menu_is_enable();
			main_sheet_draw();
		}
	}

	// 文字列編集ダイアログを表示する.
	IAsyncAction MainPage::event_edit_text_async(void)
	{
		ShapeText* s = static_cast<ShapeText*>(m_event_shape_pressed);
		m_mutex_event.lock();
		tx_edit_text().Text(s->m_text == nullptr ? L"" : s->m_text);
		tx_edit_text().SelectAll();
		if (co_await cd_edit_text_dialog().ShowAsync() == ContentDialogResult::Primary) {
			const auto len = tx_edit_text().Text().size();
			undo_push_null();
			undo_push_text_select(s, len, len, false);
			undo_push_set<UNDO_T::TEXT_CONTENT>(s, wchar_cpy(tx_edit_text().Text().c_str()));
			const bool fit_frame = ck_fit_text_frame().IsChecked().GetBoolean();
			if (fit_frame) {
				undo_push_position(s, HIT_TYPE::HIT_SE);
				s->fit_frame_to_text(m_snap_grid ? m_main_sheet.m_grid_base + 1.0f : 0.0f);
			}
			main_sheet_draw();
		}
		status_bar_set_pointer();
		m_mutex_event.unlock();
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

		wchar_t buf[64];
		wchar_t buf_x[32];
		wchar_t buf_y[32];
		conv_len_to_str<false>(m_len_unit, m_event_point_curr.x, m_main_d2d.m_logical_dpi, m_main_sheet.m_grid_base + 1.0, buf_x);
		conv_len_to_str<false>(m_len_unit, m_event_point_curr.y, m_main_d2d.m_logical_dpi, m_main_sheet.m_grid_base + 1.0, buf_y);
		swprintf_s(buf, L"%s %s", buf_x, buf_y);
		status_bar_tool_value().Text(buf);
		status_bar_tool_value().SelectAll();

		// 状態が, 左ボタンが押された状態か判定する.
		if (m_event_state == EVENT_STATE::PRESS_LBTN) {
			if (m_tool == DRAWING_TOOL::SELECT) {

				// ボタンが離れた時刻と押された時刻の差が, クリックの判定時間以下か判定する.
				const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
				const auto c_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
				if (t_stamp - m_event_time_pressed <= c_time) {
					// クリックした状態に遷移する.
					m_event_state = EVENT_STATE::CLICK;
					event_set_cursor();
					return;
				}
			}
			// クリックの確定
			else if (m_tool == DRAWING_TOOL::POINTER) {
				//using winrt::Windows::ApplicationModel::DataTransfer::DataPackage;
				//using winrt::Windows::ApplicationModel::DataTransfer::DataPackageOperation;

				/*
				wchar_t buf[512];
				wchar_t buf_x[256];
				wchar_t buf_y[256];
				//DataPackage content{ DataPackage() };
				conv_len_to_str<false>(m_len_unit, m_event_point_pressed.x, m_main_d2d.m_logical_dpi, m_main_sheet.m_grid_base + 1.0, buf_x);
				conv_len_to_str<false>(m_len_unit, m_event_point_pressed.y, m_main_d2d.m_logical_dpi, m_main_sheet.m_grid_base + 1.0, buf_y);
				swprintf_s(buf, L"%s %s", buf_x, buf_y);
				*/
				//status_bar_tool_value().BorderThickness(winrt::Windows::UI::Xaml::Thickness{ 0, 0, 0, 0 });
				//status_bar_tool_value().as<winrt::Windows::UI::Xaml::Controls::Control>().Margin(winrt::Windows::UI::Xaml::Thickness{ 0, 0, 0, 0 });
				//status_bar_tool_value().Text(buf);
				//status_bar_tool_value().SelectAll();
				/*
				content.RequestedOperation(DataPackageOperation::Copy);
				content.SetText(buf);
				Clipboard::SetContent(content);
				*/
			}
			else if (m_tool == DRAWING_TOOL::EYEDROPPER) {
				// 修飾キーが押されてないなら吸い上げ.
				if ((args.KeyModifiers() & VirtualKeyModifiers::Control) == VirtualKeyModifiers::None) {
					event_eyedropper_detect(m_event_shape_pressed, m_event_hit_pressed);
					status_bar_set_drawing_tool();
					//return;
				}
				// 制御キー押下なら吐き出し.
				else {
					if (m_event_hit_pressed == HIT_TYPE::HIT_SHEET) {
						undo_push_null();
						undo_push_set<UNDO_T::SHEET_COLOR>(&m_main_sheet, m_eyedropper_color);
						menu_is_enable();
						main_sheet_draw();
					}
					else if (m_event_shape_pressed != nullptr) {
						if (m_event_hit_pressed == HIT_TYPE::HIT_FILL) {
							undo_push_null();
							undo_push_set<UNDO_T::FILL_COLOR>(m_event_shape_pressed, m_eyedropper_color);
							menu_is_enable();
							main_sheet_draw();
						}
						else if (m_event_hit_pressed == HIT_TYPE::HIT_TEXT) {
							undo_push_null();
							undo_push_set<UNDO_T::FONT_COLOR>(m_event_shape_pressed, m_eyedropper_color);
							menu_is_enable();
							main_sheet_draw();
						}
						else if (m_event_hit_pressed == HIT_TYPE::HIT_STROKE) {
							undo_push_null();
							undo_push_set<UNDO_T::STROKE_COLOR>(m_event_shape_pressed, m_eyedropper_color);
							menu_is_enable();
							main_sheet_draw();
						}
					}
				}
				//return;
			}
		}
		// 状態が, クリック後に左ボタンが押した状態か判定する.
		else if (m_event_state == EVENT_STATE::CLICK_LBTN) {
			// ボタンが離された時刻と押された時刻の差分を得る.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			const auto c_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
			// 差分がクリックの判定時間以下, かつ押された図形が文字列図形か判定する.
			if (t_stamp - m_event_time_pressed <= c_time && m_event_shape_pressed != nullptr) {
				if (m_core_text_focused != nullptr) {
					m_core_text.NotifyFocusLeave();
					undo_push_text_unselect(m_core_text_focused);
					m_core_text_focused = nullptr;
				}
				// ダブルクリックの確定.
				// 文字列編集ダイアログを呼び出す.
				if (typeid(*m_event_shape_pressed) == typeid(ShapeText)) {
					event_edit_text_async();
				}
				//else if (typeid(*m_event_shape_pressed) == typeid(SHAPE_ARC)) {
					//edit_arc_async(static_cast<SHAPE_ARC*>(m_event_shape_pressed));
				//}
			}
		}
		// 状態が, 文字列を押している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_TEXT) {
			core_text_pressed<true>();
		}
		// 状態が, 図形を移動している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			// 制御キーが押されてないなら, 図形の移動後の調整を行なう.
			if ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control) {
				event_adjust_after_moving();
			}
			if (!undo_pop_invalid()) {
				main_bbox_update();
				main_panel_size();
			}
		}
		// 状態が, 図形を変形している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_DEFORM) {
			// 制御キーが押されてないなら, 図形の変形後の調整を行なう.
			if ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control) {
				event_adjust_after_deforming(args.KeyModifiers());
			}
			if (!undo_pop_invalid()) {
				main_bbox_update();
				main_panel_size();
			}
		}
		// 状態が, 矩形選択している状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_RECT) {
			// 作図ツールが選択ツールか判定する.
			if (m_tool == DRAWING_TOOL::SELECT) {
				event_finish_rect_selection(args.KeyModifiers());
			}
			// 作図ツールが選択ツール以外.
			else {
				// 制御キーが押されてないなら, ポインター点を頂点または方眼に合わせる.
				if ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control) {
					// 点と点をくっつける閾値がゼロより大きいなら, ほかの頂点に合わせる.
					if (m_snap_point >= FLT_MIN) {
						const bool boxed = (
							m_tool == DRAWING_TOOL::ELLIPSE ||
							m_tool == DRAWING_TOOL::POLY ||
							m_tool == DRAWING_TOOL::RECT ||
							m_tool == DRAWING_TOOL::RRECT ||
							m_tool == DRAWING_TOOL::RULER ||
							m_tool == DRAWING_TOOL::TEXT
							);
						const float interval = m_snap_point / m_main_scale;
						const double g_len = max(m_main_sheet.m_grid_base, 0.0) + 1.0;
						event_snap_point(m_main_sheet.m_shape_list, boxed, interval, m_snap_grid, g_len, m_event_point_pressed, m_event_point_curr);
					}
					// 方眼に合わせるか判定する.
					else if (m_snap_grid) {
						// 押された位置よ離された位置を方眼の大きさで丸める.
						const double g_len = max(m_main_sheet.m_grid_base + 1.0, 1.0);
						pt_round(m_event_point_pressed, g_len, m_event_point_pressed);
						pt_round(m_event_point_curr, g_len, m_event_point_curr);
					}
				}
				// ポインターの現在位置と押された位置の差分の x 値または y 値が 1 以上か判定する.
				const D2D1_POINT_2F p{
					m_event_point_curr.x - m_event_point_pressed.x,
					m_event_point_curr.y - m_event_point_pressed.y
				};
				if (fabs(p.x) >= 1.0f || fabs(p.y) >= 1.0f) {
					event_finish_creating(m_event_point_pressed, p);
				}
				else {
					unselect_all_shape();
				}
			}
		}
		// 状態が, 右ボタンを押した状態か判定する.
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
			// ポップアップ (フライアウト) が表示されているならそれを解放する.
			if (scp_main_panel().ContextFlyout() != nullptr) {
				scp_main_panel().ContextFlyout().Hide();
				scp_main_panel().ContextFlyout(nullptr);
			}
			// ポップアップメニューを表示する.
			scp_main_panel().ContextFlyout(popup_menu());
		}
		// 状態が, 初期状態か判定する.
		else if (m_event_state == EVENT_STATE::BEGIN) {
			// 本来は初期状態でこのハンドラーが呼び出されるはずはないが,
			// コンテンツダイアログやメニューを終了したとき呼び出されることがあった.
			return;
		}
		// 初期状態に戻す.
		m_event_state = EVENT_STATE::BEGIN;
		//m_event_shape_pressed = nullptr;
		//m_event_hit_pressed = HIT_TYPE::HIT_SHEET;
		menu_is_enable();
		main_sheet_draw();
	}

	//------------------------------
	// カーソルを設定する.
	//------------------------------
	void MainPage::event_set_cursor(void)
	{
		// 作図ツールが色抽出.
		if (m_tool == DRAWING_TOOL::EYEDROPPER) {
			const CoreCursor& curs = m_eyedropper_filled ? CURS_EYEDROPPER2 : CURS_EYEDROPPER1;
			Window::Current().CoreWindow().PointerCursor(curs);
		}
		else if (m_tool == DRAWING_TOOL::POINTER) {
			Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
		}
		// 作図ツールが選択ツール以外かつ状態が右ボタン押下でない.
		else if (m_tool != DRAWING_TOOL::SELECT && m_event_state != EVENT_STATE::PRESS_RBTN) {
			Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
		}
		// 描画の排他制御をロックできないか判定する.
		else if (!m_mutex_draw.try_lock()) {
			Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
		}
		else {
			// 描画の排他制御をロックできたなら, ただちに解除する.
			m_mutex_draw.unlock();
			SHAPE* s;
			const auto hit = slist_hit_test(m_main_sheet.m_shape_list, m_event_point_curr, false, s);
			if (hit == HIT_TYPE::HIT_SHEET) {
				//Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
				Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
			}
			else if (m_undo_select_cnt > 1) {
				Window::Current().CoreWindow().PointerCursor(CURS_SIZE_ALL);
			}
			else if (hit == HIT_TYPE::HIT_TEXT) {
				Window::Current().CoreWindow().PointerCursor(CURS_IBEAM);
			}
			else {
				switch (hit) {
				case HIT_TYPE::HIT_A_START:
				case HIT_TYPE::HIT_A_END:
				case HIT_TYPE::HIT_A_AXIS_X:
				case HIT_TYPE::HIT_A_AXIS_Y:
				case HIT_TYPE::HIT_R_NW:
				case HIT_TYPE::HIT_R_NE:
				case HIT_TYPE::HIT_R_SE:
				case HIT_TYPE::HIT_R_SW:
				case HIT_TYPE::HIT_START:
				case HIT_TYPE::HIT_END:
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
					break;
				case HIT_TYPE::HIT_A_CENTER:
				case HIT_TYPE::HIT_FILL:
				case HIT_TYPE::HIT_STROKE:
				case HIT_TYPE::HIT_TEXT:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_ALL);
					break;
				case HIT_TYPE::HIT_NE:
				case HIT_TYPE::HIT_SW:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_NESW);
					break;
				case HIT_TYPE::HIT_NORTH:
				case HIT_TYPE::HIT_SOUTH:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_NS);
					break;
				case HIT_TYPE::HIT_NW:
				case HIT_TYPE::HIT_SE:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_NWSE);
					break;
				case HIT_TYPE::HIT_WEST:
				case HIT_TYPE::HIT_EAST:
					Window::Current().CoreWindow().PointerCursor(CURS_SIZE_WE);
					break;
				default:
					// 図形のクラスが, 多角形または曲線であるか判定する.
					if (s != nullptr) {
						if (typeid(*s) == typeid(ShapeLine) ||
							typeid(*s) == typeid(ShapePoly) ||
							typeid(*s) == typeid(ShapeBezier) ||
							typeid(*s) == typeid(SHAPE_ARC)) {
							// 図形の判定部位が, 頂点の数を超えないか判定する.
							if (hit >= HIT_TYPE::HIT_P0 && hit < HIT_TYPE::HIT_P0 + static_cast<SHAPE_PATH*>(s)->m_lineto.size() + 1) {
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
	/*
	void MainPage::event_set_position(PointerRoutedEventArgs const& args)
	{
		const double scale = m_main_scale;
		// 境界ボックスの左上点にスクロールの値を加え, 表示されている左上点を得る.
		D2D1_POINT_2F q;
		pt_add(m_main_bbox_lt, sb_horz().Value(), sb_vert().Value(), q);
		// 引数として渡された点に,
		// 拡大率の逆数を乗じ, 表示されている左上点を加えた点を得る.
		// 得られた点を, ポインターの現在位置に格納する.
		const auto p{ args.GetCurrentPoint(scp_main_panel()).Position() };
		m_event_point_curr.x = static_cast<FLOAT>(p.X / scale + m_main_bbox_lt.x + sb_horz().Value());
		m_event_point_curr.y = static_cast<FLOAT>(p.Y / scale + m_main_bbox_lt.x + sb_vert().Value());
		//pt_mul_add(D2D1_POINT_2F{ p.X, p.Y }, 1.0 / scale, q, m_event_point_curr);
	}
	*/

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
				m_main_scale *= 1.1f;
			}
			else if (w_delta < 0 && m_main_scale > 0.25f * 1.1f + FLT_MIN) {
				m_main_scale /= 1.1f;
			}
			else {
				return;
			}
			if (equal(m_main_scale, 1.0f)) {
				rmfi_popup_sheet_zoom_100().IsChecked(true);
				rmfi_menu_sheet_zoom_100().IsChecked(true);
			}
			else if (equal(m_main_scale, 1.5f)) {
				rmfi_popup_sheet_zoom_150().IsChecked(true);
				rmfi_menu_sheet_zoom_150().IsChecked(true);
			}
			else if (equal(m_main_scale, 2.0f)) {
				rmfi_popup_sheet_zoom_200().IsChecked(true);
				rmfi_menu_sheet_zoom_200().IsChecked(true);
			}
			else if (equal(m_main_scale, 3.0f)) {
				rmfi_popup_sheet_zoom_300().IsChecked(true);
				rmfi_menu_sheet_zoom_300().IsChecked(true);
			}
			else if (equal(m_main_scale, 4.0f)) {
				rmfi_popup_sheet_zoom_400().IsChecked(true);
				rmfi_menu_sheet_zoom_400().IsChecked(true);
			}
			else if (equal(m_main_scale, 0.75f)) {
				rmfi_popup_sheet_zoom_075().IsChecked(true);
				rmfi_menu_sheet_zoom_075().IsChecked(true);
			}
			else if (equal(m_main_scale, 0.5f)) {
				rmfi_popup_sheet_zoom_050().IsChecked(true);
				rmfi_menu_sheet_zoom_050().IsChecked(true);
			}
			else if (equal(m_main_scale, 0.25f)) {
				rmfi_popup_sheet_zoom_025().IsChecked(true);
				rmfi_menu_sheet_zoom_025().IsChecked(true);
			}
			main_panel_size();
			main_sheet_draw();
			status_bar_set_pointer();
			status_bar_set_sheet_zoom();
		}
		// シフトキーが押されてるか判定する.
		else if ((mod & VirtualKeyModifiers::Shift) == VirtualKeyModifiers::Shift) {
			// 横スクロール.
			const int32_t w_delta = args.GetCurrentPoint(scp_main_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_horz(), w_delta, m_main_scale)) {
				main_sheet_draw();
				status_bar_set_pointer();
			}
		}
		// 何も押されてないか判定する.
		else if (mod == VirtualKeyModifiers::None) {
			// 縦スクロール.
			const int32_t w_delta = args.GetCurrentPoint(scp_main_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_vert(), w_delta, m_main_scale)) {
				main_sheet_draw();
				status_bar_set_pointer();
			}
		}
	}

}