//------------------------------
// MainPage_ustack.cpp
// ���ɖ߂��Ƃ�蒼������X�^�b�N
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

// �}�`���O���[�v�ɒǉ�����.
#define UndoAppendG(g, s)	UndoListGroup(static_cast<ShapeGroup* const>(g), static_cast<Shape* const>(s), static_cast<Shape* const>(nullptr))
// �}�`�����X�g�ɒǉ�����.
#define UndoAppend(s)	UndoList(static_cast<Shape* const>(s), static_cast<Shape* const>(nullptr))
// �}�`�����X�g�ɑ}������.
#define UndoInsert(s, p)	UndoList(static_cast<Shape* const>(s), static_cast<Shape* const>(p))
// �}�`���O���[�v�����菜��.
#define UndoRemoveG(g, s)	UndoListGroup(static_cast<ShapeGroup* const>(g), static_cast<Shape* const>(s))
// �}�`�����X�g�����菜��.
#define UndoRemove(s)	UndoList(static_cast<Shape* const>(s))

namespace winrt::GraphPaper::implementation
{
	// �k���ŋ�؂����A�̑����, ����̑g�Ƃ݂Ȃ�, ���̐���g���Ƃ���.
	// �X�^�b�N�ɐςނ��Ƃ��ł���ő�̑g��.
	constexpr uint32_t MAX_UCNT = 32;

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	static uint32_t ustack_clear_stack(UNDO_STACK& u_stack);
	// �f�[�^���[�_�[���瑀���ǂݍ���.
	static bool ustack_read_op(Undo*& u, DataReader const& dt_reader);
	// ������f�[�^���[�_�[�ɏ�������.
	static void ustack_write_op(Undo* u, DataWriter const& dt_writer);

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	// u_stack	����X�^�b�N
	// �߂�l	������������̑g��
	static uint32_t ustack_clear_stack(UNDO_STACK& u_stack)
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

	// �f�[�^���[�_�[���瑀���ǂݍ���.
	// u	����
	// dt_reader	�f�[�^���[�_�[
	static bool ustack_read_op(Undo*& u, DataReader const& dt_reader)
	{
		if (dt_reader.UnconsumedBufferLength() < sizeof(uint32_t)) {
			return false;
		}
		auto u_op = static_cast<UNDO_OP>(dt_reader.ReadUInt32());
		if (u_op == UNDO_OP::END) {
			return false;
		}
		switch (u_op) {
		case UNDO_OP::NIL:
			u = nullptr;
			break;
		case UNDO_OP::POS_ANCH:
			u = new UndoAnchor(dt_reader);
			break;
		case UNDO_OP::ARRANGE:
			u = new UndoArrange(dt_reader);
			break;
		case UNDO_OP::ARROW_SIZE:
			u = new UndoAttr<UNDO_OP::ARROW_SIZE>(dt_reader);
			break;
		case UNDO_OP::ARROW_STYLE:
			u = new UndoAttr<UNDO_OP::ARROW_STYLE>(dt_reader);
			break;
		case UNDO_OP::IMAGE:
			u = new UndoImage(dt_reader);
			break;
		//case UNDO_OP::IMAGE_ASPECT:
		//	u = new UndoAttr<UNDO_OP::IMAGE_ASPECT>(dt_reader);
		//	break;
		case UNDO_OP::IMAGE_OPAC:
			u = new UndoAttr<UNDO_OP::IMAGE_OPAC>(dt_reader);
			break;
		case UNDO_OP::FILL_COLOR:
			u = new UndoAttr<UNDO_OP::FILL_COLOR>(dt_reader);
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
		case UNDO_OP::GRID_COLOR:
			u = new UndoAttr<UNDO_OP::GRID_COLOR>(dt_reader);
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
		case UNDO_OP::SELECT:
			u = new UndoSelect(dt_reader);
			break;
		case UNDO_OP::SHEET_COLOR:
			u = new UndoAttr<UNDO_OP::SHEET_COLOR>(dt_reader);
			break;
		case UNDO_OP::SHEET_SIZE:
			u = new UndoAttr<UNDO_OP::SHEET_SIZE>(dt_reader);
			break;
		case UNDO_OP::POS_START:
			u = new UndoAttr<UNDO_OP::POS_START>(dt_reader);
			break;
		case UNDO_OP::STROKE_CAP:
			u = new UndoAttr<UNDO_OP::STROKE_CAP>(dt_reader);
			break;
		case UNDO_OP::STROKE_COLOR:
			u = new UndoAttr<UNDO_OP::STROKE_COLOR>(dt_reader);
			break;
		case UNDO_OP::DASH_CAP:
			u = new UndoAttr<UNDO_OP::DASH_CAP>(dt_reader);
			break;
		case UNDO_OP::DASH_PATT:
			u = new UndoAttr<UNDO_OP::DASH_PATT>(dt_reader);
			break;
		case UNDO_OP::DASH_STYLE:
			u = new UndoAttr<UNDO_OP::DASH_STYLE>(dt_reader);
			break;
		case UNDO_OP::JOIN_LIMIT:
			u = new UndoAttr<UNDO_OP::JOIN_LIMIT>(dt_reader);
			break;
		case UNDO_OP::JOIN_STYLE:
			u = new UndoAttr<UNDO_OP::JOIN_STYLE>(dt_reader);
			break;
		case UNDO_OP::STROKE_WIDTH:
			u = new UndoAttr<UNDO_OP::STROKE_WIDTH>(dt_reader);
			break;
		case UNDO_OP::TEXT_ALIGN_P:
			u = new UndoAttr<UNDO_OP::TEXT_ALIGN_P>(dt_reader);
			break;
		case UNDO_OP::TEXT_ALIGN_T:
			u = new UndoAttr<UNDO_OP::TEXT_ALIGN_T>(dt_reader);
			break;
		case UNDO_OP::TEXT_CONTENT:
			u = new UndoAttr<UNDO_OP::TEXT_CONTENT>(dt_reader);
			break;
		case UNDO_OP::TEXT_LINE_SP:
			u = new UndoAttr<UNDO_OP::TEXT_LINE_SP>(dt_reader);
			break;
		case UNDO_OP::TEXT_MARGIN:
			u = new UndoAttr<UNDO_OP::TEXT_MARGIN>(dt_reader);
			break;
		case UNDO_OP::TEXT_RANGE:
			u = new UndoAttr<UNDO_OP::TEXT_RANGE>(dt_reader);
			break;
		default:
			throw winrt::hresult_invalid_argument();
			break;
		}
		return true;
	}

	// ������f�[�^���[�_�[�ɏ�������.
	// u	����
	// dt_writer	�f�[�^���C�^�[
	static void ustack_write_op(Undo* u, DataWriter const& dt_writer)
	{
		if (u != nullptr) {
			u->write(dt_writer);
		}
		else {
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::NIL));
		}
	}

	// �ҏW���j���[�́u��蒼���v���I�����ꂽ.
	void MainPage::ustack_redo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_ustack_rcnt == 0) {
			return;
		}
		auto flag = false;
		// ���ɖ߂�����X�^�b�N����k���ŋ�؂��Ă��Ȃ� (�I���Ȃǂ�) �������菜��.
		while (m_ustack_undo.empty() != true) {
			auto u = m_ustack_undo.back();
			if (u == nullptr) {
				break;
			}
			m_ustack_undo.pop_back();
			ustack_exec(u);
			flag = true;
		}
		// ��蒼������X�^�b�N���瑀������o��, ���s����, ���ɖ߂�����ɐς�.
		// ���삪�k���łȂ�������������J��Ԃ�.
		while (m_ustack_redo.size() > 0) {
			auto r = m_ustack_redo.back();
			m_ustack_redo.pop_back();
			m_ustack_undo.push_back(r);
			if (r == nullptr) {
				// ���s���ꂽ���삪�������ꍇ, �X�^�b�N�̑g����ύX����.
				m_ustack_rcnt--;
				m_ustack_ucnt++;
				break;
			}
			ustack_exec(r);
			flag = true;
		}
		if (flag != true) {
			return;
		}
		xcvd_is_enabled();
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_update();
		}
	}

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	void MainPage::ustack_clear(void)
	{
		m_ustack_updt = false;
		m_ustack_rcnt -= ustack_clear_stack(m_ustack_redo);
		m_ustack_ucnt -= ustack_clear_stack(m_ustack_undo);
#if defined(_DEBUG)
		if (m_ustack_rcnt == 0 && m_ustack_ucnt == 0) {
			return;
		}
		using winrt::Windows::UI::Xaml::Controls::ContentDialog;
		ContentDialog dialog;
		dialog.Title(box_value(L"Undo is not empty."));
		dialog.CloseButtonText(L"Close");
		auto _{ dialog.ShowAsync() };
#endif
	}

	// �ҏW���j���[�́u���ɖ߂��v���I�����ꂽ.
	void MainPage::ustack_undo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_ustack_ucnt == 0) {
			return;
		}
		auto st = 0;
		while (m_ustack_undo.empty() != true) {
			auto u = m_ustack_undo.back();
			if (st == 0) {
				m_ustack_undo.pop_back();
				if (u != nullptr) {
					ustack_exec(u);
				}
				else {
					st = 1;
				}
				continue;
			}
			if (u == nullptr) {
				break;
			}
			m_ustack_undo.pop_back();
			ustack_exec(u);
			if (st == 1) {
				m_ustack_redo.push_back(nullptr);
				st = 2;
			}
			m_ustack_redo.push_back(u);
		}
		if (st == 2) {
			// ���s���ꂽ���삪�������ꍇ, �X�^�b�N�̑g����ύX����.
			m_ustack_ucnt--;
			m_ustack_rcnt++;
		}
		xcvd_is_enabled();
		sheet_update_bbox();
		sheet_panle_size();
		sheet_draw();
		// �ꗗ���\������Ă邩���肷��.
		if (summary_is_visible()) {
			summary_update();
		}
	}

	// ��������s����.
	// u	����
	void MainPage::ustack_exec(Undo* u)
	{
		summary_reflect(u);
		u->exec();
		auto const& u_type = typeid(*u);
		if (u_type == typeid(UndoAttr<UNDO_OP::ARROW_STYLE>)) {
			// ���g���j���[�́u��邵�̎�ށv�Ɉ������.
			ARROW_STYLE value;
			m_sheet_main.get_arrow_style(value);
			arrow_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::GRID_BASE>)) {
			// ����̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			status_set_grid();
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::GRID_EMPH>)) {
			// �p�����j���[�́u����̋����v�Ɉ������.
			GRID_EMPH value;
			m_sheet_main.get_grid_emph(value);
			grid_emph_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::GRID_SHOW>)) {
			GRID_SHOW value;
			m_sheet_main.get_grid_show(value);
			grid_show_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::FONT_STYLE>)) {
			// ���̃��j���[�́u���́v�Ɉ������.
			DWRITE_FONT_STYLE value;
			m_sheet_main.get_font_style(value);
			font_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::SHEET_SIZE>)) {
			// �p���̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			status_set_sheet();
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::STROKE_CAP>)) {
			CAP_STYLE value;
			m_sheet_main.get_stroke_cap(value);
			cap_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::DASH_CAP>)) {
			D2D1_CAP_STYLE value;
			m_sheet_main.get_dash_cap(value);
			//cap_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::DASH_STYLE>)) {
			D2D1_DASH_STYLE value;
			m_sheet_main.get_dash_style(value);
			dash_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::JOIN_STYLE>)) {
			D2D1_LINE_JOIN value;
			m_sheet_main.get_join_style(value);
			join_style_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::TEXT_ALIGN_T>)) {
			DWRITE_TEXT_ALIGNMENT value;
			m_sheet_main.get_text_align_t(value);
			text_align_t_is_checked(value);
		}
		else if (u_type == typeid(UndoAttr<UNDO_OP::TEXT_ALIGN_P>)) {
			DWRITE_PARAGRAPH_ALIGNMENT value;
			m_sheet_main.get_text_align_p(value);
			text_align_p_is_checked(value);
		}
	}

	// ���ɖ߂�/��蒼�����j���[�̉ۂ�ݒ肷��.
	void MainPage::ustack_is_enable(void)
	{
		mfi_undo().IsEnabled(m_ustack_ucnt > 0);
		mfi_redo().IsEnabled(m_ustack_rcnt > 0);
	}

	// �����ȑ���̑g���|�b�v����.
	// �߂�l	�|�b�v���ꂽ�ꍇ true
	bool MainPage::ustack_pop_if_invalid(void)
	{
		while (m_ustack_undo.size() > 0) {
			auto u = m_ustack_undo.back();
			if (u == nullptr) {
				return true;
			}
			if (u->changed()) {
				break;
			}
			m_ustack_undo.pop_back();
		}
		return false;
	}

	void MainPage::ustack_push_image(Shape* const s)
	{
		m_ustack_undo.push_back(new UndoImage(static_cast<ShapeImage*>(s)));
	}

	// �}�`�̕��ʂ̈ʒu���X�^�b�N�ɕۑ�����.
	void MainPage::ustack_push_anch(Shape* const s, const uint32_t anch)
	{
		m_ustack_undo.push_back(new UndoAnchor(s, anch));
	}

	// �}�`��ǉ�����, ���̑�����X�^�b�N�ɐς�.
	// s	�ǉ�����}�`
	void MainPage::ustack_push_append(Shape* s)
	{
		m_ustack_undo.push_back(new UndoAppend(s));
	}

	// �}�`���O���[�v�}�`�ɒǉ�����, ���̑�����X�^�b�N�ɐς�.
	// g	�O���[�v�}�`
	// s	�ǉ�����}�`
	// �߂�l	�Ȃ�
	void MainPage::ustack_push_append(ShapeGroup* g, Shape* s)
	{
		m_ustack_undo.push_back(new UndoAppendG(g, s));
	}

	// �}�`�����ւ���, ���̑�����X�^�b�N�ɐς�.
	void MainPage::ustack_push_arrange(Shape* const s, Shape* const t)
	{
		m_ustack_undo.push_back(new UndoArrange(s, t));
	}

	// �}�`��}������, ���̑�����X�^�b�N�ɐς�.
	void MainPage::ustack_push_insert(Shape* s, Shape* s_pos)
	{
		m_ustack_undo.push_back(new UndoInsert(s, s_pos));
	}

	// �}�`�̈ʒu���X�^�b�N�ɕۑ����Ă��獷�������ړ�����.
	// d_vec	�ړ������鍷��
	// all	���ׂĂ̐}�`�̏ꍇ true, �I�����ꂽ�}�`�̏ꍇ false
	void MainPage::ustack_push_move(const D2D1_POINT_2F d_vec, const bool all)
	{
		for (auto s : m_sheet_main.m_list_shapes) {
			if (s->is_deleted()) {
				continue;
			}
			if (all != true && s->is_selected() != true) {
				continue;
			}
			ustack_push_set<UNDO_OP::POS_START>(s);
			s->move(d_vec);
		}
	}

	// ��A�̑���̋�؂Ƃ��ăk��������X�^�b�N�ɐς�.
	// ��蒼������X�^�b�N�͏��������.
	void MainPage::ustack_push_null(void)
	{
		// ��蒼������X�^�b�N��������, �������ꂽ����̑g����, ����̑g���������.
		m_ustack_rcnt -= ustack_clear_stack(m_ustack_redo);
		// ���ɖ߂�����X�^�b�N�Ƀk����ς�.
		m_ustack_undo.push_back(nullptr);
		// true ���X�^�b�N���X�V���ꂽ������Ɋi�[����.
		m_ustack_updt = true;
		// ����̑g�����C���N�������g����.
		m_ustack_ucnt++;
		// ����̑g�����ő�̑g���ȉ������肷��.
		if (m_ustack_ucnt <= MAX_UCNT) {
			return;
		}
		// ���ɖ߂�����X�^�b�N����łȂ������肷��.
		while (m_ustack_undo.empty() != true) {
			// ����X�^�b�N�̈�Ԓ�̑�������o��.
			const auto u = m_ustack_undo.front();
			m_ustack_undo.pop_front();
			// ���삪�k�������肷��.
			if (u == nullptr) {
				break;
			}
		}
		// ����̑g���� 1 ������.
		m_ustack_ucnt--;
	}

	// �}�`���O���[�v����폜����, ���̑�����X�^�b�N�ɐς�.
	// g	�O���[�v�}�`
	// s	�폜����}�`
	void MainPage::ustack_push_remove(Shape* g, Shape* s)
	{
		m_ustack_undo.push_back(new UndoRemoveG(g, s));
	}

	// �}�`���폜����, ���̑�����X�^�b�N�ɐς�.
	// s	�폜����}�`
	void MainPage::ustack_push_remove(Shape* s)
	{
		m_ustack_undo.push_back(new UndoRemove(s));
	}

	// �}�`�̑I���𔽓]����, ���̑�����X�^�b�N�ɐς�.
	// s	�I���𔽓]������}�`.
	void MainPage::ustack_push_select(Shape* s)
	{
		const auto it_end = m_ustack_undo.rend();
		Undo* u;
		for (auto it = m_ustack_undo.rbegin(); it != it_end && (u = *it) != nullptr && typeid(*u) == typeid(UndoSelect); it++) {
			if (u->shape() == s) {
				// ����̐}�`���w�肳�ꂽ�}�`�ƈ�v�����ꍇ,
				// ����X�^�b�N������ɐ}�`�̑I���𔽓]����, 
				// ������X�^�b�N�����菜��, �I������.
				s->set_select(!s->is_selected());
				m_ustack_undo.remove(u);
				return;
			}
		}
		// �}�`�̑I���𔽓]����, ���̑�����X�^�b�N�ɐς�.
		m_ustack_undo.push_back(new UndoSelect(s));
	}
	/*
	void MainPage::ustack_push_select(Shape* s)
	{
		auto it_end = m_ustack_undo.rend();
		// �t����, ����X�^�b�N�̔����q�𓾂�.
		// �����q�̑��삪�I�[�łȂ��ꍇ, �ȉ�����������.
		auto it = m_ustack_undo.rbegin();
		if (it != it_end) {
			if (*it == nullptr) {
				// �X�^�b�N�g�b�v�̑��삪�k���̏ꍇ,
				// �����q�����ɐi�߂�.
				//it++;
			}
			else {
				// �����q�̑��삪�}�`�̑I�𑀍�̏ꍇ, �ȉ����J��Ԃ�.
				bool flag = false;
				Undo* u;
				while (it != it_end && (u = *it) != nullptr && typeid(*u) == typeid(UndoSelect)) {
					if (u->shape() == s) {
						// ����̐}�`���w�肳�ꂽ�}�`�ƈ�v�����ꍇ,
						// �t���O�𗧂ĂĒ��f����.
						flag = true;
						break;
					}
					it++;
				}
				// ���������f���ꂽ�����肷��.
				if (flag) {
					// ����X�^�b�N������ɐ}�`�̑I���𔽓]����, 
					// ������X�^�b�N�����菜��, �I������.
					s->set_select(!s->is_selected());
					m_ustack_undo.remove(*it);
					return;
				}
			}
		}
		// �}�`�̑I���𔽓]����, ���̑�����X�^�b�N�ɐς�.
		m_ustack_undo.push_back(new UndoSelect(s));
	}
	*/

	// �l��}�`�֊i�[����, ���̑�����X�^�b�N�ɐς�.
	// ������, �}�`�����̒l�������Ȃ��ꍇ, �܂��͂��łɓ��l�̏ꍇ�͉������Ȃ�.
	// s	�}�`
	// value	�l
	// �߂�l	�Ȃ�
	template <UNDO_OP U, typename T> void MainPage::ustack_push_set(Shape* const s, T const& value)
	{
		// �}�`�����̒l�������Ȃ��ꍇ, �܂��͂��łɓ��l�̏ꍇ,
		T t_value;
		if (UndoAttr<U>::GET(s, t_value) != true || equal(t_value, value)) {
			// �I������.
			return;
		}
		m_ustack_undo.push_back(new UndoAttr<U>(s, value));
	}

	// �l��I�����ꂽ�}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
	// U	����̎��.
	// T	�i�[����^.
	// value	�i�[����l
	// �߂�l	�i�[�����O�̒l�ƈقȂ��Ă���, �l���i�[���ꂽ�� true.
	template<UNDO_OP U, typename T> bool MainPage::ustack_push_set(T const& value)
	{
		// �i�[����^ T �͖������Ȃ��Ă������̌^���琄��ł���
		m_ustack_undo.push_back(new UndoAttr<U>(&m_sheet_main, value));
		auto flag = false;
		for (auto s : m_sheet_main.m_list_shapes) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			ustack_push_set<U>(s, value);
			flag = true;
		}
		return flag;
	}

	template bool MainPage::ustack_push_set<UNDO_OP::ARROW_SIZE>(ARROW_SIZE const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::ARROW_STYLE>(ARROW_STYLE const& value);
	//template bool MainPage::ustack_push_set<UNDO_OP::IMAGE_ASPECT>(bool const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::IMAGE_OPAC>(float const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::DASH_CAP>(D2D1_CAP_STYLE const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::DASH_PATT>(DASH_PATT const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::DASH_STYLE>(D2D1_DASH_STYLE const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::FILL_COLOR>(D2D1_COLOR_F const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::FONT_COLOR>(D2D1_COLOR_F const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::FONT_FAMILY>(wchar_t* const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::FONT_SIZE>(float const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::FONT_STRETCH>(DWRITE_FONT_STRETCH const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::FONT_STYLE>(DWRITE_FONT_STYLE const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::FONT_WEIGHT>(DWRITE_FONT_WEIGHT const& value);
	template void MainPage::ustack_push_set<UNDO_OP::GRID_BASE>(Shape* const s, float const& value);
	template void MainPage::ustack_push_set<UNDO_OP::GRID_EMPH>(Shape* const s, GRID_EMPH const& value);
	template void MainPage::ustack_push_set<UNDO_OP::GRID_COLOR>(Shape* const s, D2D1_COLOR_F const& value);
	template void MainPage::ustack_push_set<UNDO_OP::GRID_SHOW>(Shape* const s, GRID_SHOW const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::JOIN_LIMIT>(float const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::JOIN_STYLE>(D2D1_LINE_JOIN const& value);
	template void MainPage::ustack_push_set<UNDO_OP::SHEET_COLOR>(Shape* const s, D2D1_COLOR_F const& value);
	template void MainPage::ustack_push_set<UNDO_OP::SHEET_SIZE>(Shape* const s, D2D1_SIZE_F const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::STROKE_CAP>(CAP_STYLE const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::STROKE_COLOR>(D2D1_COLOR_F const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::STROKE_WIDTH>(float const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::TEXT_ALIGN_P>(DWRITE_PARAGRAPH_ALIGNMENT const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT const& value);
	template void MainPage::ustack_push_set<UNDO_OP::TEXT_CONTENT>(Shape* const s, wchar_t* const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::TEXT_LINE_SP>(float const& value);
	template bool MainPage::ustack_push_set<UNDO_OP::TEXT_MARGIN>(D2D1_SIZE_F const& value);

	// �}�`�̒l�̕ۑ������s����, ���̑�����X�^�b�N�ɐς�.
	// s	�l��ۑ�����}�`
	// �߂�l	�Ȃ�
	template <UNDO_OP U> void MainPage::ustack_push_set(Shape* const s)
	{
		m_ustack_undo.push_back(new UndoAttr<U>(s));
	}
	template void MainPage::ustack_push_set<UNDO_OP::POS_START>(Shape* const s);
	template void MainPage::ustack_push_set<UNDO_OP::IMAGE_OPAC>(Shape* const s);

	// �����͈͂̒l��}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
	// �X�^�b�N�̈�ԏ�̑���̑g�̒��ɁA���łɕ����͈͂̑��삪�ς܂�Ă���ꍇ,
	// ���̒l��������������.
	// �����łȂ��ꍇ, �V���ȑ���Ƃ��ăX�^�b�N�ɐς�.
	// s	���삷��}�`
	// value	�����͈͂̒l
	// �߂�l	�Ȃ�
	template<> void MainPage::ustack_push_set<UNDO_OP::TEXT_RANGE, DWRITE_TEXT_RANGE>(Shape* const s, DWRITE_TEXT_RANGE const& value)
	{
		auto flag = false;
		// ���ɖ߂�����X�^�b�N�̊e����ɂ���
		for (auto it = m_ustack_undo.rbegin(); it != m_ustack_undo.rend(); it++) {
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
			m_ustack_undo.push_back(new UndoAttr<UNDO_OP::TEXT_RANGE>(s, value));
		}

	}

	// �f�[�^���[�_�[���瑀��X�^�b�N��ǂݍ���.
	// dt_reader	�f�[�^���[�_�[
	// �߂�l	�Ȃ�
	void MainPage::ustack_read(DataReader const& dt_reader)
	{
		Undo* r;
		while (ustack_read_op(r, dt_reader)) {
			m_ustack_redo.push_back(r);
		}
		m_ustack_rcnt = dt_reader.ReadUInt32();
		Undo* u;
		while (ustack_read_op(u, dt_reader)) {
			m_ustack_undo.push_back(u);
		}
		m_ustack_ucnt = dt_reader.ReadUInt32();
		m_ustack_updt = dt_reader.ReadBoolean();
	}

	// ����X�^�b�N���f�[�^���C�^�[�ɏ�������.
	// dt_writer	�f�[�^���C�^�[
	void MainPage::ustack_write(DataWriter const& dt_writer)
	{
		for (const auto& r : m_ustack_redo) {
			ustack_write_op(r, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::END));
		dt_writer.WriteUInt32(m_ustack_rcnt);
		for (const auto& u : m_ustack_undo) {
			ustack_write_op(u, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::END));
		dt_writer.WriteUInt32(m_ustack_ucnt);
		dt_writer.WriteBoolean(m_ustack_updt);
	}

}