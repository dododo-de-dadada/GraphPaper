//------------------------------
// MainPage_undo.cpp
// ���ɖ߂��Ƃ�蒼��
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

// �}�`�����X�g�ɒǉ�����.
#define UndoAppend(s)	UndoList(static_cast<Shape*>(s), static_cast<Shape*>(nullptr))
// �}�`���O���[�v�ɒǉ�����.
#define UndoAppendG(g, s)	UndoListGroup(static_cast<ShapeGroup*>(g), static_cast<Shape*>(s), static_cast<Shape*>(nullptr))
// �}�`�����X�g�ɑ}������.
#define UndoInsert(s, p)	UndoList(static_cast<Shape*>(s), static_cast<Shape*>(p))
// �}�`�����X�g�����菜��.
#define UndoRemove(s)	UndoList(static_cast<Shape*>(s))
// �}�`���O���[�v�����菜��.
#define UndoRemoveG(g, s)	UndoListGroup(static_cast<ShapeGroup*>(g), static_cast<Shape*>(s))

namespace winrt::GraphPaper::implementation
{
	// �k���ŋ�؂����A�̑����, ����̑g�Ƃ݂Ȃ�, ���̐���g���Ƃ���.
	// �X�^�b�N�ɐςނ��Ƃ��ł���ő�̑g��.
	constexpr uint32_t U_MAX_CNT = 32;

	// ����X�^�b�N��������, �܂܂�鑀���j������.
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
			u = new UndoAttr<UNDO_OP::ARROW_SIZE>(dt_reader);
			break;
		case UNDO_OP::ARROW_STYLE:
			u = new UndoAttr<UNDO_OP::ARROW_STYLE>(dt_reader);
			break;
		case UNDO_OP::FILL_COLOR:
			u = new UndoAttr<UNDO_OP::FILL_COLOR>(dt_reader);
			break;
		case UNDO_OP::ANCH_POS:
			u = new UndoAnchor(dt_reader);
			break;
		case UNDO_OP::FONT_COLOR:
			u = new UndoAttr<UNDO_OP::FONT_COLOR>(dt_reader);
			break;
		case UNDO_OP::FONT_FAMILY:
			u = new UndoAttr<UNDO_OP::FONT_FAMILY>(dt_reader);
			break;
		case UNDO_OP::FONT_SIZE:
			u = new UndoAttr<UNDO_OP::FONT_SIZE>(dt_reader);
			break;
		case UNDO_OP::FONT_STYLE:
			u = new UndoAttr<UNDO_OP::FONT_STYLE>(dt_reader);
			break;
		case UNDO_OP::FONT_STRETCH:
			u = new UndoAttr<UNDO_OP::FONT_STRETCH>(dt_reader);
			break;
		case UNDO_OP::FONT_WEIGHT:
			u = new UndoAttr<UNDO_OP::FONT_WEIGHT>(dt_reader);
			break;
		case UNDO_OP::GRID_BASE:
			u = new UndoAttr<UNDO_OP::GRID_BASE>(dt_reader);
			break;
		case UNDO_OP::GRID_GRAY:
			u = new UndoAttr<UNDO_OP::GRID_GRAY>(dt_reader);
			break;
		case UNDO_OP::GRID_EMPH:
			u = new UndoAttr<UNDO_OP::GRID_EMPH>(dt_reader);
			break;
		case UNDO_OP::GRID_SHOW:
			u = new UndoAttr<UNDO_OP::GRID_SHOW>(dt_reader);
			break;
		case UNDO_OP::GROUP:
			u = new UndoListGroup(dt_reader);
			break;
		case UNDO_OP::LIST:
			u = new UndoList(dt_reader);
			break;
		case UNDO_OP::TEXT_LINE:
			u = new UndoAttr<UNDO_OP::TEXT_LINE>(dt_reader);
			break;
		case UNDO_OP::TEXT_MARGIN:
			u = new UndoAttr<UNDO_OP::TEXT_MARGIN>(dt_reader);
			break;
		case UNDO_OP::SHEET_COLOR:
			u = new UndoAttr<UNDO_OP::SHEET_COLOR>(dt_reader);
			break;
		case UNDO_OP::SHEET_SIZE:
			u = new UndoAttr<UNDO_OP::SHEET_SIZE>(dt_reader);
			break;
		case UNDO_OP::TEXT_ALIGN_P:
			u = new UndoAttr<UNDO_OP::TEXT_ALIGN_P>(dt_reader);
			break;
		case UNDO_OP::SELECT:
			u = new UndoSelect(dt_reader);
			break;
		case UNDO_OP::START_POS:
			u = new UndoAttr<UNDO_OP::START_POS>(dt_reader);
			break;
		case UNDO_OP::STROKE_COLOR:
			u = new UndoAttr<UNDO_OP::STROKE_COLOR>(dt_reader);
			break;
		case UNDO_OP::STROKE_PATT:
			u = new UndoAttr<UNDO_OP::STROKE_PATT>(dt_reader);
			break;
		case UNDO_OP::STROKE_STYLE:
			u = new UndoAttr<UNDO_OP::STROKE_STYLE>(dt_reader);
			break;
		case UNDO_OP::STROKE_WIDTH:
			u = new UndoAttr<UNDO_OP::STROKE_WIDTH>(dt_reader);
			break;
		case UNDO_OP::TEXT_CONTENT:
			u = new UndoAttr<UNDO_OP::TEXT_CONTENT>(dt_reader);
			break;
		case UNDO_OP::TEXT_ALIGN_T:
			u = new UndoAttr<UNDO_OP::TEXT_ALIGN_T>(dt_reader);
			break;
		case UNDO_OP::TEXT_RANGE:
			u = new UndoAttr<UNDO_OP::TEXT_RANGE>(dt_reader);
			break;
		default:
			throw winrt::hresult_not_implemented();
			break;
		}
		return true;
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

	// ���ɖ߂�/��蒼�����j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
	void MainPage::undo_menu_enable(void)
	{
		mfi_undo().IsEnabled(m_stack_ucnt > 0);
		mfi_redo().IsEnabled(m_stack_rcnt > 0);
	}

	// �ҏW���j���[�́u��蒼���v���I�����ꂽ.
	void MainPage::redo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_stack_rcnt == 0) {
			return;
		}
		auto flag = false;
		// ���ɖ߂�����X�^�b�N����k���ŋ�؂��Ă��Ȃ� (�I���Ȃǂ�) �������菜��.
		while (m_stack_undo.empty() != true) {
			auto u = m_stack_undo.back();
			if (u == nullptr) {
				break;
			}
			m_stack_undo.pop_back();
			undo_exec(u);
			flag = true;
		}
		// ��蒼������X�^�b�N���瑀������o��, ���s����, ���ɖ߂�����ɐς�.
		// ���삪�k���łȂ�������������J��Ԃ�.
		while (m_stack_redo.size() > 0) {
			auto r = m_stack_redo.back();
			m_stack_redo.pop_back();
			m_stack_undo.push_back(r);
			if (r == nullptr) {
				// ���s���ꂽ���삪�������ꍇ, �X�^�b�N�̑g����ύX����.
				m_stack_rcnt--;
				m_stack_ucnt++;
				break;
			}
			undo_exec(r);
			flag = true;
		}
		if (flag != true) {
			return;
		}
		// ���j���[��\�����X�V����.
		edit_menu_enable();
		sheet_bound();
		sheet_panle_size();
		sheet_draw();
		if (m_summary_atomic.load(std::memory_order_acquire)) {
		//if (m_summary_visible) {
			summary_update();
		}
	}

	// �ҏW���j���[�́u���ɖ߂��v���I�����ꂽ.
	void MainPage::undo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_stack_ucnt == 0) {
			return;
		}
		auto st = 0;
		while (m_stack_undo.empty() != true) {
			auto u = m_stack_undo.back();
			if (st == 0) {
				m_stack_undo.pop_back();
				if (u != nullptr) {
					undo_exec(u);
				}
				else {
					st = 1;
				}
				continue;
			}
			if (u == nullptr) {
				break;
			}
			m_stack_undo.pop_back();
			undo_exec(u);
			if (st == 1) {
				m_stack_redo.push_back(nullptr);
				st = 2;
			}
			m_stack_redo.push_back(u);
		}
		if (st == 2) {
			// ���s���ꂽ���삪�������ꍇ, �X�^�b�N�̑g����ύX����.
			m_stack_ucnt--;
			m_stack_rcnt++;
		}
		edit_menu_enable();
		sheet_bound();
		sheet_panle_size();
		sheet_draw();
		if (m_summary_atomic.load(std::memory_order_acquire)) {
		//if (m_summary_visible) {
			summary_update();
		}
	}

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	void MainPage::undo_clear(void)
	{
		m_stack_updt = false;
		m_stack_rcnt -= undo_clear_stack(m_stack_redo);
		m_stack_ucnt -= undo_clear_stack(m_stack_undo);
#if defined(_DEBUG)
		if (m_stack_rcnt == 0 && m_stack_ucnt == 0) {
			return;
		}
		winrt::Windows::UI::Xaml::Controls::ContentDialog dialog;
		dialog.Title(box_value(L"Undo is not empty."));
		dialog.CloseButtonText(L"Close");
		auto _{ dialog.ShowAsync() };
#endif
	}

	// ��������s����.
	void MainPage::undo_exec(Undo* u)
	{
		summary_reflect(u);
		u->exec();
		auto const& u_type = typeid(*u);
		if (u_type == typeid(UndoAttr<UNDO_OP::ARROW_STYLE>)) {
			// ���g���j���[�́u���̎�ށv�Ɉ������.
			arrow_style_check_menu(m_main_sheet.m_arrow_style);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::GRID_BASE>)) {
			// ����̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			sbar_set_grid();
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::GRID_EMPH>)) {
			// �p�����j���[�́u����̋����v�Ɉ������.
			grid_emph_check_menu(m_main_sheet.m_grid_emph);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::GRID_SHOW>)) {
			// �p�����j���[�́u����̕\���v�Ɉ������.
			grid_show_check_menu(m_main_sheet.m_grid_show);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::FONT_STYLE>)) {
			// ���̃��j���[�́u���́v�Ɉ������.
			font_style_check_menu(m_main_sheet.m_font_style);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::SHEET_SIZE>)) {
			// �p���̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			sbar_set_sheet();
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::STROKE_STYLE>)) {
			// ���g���j���[�́u��ށv�Ɉ������.
			stroke_style_check_menu(m_main_sheet.m_stroke_style);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::TEXT_ALIGN_T>)) {
			text_align_t_check_menu(m_main_sheet.m_text_align_t);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::TEXT_ALIGN_P>)) {
			text_align_p_check_menu(m_main_sheet.m_text_align_p);
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

	// �}�`�̕��ʂ̈ʒu���X�^�b�N�ɕۑ�����.
	void MainPage::undo_push_anchor(Shape* s, const uint32_t anchor)
	{
		m_stack_undo.push_back(new UndoAnchor(s, anchor));
	}

	// �}�`��ǉ�����, ���̑�����X�^�b�N�ɐς�.
	void MainPage::undo_push_append(Shape* s)
	{
		m_stack_undo.push_back(new UndoAppend(s));
	}

	// �}�`���O���[�v�}�`�ɒǉ�����, ���̑�����X�^�b�N�ɐς�.
	// g	�O���[�v�}�`
	// s	�ǉ�����}�`
	// �߂�l	�Ȃ�
	void MainPage::undo_push_append(ShapeGroup* g, Shape* s)
	{
		m_stack_undo.push_back(new UndoAppendG(g, s));
	}

	// �}�`�����ւ���, ���̑�����X�^�b�N�ɐς�.
	void MainPage::undo_push_arrange(Shape* s, Shape* t)
	{
		m_stack_undo.push_back(new UndoArrange2(s, t));
	}

	// �}�`��}������, ���̑�����X�^�b�N�ɐς�.
	void MainPage::undo_push_insert(Shape* s, Shape* s_pos)
	{
		m_stack_undo.push_back(new UndoInsert(s, s_pos));
	}

	// �}�`�̈ʒu���X�^�b�N�ɕۑ����Ă��獷�������ړ�����.
	// diff	�ړ������鍷��
	// all	���ׂĂ̐}�`�̏ꍇ true, �I�����ꂽ�}�`�̏ꍇ false
	void MainPage::undo_push_move(const D2D1_POINT_2F diff, const bool all)
	{
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (all != true && s->is_selected() != true) {
				continue;
			}
			undo_push_set<UNDO_OP::START_POS>(s);
			s->move(diff);
		}
	}

	// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
	// ��蒼������X�^�b�N�͏��������.
	void MainPage::undo_push_null(void)
	{
		// ��蒼������X�^�b�N��������, �������ꂽ����̑g����, ����̑g���������.
		m_stack_rcnt -= undo_clear_stack(m_stack_redo);
		// ���ɖ߂�����X�^�b�N�Ƀk����ς�.
		m_stack_undo.push_back(nullptr);
		// ����X�^�b�N�̍X�V�t���O�𗧂Ă�.
		m_stack_updt = true;
		// ����̑g�����C���N�������g����.
		//m_stack_nset++;
		m_stack_ucnt++;
		if (m_stack_ucnt <= U_MAX_CNT) {
		//if (m_stack_nset <= U_MAX_CNT) {
			// ����̑g�����ő�̑g���ȉ��̏ꍇ,
			// �I������.
			return;
		}
		while (m_stack_undo.empty() != true) {
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
	// ������, �}�`�����̒l�������Ȃ��ꍇ, �܂��͂��łɓ��l�̏ꍇ�͉������Ȃ�.
	// s	�}�`
	// value	�l
	// �߂�l	�Ȃ�
	template <UNDO_OP U, typename T>
	void MainPage::undo_push_set(Shape* s, T const& value)
	{
		T t_value;
		if (UndoAttr<U>::GET(s, t_value) != true || equal(t_value, value)) {
			// �}�`�����̒l�������Ȃ��ꍇ, �܂��͂��łɓ��l�̏ꍇ,
			// �I������.
			return;
		}
		// 
		m_stack_undo.push_back(new UndoAttr<U>(s, value));
	}

	// �l��I�����ꂽ�}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
	// U	����̎��.
	// T	�i�[����^.
	// value	�i�[����l
	// �i�[����^ T �͖������Ȃ��Ă������̌^���琄��ł���
	template<UNDO_OP U, typename T>
	void MainPage::undo_push_set(T const& value)
	{
		m_stack_undo.push_back(new UndoAttr<U>(&m_main_sheet, value));
		auto flag = false;
		for (auto s : m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->is_selected() != true) {
				continue;
			}
			undo_push_set<U>(s, value);
			flag = true;
		}
		if (flag != true) {
			return;
		}
		undo_push_null();
		// �ҏW���j���[���ڂ̎g�p�̉ۂ�ݒ肷��.
		edit_menu_enable();
		sheet_draw();
	}

	// �}�`�̒l�̕ۑ������s����, ���̑�����X�^�b�N�ɐς�.
	template <UNDO_OP U>
	void MainPage::undo_push_set(Shape* s)
	{
		m_stack_undo.push_back(new UndoAttr<U>(s));
	}

	template void MainPage::undo_push_set<UNDO_OP::ARROW_SIZE>(ARROW_SIZE const& value);
	template void MainPage::undo_push_set<UNDO_OP::ARROW_STYLE>(ARROW_STYLE const& value);
	template void MainPage::undo_push_set<UNDO_OP::FILL_COLOR>(D2D1_COLOR_F const& value);
	template void MainPage::undo_push_set<UNDO_OP::FONT_COLOR>(D2D1_COLOR_F const& value);
	template void MainPage::undo_push_set<UNDO_OP::FONT_FAMILY>(wchar_t* const& value);
	template void MainPage::undo_push_set<UNDO_OP::FONT_SIZE>(double const& value);
	template void MainPage::undo_push_set<UNDO_OP::FONT_STRETCH>(DWRITE_FONT_STRETCH const& value);
	template void MainPage::undo_push_set<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE const& value);
	template void MainPage::undo_push_set<UNDO_OP::FONT_WEIGHT>(DWRITE_FONT_WEIGHT const& value);
	template void MainPage::undo_push_set<UNDO_OP::GRID_BASE>(Shape* s, double const& value);
	template void MainPage::undo_push_set<UNDO_OP::GRID_GRAY>(Shape* s, double const& value);
	template void MainPage::undo_push_set<UNDO_OP::GRID_EMPH>(Shape* s, GRID_EMPH const& value);
	template void MainPage::undo_push_set<UNDO_OP::GRID_SHOW>(Shape* s, GRID_SHOW const& value);
	template void MainPage::undo_push_set<UNDO_OP::SHEET_COLOR>(Shape* s, D2D1_COLOR_F const& value);
	template void MainPage::undo_push_set<UNDO_OP::SHEET_SIZE>(Shape* s, D2D1_SIZE_F const& value);
	template void MainPage::undo_push_set<UNDO_OP::START_POS>(Shape* s);
	template void MainPage::undo_push_set<UNDO_OP::STROKE_COLOR>(D2D1_COLOR_F const& value);
	template void MainPage::undo_push_set<UNDO_OP::STROKE_PATT>(STROKE_PATT const& value);
	template void MainPage::undo_push_set<UNDO_OP::STROKE_STYLE>(D2D1_DASH_STYLE const& value);
	template void MainPage::undo_push_set<UNDO_OP::STROKE_WIDTH>(double const& value);
	template void MainPage::undo_push_set<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT const& value);
	template void MainPage::undo_push_set<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT const& value);
	template void MainPage::undo_push_set<UNDO_OP::TEXT_CONTENT>(Shape* s, wchar_t* const& value);
	template void MainPage::undo_push_set<UNDO_OP::TEXT_LINE>(double const& value);
	template void MainPage::undo_push_set<UNDO_OP::TEXT_MARGIN>(D2D1_SIZE_F const& value);

	// �����͈͂̒l��}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
	// �X�^�b�N�̈�ԏ�̑���̑g�̒��ɁA���łɕ����͈͂̑��삪�ς܂�Ă���ꍇ,
	// ���̒l��������������.
	// �����łȂ��ꍇ, �V���ȑ���Ƃ��ăX�^�b�N�ɐς�.
	// s	���삷��}�`
	// value	�����͈͂̒l
	// �߂�l	�Ȃ�
	template<>
	void MainPage::undo_push_set<UNDO_OP::TEXT_RANGE, DWRITE_TEXT_RANGE>(Shape* s, DWRITE_TEXT_RANGE const& value)
	{
		auto flag = false;
		// ���ɖ߂�����X�^�b�N�̊e����ɂ���
		for (auto it = m_stack_undo.rbegin(); it != m_stack_undo.rend(); it++) {
			const auto u = *it;
			if (u == nullptr) {
				// ���삪�k���̏ꍇ,
				break;
			}
			else if (typeid(*u) != typeid(UndoAttr<UNDO_OP::TEXT_RANGE>)) {
				// ���삪�����͈͂̑I�����鑀��łȂ��ꍇ,
				if (typeid(*u) != typeid(UndoSelect)) {
					// ���삪�}�`�̑I���𔽓]���鑀��łȂ��ꍇ,
					// ���f����.
					break;
				}
			}
			// ���삪�����͈͂̑I�����鑀��̏ꍇ,
			else if (u->m_shape == s) {
				// ���삷��}�`�������̐}�`�Ɠ����ꍇ,
				// �}�`�𒼐ڑ��삵�ăX�^�b�N�ɂ͐ς܂Ȃ��悤�ɂ���.
				s->set_text_range(value);
				flag = true;
				break;
			}
		}
		if (flag != true) {
			m_stack_undo.push_back(new UndoAttr<UNDO_OP::TEXT_RANGE>(s, value));
		}

	}

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
		m_stack_ucnt = dt_reader.ReadUInt32();
		m_stack_updt = dt_reader.ReadBoolean();
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
		dt_writer.WriteUInt32(m_stack_ucnt);
		dt_writer.WriteBoolean(m_stack_updt);
	}

}