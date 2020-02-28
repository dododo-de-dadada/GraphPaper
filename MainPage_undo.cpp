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
	// �g����, ���ɖ߂�����Ƃ�蒼������̗������킹�����ł���.
	// �Ⴆ��, �ő�̑g���� 1 �Ɍ���ꍇ, ���ɖ߂����삩��蒼�������
	// �ǂ��炩����ɂ�������͐ς܂�Ȃ�.

	// �X�^�b�N�ɐςނ��Ƃ��ł��鑀��̍ő�̑g��.
	constexpr uint32_t U_MAX_CNT = 64;

	// ����X�^�b�N����������.
	static uint32_t clear_stack(U_STACK_T& u_stack);
	// ������f�[�^���[�_�[����ǂݍ���.
	static bool read_undo(Undo*& u, DataReader const& dt_reader);
	// ������f�[�^���[�_�[�ɏ�������.
	static void write_undo(Undo* u, DataWriter const& dt_writer);

	// ����X�^�b�N����������.
	// u_stack	����X�^�b�N
	// �߂�l	������������̑g��
	static uint32_t clear_stack(U_STACK_T& u_stack)
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

	// ������f�[�^���[�_�[����ǂݍ���.
	// u	����
	// dt_reader	�f�[�^���[�_�[
	static bool read_undo(Undo*& u, DataReader const& dt_reader)
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
	void MainPage::enable_undo_menu(void)
	{
		// ����X�^�b�N�ɐ}�`�̑I���ȊO�̑��삪
		// �ς܂�Ă��邩���ׂ�.
		bool enable_undo = false;
		for (auto u : m_stack_undo) {
			if (u == nullptr) {
				enable_undo = true;
				break;
			}
		}
		bool enable_redo = false;
		for (auto r : m_stack_redo) {
			if (r == nullptr) {
				enable_redo = true;
				break;
			}
		}
		mfi_undo().IsEnabled(enable_undo);
		mfi_redo().IsEnabled(enable_redo);
	}

	// �ҏW���j���[�́u��蒼���v���I�����ꂽ.
	void MainPage::mfi_redo_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		// �X�^�b�N����łȂ��Ԉȉ����J��Ԃ�
		// ������X�^�b�N����|�b�v����.
		// �|�b�v�����������������̃X�^�b�N�Ƀv�b�V������.
		// �|�b�v�������삪�k���̏ꍇ, �J��Ԃ��𒆒f����.
		// �t���O�𗧂Ă�.
		// �|�b�v������������s����.
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
		enable_undo_menu();
		enable_edit_menu();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		draw_page();
		if (m_summary_visible) {
			summary_update();
		}
	}

	// �ҏW���j���[�́u���ɖ߂��v���I�����ꂽ.
	void MainPage::mfi_undo_click(IInspectable const& /*sender*/, RoutedEventArgs const& /*args*/)
	{
		//	�t���O����������.
		//	�X�^�b�N����łȂ��Ԉȉ����J��Ԃ�.
		//		������X�^�b�N����s�[�N����.
		//		�s�[�N�������삪�k���̏ꍇ,
		//			���t���O�������Ă���ꍇ, �J��Ԃ��𒆒f����.
		//		�s�[�N�������삪�k���łȂ��ꍇ,
		//			�t���O�𗧂�, ��������s����.
		//		�s�[�N����������|�b�v��, ��������̃X�^�b�N�Ƀv�b�V������.
		auto flag = false;
		while (m_stack_undo.size() > 0) {
			auto u = m_stack_undo.back();
			if (u == nullptr) {
				if (flag) {
					break;
				}
			}
			else {
				flag = true;
				undo_exec(u);
			}
			m_stack_undo.pop_back();
			m_stack_redo.push_back(u);
		}
		if (flag == false) {
			return;
		}
		enable_undo_menu();
		enable_edit_menu();
		s_list_bound(m_list_shapes, m_page_panel.m_page_size, m_page_min, m_page_max);
		set_page_panle_size();
		draw_page();
		if (m_summary_visible) {
			summary_update();
		}
	}

	// ��蒼������X�^�b�N����������.
	void MainPage::redo_clear(void)
	{
		m_stack_nset -= clear_stack(m_stack_redo);
	}

	// ����X�^�b�N����������.
	void MainPage::undo_clear(void)
	{
		m_stack_push = false;
		m_stack_nset -= clear_stack(m_stack_redo);
		m_stack_nset -= clear_stack(m_stack_undo);
#if defined(_DEBUG)
		if (m_stack_nset == 0) {
			return;
		}
		winrt::Windows::UI::Xaml::Controls::ContentDialog dialog;
		dialog.Title(box_value(L"Undo is not empty."));
		dialog.CloseButtonText(L"Close");
		auto _{ dialog.ShowAsync() };
#endif
		m_stack_nset = 0;
	}

	// ��������s����.
	void MainPage::undo_exec(Undo* u)
	{
		if (m_summary_visible) {
			summary_reflect(u);
		}
		u->exec();
		auto const& u_type = typeid(*u);
		if (u_type == typeid(UndoSet<UNDO_OP::GRID_SHOW>)) {
			grid_show_check_menu(m_page_panel.m_grid_show);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::STROKE_STYLE>)) {
			stroke_style_check_menu(m_page_panel.m_stroke_style);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::ARROW_STYLE>)) {
			arrow_style_check_menu(m_page_panel.m_arrow_style);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::GRID_LEN>)) {
			stat_set_grid();
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::FONT_STYLE>)) {
			font_style_check_menu(m_page_panel.m_font_style);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::TEXT_ALIGN_T>)) {
			text_align_t_check_menu(m_page_panel.m_text_align_t);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::TEXT_ALIGN_P>)) {
			text_align_p_check_menu(m_page_panel.m_text_align_p);
		}
		else if (u_type == typeid(UndoSet<UNDO_OP::PAGE_SIZE>)) {
			stat_set_page();
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

	// ��A�̑���̋�؂�ł���k�����X�^�b�N�ɐς�.
	// ��蒼������X�^�b�N�͏��������.
	void MainPage::undo_push_null(void)
	{
		// ��蒼������X�^�b�N��������,
		// �������ꂽ����̑g����, ����̑g���������.
		// �k�������ɖ߂�����X�^�b�N�ɐς�, ����̑g���� 1 ������.
		// �k���ŋ�؂�ꂽ��A�̑���̑g�����̑g���𒴂�����,
		// �X�^�b�N�̒ꂩ���g�̑������菜��.
		// ����̑g������ 1 ������.
		m_stack_nset -= clear_stack(m_stack_redo);
		m_stack_undo.push_back(nullptr);
		m_stack_push = true;
		m_stack_nset++;
		if (m_stack_nset <= U_MAX_CNT) {
			return;
		}
		while (m_stack_undo.size() > 0) {
			const auto u = m_stack_undo.front();
			m_stack_undo.pop_front();
			if (u == nullptr) {
				break;
			}
		}
		m_stack_nset--;
		m_stack_over = true;
	}

	// �}�`�����������ړ�����, �ړ��O�̒l���X�^�b�N�ɐς�.
	void MainPage::undo_push_pos(const D2D1_POINT_2F d)
	{
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() == false) {
				continue;
			}
			undo_push_set<UNDO_OP::START_POS>(s);
			s->move(d);
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

	//	�}�`�̑I���𔽓]����, ���̑�����X�^�b�N�ɐς�.
	//	s	�I���𔽓]������}�`.
	void MainPage::undo_push_select(Shape* s)
	{
		auto it_end = m_stack_undo.rend();
		//	�t����, ����X�^�b�N�̔����q�𓾂�.
		//	�����q�̑��삪�I�[�łȂ��ꍇ, �ȉ�����������.
		auto it = m_stack_undo.rbegin();
		if (it != it_end) {
			if (*it == nullptr) {
				//	�X�^�b�N�g�b�v�̑��삪�k���̏ꍇ,
				//	�����q�����ɐi�߂�.
				it++;
			}
			//	���f�t���O����������.
			bool suspended = false;
			//	�����q�̑��삪�}�`�̑I�𑀍�̏ꍇ, �ȉ����J��Ԃ�.
			while (it != it_end && *it != nullptr && typeid(**it) == typeid(UndoSelect)) {
				if ((*it)->shape() == s) {
					//	����̐}�`���w�肳�ꂽ�}�`�ƈ�v�����ꍇ,
					//	�t���O�𗧂ĂĒ��f����.
					suspended = true;
					break;
				}
				it++;
			}
			if (suspended) {
				//	���f�t���O�������Ă���ꍇ, 
				//	����X�^�b�N������ɐ}�`�̑I���𔽓]����, 
				//	������X�^�b�N�����菜��, �I������.
				s->set_select(!s->is_selected());
				m_stack_undo.remove(*it);
				return;
			}
		}
		//	�}�`�̑I���𔽓]����, ���̑�����X�^�b�N�ɐς�.
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
		enable_undo_menu();
		enable_edit_menu();
		draw_page();
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
		Undo* r;
		while (read_undo(r, dt_reader)) {
			m_stack_redo.push_back(r);
		}
		Undo* u;
		while (read_undo(u, dt_reader)) {
			m_stack_undo.push_back(u);
		}
		m_stack_nset = dt_reader.ReadUInt32();
		m_stack_push = dt_reader.ReadBoolean();
	}

	// ����X�^�b�N���f�[�^���[�_�[�ɏ�������.
	void MainPage::undo_write(DataWriter const& dt_writer)
	{
		for (const auto& r : m_stack_redo) {
			write_undo(r, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::END));
		for (const auto& u : m_stack_undo) {
			write_undo(u, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::END));
		dt_writer.WriteUInt32(m_stack_nset);
		dt_writer.WriteBoolean(m_stack_push);
	}

	// ������f�[�^���[�_�[�ɏ�������.
	// u	����
	// dt_writer	�f�[�^���C�^�[
	static void write_undo(Undo* u, DataWriter const& dt_writer)
	{
		if (u != nullptr) {
			u->write(dt_writer);
		}
		else {
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::NULLPTR));
		}
	}

}