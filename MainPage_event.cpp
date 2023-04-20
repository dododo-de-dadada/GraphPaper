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
	static void event_pos_snap_to(const SHAPE_LIST& slist, const bool boxed, const double interval, const bool g_snap, const double g_len, D2D1_POINT_2F& pressed, D2D1_POINT_2F& released);
	// �ł��߂�����𓾂�.
	static bool event_get_nearby_grid(const SHAPE_LIST& slist, const float g_len, D2D1_POINT_2F& g_pos) noexcept;
	// �ł��߂��_�𓾂�.
	static bool event_get_nearby_point(const SHAPE_LIST& slist, const double interval, D2D1_POINT_2F& v_pos) noexcept;
	// �}�`������X�^�b�N�Ɋ܂܂�邩���肷��.
	static bool event_undo_contain_shape(const UNDO_STACK& ustack, const Shape* s) noexcept;
	// �}�`���X�g�𐮗�����.
	static void event_reduce_slist(SHAPE_LIST& slist, const UNDO_STACK& ustack, const UNDO_STACK& r_stack) noexcept;
	// �}�E�X�z�C�[���̒l�ŃX�N���[������.
	static bool event_scroll_by_wheel_delta(const ScrollBar& scroll_bar, const int32_t delta, const float scale);

	// �����ꂽ�ʒu�Ɨ����ꂽ�ʒu�𒲐�����.
	static void event_pos_snap_to(
		const SHAPE_LIST& slist,	// �}�`���X�g
		const bool boxed,	// �����̑Ώۂ�, �}�`���͂ޗ̈�Ƃ���Ȃ� true, �}�`�̒��_��ΏۂƂ���Ȃ� false 
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

		// �����̑Ώۂ��̈�Ȃ�, �E��ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
		if (boxed && slist_find_vertex_closest(slist, box[1], interval, p[1])) {
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

		// �����̑Ώۂ��̈�Ȃ�, �����ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
		if (boxed && slist_find_vertex_closest(slist, box[3], interval, p[3])) {
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
	static bool event_get_nearby_grid(
		const SHAPE_LIST& slist,	// �}�`���X�g
		const float g_len,	// ����̑傫��
		D2D1_POINT_2F& g_pos	// �}�`�������ւ̈ʒu�x�N�g��
	) noexcept
	{
		D2D1_POINT_2F p[2 + N_GON_MAX];
		D2D1_POINT_2F g;	// ����̑傫���Ŋۂ߂��ʒu
		D2D1_POINT_2F d;	// �ۂ߂��ʒu�ƌ��̈ʒu�̍���
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
				pt_sub(g, p[i], d);
				const auto g_abs = pt_abs2(d);	// �ۂ߂��ʒu�ƌ��̈ʒu�̋����̓��
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

	// �ł��߂����_�𓾂�.
	// �߂�l	���������Ȃ� true
	static bool event_get_nearby_point(
		const SHAPE_LIST& slist,	// �}�`���X�g
		const double interval,	// �Ԋu (����ȏ㗣�ꂽ�_�͖�������)
		D2D1_POINT_2F& pos	// �ł��߂����_�ւ̈ʒu�x�N�g��
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
			pt_sub(n, q, pos);
		}
		return flag;
	}

	// �}�E�X�z�C�[���̒l�ŃX�N���[������.
	static bool event_scroll_by_wheel_delta(
		const ScrollBar& scroll_bar,	// �X�N���[���o�[
		const int32_t delta,	// �}�E�X�z�C�[���̃f���^�l
		const float scale	// �y�[�W�̔{��
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

	// �}�`���X�g�𐮗�����.
	static void event_reduce_slist(
		SHAPE_LIST& slist,	// �}�`���X�g
		const UNDO_STACK& ustack,	// ���ɖ߂�����X�^�b�N
		const UNDO_STACK& rstack	// ��蒼������X�^�b�N
	) noexcept
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
	// �߂�l	�܂ޏꍇ true.
	static bool event_undo_contain_shape(
		const UNDO_STACK& ustack,	// ����X�^�b�N
		const Shape* s	// �}�`
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
	// �}�`�̍쐬���I������.
	// start	�n�_
	// pos	�Ίp�_�ւ̈ʒu�x�N�g��
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
		else {
			return;
		}
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		event_reduce_slist(m_main_page.m_shape_list, m_ustack_undo, m_ustack_redo);
		undo_push_append(s);
		undo_push_select(s);
		undo_push_null();
		undo_menu_is_enabled();
		xcvd_menu_is_enabled();
		main_bbox_update(s);
		main_panel_size();
		m_event_shape_prev = s;
		main_draw();
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_append(s);
			summary_select(s);
		}
		status_bar_set_pos();
	}

	//------------------------------
	// ������}�`�̍쐬���I������.
	// start	�n�_
	// pos	�Ίp�_�ւ̈ʒu�x�N�g��
	//------------------------------
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
				//s->fit_frame_to_text(m_main_page.m_snap_grid ? m_main_page.m_grid_base + 1.0f : 0.0f);
			}
			m_fit_text_frame = ck_fit_text_frame().IsChecked().GetBoolean();
			event_reduce_slist(m_main_page.m_shape_list, m_ustack_undo, m_ustack_redo);
			undo_push_append(s);
			undo_push_select(s);
			undo_push_null();
			undo_menu_is_enabled();
			xcvd_menu_is_enabled();
			m_event_shape_prev = s;
			main_bbox_update(s);
			main_panel_size();
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_append(s);
				summary_select(s);
			}
		}
		// �|�C���^�[�̉����ꂽ��Ԃ�������Ԃɖ߂�.
		m_event_state = EVENT_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_loc_pressed = LOC_TYPE::LOC_PAGE;
		main_draw();
		status_bar_set_pos();
	}

	//------------------------------
	// �}�`�̕ό`���I������.
	//------------------------------
	void MainPage::event_finish_deforming(void)
	{
		//const auto g_snap = m_main_page.m_snap_grid;
		const auto g_snap = m_snap_grid;
		if (g_snap && m_snap_point >= FLT_MIN) {
			// ���݂̈ʒu��, ��������̑傫���Ɋۂ߂��ʒu�ƊԂ̋��������߂�.
			D2D1_POINT_2F p;
			D2D1_POINT_2F d;
			pt_round(m_event_pos_curr, m_main_page.m_grid_base + 1.0, p);
			pt_sub(p, m_event_pos_curr, d);
			float dist = min(static_cast<float>(sqrt(pt_abs2(d))), m_snap_point) / m_main_scale;
			if (slist_find_vertex_closest(
				m_main_page.m_shape_list, m_event_pos_curr, dist, p)) {
				// ����Ƃ̋������߂����_�����������Ȃ�, ���̋����ɓ���ւ���.
				pt_sub(p, m_event_pos_curr, d);
				dist = static_cast<float>(sqrt(pt_abs2(d))) / m_main_scale;
			}
			// �ߖT�̒��_�ɂ���ē_���ς��Ȃ����������肷��.
			if (!m_event_shape_pressed->set_pos_loc(m_event_pos_curr, m_event_loc_pressed, dist, m_image_keep_aspect)) {
				// �ς��Ȃ������Ȃ��, ����ɍ��킹��.
				m_event_shape_pressed->set_pos_loc(p, m_event_loc_pressed, 0.0f, m_image_keep_aspect);
			}
		}
		else if (g_snap) {
			pt_round(m_event_pos_curr, m_main_page.m_grid_base + 1.0, m_event_pos_curr);
			m_event_shape_pressed->set_pos_loc(m_event_pos_curr, m_event_loc_pressed, 0.0f, m_image_keep_aspect);
		}
		else if (m_snap_point / m_main_scale >= FLT_MIN) {
			slist_find_vertex_closest(m_main_page.m_shape_list, m_event_pos_curr, m_snap_point / m_main_scale, m_event_pos_curr);
			m_event_shape_pressed->set_pos_loc(m_event_pos_curr, m_event_loc_pressed, m_snap_point / m_main_scale, m_image_keep_aspect);
		}
		if (!undo_pop_invalid()) {
			undo_push_null();
			undo_menu_is_enabled();
			main_bbox_update();
			main_panel_size();
		}
	}

	//------------------------------
	// �}�`�̈ړ����I������.
	//------------------------------
	void MainPage::event_finish_moving(void)
	{
		// ����ɂ�������, �����_�ɂ�������.
		if (m_snap_grid && m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{};	// ����Ƃ̍���
			if (event_get_nearby_grid(m_main_page.m_shape_list, m_main_page.m_grid_base + 1.0f, p)) {
				D2D1_POINT_2F q{};	// ���_�Ƃ̍���
				if (event_get_nearby_point(m_main_page.m_shape_list, m_snap_point / m_main_scale, q) && pt_abs2(q) < pt_abs2(p)) {
					// ����ƒ��_�̂ǂ��炩�Z�����̋�����, �����ɓ���.
					p = q;
				}
				// ����ꂽ�����̕�����, �I�����ꂽ�}�`���ړ�����.
				slist_move_selected(m_main_page.m_shape_list, p);
			}
		}
		// ����ɂ�������
		else if (m_snap_grid) {
			D2D1_POINT_2F p{};	// ����Ƃ̍���
			if (event_get_nearby_grid(m_main_page.m_shape_list, m_main_page.m_grid_base + 1.0f, p)) {
				slist_move_selected(m_main_page.m_shape_list, p);
			}
		} 
		// ���_�ɂ�������
		else if (m_snap_point / m_main_scale >= FLT_MIN) {
			D2D1_POINT_2F p{};	// ���_�Ƃ̍���
			if (event_get_nearby_point(m_main_page.m_shape_list, m_snap_point / m_main_scale, p)) {
				slist_move_selected(m_main_page.m_shape_list, p);
			}
		}
		if (!undo_pop_invalid()) {
			undo_push_null();
			undo_menu_is_enabled();
			main_bbox_update();
			main_panel_size();
		}
	}

	//------------------------------
	// ��`�I�����I������.
	// k_mod	�C���L�[
	//------------------------------
	void MainPage::event_finish_rect_selection(const VirtualKeyModifiers k_mod)
	{
		// �C���L�[���R���g���[�������肷��.
		if (k_mod == VirtualKeyModifiers::Control) {
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
			if (toggle_inside(lt, rb)) {
				xcvd_menu_is_enabled();
			}
		}
		// �C���L�[��������ĂȂ������肷��.
		else if (k_mod == VirtualKeyModifiers::None) {
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
			if (select_inside(lt, rb)) {
				xcvd_menu_is_enabled();
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

		// �|�C���^�[�̉����ꂽ��Ԃ�, ������Ԃ����肷��.
		if (m_event_state == EVENT_STATE::BEGIN) {
			event_set_cursor();
		}
		// ��Ԃ�. �N���b�N������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::CLICK) {
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̒���������,
			// �������N���b�N���苗���𒴂��邩���肷��.
			const auto raw_dpi = DisplayInformation::GetForCurrentView().RawDpiX();
			const auto log_dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
			D2D1_POINT_2F pos;	// �ߖT�ւ̈ʒu�x�N�g��
			pt_sub(m_event_pos_curr, m_event_pos_pressed, pos);
			if (pt_abs2(pos) * m_main_scale > m_event_click_dist * raw_dpi / log_dpi) {
			//if (pt_abs2(pos) * m_main_page.m_page_scale > m_event_click_dist * raw_dpi / log_dpi) {
				// ������Ԃɖ߂�.
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
			// �|�C���^�[�̌��݈ʒu�ƑO��ʒu�̍����𓾂�.
			D2D1_POINT_2F pos;
			pt_sub(m_event_pos_curr, m_event_pos_prev, pos);
			slist_move_selected(m_main_page.m_shape_list, pos);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_event_pos_prev = m_event_pos_curr;
			main_draw();
		}
		// ��Ԃ�, �}�`��ό`���Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_DEFORM) {
			// �|�C���^�[�̌��݈ʒu��, �|�C���^�[�������ꂽ�}�`�̕��ʂ̓_�Ɋi�[����.
			m_event_shape_pressed->set_pos_loc(m_event_pos_curr, m_event_loc_pressed, 0.0f, m_image_keep_aspect);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_event_pos_prev = m_event_pos_curr;
			main_draw();
		}
		// ��Ԃ�, ���{�^���������Ă�����, �܂���
		// �N���b�N��ɍ��{�^������������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_LBTN || 
			m_event_state == EVENT_STATE::CLICK_LBTN) {
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�Ƃ̍����𓾂�.
			D2D1_POINT_2F pos;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, pos);
			// �������N���b�N�̔��苗���𒴂��邩���肷��.
			if (pt_abs2(pos) > m_event_click_dist / m_main_scale) {
			//if (pt_abs2(pos) > m_event_click_dist / m_main_page.m_page_scale) {
				// ��}�c�[�����I���c�[���ȊO�����肷��.
				if (m_drawing_tool != DRAWING_TOOL::SELECT) {
					// ��`�I�����Ă����ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::PRESS_RECT;
				}
				// �����ꂽ�}�`���k�������肷��.
				else if (m_event_shape_pressed == nullptr) {
					// ��`�I�����Ă����ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::PRESS_RECT;
					// �\���J�[�\�����J�[�\���ɐݒ肷��.
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
				}
				// �I�����ꂽ�}�`�̐��� 1 �𒴂���,
				// �܂��͉����ꂽ�}�`�̕��ʂ����g, ����, �����񂩂𔻒肷��.
				else if (m_list_sel_cnt > 1 ||
					m_event_loc_pressed == LOC_TYPE::LOC_STROKE ||
					m_event_loc_pressed == LOC_TYPE::LOC_FILL ||
					m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
					// �}�`���ړ����Ă����ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::PRESS_MOVE;
					// �|�C���^�[�̌��݈ʒu��O��ʒu�ɕۑ�����.
					m_event_pos_prev = m_event_pos_curr;
					undo_push_move(pos);
				}
				// �|�C���^�[�������ꂽ�̂��}�`�̊O���ȊO�����肷��.
				else if (m_event_loc_pressed != LOC_TYPE::LOC_PAGE) {
					// �}�`��ό`���Ă����ԂɑJ�ڂ���.
					// �|�C���^�[�̌��݈ʒu��O��ʒu�ɕۑ�����.
					m_event_state = EVENT_STATE::PRESS_DEFORM;
					m_event_pos_prev = m_event_pos_curr;
					undo_push_position(m_event_shape_pressed, m_event_loc_pressed);
					m_event_shape_pressed->set_pos_loc(m_event_pos_curr, m_event_loc_pressed, 0.0f, m_image_keep_aspect);
				}
				main_draw();
			}
		}
	}

	/*
	void MainPage::event_menu_is_checked(void)
	{
		arrow_style_is_checked(m_main_page.m_arrow_style);
		cap_style_is_checked(m_main_page.m_stroke_cap);
		dash_style_is_checked(m_main_page.m_dash_style);
		font_style_is_checked(m_main_page.m_font_style);
		font_stretch_is_checked(m_main_page.m_font_stretch);
		font_weight_is_checked(m_main_page.m_font_weight);
		join_style_is_checked(m_main_page.m_join_style);
		stroke_width_is_checked(m_main_page.m_stroke_width);
		text_align_horz_is_checked(m_main_page.m_text_align_horz);
		text_align_vert_is_checked(m_main_page.m_text_align_vert);
		grid_emph_is_checked(m_main_page.m_grid_emph);
		grid_show_is_checked(m_main_page.m_grid_show);
		background_color_is_checked(m_background_show, m_background_color);
		tmfi_menu_snap_grid().IsChecked(m_snap_grid);
		background_color_is_checked(m_background_show, m_background_color);
	}
	*/
	void MainPage::event_arrange_popup_prop(const bool visible, const Shape* s)
	{
		const auto v = visible ? Visibility::Visible : Visibility::Collapsed;
		mfsi_popup_dash_style().Visibility(v);
		mfi_popup_dash_pat().Visibility(v);
		mfsi_popup_stroke_width().Visibility(v);
		if (visible && s->exist_cap()) {
			mfsi_popup_cap_style().Visibility(Visibility::Visible);
		}
		else {
			mfsi_popup_cap_style().Visibility(Visibility::Collapsed);
		}
		if (visible && s->exist_join()) {
			mfsi_popup_join_style().Visibility(Visibility::Visible);
		}
		else {
			mfsi_popup_join_style().Visibility(Visibility::Collapsed);
		}
		if (visible && s->exist_cap()) {
			mfs_popup_sepa_stroke_arrow().Visibility(Visibility::Visible);
			mfsi_popup_arrow_style().Visibility(Visibility::Visible);
			mfi_popup_arrow_size().Visibility(Visibility::Visible);
		}
		else {
			mfs_popup_sepa_stroke_arrow().Visibility(Visibility::Collapsed);
			mfsi_popup_arrow_style().Visibility(Visibility::Collapsed);
			mfi_popup_arrow_size().Visibility(Visibility::Collapsed);
		}
		mfs_popup_sepa_arrow_color().Visibility(v);
		mfi_popup_stroke_color().Visibility(v);
		mfi_popup_fill_color().Visibility(v);
	}

	void MainPage::event_arrange_popup_font(const bool visible)
	{
		const auto v = visible ? Visibility::Visible : Visibility::Collapsed;
		mfi_popup_font_family().Visibility(v);
		mfi_popup_font_size().Visibility(v);
		mfsi_popup_font_weight().Visibility(v);
		mfsi_popup_font_weight().Visibility(v);
		mfsi_popup_font_stretch().Visibility(v);
		mfsi_popup_font_style().Visibility(v);
		mfs_popup_sepa_font_text().Visibility(v);
		mfsi_popup_text_align_horz().Visibility(v);
		mfsi_popup_text_align_vert().Visibility(v);
		mfi_popup_text_line_sp().Visibility(v);
		mfi_popup_text_pad().Visibility(v);
		mfi_popup_font_color().Visibility(v);
	}

	void MainPage::event_arrange_popup_image(const bool visible)
	{
		const auto v = visible ? Visibility::Visible : Visibility::Collapsed;
		tmfi_menu_meth_image_keep_asp_2().Visibility(v);
		mfi_popup_image_revert().Visibility(v);
		mfi_popup_image_opac().Visibility(v);
	}

	void MainPage::event_arrange_popup_layout(const bool visible)
	{
		const auto v = visible ? Visibility::Visible : Visibility::Collapsed;
		mfsi_popup_grid_show().Visibility(v);
		mfsi_popup_grid_len().Visibility(v);
		mfsi_popup_grid_emph().Visibility(v);
		mfi_popup_grid_color().Visibility(v);
		mfs_popup_sepa_grid_page().Visibility(v);
		mfi_popup_page_size().Visibility(v);
		mfi_popup_page_color().Visibility(v);
		mfs_popup_sepa_layout_zoom().Visibility(v);
		mfsi_popup_layout_zoom().Visibility(v);
		mfsi_popup_background_pattern().Visibility(v);
	}

	//------------------------------
	// �|�b�v�A�b�v���j���[��\������.
	//------------------------------
	void MainPage::event_show_popup(void)
	{
		// �|�b�v�A�b�v���\������Ă���Ȃ炻����������.
		if (scp_main_panel().ContextFlyout() != nullptr) {
			scp_main_panel().ContextFlyout(nullptr);
		}
		Shape* pressed = m_event_shape_pressed;
		uint32_t loc_pressed = m_event_loc_pressed;
		undo_menu_is_enabled();
		int undo_visible = 0;
		if (mfi_popup_undo().IsEnabled()) {
			mfi_popup_undo().Visibility(Visibility::Visible);
			undo_visible++;
		}
		else {
			mfi_popup_undo().Visibility(Visibility::Collapsed);
		}
		if (mfi_popup_redo().IsEnabled()) {
			mfi_popup_redo().Visibility(Visibility::Visible);
			undo_visible++;
		}
		else {
			mfi_popup_redo().Visibility(Visibility::Collapsed);
		}
		xcvd_menu_is_enabled();
		int xcvd_visible = 0;
		if (mfi_popup_xcvd_cut().IsEnabled()) {
			mfi_popup_xcvd_cut().Visibility(Visibility::Visible);
			xcvd_visible++;
		}
		else {
			mfi_popup_xcvd_cut().Visibility(Visibility::Collapsed);
		}
		if (mfi_popup_xcvd_copy().IsEnabled()) {
			mfi_popup_xcvd_copy().Visibility(Visibility::Visible);
			xcvd_visible++;
		}
		else {
			mfi_popup_xcvd_copy().Visibility(Visibility::Collapsed);
		}
		if (mfi_popup_xcvd_paste().IsEnabled()) {
			mfi_popup_xcvd_paste().Visibility(Visibility::Visible);
			xcvd_visible++;
		}
		else {
			mfi_popup_xcvd_paste().Visibility(Visibility::Collapsed);
		}
		if (mfi_popup_xcvd_delete().IsEnabled()) {
			mfi_popup_xcvd_delete().Visibility(Visibility::Visible);
			xcvd_visible++;
		}
		else {
			mfi_popup_xcvd_delete().Visibility(Visibility::Collapsed);
		}
		if ((undo_visible > 0 && xcvd_visible > 1) ||
			(undo_visible > 1 && xcvd_visible > 0)) {
			mfs_popup_sepa_undo_xcvd().Visibility(Visibility::Visible);
		}
		else {
			mfs_popup_sepa_undo_xcvd().Visibility(Visibility::Collapsed);
		}
		int select_visible = 0;
		if (mfi_popup_select_all().IsEnabled()) {
			mfi_popup_select_all().Visibility(Visibility::Visible);
			select_visible++;
		}
		else {
			mfi_popup_select_all().Visibility(Visibility::Collapsed);
		}
		if (mfsi_popup_order().IsEnabled()) {
			mfsi_popup_order().Visibility(Visibility::Visible);
			select_visible++;
		}
		else {
			mfsi_popup_order().Visibility(Visibility::Collapsed);
		}
		if ((undo_visible + xcvd_visible > 0 && select_visible > 1) ||
			(undo_visible + xcvd_visible > 1 && select_visible > 0)) {
			mfs_popup_sepa_xcvd_select().Visibility(Visibility::Visible);
		}
		else {
			mfs_popup_sepa_xcvd_select().Visibility(Visibility::Collapsed);
		}
		int group_visible = 0;
		if (mfi_popup_group().IsEnabled()) {
			mfi_popup_group().Visibility(Visibility::Visible);
			group_visible++;
		}
		else {
			mfi_popup_group().Visibility(Visibility::Collapsed);
		}
		if (mfi_popup_ungroup().IsEnabled()) {
			mfi_popup_ungroup().Visibility(Visibility::Visible);
			group_visible++;
		}
		else {
			mfi_popup_ungroup().Visibility(Visibility::Collapsed);
		}
		if ((undo_visible + xcvd_visible + select_visible > 0 && group_visible > 1) ||
			(undo_visible + xcvd_visible + select_visible > 1 && group_visible > 0)) {
			mfs_popup_sepa_select_group().Visibility(Visibility::Visible);
		}
		else {
			mfs_popup_sepa_select_group().Visibility(Visibility::Collapsed);
		}
		int edit_visible = 0;
		if (mfsi_popup_edit_poly_end().IsEnabled()) {
			mfsi_popup_edit_poly_end().Visibility(Visibility::Visible);
			edit_visible++;
		}
		else {
			mfsi_popup_edit_poly_end().Visibility(Visibility::Collapsed);
		}
		if (mfi_popup_edit_arc().IsEnabled()) {
			mfi_popup_edit_arc().Visibility(Visibility::Visible);
			edit_visible++;
		}
		else {
			mfi_popup_edit_arc().Visibility(Visibility::Collapsed);
		}
		if (mfi_popup_edit_text().IsEnabled()) {
			mfi_popup_edit_text().Visibility(Visibility::Visible);
			edit_visible++;
		}
		else {
			mfi_popup_edit_text().Visibility(Visibility::Collapsed);
		}
		if (mfi_popup_find_text().IsEnabled()) {
			mfi_popup_find_text().Visibility(Visibility::Visible);
			edit_visible++;
		}
		else {
			mfi_popup_find_text().Visibility(Visibility::Collapsed);
		}
		if (mfi_popup_fit_text_frame().IsEnabled()) {
			mfi_popup_fit_text_frame().Visibility(Visibility::Visible);
			edit_visible++;
		}
		else {
			mfi_popup_fit_text_frame().Visibility(Visibility::Collapsed);
		}
		if ((undo_visible + xcvd_visible + select_visible + group_visible > 0 && edit_visible > 1) ||
			(undo_visible + xcvd_visible + select_visible + group_visible > 1 && edit_visible > 0)) {
			mfs_popup_sepa_group_edit().Visibility(Visibility::Visible);
		}
		else {
			mfs_popup_sepa_group_edit().Visibility(Visibility::Collapsed);
		}
		if (undo_visible + xcvd_visible + select_visible + group_visible + edit_visible > 0) {
			mfs_popup_sepa_edit_stroke().Visibility(Visibility::Visible);
		}
		else {
			mfs_popup_sepa_edit_stroke().Visibility(Visibility::Collapsed);
		}

		// �����ꂽ�}�`���k��, �܂��͉����ꂽ�}�`�̕��ʂ��O�������肷��.
		if (pressed == nullptr || loc_pressed == LOC_TYPE::LOC_PAGE) {
			event_arrange_popup_prop(false, pressed);
			event_arrange_popup_font(false);
			event_arrange_popup_image(false);
			event_arrange_popup_layout(true);
		}
		else {
			// �����ꂽ�}�`�̑����l��\���Ɋi�[����.
			m_main_page.set_attr_to(pressed);
			// ���j���[�o�[���X�V����
			dash_style_is_checked(m_main_page.m_dash_style);
			stroke_width_is_checked(m_main_page.m_stroke_width);
			cap_style_is_checked(m_main_page.m_stroke_cap);
			join_style_is_checked(m_main_page.m_join_style);
			arrow_style_is_checked(m_main_page.m_arrow_style);
			font_weight_is_checked(m_main_page.m_font_weight);
			font_stretch_is_checked(m_main_page.m_font_stretch);
			font_style_is_checked(m_main_page.m_font_style);
			text_align_horz_is_checked(m_main_page.m_text_align_horz);
			text_align_vert_is_checked(m_main_page.m_text_align_vert);
			grid_emph_is_checked(m_main_page.m_grid_emph);
			grid_show_is_checked(m_main_page.m_grid_show);

			// �����ꂽ���ʂ�������Ȃ�, �t�H���g�̃��j���[���ڂ�\������.
			if (loc_pressed == LOC_TYPE::LOC_TEXT) {
				event_arrange_popup_prop(false, pressed);
				event_arrange_popup_font(true);
				event_arrange_popup_image(false);
			}
			// �����ꂽ���ʂ��摜�Ȃ�, �摜�̃��j���[���ڂ�\������.
			else if (typeid(*pressed) == typeid(ShapeImage)) {
				event_arrange_popup_prop(false, pressed);
				event_arrange_popup_font(false);
				event_arrange_popup_image(true);
			}
			// ��L�ȊO�Ȃ�, �����̃��j���[���ڂ�\������.
			else {
				event_arrange_popup_prop(true, pressed);
				event_arrange_popup_font(false);
				event_arrange_popup_image(false);
			}
			event_arrange_popup_layout(false);
		}
		scp_main_panel().ContextFlyout(mf_popup_menu());
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

		// �|�C���^�[�̃L���v�`�����n�߂�.
		// �����̒l���|���C���^�[�̌��݈ʒu�Ɋi�[����.
		// �|�C���^�[�̃C�x���g�������Ԃ𓾂�.
		// �|�C���^�[�̊g�����𓾂�.
		const SwapChainPanel& swap_chain_panel{
			sender.as<SwapChainPanel>()
		};
		swap_chain_panel.CapturePointer(args.Pointer());
		const uint64_t t_stamp = args.GetCurrentPoint(swap_chain_panel).Timestamp();
		const PointerPointProperties& p_prop = args.GetCurrentPoint(swap_chain_panel).Properties();
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
		// ��}�c�[�����I���c�[�������肷��.
		if (m_drawing_tool == DRAWING_TOOL::SELECT || m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
			m_event_loc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, m_event_shape_pressed);
			m_main_page.set_attr_to(m_event_shape_pressed);
			// ���j���[�o�[���X�V����
			dash_style_is_checked(m_main_page.m_dash_style);
			stroke_width_is_checked(m_main_page.m_stroke_width);
			cap_style_is_checked(m_main_page.m_stroke_cap);
			join_style_is_checked(m_main_page.m_join_style);
			arrow_style_is_checked(m_main_page.m_arrow_style);
			font_weight_is_checked(m_main_page.m_font_weight);
			font_stretch_is_checked(m_main_page.m_font_stretch);
			font_style_is_checked(m_main_page.m_font_style);
			text_align_horz_is_checked(m_main_page.m_text_align_horz);
			text_align_vert_is_checked(m_main_page.m_text_align_vert);
			grid_emph_is_checked(m_main_page.m_grid_emph);
			grid_show_is_checked(m_main_page.m_grid_show);
			// �����ꂽ�̂��}�`�̊O�������肷��.
			if (m_event_loc_pressed == LOC_TYPE::LOC_PAGE) {
				m_event_shape_pressed = nullptr;
				m_event_shape_prev = nullptr;
				// �C���L�[��������Ă��Ȃ��Ȃ��, ���ׂĂ̐}�`�̑I������������.
				// �������ꂽ�}�`�����邩���肷��.
				if (args.KeyModifiers() == VirtualKeyModifiers::None && unselect_all()) {
					xcvd_menu_is_enabled();
					main_draw();
				}
			}
			else {
				// ��Ԃ����{�^���������ꂽ���, �܂���, �E�{�^����������Ă��Ă������ꂽ�}�`���I������ĂȂ������肷��.
				if (m_event_state == EVENT_STATE::PRESS_LBTN ||
					(m_event_state == EVENT_STATE::PRESS_RBTN && !m_event_shape_pressed->is_selected())) {
					select_shape(m_event_shape_pressed, args.KeyModifiers());
				}
			}
		}
	}

	//------------------------------
	// �|�C���^�[�̃{�^�����グ��ꂽ.
	//------------------------------
	void MainPage::event_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		//if (sender == sp_status_bar_panel()) {
		//	const PointerPointProperties& prop = args.GetCurrentPoint(sp_status_bar_panel()).Properties();
		//	if (prop.PointerUpdateKind() == PointerUpdateKind::RightButtonReleased) {
		//		if (ContextFlyout() != nullptr) {
		//			ContextFlyout(nullptr);
		//		}
		//		MenuFlyout popup{};
		//		popup.Items().Append(mfsi_menu_status_bar());
		//		popup.Closed([=](auto, auto) {
		//			ContextFlyout(nullptr);
		//			});
		//		ContextFlyout(popup);
		//	}
		//	return;
		//}
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
				// �N���b�N�̊m��
				if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
					event_eyedropper_detect(m_event_shape_pressed, m_event_loc_pressed);
					status_bar_set_draw();
				}
				else {
					// �N���b�N������ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::CLICK;
					event_set_cursor();
					return;
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
				// �_�u���N���b�N�̊m��.
				if (typeid(*m_event_shape_pressed) == typeid(ShapeText)) {
					edit_text_click_async(nullptr, nullptr);
				}
				else if (typeid(*m_event_shape_pressed) == typeid(ShapeArc)) {
					edit_arc_click_async(nullptr, nullptr);
				}
			}
		}
		// ��Ԃ�, �}�`���ړ����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			event_finish_moving();
		}
		// ��Ԃ�, �}�`��ό`���Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_DEFORM) {
			event_finish_deforming();
		}
		// ��Ԃ�, ��`�I�����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_RECT) {
			// ��}�c�[�����I���c�[�������肷��.
			if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				event_finish_rect_selection(args.KeyModifiers());
			}
			// ��}�c�[�����I���c�[���ȊO.
			else {
				unselect_all();
				// ���_����������臒l���[�����傫�������肷��.
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
					event_pos_snap_to(m_main_page.m_shape_list, boxed, interval, m_snap_grid, g_len, m_event_pos_pressed, m_event_pos_curr);
				}
				// ����ɍ��킹�邩���肷��.
				//else if (m_main_page.m_snap_grid) {
				else if (m_snap_grid) {
					// �����ꂽ�ʒu�旣���ꂽ�ʒu�����̑傫���Ŋۂ߂�.
					const double g_len = max(m_main_page.m_grid_base + 1.0, 1.0);
					pt_round(m_event_pos_pressed, g_len, m_event_pos_pressed);
					pt_round(m_event_pos_curr, g_len, m_event_pos_curr);
				}

				// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̍��������߂�.
				D2D1_POINT_2F pos;	// ���݈ʒu�ւ̈ʒu�x�N�g��
				pt_sub(m_event_pos_curr, m_event_pos_pressed, pos);
				// ������ x �l�܂��� y �l�̂����ꂩ�� 1 �ȏォ���肷��.
				if (fabs(pos.x) >= 1.0f || fabs(pos.y) >= 1.0f) {
					if (m_drawing_tool == DRAWING_TOOL::TEXT) {
						event_finish_creating_text_async(m_event_pos_pressed, pos);
						return;
					}
					event_finish_creating(m_event_pos_pressed, pos);
				}
			}
		}
		// ��Ԃ�, �E�{�^������������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
			if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
				Shape* s;
				const uint32_t loc = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed, s);
				if (loc == LOC_TYPE::LOC_PAGE) {
					undo_push_set<UNDO_T::PAGE_COLOR>(&m_main_page, m_eyedropper_color);
					undo_push_null();
					undo_menu_is_enabled();
					//xcvd_menu_is_enabled();
				}
				else if (s != nullptr) {
					if (m_event_loc_pressed == LOC_TYPE::LOC_FILL) {
						undo_push_set<UNDO_T::FILL_COLOR>(s, m_eyedropper_color);
						undo_push_null();
						undo_menu_is_enabled();
						//xcvd_menu_is_enabled();
					}
					else if (m_event_loc_pressed == LOC_TYPE::LOC_TEXT) {
						undo_push_set<UNDO_T::FONT_COLOR>(s, m_eyedropper_color);
						undo_push_null();
						undo_menu_is_enabled();
						//xcvd_menu_is_enabled();
					}
					else if (m_event_loc_pressed == LOC_TYPE::LOC_STROKE) {
						undo_push_set<UNDO_T::STROKE_COLOR>(s, m_eyedropper_color);
						undo_push_null();
						undo_menu_is_enabled();
						//xcvd_menu_is_enabled();
					}
				}
			}
			else {
				event_show_popup();
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
		m_event_shape_pressed = nullptr;
		m_event_loc_pressed = LOC_TYPE::LOC_PAGE;
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
			const auto loc = slist_hit_test(m_main_page.m_shape_list, m_event_pos_curr, s);
			if (loc == LOC_TYPE::LOC_PAGE) {
				Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
			}
			else if (m_list_sel_cnt > 1) {
				Window::Current().CoreWindow().PointerCursor(CURS_SIZE_ALL);
			}
			else {
				switch (loc) {
				case LOC_TYPE::LOC_A_START:
				case LOC_TYPE::LOC_A_END:
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

		// �R���g���[���L�[��������Ă邩���肷��.
		const auto mod = args.KeyModifiers();
		if (mod == VirtualKeyModifiers::Control) {
			// �g��k��
			const int32_t w_delta = args.GetCurrentPoint(scp_main_panel()).Properties().MouseWheelDelta();
			if (w_delta > 0 && m_main_scale < 16.f / 1.1f - FLT_MIN) {
			//if (w_delta > 0 && m_main_page.m_page_scale < 16.f / 1.1f - FLT_MIN) {
				//m_main_page.m_page_scale *= 1.1f;
				//zoom_is_cheched(m_main_page.m_page_scale);
				m_main_scale *= 1.1f;
				zoom_is_checked(m_main_scale);
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
				zoom_is_checked(m_main_scale);
				main_panel_size();
				main_draw();
				status_bar_set_pos();
				status_bar_set_zoom();
			}
		}
		// �V�t�g�L�[��������Ă邩���肷��.
		else if (mod == VirtualKeyModifiers::Shift) {
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