//-------------------------------
// MainPage_event.cpp
// �|�C���^�[�̃C�x���g�n���h���[
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

	static auto const& CURS_IBEAM = CoreCursor(CoreCursorType::IBeam, 0);	// ������J�[�\��
	static auto const& CURS_ARROW = CoreCursor(CoreCursorType::Arrow, 0);	// ���J�[�\��
	static auto const& CURS_CROSS = CoreCursor(CoreCursorType::Cross, 0);	// �\���J�[�\��
	static auto const& CURS_SIZE_ALL = CoreCursor(CoreCursorType::SizeAll, 0);	// �ړ��J�[�\��
	static auto const& CURS_SIZE_NESW = CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);	// �E�㍶���J�[�\��
	static auto const& CURS_SIZE_NS = CoreCursor(CoreCursorType::SizeNorthSouth, 0);	// �㉺�J�[�\��
	static auto const& CURS_SIZE_NWSE = CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);	// ����E���J�[�\��
	static auto const& CURS_SIZE_WE = CoreCursor(CoreCursorType::SizeWestEast, 0);	// ���E�J�[�\��
	static auto const& CURS_EYEDROPPER1 = CoreCursor(CoreCursorType::Custom, IDC_CURSOR1);	// �X�|�C�g�J�[�\��
	static auto const& CURS_EYEDROPPER2 = CoreCursor(CoreCursorType::Custom, IDC_CURSOR2);	// �X�|�C�g�J�[�\��

	// �����ꂽ�_�Ɨ����ꂽ�_�𒲐�����.
	static void event_snap_point(const SHAPE_LIST& slist, const bool boxed, const double interval, const bool g_snap, const double g_len, D2D1_POINT_2F& pressed, D2D1_POINT_2F& released);
	// �ł��߂�����𓾂�.
	static bool event_get_nearest_grid(const SHAPE_LIST& slist, const float g_len, D2D1_POINT_2F& g_pos) noexcept;
	// �ł��߂��_�𓾂�.
	static bool event_get_nearest_point(const SHAPE_LIST& slist, const double interval, D2D1_POINT_2F& v_pos) noexcept;
	// �}�`������X�^�b�N�Ɋ܂܂�邩���肷��.
	static bool event_undo_contain_shape(const UNDO_STACK& ustack, const Shape* s) noexcept;
	// �}�`���X�g�𐮗�����.
	static void event_reduce_slist(SHAPE_LIST& slist, const UNDO_STACK& ustack, const UNDO_STACK& r_stack) noexcept;
	// �}�E�X�z�C�[���̒l�ŃX�N���[������.
	static bool event_scroll_by_wheel_delta(const ScrollBar& scroll_bar, const int32_t delta, const float scale);

	// �_�����낦��.
	static void event_snap_point(
		const SHAPE_LIST& slist,	// �}�`���X�g
		const bool is_box,	// �����̑Ώۂ�, �}�`���͂ދ�`�Ƃ���Ȃ� true, �}�`�̒��_��ΏۂƂ���Ȃ� false 
		const double interval,	// ��������
		const bool g_snap,	// ����ɂ��낦��.
		const double g_len,	// ����̑傫��
		D2D1_POINT_2F& pressed,	// �����ꂽ�ʒu
		D2D1_POINT_2F& released	// �����ꂽ�ʒu
	)
	{
		D2D1_POINT_2F box[4]{	// �����ꂽ�ʒu�Ɨ����ꂽ�ʒu�ň͂܂ꂽ���`�̒��_
			pressed, { released.x, pressed.y }, released, { pressed.x, released.y },
		};
		double p_abs[4];	// �ʒu�ƒ��_�Ƃ̋��� (�̎���).
		D2D1_POINT_2F p[4];	// ���_�̈ʒu

		// ����ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
		if (slist_find_vertex_closest(slist, box[0], interval, p[0])) {
			D2D1_POINT_2F d;
			pt_sub(p[0], box[0], d);
			p_abs[0] = pt_abs2(d);
		}
		else {
			p_abs[0] = FLT_MAX;
		}

		// �����̑Ώۂ���`�Ȃ�, �E��ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
		if (is_box && slist_find_vertex_closest(slist, box[1], interval, p[1])) {
			D2D1_POINT_2F d;
			pt_sub(p[1], box[1], d);
			p_abs[1] = pt_abs2(d);
		}
		else {
			p_abs[1] = FLT_MAX;
		}

		// �E���ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
		if (slist_find_vertex_closest(slist, box[2], interval, p[2])) {
			D2D1_POINT_2F d;
			pt_sub(p[2], box[2], d);
			p_abs[2] = pt_abs2(d);
		}
		else {
			p_abs[2] = FLT_MAX;
		}

		// �����̑Ώۂ���`�Ȃ�, �����ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
		if (is_box && slist_find_vertex_closest(slist, box[3], interval, p[3])) {
			D2D1_POINT_2F d;
			pt_sub(p[3] , box[3], d);
			p_abs[3] = pt_abs2(d);
		}
		else {
			p_abs[3] = FLT_MAX;
		}

		// ����ɂ��낦��ꍇ,
		// �����ꂽ�ʒu�Ɨ����ꂽ�ʒu��, �ł��߂��i�q�̈ʒu�𓾂�.
		double g_abs[2];	// ����̊i�q�Ƃ̋��� (�̎���)
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

		// ����ɂ��낦�Ȃ��ꍇ,
		// �����ꂽ�ʒu�Ɨ����ꂽ�ʒu��, �ł��߂��i�q�̈ʒu�Ɋi�[��,
		// ���̋����͍ő�l�Ƃ���.
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

	// �ł��߂�����ւ̈ʒu�x�N�g�������߂�.
	// slist	�}�`���X�g
	// g_len	����̑傫��
	// to_grid	�}�`�������ւ̈ʒu�x�N�g��
	static bool event_get_nearest_grid(const SHAPE_LIST& slist, const float g_len, D2D1_POINT_2F& to_grid) noexcept
	{
		D2D1_POINT_2F p[2 + N_GON_MAX];
		D2D1_POINT_2F g;	// ����̑傫���Ŋۂ߂��ʒu
		D2D1_POINT_2F to;	// �ۂ߂��ʒu�ƌ��̈ʒu�̍���
		double d_min = FLT_MAX;	// �ł��Z������
		for (const auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			// ���E��`�̍���ʒu, �E���ʒu�𓾂�.
			s->get_bbox(D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX }, p[0], p[1]);
			const auto p_cnt = s->get_verts(p + 2);
			for (size_t i = 0; i < 2 + p_cnt; i++) {
				pt_round(p[i], g_len, g);
				pt_sub(g, p[i], to);
				const auto g_abs = pt_abs2(to);	// �ۂ߂��ʒu�ƌ��̈ʒu�̋����̓��
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

	// �ł��߂����_�𓾂�.
	// slist	�}�`���X�g
	// interval	�Ԋu (����ȏ㗣�ꂽ�_�͖�������)
	// to_pt	�ł��߂����_�ւ̈ʒu�x�N�g��
	// �߂�l	���������Ȃ� true
	static bool event_get_nearest_point(
		const SHAPE_LIST& slist,	// �}�`���X�g
		const double interval,	// �Ԋu (����ȏ㗣�ꂽ�_�͖�������)
		D2D1_POINT_2F& to_pt	// �ł��߂����_�ւ̈ʒu�x�N�g��
	) noexcept
	{
		double dd = interval * interval;
		bool flag = false;
		D2D1_POINT_2F p[N_GON_MAX];
		D2D1_POINT_2F q{};
		D2D1_POINT_2F n{};	// �ߖT�_
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

	// �}�E�X�z�C�[���̒l�ŃX�N���[������.
	// scroll_bar	�X�N���[���o�[
	// t delta	�}�E�X�z�C�[���̃f���^�l
	// scale	�p���̔{��
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

	// �}�`���X�g�𐮗�����.
	// slist	�}�`���X�g
	// ustack	���ɖ߂�����X�^�b�N
	// rstack	��蒼������X�^�b�N
	static void event_reduce_slist(SHAPE_LIST& slist, const UNDO_STACK& ustack, const UNDO_STACK& rstack) noexcept
	{
		// �����t���O�̗��}�`���������X�g�Ɋi�[����.
		SHAPE_LIST delete_list;	// �������X�g
		for (const auto t : slist) {
			// �}�`�̏����t���O���Ȃ�,
			// �܂��͐}�`�����ɖ߂�����X�^�b�N�Ɋ܂܂��,
			// �܂��͐}�`����蒼������X�^�b�N�Ɋ܂܂��,�@�����肷��.
			if (!t->is_deleted() || event_undo_contain_shape(ustack, t) || event_undo_contain_shape(rstack, t)) {
				continue;
			}
			// ��L�̂�����ł��Ȃ��}�`���������X�g�ɒǉ�����.
			delete_list.push_back(t);
		}
		// �������X�g�Ɋ܂܂��}�`�����X�g�����菜��, �������.
		auto begin = slist.begin();
		for (const auto s : delete_list) {
			const auto it = std::find(begin, slist.end(), s);
			begin = slist.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// �������X�g����������.
		delete_list.clear();
	}

	// ����X�^�b�N���}�`���܂ނ����肷��.
	// ustack	����X�^�b�N
	// s	�}�`
	// �߂�l	�܂ޏꍇ true.
	static bool event_undo_contain_shape(const UNDO_STACK& ustack, const Shape* s) noexcept
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
	// �|�C���^�[�̃{�^�����グ��ꂽ.
	//------------------------------
	void MainPage::event_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		event_released(sender, args);
	}

	//------------------------------
	// �|�C���^�[���X���b�v�`�F�[���p�l���̒��ɓ�����.
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
	// �|�C���^�[���X���b�v�`�F�[���p�l������o��.
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
	// �F�����o����.
	//------------------------------
	void MainPage::event_eyedropper_detect(
		const Shape* s,	// �����ꂽ�}�`
		const uint32_t loc	// �����ꂽ����
	)
	{
		if (loc == LOC_TYPE::LOC_SHEET) {
			m_eyedropper_color = m_main_sheet.m_sheet_color;
			m_eyedropper_filled = true;
			Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
		}
		else if (s != nullptr) {
			if (loc == LOC_TYPE::LOC_FILL) {
				if (typeid(*s) == typeid(ShapeImage)) {
					if (static_cast<const ShapeImage*>(s)->get_pixcel(m_event_pos_pressed, m_eyedropper_color)) {
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
			else if (loc == LOC_TYPE::LOC_TEXT) {
				s->get_font_color(m_eyedropper_color);
				m_main_sheet.m_font_color = m_eyedropper_color;
				m_eyedropper_filled = true;
				Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
			}
			else if (loc == LOC_TYPE::LOC_STROKE) {
				s->get_stroke_color(m_eyedropper_color);
				m_main_sheet.m_stroke_color = m_eyedropper_color;
				m_eyedropper_filled = true;
				Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
			}
		}
	}

	//------------------------------
	// �}�`�̍쐬���I������.
	// start	�n�_
	// end_to	�Ίp�_�ւ̈ʒu�x�N�g��
	//------------------------------
	void MainPage::event_finish_creating(const D2D1_POINT_2F start, const D2D1_POINT_2F lineto)
	{
		const auto d_tool = m_drawing_tool;
		Shape* s;
		if (d_tool == DRAWING_TOOL::RECT) {
			s = new ShapeRect(start, lineto, &m_main_sheet);
		}
		else if (d_tool == DRAWING_TOOL::RRECT) {
			s = new ShapeRRect(start, lineto, &m_main_sheet);
		}
		else if (d_tool == DRAWING_TOOL::POLY) {
			const auto poly_opt = m_drawing_poly_opt;
			s = new ShapePoly(start, lineto, &m_main_sheet, poly_opt);
		}
		else if (d_tool == DRAWING_TOOL::ELLIPSE) {
			s = new ShapeEllipse(start, lineto, &m_main_sheet);
		}
		else if (d_tool == DRAWING_TOOL::LINE) {
			s = new ShapeLine(start, lineto, &m_main_sheet);
		}
		else if (d_tool == DRAWING_TOOL::BEZIER) {
			s = new ShapeBezier(start, lineto, &m_main_sheet);
		}
		else if (d_tool == DRAWING_TOOL::RULER) {
			s = new ShapeRuler(start, lineto, &m_main_sheet);
		}
		else if (d_tool == DRAWING_TOOL::ARC) {
			s = new ShapeArc(start, lineto, &m_main_sheet);
		}
		else if (d_tool == DRAWING_TOOL::TEXT) {
			if (m_core_text_shape != nullptr) {
				m_core_text.NotifyFocusLeave();
				undo_push_text_unselect(m_core_text_shape);
			}
			s = new ShapeText(start, lineto, nullptr, &m_main_sheet);
			m_core_text_shape = static_cast<ShapeText*>(s);
			m_core_text_shape->create_text_layout();
			m_core_text.NotifyFocusEnter();
		}
		else {
			return;
		}
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		event_reduce_slist(m_main_sheet.m_shape_list, m_undo_stack, m_redo_stack);
		undo_push_null();
		unselect_shape_all();
		undo_push_append(s);
		undo_push_select(s);
		main_bbox_update(s);
		main_panel_size();
		m_event_shape_pressed = s;
		m_event_shape_last = s;
		main_draw();
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_append(s);
			summary_select(s);
		}
		status_bar_set_pos();
	}

	//------------------------------
	// �}�`�̕ό`���I������.
	//------------------------------
	void MainPage::event_adjust_after_deforming(const VirtualKeyModifiers key_mod)
	{
		//const auto g_snap = m_main_sheet.m_snap_grid;
		//const auto g_snap = m_snap_grid;
		if (m_snap_grid && m_snap_point >= FLT_MIN) {
			// ���݂̈ʒu��, ��������̑傫���Ɋۂ߂��ʒu�ƊԂ̋��������߂�.
			D2D1_POINT_2F p{ m_event_pos_curr };
			D2D1_POINT_2F q;
			D2D1_POINT_2F r;
			m_event_shape_pressed->get_pos_loc(m_event_loc_pressed, p);
			pt_round(p, m_main_sheet.m_grid_base + 1.0, q);
			pt_sub(q, p, r);
			const double s = sqrt(pt_abs2(r));
			double grid_dist = min(s, static_cast<double>(m_snap_point)) / m_main_scale;
			// ����Ƃ̋������߂����_�����������Ȃ�, ���̋����ɓ���ւ���.
			if (slist_find_vertex_closest(m_main_sheet.m_shape_list, p, grid_dist, q)) {
				pt_sub(q, p, r);
				grid_dist = sqrt(pt_abs2(r)) / m_main_scale;
			}
			// �ߖT�̒��_�ɂ���ē_���ς��Ȃ����������肷��.
			const auto keep_aspect = ((key_mod & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			if (!m_event_shape_pressed->set_pos_loc(p, m_event_loc_pressed, grid_dist, keep_aspect)) {
				// �ς��Ȃ������Ȃ��, ����ɍ��킹��.
				m_event_shape_pressed->set_pos_loc(q, m_event_loc_pressed, 0.0f, keep_aspect);
			}
		}
		else if (m_snap_grid) {
			D2D1_POINT_2F p{ m_event_pos_curr };
			m_event_shape_pressed->get_pos_loc(m_event_loc_pressed, p);
			const auto keep_aspect = ((key_mod & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			pt_round(p, m_main_sheet.m_grid_base + 1.0, p);
			m_event_shape_pressed->set_pos_loc(p, m_event_loc_pressed, 0.0f, keep_aspect);
		}
		else if (m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{ m_event_pos_curr };
			m_event_shape_pressed->get_pos_loc(m_event_loc_pressed, p);
			const auto keep_aspect = ((key_mod & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			slist_find_vertex_closest(m_main_sheet.m_shape_list, p, m_snap_point / m_main_scale, p);
			m_event_shape_pressed->set_pos_loc(p, m_event_loc_pressed, m_snap_point / m_main_scale, keep_aspect);
		}
	}

	//------------------------------
	// �}�`�̈ړ����I������.
	//------------------------------
	void MainPage::event_adjust_after_moving(void)
	{
		// ����ɂ�������, �����_�ɂ�������.
		if (m_snap_grid && m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{};	// ����Ƃ̍���
			if (event_get_nearest_grid(m_main_sheet.m_shape_list, m_main_sheet.m_grid_base + 1.0f, p)) {
				D2D1_POINT_2F q{};	// ���_�Ƃ̍���
				if (event_get_nearest_point(m_main_sheet.m_shape_list, m_snap_point / m_main_scale, q) && pt_abs2(q) < pt_abs2(p)) {
					// ����ƒ��_�̂ǂ��炩�Z�����̋�����, �����ɓ���.
					p = q;
				}
				// ����ꂽ�����̕�����, �I�����ꂽ�}�`���ړ�����.
				slist_move_selected(m_main_sheet.m_shape_list, p);
			}
		}
		// ����ɂ�������
		else if (m_snap_grid) {
			D2D1_POINT_2F p{};	// ����Ƃ̍���
			if (event_get_nearest_grid(m_main_sheet.m_shape_list, m_main_sheet.m_grid_base + 1.0f, p)) {
				slist_move_selected(m_main_sheet.m_shape_list, p);
			}
		}
		// ���_�ɂ�������
		else if (m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{};	// ���_�Ƃ̍���
			if (event_get_nearest_point(m_main_sheet.m_shape_list, m_snap_point / m_main_scale, p)) {
				slist_move_selected(m_main_sheet.m_shape_list, p);
			}
		}
	}

	//------------------------------
	// ��`�I�����I������.
	// key_mod	�C���L�[
	//------------------------------
	void MainPage::event_finish_rect_selection(const VirtualKeyModifiers key_mod)
	{
		// �C���L�[���R���g���[�������肷��.
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
				//xcvd_menu_is_enabled();
			}
		}
		// �C���L�[��������ĂȂ������肷��.
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
				//xcvd_menu_is_enabled();
			}
		}
		Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
	}

	//------------------------------
	// �|�C���^�[��������.
	//------------------------------
	void MainPage::event_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_main_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// �s�b�J�[���Ԓl��߂��܂�, �C�x���g�����������Ȃ����߂̔r��.
		if (!m_mutex_event.try_lock()) {
			wait_cursor_show();
			return;
		}
		m_mutex_event.unlock();

		event_set_position(args);
		status_bar_set_pos();

		// ��Ԃ�, ������Ԃ����肷��.
		if (m_event_state == EVENT_STATE::BEGIN) {
			event_set_cursor();
		}
		// ��Ԃ�. �N���b�N������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::CLICK) {
			// �|�C���^�[�̌��ݓ_�Ɖ����ꂽ�_�̊Ԃ̒���������, �������N���b�N���苗���𒴂����珉����Ԃɖ߂�.
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
		// ��Ԃ�, ��`�I�����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_RECT) {
			main_draw();
		}
		// ��Ԃ�, �}�`���ړ����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			// �|�C���^�[�̌��݈ʒu�ƑO��ʒu�̍�������, �I�����ꂽ�}�`���ړ�����.
			const D2D1_POINT_2F p{
				m_event_pos_curr.x - m_event_pos_prev.x, m_event_pos_curr.y - m_event_pos_prev.y
			};
			slist_move_selected(m_main_sheet.m_shape_list, p);
			m_event_pos_prev = m_event_pos_curr;
			main_draw();
		}
		// ��Ԃ�, �}�`��ό`���Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_DEFORM) {
			// �|�C���^�[�̌��݈ʒu��, �}�`�̕��ʂ̓_�Ɋi�[����.
			const auto keep_aspect = ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
			m_event_shape_pressed->set_pos_loc(m_event_pos_curr, m_event_loc_pressed, 0.0f, keep_aspect);
			m_event_pos_prev = m_event_pos_curr;
			main_draw();
		}
		// ��Ԃ�, �������I�����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_TEXT) {
			bool trail;
			const auto end = m_core_text_shape->get_text_pos(m_event_pos_curr, trail);
			undo_push_text_select(m_core_text_shape, m_main_sheet.m_select_start, end, trail);
			main_draw();
		}
		// ��Ԃ�, ���{�^���������Ă�����, �܂���
		// �N���b�N��ɍ��{�^������������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_LBTN || m_event_state == EVENT_STATE::CLICK_LBTN) {
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̊Ԃ̒�����, �N���b�N�̔��苗���𒴂��邩���肷��.
			const D2D1_POINT_2F p{
				m_event_pos_curr.x - m_event_pos_pressed.x, m_event_pos_curr.y - m_event_pos_pressed.y
			};
			if (pt_abs2(p) * m_main_scale > m_event_click_dist) {
				// ��}�c�[�����}�`�쐬�Ȃ�, ��`�I�����Ă����ԂɑJ�ڂ���.
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
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
				}
				// �����ꂽ�}�`���k���Ȃ�, ��`�I�����Ă����ԂɑJ�ڂ���.
				else if (m_drawing_tool == DRAWING_TOOL::SELECT && m_event_shape_pressed == nullptr) {
					m_event_state = EVENT_STATE::PRESS_RECT;
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
				}
				// �I�����ꂽ�}�`�̐��� 1 �𒴂���,
				// �܂��͉����ꂽ�}�`�̕��ʂ����g, �����̂Ƃ���, �}�`���ړ����Ă����ԂɑJ�ڂ���.
				else if (m_drawing_tool == DRAWING_TOOL::SELECT &&
					(m_event_loc_pressed == LOC_TYPE::LOC_STROKE || m_event_loc_pressed == LOC_TYPE::LOC_FILL || m_undo_select_cnt > 1)) {
					m_event_state = EVENT_STATE::PRESS_MOVE;
					m_event_pos_prev = m_event_pos_curr;
					undo_push_null();
					undo_push_move(p);
				}
				// �����ꂽ�}�`�̕��ʂ�������Ȃ�, ������̑I�������Ă����ԂɑJ�ڂ���.
				else if (m_drawing_tool == DRAWING_TOOL::SELECT && m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
					// �ҏW���̕����񂪂Ȃ�, �܂��͉����ꂽ�}�`���ҏW���̕�����ȊO�̏ꍇ.
					if (m_core_text_shape == nullptr || m_core_text_shape != m_event_shape_pressed) {
						m_event_state = EVENT_STATE::PRESS_MOVE;
						m_event_pos_prev = m_event_pos_curr;
						undo_push_null();
						undo_push_position(m_event_shape_pressed, m_event_loc_pressed);
						const auto keep_aspect = ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
						m_event_shape_pressed->set_pos_loc(m_event_pos_curr, m_event_loc_pressed, 0.0f, keep_aspect);

						//////
						if (m_core_text_shape != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_shape);
							m_core_text_shape = nullptr;
						}
					}
					// �����ꂽ�}�`���ҏW���̕�����̏ꍇ.
					else if (m_core_text_shape != nullptr && m_core_text_shape == m_event_shape_pressed) {
						m_event_state = EVENT_STATE::PRESS_TEXT;
						bool trail;
						const auto end = m_core_text_shape->get_text_pos(m_event_pos_curr, trail);
						undo_push_text_select(m_core_text_shape, m_main_sheet.m_select_start, end, trail);
					}
				}
				// �|�C���^�[�������ꂽ�̂��}�`�̊O���ȊO�Ȃ�, �}�`��ό`���Ă����ԂɑJ�ڂ���.
				else if (m_drawing_tool == DRAWING_TOOL::SELECT && m_event_loc_pressed != LOC_TYPE::LOC_SHEET) {
					if (m_event_shape_pressed->is_selected()) {
						m_event_state = EVENT_STATE::PRESS_DEFORM;
						m_event_pos_prev = m_event_pos_curr;
						undo_push_null();
						undo_push_position(m_event_shape_pressed, m_event_loc_pressed);
						const auto keep_aspect = ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control);
						m_event_shape_pressed->set_pos_loc(m_event_pos_curr, m_event_loc_pressed, 0.0f, keep_aspect);
					}
				}
				//xcvd_menu_is_enabled();
				main_draw();
			}
		}
	}

	//------------------------------
	// �|�C���^�[�̃{�^���������ꂽ.
	// �L���v�`���̗L���ɂ�����炸, �Е��̃}�E�X�{�^������������Ԃ�, ��������̃{�^���������Ă�, ����͒ʒm����Ȃ�.
	//------------------------------
	void MainPage::event_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_main_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// �s�b�J�[���Ԓl��߂��܂�, �C�x���g�����������Ȃ����߂̔r��.
		if (!m_mutex_event.try_lock()) {
			wait_cursor_show();
			return;
		}
		m_mutex_event.unlock();

		Focus(winrt::Windows::UI::Xaml::FocusState::Programmatic);

		const SwapChainPanel& swap_chain_panel{
			sender.as<SwapChainPanel>()
		};

		// �|�C���^�[�̃L���v�`�����n�߂�.
		swap_chain_panel.CapturePointer(args.Pointer());
		const uint64_t t_stamp = args.GetCurrentPoint(swap_chain_panel).Timestamp();	// �C�x���g��������
		const PointerPointProperties& p_prop = args.GetCurrentPoint(swap_chain_panel).Properties();	// �|�C���^�[�̊g�����
		event_set_position(args);

		// �|�C���^�[�̃f�o�C�X�^�C�v�𔻒肷��.
		const auto c_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
		switch (args.GetCurrentPoint(swap_chain_panel).PointerDevice().PointerDeviceType()) {
		// �f�o�C�X�^�C�v���}�E�X�̏ꍇ
		case PointerDeviceType::Mouse:
			// �g����񂪉E�{�^�����������肷��.
			if (p_prop.IsRightButtonPressed()) {
				m_event_state = EVENT_STATE::PRESS_RBTN;
			}
			// �g����񂪍��{�^�����������肷��.
			else if (p_prop.IsLeftButtonPressed()) {
				[[fallthrough]];
		// �f�o�C�X�^�C�v���y���܂��̓^�b�`�̏ꍇ
		case PointerDeviceType::Pen:
		case PointerDeviceType::Touch:
				// �|�C���^�[�̉����ꂽ��Ԃ𔻒肷��.
				switch (m_event_state) {
				// ��Ԃ��N���b�N������Ԃ̏ꍇ
				case EVENT_STATE::CLICK:
					// �C�x���g���������ƑO��|�C���^�[�������ꂽ�����̍����N���b�N���莞�Ԉȉ������肷��.
					if (t_stamp - m_event_time_pressed <= c_time) {
						m_event_state = EVENT_STATE::CLICK_LBTN;
					}
					else {
						[[fallthrough]];
				// ��Ԃ�������Ԃ̏ꍇ
				case EVENT_STATE::BEGIN:
				default:
						m_event_state = EVENT_STATE::PRESS_LBTN;
					}
				}
				break;
			}
			else {
				[[fallthrough]];
		// �f�o�C�X�^�C�v������ȊO�̏ꍇ
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

		// �����ꂽ�}�`������Ȃ�, ���̑������y�[�W�ɔ��f����
		if (m_event_loc_pressed != LOC_TYPE::LOC_SHEET) {
		}

		bool changed = false;
		if (m_event_state == EVENT_STATE::PRESS_LBTN) {
			if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				if ((args.KeyModifiers() & VirtualKeyModifiers::Control) == VirtualKeyModifiers::Control) {
					// ����L�[�����ł�, �����ꂽ�}�`���I������Ă����Ȃ�O��, �I������Ă��Ȃ������Ȃ�t����.
					// ������, �����ꂽ�}�`�̕��ʂ�, �g����h��ȊO�̕��ʂ������Ȃ��, �ό`���s�Ȃ�.
					m_event_shape_pressed = nullptr;
					m_event_loc_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_pos_pressed, true, m_event_shape_pressed);
					if (m_event_loc_pressed == LOC_TYPE::LOC_SHEET) {
						/////
						// �ҏW���̕�����}�`������Ȃ�t�H�[�J�X��������.
						if (m_core_text_shape != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_shape);
							m_core_text_shape = nullptr;
							changed = true;
						}
					}
					else if (m_event_loc_pressed == LOC_TYPE::LOC_STROKE || m_event_loc_pressed == LOC_TYPE::LOC_FILL) {
						if (!m_event_shape_pressed->is_selected()) {
							m_main_sheet.set_attr_to(m_event_shape_pressed);
							m_event_shape_last = m_event_shape_pressed;
							undo_push_select(m_event_shape_pressed);
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
						// �ҏW���̕�����}�`������Ȃ�t�H�[�J�X��������.
						if (m_core_text_shape != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_shape);
							m_core_text_shape = nullptr;
						}
						changed = true;
					}
					else if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
						if (!m_event_shape_pressed->is_selected()) {
							m_main_sheet.set_attr_to(m_event_shape_pressed);
							m_event_shape_last = m_event_shape_pressed;
							undo_push_select(m_event_shape_pressed);
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
						// �ҏW���̕�����}�`������Ȃ�t�H�[�J�X��������.
						if (m_core_text_shape != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_shape);
							m_core_text_shape = nullptr;
						}
						changed = true;
					}
					else {
						m_main_sheet.set_attr_to(m_event_shape_pressed);
						////
						// �ҏW���̕�����}�`������Ȃ�t�H�[�J�X��������.
						if (m_core_text_shape != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_shape);
							m_core_text_shape = nullptr;
							changed = true;
						}
					}
				}
				// �V�t�g�L�[�����ł�, �}�`���X�g�̒���, ���O�ɉ����ꂽ�}�`����, �������������ꂽ�}�`�܂ł̊Ԃɕ���ł���}�`��, �͈͑I������.
				// ������, �ҏW���̕����񂪂���ꍇ��, ���O�ɉ����ꂽ�L�����b�g�ʒu����, �������������ꂽ�ʒu�̊Ԃɕ���ł��镶�����, �͈͑I������.
				else if ((args.KeyModifiers() & VirtualKeyModifiers::Shift) == VirtualKeyModifiers::Shift) {
					m_event_shape_pressed = nullptr;
					m_event_loc_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_pos_pressed, false, m_event_shape_pressed);
					if (m_event_loc_pressed == LOC_TYPE::LOC_SHEET) {
						////
						// �ҏW���̕�����}�`������Ȃ�t�H�[�J�X��������.
						if (m_core_text_shape != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_shape);
							m_core_text_shape = nullptr;
							changed = true;
						}
					}
					else if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
						////
						// �����ꂽ�}�`�͕ҏW���̐}�`
						if (m_core_text_shape == m_event_shape_pressed) {
							//�����ꂽ�_����L�����b�g�ʒu��ݒ肵, �������I������.
							m_main_sheet.set_attr_to(m_event_shape_pressed);
							////
							bool trail;
							const auto end = m_core_text_shape->get_text_pos(m_event_pos_curr, trail);
							undo_push_text_select(m_core_text_shape, m_main_sheet.m_select_start, end, trail);
							changed = true;
						}
						// �����ꂽ�}�`�͕ҏW���̐}�`�ȊO
						else {
							m_main_sheet.set_attr_to(m_event_shape_pressed);
							if (m_event_shape_last == nullptr) {
								m_event_shape_last = m_event_shape_pressed;
							}
							if (select_shape_range(m_event_shape_last, m_event_shape_pressed)) {
								changed = true;
							}
							// ���݂ɕҏW���̕����񂪂���Β��f��, �����ꂽ�������ҏW���̐}�`�ɂ���.
							if (m_core_text_shape != nullptr) {
								m_core_text.NotifyFocusLeave();
								undo_push_text_unselect(m_core_text_shape);
								m_core_text_shape = nullptr;
								changed = true;
							}
						}
					}
					else {
						// �Ō�ɉ����ꂽ�}�`������Ȃ�, �w�肵���͈͂̐}�`��I��, �͈͊O�̐}�`�̑I�����O��.
						if (m_event_shape_last == nullptr) {
							m_event_shape_last = m_event_shape_pressed;
						}
						if (select_shape_range(m_event_shape_last, m_event_shape_pressed)) {
							changed = true;
						}
						////
						// ���݂ɕҏW���̕����񂪂���Β��f����.
						if (m_core_text_shape != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_shape);
							m_core_text_shape = nullptr;
							changed = true;
						}
					}
				}
				// �C���L�[���Ȃ��ꍇ��, 
				else if (args.KeyModifiers() == VirtualKeyModifiers::None) {
					m_event_shape_pressed = nullptr;
					m_event_loc_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_pos_pressed, false, m_event_shape_pressed);
					// �����ꂽ�}�`�͂Ȃ�.
					if (m_event_loc_pressed == LOC_TYPE::LOC_SHEET) {
						if (unselect_shape_all()) {
							m_event_shape_last = nullptr;
							////
							if (m_core_text_shape != nullptr) {
								m_core_text.NotifyFocusLeave();
								undo_push_text_unselect(m_core_text_shape);
								m_core_text_shape = nullptr;
							}
							changed = true;
						}
					}
					// �����ꂽ�}�`�͕�����.
					else if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
						// �����ꂽ�}�`���I������ĂȂ��ꍇ
						//  
						if (!m_event_shape_pressed->is_selected()) {
							m_main_sheet.set_attr_to(m_event_shape_pressed);
							m_event_shape_last = m_event_shape_pressed;

							unselect_shape_all();
							undo_push_select(m_event_shape_pressed);
							if (summary_is_visible()) {
								summary_select(m_event_shape_pressed);
							}
						}
						// �����ꂽ�}�`���ҏW���̕�����łȂ��ꍇ
						//  �ҏW���̕����񂪂������Ȃ�, �ҏW�𒆒f����.
						//  �����ꂽ�}�`��ҏW���̕�����}�`�ɐݒ肷��.
						// �����ꂽ�}�`���ҏW���̕�����ł���ꍇ.
						//  ���͕ϊ����Ȃ�ϊ��𒆒f���邽�߃t�H�[�J�X���͂���.
						///////
						if (m_core_text_shape != m_event_shape_pressed) {
							if (m_core_text_shape != nullptr) {
								m_core_text.NotifyFocusLeave();
								undo_push_text_unselect(m_core_text_shape);
							}
							m_core_text_shape = static_cast<ShapeText*>(m_event_shape_pressed);
							m_core_text.NotifyFocusEnter();
						}
						else {
							if (m_core_text_comp) {
								m_core_text.NotifyFocusLeave();
							}
						}
						bool trail;
						const auto end = m_core_text_shape->get_text_pos(m_event_pos_curr, trail);
						const auto start = trail ? end + 1 : end;
						undo_push_text_select(m_core_text_shape, start, end, trail);
						changed = true;
					}
					// �����ꂽ�}�`�͕�����ȊO.
					else {
						// �����ꂽ�}�`���I������ĂȂ��Ȃ�I������.
						if (!m_event_shape_pressed->is_selected()) {
							m_main_sheet.set_attr_to(m_event_shape_pressed);
							m_event_shape_last = m_event_shape_pressed;
							unselect_shape_all();
							undo_push_select(m_event_shape_pressed);
							if (summary_is_visible()) {
								summary_select(m_event_shape_pressed);
							}
							changed = true;
						}
						// �ҏW���̕�����}�`���������Ȃ�ҏW�𒆒f����.
						////
						if (m_core_text_shape != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_shape);
							m_core_text_shape = nullptr;
							changed = true;
						}
					}
				}
			}
			else if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
				m_event_shape_pressed = nullptr;
				m_event_loc_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_pos_pressed, false, m_event_shape_pressed);
			}
			else {

			}
		}
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
			if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				// PRESS_LBTN �ƈႤ�͎̂��� 2 �_.
				// 1. �C���L�[�������̂Ƃ��͉������Ȃ�����,
				// 2. �C���L�[����������ĂȂ��Ƃ��ł�, m_event_shape_last �̍X�V�͂��Ȃ�����.
				if (args.KeyModifiers() == VirtualKeyModifiers::None) {
					m_event_shape_pressed = nullptr;
					m_event_loc_pressed = slist_hit_test(m_main_sheet.m_shape_list, m_event_pos_pressed, false, m_event_shape_pressed);
					// �����ꂽ�}�`�͂Ȃ�.
					if (m_event_loc_pressed == LOC_TYPE::LOC_SHEET) {
						if (unselect_shape_all()) {
							//m_event_shape_last = nullptr;
							////
							if (m_core_text_shape != nullptr) {
								m_core_text.NotifyFocusLeave();
								undo_push_text_unselect(m_core_text_shape);
								m_core_text_shape = nullptr;
							}
							changed = true;
						}
					}
					// �����ꂽ�}�`�͕�����.
					else if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
						// �����ꂽ�}�`���I������ĂȂ��Ȃ�I������.
						if (!m_event_shape_pressed->is_selected()) {
							m_main_sheet.set_attr_to(m_event_shape_pressed);
							//m_event_shape_last = m_event_shape_pressed;
							unselect_shape_all();
							undo_push_select(m_event_shape_pressed);
							if (summary_is_visible()) {
								summary_select(m_event_shape_pressed);
							}
						}
						// �����ꂽ�}�`���ҏW���̕�����łȂ��ꍇ
						// �ҏW���̕����񂪂������Ȃ�, �ҏW�𒆒f����.
						// �����ꂽ�}�`��ҏW���̕�����}�`�ɐݒ肷��.
						///////
						if (m_core_text_shape != m_event_shape_pressed) {
							if (m_core_text_shape != nullptr) {
								m_core_text.NotifyFocusLeave();
								undo_push_text_unselect(m_core_text_shape);
							}
							m_core_text_shape = static_cast<ShapeText*>(m_event_shape_pressed);
						}
						bool trail;
						const auto end = m_core_text_shape->get_text_pos(m_event_pos_curr, trail);
						const auto start = trail ? end + 1 : end;
						undo_push_text_select(m_core_text_shape, start, end, trail);
						m_core_text.NotifyFocusEnter();
						changed = true;
					}
					// �����ꂽ�}�`�͕�����ȊO.
					else {
						// �����ꂽ�}�`���I������ĂȂ��Ȃ�I������.
						if (!m_event_shape_pressed->is_selected()) {
							m_main_sheet.set_attr_to(m_event_shape_pressed);
							//m_event_shape_last = m_event_shape_pressed;
							unselect_shape_all();
							undo_push_select(m_event_shape_pressed);
							if (summary_is_visible()) {
								summary_select(m_event_shape_pressed);
							}
							changed = true;
						}
						// �ҏW���̕�����}�`���������Ȃ�ҏW�𒆒f����.
						////
						if (m_core_text_shape != nullptr) {
							m_core_text.NotifyFocusLeave();
							undo_push_text_unselect(m_core_text_shape);
							m_core_text_shape = nullptr;
							changed = true;
						}
					}
				}
			}
		}
		if (changed) {
			main_draw();
		}
	}

	//------------------------------
	// �|�C���^�[�̃{�^�����グ��ꂽ.
	//------------------------------
	void MainPage::event_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
//#if defined(_DEBUG)
		if (sender != scp_main_panel()) {
			return;
		}
//#endif
		// �s�b�J�[���Ԓl��߂��܂�, �C�x���g�����������Ȃ����߂̔r��.
		if (!m_mutex_event.try_lock()) {
			wait_cursor_show();
			return;
		}
		m_mutex_event.unlock();

		const auto& panel = sender.as<SwapChainPanel>();

		// �|�C���^�[�̒ǐՂ��~����.
		panel.ReleasePointerCaptures();
		event_set_position(args);
		// ��Ԃ�, ���{�^���������ꂽ��Ԃ����肷��.
		if (m_event_state == EVENT_STATE::PRESS_LBTN) {
			// �{�^�������ꂽ�����Ɖ����ꂽ�����̍���, �N���b�N�̔��莞�Ԉȉ������肷��.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			const auto c_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
			if (t_stamp - m_event_time_pressed <= c_time) {
				// �N���b�N������ԂɑJ�ڂ���.
				m_event_state = EVENT_STATE::CLICK;
				event_set_cursor();
				return;
			}
			// �N���b�N�̊m��
			if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
				// �C���L�[��������ĂȂ��Ȃ�z���グ.
				if ((args.KeyModifiers() & VirtualKeyModifiers::Control) == VirtualKeyModifiers::None) {
					event_eyedropper_detect(m_event_shape_pressed, m_event_loc_pressed);
					status_bar_set_draw();
					return;
				}
				// ����L�[�����Ȃ�f���o��.
				else {
					if (m_event_loc_pressed == LOC_TYPE::LOC_SHEET) {
						undo_push_null();
						undo_push_set<UNDO_T::SHEET_COLOR>(&m_main_sheet, m_eyedropper_color);
					}
					else if (m_event_shape_pressed != nullptr) {
						if (m_event_loc_pressed == LOC_TYPE::LOC_FILL) {
							undo_push_null();
							undo_push_set<UNDO_T::FILL_COLOR>(m_event_shape_pressed, m_eyedropper_color);
						}
						else if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
							undo_push_null();
							undo_push_set<UNDO_T::FONT_COLOR>(m_event_shape_pressed, m_eyedropper_color);
						}
						else if (m_event_loc_pressed == LOC_TYPE::LOC_STROKE) {
							undo_push_null();
							undo_push_set<UNDO_T::STROKE_COLOR>(m_event_shape_pressed, m_eyedropper_color);
						}
					}
				}
			}
		}
		// ��Ԃ�, �N���b�N��ɍ��{�^������������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::CLICK_LBTN) {
			// �{�^���������ꂽ�����Ɖ����ꂽ�����̍����𓾂�.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			const auto c_time = static_cast<uint64_t>(UISettings().DoubleClickTime()) * 1000L;
			// �������N���b�N�̔��莞�Ԉȉ�, �������ꂽ�}�`��������}�`�����肷��.
			if (t_stamp - m_event_time_pressed <= c_time && m_event_shape_pressed != nullptr) {
				if (m_core_text_shape != nullptr) {
					m_core_text.NotifyFocusLeave();
					undo_push_text_unselect(m_core_text_shape);
					m_core_text_shape = nullptr;
				}
				// �_�u���N���b�N�̊m��.
				if (typeid(*m_event_shape_pressed) == typeid(ShapeText)) {
					edit_text_async(static_cast<ShapeText*>(m_event_shape_pressed));
				}
				else if (typeid(*m_event_shape_pressed) == typeid(ShapeArc)) {
					edit_arc_async(static_cast<ShapeArc*>(m_event_shape_pressed));
				}
			}
		}
		// ��Ԃ�, ������������Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_TEXT) {
			bool trail;
			const auto end = m_core_text_shape->get_text_pos(m_event_pos_curr, trail);
			const auto start = m_main_sheet.m_select_start;
			undo_push_text_select(m_core_text_shape, start, end, trail);
		}
		// ��Ԃ�, �}�`���ړ����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			// ����L�[��������ĂȂ��Ȃ�, �}�`�̈ړ���̒������s�Ȃ�.
			if ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control) {
				event_adjust_after_moving();
			}
			if (!undo_pop_invalid()) {
				main_bbox_update();
				main_panel_size();
			}
		}
		// ��Ԃ�, �}�`��ό`���Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_DEFORM) {
			// ����L�[��������ĂȂ��Ȃ�, �}�`�̕ό`��̒������s�Ȃ�.
			if ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control) {
				event_adjust_after_deforming(args.KeyModifiers());
			}
			if (!undo_pop_invalid()) {
				main_bbox_update();
				main_panel_size();
			}
		}
		// ��Ԃ�, ��`�I�����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_RECT) {
			// ��}�c�[�����I���c�[�������肷��.
			if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				event_finish_rect_selection(args.KeyModifiers());
			}
			// ��}�c�[�����I���c�[���ȊO.
			else {
				// ����L�[��������ĂȂ��Ȃ�, �|�C���^�[�_�𒸓_�܂��͕���ɍ��킹��.
				if ((args.KeyModifiers() & VirtualKeyModifiers::Control) != VirtualKeyModifiers::Control) {
					// �_�Ɠ_����������臒l���[�����傫���Ȃ�, �ق��̒��_�ɍ��킹��.
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
						const double g_len = max(m_main_sheet.m_grid_base, 0.0) + 1.0;
						event_snap_point(m_main_sheet.m_shape_list, boxed, interval, m_snap_grid, g_len, m_event_pos_pressed, m_event_pos_curr);
					}
					// ����ɍ��킹�邩���肷��.
					else if (m_snap_grid) {
						// �����ꂽ�ʒu�旣���ꂽ�ʒu�����̑傫���Ŋۂ߂�.
						const double g_len = max(m_main_sheet.m_grid_base + 1.0, 1.0);
						pt_round(m_event_pos_pressed, g_len, m_event_pos_pressed);
						pt_round(m_event_pos_curr, g_len, m_event_pos_curr);
					}
				}
				// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̍����� x �l�܂��� y �l�� 1 �ȏォ���肷��.
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
		// ��Ԃ�, �E�{�^������������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
			if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
			}
			else {
				// �|�b�v�A�b�v (�t���C�A�E�g) ���\������Ă���Ȃ炻����������.
				if (scp_main_panel().ContextFlyout() != nullptr) {
					scp_main_panel().ContextFlyout().Hide();
					scp_main_panel().ContextFlyout(nullptr);
				}
				// �|�b�v�A�b�v���j���[��\������.
				scp_main_panel().ContextFlyout(mf_popup_menu());
			}
		}
		// ��Ԃ�, ������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::BEGIN) {
			// �{���͏�����Ԃł��̃n���h���[���Ăяo�����͂��͂Ȃ���,
			// �R���e���c�_�C�A���O�⃁�j���[���I�������Ƃ��Ăяo����邱�Ƃ�������.
			return;
		}
		// ������Ԃɖ߂�.
		m_event_state = EVENT_STATE::BEGIN;
		//m_event_shape_pressed = nullptr;
		//m_event_loc_pressed = LOC_TYPE::LOC_SHEET;
		main_draw();
	}

	//------------------------------
	// �J�[�\����ݒ肷��.
	//------------------------------
	void MainPage::event_set_cursor(void)
	{
		// ��}�c�[�����F���o.
		if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
			const CoreCursor& curs = m_eyedropper_filled ? CURS_EYEDROPPER2 : CURS_EYEDROPPER1;
			Window::Current().CoreWindow().PointerCursor(curs);
		}
		// ��}�c�[�����I���c�[���ȊO����Ԃ��E�{�^�������łȂ�.
		else if (m_drawing_tool != DRAWING_TOOL::SELECT && m_event_state != EVENT_STATE::PRESS_RBTN) {
			Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
		}
		// �`��̔r����������b�N�ł��Ȃ������肷��.
		else if (!m_mutex_draw.try_lock()) {
			Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
		}
		else {
			// �`��̔r����������b�N�ł����Ȃ�, �������ɉ�������.
			m_mutex_draw.unlock();
			Shape* s;
			const auto loc = slist_hit_test(m_main_sheet.m_shape_list, m_event_pos_curr, false, s);
			if (loc == LOC_TYPE::LOC_SHEET) {
				Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
			}
			else if (m_undo_select_cnt > 1) {
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
					// �}�`�̃N���X��, ���p�`�܂��͋Ȑ��ł��邩���肷��.
					if (s != nullptr) {
						if (typeid(*s) == typeid(ShapeLine) ||
							typeid(*s) == typeid(ShapePoly) ||
							typeid(*s) == typeid(ShapeBezier) ||
							typeid(*s) == typeid(ShapeArc)) {
							// �}�`�̕��ʂ�, ���_�̐��𒴂��Ȃ������肷��.
							if (loc >= LOC_TYPE::LOC_P0 && loc < LOC_TYPE::LOC_P0 + static_cast<ShapePath*>(s)->m_lineto.size() + 1) {
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
	// �|�C���^�[�̌��݈ʒu�Ɋi�[����.
	//------------------------------
	void MainPage::event_set_position(PointerRoutedEventArgs const& args)
	{
		const auto scale = m_main_scale;
		// ���E�{�b�N�X�̍���_�ɃX�N���[���̒l������, �\������Ă��鍶��_�𓾂�.
		D2D1_POINT_2F q;
		pt_add(m_main_bbox_lt, sb_horz().Value(), sb_vert().Value(), q);
		// �����Ƃ��ēn���ꂽ�_��,
		// �g�嗦�̋t�����悶, �\������Ă��鍶��_���������_�𓾂�.
		// ����ꂽ�_��, �|�C���^�[�̌��݈ʒu�Ɋi�[����.
		const auto p{ args.GetCurrentPoint(scp_main_panel()).Position() };
		pt_mul_add(D2D1_POINT_2F{ p.X, p.Y }, 1.0 / scale, q, m_event_pos_curr);
	}

	//------------------------------
	// �|�C���^�[�̃z�C�[���{�^�������삳�ꂽ.
	//------------------------------
	void MainPage::event_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_main_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// �s�b�J�[���Ԓl��߂��܂�, �C�x���g�����������Ȃ����߂̔r��.
		if (!m_mutex_event.try_lock()) {
			wait_cursor_show();
			return;
		}
		m_mutex_event.unlock();

		// ����L�[��������Ă邩���肷��.
		const auto mod = args.KeyModifiers();
		if ((mod & VirtualKeyModifiers::Control) == VirtualKeyModifiers::Control) {
			// �g��k��
			const int32_t w_delta = args.GetCurrentPoint(scp_main_panel()).Properties().MouseWheelDelta();
			if (w_delta > 0 && m_main_scale < 16.f / 1.1f - FLT_MIN) {
				m_main_scale *= 1.1f;
				main_panel_size();
				main_draw();
				status_bar_set_pos();
				status_bar_set_zoom();
			}
			else if (w_delta < 0 && m_main_scale > 0.25f * 1.1f + FLT_MIN) {
				m_main_scale /= 1.1f;
				main_panel_size();
				main_draw();
				status_bar_set_pos();
				status_bar_set_zoom();
			}
		}
		// �V�t�g�L�[��������Ă邩���肷��.
		else if ((mod & VirtualKeyModifiers::Shift) == VirtualKeyModifiers::Shift) {
			// ���X�N���[��.
			const int32_t w_delta = args.GetCurrentPoint(scp_main_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_horz(), w_delta, m_main_scale)) {
				main_draw();
				status_bar_set_pos();
			}
		}
		// ����������ĂȂ������肷��.
		else if (mod == VirtualKeyModifiers::None) {
			// �c�X�N���[��.
			const int32_t w_delta = args.GetCurrentPoint(scp_main_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_vert(), w_delta, m_main_scale)) {
				main_draw();
				status_bar_set_pos();
			}
		}
	}

}