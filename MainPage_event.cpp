//-------------------------------
// MainPage_event.cpp
// �|�C���^�[�̃C�x���g�n���h���[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Devices::Input::PointerDeviceType;
	using winrt::Windows::System::VirtualKeyModifiers;
	using winrt::Windows::UI::Core::CoreCursorType;
	using winrt::Windows::UI::Input::PointerPointProperties;
	using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;
	using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;
	using winrt::Windows::UI::Xaml::Controls::SwapChainPanel;
	using winrt::Windows::UI::Xaml::UIElement;
	using winrt::Windows::UI::Xaml::Window;

	static auto const& CC_ARROW = CoreCursor(CoreCursorType::Arrow, 0);	// ���J�[�\��
	static auto const& CC_CROSS = CoreCursor(CoreCursorType::Cross, 0);	// �\���J�[�\��
	static auto const& CC_SIZE_ALL = CoreCursor(CoreCursorType::SizeAll, 0);	// �ړ��J�[�\��
	static auto const& CC_SIZE_NESW = CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);	// �E�㍶���J�[�\��
	static auto const& CC_SIZE_NS = CoreCursor(CoreCursorType::SizeNorthSouth, 0);	// �㉺�J�[�\��
	static auto const& CC_SIZE_NWSE = CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);	// ����E���J�[�\��
	static auto const& CC_SIZE_WE = CoreCursor(CoreCursorType::SizeWestEast, 0);	// ���E�J�[�\��

	// �}�`������X�^�b�N�Ɋ܂܂�邩���肷��.
	static bool event_if_shape_on_stack(UNDO_STACK const& u_stack, Shape* const s) noexcept;
	// �����t���O�̗��}�`�����X�g����폜����.
	static void event_reduce_slist(SHAPE_LIST& slist, UNDO_STACK const& u_stack, UNDO_STACK const& r_stack) noexcept;
	// �}�E�X�z�C�[���̒l�ŃX�N���[������.
	static bool event_scroll_by_wheel_delta(const ScrollBar& scroll_bar, const int32_t delta, const float scale);
	// �I�����ꂽ�}�`�̒��_�ɍł��߂����������, ���_�ƕ���Ƃ̍��������߂�.
	static bool event_get_vec_nearby_grid(const SHAPE_LIST& slist, const float g_len, D2D1_POINT_2F& g_vec) noexcept;
	// ��I���̐}�`�̒��_�̒�����, �I�����ꂽ�}�`�̒��_�ɍł��߂����_������, �Q�_�Ԃ̍��������߂�.
	static bool event_get_vec_nearby_vert(const SHAPE_LIST& slist, const float d_limit, D2D1_POINT_2F& v_vec) noexcept;

	// ����X�^�b�N���}�`���Q�Ƃ��邩���肷��.
	// u_stack	����X�^�b�N
	// s	�}�`
	// �߂�l	�Q�Ƃ���ꍇ true.
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

	// �����t���O�̗��}�`�����X�g����폜����.
	// slist	�}�`���X�g
	// u_stack	���ɖ߂�����X�^�b�N
	// r_stack	��蒼������X�^�b�N
	static void event_reduce_slist(SHAPE_LIST& slist, UNDO_STACK const& u_stack, UNDO_STACK const& r_stack) noexcept
	{
		// �����t���O�̗��}�`���������X�g�Ɋi�[����.
		SHAPE_LIST list_del;
		for (const auto t : slist) {
			// �}�`�̏����t���O���Ȃ�,
			// �܂��͐}�`�����ɖ߂�����X�^�b�N�Ɋ܂܂��,
			// �܂��͐}�`����蒼������X�^�b�N�Ɋ܂܂��,�@�����肷��.
			if (!t->is_deleted() ||
				event_if_shape_on_stack(u_stack, t) ||
				event_if_shape_on_stack(r_stack, t)) {
				continue;
			}
			// ��L�̂�����ł��Ȃ��}�`���������X�g�ɒǉ�����.
			list_del.push_back(t);
		}
		// �������X�g�Ɋ܂܂��}�`�����X�g�����菜��, �������.
		auto it_beg = slist.begin();
		for (const auto s : list_del) {
			const auto it = std::find(it_beg, slist.end(), s);
			it_beg = slist.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// �������X�g����������.
		list_del.clear();
	}

	// �}�E�X�z�C�[���̒l�ŃX�N���[������.
	static bool event_scroll_by_wheel_delta(const ScrollBar& scroll_bar, const int32_t delta, const float scale)
	{
		constexpr double DELTA = 32.0;
		double value = scroll_bar.Value();
		double limit = 0.0;
		if (delta < 0 && value < (limit = scroll_bar.Maximum())) {
			value = min(value + DELTA / scale, limit);
		}
		else if (delta > 0 && value > (limit = scroll_bar.Minimum())) {
			value = max(value - DELTA / scale, limit);
		}
		else {
			return false;
		}
		scroll_bar.Value(value);
		return true;
	}

	// �I�����ꂽ�}�`�ɍł��߂����������, �}�`�ƕ���Ƃ̍��������߂�.
	// slist	�}�`���X�g
	// g_len	����̑傫��
	// g_vec	�}�`�ƕ���Ƃ̍���
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

	// ��I���̐}�`�̒��_�̒�����, �I�����ꂽ�}�`�̒��_�ɍł��߂����_������, �Q�_�Ԃ̍��������߂�.
	// s_list	�}�`���X�g
	// d_limit	�������� (����ȏ㗣�ꂽ���_�͑ΏۂƂ��Ȃ�)
	// v_vec	�ł��߂����_�Ԃ̍���
	// �߂�l	���������Ȃ� true
	static bool event_get_vec_nearby_vert(const SHAPE_LIST& slist, const float d_limit, D2D1_POINT_2F& v_vec) noexcept
	{
		float dd = d_limit * d_limit;
		bool flag = false;
		D2D1_POINT_2F v_pos[MAX_N_GON];
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
						flag = true;
					}
				}
			}
		}
		if (flag) {
			pt_sub(n_pos, w_pos, v_vec);
		}
		return flag;
	}

	// �|�C���^�[�̃{�^�����グ��ꂽ.
	void MainPage::event_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		event_released(sender, args);
	}

	// �|�C���^�[���p���̃X���b�v�`�F�[���p�l���̒��ɓ�����.
	void MainPage::event_entered(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		if (sender != scp_sheet_panel()) {
			Window::Current().CoreWindow().PointerCursor(CC_ARROW);
		}
		else {
			event_get_position(args);
			event_set_cursor();
			status_set_curs();
		}
	}

	// �|�C���^�[���p���̃X���b�v�`�F�[���p�l������o��.
	void MainPage::event_exited(IInspectable const& sender, PointerRoutedEventArgs const&)
	{
		if (sender == scp_sheet_panel()) {
			auto const& c_win = Window::Current().CoreWindow();
			auto const& cur = c_win.PointerCursor();
			if (cur.Type() != CC_ARROW.Type()) {
				c_win.PointerCursor(CC_ARROW);
			}
		}
	}

	// �}�`�̍쐬���I������.
	// b_pos	�͂ޗ̈�̊J�n�ʒu
	// b_vec	�I���ʒu�ւ̍���
	void MainPage::event_finish_creating(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec)
	{
		const auto t_draw = m_tool_draw;
		Shape* s;
		if (t_draw == DRAW_TOOL::RECT) {
			s = new ShapeRect(b_pos, b_vec, &m_sheet_main);
		}
		else if (t_draw == DRAW_TOOL::RRECT) {
			s = new ShapeRRect(b_pos, b_vec, &m_sheet_main);
		}
		else if (t_draw == DRAW_TOOL::POLY) {
			s = new ShapePoly(b_pos, b_vec, &m_sheet_main, m_tool_poly);
		}
		else if (t_draw == DRAW_TOOL::ELLI) {
			s = new ShapeElli(b_pos, b_vec, &m_sheet_main);
		}
		else if (t_draw == DRAW_TOOL::LINE) {
			s = new ShapeLineA(b_pos, b_vec, &m_sheet_main);
		}
		else if (t_draw == DRAW_TOOL::BEZI) {
			s = new ShapeBezi(b_pos, b_vec, &m_sheet_main);
		}
		else if (t_draw == DRAW_TOOL::RULER) {
			s = new ShapeRuler(b_pos, b_vec, &m_sheet_main);
		}
		else {
			return;
		}
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		event_reduce_slist(m_sheet_main.m_list_shapes, m_ustack_undo, m_ustack_redo);
		ustack_push_append(s);
		ustack_push_select(s);
		ustack_push_null();
		m_event_shape_prev = s;
		xcvd_is_enabled();
		sheet_update_bbox(s);
		sheet_panle_size();
		sheet_draw();
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_append(s);
			summary_select(s);
		}
	}

	// ������}�`�̍쐬���I������.
	IAsyncAction MainPage::event_finish_creating_text_async(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec)
	{
		tx_edit().Text(L"");
		ck_edit_text_frame().IsChecked(m_edit_text_frame);
		if (co_await cd_edit_text_dialog().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit().Text().c_str());
			auto s = new ShapeText(b_pos, b_vec, text, &m_sheet_main);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			if (m_edit_text_frame) {
				s->adjust_bbox(m_sheet_main.m_grid_snap ? m_sheet_main.m_grid_base + 1.0f : 0.0f);
			}
			m_edit_text_frame = ck_edit_text_frame().IsChecked().GetBoolean();
			event_reduce_slist(m_sheet_main.m_list_shapes, m_ustack_undo, m_ustack_redo);
			//unselect_all();
			ustack_push_append(s);
			ustack_push_select(s);
			ustack_push_null();
			m_event_shape_prev = s;
			xcvd_is_enabled();
			sheet_update_bbox(s);
			sheet_panle_size();
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_append(s);
				summary_select(s);
			}
		}
		// �|�C���^�[�̉����ꂽ��Ԃ�������Ԃɖ߂�.
		m_event_state = EVENT_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
		sheet_draw();
	}

	// �����ꂽ�}�`�̕ό`���I������.
	void MainPage::event_finish_forming(void)
	{
		const auto g_snap = m_sheet_main.m_grid_snap;
		if (g_snap && m_misc_vert_stick >= FLT_MIN) {
			// ���݂̈ʒu��, ��������̑傫���Ɋۂ߂��ʒu�ƊԂ̋��������߂�.
			const auto s_scale = m_sheet_main.m_sheet_scale;
			D2D1_POINT_2F g_pos;
			D2D1_POINT_2F g_vec;
			pt_round(m_event_pos_curr, m_sheet_main.m_grid_base + 1.0, g_pos);
			pt_sub(g_pos, m_event_pos_curr, g_vec);
			float g_len = min(static_cast<float>(sqrt(pt_abs2(g_vec))), m_misc_vert_stick) / s_scale;
			if (slist_find_vertex_closest(m_sheet_main.m_list_shapes, m_event_pos_curr, g_len, g_pos)) {
				// ����Ƃ̋������߂����_�����������Ȃ�, ���̋����ɓ���ւ���.
				pt_sub(g_pos, m_event_pos_curr, g_vec);
				g_len = static_cast<float>(sqrt(pt_abs2(g_vec))) / s_scale;
			}
			// �ߖT�̒��_�ɂ���Ĉʒu���ς��Ȃ����������肷��.
			if (!m_event_shape_pressed->set_pos_anch(m_event_pos_curr, m_event_anch_pressed, g_len, m_image_keep_aspect)) {
				m_event_shape_pressed->set_pos_anch(g_pos, m_event_anch_pressed, 0.0f, m_image_keep_aspect);
			}
		}
		else if (g_snap) {
			pt_round(m_event_pos_curr, m_sheet_main.m_grid_base + 1.0, m_event_pos_curr);
			m_event_shape_pressed->set_pos_anch(m_event_pos_curr, m_event_anch_pressed, 0.0f, m_image_keep_aspect);
		}
		else if (m_misc_vert_stick >= FLT_MIN) {
			slist_find_vertex_closest(m_sheet_main.m_list_shapes, m_event_pos_curr, m_misc_vert_stick / m_sheet_main.m_sheet_scale, m_event_pos_curr);
			m_event_shape_pressed->set_pos_anch(m_event_pos_curr, m_event_anch_pressed, m_misc_vert_stick / m_sheet_main.m_sheet_scale, m_image_keep_aspect);
		}
		if (!ustack_pop_if_invalid()) {
			ustack_push_null();
			sheet_update_bbox();
			sheet_panle_size();
		}
	}

	// �I�����ꂽ�}�`�̈ړ����I������.
	void MainPage::event_finish_moving(void)
	{
		// ����ɍ��킹��, �����_�ɍ��킹�邩���肷��.
		if (m_sheet_main.m_grid_snap && m_misc_vert_stick >= FLT_MIN) {
			D2D1_POINT_2F g_vec{};	// ����ւ̍���
			if (event_get_vec_nearby_grid(m_sheet_main.m_list_shapes, m_sheet_main.m_grid_base + 1.0f, g_vec)) {
				D2D1_POINT_2F v_vec{};	// ���_�ւ̍���
				if (event_get_vec_nearby_vert(m_sheet_main.m_list_shapes, m_misc_vert_stick / m_sheet_main.m_sheet_scale, v_vec) && pt_abs2(v_vec) < pt_abs2(g_vec)) {
					g_vec = v_vec;
				}
				slist_move(m_sheet_main.m_list_shapes, g_vec);
			}
		}
		// ����ɍ��킹��, �����_�ɍ��킹�Ȃ��𔻒肷��.
		else if (m_sheet_main.m_grid_snap) {
			D2D1_POINT_2F g_vec{};	// ����Ƃ̍���
			if (event_get_vec_nearby_grid(m_sheet_main.m_list_shapes, m_sheet_main.m_grid_base + 1.0f, g_vec)) {
				slist_move(m_sheet_main.m_list_shapes, g_vec);
			}
		} 
		// ����ɍ��킹�Ȃ�, �����_�ɍ��킹�邩���肷��.
		else if (m_misc_vert_stick >= FLT_MIN) {
			D2D1_POINT_2F v_vec{};	// ���_�Ƃ̍���
			if (event_get_vec_nearby_vert(m_sheet_main.m_list_shapes, m_misc_vert_stick / m_sheet_main.m_sheet_scale, v_vec)) {
				slist_move(m_sheet_main.m_list_shapes, v_vec);
			}
		}
		if (!ustack_pop_if_invalid()) {
			ustack_push_null();
			sheet_update_bbox();
			sheet_panle_size();
			xcvd_is_enabled();
		}
	}

	// �͈͑I�����I������.
	void MainPage::event_finish_selecting_area(const VirtualKeyModifiers k_mod)
	{
		// �C���L�[���R���g���[�������肷��.
		if (k_mod == VirtualKeyModifiers::Control) {
			D2D1_POINT_2F a_min;
			D2D1_POINT_2F a_max;
			pt_bound(m_event_pos_pressed, m_event_pos_curr, a_min, a_max);
			if (toggle_area(a_min, a_max)) {
				xcvd_is_enabled();
			}
		}
		// �C���L�[��������ĂȂ������肷��.
		else if (k_mod == VirtualKeyModifiers::None) {
			D2D1_POINT_2F a_min;
			D2D1_POINT_2F a_max;
			pt_bound(m_event_pos_pressed, m_event_pos_curr, a_min, a_max);
			if (select_area(a_min, a_max)) {
				xcvd_is_enabled();
			}
		}
		Window::Current().CoreWindow().PointerCursor(CC_ARROW);
	}

	// �|�C���^�[�̌��݈ʒu�𓾂�.
	void MainPage::event_get_position(PointerRoutedEventArgs const& args)
	{
		// �X���b�v�`�F�[���p�l����ł̃|�C���^�[�̈ʒu�𓾂�, 
		// �p�����W�n�ɕϊ���, �|�C���^�[�̌��݈ʒu�Ɋi�[����.
		D2D1_POINT_2F sheet_pos;
		pt_add(m_sheet_min, sb_horz().Value(), sb_vert().Value(), sheet_pos);
		pt_mul(args.GetCurrentPoint(scp_sheet_panel()).Position(), 1.0 / m_sheet_main.m_sheet_scale, sheet_pos, m_event_pos_curr);
	}

	// �|�C���^�[��������.
	void MainPage::event_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		event_get_position(args);
		status_set_curs();
		// �|�C���^�[�̉����ꂽ��Ԃ�, ������Ԃ����肷��.
		if (m_event_state == EVENT_STATE::BEGIN) {
			event_set_cursor();
		}
		// ��Ԃ�. �N���b�N������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::CLICK) {
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̒���������,
			// �������N���b�N���苗���𒴂��邩���肷��.
			D2D1_POINT_2F vec;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, vec);
			if (pt_abs2(vec) > m_event_click_dist) {
				// ������Ԃɖ߂�.
				m_event_state = EVENT_STATE::BEGIN;
				event_set_cursor();
			}
		}
		// ��Ԃ�, �͈͂�I�����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_AREA) {
			sheet_draw();
		}
		// ��Ԃ�, �}�`���ړ����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			// �|�C���^�[�̌��݈ʒu�ƑO��ʒu�̍����𓾂�.
			D2D1_POINT_2F d_vec;
			pt_sub(m_event_pos_curr, m_event_pos_prev, d_vec);
			slist_move(m_sheet_main.m_list_shapes, d_vec);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_event_pos_prev = m_event_pos_curr;
			sheet_draw();
		}
		// ��Ԃ�, �}�`��ό`���Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_FORM) {
			// �|�C���^�[�̌��݈ʒu��, �|�C���^�[�������ꂽ�}�`��, ���ʂ̈ʒu�Ɋi�[����.
			m_event_shape_pressed->set_pos_anch(m_event_pos_curr, m_event_anch_pressed, 0.0f, m_image_keep_aspect);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_event_pos_prev = m_event_pos_curr;
			sheet_draw();
		}
		// ��Ԃ�, ���{�^���������Ă�����, �܂��̓N���b�N��ɍ��{�^������������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_LBTN || m_event_state == EVENT_STATE::CLICK_LBTN) {
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�Ƃ̍����𓾂�.
			D2D1_POINT_2F vec;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, vec);
			// �����̒������N���b�N�̔��苗���𒴂��邩���肷��.
			if (pt_abs2(vec) > m_event_click_dist) {
				// ��}�c�[�����I���c�[���ȊO�����肷��.
				if (m_tool_draw != DRAW_TOOL::SELECT) {
					// �͈͂�I�����Ă����ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::PRESS_AREA;
				}
				// �����ꂽ�}�`���k�������肷��.
				else if (m_event_shape_pressed == nullptr) {
					// �͈͂�I�����Ă����ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::PRESS_AREA;
					// �\���J�[�\�����J�[�\���ɐݒ肷��.
					Window::Current().CoreWindow().PointerCursor(CC_CROSS);
				}
				// �I�����ꂽ�}�`�̐��� 1 �𒴂���,
				// �܂��͉����ꂽ�}�`�̕��ʂ����g, ����, �����񂩂𔻒肷��.
				else if (m_list_sel_cnt > 1 ||
					m_event_anch_pressed == ANCH_TYPE::ANCH_STROKE ||
					m_event_anch_pressed == ANCH_TYPE::ANCH_FILL ||
					m_event_anch_pressed == ANCH_TYPE::ANCH_TEXT) {
					// ��Ԃ�}�`���ړ����Ă����ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::PRESS_MOVE;
					// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
					m_event_pos_prev = m_event_pos_curr;
					ustack_push_move(vec);
				}
				// �|�C���^�[�������ꂽ�̂��}�`�̊O���ȊO�����肷��.
				else if (m_event_anch_pressed != ANCH_TYPE::ANCH_SHEET) {
					// �}�`��ό`���Ă����ԂɑJ�ڂ���.
					m_event_state = EVENT_STATE::PRESS_FORM;
					m_event_pos_prev = m_event_pos_curr;
					ustack_push_anch(m_event_shape_pressed, m_event_anch_pressed);
					m_event_shape_pressed->set_pos_anch(m_event_pos_curr, m_event_anch_pressed, 0.0f, m_image_keep_aspect);
				}
				sheet_draw();
			}
		}
	}

	// �|�C���^�[�̃{�^���������ꂽ.
	// �L���v�`���̗L���ɂ�����炸, �Е��̃}�E�X�{�^������������Ԃ�, ��������̃{�^���������Ă�, ����͒ʒm����Ȃ�.
	void MainPage::event_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		auto const& swap_chain_panel = sender.as<SwapChainPanel>();
		// �|�C���^�[�̃L���v�`�����n�߂�.
		swap_chain_panel.CapturePointer(args.Pointer());
		// �|�C���^�[�̃C�x���g�������Ԃ𓾂�.
		auto t_stamp = args.GetCurrentPoint(swap_chain_panel).Timestamp();
		event_get_position(args);
		// �|�C���^�[�̃v���p�e�B�[�𓾂�.
		auto const& p_prop = args.GetCurrentPoint(swap_chain_panel).Properties();
		// �|�C���^�[�̃f�o�C�X�^�C�v�𔻒肷��.
		switch (args.GetCurrentPoint(swap_chain_panel).PointerDevice().PointerDeviceType()) {
		// �f�o�C�X�^�C�v���}�E�X�̏ꍇ
		case PointerDeviceType::Mouse:
			// �v���p�e�B�[���E�{�^�����������肷��.
			if (p_prop.IsRightButtonPressed()) {
				m_event_state = EVENT_STATE::PRESS_RBTN;
			}
			// �v���p�e�B�[�����{�^�����������肷��.
			else if (p_prop.IsLeftButtonPressed()) {
				[[fallthrough]];
		// �f�o�C�X�^�C�v���y���܂��̓^�b�`�̏ꍇ
		case PointerDeviceType::Pen:
		case PointerDeviceType::Touch:
				// �|�C���^�[�̉����ꂽ��Ԃ𔻒肷��.
				switch (m_event_state) {
				// ��Ԃ��N���b�N������Ԃ̏ꍇ
				case EVENT_STATE::CLICK:
					if (t_stamp - m_event_time_pressed <= m_event_click_time) {
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
		// ��}�c�[�����I���c�[���ȊO�����肷��.
		if (m_tool_draw != DRAW_TOOL::SELECT) {
			return;
		}
		m_event_anch_pressed = slist_hit_test(m_sheet_main.m_list_shapes, m_event_pos_pressed, m_event_shape_pressed);
		// �����ꂽ�̂��}�`�̊O���ȊO�����肷��.
		if (m_event_anch_pressed != ANCH_TYPE::ANCH_SHEET) {
			// ��Ԃ����{�^���������ꂽ���, �܂���, �E�{�^����������Ă��Ă������ꂽ�}�`���I������ĂȂ������肷��.
			if (m_event_state == EVENT_STATE::PRESS_LBTN ||
				(m_event_state == EVENT_STATE::PRESS_RBTN && !m_event_shape_pressed->is_selected())) {
				select_shape(m_event_shape_pressed, args.KeyModifiers());
			}
			return;
		}
		m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
		m_event_shape_pressed = nullptr;
		m_event_shape_prev = nullptr;
		// �C���L�[��������Ă��Ȃ��Ȃ��, ���ׂĂ̐}�`�̑I����������, �������ꂽ�}�`�����邩���肷��.
		if (args.KeyModifiers() == VirtualKeyModifiers::None && unselect_all()) {
			xcvd_is_enabled();
			sheet_draw();
		}
	}

	// ���_�ɍ��킹��悤, �����ꂽ�ʒu�Ɨ����ꂽ�ʒu�𒲐�����.
	// slist	�}�`���X�g
	// box_type	
	static void event_released_snap_to_vertex(const SHAPE_LIST& slist, const bool box_type, const float d_limit, const bool g_snap, const double g_len, D2D1_POINT_2F& p_pos, D2D1_POINT_2F& r_pos)
	{
		// �l���̈ʒu�𓾂�.
		D2D1_POINT_2F b_pos[4]{
			p_pos, { r_pos.x, p_pos.y }, r_pos, { p_pos.x, r_pos.y },
		};
		// ����ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
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
		// ���^�Ȃ�, �E��ʒu�ɍł��߂����_�Ƃ��̋����𓾂�.
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
		double g_abs[2];	// ����Ƃ̋����̎���
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

	// �|�C���^�[�̃{�^�����グ��ꂽ.
	void MainPage::event_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif
		// �|�C���^�[�̒ǐՂ��~����.
		auto const& panel = sender.as<SwapChainPanel>();
		panel.ReleasePointerCaptures();
		event_get_position(args);
		// ��Ԃ�, ���{�^���������ꂽ��Ԃ����肷��.
		if (m_event_state == EVENT_STATE::PRESS_LBTN) {
			// �{�^�������ꂽ�����Ɖ����ꂽ�����̍���, �N���b�N�̔��莞�Ԉȉ������肷��.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			if (t_stamp - m_event_time_pressed <= m_event_click_time) {
				// �N���b�N������ԂɑJ�ڂ���.
				m_event_state = EVENT_STATE::CLICK;
				event_set_cursor();
				return;
			}
		}
		// ��Ԃ�, �N���b�N��ɍ��{�^������������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::CLICK_LBTN) {
			// �{�^���������ꂽ�����Ɖ����ꂽ�����̍����𓾂�.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			// �������N���b�N�̔��莞�Ԉȉ�, �������ꂽ�}�`��������}�`�����肷��.
			if (t_stamp - m_event_time_pressed <= m_event_click_time &&
				m_event_shape_pressed != nullptr && typeid(*m_event_shape_pressed) == typeid(ShapeText)) {
				edit_text_async(static_cast<ShapeText*>(m_event_shape_pressed));
			}
		}
		// ��Ԃ�, �}�`���ړ����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_MOVE) {
			event_finish_moving();
		}
		// ��Ԃ�, �}�`��ό`���Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_FORM) {
			event_finish_forming();
		}
		// ��Ԃ�, �͈͑I�����Ă����Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_AREA) {
			// ��}�c�[�����I���c�[�������肷��.
			if (m_tool_draw == DRAW_TOOL::SELECT) {
				event_finish_selecting_area(args.KeyModifiers());
			}
			else {
				// ��}�c�[�����I���c�[���ȊO�Ȃ�,
				// ����ɍ��킹�邩, ���V�t�g�L�[��������Ă��Ȃ������肷��.
				unselect_all();
				if (m_misc_vert_stick >= FLT_MIN) {
					const bool box_type = (
						m_tool_draw == DRAW_TOOL::ELLI ||
						m_tool_draw == DRAW_TOOL::POLY ||
						m_tool_draw == DRAW_TOOL::RECT ||
						m_tool_draw == DRAW_TOOL::RRECT ||
						m_tool_draw == DRAW_TOOL::RULER ||
						m_tool_draw == DRAW_TOOL::TEXT);
					//if (m_tool_draw == DRAW_TOOL::POLY) {
					//	D2D1_POINT_2F v_pos[MAX_N_GON];
					//	D2D1_POINT_2F v_vec;
					//	pt_sub(m_event_pos_curr, m_event_pos_pressed, v_vec);
					//	ShapePoly::create_poly_by_bbox(m_event_pos_pressed, v_vec, m_tool_poly, v_pos, v_vec);
					//	pt_add(m_event_pos_pressed, v_vec, m_event_pos_curr);
					//}
					const float d_lim = m_misc_vert_stick / m_sheet_main.m_sheet_scale;
					const double g_len = max(m_sheet_main.m_grid_base + 1.0, 1.0);
					event_released_snap_to_vertex(m_sheet_main.m_list_shapes, box_type, d_lim, m_sheet_main.m_grid_snap, g_len, m_event_pos_pressed, m_event_pos_curr);
				}
				else if (m_sheet_main.m_grid_snap) {
					// �����ꂽ�ʒu�旣���ꂽ�ʒu�����̑傫���Ŋۂ߂�.
					const double g_len = max(m_sheet_main.m_grid_base + 1.0, 1.0);
					pt_round(m_event_pos_pressed, g_len, m_event_pos_pressed);
					pt_round(m_event_pos_curr, g_len, m_event_pos_curr);
				}

				// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̍��������߂�.
				D2D1_POINT_2F c_vec;	// ���݈ʒu�ւ̍���
				pt_sub(m_event_pos_curr, m_event_pos_pressed, c_vec);
				if (fabs(c_vec.x) >= 1.0f || fabs(c_vec.y) >= 1.0f) {
					if (m_tool_draw == DRAW_TOOL::TEXT) {
						event_finish_creating_text_async(m_event_pos_pressed, c_vec);
						return;
					}
					event_finish_creating(m_event_pos_pressed, c_vec);
				}
			}
		}
		// ��Ԃ�, �E�{�^������������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::PRESS_RBTN) {
			event_show_context_menu();
		}
		// ��Ԃ�, ������Ԃ����肷��.
		else if (m_event_state == EVENT_STATE::BEGIN) {
			// �{���͏�����Ԃł��̃n���h���[���Ăяo�����͂��͂Ȃ���,
			// �R���e���c�_�C�A���O���I�������Ƃ��Ăяo����Ă��܂�.
			return;
		}
		// ������Ԃɖ߂�.
		m_event_state = EVENT_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
		sheet_draw();
	}

	// �󋵂ɉ������`��̃J�[�\����ݒ肷��.
	void MainPage::event_set_cursor(void)
	{
		// ��}�c�[�����I���c�[���ȊO�����肷��.
		if (m_tool_draw != DRAW_TOOL::SELECT) {
			Window::Current().CoreWindow().PointerCursor(CC_CROSS);
		}
		else if (!m_dx_mutex.try_lock()) {
			Window::Current().CoreWindow().PointerCursor(CC_ARROW);
		}
		else {
			Shape* s;
			const auto anch = slist_hit_test(m_sheet_main.m_list_shapes, m_event_pos_curr, s);
			m_dx_mutex.unlock();
			if (anch == ANCH_TYPE::ANCH_SHEET) {
				Window::Current().CoreWindow().PointerCursor(CC_ARROW);
			}
			else if (m_list_sel_cnt > 1) {
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
					// �}�`�̃N���X��, ���p�`�܂��͋Ȑ��ł��邩���肷��.
					if (s != nullptr &&
						(typeid(*s) == typeid(ShapeLineA) || typeid(*s) == typeid(ShapePoly) || typeid(*s) == typeid(ShapeBezi))) {
						// �}�`�̕��ʂ�, ���_�̐��𒴂��Ȃ������肷��.
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
	}

	// �R���e�L�X�g���j���[��\������.
	void MainPage::event_show_context_menu(void)
	{
		// �R���e�L�X�g���j���[���������.
		scp_sheet_panel().ContextFlyout(nullptr);
		// �����ꂽ�}�`���k��, �܂��͉����ꂽ�}�`�̕��ʂ��O�������肷��.
		if (m_event_shape_pressed == nullptr || m_event_anch_pressed == ANCH_TYPE::ANCH_SHEET) {
			if (m_menu_sheet != nullptr) {
				scp_sheet_panel().ContextFlyout(m_menu_sheet);
			}
		}
		// �����ꂽ�}�`���O���[�v�����肷��.
		else if (typeid(*m_event_shape_pressed) == typeid(ShapeGroup)) {
			if (m_menu_ungroup != nullptr) {
				scp_sheet_panel().ContextFlyout(m_menu_ungroup);
			}
		}
		// �����ꂽ�}�`����K�����肷��.
		else if (typeid(*m_event_shape_pressed) == typeid(ShapeRuler)) {
			if (m_menu_ruler != nullptr) {
				scp_sheet_panel().ContextFlyout(m_menu_ruler);
			}
		}
		// �����ꂽ�}�`����K�����肷��.
		else if (typeid(*m_event_shape_pressed) == typeid(ShapeImage)) {
			if (m_menu_image != nullptr) {
				scp_sheet_panel().ContextFlyout(m_menu_image);
			}
		}
		else {
			// �����ꂽ�}�`�̑����l��p���Ɋi�[����.
			m_sheet_main.set_attr_to(m_event_shape_pressed);
			sheet_attr_is_checked();
			// �����ꂽ�}�`�̕��ʂ����������肷��.
			if (m_event_anch_pressed == ANCH_TYPE::ANCH_FILL) {
				if (m_menu_fill != nullptr) {
					scp_sheet_panel().ContextFlyout(m_menu_fill);
				}
			}
			// �����ꂽ�}�`�̕��ʂ������񂩔��肷��.
			else if (m_event_anch_pressed == ANCH_TYPE::ANCH_TEXT) {
				if (m_menu_font != nullptr) {
					scp_sheet_panel().ContextFlyout(m_menu_font);
				}
			}
			// �����ꂽ�}�`�̕��ʂ����g�����肷��.
			else if (m_event_anch_pressed == ANCH_TYPE::ANCH_STROKE) {
				if (m_menu_stroke != nullptr) {
					scp_sheet_panel().ContextFlyout(m_menu_stroke);
				}
			}
		}
	}

	// �|�C���^�[�̃z�C�[���{�^�������삳�ꂽ.
	void MainPage::event_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// �R���g���[���L�[��������Ă邩���肷��.
		if (args.KeyModifiers() == VirtualKeyModifiers::Control) {
			// �g��k��
			const int32_t delta = args.GetCurrentPoint(scp_sheet_panel()).Properties().MouseWheelDelta();
			sheet_zoom_delta(delta);
		}
		// �V�t�g�L�[��������Ă邩���肷��.
		else if (args.KeyModifiers() == VirtualKeyModifiers::Shift) {
			// ���X�N���[��.
			const int32_t delta = args.GetCurrentPoint(scp_sheet_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_horz(), delta, m_sheet_main.m_sheet_scale)) {
				sheet_draw();
				status_set_curs();
			}
		}
		// ����������ĂȂ������肷��.
		else if (args.KeyModifiers() == VirtualKeyModifiers::None) {
			// �c�X�N���[��.
			const int32_t delta = args.GetCurrentPoint(scp_sheet_panel()).Properties().MouseWheelDelta();
			if (event_scroll_by_wheel_delta(sb_vert(), delta, m_sheet_main.m_sheet_scale)) {
				sheet_draw();
				status_set_curs();
			}
		}
	}

}