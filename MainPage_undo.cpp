//------------------------------
// MainPage_undo.cpp
// ���ɖ߂��Ƃ�蒼������X�^�b�N
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �k���ŋ�؂����A�̑����, ����̑g�Ƃ݂Ȃ�, ���̐���g���Ƃ���.
	// �X�^�b�N�ɐςނ��Ƃ��ł���ő�̑g��.
	constexpr uint32_t MAX_UCNT = 32;

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	static uint32_t undo_clear_stack(UNDO_STACK& ustack);
	// �f�[�^���[�_�[���瑀���ǂݍ���.
	static bool undo_read_op(Undo*& u, DataReader const& dt_reader);
	// ������f�[�^���[�_�[�ɏ�������.
	static void undo_write_op(Undo* u, DataWriter const& dt_writer);

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	// ustack	����X�^�b�N
	// �߂�l	������������̑g��
	static uint32_t undo_clear_stack(UNDO_STACK& ustack)
	{
		uint32_t n = 0;
		for (auto u : ustack) {
			if (u == nullptr) {
				n++;
				continue;
			}
			delete u;
		}
		ustack.clear();
		return n;
	}

	// �f�[�^���[�_�[���瑀���ǂݍ���.
	// u	����
	// dt_reader	�f�[�^���[�_�[
	static bool undo_read_op(Undo*& u, DataReader const& dt_reader)
	{
		if (dt_reader.UnconsumedBufferLength() < sizeof(uint32_t)) {
			return false;
		}
		auto u_op = static_cast<UNDO_T>(dt_reader.ReadUInt32());
		if (u_op == UNDO_T::END) {
			return false;
		}
		switch (u_op) {
		case UNDO_T::NIL:
			u = nullptr;
			break;
		case UNDO_T::ARC_DIR:
			u = new UndoValue<UNDO_T::ARC_DIR>(dt_reader);
			break;
		case UNDO_T::ARC_END:
			u = new UndoValue<UNDO_T::ARC_END>(dt_reader);
			break;
		case UNDO_T::ARC_ROT:
			u = new UndoValue<UNDO_T::ARC_ROT>(dt_reader);
			break;
		case UNDO_T::ARC_START:
			u = new UndoValue<UNDO_T::ARC_START>(dt_reader);
			break;
		case UNDO_T::DEFORM:
			u = new UndoDeform(dt_reader);
			break;
		case UNDO_T::ORDER:
			u = new UndoOrder(dt_reader);
			break;
		case UNDO_T::ARROW_SIZE:
			u = new UndoValue<UNDO_T::ARROW_SIZE>(dt_reader);
			break;
		case UNDO_T::ARROW_STYLE:
			u = new UndoValue<UNDO_T::ARROW_STYLE>(dt_reader);
			break;
		case UNDO_T::IMAGE:
			u = new UndoImage(dt_reader);
			break;
		case UNDO_T::IMAGE_OPAC:
			u = new UndoValue<UNDO_T::IMAGE_OPAC>(dt_reader);
			break;
		case UNDO_T::FILL_COLOR:
			u = new UndoValue<UNDO_T::FILL_COLOR>(dt_reader);
			break;
		case UNDO_T::FONT_COLOR:
			u = new UndoValue<UNDO_T::FONT_COLOR>(dt_reader);
			break;
		case UNDO_T::FONT_FAMILY:
			u = new UndoValue<UNDO_T::FONT_FAMILY>(dt_reader);
			break;
		case UNDO_T::FONT_SIZE:
			u = new UndoValue<UNDO_T::FONT_SIZE>(dt_reader);
			break;
		case UNDO_T::FONT_STYLE:
			u = new UndoValue<UNDO_T::FONT_STYLE>(dt_reader);
			break;
		case UNDO_T::FONT_STRETCH:
			u = new UndoValue<UNDO_T::FONT_STRETCH>(dt_reader);
			break;
		case UNDO_T::FONT_WEIGHT:
			u = new UndoValue<UNDO_T::FONT_WEIGHT>(dt_reader);
			break;
		case UNDO_T::GRID_BASE:
			u = new UndoValue<UNDO_T::GRID_BASE>(dt_reader);
			break;
		case UNDO_T::GRID_COLOR:
			u = new UndoValue<UNDO_T::GRID_COLOR>(dt_reader);
			break;
		case UNDO_T::GRID_EMPH:
			u = new UndoValue<UNDO_T::GRID_EMPH>(dt_reader);
			break;
		case UNDO_T::GRID_SHOW:
			u = new UndoValue<UNDO_T::GRID_SHOW>(dt_reader);
			break;
		case UNDO_T::GROUP:
			u = new UndoGroup(dt_reader);
			break;
		case UNDO_T::LIST:
			u = new UndoList(dt_reader);
			break;
		case UNDO_T::SELECT:
			u = new UndoSelect(dt_reader);
			break;
		case UNDO_T::SHEET_COLOR:
			u = new UndoValue<UNDO_T::SHEET_COLOR>(dt_reader);
			break;
		case UNDO_T::SHEET_SIZE:
			u = new UndoValue<UNDO_T::SHEET_SIZE>(dt_reader);
			break;
		case UNDO_T::SHEET_PAD:
			u = new UndoValue<UNDO_T::SHEET_PAD>(dt_reader);
			break;
		case UNDO_T::MOVE:
			u = new UndoValue<UNDO_T::MOVE>(dt_reader);
			break;
		case UNDO_T::STROKE_CAP:
			u = new UndoValue<UNDO_T::STROKE_CAP>(dt_reader);
			break;
		case UNDO_T::STROKE_COLOR:
			u = new UndoValue<UNDO_T::STROKE_COLOR>(dt_reader);
			break;
		//case UNDO_T::DASH_CAP:
		//	u = new UndoValue<UNDO_T::DASH_CAP>(dt_reader);
		//	break;
		case UNDO_T::DASH_PAT:
			u = new UndoValue<UNDO_T::DASH_PAT>(dt_reader);
			break;
		case UNDO_T::DASH_STYLE:
			u = new UndoValue<UNDO_T::DASH_STYLE>(dt_reader);
			break;
		case UNDO_T::JOIN_LIMIT:
			u = new UndoValue<UNDO_T::JOIN_LIMIT>(dt_reader);
			break;
		case UNDO_T::JOIN_STYLE:
			u = new UndoValue<UNDO_T::JOIN_STYLE>(dt_reader);
			break;
		case UNDO_T::STROKE_WIDTH:
			u = new UndoValue<UNDO_T::STROKE_WIDTH>(dt_reader);
			break;
		case UNDO_T::TEXT_ALIGN_P:
			u = new UndoValue<UNDO_T::TEXT_ALIGN_P>(dt_reader);
			break;
		case UNDO_T::TEXT_ALIGN_T:
			u = new UndoValue<UNDO_T::TEXT_ALIGN_T>(dt_reader);
			break;
		case UNDO_T::TEXT_CONTENT:
			u = new UndoValue<UNDO_T::TEXT_CONTENT>(dt_reader);
			break;
		case UNDO_T::TEXT_LINE_SP:
			u = new UndoValue<UNDO_T::TEXT_LINE_SP>(dt_reader);
			break;
		case UNDO_T::TEXT_PAD:
			u = new UndoValue<UNDO_T::TEXT_PAD>(dt_reader);
			break;
		case UNDO_T::TEXT_WRAP:
			u = new UndoValue<UNDO_T::TEXT_WRAP>(dt_reader);
			break;
		case UNDO_T::TEXT_SELECT:
			u = new UndoTextSelect(dt_reader);
			break;
		default:
			throw winrt::hresult_invalid_argument();
			break;
		}
		return true;
	}

	// ������f�[�^���[�_�[�ɏ�������.
	// u	����
	// dt_writer	�o�͐�
	static void undo_write_op(Undo* u, DataWriter const& dt_writer)
	{
		if (u != nullptr) {
			u->write(dt_writer);
		}
		else {
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::NIL));
		}
	}

	// �ҏW���j���[�́u��蒼���v���I�����ꂽ.
	void MainPage::redo_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �ŏ����󑀍�Ȃ炻�������̂���.
		while (m_redo_stack.size() > 0 && m_redo_stack.back() == nullptr) {
			m_redo_stack.pop_back();
		}
		bool pushed = false;
		Undo* u;
		while (m_redo_stack.size() > 0 && (u = m_redo_stack.back()) != nullptr) {
			summary_reflect(u);
			undo_exec(u);
			m_redo_stack.pop_back();
			if (!pushed) {
				if (m_undo_stack.size() > 0) {
					m_undo_stack.push_back(nullptr);
				}
				pushed = true;
			}
			m_undo_stack.push_back(u);
		}
		if (pushed) {
			main_bbox_update();
			main_panel_size();
			main_draw();
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_update();
			}
		}
		status_bar_set_pos();
	}

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	void MainPage::undo_clear(void)
	{
		//m_ustack_is_changed = false;
		undo_clear_stack(m_redo_stack);
		undo_clear_stack(m_undo_stack);
	}

	// �ҏW���j���[�́u���ɖ߂��v���I�����ꂽ.
	void MainPage::undo_click(IInspectable const&, RoutedEventArgs const&)
	{
		// �ŏ����󑀍�Ȃ炻�������̂���.
		while (m_undo_stack.size() > 0 && m_undo_stack.back() == nullptr) {
			m_undo_stack.pop_back();
		}
		bool pushed = false;
		Undo* u;
		while (m_undo_stack.size() > 0 && (u = m_undo_stack.back()) != nullptr) {
			summary_reflect(u);
			undo_exec(u);
			m_undo_stack.pop_back();
			if (!pushed) {
				if (m_redo_stack.size() > 0) {
					m_redo_stack.push_back(nullptr);
				}
				pushed = true;
			}
			m_redo_stack.push_back(u);
		}
		if (pushed) {
			main_bbox_update();
			main_panel_size();
			main_draw();
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_update();
			}
		}
		status_bar_set_pos();
	}

	// ��������s����.
	// u	����
	void MainPage::undo_exec(Undo* u)
	{
		u->exec();
		auto const& u_type = typeid(*u);
		if (u_type == typeid(UndoSelect)) {
			if (u->m_shape->is_selected()) {
				//undo_selected_cnt<true>(u->m_shape);
				m_undo_select_cnt++;
			}
			else {
				//undo_selected_cnt<false>(u->m_shape);
				m_undo_select_cnt--;
			}
#ifdef _DEBUG
			debug_cnt();
#endif
		}
		else if (u_type == typeid(UndoList)) {
			const auto s = u->m_shape;
			// �}�`���폜���ꂽ.
			if (static_cast<UndoList*>(u)->m_insert) {
				// �I�����ꂽ���� 1 ���炷.
				if (s->is_selected()) {
					//undo_selected_cnt<true>(s);
					m_undo_select_cnt++;
				}
				m_undo_undeleted_cnt--;
#ifdef _DEBUG
				debug_cnt();
#endif
			}
			// �}�`���ǉ����ꂽ.
			else {
				if (s->is_selected()) {
					//undo_selected_cnt<true>(s);
					m_undo_select_cnt++;
				}
				m_undo_undeleted_cnt++;
#ifdef _DEBUG
				debug_cnt();
#endif
			}
		}
		else if (u_type == typeid(UndoValue<UNDO_T::ARROW_STYLE>)) {
			ARROW_STYLE val;
			m_main_sheet.get_arrow_style(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::FONT_STYLE>)) {
			// ���̃��j���[�́u���́v�Ɉ������.
			DWRITE_FONT_STYLE val;
			m_main_sheet.get_font_style(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::FONT_WEIGHT>)) {
			// ���̃��j���[�́u���́v�Ɉ������.
			DWRITE_FONT_WEIGHT val;
			m_main_sheet.get_font_weight(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::FONT_STRETCH>)) {
			// ���̃��j���[�́u���́v�Ɉ������.
			DWRITE_FONT_STRETCH val;
			m_main_sheet.get_font_stretch(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::GRID_BASE>)) {
			// ����̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			status_bar_set_grid();
		}
		else if (u_type == typeid(UndoValue<UNDO_T::GRID_EMPH>)) {
			// ���C�A�E�g���j���[�́u����̋����v�Ɉ������.
			GRID_EMPH val;
			m_main_sheet.get_grid_emph(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::GRID_SHOW>)) {
			GRID_SHOW val;
			m_main_sheet.get_grid_show(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::SHEET_SIZE>)) {
			// �y�[�W�̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			status_bar_set_sheet();
		}
		else if (u_type == typeid(UndoValue<UNDO_T::STROKE_CAP>)) {
			D2D1_CAP_STYLE val;
			m_main_sheet.get_stroke_cap(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::DASH_STYLE>)) {
			D2D1_DASH_STYLE val;
			m_main_sheet.get_stroke_dash(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::JOIN_STYLE>)) {
			D2D1_LINE_JOIN val;
			m_main_sheet.get_stroke_join(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::STROKE_WIDTH>)) {
			float val;
			m_main_sheet.get_stroke_width(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::TEXT_ALIGN_T>)) {
			DWRITE_TEXT_ALIGNMENT val;
			m_main_sheet.get_text_align_horz(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::TEXT_ALIGN_P>)) {
			DWRITE_PARAGRAPH_ALIGNMENT val;
			m_main_sheet.get_text_align_vert(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::TEXT_WRAP>)) {
			DWRITE_WORD_WRAPPING val;
			m_main_sheet.get_text_wrap(val);
		}
	}

	// �����ȑ�����|�b�v����.
	// �߂�l	���삪�ЂƂȏ�|�b�v���ꂽ�Ȃ� true
	bool MainPage::undo_pop_invalid(void)
	{
		while (m_undo_stack.size() > 0) {
			auto u = m_undo_stack.back();
			if (u == nullptr) {
				return true;
			}
			if (u->changed()) {
				break;
			}
			m_undo_stack.pop_back();
		}
		return false;
	}

	// �I�����ꂽ (���邢�͑S�Ă�) �}�`�̈ʒu���X�^�b�N�ɕۑ����Ă��獷�������ړ�����.
	// to	�ړ���ւ̃x�N�g��
	// any_shape	�}�`���ׂĂ̏ꍇ true, �I�����ꂽ�}�`�݂̂̏ꍇ false
	void MainPage::undo_push_move(const D2D1_POINT_2F to, const bool any_shape)
	{
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted() || (!any_shape && !s->is_selected())) {
				continue;
			}
			undo_push_set<UNDO_T::MOVE>(s);
			s->move(to);
		}
	}

	// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
	// ��蒼������X�^�b�N�͏��������.
	void MainPage::undo_push_null(void)
	{		
		// ��蒼������X�^�b�N��������, �������ꂽ����̑g����, ����̑g���������.
		undo_clear_stack(m_redo_stack);
		if (m_undo_stack.size() > 0 && m_undo_stack.back() != nullptr) {
			m_undo_stack.push_back(nullptr);
		}
		if (m_undo_stack.size() > 1024) {
			// 1024 �𒴂��鑀��͖ⓚ���p�ŃX�^�b�N�̐擪������̂����č폜����.
			while (m_undo_stack.size() > 1024) {
				Undo* u = m_undo_stack.front();
				m_undo_stack.pop_front();
				delete u;
			}
			// ��ȊO�̑�����X�^�b�N�̐擪������̂����č폜����.
			Undo* u;
			while (!m_undo_stack.empty() && (u = m_undo_stack.front()) != nullptr) {
				m_undo_stack.pop_front();
				delete u;
			}
			// �󑀍����菜���č폜����.
			if (!m_undo_stack.empty()) {
				m_undo_stack.pop_front();
			}
		}
		// true ���X�^�b�N���X�V���ꂽ������Ɋi�[����.
		//m_ustack_is_changed = true;
	}


	//template<bool I> void MainPage::undo_selected_cnt(Shape* s)
	//{
	//	if constexpr (I) m_undo_select_cnt++; else m_undo_select_cnt--;
	//	if (s->exist_cap()) {
	//		if constexpr (I) m_undo_selected_exist_cap++; else m_undo_selected_exist_cap--;
	//	}
	//	if (typeid(*s) == typeid(ShapeGroup)) {
	//		if constexpr (I) m_undo_selected_group++; else m_undo_selected_group--;
	//	}
	//	else if (typeid(*s) == typeid(ShapeText)) {
	//		if constexpr (I) m_undo_selected_text++; else m_undo_selected_text--;
	//	}
	//	else if (typeid(*s) == typeid(ShapeLine)) {
	//		if constexpr (I) m_undo_selected_line++; else m_undo_selected_line--;
	//	}
	//	else if (typeid(*s) == typeid(ShapeImage)) {
	//		if constexpr (I) m_undo_selected_image++; else m_undo_selected_image--;
	//	}
	//	else if (typeid(*s) == typeid(ShapeRuler)) {
	//		if constexpr (I) m_undo_selected_ruler++; else m_undo_selected_ruler--;
	//	}
	//	else if (typeid(*s) == typeid(ShapeArc)) {
	//		if constexpr (I) m_undo_selected_arc++; else m_undo_selected_arc--;
	//	}
	//	else if (typeid(*s) == typeid(ShapePoly)) {
	//		if (s->exist_cap()) {
	//			if constexpr (I) m_undo_selected_polyline++; else m_undo_selected_polyline--;
	//		}
	//		else {
	//			if constexpr (I) m_undo_selected_polygon++; else m_undo_selected_polygon--;
	//		}
	//	}
	//}

	// �}�`�̑I���𔽓]����, ���̑�����X�^�b�N�ɐς�.
	// s	�I���𔽓]������}�`.
	void MainPage::undo_push_select(Shape* s)
	{
		const auto it_end = m_undo_stack.rend();
		Undo* u;
		for (auto it = m_undo_stack.rbegin();
			it != it_end && (u = *it) != nullptr && typeid(*u) == typeid(UndoSelect); it++) {
			if (u->shape() == s) {
				// ����̐}�`���w�肳�ꂽ�}�`�ƈ�v�����ꍇ,
				// ����X�^�b�N������ɐ}�`�̑I���𔽓]����, 
				// �ȑO�̑�����X�^�b�N�����菜��, �I������.
				if (s->is_selected()) {
					s->set_select(false);
					//undo_selected_cnt<false>(s);
					m_undo_select_cnt--;
				}
				else {
					s->set_select(true);
					//undo_selected_cnt<true>(s);
					m_undo_select_cnt++;
				}
#ifdef _DEBUG
				debug_cnt();
#endif
				m_undo_stack.remove(u);
				return;
			}
		}
		// �}�`�̑I���𔽓]����, ���̑�����X�^�b�N�ɐς�.
		m_undo_stack.push_back(new UndoSelect(s));
		if (s->is_selected()) {
			//undo_selected_cnt<true>(s);
			m_undo_select_cnt++;
		}
		else {
			//undo_selected_cnt<false>(s);
			m_undo_select_cnt--;
		}
#ifdef _DEBUG
		debug_cnt();
#endif
	}

	// �l���w�肷��}�`�֊i�[��, ���̑�����X�^�b�N�ɐς�.
	// U	����̎��
	// T	�l�̌^
	// s	�}�`
	// val	�l
	template <UNDO_T U, typename T> void MainPage::undo_push_set(Shape* const s, T const& val)
	{
		// �}�`�����̒l�������Ȃ�, �܂��͓��l�Ȃ璆�f����.
		T old_val;
		if (!UndoValue<U>::GET(s, old_val) || equal(val, old_val)) {
			return;
		}
		// ����ȊO�Ȃ�, �l���i�[���Ă��̑�����X�^�b�N�ɐς�.
		m_undo_stack.push_back(new UndoValue<U>(s, val));
	}

	template void MainPage::menu_is_checked<UNDO_T::NIL>(void);

	template<UNDO_T U> void MainPage::menu_is_checked(void)
	{
		if constexpr (U == UNDO_T::DASH_STYLE || U == UNDO_T::NIL) {
			if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID) {
				rmfi_popup_stroke_dash_solid().IsChecked(true);
				rmfi_menu_stroke_dash_solid().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(false);
				mfi_menu_stroke_dash_pat().IsEnabled(false);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH) {
				rmfi_popup_stroke_dash_dash().IsChecked(true);
				rmfi_menu_stroke_dash_dash().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
				rmfi_popup_stroke_dash_dot().IsChecked(true);
				rmfi_menu_stroke_dash_dot().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT) {
				rmfi_popup_stroke_dash_dash_dot().IsChecked(true);
				rmfi_menu_stroke_dash_dash_dot().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
			else if (m_main_sheet.m_stroke_dash == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DASH_DOT_DOT) {
				rmfi_popup_stroke_dash_dash_dot_dot().IsChecked(true);
				rmfi_menu_stroke_dash_dash_dot_dot().IsChecked(true);
				mfi_popup_stroke_dash_pat().IsEnabled(true);
				mfi_menu_stroke_dash_pat().IsEnabled(true);
			}
		}
		else if constexpr (U == UNDO_T::STROKE_WIDTH || U == UNDO_T::NIL) {
			if (equal(m_main_sheet.m_stroke_width, 0.0f)) {
				rmfi_popup_stroke_width_0px().IsChecked(true);
				rmfi_menu_stroke_width_0px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 1.0f)) {
				rmfi_popup_stroke_width_1px().IsChecked(true);
				rmfi_menu_stroke_width_1px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 2.0f)) {
				rmfi_popup_stroke_width_2px().IsChecked(true);
				rmfi_menu_stroke_width_2px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 3.0f)) {
				rmfi_popup_stroke_width_3px().IsChecked(true);
				rmfi_menu_stroke_width_3px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 4.0f)) {
				rmfi_popup_stroke_width_4px().IsChecked(true);
				rmfi_menu_stroke_width_4px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 8.0f)) {
				rmfi_popup_stroke_width_8px().IsChecked(true);
				rmfi_menu_stroke_width_8px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 12.0f)) {
				rmfi_popup_stroke_width_12px().IsChecked(true);
				rmfi_menu_stroke_width_12px().IsChecked(true);
			}
			else if (equal(m_main_sheet.m_stroke_width, 16.0f)) {
				rmfi_popup_stroke_width_16px().IsChecked(true);
				rmfi_menu_stroke_width_16px().IsChecked(true);
			}
			else {
				rmfi_popup_stroke_width_other().IsChecked(true);
				rmfi_menu_stroke_width_other().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::STROKE_CAP || U == UNDO_T::NIL) {
			if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT) {
				rmfi_popup_stroke_cap_flat().IsChecked(true);
				rmfi_menu_stroke_cap_flat().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
				rmfi_popup_stroke_cap_square().IsChecked(true);
				rmfi_menu_stroke_cap_square().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND) {
				rmfi_popup_stroke_cap_round().IsChecked(true);
				rmfi_menu_stroke_cap_round().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_cap == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				rmfi_popup_stroke_cap_triangle().IsChecked(true);
				rmfi_menu_stroke_cap_triangle().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::ARROW_STYLE || U == UNDO_T::NIL) {
			if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_NONE) {
				rmfi_popup_stroke_arrow_none().IsChecked(true);
				rmfi_menu_stroke_arrow_none().IsChecked(true);
				mfi_popup_stroke_arrow_size().IsEnabled(false);
				mfi_menu_stroke_arrow_size().IsEnabled(false);
			}
			else if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_OPENED) {
				rmfi_popup_stroke_arrow_opened().IsChecked(true);
				rmfi_menu_stroke_arrow_opened().IsChecked(true);
				mfi_popup_stroke_arrow_size().IsEnabled(true);
				mfi_menu_stroke_arrow_size().IsEnabled(true);
			}
			else if (m_main_sheet.m_arrow_style == ARROW_STYLE::ARROW_FILLED) {
				rmfi_popup_stroke_arrow_filled().IsChecked(true);
				rmfi_menu_stroke_arrow_filled().IsChecked(true);
				mfi_popup_stroke_arrow_size().IsEnabled(true);
				mfi_menu_stroke_arrow_size().IsEnabled(true);
			}
		}
		else if constexpr (U == UNDO_T::JOIN_STYLE || U == UNDO_T::NIL) {
			if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL) {
				rmfi_popup_stroke_join_bevel().IsChecked(true);
				rmfi_menu_stroke_join_bevel().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER) {
				rmfi_popup_stroke_join_miter().IsChecked(true);
				rmfi_menu_stroke_join_miter().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER_OR_BEVEL) {
				rmfi_popup_stroke_join_miter_or_bevel().IsChecked(true);
				rmfi_menu_stroke_join_miter_or_bevel().IsChecked(true);
			}
			else if (m_main_sheet.m_stroke_join == D2D1_LINE_JOIN::D2D1_LINE_JOIN_ROUND) {
				rmfi_popup_stroke_join_round().IsChecked(true);
				rmfi_menu_stroke_join_round().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::FONT_STRETCH || U == UNDO_T::NIL) {
			if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_CONDENSED) {
				rmfi_popup_font_stretch_ultra_condensed().IsChecked(true);
				rmfi_menu_font_stretch_ultra_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_CONDENSED) {
				rmfi_popup_font_stretch_extra_condensed().IsChecked(true);
				rmfi_menu_font_stretch_extra_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_CONDENSED) {
				rmfi_popup_font_stretch_condensed().IsChecked(true);
				rmfi_menu_font_stretch_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_CONDENSED) {
				rmfi_popup_font_stretch_semi_condensed().IsChecked(true);
				rmfi_menu_font_stretch_semi_condensed().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL) {
				rmfi_popup_font_stretch_normal().IsChecked(true);
				rmfi_menu_font_stretch_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_SEMI_EXPANDED) {
				rmfi_popup_font_stretch_semi_expanded().IsChecked(true);
				rmfi_menu_font_stretch_semi_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXPANDED) {
				rmfi_popup_font_stretch_expanded().IsChecked(true);
				rmfi_menu_font_stretch_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_EXTRA_EXPANDED) {
				rmfi_popup_font_stretch_extra_expanded().IsChecked(true);
				rmfi_menu_font_stretch_extra_expanded().IsChecked(true);
			}
			else if (m_main_sheet.m_font_stretch == DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_ULTRA_EXPANDED) {
				rmfi_popup_font_stretch_ultra_expanded().IsChecked(true);
				rmfi_menu_font_stretch_ultra_expanded().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::FONT_STYLE || U == UNDO_T::NIL) {
			if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_ITALIC) {
				rmfi_popup_font_style_italic().IsChecked(true);
				rmfi_menu_font_style_italic().IsChecked(true);
			}
			else if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL) {
				rmfi_popup_font_style_normal().IsChecked(true);
				rmfi_menu_font_style_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_style == DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_OBLIQUE) {
				rmfi_popup_font_style_oblique().IsChecked(true);
				rmfi_menu_font_style_oblique().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::FONT_WEIGHT || U == UNDO_T::NIL) {
			if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_THIN) {
				rmfi_popup_font_weight_thin().IsChecked(true);
				rmfi_menu_font_weight_thin().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_LIGHT) {
				rmfi_popup_font_weight_extra_light().IsChecked(true);
				rmfi_menu_font_weight_extra_light().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_LIGHT) {
				rmfi_popup_font_weight_light().IsChecked(true);
				rmfi_menu_font_weight_light().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL) {
				rmfi_popup_font_weight_normal().IsChecked(true);
				rmfi_menu_font_weight_normal().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_MEDIUM) {
				rmfi_popup_font_weight_medium().IsChecked(true);
				rmfi_menu_font_weight_medium().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_SEMI_BOLD) {
				rmfi_popup_font_weight_semi_bold().IsChecked(true);
				rmfi_menu_font_weight_semi_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BOLD) {
				rmfi_popup_font_weight_bold().IsChecked(true);
				rmfi_menu_font_weight_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BOLD) {
				rmfi_popup_font_weight_extra_bold().IsChecked(true);
				rmfi_menu_font_weight_extra_bold().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_BLACK) {
				rmfi_popup_font_weight_black().IsChecked(true);
				rmfi_menu_font_weight_black().IsChecked(true);
			}
			else if (m_main_sheet.m_font_weight == DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_EXTRA_BLACK) {
				rmfi_popup_font_weight_extra_black().IsChecked(true);
				rmfi_menu_font_weight_extra_black().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::TEXT_ALIGN_T || U == UNDO_T::NIL) {
			if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING) {
				rmfi_popup_text_align_left().IsChecked(true);
				rmfi_menu_text_align_left().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING) {
				rmfi_popup_text_align_right().IsChecked(true);
				rmfi_menu_text_align_right().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER) {
				rmfi_popup_text_align_center().IsChecked(true);
				rmfi_menu_text_align_center().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_horz == DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_JUSTIFIED) {
				rmfi_popup_text_align_just().IsChecked(true);
				rmfi_menu_text_align_just().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::TEXT_ALIGN_P || U == UNDO_T::NIL) {
			if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR) {
				rmfi_popup_text_align_top().IsChecked(true);
				rmfi_menu_text_align_top().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_FAR) {
				rmfi_popup_text_align_bot().IsChecked(true);
				rmfi_menu_text_align_bot().IsChecked(true);
			}
			else if (m_main_sheet.m_text_align_vert == DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER) {
				rmfi_popup_text_align_mid().IsChecked(true);
				rmfi_menu_text_align_mid().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::TEXT_WRAP || U == UNDO_T::NIL) {
			if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP) {
				rmfi_popup_text_wrap().IsChecked(true);
				rmfi_menu_text_wrap().IsChecked(true);
			}
			else if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_NO_WRAP) {
				rmfi_popup_text_no_wrap().IsChecked(true);
				rmfi_menu_text_no_wrap().IsChecked(true);
			}
			else if (m_main_sheet.m_text_word_wrap == DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_CHARACTER) {
				rmfi_popup_text_wrap_char().IsChecked(true);
				rmfi_menu_text_wrap_char().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::GRID_EMPH || U == UNDO_T::NIL) {
			if (m_main_sheet.m_grid_emph.m_gauge_1 == 0 && m_main_sheet.m_grid_emph.m_gauge_2 == 0) {
				rmfi_popup_grid_emph_1().IsChecked(true);
				rmfi_menu_grid_emph_1().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_emph.m_gauge_1 != 0 && m_main_sheet.m_grid_emph.m_gauge_2 == 0) {
				rmfi_popup_grid_emph_2().IsChecked(true);
				rmfi_menu_grid_emph_2().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_emph.m_gauge_1 != 0 && m_main_sheet.m_grid_emph.m_gauge_2 != 0) {
				rmfi_popup_grid_emph_3().IsChecked(true);
				rmfi_menu_grid_emph_3().IsChecked(true);
			}
		}
		else if constexpr (U == UNDO_T::GRID_SHOW || U == UNDO_T::NIL) {
			if (m_main_sheet.m_grid_show == GRID_SHOW::BACK) {
				rmfi_popup_grid_show_back().IsChecked(true);
				rmfi_menu_grid_show_back().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_show == GRID_SHOW::FRONT) {
				rmfi_popup_grid_show_front().IsChecked(true);
				rmfi_menu_grid_show_front().IsChecked(true);
			}
			else if (m_main_sheet.m_grid_show == GRID_SHOW::HIDE) {
				rmfi_popup_grid_show_hide().IsChecked(true);
				rmfi_menu_grid_show_hide().IsChecked(true);
			}
		}
	}
	// �l��I�����ꂽ�}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
	// �߂�l	�i�[�����O�̒l�ƈقȂ��Ă���, �l���i�[���ꂽ�� true.
	// U	����̎��
	// T	�l�̌^
	// val	�l
	template<UNDO_T U, typename T> bool MainPage::undo_push_set(const T& val)
	{
		// ���C���p���ɑ�������Ȃ�, ���C���p���̑������ύX����.
		T sheet_val;
		if (UndoValue<U>::GET(&m_main_sheet, sheet_val) && !equal(sheet_val, val)) {
			m_undo_stack.push_back(new UndoValue<U>(&m_main_sheet, val));
			menu_is_checked<U>();
		}
		auto changed = false;	// �l���ς�����}�`�����邩.
		for (auto s : m_main_sheet.m_shape_list) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			undo_push_set<U>(s, val);
			changed = true;
		}
		return changed;
	}

	// �}�`�̒l�̕ۑ���, ���̑�����X�^�b�N�ɐς�.
	// U	����̎��.
	// s	�}�`
	template <UNDO_T U> void MainPage::undo_push_set(Shape* const s)
	{
		m_undo_stack.push_back(new UndoValue<U>(s));
	}

	template bool MainPage::undo_push_set<UNDO_T::ARC_DIR>(D2D1_SWEEP_DIRECTION const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARC_END>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARC_ROT>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARC_START>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARROW_CAP>(D2D1_CAP_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARROW_JOIN>(D2D1_LINE_JOIN const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARROW_SIZE>(ARROW_SIZE const& val);
	template bool MainPage::undo_push_set<UNDO_T::ARROW_STYLE>(ARROW_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::IMAGE_OPAC>(float const& val);
	//template bool MainPage::undo_push_set<UNDO_T::DASH_CAP>(D2D1_CAP_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::DASH_PAT>(DASH_PAT const& val);
	template bool MainPage::undo_push_set<UNDO_T::DASH_STYLE>(D2D1_DASH_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::FILL_COLOR>(D2D1_COLOR_F const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_COLOR>(D2D1_COLOR_F const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_FAMILY>(wchar_t* const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_SIZE>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_STRETCH>(DWRITE_FONT_STRETCH const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_STYLE>(DWRITE_FONT_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::FONT_WEIGHT>(DWRITE_FONT_WEIGHT const& val);
	template bool MainPage::undo_push_set<UNDO_T::JOIN_LIMIT>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::JOIN_STYLE>(D2D1_LINE_JOIN const& val);
	template bool MainPage::undo_push_set<UNDO_T::POLY_END>(D2D1_FIGURE_END const& val);
	template bool MainPage::undo_push_set<UNDO_T::STROKE_CAP>(D2D1_CAP_STYLE const& val);
	template bool MainPage::undo_push_set<UNDO_T::STROKE_COLOR>(D2D1_COLOR_F const& val);
	template bool MainPage::undo_push_set<UNDO_T::STROKE_WIDTH>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT const& val);
	template bool MainPage::undo_push_set<UNDO_T::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT const& val);
	template bool MainPage::undo_push_set<UNDO_T::TEXT_LINE_SP>(float const& val);
	template bool MainPage::undo_push_set<UNDO_T::TEXT_PAD>(D2D1_SIZE_F const& val);
	template bool MainPage::undo_push_set<UNDO_T::TEXT_WRAP>(DWRITE_WORD_WRAPPING const& val);

	template void MainPage::undo_push_set<UNDO_T::GRID_BASE>(Shape* const s, float const& val);
	template void MainPage::undo_push_set<UNDO_T::GRID_EMPH>(Shape* const s, GRID_EMPH const& val);
	template void MainPage::undo_push_set<UNDO_T::GRID_COLOR>(Shape* const s, D2D1_COLOR_F const& val);
	template void MainPage::undo_push_set<UNDO_T::GRID_SHOW>(Shape* const s, GRID_SHOW const& val);
	template void MainPage::undo_push_set<UNDO_T::SHEET_COLOR>(Shape* const s, D2D1_COLOR_F const& val);
	template void MainPage::undo_push_set<UNDO_T::SHEET_SIZE>(Shape* const s, D2D1_SIZE_F const& val);
	template void MainPage::undo_push_set<UNDO_T::SHEET_PAD>(Shape* const s, D2D1_RECT_F const& val);
	template void MainPage::undo_push_set<UNDO_T::TEXT_CONTENT>(Shape* const s, wchar_t* const& val);

	template void MainPage::undo_push_set<UNDO_T::MOVE>(Shape* const s);
	template void MainPage::undo_push_set<UNDO_T::IMAGE_OPAC>(Shape* const s);

	// �f�[�^���[�_�[���瑀��X�^�b�N��ǂݍ���.
	// dt_reader	�f�[�^���[�_�[
	void MainPage::undo_read_stack(DataReader const& dt_reader)
	{
		Undo* r;
		while (undo_read_op(r, dt_reader)) {
			m_redo_stack.push_back(r);
		}
		Undo* u;
		while (undo_read_op(u, dt_reader)) {
			m_undo_stack.push_back(u);
		}
		//m_ustack_is_changed = dt_reader.ReadBoolean();
	}

	// ����X�^�b�N���f�[�^���C�^�[�ɏ�������.
	// dt_writer	�f�[�^���C�^�[
	void MainPage::undo_write_stack(DataWriter const& dt_writer)
	{
		for (const auto& r : m_redo_stack) {
			undo_write_op(r, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::END));
		for (const auto& u : m_undo_stack) {
			undo_write_op(u, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::END));
		//dt_writer.WriteBoolean(m_ustack_is_changed);
	}

}