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

	static auto const& CURS_ARROW = CoreCursor(CoreCursorType::Arrow, 0);	// ���J�[�\��
	static auto const& CURS_CROSS = CoreCursor(CoreCursorType::Cross, 0);	// �\���J�[�\��
	static auto const& CURS_SIZE_ALL = CoreCursor(CoreCursorType::SizeAll, 0);	// �ړ��J�[�\��
	static auto const& CURS_SIZE_NESW = CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);	// �E�㍶���J�[�\��
	static auto const& CURS_SIZE_NS = CoreCursor(CoreCursorType::SizeNorthSouth, 0);	// �㉺�J�[�\��
	static auto const& CURS_SIZE_NWSE = CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);	// ����E���J�[�\��
	static auto const& CURS_SIZE_WE = CoreCursor(CoreCursorType::SizeWestEast, 0);	// ���E�J�[�\��
	static auto const& CURS_EYEDROPPER1 = CoreCursor(CoreCursorType::Custom, IDC_CURSOR1);	// �X�|�C�g�J�[�\��
	static auto const& CURS_EYEDROPPER2 = CoreCursor(CoreCursorType::Custom, IDC_CURSOR2);	// �X�|�C�g�J�[�\��

	// ���_�ɍ��킹��悤, �����ꂽ�ʒu�Ɨ����ꂽ�ʒu�𒲐�����.
	static void event_get_pos_snap_to_vertex(const SHAPE_LIST& slist, const bool boxed, const float limit, const bool g_snap, const double g_len, D2D1_POINT_2F& p_pos, D2D1_POINT_2F& r_pos);
	// �I�����ꂽ�}�`�̒��_�ɍł��߂����������, ���_�ƕ���Ƃ̍��������߂�.
	static bool event_get_vector_nearby_grid(const SHAPE_LIST& slist, const float g_len, D2D1_POINT_2F& g_vec) noexcept;
	// ��I���̐}�`�̒��_�̒�����, �I�����ꂽ�}�`�̒��_�ɍł��߂����_������, �Q�_�Ԃ̍��������߂�.
	static bool event_get_vector_nearby_vertex(const SHAPE_LIST& slist, const float d_limit, D2D1_POINT_2F& v_vec) noexcept;
	// �}�`������X�^�b�N�Ɋ܂܂�邩���肷��.
	static bool event_ustack_contain_shape(UNDO_STACK const& ustack, Shape* const s) noexcept;
	// �����t���O�̗��}�`�����X�g����폜����.
	static void event_slist_reduce(SHAPE_LIST& slist, UNDO_STACK const& ustack, UNDO_STACK const& r_stack) noexcept;
	// �}�E�X�z�C�[���̒l�ŃX�N���[������.
	static bool event_scroll_by_wheel_delta(const ScrollBar& scroll_bar, const int32_t delta, const float scale);

	//------------------------------
	// ���_�ɍ��킹��悤, �����ꂽ�ʒu�Ɨ����ꂽ�ʒu�𒲐�����.
	// slist	�}�`���X�g
	// boxed	�����̑Ώۂ�, �}�`���͂ޗ̈�Ƃ���Ȃ� true, �}�`�̒��_��ΏۂƂ���Ȃ� false 
	// limit	��������
	// g_snap	����ɂ��낦��.
	// g_len	����̑傫��
	// p_pos	�����ꂽ�ʒu
	// r_pos	�����ꂽ�ʒu
	//------------------------------
	static void event_get_pos_snap_to_vertex(
		const SHAPE_LIST& slist, const bool boxed, const float limit, const bool g_snap,
		const double g_len, D2D1_POINT_2F& p_pos, D2D1_POINT_2F& r_pos)
	{
		D2D1_POINT_2F b_pos[4]{	// �����ꂽ�ʒu�Ɨ����ꂽ�ʒu�ň͂܂ꂽ���`�̒��_
			p_pos, { r_pos.x, p_pos.y }, r_pos, { p_pos.x, r_pos.y },
		};
		double v_abs[4];	// �ʒu�ƒ��_�Ƃ̋��� (�̎���).
		D2D1_POINT_2F v_pos[4];	// ���_�̈ʒu

		// ����ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
		if (slist_find_vertex_closest(slist, b_pos[0], limit, v_pos[0])) {
			D2D1_POINT_2F v_sub;
			pt_sub(v_pos[0], b_pos[0], v_sub);
			v_abs[0] = pt_abs2(v_sub);
		}
		else {
			v_abs[0] = FLT_MAX;
		}

		// �����̑Ώۂ��̈�Ȃ�, �E��ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
		if (boxed && slist_find_vertex_closest(slist, b_pos[1], limit, v_pos[1])) {
			D2D1_POINT_2F v_sub;
			pt_sub(v_pos[1], b_pos[1], v_sub);
			v_abs[1] = pt_abs2(v_sub);
		}
		else {
			v_abs[1] = FLT_MAX;
		}

		// �E���ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
		if (slist_find_vertex_closest(slist, b_pos[2], limit, v_pos[2])) {
			D2D1_POINT_2F v_sub;
			pt_sub(v_pos[2], b_pos[2], v_sub);
			v_abs[2] = pt_abs2(v_sub);
		}
		else {
			v_abs[2] = FLT_MAX;
		}

		// �����̑Ώۂ��̈�Ȃ�, �����ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
		if (boxed && slist_find_vertex_closest(slist, b_pos[3], limit, v_pos[3])) {
			D2D1_POINT_2F v_sub;
			pt_sub(v_pos[3], b_pos[3], v_sub);
			v_abs[3] = pt_abs2(v_sub);
		}
		else {
			v_abs[3] = FLT_MAX;
		}

		// ����ɂ��낦��ꍇ,
		// �����ꂽ�ʒu�Ɨ����ꂽ�ʒu��, �ł��߂��i�q�̈ʒu�𓾂�.
		double g_abs[2];	// ����̊i�q�Ƃ̋��� (�̎���)
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
		// ����ɂ��낦�Ȃ��ꍇ,
		// �����ꂽ�ʒu�Ɨ����ꂽ�ʒu��, �ł��߂��i�q�̈ʒu�Ɋi�[��,
		// ���̋����͍ő�l�Ƃ���.
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

	//------------------------------
	// �I�����ꂽ�}�`�ɍł��߂����������, �}�`�ƕ���Ƃ̍��������߂�.
	// slist	�}�`���X�g
	// g_len	����̑傫��
	// g_vec	�}�`�ƕ���Ƃ̍���
	//------------------------------
	static bool event_get_vector_nearby_grid(
		const SHAPE_LIST& slist, const float g_len, D2D1_POINT_2F& g_vec) noexcept
	{
		D2D1_POINT_2F v_pos[2 + N_GON_MAX];
		D2D1_POINT_2F g_pos;
		D2D1_POINT_2F g_sub;
		double min_d = FLT_MAX;	// �ŒZ����
		for (const auto s : slist) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			s->get_bound(
				D2D1_POINT_2F{ FLT_MAX, FLT_MAX }, D2D1_POINT_2F{ -FLT_MAX, -FLT_MAX },
				v_pos[0], v_pos[1]);
			const auto v_cnt = s->get_verts(v_pos + 2);
			for (size_t i = 0; i < 2 + v_cnt; i++) {
				pt_round(v_pos[i], g_len, g_pos);
				pt_sub(g_pos, v_pos[i], g_sub);
				const auto g_abs = pt_abs2(g_sub);
				if (g_abs < min_d) {
					g_vec = g_sub;
					min_d = g_abs;
					if (g_abs < FLT_MIN) {
						return true;
					}
				}
			}
		}
		return min_d < FLT_MAX;
	}

	//------------------------------
	// ��I���̐}�`�̒��_�̒�����, �I�����ꂽ�}�`�̒��_�ɍł��߂����_������, �Q�_�Ԃ̍��������߂�.
	// slist	�}�`���X�g
	// d_limit	�������� (����ȏ㗣�ꂽ���_�͑ΏۂƂ��Ȃ�)
	// v_vec	�ł��߂����_�Ԃ̍���
	// �߂�l	���������Ȃ� true
	//------------------------------
	static bool event_get_vector_nearby_vertex(
		const SHAPE_LIST& slist, const float d_limit, D2D1_POINT_2F& v_vec) noexcept
	{
		float dd = d_limit * d_limit;
		bool done = false;
		D2D1_POINT_2F v_pos[N_GON_MAX];
		D2D1_POINT_2F w_pos{};
		D2D1_POINT_2F n_pos{};	// �ߖT�_
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

	//------------------------------
	// �}�E�X�z�C�[���̒l�ŃX�N���[������.
	// scroll	�X�N���[���o�[
	// delta	�}�E�X�z�C�[���̃f���^�l
	// scale	�y�[�W�̔{��
	//------------------------------
	static bool event_scroll_by_wheel_delta(
		const ScrollBar& scroll, const int32_t delta, const float scale)
	{
		constexpr double DELTA = 32.0;
		double val = scroll.Value();
		double limit = 0.0;
		if (delta < 0 && val < (limit = scroll.Maximum())) {
			val = min(val + DELTA / scale, limit);
		}
		else if (delta > 0 && val > (limit = scroll.Minimum())) {
			val = max(val - DELTA / scale, limit);
		}
		else {
			return false;
		}
		scroll.Value(val);
		return true;
	}

	//------------------------------
	// �����t���O�̗��}�`�����X�g����폜����.
	// ������, ����X�^�b�N�ŎQ�Ƃ���Ă���}�`�͍폜����Ȃ�.
	// slist	�}�`���X�g
	// ustack	���ɖ߂�����X�^�b�N
	// rstack	��蒼������X�^�b�N
	//------------------------------
	static void event_slist_reduce(
		SHAPE_LIST& slist, UNDO_STACK const& ustack, UNDO_STACK const& rstack) noexcept
	{
		// �����t���O�̗��}�`���������X�g�Ɋi�[����.
		SHAPE_LIST slist_del;	// �������X�g
		for (const auto t : slist) {
			// �}�`�̏����t���O���Ȃ�,
			// �܂��͐}�`�����ɖ߂�����X�^�b�N�Ɋ܂܂��,
			// �܂��͐}�`����蒼������X�^�b�N�Ɋ܂܂��,�@�����肷��.
			if (!t->is_deleted() ||
				event_ustack_contain_shape(ustack, t) ||
				event_ustack_contain_shape(rstack, t)) {
				continue;
			}
			// ��L�̂�����ł��Ȃ��}�`���������X�g�ɒǉ�����.
			slist_del.push_back(t);
		}
		// �������X�g�Ɋ܂܂��}�`�����X�g�����菜��, �������.
		auto it_beg = slist.begin();
		for (const auto s : slist_del) {
			const auto it = std::find(it_beg, slist.end(), s);
			it_beg = slist.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// �������X�g����������.
		slist_del.clear();
	}

	//------------------------------
	// ����X�^�b�N���}�`���܂ނ����肷��.
	// ustack	����X�^�b�N
	// s	�}�`
	// �߂�l	�܂ޏꍇ true.
	//------------------------------
	static bool event_ustack_contain_shape(UNDO_STACK const& ustack, Shape* const s) noexcept
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
		if (sender != scp_page_panel()) {
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
		if (sender == scp_page_panel()) {
			auto const& win = Window::Current().CoreWindow();
			auto const& curs = win.PointerCursor();
			if (curs.Type() != CURS_ARROW.Type()) {
				win.PointerCursor(CURS_ARROW);
			}
		}
	}

	//------------------------------
	// �F�����o����.
	//------------------------------
	void MainPage::event_eyedropper_detect(const Shape* s, const uint32_t anc)
	{
		if (anc == ANC_TYPE::ANC_PAGE) {
			m_eyedropper_color = m_main_page.m_page_color;
			m_eyedropper_filled = true;
			Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
		}
		else if (s != nullptr) {
			if (anc == ANC_TYPE::ANC_FILL) {
				if (typeid(*s) == typeid(ShapeImage)) {
					if (static_cast<const ShapeImage*>(s)->get_pixcel(
						m_event_pos_pressed, m_eyedropper_color)) {
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
			else if (anc == ANC_TYPE::ANC_TEXT) {
				s->get_font_color(m_eyedropper_color);
				m_main_page.m_font_color = m_eyedropper_color;
				m_eyedropper_filled = true;
				Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
			}
			else if (anc == ANC_TYPE::ANC_STROKE) {
				s->get_stroke_color(m_eyedropper_color);
				m_main_page.m_stroke_color = m_eyedropper_color;
				m_eyedropper_filled = true;
				Window::Current().CoreWindow().PointerCursor(CURS_EYEDROPPER2);
			}
		}
	}

	//------------------------------
	// �}�`�̍쐬���I������.
	// b_pos	�͂ޗ̈�̊J�n�ʒu
	// b_vec	�I���ʒu�ւ̍���
	//------------------------------
	void MainPage::event_finish_creating(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec)
	{
		const auto d_tool = m_drawing_tool;
		Shape* s;
		if (d_tool == DRAWING_TOOL::RECT) {
			s = new ShapeRect(b_pos, b_vec, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::RRECT) {
			s = new ShapeRRect(b_pos, b_vec, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::POLY) {
			const auto poly_opt = m_drawing_poly_opt;
			s = new ShapePolygon(b_pos, b_vec, &m_main_page, poly_opt);
		}
		else if (d_tool == DRAWING_TOOL::ELLI) {
			s = new ShapeEllipse(b_pos, b_vec, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::LINE) {
			s = new ShapeLine(b_pos, b_vec, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::BEZIER) {
			s = new ShapeBezier(b_pos, b_vec, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::RULER) {
			s = new ShapeRuler(b_pos, b_vec, &m_main_page);
		}
		else if (d_tool == DRAWING_TOOL::QELLIPSE) {
			s = new ShapeQEllipse(b_pos, b_vec, 0.0f, &m_main_page);
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
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_append(s);
			summary_select(s);
		}
		status_bar_set_pos();
	}

	//------------------------------
	// ������}�`�̍쐬���I������.
	//------------------------------
	IAsyncAction MainPage::event_finish_creating_text_async(
		const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec)
	{
		const auto fit_text = m_text_fit_frame_to_text;
		tx_edit_text().Text(L"");
		ck_text_fit_frame_to_text().IsChecked(fit_text);
		if (co_await cd_edit_text_dialog().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit_text().Text().c_str());
			auto s = new ShapeText(b_pos, b_vec, text, &m_main_page);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			if (fit_text) {
				s->fit_frame_to_text(
					m_main_page.m_grid_snap ? m_main_page.m_grid_base + 1.0f : 0.0f);
			}
			m_text_fit_frame_to_text = ck_text_fit_frame_to_text().IsChecked().GetBoolean();
			event_slist_reduce(m_main_page.m_shape_list, m_ustack_undo, m_ustack_redo);
			ustack_push_append(s);
			ustack_push_select(s);
			ustack_push_null();
			m_event_shape_prev = s;
			xcvd_is_enabled();
			page_bbox_update(s);
			page_panel_size();
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_append(s);
				summary_select(s);
			}
		}
		// �|�C���^�[�̉����ꂽ��Ԃ�������Ԃɖ߂�.
		m_event_state = EVENT_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_anc_pressed = ANC_TYPE::ANC_PAGE;
		page_draw();
		status_bar_set_pos();
	}

	//------------------------------
	// �����ꂽ�}�`�̕ό`���I������.
	//------------------------------
	void MainPage::event_finish_forming(void)
	{
		const auto g_snap = m_main_page.m_grid_snap;
		if (g_snap && m_vert_stick >= FLT_MIN) {
			// ���݂̈ʒu��, ��������̑傫���Ɋۂ߂��ʒu�ƊԂ̋��������߂�.
			const auto s_scale = m_main_page.m_page_scale;
			D2D1_POINT_2F g_pos;
			D2D1_POINT_2F g_vec;
			pt_round(m_event_pos_curr, m_main_page.m_grid_base + 1.0, g_pos);
			pt_sub(g_pos, m_event_pos_curr, g_vec);
			float g_len = min(static_cast<float>(sqrt(pt_abs2(g_vec))), m_vert_stick) / s_scale;
			if (slist_find_vertex_closest(
				m_main_page.m_shape_list, m_event_pos_curr, g_len, g_pos)) {
				// ����Ƃ̋������߂����_�����������Ȃ�, ���̋����ɓ���ւ���.
				pt_sub(g_pos, m_event_pos_curr, g_vec);
				g_len = static_cast<float>(sqrt(pt_abs2(g_vec))) / s_scale;
			}
			// �ߖT�̒��_�ɂ���Ĉʒu���ς��Ȃ����������肷��.
			if (!m_event_shape_pressed->set_pos_anc(
				m_event_pos_curr, m_event_anc_pressed, g_len, m_image_keep_aspect)) {
				// �ς��Ȃ������Ȃ��, ����ɍ��킹��.
				m_event_shape_pressed->set_pos_anc(
					g_pos, m_event_anc_pressed, 0.0f, m_image_keep_aspect);
			}
		}
		else if (g_snap) {
			pt_round(m_event_pos_curr, m_main_page.m_grid_base + 1.0, m_event_pos_curr);
			m_event_shape_pressed->set_pos_anc(
				m_event_pos_curr, m_event_anc_pressed, 0.0f, m_image_keep_aspect);
		}
		else if (m_vert_stick >= FLT_MIN) {
			slist_find_vertex_closest(
				m_main_page.m_shape_list, m_event_pos_curr,
				m_vert_stick / m_main_page.m_page_scale, m_event_pos_curr);
			m_event_shape_pressed->set_pos_anc(m_event_pos_curr, m_event_anc_pressed,
				m_vert_stick / m_main_page.m_page_scale, m_image_keep_aspect);
		}
		if (!ustack_pop_if_invalid()) {
			ustack_push_null();
			page_bbox_update();
			page_panel_size();
			xcvd_is_enabled();
		}
	}

	//------------------------------
	// �I�����ꂽ�}�`�̈ړ����I������.
	//------------------------------
	void MainPage::event_finish_moving(void)
	{
		const auto g_snap = m_main_page.m_grid_snap;
		const auto v_stick = m_vert_stick;
		const auto g_base = m_main_page.m_grid_base;
		const auto p_scale = m_main_page.m_page_scale;

		// ����ɂ�������, �����_�ɂ�������.
		if (g_snap && v_stick >= FLT_MIN) {
			D2D1_POINT_2F vec{};	// ����
			if (event_get_vector_nearby_grid(m_main_page.m_shape_list, g_base + 1.0f, vec)) {
				D2D1_POINT_2F v_vec{};	// ���_�ւ̍���
				if (event_get_vector_nearby_vertex(
					m_main_page.m_shape_list, v_stick / p_scale, v_vec) && 
					pt_abs2(v_vec) < pt_abs2(vec)) {
					// ����ƒ��_�̂ǂ��炩�Z�����̋�����, �����ɓ���.
					vec = v_vec;
				}
				// ����ꂽ�����̕�����, �I�����ꂽ�}�`���ړ�����.
				slist_move(m_main_page.m_shape_list, vec);
			}
		}
		// ����ɂ�������
		else if (g_snap) {
			D2D1_POINT_2F vec{};	// ����
			if (event_get_vector_nearby_grid(m_main_page.m_shape_list, g_base + 1.0f, vec)) {
				slist_move(m_main_page.m_shape_list, vec);
			}
		} 
		// ���_�ɂ�������
		else if (v_stick >= FLT_MIN) {
			D2D1_POINT_2F vec{};	// ���_�Ƃ̍���
			if (event_get_vector_nearby_vertex(
				m_main_page.m_shape_list, v_stick / p_scale, vec)) {
				slist_move(m_main_page.m_shape_list, vec);
			}
		}
		if (!ustack_pop_if_invalid()) {
			ustack_push_null();
			page_bbox_update();
			page_panel_size();
			xcvd_is_enabled();
		}
	}

	//------------------------------
	// �͈͑I�����I������.
	// k_mod	�C���L�[
	//------------------------------
	void MainPage::event_finish_selecting_area(const VirtualKeyModifiers k_mod)
	{
		// �C���L�[���R���g���[�������肷��.
		if (k_mod == VirtualKeyModifiers::Control) {
			D2D1_POINT_2F area_lt{};
			D2D1_POINT_2F area_rb{};
			if (m_event_pos_pressed.x < m_event_pos_curr.x) {
				area_lt.x = m_event_pos_pressed.x;
				area_rb.x = m_event_pos_curr.x;
			}
			else {
				area_lt.x = m_event_pos_curr.x;
				area_rb.x = m_event_pos_pressed.x;
			}
			if (m_event_pos_pressed.y < m_event_pos_curr.y) {
				area_lt.y = m_event_pos_pressed.y;
				area_rb.y = m_event_pos_curr.y;
			}
			else {
				area_lt.y = m_event_pos_curr.y;
				area_rb.y = m_event_pos_pressed.y;
			}
			if (toggle_area(area_lt, area_rb)) {
				xcvd_is_enabled();
			}
		}
		// �C���L�[��������ĂȂ������肷��.
		else if (k_mod == VirtualKeyModifiers::None) {
			D2D1_POINT_2F area_lt{};
			D2D1_POINT_2F area_rb{};
			if (m_event_pos_pressed.x < m_event_pos_curr.x) {
				area_lt.x = m_event_pos_pressed.x;
				area_rb.x = m_event_pos_curr.x;
			}
			else {
				area_lt.x = m_event_pos_curr.x;
				area_rb.x = m_event_pos_pressed.x;
			}
			if (m_event_pos_pressed.y < m_event_pos_curr.y) {
				area_lt.y = m_event_pos_pressed.y;
				area_rb.y = m_event_pos_curr.y;
			}
			else {
				area_lt.y = m_event_pos_curr.y;
				area_rb.y = m_event_pos_pressed.y;
			}
			if (select_area(area_lt, area_rb)) {
				xcvd_is_enabled();
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
		if (sender != scp_page_panel()) {
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
			D2D1_POINT_2F vec;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, vec);
			if (pt_abs2(vec) * m_main_page.m_page_scale > m_event_click_dist * raw_dpi / log_dpi) {
				// ������Ԃɖ߂�.
				m_event_state = EVENT_STATE::BEGIN;
				event_set_cursor();
			}
		}
		// ��Ԃ�, �͈͂�I�����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_AREA) {
			page_draw();
		}
		// ��Ԃ�, �}�`���ړ����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			// �|�C���^�[�̌��݈ʒu�ƑO��ʒu�̍����𓾂�.
			D2D1_POINT_2F vec;
			pt_sub(m_event_pos_curr, m_event_pos_prev, vec);
			slist_move(m_main_page.m_shape_list, vec);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_event_pos_prev = m_event_pos_curr;
			page_draw();
		}
		// ��Ԃ�, �}�`��ό`���Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_DEFORM) {
			// �|�C���^�[�̌��݈ʒu��, �|�C���^�[�������ꂽ�}�`��, ���ʂ̈ʒu�Ɋi�[����.
			m_event_shape_pressed->set_pos_anc(
				m_event_pos_curr, m_event_anc_pressed, 0.0f, m_image_keep_aspect);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_event_pos_prev = m_event_pos_curr;
			page_draw();
		}
		// ��Ԃ�, ���{�^���������Ă�����, �܂���
		// �N���b�N��ɍ��{�^������������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_LBTN || 
			m_event_state == EVENT_STATE::CLICK_LBTN) {
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�Ƃ̍����𓾂�.
			D2D1_POINT_2F vec;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, vec);
			// �������N���b�N�̔��苗���𒴂��邩���肷��.
			if (pt_abs2(vec) > m_event_click_dist / m_main_page.m_page_scale) {
				// ��}�c�[�����I���c�[���ȊO�����肷��.
				if (m_drawing_tool != DRAWING_TOOL::SELECT) {
					// �͈͂�I�����Ă����ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::PRESS_AREA;
				}
				// �����ꂽ�}�`���k�������肷��.
				else if (m_event_shape_pressed == nullptr) {
					// �͈͂�I�����Ă����ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::PRESS_AREA;
					// �\���J�[�\�����J�[�\���ɐݒ肷��.
					Window::Current().CoreWindow().PointerCursor(CURS_CROSS);
				}
				// �I�����ꂽ�}�`�̐��� 1 �𒴂���,
				// �܂��͉����ꂽ�}�`�̕��ʂ����g, ����, �����񂩂𔻒肷��.
				else if (m_list_sel_cnt > 1 ||
					m_event_anc_pressed == ANC_TYPE::ANC_STROKE ||
					m_event_anc_pressed == ANC_TYPE::ANC_FILL ||
					m_event_anc_pressed == ANC_TYPE::ANC_TEXT) {
					// �}�`���ړ����Ă����ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::PRESS_MOVE;
					// �|�C���^�[�̌��݈ʒu��O��ʒu�ɕۑ�����.
					m_event_pos_prev = m_event_pos_curr;
					ustack_push_move(vec);
				}
				// �|�C���^�[�������ꂽ�̂��}�`�̊O���ȊO�����肷��.
				else if (m_event_anc_pressed != ANC_TYPE::ANC_PAGE) {
					// �}�`��ό`���Ă����ԂɑJ�ڂ���.
					// �|�C���^�[�̌��݈ʒu��O��ʒu�ɕۑ�����.
					m_event_state = EVENT_STATE::PRESS_DEFORM;
					m_event_pos_prev = m_event_pos_curr;
					ustack_push_position(m_event_shape_pressed, m_event_anc_pressed);
					m_event_shape_pressed->set_pos_anc(
						m_event_pos_curr, m_event_anc_pressed, 0.0f, m_image_keep_aspect);
				}
				page_draw();
			}
		}
	}

	//------------------------------
	// �|�b�v�A�b�v���j���[��\������.
	//------------------------------
	void MainPage::event_show_popup(void)
	{
		Shape* shape_pressed;
		const uint32_t anc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_curr,
			shape_pressed);

		MenuFlyout popup{};
		// �����ꂽ�}�`���k��, �܂��͉����ꂽ�}�`�̕��ʂ��O�������肷��.
		if (shape_pressed == nullptr ||
			anc_pressed == ANC_TYPE::ANC_PAGE) {
			for (const auto item : mbi_grid().Items()) {
				popup.Items().Append(item);
			}
		}
		else {
			// �u�ҏW�v���j���[���R���e�L�X�g���j���[�ɒǉ�
			xcvd_is_enabled();
			for (const auto item : mbi_edit().Items()) {
				if (item.IsEnabled()) {
					popup.Items().Append(item);
				}
			}
			// �����ꂽ�}�`���O���[�v�ȊO�����肷��.
			if (typeid(*shape_pressed) != typeid(ShapeGroup)) {
				if (popup.Items().Size() > 0) {
					// �Z�p���[�^�[��ǉ�����.
					popup.Items().Append(MenuFlyoutSeparator{});
				}
				// �����ꂽ�}�`����K�����肷��.
				if (typeid(*shape_pressed) == typeid(ShapeRuler)) {
					popup.Items().Append(mfi_stroke_color());
					popup.Items().Append(mfi_fill_color());
					popup.Items().Append(MenuFlyoutSeparator());
					popup.Items().Append(mfi_font_family());
					popup.Items().Append(mfi_font_size());
				}
				// �����ꂽ�}�`���摜�����肷��.
				else if (typeid(*shape_pressed) == typeid(ShapeImage)) {
					for (const auto item : mbi_image().Items()) {
						popup.Items().Append(item);
					}
				}
				// �����ꂽ�}�`�̕��ʂ������񂩔��肷��.
				else if (anc_pressed == ANC_TYPE::ANC_TEXT) {
					for (const auto item : mbi_font().Items()) {
						popup.Items().Append(item);
					}
				}
				// ��L�ȊO�̏ꍇ,
				else {
					for (const auto item : mbi_properties().Items()) {
						popup.Items().Append(item);
					}
				}
			}
			// �����ꂽ�}�`�̑����l��\���Ɋi�[����.
			m_main_page.set_attr_to(shape_pressed);
			page_setting_is_checked();
		}
		popup.Closed([=](IInspectable const&, IInspectable const&) {
			ContextFlyout(nullptr);
			});
		ContextFlyout(popup);
	}

	//------------------------------
	// �|�C���^�[�̃{�^���������ꂽ.
	// �L���v�`���̗L���ɂ�����炸, �Е��̃}�E�X�{�^������������Ԃ�, ��������̃{�^���������Ă�, ����͒ʒm����Ȃ�.
	//------------------------------
	void MainPage::event_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
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
		const SwapChainPanel& swap_chain_panel = sender.as<SwapChainPanel>();
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
			m_event_anc_pressed = slist_hit_test(m_main_page.m_shape_list, m_event_pos_pressed,
				m_event_shape_pressed);
			// �����ꂽ�̂��}�`�̊O�������肷��.
			if (m_event_anc_pressed == ANC_TYPE::ANC_PAGE) {
				m_event_shape_pressed = nullptr;
				m_event_shape_prev = nullptr;
				// �C���L�[��������Ă��Ȃ��Ȃ��, ���ׂĂ̐}�`�̑I������������.
				// �������ꂽ�}�`�����邩���肷��.
				if (args.KeyModifiers() == VirtualKeyModifiers::None && unselect_all()) {
					xcvd_is_enabled();
					page_draw();
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
//#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
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
				if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
					event_eyedropper_detect(m_event_shape_pressed, m_event_anc_pressed);
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
			if (t_stamp - m_event_time_pressed <= c_time &&
				m_event_shape_pressed != nullptr) {
				if (typeid(*m_event_shape_pressed) == typeid(ShapeText)) {
					edit_text_click_async(nullptr, nullptr);
				}
				else if (typeid(*m_event_shape_pressed) == typeid(ShapeQEllipse)) {
					rotation_click_async(m_event_shape_pressed);
				}
			}
		}
		// ��Ԃ�, �}�`���ړ����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			event_finish_moving();
		}
		// ��Ԃ�, �}�`��ό`���Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_DEFORM) {
			event_finish_forming();
		}
		// ��Ԃ�, �͈͂�I�����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_AREA) {
			// ��}�c�[�����I���c�[�������肷��.
			if (m_drawing_tool == DRAWING_TOOL::SELECT) {
				event_finish_selecting_area(args.KeyModifiers());
			}
			// ��}�c�[�����I���c�[���ȊO.
			else {
				unselect_all();
				// ���_����������臒l���[�����傫�������肷��.
				if (m_vert_stick >= FLT_MIN) {
					const bool boxed = (
						m_drawing_tool == DRAWING_TOOL::ELLI ||
						m_drawing_tool == DRAWING_TOOL::POLY ||
						m_drawing_tool == DRAWING_TOOL::RECT ||
						m_drawing_tool == DRAWING_TOOL::RRECT ||
						m_drawing_tool == DRAWING_TOOL::RULER ||
						m_drawing_tool == DRAWING_TOOL::TEXT
					);
					const float v_stick = m_vert_stick / m_main_page.m_page_scale;
					const double g_len = max(m_main_page.m_grid_base, 0.0) + 1.0;
					event_get_pos_snap_to_vertex(m_main_page.m_shape_list, boxed, v_stick,
						m_main_page.m_grid_snap, g_len, m_event_pos_pressed, m_event_pos_curr);
				}
				// ����ɍ��킹�邩���肷��.
				else if (m_main_page.m_grid_snap) {
					// �����ꂽ�ʒu�旣���ꂽ�ʒu�����̑傫���Ŋۂ߂�.
					const double g_len = max(m_main_page.m_grid_base + 1.0, 1.0);
					pt_round(m_event_pos_pressed, g_len, m_event_pos_pressed);
					pt_round(m_event_pos_curr, g_len, m_event_pos_curr);
				}

				// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̍��������߂�.
				D2D1_POINT_2F c_vec;	// ���݈ʒu�ւ̍���
				pt_sub(m_event_pos_curr, m_event_pos_pressed, c_vec);
				// ������ x �l�܂��� y �l�̂����ꂩ�� 1 �ȏォ���肷��.
				if (fabs(c_vec.x) >= 1.0f || fabs(c_vec.y) >= 1.0f) {
					if (m_drawing_tool == DRAWING_TOOL::TEXT) {
						event_finish_creating_text_async(m_event_pos_pressed, c_vec);
						return;
					}
					event_finish_creating(m_event_pos_pressed, c_vec);
				}
			}
		}
		// ��Ԃ�, �E�{�^������������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
			if (m_drawing_tool == DRAWING_TOOL::EYEDROPPER) {
				Shape* s;
				const uint32_t anc = slist_hit_test(
					m_main_page.m_shape_list, m_event_pos_pressed, s);
				if (anc == ANC_TYPE::ANC_PAGE) {
					ustack_push_set<UNDO_ID::PAGE_COLOR>(&m_main_page, m_eyedropper_color);
					ustack_push_null();
					xcvd_is_enabled();
				}
				else if (s != nullptr) {
					if (m_event_anc_pressed == ANC_TYPE::ANC_FILL) {
						ustack_push_set<UNDO_ID::FILL_COLOR>(s, m_eyedropper_color);
						ustack_push_null();
						xcvd_is_enabled();
					}
					else if (m_event_anc_pressed == ANC_TYPE::ANC_TEXT) {
						ustack_push_set<UNDO_ID::FONT_COLOR>(s, m_eyedropper_color);
						ustack_push_null();
						xcvd_is_enabled();
					}
					else if (m_event_anc_pressed == ANC_TYPE::ANC_STROKE) {
						ustack_push_set<UNDO_ID::STROKE_COLOR>(s, m_eyedropper_color);
						ustack_push_null();
						xcvd_is_enabled();
					}
				}
			}
			else {
				// �R���e�L�X�g���j���[���������.
				if (ContextFlyout() != nullptr) {
					ContextFlyout(nullptr);
				}
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
		m_event_anc_pressed = ANC_TYPE::ANC_PAGE;
		page_draw();
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
		else if (m_drawing_tool != DRAWING_TOOL::SELECT &&
			m_event_state != EVENT_STATE::PRESS_RBTN) {
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
			const auto anc = slist_hit_test(m_main_page.m_shape_list, m_event_pos_curr, s);
			if (anc == ANC_TYPE::ANC_PAGE) {
				Window::Current().CoreWindow().PointerCursor(CURS_ARROW);
			}
			else if (m_list_sel_cnt > 1) {
				Window::Current().CoreWindow().PointerCursor(CURS_SIZE_ALL);
			}
			else {
				switch (anc) {
				case ANC_TYPE::ANC_CENTER:
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
					// �}�`�̃N���X��, ���p�`�܂��͋Ȑ��ł��邩���肷��.
					if (s != nullptr) {
						if (typeid(*s) == typeid(ShapeLine) ||
							typeid(*s) == typeid(ShapePolygon) ||
							typeid(*s) == typeid(ShapeBezier) ||
							typeid(*s) == typeid(ShapeQEllipse)
							) {
							// �}�`�̕��ʂ�, ���_�̐��𒴂��Ȃ������肷��.
							if (anc >= ANC_TYPE::ANC_P0 && anc < ANC_TYPE::ANC_P0 +
								static_cast<ShapePath*>(s)->m_vec.size() + 1) {
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
		const auto p_scale = m_main_page.m_page_scale;
		// ���E�{�b�N�X�̍���ʒu�ɃX�N���[���̒l������, ���E�{�b�N�X�̕\������Ă��鍶��ʒu�𓾂�
		D2D1_POINT_2F bbox_lt;
		pt_add(m_main_bbox_lt, sb_horz().Value(), sb_vert().Value(), bbox_lt);
		// �����Ƃ��ēn���ꂽ�ʒu���X���b�v�`�F�[���p�l����̈ʒu�Ƃ��ē���.
		// ����ꂽ�ʒu�Ɋg�嗦�̋t�����悶��, ���E�{�b�N�X�̕\������Ă��鍶��ʒu���������l��,
		// �|�C���^�[�̌��݈ʒu�Ɋi�[����.
		pt_mul_add(args.GetCurrentPoint(scp_page_panel()).Position(), 1.0 / p_scale, bbox_lt, m_event_pos_curr);
	}

	//------------------------------
	// �|�C���^�[�̃z�C�[���{�^�������삳�ꂽ.
	//------------------------------
	void MainPage::event_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
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
			const int32_t w_delta = args.GetCurrentPoint(scp_page_panel()).Properties().MouseWheelDelta();
			page_zoom_delta(w_delta);
		}
		// �V�t�g�L�[��������Ă邩���肷��.
		else if (mod == VirtualKeyModifiers::Shift) {
			// ���X�N���[��.
			const int32_t w_delta = args.GetCurrentPoint(scp_page_panel()).Properties().MouseWheelDelta();
			const auto p_scale = m_main_page.m_page_scale;
			if (event_scroll_by_wheel_delta(sb_horz(), w_delta, p_scale)) {
				page_draw();
				status_bar_set_pos();
			}
		}
		// ����������ĂȂ������肷��.
		else if (mod == VirtualKeyModifiers::None) {
			// �c�X�N���[��.
			const int32_t w_delta = args.GetCurrentPoint(scp_page_panel()).Properties().MouseWheelDelta();
			const auto p_scale = m_main_page.m_page_scale;
			if (event_scroll_by_wheel_delta(sb_vert(), w_delta, p_scale)) {
				page_draw();
				status_bar_set_pos();
			}
		}
	}

}