//-------------------------------
// MainPage_pointer.cpp
// �|�C���^�[�̃C�x���g�n���h���[
//-------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::UI::Core::CoreCursorType;

	static auto const& CUR_ARROW = CoreCursor(CoreCursorType::Arrow, 0);	// ���J�[�\��
	static auto const& CUR_CROSS = CoreCursor(CoreCursorType::Cross, 0);	// �\���J�[�\��
	static auto const& CUR_SIZEALL = CoreCursor(CoreCursorType::SizeAll, 0);	// �ړ��J�[�\��
	static auto const& CUR_SIZENESW = CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);	// �E�㍶���J�[�\��
	static auto const& CUR_SIZENS = CoreCursor(CoreCursorType::SizeNorthSouth, 0);	// �㉺�J�[�\��
	static auto const& CUR_SIZENWSE = CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);	// ����E���J�[�\��
	static auto const& CUR_SIZEWE = CoreCursor(CoreCursorType::SizeWestEast, 0);	// ���E�J�[�\��

	// �����t���O�̗��}�`�����X�g����폜����.
	static void reduce_list(S_LIST_T& s_list, U_STACK_T const& u_stack, U_STACK_T const& r_stack);
	// �}�`������X�^�b�N�Ɋ܂܂�邩���ׂ�.
	static bool refer_ro(U_STACK_T const& u_stack, Shape* const s) noexcept;

	// �����t���O�̗��}�`�����X�g����폜����.
	static void reduce_list(S_LIST_T& s_list, U_STACK_T const& u_stack, U_STACK_T const& r_stack)
	{
		// �����t���O�̗��}�`���������X�g�Ɋi�[����.
		S_LIST_T d_list;
		for (const auto t : s_list) {
			if (t->is_deleted() == false) {
				// �����t���O���Ȃ��}�`�͖�������.
				continue;
			}
			if (refer_ro(u_stack, t)) {
				// ���ɖ߂�����X�^�b�N���Q�Ƃ���}�`�͖�������.
				continue;
			}
			if (refer_ro(r_stack, t)) {
				// ��蒼������X�^�b�N���Q�Ƃ���}�`�͖�������.
				continue;
			}
			// ��L�̂�����ł��Ȃ��}�`���������X�g�ɒǉ�����.
			d_list.push_back(t);
		}
		// �������X�g�Ɋ܂܂��}�`�����X�g�����菜��, �������.
		auto it_begin = s_list.begin();
		for (const auto s : d_list) {
			auto it = std::find(it_begin, s_list.end(), s);
			it_begin = s_list.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// �������X�g����������.
		d_list.clear();

	}

	// ����X�^�b�N���}�`���Q�Ƃ��邩���ׂ�.
	// u_stack	����X�^�b�N
	// s	�}�`
	// �߂�l	�Q�Ƃ���ꍇ true.
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

	// �͈͑I�����I������.
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
			// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
			enable_edit_menu();
		}
		Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
	}

	// �}�`�̍쐬���I������.
	void MainPage::finish_create_shape(void)
	{
		if (m_page_panel.m_grid_snap) {
			// ����ɐ���̏ꍇ, �n�_�ƏI�_�����̑傫���Ŋۂ߂�
			double g = max(m_page_panel.m_grid_base + 1.0, 1.0);
			pt_round(m_press_pos, g, m_press_pos);
			pt_round(m_curr_pos, g, m_curr_pos);
		}
		// �|�C���^�[�̌��݂̈ʒu�Ɖ����ꂽ�ʒu�̍��������߂�.
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
						// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
						undo_push_null();
						// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
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
				// ��ʂɔ͈͂�\�������܂܂ɂ��邽�߂ɒ��f����.
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
			// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
			undo_push_null();
			m_press_shape_summary = m_press_shape_prev = s;
			// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
			enable_edit_menu();
			s->get_bound(m_page_min, m_page_max);
			set_page_panle_size();
			page_draw();
			if (m_summary_visible) {
				summary_append(s);
			}
		}
	}

	// �}�`�̈ړ����I������.
	void MainPage::finish_move_shape(void)
	{
		if (m_page_panel.m_grid_snap) {
			// �I�����ꂽ�}�`���͂ޕ��`�̍���_�𓾂�.
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
				// ��������_�����̑傫���Ŋۂ߂�.
				// �ۂ߂̑O��Ő��������𓾂�.
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
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		enable_edit_menu();
	}

	// �}�`�̕ό`���I������.
	void MainPage::finish_form_shape(void)
	{
		if (m_page_panel.m_grid_snap) {
			pt_round(m_curr_pos, m_page_panel.m_grid_base + 1.0, m_curr_pos);
		}
		m_press_shape->set_pos(m_curr_pos, m_press_anchor);
		if (undo_pop_if_invalid()) {
			return;
		}
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
	}

	// �|�C���^�[�̃{�^�����グ��ꂽ.
	void MainPage::scp_pointer_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		scp_pointer_released(sender, args);
	}

	// �|�C���^�[���y�[�W�̃p�l���̒��ɓ�����.
	void MainPage::scp_pointer_entered(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		if (sender != scp_page_panel()) {
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
			return;
		}
		// �X���b�v�`�F�[���p�l����ł̃|�C���^�[�̈ʒu�𓾂�, �y�[�W���W�ɕϊ���,
		// �|�C���^�[�̌��݈ʒu�Ɋi�[����.
		D2D1_POINT_2F p_offs;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), p_offs);
		const auto ui_pos{ args.GetCurrentPoint(scp_page_panel()).Position() };
		pt_scale(ui_pos, 1.0 / m_page_panel.m_page_scale, p_offs, m_curr_pos);
		set_pointer();
		// �|�C���^�[�̈ʒu���X�e�[�^�X�o�[�Ɋi�[����.
		status_set_curs();
	}

	// �|�C���^�[���y�[�W�̃p�l������o��.
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

	// �|�C���^�[��������.
	void MainPage::scp_pointer_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::UI::Xaml::Window;
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// �X���b�v�`�F�[���p�l����ł̃|�C���^�[�̈ʒu�𓾂�, �y�[�W���W�ɕϊ���,
		// �|�C���^�[�̌��݈ʒu�Ɋi�[����.
		D2D1_POINT_2F p_offs;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), p_offs);
		const auto ui_pos{ args.GetCurrentPoint(scp_page_panel()).Position() };
		pt_scale(ui_pos, 1.0 / m_page_panel.m_page_scale, p_offs, m_curr_pos);

		// �|�C���^�[�̈ʒu���X�e�[�^�X�o�[�Ɋi�[����.
		status_set_curs();
		if (m_press_state == STATE_TRAN::BEGIN) {
			// ��Ԃ�������Ԃ̏ꍇ,
			// �󋵂ɉ������`��̃J�[�\����ݒ肷��.
			set_pointer();
		}
		else if (m_press_state == STATE_TRAN::CLICK) {
			// ��Ԃ��N���b�N������Ԃ̏ꍇ,
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̒����𓾂�.
			D2D1_POINT_2F d_pos;
			pt_sub(m_curr_pos, m_press_pos, d_pos);
			if (pt_abs2(d_pos) > m_click_dist) {
				// ������臒l�𒴂���ꍇ, ������Ԃɖ߂�.
				m_press_state = STATE_TRAN::BEGIN;
				set_pointer();
			}
		}
		else if (m_press_state == STATE_TRAN::PRESS_AREA) {
			// ��Ԃ��͈͑I�����Ă����Ԃ̏ꍇ,
			// �y�[�W�Ɛ}�`��\������.
			page_draw();
		}
		else if (m_press_state == STATE_TRAN::PRESS_MOVE) {
			// ��Ԃ��}�`���ړ����Ă����Ԃ̏ꍇ,
			// �|�C���^�[�̌��݈ʒu�ƑO��ʒu�̍����𓾂�.
			// �I���t���O�̗����ׂĂ̐}�`�����������ړ�����.
			D2D1_POINT_2F d_pos;
			pt_sub(m_curr_pos, m_prev_pos, d_pos);
			s_list_move(m_list_shapes, d_pos);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_prev_pos = m_curr_pos;
			// �y�[�W�Ɛ}�`��\������.
			page_draw();
		}
		else if (m_press_state == STATE_TRAN::PRESS_FORM) {
			// ��Ԃ��}�`��ό`���Ă����Ԃ̏ꍇ,
			// �|�C���^�[�̌��݈ʒu�������ꂽ�}�`�̕��ʂ̈ʒu�Ɋi�[����.
			m_press_shape->set_pos(m_curr_pos, m_press_anchor);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_prev_pos = m_curr_pos;
			// �y�[�W�Ɛ}�`��\������.
			page_draw();
		}
		else if (m_press_state == STATE_TRAN::PRESS_L
			|| m_press_state == STATE_TRAN::CLICK_2) {
			// ��Ԃ����{�^���������Ă�����,
			// �܂��̓N���b�N��ɍ��{�^���������Ă����Ԃ̏ꍇ,
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̒����𓾂�.
			D2D1_POINT_2F d_pos;
			pt_sub(m_curr_pos, m_press_pos, d_pos);
			if (pt_abs2(d_pos) > m_click_dist) {
				// ������臒l�𒴂���ꍇ,
				if (m_draw_tool != DRAW_TOOL::SELECT) {
					// ��}�c�[�����I���c�[���łȂ��ꍇ,
					// �͈͂�I�����Ă����ԂɑJ�ڂ���.
					m_press_state = STATE_TRAN::PRESS_AREA;
				}
				else if (m_press_shape == nullptr) {
					// �����ꂽ�}�`���k���̏ꍇ,
					// �͈͂�I�����Ă����ԂɑJ�ڂ���.
					m_press_state = STATE_TRAN::PRESS_AREA;
					// �\���J�[�\�����J�[�\���ɐݒ肷��.
					Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
				}
				else if (m_list_selected > 1
					|| m_press_anchor == ANCH_WHICH::ANCH_FRAME
					|| m_press_anchor == ANCH_WHICH::ANCH_INSIDE
					|| m_press_anchor == ANCH_WHICH::ANCH_TEXT) {
					// �I�����ꂽ�}�`�̐��� 1 �𒴂���
					// �܂��͉����ꂽ�}�`�̕��ʂ����g
					// �܂��͉����ꂽ�}�`�̕��ʂ�����
					// �܂��͉����ꂽ�}�`�̕��ʂ�������
					m_press_state = STATE_TRAN::PRESS_MOVE;
					// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
					m_prev_pos = m_curr_pos;
					// �}�`�̈ʒu���X�^�b�N�ɐς�, ���������ړ�����.
					undo_push_pos(d_pos);
				}
				else if (m_press_anchor != ANCH_WHICH::ANCH_OUTSIDE) {
					m_press_state = STATE_TRAN::PRESS_FORM;
					// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
					m_prev_pos = m_curr_pos;
					// �}�`�̕ό`�O�̈ʒu���X�^�b�N�ɕۑ�����, �}�`��ό`����.
					undo_push_form(m_press_shape, m_press_anchor, m_curr_pos);
				}
				page_draw();
			}
		}
	}

	// �|�C���^�[�̃{�^���������ꂽ.
	void MainPage::scp_pointer_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
		using winrt::Windows::UI::Input::PointerPointProperties;

#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		// �X���b�v�`�F�[���p�l���̃|�C���^�[�̃v���p�e�B�[�𓾂�.
		auto const& p_prop = args.GetCurrentPoint(scp_page_panel()).Properties();
		// �|�C���^�[�̃L���v�`�����n�߂�.
		scp_page_panel().CapturePointer(args.Pointer());
		// �C�x���g�������Ԃ𓾂�.
		auto t_stamp = args.GetCurrentPoint(scp_page_panel()).Timestamp();
		// �X���b�v�`�F�[���p�l����ł̃|�C���^�[�̈ʒu�𓾂�, 
		// �y�[�W���W�n�ɕϊ���, �|�C���^�[�̌��݈ʒu�Ɋi�[����.
		D2D1_POINT_2F p_offs;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), p_offs);
		const auto ui_pos{ args.GetCurrentPoint(scp_page_panel()).Position() };
		pt_scale(ui_pos, 1.0 / m_page_panel.m_page_scale, p_offs, m_curr_pos);
		switch (m_press_state) {
		case STATE_TRAN::CLICK:
			// ��Ԃ��N���b�N������Ԃ̏ꍇ,
			// �C�x���g�������ԂƑO�񉟂��ꂽ���ԂƂ̎��ԍ��𓾂�.
			if (t_stamp - m_press_time <= m_click_time) {
				if (p_prop.IsLeftButtonPressed()) {
					// ���ԍ���臒l�ȉ�,
					// ���v���p�e�B�[�����{�^���̉����̏ꍇ,
					// �N���b�N����ɍ��{�^������������ԂɑJ�ڂ���.
					m_press_state = STATE_TRAN::CLICK_2;
					break;
				}
			}
			// �u���[�N�Ȃ�.
		case STATE_TRAN::BEGIN:
			// ��Ԃ�������Ԃ̏ꍇ, 
			// �X���b�v�`�F�[���p�l���̃|�C���^�[�̃v���p�e�B�[�𓾂�.
			if (p_prop.IsLeftButtonPressed()) {
				// �v���p�e�B�[�����{�^���̉����̏ꍇ,
				// ���{�^���������ꂽ��ԂɑJ�ڂ���.
				m_press_state = STATE_TRAN::PRESS_L;
				break;
			}
			else if (p_prop.IsRightButtonPressed()) {
				// �v���p�e�B�[���E�{�^���̉����̏ꍇ,
				// �E�{�^���������ꂽ��ԂɑJ�ڂ���.
				m_press_state = STATE_TRAN::PRESS_R;
				break;
			}
			// �u���[�N�Ȃ�.
		default:
			// ����ȊO�̏ꍇ, ��Ԃ�������Ԃɖ߂��ďI������.
			m_press_state = STATE_TRAN::BEGIN;
			return;
		}
		// �C�x���g�������Ԃ��|�C���^�[�������ꂽ���ԂɊi�[����.
		// �C�x���g�����ʒu���|�C���^�[�������ꂽ�ʒu�Ɋi�[����.
		m_press_time = t_stamp;
		m_press_pos = m_curr_pos;
		if (m_draw_tool != DRAW_TOOL::SELECT) {
			// ��}�c�[�����I���c�[���łȂ��ꍇ,
			// �I������.
			return;
		}
		// �|�C���^�[�������ꂽ�ʒu���܂ސ}�`�Ƃ��̕��ʂ����X�g���瓾��.
		m_press_anchor = s_list_hit_test(m_list_shapes, m_press_pos, m_page_dx.m_anch_len, m_press_shape);
		if (m_press_anchor != ANCH_WHICH::ANCH_OUTSIDE) {
			// �}�`�Ƃ��̕��ʂ𓾂��ꍇ,
			// �����l���|�C���^�[�������ꂽ�}�`�ƕ��ʂɊi�[����.
			// �����}�`���ꗗ�Ń|�C���^�[�������ꂽ�}�`�Ɋi�[����.
			m_press_shape_summary = m_press_shape;
			// �����}�`��I������.
			select_shape(m_press_shape, args.KeyModifiers());
			// �I������.
			return;
		}
		// �k����, �|�C���^�[�������ꂽ�}�`�ƑO��|�C���^�[�������ꂽ�}�`, �ꗗ�Ń|�C���^�[�������ꂽ�}�`�Ɋi�[����.
		// ANCH_OUTSIDE ���|�C���^�[�������ꂽ���ʂɊi�[����.
		m_press_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		m_press_shape = nullptr;
		m_press_shape_prev = nullptr;
		m_press_shape_summary = nullptr;
		// �L�[�C���q���n���h���[�̈������瓾��.
		if (args.KeyModifiers() != VirtualKeyModifiers::None) {
			// �L�[�C���q�� None �łȂ��ꍇ, �I������.
			return;
		}
		// ���ׂĂ̐}�`�̑I������������.
		if (unselect_all() == false) {
			// �I�����������ꂽ�}�`���Ȃ��ꍇ, �I������.
			return;
		}
		// ��蒼������X�^�b�N��������, �܂܂�鑀���j������.
		//redo_clear();
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		enable_edit_menu();
		// �y�[�W�Ɛ}�`��\������.
		page_draw();
	}

	// �R���e�L�X�g���j���[��\������.
	void MainPage::show_context_menu(void)
	{
		if (m_press_shape == nullptr
			|| m_press_anchor == ANCH_WHICH::ANCH_OUTSIDE) {
			// �����ꂽ�}�`���k��, 
			// �܂��͉����ꂽ���ʂ͊O���̏ꍇ,
			// �y�[�W�R���e�L�X�g���j���[��\������.
			scp_page_panel().ContextFlyout(nullptr);
			scp_page_panel().ContextFlyout(m_menu_layout);
		}
		else if (typeid(*m_press_shape) == typeid(ShapeGroup)) {
			// �����ꂽ�}�`���O���[�v�̏ꍇ,
			scp_page_panel().ContextFlyout(nullptr);
			scp_page_panel().ContextFlyout(m_menu_ungroup);
		}
		else {
			// �����ꂽ�}�`�̑����l���y�[�W�̃p�l���Ɋi�[����.
			m_page_panel.set_to_shape(m_press_shape);
			if (m_press_anchor == ANCH_WHICH::ANCH_INSIDE) {
				// �����ꂽ�}�`�̕��ʂ������̏ꍇ,
				// �h��Ԃ��R���e�L�X�g���j���[��\������.
				scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_fill);
			}
			else if (m_press_anchor == ANCH_WHICH::ANCH_TEXT) {
				// �����ꂽ�}�`�̕��ʂ�������̏ꍇ,
				// ���̃R���e�L�X�g���j���[��\������.
				scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_font);
			}
			else if (m_press_anchor == ANCH_WHICH::ANCH_FRAME) {
				// �����ꂽ�}�`�̕��ʂ��g��̏ꍇ,
				// ���g�R���e�L�X�g���j���[��\������.
				scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_stroke);
			}
		}
	}

	// �|�C���^�[�̃{�^�����グ��ꂽ.
	void MainPage::scp_pointer_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif
		// �|�C���^�[�̒ǐՂ��~����.
		scp_page_panel().ReleasePointerCaptures();
		// �X���b�v�`�F�[���p�l����ł̃|�C���^�[�̈ʒu�𓾂�, �y�[�W���W�ɕϊ���,
		// �|�C���^�[�̌��݈ʒu�Ɋi�[����.
		D2D1_POINT_2F p_offs;
		pt_add(m_page_min, sb_horz().Value(), sb_vert().Value(), p_offs);
		const auto ui_pos{ args.GetCurrentPoint(scp_page_panel()).Position() };
		pt_scale(ui_pos, 1.0 / m_page_panel.m_page_scale, p_offs, m_curr_pos);
		if (m_press_state == STATE_TRAN::PRESS_L) {
			// ���{�^���������ꂽ��Ԃ̏ꍇ,
			// �{�^�������ꂽ�����Ɖ����ꂽ�����̍����𓾂�.
			const auto t_stamp = args.GetCurrentPoint(scp_page_panel()).Timestamp();
			if (t_stamp - m_press_time <= m_click_time) {
				// �������N���b�N�̔��莞�Ԉȉ��̏ꍇ,
				// �N���b�N������ԂɑJ�ڂ���.
				// �󋵂ɉ������`��̃J�[�\����ݒ肷��.
				m_press_state = STATE_TRAN::CLICK;
				set_pointer();
				return;
			}
		}
		else if (m_press_state == STATE_TRAN::CLICK_2) {
			// �N���b�N��ɍ��{�^������������Ԃ̏ꍇ,
			// �{�^���������ꂽ�����Ɖ����ꂽ�����̍����𓾂�.
			const auto t_stamp = args.GetCurrentPoint(scp_page_panel()).Timestamp();
			if (t_stamp - m_press_time <= m_click_time) {
				// �������N���b�N�̔��莞�Ԉȉ���
				if (m_press_shape != nullptr && typeid(*m_press_shape) == typeid(ShapeText)) {
					// �����ꂽ�}�`��������}�`�̏ꍇ, 
					// ������ҏW�_�C�A���O��\������.
					text_edit_in(static_cast<ShapeText*>(m_press_shape));
				}
			}
		}
		else if (m_press_state == STATE_TRAN::PRESS_MOVE) {
			// ��Ԃ��}�`���ړ����Ă����Ԃ̏ꍇ,
			// �}�`�̈ړ����I������.
			finish_move_shape();
		}
		else if (m_press_state == STATE_TRAN::PRESS_FORM) {
			// ��Ԃ��}�`��ό`���Ă����Ԃ̏ꍇ,
			// �}�`�̕ό`���I������.
			finish_form_shape();
		}
		else if (m_press_state == STATE_TRAN::PRESS_AREA) {
			// ��Ԃ��͈͑I�����Ă����Ԃ̏ꍇ,
			if (m_draw_tool == DRAW_TOOL::SELECT) {
				// ��}�c�[�����I���c�[���̏ꍇ,
				// �͈͑I�����I������.
				finish_area_select(args.KeyModifiers());
			}
			else {
				// �}�`�̍쐬���I������.
				finish_create_shape();
			}
		}
		else if (m_press_state == STATE_TRAN::PRESS_R) {
			// ��Ԃ��E�{�^������������Ԃ̏ꍇ
			show_context_menu();
		}
		// ������Ԃɖ߂�.
		m_press_state = STATE_TRAN::BEGIN;
		m_press_shape = nullptr;
		m_press_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		page_draw();
	}

	// �|�C���^�[�̃z�C�[���{�^�������삳�ꂽ.
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
			// �R���g���[���L�[��������Ă����ꍇ, �p�l�����g��k������.
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
			// �V�t�g�L�[��������Ă����ꍇ, ���X�N���[������.
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

	// �󋵂ɉ������`��̃J�[�\����ݒ肷��.
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