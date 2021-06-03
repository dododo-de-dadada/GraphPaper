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

	static auto const& CC_ARROW = CoreCursor(CoreCursorType::Arrow, 0);	// ���J�[�\��
	static auto const& CC_CROSS = CoreCursor(CoreCursorType::Cross, 0);	// �\���J�[�\��
	static auto const& CC_SIZE_ALL = CoreCursor(CoreCursorType::SizeAll, 0);	// �ړ��J�[�\��
	static auto const& CC_SIZE_NESW = CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);	// �E�㍶���J�[�\��
	static auto const& CC_SIZE_NS = CoreCursor(CoreCursorType::SizeNorthSouth, 0);	// �㉺�J�[�\��
	static auto const& CC_SIZE_NWSE = CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);	// ����E���J�[�\��
	static auto const& CC_SIZE_WE = CoreCursor(CoreCursorType::SizeWestEast, 0);	// ���E�J�[�\��

	// �����t���O�̗��}�`�����X�g����폜����.
	static void reduce_slist(SHAPE_LIST& slist, UNDO_STACK const& u_stack, UNDO_STACK const& r_stack);
	// �}�`������X�^�b�N�Ɋ܂܂�邩���肷��.
	static bool refer_ro(UNDO_STACK const& u_stack, Shape* const s) noexcept;

	// �����t���O�̗��}�`�����X�g����폜����.
	static void reduce_slist(SHAPE_LIST& slist, UNDO_STACK const& u_stack, UNDO_STACK const& r_stack)
	{
		// �����t���O�̗��}�`���������X�g�Ɋi�[����.
		SHAPE_LIST list_deleted;
		for (const auto t : slist) {
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
		auto it_begin = slist.begin();
		for (const auto s : list_deleted) {
			const auto it = std::find(it_begin, slist.end(), s);
			it_begin = slist.erase(it);
			delete s;
#if defined(_DEBUG)
			debug_leak_cnt--;
#endif
		}
		// �������X�g����������.
		list_deleted.clear();
	}

	// ����X�^�b�N���}�`���Q�Ƃ��邩���肷��.
	// u_stack	����X�^�b�N
	// s	�}�`
	// �߂�l	�Q�Ƃ���ꍇ true.
	static bool refer_ro(UNDO_STACK const& u_stack, Shape* const s) noexcept
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

	// �|�C���^�[�̃{�^�����グ��ꂽ.
	void MainPage::event_canceled(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		event_released(sender, args);
	}

	// �R���e�L�X�g���j���[��\������.
	void MainPage::event_context_menu(void)
	{
		// �R���e�L�X�g���j���[���������.
		scp_sheet_panel().ContextFlyout(nullptr);
		// �����ꂽ�}�`���k��, �܂��͉����ꂽ�}�`�̕��ʂ��O�������肷��.
		if (m_event_shape_pressed == nullptr || m_event_anch_pressed == ANCH_TYPE::ANCH_SHEET) {
			scp_sheet_panel().ContextFlyout(m_menu_sheet);
		}
		// �����ꂽ�}�`���O���[�v�}�`�����肷��.
		else if (typeid(*m_event_shape_pressed) == typeid(ShapeGroup)) {
			scp_sheet_panel().ContextFlyout(m_menu_ungroup);
		}
		else {
			// �����ꂽ�}�`�̑����l��p���Ɋi�[����.
			m_sheet_main.set_attr_to(m_event_shape_pressed);
			// �����ꂽ�}�`�̕��ʂ����������肷��.
			if (m_event_anch_pressed == ANCH_TYPE::ANCH_FILL) {
				scp_sheet_panel().ContextFlyout(m_menu_fill);
			}
			// �����ꂽ�}�`�̕��ʂ������񂩔��肷��.
			else if (m_event_anch_pressed == ANCH_TYPE::ANCH_TEXT) {
				scp_sheet_panel().ContextFlyout(m_menu_font);
			}
			// �����ꂽ�}�`�̕��ʂ����g�����肷��.
			else if (m_event_anch_pressed == ANCH_TYPE::ANCH_STROKE) {
				scp_sheet_panel().ContextFlyout(m_menu_stroke);
			}
		}
	}

	// �|�C���^�[�̌��݈ʒu�𓾂�.
	void MainPage::event_pos_args(PointerRoutedEventArgs const& args)
	{
		// �X���b�v�`�F�[���p�l����ł̃|�C���^�[�̈ʒu�𓾂�, 
		// �p�����W�n�ɕϊ���, �|�C���^�[�̌��݈ʒu�Ɋi�[����.
		D2D1_POINT_2F p_offs;
		pt_add(sheet_min(), sb_horz().Value(), sb_vert().Value(), p_offs);
		pt_mul(args.GetCurrentPoint(scp_sheet_panel()).Position(), 1.0 / m_sheet_main.m_sheet_scale, p_offs, m_event_pos_curr);
	}

	// �|�C���^�[���p���̃X���b�v�`�F�[���p�l���̒��ɓ�����.
	void MainPage::event_entered(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		if (sender != scp_sheet_panel()) {
			Window::Current().CoreWindow().PointerCursor(CC_ARROW);
			return;
		}
		event_pos_args(args);
		event_curs_style();
		sbar_set_curs();
	}

	// �|�C���^�[���p���̃X���b�v�`�F�[���p�l������o��.
	void MainPage::event_exited(IInspectable const& sender, PointerRoutedEventArgs const&)
	{
		if (sender != scp_sheet_panel()) {
			return;
		}
		auto const& c_win = Window::Current().CoreWindow();
		auto const& cur = c_win.PointerCursor();
		if (cur.Type() == CC_ARROW.Type()) {
			return;
		}
		c_win.PointerCursor(CC_ARROW);
	}

	// �}�`�̍쐬���I������.
	void MainPage::event_finish_creating(const D2D1_POINT_2F diff)
	{
		const auto tool = tool_draw();
		Shape* s;
		if (tool == TOOL_DRAW::RECT) {
			s = new ShapeRect(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else if (tool == TOOL_DRAW::RRECT) {
			s = new ShapeRRect(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else if (tool == TOOL_DRAW::POLY) {
			s = new ShapePoly(m_event_pos_pressed, diff, &m_sheet_main, tool_poly());
		}
		else if (tool == TOOL_DRAW::ELLI) {
			s = new ShapeElli(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else if (tool == TOOL_DRAW::LINE) {
			s = new ShapeLineA(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else if (tool == TOOL_DRAW::BEZI) {
			s = new ShapeBezi(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else if (tool == TOOL_DRAW::RULER) {
			s = new ShapeRuler(m_event_pos_pressed, diff, &m_sheet_main);
		}
		else {
			return;
		}
#if defined(_DEBUG)
		debug_leak_cnt++;
#endif
		reduce_slist(m_list_shapes, m_stack_undo, m_stack_redo);
		unselect_all();
		undo_push_append(s);
		undo_push_select(s);
		undo_push_null();
		m_event_shape_smry = m_event_shape_prev = s;
		edit_menu_enable();
		sheet_update_bbox(s);
		sheet_panle_size();
		sheet_draw();
		// �}�`�ꗗ�̔r�����䂪 true �����肷��.
		if (m_smry_atomic.load(std::memory_order_acquire)) {
			smry_append(s);
			smry_select(s);
		}
	}

	// ������}�`�̍쐬���I������.
	IAsyncAction MainPage::event_finish_creating_text_async(const D2D1_POINT_2F diff)
	{
		using winrt::Windows::UI::Xaml::Controls::ContentDialogResult;

		tx_edit().Text(L"");
		ck_text_adjust_bbox().IsChecked(text_adjust());
		if (co_await cd_edit_text().ShowAsync() == ContentDialogResult::Primary) {
			auto text = wchar_cpy(tx_edit().Text().c_str());
			auto s = new ShapeText(m_event_pos_pressed, diff, text, &m_sheet_main);
#if defined(_DEBUG)
			debug_leak_cnt++;
#endif
			text_adjust(ck_text_adjust_bbox().IsChecked().GetBoolean());
			if (text_adjust()) {
				static_cast<ShapeText*>(s)->adjust_bbox();
			}
			reduce_slist(m_list_shapes, m_stack_undo, m_stack_redo);
			unselect_all();
			undo_push_append(s);
			undo_push_select(s);
			undo_push_null();
			m_event_shape_smry = m_event_shape_prev = s;
			edit_menu_enable();
			sheet_update_bbox(s);
			sheet_panle_size();
			// �}�`�ꗗ�̔r�����䂪 true �����肷��.
			if (m_smry_atomic.load(std::memory_order_acquire)) {
				smry_append(s);
				smry_select(s);
			}
		}
		// �|�C���^�[�̉����ꂽ��Ԃ�������Ԃɖ߂�.
		m_event_state = PBTN_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
		sheet_draw();
	}

	// �}�`�̕ό`���I������.
	void MainPage::event_finish_forming(void)
	{
		bool g_snap;
		m_sheet_main.get_grid_snap(g_snap);
		if (g_snap) {
			float g_base;
			m_sheet_main.get_grid_base(g_base);
			pt_round(m_event_pos_curr, g_base + 1.0, m_event_pos_curr);
		}
		m_event_shape_pressed->set_anchor_pos(m_event_pos_curr, m_event_anch_pressed);
		if (undo_pop_if_invalid()) {
			return;
		}
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		sheet_update_bbox();
		sheet_panle_size();
	}

	// �}�`�̈ړ����I������.
	void MainPage::event_finish_moving(void)
	{
		bool g_snap;
		m_sheet_main.get_grid_snap(g_snap);
		if (g_snap) {
			D2D1_POINT_2F b_nw{};
			D2D1_POINT_2F b_se{};
			bool flag = false;
			for (auto s : m_list_shapes) {
				if (s->is_deleted()) {
					continue;
				}
				if (s->is_selected() != true) {
					continue;
				}
				if (!flag) {
					flag = true;
					s->get_bound({ FLT_MAX, FLT_MAX }, { -FLT_MAX, -FLT_MAX }, b_nw, b_se);
				}
				else {
					s->get_bound(b_nw, b_se, b_nw, b_se);
				}
			}
			if (flag) {
				float g_base;
				m_sheet_main.get_grid_base(g_base);
				const double g_len = g_base + 1.0;
				D2D1_POINT_2F b_ne{ b_se.x, b_nw.y };
				D2D1_POINT_2F b_sw{ b_nw.x, b_se.y };

				D2D1_POINT_2F g_nw;
				D2D1_POINT_2F g_se;
				D2D1_POINT_2F g_ne;
				D2D1_POINT_2F g_sw;
				pt_round(b_nw, g_len, g_nw);
				pt_round(b_se, g_len, g_se);
				pt_round(b_ne, g_len, g_ne);
				pt_round(b_sw, g_len, g_sw);

				D2D1_POINT_2F d_nw;
				D2D1_POINT_2F d_se;
				D2D1_POINT_2F d_ne;
				D2D1_POINT_2F d_sw;
				pt_sub(g_nw, b_nw, d_nw);
				pt_sub(g_se, b_se, d_se);
				pt_sub(g_ne, b_ne, d_ne);
				pt_sub(g_sw, b_sw, d_sw);

				double a_nw = pt_abs2(d_nw);
				double a_se = pt_abs2(d_se);
				double a_ne = pt_abs2(d_ne);
				double a_sw = pt_abs2(d_sw);
				D2D1_POINT_2F diff;
				if (a_se <= a_nw && a_se <= a_ne && a_nw <= a_sw) {
					diff = d_se;
				}
				else if (a_ne <= a_nw && a_ne <= a_se && a_nw <= a_sw) {
					diff = d_ne;
				}
				else if (a_sw <= a_nw && a_sw <= a_se && a_sw <= a_ne) {
					diff = d_sw;
				}
				else {
					diff = d_nw;
				}
				if (flag != true) {
					flag = true;
				}
				slist_move(m_list_shapes, diff);
			}
		}
		if (undo_pop_if_invalid()) {
			return;
		}
		// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
		undo_push_null();
		sheet_update_bbox();
		sheet_panle_size();
		edit_menu_enable();
	}

	// �͈͑I�����I������.
	void MainPage::event_finish_selecting_area(const VirtualKeyModifiers k_mod)
	{
		using winrt::Windows::UI::Xaml::Window;

		auto flag = false;
		if (k_mod == VirtualKeyModifiers::Control) {
			D2D1_POINT_2F a_min;
			D2D1_POINT_2F a_max;
			pt_bound(m_event_pos_pressed, m_event_pos_curr, a_min, a_max);
			flag = toggle_area(a_min, a_max);
		}
		else if (k_mod == VirtualKeyModifiers::None) {
			D2D1_POINT_2F a_min;
			D2D1_POINT_2F a_max;
			pt_bound(m_event_pos_pressed, m_event_pos_curr, a_min, a_max);
			flag = select_area(a_min, a_max);
		}
		if (flag == true) {
			// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
			edit_menu_enable();
		}
		Window::Current().CoreWindow().PointerCursor(CC_ARROW);
	}

	// �󋵂ɉ������`��̃J�[�\����ݒ肷��.
	void MainPage::event_curs_style(void)
	{
		if (tool_draw() != TOOL_DRAW::SELECT) {
			// ��}�c�[�����I���c�[���łȂ��ꍇ
			Window::Current().CoreWindow().PointerCursor(CC_CROSS);
			return;
		}
		if (m_dx_mutex.try_lock() != true) {
			Window::Current().CoreWindow().PointerCursor(CC_ARROW);
			return;
		}
		Shape* s;
		const auto anch = slist_hit_test(m_list_shapes, m_event_pos_curr, s);
		m_dx_mutex.unlock();
		if (anch == ANCH_TYPE::ANCH_SHEET) {
			Window::Current().CoreWindow().PointerCursor(CC_ARROW);
		}
		else if (m_cnt_selected > 1) {
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

	// �|�C���^�[��������.
	void MainPage::event_moved(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::UI::Xaml::Window;
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		event_pos_args(args);
		sbar_set_curs();
		// �|�C���^�[�̉����ꂽ��Ԃ�������Ԃ����肷��.
		if (m_event_state == PBTN_STATE::BEGIN) {
			event_curs_style();
		}
		// �|�C���^�[�̉����ꂽ��Ԃ��N���b�N������Ԃ����肷��.
		else if (m_event_state == PBTN_STATE::CLICK) {
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�̒����𓾂�.
			D2D1_POINT_2F diff;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, diff);
			if (pt_abs2(diff) > m_event_click_dist) {
				// ������臒l�𒴂���ꍇ, ������Ԃɖ߂�.
				m_event_state = PBTN_STATE::BEGIN;
				event_curs_style();
			}
		}
		// �|�C���^�[�̉����ꂽ��Ԃ��͈͂�I�����Ă����Ԃ����肷��.
		else if (m_event_state == PBTN_STATE::PRESS_AREA) {
			sheet_draw();
		}
		// �|�C���^�[�̉����ꂽ��Ԃ��}�`���ړ����Ă����Ԃ����肷��.
		else if (m_event_state == PBTN_STATE::PRESS_MOVE) {
			// �|�C���^�[�̌��݈ʒu�ƑO��ʒu�̍����𓾂�.
			D2D1_POINT_2F diff;
			pt_sub(m_event_pos_curr, m_event_pos_prev, diff);
			slist_move(m_list_shapes, diff);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_event_pos_prev = m_event_pos_curr;
			sheet_draw();
		}
		// �|�C���^�[�̉����ꂽ��Ԃ��}�`��ό`���Ă����Ԃ����肷��.
		else if (m_event_state == PBTN_STATE::PRESS_FORM) {
			// �|�C���^�[�̌��݈ʒu��, �|�C���^�[�������ꂽ�}�`��, ���ʂ̈ʒu�Ɋi�[����.
			m_event_shape_pressed->set_anchor_pos(m_event_pos_curr, m_event_anch_pressed);
			// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
			m_event_pos_prev = m_event_pos_curr;
			sheet_draw();
		}
		// �|�C���^�[�̉����ꂽ��Ԃ����{�^���������Ă�����, �܂��̓N���b�N��ɍ��{�^������������Ԃ����肷��.
		else if (m_event_state == PBTN_STATE::PRESS_LBTN || m_event_state == PBTN_STATE::CLICK_LBTN) {
			// �|�C���^�[�̌��݈ʒu�Ɖ����ꂽ�ʒu�Ƃ̍����𓾂�.
			D2D1_POINT_2F diff;
			pt_sub(m_event_pos_curr, m_event_pos_pressed, diff);
			// �����̒������N���b�N�̔��苗���𒴂��邩���肷��.
			if (pt_abs2(diff) > m_event_click_dist) {
				// ��}�c�[�����I���c�[���łȂ������肷��.
				if (tool_draw() != TOOL_DRAW::SELECT) {
					// �͈͂�I�����Ă����ԂɑJ�ڂ���.
					m_event_state = PBTN_STATE::PRESS_AREA;
				}
				// �����ꂽ�}�`���k�������肷��.
				else if (m_event_shape_pressed == nullptr) {
					// �͈͂�I�����Ă����ԂɑJ�ڂ���.
					m_event_state = PBTN_STATE::PRESS_AREA;
					// �\���J�[�\�����J�[�\���ɐݒ肷��.
					Window::Current().CoreWindow().PointerCursor(CC_CROSS);
				}
				// �I�����ꂽ�}�`�̐��� 1 �𒴂���,
				// �܂��͉����ꂽ�}�`�̕��ʂ����g, ����, �����񂩂𔻒肷��.
				else if (m_cnt_selected > 1 ||
					m_event_anch_pressed == ANCH_TYPE::ANCH_STROKE || m_event_anch_pressed == ANCH_TYPE::ANCH_FILL || m_event_anch_pressed == ANCH_TYPE::ANCH_TEXT) {
					// ��Ԃ�}�`���ړ����Ă����ԂɑJ�ڂ���.
					m_event_state = PBTN_STATE::PRESS_MOVE;
					// �|�C���^�[�̌��݈ʒu��O��ʒu�Ɋi�[����.
					m_event_pos_prev = m_event_pos_curr;
					undo_push_move(diff);
				}
				// �|�C���^�[�������ꂽ�}�`�̕��ʂ��}�`�̊O���łȂ������肷��
				else if (m_event_anch_pressed != ANCH_TYPE::ANCH_SHEET) {
					// �}�`��ό`���Ă����ԂɑJ�ڂ���.
					m_event_state = PBTN_STATE::PRESS_FORM;
					m_event_pos_prev = m_event_pos_curr;
					undo_push_anchor(m_event_shape_pressed, m_event_anch_pressed);
					m_event_shape_pressed->set_anchor_pos(m_event_pos_curr, m_event_anch_pressed);
				}
				sheet_draw();
			}
		}
	}

	// �|�C���^�[�̃{�^���������ꂽ.
	// �L���v�`���̗L���ɂ�����炸, �Е��̃}�E�X�{�^������������Ԃ�, ��������̃{�^���������Ă�, ����͒ʒm����Ȃ�.
	void MainPage::event_pressed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
		using winrt::Windows::UI::Input::PointerPointProperties;
		using winrt::Windows::Devices::Input::PointerDeviceType;

#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		auto const& panel = sender.as<SwapChainPanel>();
		// �|�C���^�[�̃L���v�`�����n�߂�.
		panel.CapturePointer(args.Pointer());
		// �|�C���^�[�̃C�x���g�������Ԃ𓾂�.
		auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
		event_pos_args(args);
		// �|�C���^�[�̃v���p�e�B�[�𓾂�.
		auto const& p_prop = args.GetCurrentPoint(panel).Properties();
		// �|�C���^�[�̃f�o�C�X�^�C�v�𔻒肷��.
		switch (args.GetCurrentPoint(panel).PointerDevice().PointerDeviceType()) {
		// �f�o�C�X�^�C�v���}�E�X�̏ꍇ
		case PointerDeviceType::Mouse:
			// �v���p�e�B�[���E�{�^�����������肷��.
			if (p_prop.IsRightButtonPressed()) {
				m_event_state = PBTN_STATE::PRESS_RBTN;
			}
			// �v���p�e�B�[�����{�^�����������肷��.
			else if (p_prop.IsLeftButtonPressed()) {
				[[fallthrough]];
		// �f�o�C�X�^�C�v���y���܂��̓^�b�`�̏ꍇ
		case PointerDeviceType::Pen:
		case PointerDeviceType::Touch:
				// �|�C���^�[�̉����ꂽ��Ԃ𔻒肷��.
				switch (m_event_state) {
				// �����ꂽ��Ԃ��N���b�N������Ԃ̏ꍇ
				case PBTN_STATE::CLICK:
					if (t_stamp - m_event_time_pressed <= m_event_click_time) {
						m_event_state = PBTN_STATE::CLICK_LBTN;
					}
					else {
						[[fallthrough]];
				// �����ꂽ��Ԃ�������Ԃ̏ꍇ
				case PBTN_STATE::BEGIN:
				default:
						m_event_state = PBTN_STATE::PRESS_LBTN;
					}
				}
				break;
			}
			else {
				[[fallthrough]];
		// �f�o�C�X�^�C�v������ȊO�̏ꍇ
		default:
				m_event_state = PBTN_STATE::BEGIN;
				return;
			}
		}
		m_event_time_pressed = t_stamp;
		m_event_pos_pressed = m_event_pos_curr;
		// ��}�c�[�����I���c�[���łȂ������肷��.
		if (tool_draw() != TOOL_DRAW::SELECT) {
			return;
		}
		m_event_anch_pressed = slist_hit_test(m_list_shapes, m_event_pos_pressed, m_event_shape_pressed);
		if (m_event_anch_pressed != ANCH_TYPE::ANCH_SHEET) {
			if (m_event_state == PBTN_STATE::PRESS_LBTN
				|| (m_event_state == PBTN_STATE::PRESS_RBTN && m_event_shape_pressed->is_selected() != true)) {
				m_event_shape_smry = m_event_shape_pressed;
				select_shape(m_event_shape_pressed, args.KeyModifiers());
			}
			return;
		}
		// �k����, �|�C���^�[�������ꂽ�}�`�ƑO��|�C���^�[�������ꂽ�}�`, �ꗗ�Ń|�C���^�[�������ꂽ�}�`�Ɋi�[����.
		// ANCH_SHEET ���|�C���^�[�������ꂽ���ʂɊi�[����.
		m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
		m_event_shape_pressed = nullptr;
		m_event_shape_prev = nullptr;
		m_event_shape_smry = nullptr;
		// �L�[�C���q���n���h���[�̈������瓾��.
		if (args.KeyModifiers() != VirtualKeyModifiers::None) {
			// �L�[�C���q�� None �łȂ��ꍇ
			return;
		}
		// �I�����������ꂽ�}�`���Ȃ��ꍇ
		if (!unselect_all()) {
			return;
		}
		edit_menu_enable();
		sheet_draw();
	}

	// �|�C���^�[�̃{�^�����グ��ꂽ.
	void MainPage::event_released(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			return;
		}
#endif
		auto const& panel = sender.as<SwapChainPanel>();
		// �|�C���^�[�̒ǐՂ��~����.
		panel.ReleasePointerCaptures();
		event_pos_args(args);
		// ���{�^���������ꂽ��Ԃ����肷��.
		if (m_event_state == PBTN_STATE::PRESS_LBTN) {
			// �{�^�������ꂽ�����Ɖ����ꂽ�����̍����𓾂�.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			// �������N���b�N�̔��莞�Ԉȉ������肷��.
			if (t_stamp - m_event_time_pressed <= m_event_click_time) {
				// �N���b�N������ԂɑJ�ڂ���.
				m_event_state = PBTN_STATE::CLICK;
				event_curs_style();
				return;
			}
		}
		// �N���b�N��ɍ��{�^������������Ԃ����肷��.
		else if (m_event_state == PBTN_STATE::CLICK_LBTN) {
			// �{�^���������ꂽ�����Ɖ����ꂽ�����̍����𓾂�.
			const auto t_stamp = args.GetCurrentPoint(panel).Timestamp();
			// �������N���b�N�̔��莞�Ԉȉ������肷��.
			if (t_stamp - m_event_time_pressed <= m_event_click_time) {
				// �����ꂽ�}�`��������}�`�����肷��. 
				if (m_event_shape_pressed != nullptr && typeid(*m_event_shape_pressed) == typeid(ShapeText)) {
					text_edit_async(static_cast<ShapeText*>(m_event_shape_pressed));
				}
			}
		}
		else if (m_event_state == PBTN_STATE::PRESS_MOVE) {
			// ��Ԃ��}�`���ړ����Ă����Ԃ̏ꍇ,
			// �}�`�̈ړ����I������.
			event_finish_moving();
		}
		else if (m_event_state == PBTN_STATE::PRESS_FORM) {
			// ��Ԃ��}�`��ό`���Ă����Ԃ̏ꍇ,
			// �}�`�̕ό`���I������.
			event_finish_forming();
		}
		else if (m_event_state == PBTN_STATE::PRESS_AREA) {
			// ��Ԃ��͈͑I�����Ă����Ԃ̏ꍇ,
			if (tool_draw() == TOOL_DRAW::SELECT) {
				// ��}�c�[�����I���c�[���̏ꍇ,
				event_finish_selecting_area(args.KeyModifiers());
			}
			else {
				bool g_snap;
				m_sheet_main.get_grid_snap(g_snap);
				if (g_snap) {
					// ����ɐ���̏ꍇ, �n�_�ƏI�_�����̑傫���Ŋۂ߂�
					if (args.KeyModifiers() != VirtualKeyModifiers::Shift) {
						float g_base;
						m_sheet_main.get_grid_base(g_base);
						const double g_len = max(g_base + 1.0, 1.0);
						pt_round(m_event_pos_pressed, g_len, m_event_pos_pressed);
						pt_round(m_event_pos_curr, g_len, m_event_pos_curr);
					}
				}
				// �|�C���^�[�̌��݂̈ʒu�Ɖ����ꂽ�ʒu�̍��������߂�.
				D2D1_POINT_2F diff;
				pt_sub(m_event_pos_curr, m_event_pos_pressed, diff);
				if (fabs(diff.x) >= 1.0f || fabs(diff.y) >= 1.0f) {
					if (tool_draw() == TOOL_DRAW::TEXT) {
						event_finish_creating_text_async(diff);
						return;
					}
					event_finish_creating(diff);
				}
			}
		}
		else if (m_event_state == PBTN_STATE::PRESS_RBTN) {
			// ��Ԃ��E�{�^������������Ԃ̏ꍇ
			event_context_menu();
		}
		else if (m_event_state == PBTN_STATE::BEGIN) {
			// ��Ԃ�������Ԃ̏ꍇ,
			// �{���͏�����Ԃł��̃n���h���[���Ăяo�����͂��͂Ȃ���,
			// �R���e���c�_�C�A���O���I�������Ƃ��Ăяo����Ă��܂�.
			return;
		}
		// ������Ԃɖ߂�.
		m_event_state = PBTN_STATE::BEGIN;
		m_event_shape_pressed = nullptr;
		m_event_anch_pressed = ANCH_TYPE::ANCH_SHEET;
		sheet_draw();
	}

	// �|�C���^�[�̃z�C�[���{�^�������삳�ꂽ.
	void MainPage::event_wheel_changed(IInspectable const& sender, PointerRoutedEventArgs const& args)
	{
		using winrt::Windows::System::VirtualKeyModifiers;
		using winrt::Windows::UI::Xaml::UIElement;
		using winrt::Windows::UI::Xaml::Controls::Primitives::ScrollBar;

#if defined(_DEBUG)
		if (sender != scp_sheet_panel()) {
			throw winrt::hresult_not_implemented();
		}
#endif
		int32_t delta = args.GetCurrentPoint(scp_sheet_panel()).Properties().MouseWheelDelta();
		if (args.KeyModifiers() == VirtualKeyModifiers::Control) {
			// �R���g���[���L�[��������Ă����ꍇ, �p�����g��k������.
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
				value = min(value + 32.0 * m_sheet_main.m_sheet_scale, limit);
			}
			else if (delta > 0 && value > (limit = stbar.Minimum())) {
				value = max(value - 32.0 * m_sheet_main.m_sheet_scale, limit);
			}
			else {
				return;
			}
			stbar.Value(value);
			sheet_draw();
		}
	}

}