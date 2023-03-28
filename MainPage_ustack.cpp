//------------------------------
// MainPage_ustack.cpp
// ���ɖ߂��Ƃ�蒼������X�^�b�N
//------------------------------
#include "pch.h"
#include "MainPage.h"

using namespace winrt;

// �}�`���O���[�v�ɒǉ�����.
#define UndoAppendG(g, s)	UndoGroup(static_cast<ShapeGroup* const>(g), static_cast<Shape* const>(s), static_cast<Shape* const>(nullptr))
// �}�`�����X�g�ɒǉ�����.
#define UndoAppend(s)	UndoList(static_cast<Shape* const>(s), static_cast<Shape* const>(nullptr))
// �}�`�����X�g�ɑ}������.
#define UndoInsert(s, p)	UndoList(static_cast<Shape* const>(s), static_cast<Shape* const>(p))
// �}�`���O���[�v�����菜��.
#define UndoRemoveG(g, s)	UndoGroup(static_cast<ShapeGroup* const>(g), static_cast<Shape* const>(s))
// �}�`�����X�g�����菜��.
#define UndoRemove(s)	UndoList(static_cast<Shape* const>(s))

namespace winrt::GraphPaper::implementation
{
	// �k���ŋ�؂����A�̑����, ����̑g�Ƃ݂Ȃ�, ���̐���g���Ƃ���.
	// �X�^�b�N�ɐςނ��Ƃ��ł���ő�̑g��.
	constexpr uint32_t MAX_UCNT = 32;

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	static uint32_t ustack_clear_stack(UNDO_STACK& ustack);
	// �f�[�^���[�_�[���瑀���ǂݍ���.
	static bool ustack_read_op(Undo*& u, DataReader const& dt_reader);
	// ������f�[�^���[�_�[�ɏ�������.
	static void ustack_write_op(Undo* u, DataWriter const& dt_writer);

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	// ustack	����X�^�b�N
	// �߂�l	������������̑g��
	static uint32_t ustack_clear_stack(UNDO_STACK& ustack)
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
	static bool ustack_read_op(Undo*& u, DataReader const& dt_reader)
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
		//case UNDO_T::IMAGE_ASPECT:
		//	u = new UndoValue<UNDO_T::IMAGE_ASPECT>(dt_reader);
		//	break;
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
		case UNDO_T::PAGE_COLOR:
			u = new UndoValue<UNDO_T::PAGE_COLOR>(dt_reader);
			break;
		case UNDO_T::PAGE_SIZE:
			u = new UndoValue<UNDO_T::PAGE_SIZE>(dt_reader);
			break;
		case UNDO_T::PAGE_PAD:
			u = new UndoValue<UNDO_T::PAGE_PAD>(dt_reader);
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
		case UNDO_T::DASH_CAP:
			u = new UndoValue<UNDO_T::DASH_CAP>(dt_reader);
			break;
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
		case UNDO_T::TEXT_PAR_ALIGN:
			u = new UndoValue<UNDO_T::TEXT_PAR_ALIGN>(dt_reader);
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
		case UNDO_T::TEXT_RANGE:
			u = new UndoValue<UNDO_T::TEXT_RANGE>(dt_reader);
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
	static void ustack_write_op(Undo* u, DataWriter const& dt_writer)
	{
		if (u != nullptr) {
			u->write(dt_writer);
		}
		else {
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::NIL));
		}
	}

	// �ҏW���j���[�́u��蒼���v���I�����ꂽ.
	void MainPage::ustack_redo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_ustack_rcnt > 0) {
			auto flag = false;
			// ���ɖ߂�����X�^�b�N����k���ŋ�؂��Ă��Ȃ� (�I���Ȃǂ�) �������菜��.
			while (!m_ustack_undo.empty()) {
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
			if (flag) {
				xcvd_is_enabled();
				page_bbox_update();
				main_panel_size();
				page_draw();
				// �ꗗ���\������Ă邩���肷��.
				if (summary_is_visible()) {
					summary_update();
				}
			}
		}
		status_bar_set_pos();
	}

	// ����X�^�b�N��������, �܂܂�鑀���j������.
	void MainPage::ustack_clear(void)
	{
		m_ustack_is_changed = false;
		m_ustack_rcnt -= ustack_clear_stack(m_ustack_redo);
		m_ustack_ucnt -= ustack_clear_stack(m_ustack_undo);
#if defined(_DEBUG)
		if (m_ustack_rcnt == 0 && m_ustack_ucnt == 0) {
			return;
		}
		//using winrt::Windows::UI::Xaml::Controls::ContentDialog;
		ContentDialog dialog;
		dialog.Title(box_value(L"Undo is not empty."));
		dialog.CloseButtonText(L"Close");
		auto _{ dialog.ShowAsync() };
#endif
	}

	// �ҏW���j���[�́u���ɖ߂��v���I�����ꂽ.
	void MainPage::ustack_undo_click(IInspectable const&, RoutedEventArgs const&)
	{
		if (m_ustack_ucnt > 0) {
			auto st = 0;
			while (!m_ustack_undo.empty()) {
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
			page_bbox_update();
			main_panel_size();
			page_draw();
			// �ꗗ���\������Ă邩���肷��.
			if (summary_is_visible()) {
				summary_update();
			}
		}
		status_bar_set_pos();
	}

	// ��������s����.
	// u	����
	void MainPage::ustack_exec(Undo* u)
	{
		summary_reflect(u);
		u->exec();
		auto const& u_type = typeid(*u);
		if (u_type == typeid(UndoValue<UNDO_T::ARROW_STYLE>)) {
			// ���g���j���[�́u��邵�̌`���v�Ɉ������.
			ARROW_STYLE val;
			m_main_page.get_arrow_style(val);
			arrow_style_is_checked(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::FONT_STYLE>)) {
			// ���̃��j���[�́u���́v�Ɉ������.
			DWRITE_FONT_STYLE val;
			m_main_page.get_font_style(val);
			font_style_is_checked(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::GRID_BASE>)) {
			// ����̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			status_bar_set_grid();
		}
		else if (u_type == typeid(UndoValue<UNDO_T::GRID_EMPH>)) {
			// ���C�A�E�g���j���[�́u����̋����v�Ɉ������.
			GRID_EMPH val;
			m_main_page.get_grid_emph(val);
			grid_emph_is_checked(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::GRID_SHOW>)) {
			GRID_SHOW val;
			m_main_page.get_grid_show(val);
			grid_show_is_checked(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::PAGE_SIZE>)) {
			// �y�[�W�̑傫�����X�e�[�^�X�o�[�Ɋi�[����.
			status_bar_set_page();
		}
		else if (u_type == typeid(UndoValue<UNDO_T::STROKE_CAP>)) {
			CAP_STYLE val;
			m_main_page.get_stroke_cap(val);
			cap_style_is_checked(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::DASH_CAP>)) {
			D2D1_CAP_STYLE val;
			m_main_page.get_dash_cap(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::DASH_STYLE>)) {
			D2D1_DASH_STYLE val;
			m_main_page.get_dash_style(val);
			dash_style_is_checked(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::JOIN_STYLE>)) {
			D2D1_LINE_JOIN val;
			m_main_page.get_join_style(val);
			join_style_is_checked(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::STROKE_WIDTH>)) {
			float val;
			m_main_page.get_stroke_width(val);
			stroke_width_is_checked(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::TEXT_ALIGN_T>)) {
			DWRITE_TEXT_ALIGNMENT val;
			m_main_page.get_text_align_horz(val);
			text_align_horz_is_checked(val);
		}
		else if (u_type == typeid(UndoValue<UNDO_T::TEXT_PAR_ALIGN>)) {
			DWRITE_PARAGRAPH_ALIGNMENT val;
			m_main_page.get_text_align_vert(val);
			text_align_vert_is_checked(val);
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

	// �}�`�̌`���X�^�b�N�ɕۑ�����.
	// s	�ۑ�����}�`
	// anc	�}�`�̕���
	void MainPage::ustack_push_position(Shape* const s, const uint32_t anc)
	{
		m_ustack_undo.push_back(new UndoDeform(s, anc));
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
	void MainPage::ustack_push_order(Shape* const s, Shape* const t)
	{
		m_ustack_undo.push_back(new UndoOrder(s, t));
	}

	// �}�`��}������, ���̑�����X�^�b�N�ɐς�.
	void MainPage::ustack_push_insert(Shape* s, Shape* s_pos)
	{
		m_ustack_undo.push_back(new UndoInsert(s, s_pos));
	}

	// �I�����ꂽ (���邢�͑S�Ă�) �}�`�̈ʒu���X�^�b�N�ɕۑ����Ă��獷�������ړ�����.
	// pos	�ʒu�x�N�g��
	// any	�}�`���ׂĂ̏ꍇ true, �I�����ꂽ�}�`�݂̂̏ꍇ false
	void MainPage::ustack_push_move(const D2D1_POINT_2F pos, const bool any)
	{
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted() || (!any && !s->is_selected())) {
				continue;
			}
			ustack_push_set<UNDO_T::MOVE>(s);
			s->move(pos);
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
		m_ustack_is_changed = true;
		// ����̑g�����C���N�������g����.
		m_ustack_ucnt++;
		// ����̑g�����ő�̑g���ȉ������肷��.
		if (m_ustack_ucnt <= MAX_UCNT) {
			return;
		}
		// ���ɖ߂�����X�^�b�N����łȂ������肷��.
		while (!m_ustack_undo.empty()) {
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

	// �}�`���O���[�v�����苎��, ���̑�����X�^�b�N�ɐς�.
	// g	�O���[�v�}�`
	// s	��苎��}�`
	void MainPage::ustack_push_remove(Shape* g, Shape* s)
	{
		m_ustack_undo.push_back(new UndoRemoveG(g, s));
	}

	// �}�`����苎��, ���̑�����X�^�b�N�ɐς�.
	// s	��苎��}�`
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
		for (auto it = m_ustack_undo.rbegin();
			it != it_end && (u = *it) != nullptr && typeid(*u) == typeid(UndoSelect); it++) {
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

	// �l��}�`�֊i�[����, ���̑�����X�^�b�N�ɐς�.
	// ������, �}�`�����̒l�������Ȃ��ꍇ, �܂��͂��łɓ��l�̏ꍇ�͉������Ȃ�.
	// s	�}�`
	// val	�l
	// �߂�l	�Ȃ�
	template <UNDO_T U, typename T> void MainPage::ustack_push_set(Shape* const s, T const& val)
	{
		// �}�`�����̒l�������Ȃ��ꍇ, �܂��͂��łɓ��l�̏ꍇ,
		T t_val;
		if (!UndoValue<U>::GET(s, t_val) || equal(t_val, val)) {
			// �I������.
			return;
		}
		m_ustack_undo.push_back(new UndoValue<U>(s, val));
	}

	// �l��I�����ꂽ�}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
	// U	����̎��ʎq.
	// T	�i�[����^.
	// val	�i�[����l
	// �߂�l	�i�[�����O�̒l�ƈقȂ��Ă���, �l���i�[���ꂽ�� true.
	template<UNDO_T U, typename T> bool MainPage::ustack_push_set(T const& val)
	{
		// ���C���y�[�W�ɑ�������Ȃ�, ���C���y�[�W�̑������ύX����.
		T page_val;
		if (UndoValue<U>::GET(&m_main_page, page_val)) {
			m_ustack_undo.push_back(new UndoValue<U>(&m_main_page, val));
		}
		auto flag = false;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted() || !s->is_selected()) {
				continue;
			}
			ustack_push_set<U>(s, val);
			flag = true;
		}
		return flag;
	}

	// �}�`�̒l�̕ۑ������s����, ���̑�����X�^�b�N�ɐς�.
	// U	����̎��.
	// s	�l��ۑ�����}�`
	// �߂�l	�Ȃ�
	template <UNDO_T U> void MainPage::ustack_push_set(Shape* const s)
	{
		m_ustack_undo.push_back(new UndoValue<U>(s));
	}

	template bool MainPage::ustack_push_set<UNDO_T::ARC_DIR>(D2D1_SWEEP_DIRECTION const& val);
	template bool MainPage::ustack_push_set<UNDO_T::ARC_END>(float const& val);
	template bool MainPage::ustack_push_set<UNDO_T::ARC_ROT>(float const& val);
	template bool MainPage::ustack_push_set<UNDO_T::ARC_START>(float const& val);
	template bool MainPage::ustack_push_set<UNDO_T::ARROW_SIZE>(ARROW_SIZE const& val);
	template bool MainPage::ustack_push_set<UNDO_T::ARROW_STYLE>(ARROW_STYLE const& val);
	template bool MainPage::ustack_push_set<UNDO_T::IMAGE_OPAC>(float const& val);
	template bool MainPage::ustack_push_set<UNDO_T::DASH_CAP>(D2D1_CAP_STYLE const& val);
	template bool MainPage::ustack_push_set<UNDO_T::DASH_PAT>(DASH_PAT const& val);
	template bool MainPage::ustack_push_set<UNDO_T::DASH_STYLE>(D2D1_DASH_STYLE const& val);
	template bool MainPage::ustack_push_set<UNDO_T::FILL_COLOR>(D2D1_COLOR_F const& val);
	template bool MainPage::ustack_push_set<UNDO_T::FONT_COLOR>(D2D1_COLOR_F const& val);
	template bool MainPage::ustack_push_set<UNDO_T::FONT_FAMILY>(wchar_t* const& val);
	template bool MainPage::ustack_push_set<UNDO_T::FONT_SIZE>(float const& val);
	template bool MainPage::ustack_push_set<UNDO_T::FONT_STRETCH>(DWRITE_FONT_STRETCH const& val);
	template bool MainPage::ustack_push_set<UNDO_T::FONT_STYLE>(DWRITE_FONT_STYLE const& val);
	template bool MainPage::ustack_push_set<UNDO_T::FONT_WEIGHT>(DWRITE_FONT_WEIGHT const& val);
	template bool MainPage::ustack_push_set<UNDO_T::JOIN_LIMIT>(float const& val);
	template bool MainPage::ustack_push_set<UNDO_T::JOIN_STYLE>(D2D1_LINE_JOIN const& val);
	template bool MainPage::ustack_push_set<UNDO_T::POLY_END>(bool const& val);
	template bool MainPage::ustack_push_set<UNDO_T::STROKE_CAP>(CAP_STYLE const& val);
	template bool MainPage::ustack_push_set<UNDO_T::STROKE_COLOR>(D2D1_COLOR_F const& val);
	template bool MainPage::ustack_push_set<UNDO_T::STROKE_WIDTH>(float const& val);
	template bool MainPage::ustack_push_set<UNDO_T::TEXT_PAR_ALIGN>(DWRITE_PARAGRAPH_ALIGNMENT const& val);
	template bool MainPage::ustack_push_set<UNDO_T::TEXT_ALIGN_T>(DWRITE_TEXT_ALIGNMENT const& val);
	template bool MainPage::ustack_push_set<UNDO_T::TEXT_LINE_SP>(float const& val);
	template bool MainPage::ustack_push_set<UNDO_T::TEXT_PAD>(D2D1_SIZE_F const& val);

	template void MainPage::ustack_push_set<UNDO_T::GRID_BASE>(Shape* const s, float const& val);
	template void MainPage::ustack_push_set<UNDO_T::GRID_EMPH>(Shape* const s, GRID_EMPH const& val);
	template void MainPage::ustack_push_set<UNDO_T::GRID_COLOR>(Shape* const s, D2D1_COLOR_F const& val);
	template void MainPage::ustack_push_set<UNDO_T::GRID_SHOW>(Shape* const s, GRID_SHOW const& val);
	template void MainPage::ustack_push_set<UNDO_T::PAGE_COLOR>(Shape* const s, D2D1_COLOR_F const& val);
	template void MainPage::ustack_push_set<UNDO_T::PAGE_SIZE>(Shape* const s, D2D1_SIZE_F const& val);
	template void MainPage::ustack_push_set<UNDO_T::PAGE_PAD>(Shape* const s, D2D1_RECT_F const& val);
	template void MainPage::ustack_push_set<UNDO_T::TEXT_CONTENT>(Shape* const s, wchar_t* const& val);

	template void MainPage::ustack_push_set<UNDO_T::MOVE>(Shape* const s);
	template void MainPage::ustack_push_set<UNDO_T::IMAGE_OPAC>(Shape* const s);

	// ���S���ꉻ
	// �I������Ă���l�����~����]��, ���̑�����X�^�b�N�ɐς�.
	/*
	template<> bool MainPage::ustack_push_set<UNDO_T::ARC_ROT, float>(float const& val)
	{
		auto flag = false;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted() || !s->is_selected() || 
				typeid(*s) != typeid(ShapeArc)) {
				continue;
			}
			m_ustack_undo.push_back(new UndoValue<UNDO_T::ARC_ROT>(s, val));
			flag = true;
		}
		return flag;
	}
	*/

	// ���S���ꉻ
	// �I������Ă���l�����~����]��, ���̑�����X�^�b�N�ɐς�.
	/*
	template<> bool MainPage::ustack_push_set<UNDO_T::ARC_START, float>(float const& val)
	{
		auto flag = false;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted() || !s->is_selected() ||
				typeid(*s) != typeid(ShapeArc)) {
				continue;
			}
			m_ustack_undo.push_back(new UndoValue<UNDO_T::ARC_START>(s, val));
			flag = true;
		}
		return flag;
	}
	*/

	// ���S���ꉻ
	// �I������Ă���l�����~����]��, ���̑�����X�^�b�N�ɐς�.
	/*
	template<> bool MainPage::ustack_push_set<UNDO_T::ARC_END, float>(float const& val)
	{
		auto flag = false;
		for (auto s : m_main_page.m_shape_list) {
			if (s->is_deleted() || !s->is_selected() ||
				typeid(*s) != typeid(ShapeArc)) {
				continue;
			}
			m_ustack_undo.push_back(new UndoValue<UNDO_T::ARC_END>(s, val));
			flag = true;
		}
		return flag;
	}
	*/

	// ���S���ꉻ
	// �����͈͂̒l��}�`�Ɋi�[����, ���̑�����X�^�b�N�ɐς�.
	// �X�^�b�N�̈�ԏ�̑���̑g�̒��ɁA���łɕ����͈͂̑��삪�ς܂�Ă���ꍇ,
	// ���̒l��������������.
	// �����łȂ��ꍇ, �V���ȑ���Ƃ��ăX�^�b�N�ɐς�.
	// s	���삷��}�`
	// val	�����͈͂̒l
	// �߂�l	�Ȃ�
	template<> void MainPage::ustack_push_set<UNDO_T::TEXT_RANGE, DWRITE_TEXT_RANGE>(Shape* const s, DWRITE_TEXT_RANGE const& val)
	{
		auto flag = false;
		// ���ɖ߂�����X�^�b�N�̊e����ɂ���
		for (auto it = m_ustack_undo.rbegin(); it != m_ustack_undo.rend(); it++) {
			const auto u = *it;
			if (u == nullptr) {
				// ���삪�k���̏ꍇ,
				break;
			}
			else if (typeid(*u) != typeid(UndoValue<UNDO_T::TEXT_RANGE>)) {
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
				s->set_text_selected(val);
				flag = true;
				break;
			}
		}
		if (!flag) {
			m_ustack_undo.push_back(new UndoValue<UNDO_T::TEXT_RANGE>(s, val));
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
		m_ustack_is_changed = dt_reader.ReadBoolean();
	}

	// ����X�^�b�N���f�[�^���C�^�[�ɏ�������.
	// dt_writer	�f�[�^���C�^�[
	void MainPage::ustack_write(DataWriter const& dt_writer)
	{
		for (const auto& r : m_ustack_redo) {
			ustack_write_op(r, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::END));
		dt_writer.WriteUInt32(m_ustack_rcnt);
		for (const auto& u : m_ustack_undo) {
			ustack_write_op(u, dt_writer);
		}
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::END));
		dt_writer.WriteUInt32(m_ustack_ucnt);
		dt_writer.WriteBoolean(m_ustack_is_changed);
	}

}