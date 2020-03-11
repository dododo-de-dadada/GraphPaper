#pragma once
//------------------------------
// undo.h
//
// undo.cpp
//
// ���ɖ߂� / ��蒼������
// �}�`�̒ǉ���폜, �ύX��, �u����v��ʂ��čs����.
//------------------------------
#include <list>
#include <winrt/Windows.Storage.Streams.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;

	// �}�`�����X�g�ɒǉ�����.
#define UndoAppend(s)	UndoList(static_cast<Shape*>(s), static_cast<Shape*>(nullptr))
// �}�`�����X�g�ɑ}������.
#define UndoInsert(s, p)	UndoList(static_cast<Shape*>(s), static_cast<Shape*>(p))
// �}�`�����X�g�����菜��.
#define UndoRemove(s)	UndoList(static_cast<Shape*>(s))
// �}�`���O���[�v�ɒǉ�����.
#define UndoAppendG(g, s)		UndoListG(static_cast<ShapeGroup*>(g), static_cast<Shape*>(s), static_cast<Shape*>(nullptr))
// �}�`���O���[�v�����菜��.
#define UndoRemoveG(g, s)		UndoListG(static_cast<ShapeGroup*>(g), static_cast<Shape*>(s))

	//------------------------------
	// ����
	//------------------------------
	enum struct UNDO_OP {
		END = -1,	// ����X�^�b�N�̏I�[ (�t�@�C���̓ǂݏ����Ŏg�p)
		NULLPTR = 0,	// ����̋�؂� (�t�@�C���̓ǂݏ����Ŏg�p)
		ARRANGE,	// �}�`�̏��Ԃ̓���ւ�
		ARROW_SIZE,	// ���̑傫���𑀍�
		ARROW_STYLE,	// ���̌`���𑀍�
		FILL_COLOR,	// �h��Ԃ��̐F�𑀍�
		FORM,	// �}�`�̕ό`�𑀍�
		FONT_COLOR,	// ���̂̐F�𑀍�
		FONT_FAMILY,	// ���̖��𑀍�
		FONT_SIZE,	// ���̂̑傫���𑀍�
		FONT_STYLE,	// ���̂̎��̂𑀍�
		FONT_STRETCH,	// ���̂̐L�k�𑀍�
		FONT_WEIGHT,	// ���̂̑����𑀍�
		GRID_LEN,	// ����̑傳�𑀍�
		GRID_OPAC,	// ����̐F�̔Z���𑀍�
		GRID_SHOW,	// ����̕\�����@�𑀍�
		GROUP,	// �}�`���O���[�v�ɑ}���܂��͍폜���鑀��
		LIST,	// �}�`�����X�g�ɑ}���܂��͍폜���鑀��
		PAGE_COLOR,	// �y�[�W�̐F�𑀍�
		PAGE_SIZE,	// �y�[�W�̐��@�𑀍�
		SELECT,	// �}�`�̑I����؂�ւ�
		START_POS,	// �}�`�̊J�n�ʒu�𑀍�
		STROKE_COLOR,	// ���g�̐F�𑀍�
		STROKE_PATTERN,	// �j���̔z�u�𑀍�
		STROKE_STYLE,	// ���g�̌`���𑀍�
		STROKE_WIDTH,	// ���g�̑����𑀍�
		TEXT,	// ������𑀍�
		TEXT_ALIGN_P,	// �i���̐���𑀍�
		TEXT_ALIGN_T,	// ������̐���𑀍�
		TEXT_LINE,	// �s�Ԃ𑀍�
		TEXT_MARGIN,	// ������̗]���𑀍�
		TEXT_RANGE,	// ������͈̔͑I���𑀍�
	};

	//------------------------------
	// ����X�^�b�N����������֐�.
	//------------------------------
	typedef std::list<struct Undo*> U_STACK_T;	// ����X�^�b�N

	//------------------------------
	// ����̂ЂȌ^
	//------------------------------
	struct Undo {
		// �Q�Ƃ���}�`���X�g
		static S_LIST_T* s_shape_list;
		// �Q�Ƃ���y�[�W�}�`
		static ShapePanel* s_shape_page;
		// ���삷��}�`
		Shape* m_shape;

		// �����j������.
		virtual ~Undo() {}
		// ��������s����ƒl���ς�邩���ׂ�.
		virtual bool changed(void) const noexcept { return false; }
		// ��������s����.
		virtual void exec(void) {}
		// ������f�[�^���C�^�[����ǂݍ���.
		virtual void read(DataReader const& /*dt_reader*/) {}
		// �}�`���Q�Ƃ��Ă��邩���ׂ�.
		virtual bool refer_to(const Shape* s) const noexcept { return m_shape == s; };
		// �Q�Ƃ���}�`���X�g�ƃy�[�W�}�`���i�[����.
		static void set(S_LIST_T* s_list, ShapePanel* s_page) noexcept;
		// ���삷��}�`�𓾂�.
		Shape* shape(void) const noexcept { return m_shape; }
		// ������쐬����.
		Undo(Shape* s) : m_shape(s) {}
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& /*dt_writer*/) {}
	};

	//------------------------------
	// �}�`�̏��Ԃ����ւ��鑀��
	//------------------------------
	struct UndoArrange2 : Undo {
		// ����ւ���̐}�`
		Shape* m_s_dst;

		// ��������s����ƒl���ς�邩���ׂ�.
		bool changed(void) const noexcept { return m_shape != m_s_dst; }
		// ����ւ����̐}�`�𓾂�.
		Shape* dest(void) const noexcept { return m_s_dst; }
		// ��������s����.
		void exec(void);
		// �}�`���Q�Ƃ��Ă��邩���ׂ�.
		bool refer_to(const Shape* s) const noexcept { return Undo::refer_to(s) || m_s_dst == s; };
		// �}�`�̏��Ԃ����ւ���.
		UndoArrange2(Shape* s, Shape* t);
		// ������f�[�^���[�_�[����ǂݍ���.
		UndoArrange2(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`�����X�g�ɑ}���܂��͍폜���鑀��.
	//------------------------------
	struct UndoList : Undo {
		// �}���t���O
		bool m_insert;
		// ���삷��ʒu
		Shape* m_s_pos;

		// ��������s����ƒl���ς�邩���ׂ�.
		bool changed(void) const noexcept { return true; }
		// ��������s����.
		void exec(void);
		// ���삪�}�������ׂ�.
		bool is_insert(void) const noexcept { return m_insert; }
		// �}�`���Q�Ƃ��Ă��邩���ׂ�.
		bool refer_to(const Shape* s) const noexcept { return Undo::refer_to(s) || m_s_pos == s; };
		// ���삷��ʒu�𓾂�.
		Shape* shape_pos(void) const noexcept { return m_s_pos; }
		// ������f�[�^���[�_�[����ǂݍ���.
		UndoList(DataReader const& dt_reader);
		// �}�`�����X�g�����菜��.
		UndoList(Shape* s, const bool dont = false);
		// �}�`�����X�g�ɑ}������.
		UndoList(Shape* s, Shape* s_pos, const bool dont = false);
		// ������f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`���O���[�v�ɑ}���܂��͍폜���鑀��.
	//------------------------------
	struct UndoListG : UndoList {
		// ���삷��O���[�v�̈ʒu
		ShapeGroup* m_g_pos;

		// ��������s����.
		void exec(void);
		// ���삷��O���[�v�̈ʒu�𓾂�.
		Shape* group_pos(void) { return m_g_pos; }
		// �}�`���Q�Ƃ��Ă��邩���ׂ�.
		bool refer_to(const Shape* s) const noexcept { return UndoList::refer_to(s) || m_g_pos == s; };
		// ������f�[�^���[�_�[����ǂݍ���.
		UndoListG(DataReader const& dt_reader);
		// �}�`���O���[�v�����菜��.
		UndoListG(ShapeGroup* g, Shape* s);
		// �}�`���O���[�v�ɒǉ�����.
		UndoListG(ShapeGroup* g, Shape* s, Shape* s_pos);
		// ������f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`�̑I���𔽓]���鑀��
	//------------------------------
	struct UndoSelect : Undo {
		// ��������s����ƒl���ς�邩���ׂ�.
		bool changed(void) const noexcept { return true; }
		// ��������s����.
		void exec(void);
		// ������f�[�^���[�_�[����ǂݍ���.
		UndoSelect(DataReader const& dt_reader);
		// �}�`�̑I���𔽓]����.
		UndoSelect(Shape* s);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`��ό`���鑀��
	//------------------------------
	struct UndoForm : Undo {
		// �ύX�����}�`�̕���
		ANCH_WHICH m_anchor;
		// �}�`�̕��ʂ̈ʒu
		D2D1_POINT_2F m_a_pos;

		// ��������s����ƒl���ς�邩���ׂ�.
		bool changed(void) const noexcept;
		// ��������s����.
		void exec(void);
		// ������f�[�^���[�_�[����ǂݍ���.
		UndoForm(DataReader const& dt_reader);
		// �}�`�̕��ʂ̈ʒu��ۑ�����.
		UndoForm(Shape* s, const ANCH_WHICH a);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// ����̎�ނ���^�𓾂�e���v���[�g
	//------------------------------
	template <UNDO_OP U>
	struct U_TYPE {
		using type = int;
	};
	template <> struct U_TYPE<UNDO_OP::ARROW_SIZE> { using type = ARROW_SIZE; };
	template <> struct U_TYPE<UNDO_OP::ARROW_STYLE> { using type = ARROW_STYLE; };
	template <> struct U_TYPE<UNDO_OP::FILL_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::FONT_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::FONT_FAMILY> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_OP::FONT_SIZE> { using type = double; };
	template <> struct U_TYPE<UNDO_OP::FONT_STRETCH> { using type = DWRITE_FONT_STRETCH; };
	template <> struct U_TYPE<UNDO_OP::FONT_STYLE> { using type = DWRITE_FONT_STYLE; };
	template <> struct U_TYPE<UNDO_OP::FONT_WEIGHT> { using type = DWRITE_FONT_WEIGHT; };
	template <> struct U_TYPE<UNDO_OP::GRID_LEN> { using type = double; };
	template <> struct U_TYPE<UNDO_OP::GRID_OPAC> { using type = double; };
	template <> struct U_TYPE<UNDO_OP::GRID_SHOW> { using type = GRID_SHOW; };
	template <> struct U_TYPE<UNDO_OP::TEXT_LINE> { using type = double; };
	template <> struct U_TYPE<UNDO_OP::PAGE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::PAGE_SIZE> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_OP::START_POS> { using type = D2D1_POINT_2F; };
	template <> struct U_TYPE<UNDO_OP::STROKE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::STROKE_PATTERN> { using type = STROKE_PATTERN; };
	template <> struct U_TYPE<UNDO_OP::STROKE_STYLE> { using type = D2D1_DASH_STYLE; };
	template <> struct U_TYPE<UNDO_OP::STROKE_WIDTH> { using type = double; };
	template <> struct U_TYPE<UNDO_OP::TEXT> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_OP::TEXT_ALIGN_P> { using type = DWRITE_PARAGRAPH_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_OP::TEXT_ALIGN_T> { using type = DWRITE_TEXT_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_OP::TEXT_MARGIN> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_OP::TEXT_RANGE> { using type = DWRITE_TEXT_RANGE; };

	//------------------------------
	// �}�`�̑����̒l��ۑ����ĕύX���鑀��.
	//------------------------------
	template <UNDO_OP U>
	struct UndoSet : Undo, U_TYPE<U> {
		// �����̒l
		U_TYPE<U>::type m_val;

		//------------------------------
		// undo_push_value.cpp
		//------------------------------

		// ��������s����ƒl���ς�邩���ׂ�.
		bool changed(void) const noexcept;
		// ��������s����.
		void exec(void);
		// �}�`�̑����l����l�𓾂�.
		static bool GET(Shape* s, U_TYPE<U>::type& t_val) noexcept;
		// �}�`�̑����l�ɒl���i�[����.
		static void SET(Shape* s, const U_TYPE<U>::type& t_val);
		// �f�[�^���[�_�[���瑀���ǂݍ���ō쐬����.
		UndoSet(DataReader const& dt_reader);
		// �}�`�̑����l��ۑ�����.
		UndoSet(Shape* s);
		// �}�`�̑����l��ۑ��������ƒl���i�[����.
		UndoSet(Shape* s, const U_TYPE<U>::type& t_val);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

}