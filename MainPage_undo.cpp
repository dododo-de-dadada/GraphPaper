//------------------------------
// MainPage_undo.cpp
// ���ɖ߂��Ƃ�蒼��
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �k���ŋ�؂����A�̑����, ����̑g�Ƃ݂Ȃ�, ���̐���g���Ƃ���.
	// �X�^�b�N�ɐςނ��Ƃ��ł���ő�̑g��.
	constexpr uint32_t U_MAX_CNT = 32;

	// ����X�^�b�N����������.
	static uint32_t undo_clear_stack(U_STACK_T& u_stack);
	// ������f�[�^���[�_�[����ǂݍ���.
	static bool undo_read_op(Undo*& u, DataReader const& dt_reader);
	// ������f�[�^���[�_�[�ɏ�������.
	static void undo_write_op(Undo* u, DataWriter const& dt_writer);

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	// u_stack	����X�^�b�N
	// �߂�l	������������̑g��
	static uint32_t undo_clear_stack(U_STACK_T& u_stack)
	{
		uint32_t n = 0;
		for (auto u : u_stack) {
			if (u == nullptr) {
				n++;
				continue;
			}
			delete u;
		}
		u_stack.clear();
		return n;
	}

	// �}�`���O���[�v�}�`�ɒǉ�����, ���̑�����X�^�b�N�ɐς�.
	// g	�O���[�v�}�`
	// s	�ǉ�����}�`
	// �߂�l	�Ȃ�
	void MainPage::undo_push_group(ShapeGroup* g, Shape* s)
	{
		m_stack_undo.push_back(new UndoAppendG(g, s));
	}

	// ������f�[�^���[�_�[����ǂݍ���.
	// u	����
	// dt_reader	�f�[�^���[�_�[
	static bool undo_read_op(Undo*& u, DataReader const& dt_reader)
	{
		if (dt_reader.UnconsumedBufferLength() < sizeof(uint32_t)) {
			return false;
		}
		auto u_op = static_cast<UNDO_OP>(dt_reader.ReadUInt32());
		if (u_op == UNDO_OP::END) {
			return false;
		}
		switch (u_op) {
		case UNDO_OP::NULLPTR:
			u = nullptr;
			break;
		case UNDO_OP::ARRANGE:
			u = new UndoArrange2(dt_reader);
			break;
		case UNDO_OP::ARROW_SIZE:
			u = new UndoSet<UNDO_OP::ARROW_SIZE>(dt_reader);
			break;
		case UNDO_OP::ARROW_STYLE:
			u = new UndoSet<UNDO_OP::ARROW_STYLE>(dt_reader);
			break;
		case UNDO_OP::FILL_COLOR:
			u = new UndoSet<UNDO_OP::FILL_COLOR>(dt_reader);
			break;
		case UNDO_OP::FORM:
			u = new UndoForm(dt_reader);
			break;
		case UNDO_OP::FONT_COLOR:
			u = new UndoSet<UNDO_OP::FONT_COLOR>(dt_reader);
			break;
		case UNDO_OP::FONT_FAMILY:
			u = new UndoSet<UNDO_OP::FONT_FAMILY>(dt_reader);
			break;
		case UNDO_OP::FONT_SIZE:
			u = new UndoSet<UNDO_OP::FONT_SIZE>(dt_reader);
			break;
		case UNDO_OP::FONT_STYLE:
			u = new UndoSet<UNDO_OP::FONT_STYLE>(dt_reader);
			break;
		case UNDO_OP::FONT_STRETCH:
			u = new UndoSet<UNDO_OP::FONT_STRETCH>(dt_reader);
			break;
		case UNDO_OP::FONT_WEIGHT:
			u = new UndoSet<UNDO_OP::FONT_WEIGHT>(dt_reader);
			break;
		case UNDO_OP::GRID_LEN:
			u = new UndoSet<UNDO_OP::GRID_LEN>(dt_reader);
			break;
		case UNDO_OP::GRID_OPAC:
			u = new UndoSet<UNDO_OP::GRID_OPAC>(dt_reader);
			break;
		case UNDO_OP::GRID_SHOW:
			u = new UndoSet<UNDO_OP::GRID_SHOW>(dt_reader);
			break;
		case UNDO_OP::GROUP:
			u = new UndoListG(dt_reader);
			break;
		case UNDO_OP::LIST:
			u = new UndoList(dt_reader);
			break;
		case UNDO_OP::TEXT_LINE:
			u = new UndoSet<UNDO_OP::TEXT_LINE>(dt_reader);
			break;
		case UNDO_OP::TEXT_MARGIN:
			u = new UndoSet<UNDO_OP::TEXT_MARGIN>(dt_reader);
			break;
		case UNDO_OP::PAGE_COLOR:
			u = new UndoSet<UNDO_OP::PAGE_COLOR>(dt_reader);
			break;
		case UNDO_OP::PAGE_SIZE:
			u = new UndoSet<UNDO_OP::PAGE_SIZE>(dt_reader);
			break;
		case UNDO_OP::TEXT_ALIGN_P:
			u = new UndoSet<UNDO_OP::TEXT_ALIGN_P>(dt_reader);
			break;
		case UNDO_OP::SELECT:
			u = new UndoSelect(dt_reader);
			break;
		case UNDO_OP::START_POS:
			u = new UndoSet<UNDO_OP::START_POS>(dt_reader);
			break;
		case UNDO_OP::STROKE_COLOR:
			u = new UndoSet<UNDO_OP::STROKE_COLOR>(dt_reader);
			break;
		case UNDO_OP::STROKE_PATTERN:
			u = new UndoSet<UNDO_OP::STROKE_PATTERN>(dt_reader);
			break;
		case UNDO_OP::STROKE_STYLE:
			u = new UndoSet<UNDO_OP::STROKE_STYLE>(dt_reader);
			break;
		case UNDO_OP::STROKE_WIDTH:
			u = new UndoSet<UNDO_OP::STROKE_WIDTH>(dt_reader);
			break;
		case UNDO_OP::TEXT:
			u = new UndoSet<UNDO_OP::TEXT>(dt_reader);
			break;
		case UNDO_OP::TEXT_ALIGN_T:
			u = new UndoSet<UNDO_OP::TEXT_ALIGN_T>(dt_reader);
			break;
		case UNDO_OP::TEXT_RANGE:
			u = new UndoSet<UNDO_OP::TEXT_RANGE>(dt_reader);
			break;
		default:
			throw winrt::hresult_not_implemented();
			break;
		}
		return true;
	}

	// ���ɖ߂�/��蒼�����j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
	// ����X�^�b�N�Ƀk�����삪�܂܂�Ă���ꍇ, ���j�����ڂ͎g�p�ł���.
	// �܂܂�Ă���ꍇ, �k������ɂ���ċ�؂�ꂽ��A�̑��삪�X�^�b�N�Ɋ܂܂�Ă���.
	// �����łȂ��ꍇ, �}�`�̑I�𑀍�Ȃ�, ���ꂾ���ł͌��ɖ߂��K�v�̂Ȃ����삾�����܂܂�Ă���.
	void MainPage::enable_undo_menu(void)
	{
		mfi_undo().IsEnabled(m_stack_ucnt > 0);
		mfi_redo().IsEnabled(m_stack_rcnt > 0);
		//bool enable_undo = false;
		//bool enable_redo = false;
		//if (m_stack_nset > 0) {
		//	for (const auto u : m_stack_undo) {
		//		if (u == nullptr) {
		//			enable_undo = true;
		//			break;
		//		}
		//	}
		//	for (const auto r : m_stack_redo) {
		//		if (r == nullptr) {
		//			enable_redo = true;
		//			break;
		//		}
		//	}
		//}
		//mfi_undo().IsEnabled(enable_undo);
		//mfi_redo().IsEnabled(enable_redo);
	}

	// �ҏW���j���[�́u��蒼���v���I�����ꂽ.
	void MainPage::mfi_redo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_stack_rcnt == 0) {
			return;
		}
		while (m_stack_undo.empty() == false) {
			auto u = m_stack_undo.back();
			if (u == nullptr) {
				break;
			}
			m_stack_undo.pop_back();
			undo_exec(u);
		}
		auto flag = false;
		while (m_stack_redo.size() > 0) {
			auto r = m_stack_redo.back();
			m_stack_redo.pop_back();
			m_stack_undo.push_back(r);
			if (r == nullptr) {
				break;
			}
			flag = true;
			undo_exec(r);
		}
		if (flag == false) {
			// �t���O���Ȃ��ꍇ, ���f����.
			return;
		}
		m_stack_rcnt--;
		m_stack_ucnt++;
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		enable_edit_menu();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		if (m_summary_visible) {
			summary_update();
		}
	}

	// �ҏW���j���[�́u���ɖ߂��v���I�����ꂽ.
	void MainPage::mfi_undo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_stack_ucnt == 0) {
			return;
		}
		auto st = 0;
		while (m_stack_undo.empty() == false) {
			auto u = m_stack_undo.back();
			if (st == 0) {
				m_stack_undo.pop_back();
				if (u != nullptr) {
					undo_exec(u);
				}
				else {
					m_stack_redo.push_back(nullptr);
					st = 1;
				}
			}
			else if (st == 1) {
				if (u != nullptr) {
					m_stack_undo.pop_back();
					undo_exec(u);
					m_stack_redo.push_back(u);
				}
				else {
					break;
				}
			}
		}
		if (st != 0) {
			m_stack_ucnt--;
			m_stack_rcnt++;
		}
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		enable_edit_menu();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		page_draw();
		if (m_summary_visible) {
			summary_update();
		}
	}

	// ��蒼������X�^�b�N��������, �܂܂�鑀���j������.
	/*
	void MainPage::redo_clear(void)
	{
		//m_stack_nset -= undo_clear_stack(m_stack_redo);
		m_stack_rcnt -= undo_clear_stack(m_stack_redo);
	}
	*/

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	void MainPage::undo_clear(void)
	{
		m_stack_push = false;
		//m_stack_nset -= undo_clear_stack(m_stack_redo);
		//m_stack_nset -= undo_clear_stack(m_stack_undo);
		m_stack_rcnt -= undo_clear_stack(m_stack_redo);
		m_stack_ucnt -= undo_clear_stack(m_stack_undo);
#if defined(_DEBUG)
		if (m_stack_rcnt == 0 && m_stack_ucnt == 0) {
			return;
		}
		//if (m_stack_nset == 0) {
		//	return;
		//}
		winrt::Windows::UI::Xaml::Controls::ContentDialog dialog;
		dialog.Title(box_value(L"Undo is not empty."));
		dialog.CloseButtonText(L"Close");
		auto _{ dialog.ShowAsync() };
#endif
		//m_stack_nset = 0;
	}

	// ��������s����.
	void MainPage::undo_exec(Undo* u)
	{
		summary_reflect(u);
		u->exec();
		auto const& u_type = typeid(*u);
		if (u_type == typeid(UndoSet<UNDO_OP::ARROW_STYLE>)) {
			// ���g���j���[�́u���̎�ށv�Ɉ������.
			arrow_style_check_menu(m_page_panel.m_arrow_style);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::GRID_LEN>)) {
			// ����̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			status_set_grid();
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::GRID_SHOW>)) {
			// �y�[�W���j���[�́u����̕\���v�Ɉ������.
			grid_show_check_menu(m_page_panel.m_grid_show);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::FONT_STYLE>)) {
			// ���̃��j���[�́u���́v�Ɉ������.
			font_style_check_menu(m_page_panel.m_font_style);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::PAGE_SIZE>)) {
			// �y�[�W�̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			status_set_page();
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::STROKE_STYLE>)) {
			// ���g���j���[�́u��ށv�Ɉ������.
			stroke_style_check_menu(m_page_panel.m_stroke_style);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::TEXT_ALIGN_T>)) {
			text_align_t_check_menu(m_page_panel.m_text_align_t);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::TEXT_ALIGN_P>)) {
			text_align_p_check_menu(m_page_panel.m_text_align_p);
		}
	}

	// �����ȑ���̑g���|�b�v����.
	// �߂�l	�|�b�v���ꂽ�ꍇ true
	bool MainPage::undo_pop_if_invalid(void)
	{
		while (m_stack_undo.size() > 0) {
			auto u = m_stack_undo.back();
			if (u == nullptr) {
				return true;
			}
			if (u->changed()) {
				break;
			}
			m_stack_undo.pop_back();
		}
		return false;
	}

	// �}�`��ǉ�����, ���̑�����X�^�b�N�ɐς�.
	void MainPage::undo_push_append(Shape* s)
	{
		m_stack_undo.push_back(new UndoAppend(s));
	}

	// �}�`�����ւ���, ���̑�����X�^�b�N�ɐς�.
	void MainPage::undo_push_arrange(Shape* s, Shape* t)
	{
		m_stack_undo.push_back(new UndoArrange2(s, t));
	}

	// �}�`�̕��ʂ̈ʒu��ύX����, �ύX�O�̒l���X�^�b�N�ɐς�.
	void MainPage::undo_push_form(Shape* s, const ANCH_WHICH a, const D2D1_POINT_2F a_pos)
	{
		m_stack_undo.push_back(new UndoForm(s, a));
		s->set_pos(a_pos, a);
	}

	// �}�`��}������, ���̑�����X�^�b�N�ɐς�.
	void MainPage::undo_push_insert(Shape* s, Shape* s_pos)
	{
		m_stack_undo.push_back(new UndoInsert(s, s_pos));
	}

	// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
	// ��蒼������X�^�b�N�͏��������.
	void MainPage::undo_push_null(void)
	{
		// ��蒼������X�^�b�N��������, �������ꂽ����̑g����, ����̑g���������.
		//m_stack_nset -= undo_clear_stack(m_stack_redo);
		m_stack_rcnt -= undo_clear_stack(m_stack_redo);
		// ���ɖ߂�����X�^�b�N�Ƀk����ς�.
		m_stack_undo.push_back(nullptr);
		// ����X�^�b�N�̍X�V�t���O�𗧂Ă�.
		m_stack_push = true;
		// ����̑g�����C���N�������g����.
		//m_stack_nset++;
		m_stack_ucnt++;
		if (m_stack_ucnt <= U_MAX_CNT) {
		//if (m_stack_nset <= U_MAX_CNT) {
			// ����̑g�����ő�̑g���ȉ��̏ꍇ,
			// �I������.
			return;
		}
		while (m_stack_undo.empty() == false) {
			// ���ɖ߂�����X�^�b�N����łȂ��ꍇ,
			// ����X�^�b�N�̈�Ԓ�̑�������o��.
			const auto u = m_stack_undo.front();
			m_stack_undo.pop_front();
			if (u == nullptr) {
				// ���삪�k���̏ꍇ,
				// ���f����.
				break;
			}
		}
		// ����̑g���� 1 ������.
		//m_stack_nset--;
		m_stack_ucnt--;
	}

	// �}�`�����������ړ�����, �ړ��O�̒l���X�^�b�N�ɐς�.
	// d_pos	�ړ������鍷��
	void MainPage::undo_push_pos(const D2D1_POINT_2F d_pos)
	{
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() == false) {
				continue;
			}
			undo_push_set<UNDO_OP::START_POS>(s);
			s->move(d_pos);
		}
	}

	// �}�`���O���[�v����폜����, ���̑�����X�^�b�N�ɐς�.
	void MainPage::undo_push_remove(Shape* g, Shape* s)
	{
		m_stack_undo.push_back(new UndoRemoveG(g, s));
	}

	// �}�`���폜����, ���̑�����X�^�b�N�ɐς�.
	void MainPage::undo_push_remove(Shape* s)
	{
		m_stack_undo.push_back(new UndoRemove(s));
	}

	// �}�`�̑I���𔽓]����, ���̑�����X�^�b�N�ɐς�.
	// s	�I���𔽓]������}�`.
	void MainPage::undo_push_select(Shape* s)
	{
		auto it_end = m_stack_undo.rend();
		// �t����, ����X�^�b�N�̔����q�𓾂�.
		// �����q�̑��삪�I�[�łȂ��ꍇ, �ȉ�����������.
		auto it = m_stack_undo.rbegin();
		if (it != it_end) {
			if (*it == nullptr) {
				// �X�^�b�N�g�b�v�̑��삪�k���̏ꍇ,
				// �����q�����ɐi�߂�.
				it++;
			}
			// ���f�t���O����������.
			bool suspended = false;
			// �����q�̑��삪�}�`�̑I�𑀍�̏ꍇ, �ȉ����J��Ԃ�.
			while (it != it_end && *it != nullptr && typeid(**it) == typeid(UndoSelect)) {
				if ((*it)->shape() == s) {
					// ����̐}�`���w�肳�ꂽ�}�`�ƈ�v�����ꍇ,
					// �t���O�𗧂ĂĒ��f����.
					suspended = true;
					break;
				}
				it++;
			}
			if (suspended) {
				// ���f�t���O�������Ă���ꍇ, 
				// ����X�^�b�N������ɐ}�`�̑I���𔽓]����, 
				// ������X�^�b�N�����菜��, �I������.
				s->set_select(!s->is_selected());
				m_stack_undo.remove(*it);
				return;
			}
		}
		// �}�`�̑I���𔽓]����, ���̑�����X�^�b�N�ɐς�.
		m_stack_undo.push_back(new UndoSelect(s));
	}

	// �l��}�`�֊i�[����, ���̑�����X�^�b�N�ɐς�.
	template <UNDO_OP U, typename T>
	void MainPage::undo_push_set(Shape* s, T const& val)
	{
		m_stack_undo.push_back(new UndoSet<U>(s, val));
	}

	template void MainPage::undo_push_set<UNDO_OP::GRID_LEN>(Shape* s, double const& val);
	template void MainPage::undo_push_set<UNDO_OP::GRID_OPAC>(Shape* s, double const& val);
	template void MainPage::undo_push_set<UNDO_OP::GRID_SHOW>(Shape* s, GRID_SHOW const& val);
	template void MainPage::undo_push_set<UNDO_OP::PAGE_COLOR>(Shape* s, D2D1_COLOR_F const& val);
	template void MainPage::undo_push_set<UNDO_OP::PAGE_SIZE>(Shape* s, D2D1_SIZE_F const& val);
	template void MainPage::undo_push_set<UNDO_OP::START_POS>(Shape* s);
	template void MainPage::undo_push_set<UNDO_OP::TEXT_RANGE>(Shape* s, DWRITE_TEXT_RANGE const& val);
	template void MainPage::undo_push_set<UNDO_OP::TEXT>(Shape* s, wchar_t* const& val);

	// �}�`�̒l�̕ۑ������s����, ���̑�����X�^�b�N�ɐς�.
	template <UNDO_OP U>
	void MainPage::undo_push_set(Shape* s)
	{
		m_stack_undo.push_back(new UndoSet<U>(s));
	}

	// �l��}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
	// U	����̎��.
	// T	�i�[����^.
	// val	�i�[����l
	// �i�[����^ T �͖������Ȃ��Ă������̌^���琄��ł���
	template<UNDO_OP U, typename T>
	void MainPage::undo_push_value(T const& val)
	{
		m_stack_undo.push_back(new UndoSet<U>(&m_page_panel, val));
		auto flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() == false) {
				continue;
			}
			T old_val{};
			if (UndoSet<U>::GET(s, old_val) == false) {
				continue;
			}
			if (equal(old_val, val)) {
				continue;
			}
			m_stack_undo.push_back(new UndoSet<U>(s, val));
			flag = true;
		}
		if (flag == false) {
			return;
		}
		undo_push_null();
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		enable_edit_menu();
		page_draw();
	}

	template void MainPage::undo_push_value<UNDO_OP::ARROW_SIZE>(ARROW_SIZE const& val);
	template void MainPage::undo_push_value<UNDO_OP::ARROW_STYLE>(ARROW_STYLE const& val);
	template void MainPage::undo_push_value<UNDO_OP::FILL_COLOR>(D2D1_COLOR_F const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_COLOR>(D2D1_COLOR_F const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_FAMILY>(wchar_t* const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_SIZE>(double const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_STRETCH>(DWRITE_FONT_STRETCH const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE const& val);
	template void MainPage::undo_push_value<UNDO_OP::FONT_WEIGHT>(DWRITE_FONT_WEIGHT const& val);
	template void MainPage::undo_push_value<UNDO_OP::STROKE_COLOR>(D2D1_COLOR_F const& val);
	template void MainPage::undo_push_value<UNDO_OP::STROKE_PATTERN>(STROKE_PATTERN const& val);
	template void MainPage::undo_push_value<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE const& val);
	template void MainPage::undo_push_value<UNDO_OP::STROKE_WIDTH>(double const& val);
	template void MainPage::undo_push_value<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT const& val);
	template void MainPage::undo_push_value<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT const& val);
	template void MainPage::undo_push_value<UNDO_OP::TEXT_LINE>(double const& val);
	template void MainPage::undo_push_value<UNDO_OP::TEXT_MARGIN>(D2D1_SIZE_F const& val);

	// ����X�^�b�N���f�[�^���[�_�[����ǂݍ���.
	void MainPage::undo_read(DataReader const& dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		Undo* r;
		while (undo_read_op(r, dt_reader)) {
			m_stack_redo.push_back(r);
		}
		m_stack_rcnt = dt_reader.ReadUInt32();
		Undo* u;
		while (undo_read_op(u, dt_reader)) {
			m_stack_undo.push_back(u);
		}
		//m_stack_nset = dt_reader.ReadUInt32();
		m_stack_ucnt = dt_reader.ReadUInt32();
		m_stack_push = dt_reader.ReadBoolean();
	}

	// ����X�^�b�N���f�[�^���[�_�[�ɏ�������.
	void MainPage::undo_write(DataWriter const& dt_writer)
	{
		for (const auto& r : m_stack_redo) {
			undo_write_op(r, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::END));
		dt_writer.WriteUInt32(m_stack_rcnt);
		for (const auto& u : m_stack_undo) {
			undo_write_op(u, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::END));
		//dt_writer.WriteUInt32(m_stack_nset);
		dt_writer.WriteUInt32(m_stack_ucnt);
		dt_writer.WriteBoolean(m_stack_push);
	}

	// ������f�[�^���[�_�[�ɏ�������.
	// u	����
	// dt_writer	�f�[�^���C�^�[
	static void undo_write_op(Undo* u, DataWriter const& dt_writer)
	{
		if (u != nullptr) {
			u->write(dt_writer);
		}
		else {
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::NULLPTR));
		}
	}

}