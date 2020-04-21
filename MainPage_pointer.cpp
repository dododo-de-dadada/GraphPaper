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
		S_LIST_T list_deleted;
		for (const auto t : s_list) {
			if (t->is_deleted() != true) {
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
			list_deleted.push_back(t);
		}
		// �������X�g�Ɋ܂܂��}�`�����X�g�����菜��, �������.
		auto it_begin = s_list.begin();
		for (const auto s : list_deleted) {
			auto it = std::find(it_begin, s_list.end(), s);
			it_begin = s_list.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// �������X�g����������.
		list_deleted.clear();
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

	// �R���e�L�X�g���j���[��\������.
	void MainPage::pointer_context_menu(void)
	{
		scp_page_panel().ContextFlyout(nullptr);
		if (m_pointer_shape == nullptr
			|| m_pointer_anchor == ANCH_WHICH::ANCH_OUTSIDE) {
			// �����ꂽ�}�`���k��, 
			// �܂��͉����ꂽ���ʂ͊O���̏ꍇ,
			// �y�[�W�R���e�L�X�g���j���[��\������.
			//scp_page_panel().ContextFlyout(nullptr);
			scp_page_panel().ContextFlyout(m_menu_layout);
		}
		else if (typeid(*m_pointer_shape) == typeid(ShapeGroup)) {
			// �����ꂽ�}�`���O���[�v�̏ꍇ,
			//scp_page_panel().ContextFlyout(nullptr);
			scp_page_panel().ContextFlyout(m_menu_ungroup);
		}
		else {
			// �����ꂽ�}�`�̑����l���y�[�W���C�A�E�g�Ɋi�[����.
			m_page_layout.set_to(m_pointer_shape);
			if (m_pointer_anchor == ANCH_WHICH::ANCH_INSIDE) {
				// �����ꂽ�}�`�̕��ʂ������̏ꍇ,
				// �h��Ԃ��R���e�L�X�g���j���[��\������.
				//scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_fill);
			}
			else if (m_pointer_anchor == ANCH_WHICH::ANCH_TEXT) {
				// �����ꂽ�}�`�̕��ʂ�������̏ꍇ,
				// ���̃R���e�L�X�g���j���[��\������.
				//scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_font);
			}
			else if (m_pointer_anchor == ANCH_WHICH::ANCH_FRAME) {
				// �����ꂽ�}�`�̕��ʂ��g��̏ꍇ,
				// ���g�R���e�L�X�g���j���[��\������.
				//scp_page_panel().ContextFlyout(nullptr);
				scp_page_panel().ContextFlyout(m_menu_stroke);
			}
		}
	}

	// �|�C���^�[�̌��݈ʒu�𓾂�.
	void MainPage::pointer_cur_pos(PointerRoutedEventArgs const& args)
	{
		// �X���b�v�`�F�[���p�l����ł̃|�C���^�[�̈ʒu�𓾂�, 
		// �y�[�W���W�n�ɕϊ���, �|�C���^�[�̌��݈ʒu�Ɋi�[����.
		D2D1_POINT_2F p_offs;
		pt_add(page_min(), sb_horz().Value(), sb_vert().Value(), p_offs);
		pt_scale(args.GetCurrentPoint(scp_page_panel()).Position(), 1.0 / m_page_layout.m_page_scale, p_offs, m_pointer_cur);
	}

	// ������}�`�̍쐬���I������.
	IAsyncAction MainPage::pointer_finish_creating_text_async(const D2D1_POINT_2F diff)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		tx_edit().Text(L"");
		ck_text_adjust_bound().IsChecked(text_adjust());
		if (co_await cd_edit_text().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit().Text().c_str());
			auto s = new ShapeText(m_pointer_pressed, diff, text, &m_page_layout);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			text_adjust(ck_text_adjust_bound().IsChecked().GetBoolean());
			if (text_adjust()) {
				static_cast<ShapeText*>(s)->adjust_bound();
			}
			reduce_list(m_list_shapes, m_stack_undo, m_stack_redo);
			unselect_all();
			undo_push_append(s);
			undo_push_null();
			m_pointer_shape_summary = m_pointer_shape_prev = s;
			enable_edit_menu();
			page_bound(s);
			page_panle_size();
			if (m_mutex_summary.load(std::memory_order_acquire)) {
			//if (m_summary_visible) {
				summary_append(s);
			}
		}
		// ������Ԃɖ߂�.
		m_pointer_state = STATE_TRAN::BEGIN;
		m_pointer_shape = nullptr;
		m_pointer_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		page_draw();
	}

	// �}�`�̍쐬���I������.
	void MainPage::pointer_finish_creating(const D2D1_POINT_2F diff)
	{
		const auto draw_tool = tool();
		Shape* s;
		if (draw_tool == DRAW_TOOL::RECT) {
			s = new ShapeRect(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::RRCT) {
			s = new ShapeRRect(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::QUAD) {
			s = new ShapeQuad(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::ELLI) {
			s = new ShapeElli(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::LINE) {
			s = new ShapeLine(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::BEZI) {
			s = new ShapeBezi(m_pointer_pressed, diff, &m_page_layout);
		}
		else if (draw_tool == DRAW_TOOL::SCALE) {
			s = new ShapeScale(m_pointer_pressed, diff, &m_page_layout);
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
		undo_push_null();
		m_pointer_shape_summary = m_pointer_shape_prev = s;
		enable_edit_menu();
		page_bound(s);
		page_panle_size();
		page_draw();
		if (m_mutex_summary.load(std::memory_order_acquire)) {
		//if (m_summary_visible) {
			summary_append(s);
		}
	}

	// �}�`�̕ό`���I������.
	void MainPage::pointer_finish_forming(void)
	{
		if (m_page_layout.m_grid_snap) {
			pt_round(m_pointer_cur, m_page_layout.m_grid_base + 1.0, m_pointer_cur);
		}
		m_pointer_shape->set_pos(m_pointer_cur, m_pointer_anchor);
		if (undo_pop_if_invalid()) {
			return;
		}
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		page_bound();
		page_panle_size();
	}

	// �}�`�̈ړ����I������.
	void MainPage::pointer_finish_moving(void)
	{
		if (m_page_layout.m_grid_snap) {
			D2D1_POINT_2F p_min = {};
			bool flag = false;
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					continue;
				}
				if (s->is_selected() != true) {
					continue;
				}
				if (flag != true) {
					flag = true;
					s->get_min_pos(p_min);
					continue;
				}
				D2D1_POINT_2F nw_pos;
				s->get_min_pos(nw_pos);
				pt_min(nw_pos, p_min, p_min);
			}
			if (flag) {
				// ��������_�����̑傫���Ŋۂ߂�.
				// �ۂ߂̑O��Ő��������𓾂�.
				D2D1_POINT_2F g_pos;
				pt_round(p_min, m_page_layout.m_grid_base + 1.0, g_pos);
				D2D1_POINT_2F diff;
				pt_sub(g_pos, p_min, diff);
				s_list_move(m_list_shapes, diff);
			}
		}
		if (undo_pop_if_invalid()) {
			return;
		}
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		page_bound();
		page_panle_size();
		enable_edit_menu();
	}

	// �͈͑I�����I������.
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
			// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
			enable_edit_menu();
		}
		Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
	}

	// �󋵂ɉ������`��̃J�[�\����ݒ肷��.
	void MainPage::pointer_set(void)
	{
		if (tool() != DRAW_TOOL::SELECT) {
			Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
			return;
		}
		if (m_mutex_page.try_lock() != true) {
		//if (m_mutex_page.load()) {
			// ���b�N�ł��Ȃ��ꍇ
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
			return;
		}
		Shape* s;
		const auto a = s_list_hit_test(m_list_shapes, m_pointer_cur, page_anch_len(), s);
		if (a == ANCH_WHICH::ANCH_OUTSIDE || s->is_selected() != true) {
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
			m_mutex_page.unlock();
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
		m_mutex_page.unlock();
		return;
		//}
		//Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
	}

	// �|�C���^�[�̃{�^�����グ��ꂽ.
	void MainPage::pointer_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		pointer_released(sender, args);
	}

	// �|�C���^�[���y�[�W�̃X���b�v�`�F�[���p�l���̒��ɓ�����.
	void MainPage::pointer_entered(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		if (sender != scp_page_panel()) {
			Window::Current().CoreWindow().PointerCursor(CUR_ARROW);
			return;
		}
		pointer_cur_pos(args);
		pointer_set();
		stbar_set_curs();
	}

	// �|�C���^�[���y�[�W�̃X���b�v�`�F�[���p�l������o��.
	void MainPage::pointer_exited(IInspectable const& sender, PointerRoutedEventArgs const&)
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
	void MainPage::pointer_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::UI::Xaml::Window;
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		pointer_cur_pos(args);
		stbar_set_curs();
		if (m_pointer_state == STATE_TRAN::BEGIN) {
			// ��Ԃ�������Ԃ̏ꍇ,
			pointer_set();
		}
		else if (m_pointer_state == STATE_TRAN::CLICK) {
			// ��Ԃ��N���b�N������Ԃ̏ꍇ,
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̒����𓾂�.
			D2D1_POINT_2F diff;
			pt_sub(m_pointer_cur, m_pointer_pressed, diff);
			if (pt_abs2(diff) > m_pointer_click_dist) {
				// ������臒l�𒴂���ꍇ, ������Ԃɖ߂�.
				m_pointer_state = STATE_TRAN::BEGIN;
				pointer_set();
			}
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_AREA) {
			// ��Ԃ��͈͂�I�����Ă����Ԃ̏ꍇ,
			page_draw();
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_MOVE) {
			// ��Ԃ��}�`���ړ����Ă����Ԃ̏ꍇ,
			// �|�C���^�[�̌��݈ʒu�ƑO��ʒu�̍����𓾂�.
			D2D1_POINT_2F diff;
			pt_sub(m_pointer_cur, m_pointer_pre, diff);
			s_list_move(m_list_shapes, diff);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_pointer_pre = m_pointer_cur;
			page_draw();
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_FORM) {
			// ��Ԃ��}�`��ό`���Ă����Ԃ̏ꍇ,
			// �|�C���^�[�̌��݈ʒu�������ꂽ�}�`�̕��ʂ̈ʒu�Ɋi�[����.
			m_pointer_shape->set_pos(m_pointer_cur, m_pointer_anchor);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_pointer_pre = m_pointer_cur;
			page_draw();
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_L
			|| m_pointer_state == STATE_TRAN::CLICK_2) {
			// ��Ԃ����{�^���������Ă�����,
			// �܂��̓N���b�N��ɍ��{�^���������Ă����Ԃ̏ꍇ,
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̒����𓾂�.
			D2D1_POINT_2F diff;
			pt_sub(m_pointer_cur, m_pointer_pressed, diff);
			if (pt_abs2(diff) > m_pointer_click_dist) {
				// ������臒l�𒴂���ꍇ,
				if (tool() != DRAW_TOOL::SELECT) {
					// ��}�c�[�����I���c�[���łȂ��ꍇ,
					// �͈͂�I�����Ă����ԂɑJ�ڂ���.
					m_pointer_state = STATE_TRAN::PRESS_AREA;
				}
				else if (m_pointer_shape == nullptr) {
					// �����ꂽ�}�`���Ȃ��ꍇ,
					// �͈͂�I�����Ă����ԂɑJ�ڂ���.
					m_pointer_state = STATE_TRAN::PRESS_AREA;
					// �\���J�[�\�����J�[�\���ɐݒ肷��.
					Window::Current().CoreWindow().PointerCursor(CUR_CROSS);
				}
				else if (m_list_selected > 1
					|| m_pointer_anchor == ANCH_WHICH::ANCH_FRAME
					|| m_pointer_anchor == ANCH_WHICH::ANCH_INSIDE
					|| m_pointer_anchor == ANCH_WHICH::ANCH_TEXT) {
					// �I�����ꂽ�}�`�̐��� 1 �𒴂���
					// �܂��͉����ꂽ�}�`�̕��ʂ����g
					// �܂��͉����ꂽ�}�`�̕��ʂ�����
					// �܂��͉����ꂽ�}�`�̕��ʂ�������
					m_pointer_state = STATE_TRAN::PRESS_MOVE;
					// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
					m_pointer_pre = m_pointer_cur;
					undo_push_move(diff);
				}
				else if (m_pointer_anchor != ANCH_WHICH::ANCH_OUTSIDE) {
					// �����ꂽ�}�`�̕��ʂ��}�`�̊O���łȂ��ꍇ,
					// �}�`��ό`���Ă����ԂɑJ�ڂ���.
					m_pointer_state = STATE_TRAN::PRESS_FORM;
					m_pointer_pre = m_pointer_cur;
					undo_push_anchor(m_pointer_shape, m_pointer_anchor);
					m_pointer_shape->set_pos(m_pointer_cur, m_pointer_anchor);
				}
				page_draw();
			}
		}
	}

	// �|�C���^�[�̃{�^���������ꂽ.
	// �L���v�`���̗L���ɂ�����炸, �Е��̃}�E�X�{�^������������Ԃ�, ��������̃{�^���������Ă�, ����͒ʒm����Ȃ�.
	void MainPage::pointer_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
		using winrt::Windows::UI::Input::PointerPointProperties;
		using winrt::Windows::Devices::Input::PointerDeviceType;

#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		auto const& scp = sender.as<SwapChainPanel>();
		// �|�C���^�[�̃L���v�`�����n�߂�.
		scp.CapturePointer(args.Pointer());
		// �|�C���^�[�̃C�x���g�������Ԃ𓾂�.
		auto t_stamp = args.GetCurrentPoint(scp).Timestamp();
		pointer_cur_pos(args);
		// �|�C���^�[�̃v���p�e�B�[�𓾂�.
		auto const& p_prop = args.GetCurrentPoint(scp).Properties();
		switch (args.GetCurrentPoint(scp).PointerDevice().PointerDeviceType()) {
		case PointerDeviceType::Mouse:
			// �|�C���^�[�̃f�o�C�X�^�C�v���}�E�X�̏ꍇ
			if (p_prop.IsRightButtonPressed()) {
				// �v���p�e�B�[���E�{�^�������̏ꍇ,
				m_pointer_state = STATE_TRAN::PRESS_R;
			}
			else if (p_prop.IsLeftButtonPressed()) {
				// �v���p�e�B�[�����{�^�������̏ꍇ,
		case PointerDeviceType::Pen:
		case PointerDeviceType::Touch:
			// �|�C���^�[�̃f�o�C�X�^�C�v���y���܂��̓^�b�`�̏ꍇ
				switch (m_pointer_state) {
				case STATE_TRAN::CLICK:
					// �|�C���^�[�������ꂽ��Ԃ��N���b�N������Ԃ̏ꍇ,
					if (t_stamp - m_pointer_time <= m_pointer_click_time) {
						m_pointer_state = STATE_TRAN::CLICK_2;
					}
					else {
				case STATE_TRAN::BEGIN:
				default:
					// �|�C���^�[�������ꂽ��Ԃ��N���b�N������Ԃ̏ꍇ,
					m_pointer_state = STATE_TRAN::PRESS_L;
					}
				}
				break;
			}
			else {
		default:
				// �|�C���^�[�̃f�o�C�X�^�C�v������ȊO�̏ꍇ
				m_pointer_state = STATE_TRAN::BEGIN;
				return;
			}
		}
		m_pointer_time = t_stamp;
		m_pointer_pressed = m_pointer_cur;
		if (tool() != DRAW_TOOL::SELECT) {
			// ��}�c�[�����I���c�[���łȂ��ꍇ,
			return;
		}
		m_pointer_anchor = s_list_hit_test(m_list_shapes, m_pointer_pressed, page_anch_len(), m_pointer_shape);
		if (m_pointer_anchor != ANCH_WHICH::ANCH_OUTSIDE) {
			// �}�`�Ƃ��̕��ʂ𓾂��ꍇ,
			if (m_pointer_state == STATE_TRAN::PRESS_L
				|| (m_pointer_state == STATE_TRAN::PRESS_R && m_pointer_shape->is_selected() != true)) {
				m_pointer_shape_summary = m_pointer_shape;
				select_shape(m_pointer_shape, args.KeyModifiers());
			}
			return;
		}
		// �k����, �|�C���^�[�������ꂽ�}�`�ƑO��|�C���^�[�������ꂽ�}�`, �ꗗ�Ń|�C���^�[�������ꂽ�}�`�Ɋi�[����.
		// ANCH_OUTSIDE ���|�C���^�[�������ꂽ���ʂɊi�[����.
		m_pointer_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		m_pointer_shape = nullptr;
		m_pointer_shape_prev = nullptr;
		m_pointer_shape_summary = nullptr;
		// �L�[�C���q���n���h���[�̈������瓾��.
		if (args.KeyModifiers() != VirtualKeyModifiers::None) {
			// �L�[�C���q�� None �łȂ��ꍇ
			return;
		}
		if (unselect_all() != true) {
			// �I�����������ꂽ�}�`���Ȃ��ꍇ
			return;
		}
		enable_edit_menu();
		page_draw();
	}

	// �|�C���^�[�̃{�^�����グ��ꂽ.
	void MainPage::pointer_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
#if defined(_DEBUG)
		if (sender != scp_page_panel()) {
			return;
		}
#endif
		auto const& scp = sender.as<SwapChainPanel>();
		// �|�C���^�[�̒ǐՂ��~����.
		scp.ReleasePointerCaptures();
		pointer_cur_pos(args);
		if (m_pointer_state == STATE_TRAN::PRESS_L) {
			// ���{�^���������ꂽ��Ԃ̏ꍇ,
			// �{�^�������ꂽ�����Ɖ����ꂽ�����̍����𓾂�.
			const auto t_stamp = args.GetCurrentPoint(scp).Timestamp();
			if (t_stamp - m_pointer_time <= m_pointer_click_time) {
				// �������N���b�N�̔��莞�Ԉȉ��̏ꍇ,
				// �N���b�N������ԂɑJ�ڂ���.
				m_pointer_state = STATE_TRAN::CLICK;
				pointer_set();
				return;
			}
		}
		else if (m_pointer_state == STATE_TRAN::CLICK_2) {
			// �N���b�N��ɍ��{�^������������Ԃ̏ꍇ,
			// �{�^���������ꂽ�����Ɖ����ꂽ�����̍����𓾂�.
			const auto t_stamp = args.GetCurrentPoint(scp).Timestamp();
			if (t_stamp - m_pointer_time <= m_pointer_click_time) {
				// �������N���b�N�̔��莞�Ԉȉ���
				if (m_pointer_shape != nullptr && typeid(*m_pointer_shape) == typeid(ShapeText)) {
					// �����ꂽ�}�`��������}�`�̏ꍇ, 
					text_edit_async(static_cast<ShapeText*>(m_pointer_shape));
				}
			}
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_MOVE) {
			// ��Ԃ��}�`���ړ����Ă����Ԃ̏ꍇ,
			// �}�`�̈ړ����I������.
			pointer_finish_moving();
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_FORM) {
			// ��Ԃ��}�`��ό`���Ă����Ԃ̏ꍇ,
			// �}�`�̕ό`���I������.
			pointer_finish_forming();
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_AREA) {
			// ��Ԃ��͈͑I�����Ă����Ԃ̏ꍇ,
			if (tool() == DRAW_TOOL::SELECT) {
				// ��}�c�[�����I���c�[���̏ꍇ,
				pointer_finish_selecting_area(args.KeyModifiers());
			}
			else {
				if (m_page_layout.m_grid_snap) {
					// ����ɐ���̏ꍇ, �n�_�ƏI�_�����̑傫���Ŋۂ߂�
					double g = max(m_page_layout.m_grid_base + 1.0, 1.0);
					pt_round(m_pointer_pressed, g, m_pointer_pressed);
					pt_round(m_pointer_cur, g, m_pointer_cur);
				}
				// �|�C���^�[�̌��݂̈ʒu�Ɖ����ꂽ�ʒu�̍��������߂�.
				D2D1_POINT_2F diff;
				pt_sub(m_pointer_cur, m_pointer_pressed, diff);
				if (fabs(diff.x) >= 1.0f || fabs(diff.y) >= 1.0f) {
					if (tool() == DRAW_TOOL::TEXT) {
						pointer_finish_creating_text_async(diff);
						return;
					}
					pointer_finish_creating(diff);
				}
			}
		}
		else if (m_pointer_state == STATE_TRAN::PRESS_R) {
			// ��Ԃ��E�{�^������������Ԃ̏ꍇ
			pointer_context_menu();
		}
		else if (m_pointer_state == STATE_TRAN::BEGIN) {
			// ��Ԃ�������Ԃ̏ꍇ,
			// �{���͏�����Ԃł��̃n���h���[���Ăяo�����͂��͂Ȃ���,
			// �R���e���c�_�C�A���O���I�������Ƃ��Ăяo����Ă��܂�.
			return;
		}
		// ������Ԃɖ߂�.
		m_pointer_state = STATE_TRAN::BEGIN;
		m_pointer_shape = nullptr;
		m_pointer_anchor = ANCH_WHICH::ANCH_OUTSIDE;
		page_draw();
	}

	// �|�C���^�[�̃z�C�[���{�^�������삳�ꂽ.
	void MainPage::pointer_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
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
			// �R���g���[���L�[��������Ă����ꍇ, �y�[�W���g��k������.
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
			// �V�t�g�L�[��������Ă����ꍇ, ���X�N���[������.
			double value = stbar.Value();
			double limit = 0.0;
			if (delta < 0 && value < (limit = stbar.Maximum())) {
				value = min(value + 32.0 * m_page_layout.m_page_scale, limit);
			}
			else if (delta > 0 && value > (limit = stbar.Minimum())) {
				value = max(value - 32.0 * m_page_layout.m_page_scale, limit);
			}
			else {
				return;
			}
			stbar.Value(value);
			page_draw();
		}
	}

}