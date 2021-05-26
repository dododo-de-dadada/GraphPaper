#pragma once
//------------------------------
// undo.h
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

	//------------------------------
	// ����X�^�b�N
	//------------------------------
	using U_STACK_T = std::list<struct Undo*>;	// ����X�^�b�N

	//------------------------------
	// ����
	//------------------------------
	enum struct UNDO_OP {
		END = -1,	// ����X�^�b�N�̏I�[ (�t�@�C���̓ǂݏ����Ŏg�p)
		NULLPTR = 0,	// ����̋�؂� (�t�@�C���̓ǂݏ����Ŏg�p)
		ARRANGE,	// �}�`�̏��Ԃ̓���ւ�
		ARROWHEAD_SIZE,	// ���̑傫���̑���
		ARROWHEAD_STYLE,	// ���̌`���̑���
		FILL_COLOR,	// �h��Ԃ��̐F�̑���
		ANCH_POS,	// �}�`�̕��ʂ̈ʒu�̑���
		FONT_COLOR,	// ���̂̐F�̑���
		FONT_FAMILY,	// ���̖��̑���
		FONT_SIZE,	// ���̂̑傫���̑���
		FONT_STYLE,	// ���̂̎��̂̑���
		FONT_STRETCH,	// ���̂̐L�k�̑���
		FONT_WEIGHT,	// ���̂̑����̑���
		GRID_BASE,	// ����̊�̑傳�̑���
		GRID_GRAY,	// ����̐F�̔Z���̑���
		GRID_EMPH,	// ����̌`���̑���
		GRID_SHOW,	// ����̕\�����@�̑���
		GROUP,	// �}�`���O���[�v�ɑ}���܂��͍폜���鑀��
		LIST,	// �}�`�����X�g�ɑ}���܂��͍폜���鑀��
		SHEET_COLOR,	// �p���̐F�̑���
		SHEET_SIZE,	// �p���̐��@�̑���
		SELECT,	// �}�`�̑I����؂�ւ�
		START_POS,	// �}�`�̊J�n�ʒu�̑���
		STROKE_COLOR,	// ���g�̐F�̑���
		STROKE_PATT,	// �j���̔z�u�̑���
		STROKE_STYLE,	// ���g�̌`���̑���
		STROKE_WIDTH,	// ���g�̑����̑���
		TEXT_CONTENT,	// ������̑���
		TEXT_ALIGN_P,	// �i���̐���̑���
		TEXT_ALIGN_T,	// ������̐���̑���
		TEXT_LINE,	// �s�Ԃ̑���
		TEXT_MARGIN,	// ������̗]���̑���
		TEXT_RANGE,	// �����͈͂̑���
	};

	//------------------------------
	// ���삩��l�̌^�𓾂�e���v���[�g
	//------------------------------
	template <UNDO_OP U>
	struct U_TYPE { using type = int; };
	template <> struct U_TYPE<UNDO_OP::ARROWHEAD_SIZE> { using type = ARROWHEAD_SIZE; };
	template <> struct U_TYPE<UNDO_OP::ARROWHEAD_STYLE> { using type = ARROWHEAD_STYLE; };
	template <> struct U_TYPE<UNDO_OP::FILL_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::FONT_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::FONT_FAMILY> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_OP::FONT_SIZE> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::FONT_STRETCH> { using type = DWRITE_FONT_STRETCH; };
	template <> struct U_TYPE<UNDO_OP::FONT_STYLE> { using type = DWRITE_FONT_STYLE; };
	template <> struct U_TYPE<UNDO_OP::FONT_WEIGHT> { using type = DWRITE_FONT_WEIGHT; };
	template <> struct U_TYPE<UNDO_OP::GRID_BASE> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::GRID_GRAY> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::GRID_EMPH> { using type = GRID_EMPH; };
	template <> struct U_TYPE<UNDO_OP::GRID_SHOW> { using type = GRID_SHOW; };
	template <> struct U_TYPE<UNDO_OP::SHEET_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::SHEET_SIZE> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_OP::START_POS> { using type = D2D1_POINT_2F; };
	template <> struct U_TYPE<UNDO_OP::STROKE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_OP::STROKE_PATT> { using type = STROKE_PATT; };
	template <> struct U_TYPE<UNDO_OP::STROKE_STYLE> { using type = D2D1_DASH_STYLE; };
	template <> struct U_TYPE<UNDO_OP::STROKE_WIDTH> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::TEXT_CONTENT> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_OP::TEXT_ALIGN_P> { using type = DWRITE_PARAGRAPH_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_OP::TEXT_ALIGN_T> { using type = DWRITE_TEXT_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_OP::TEXT_LINE> { using type = float; };
	template <> struct U_TYPE<UNDO_OP::TEXT_MARGIN> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_OP::TEXT_RANGE> { using type = DWRITE_TEXT_RANGE; };

	//------------------------------
	// ����̂ЂȌ^
	//------------------------------
	struct Undo {
		// �Q�Ƃ���}�`���X�g
		static S_LIST_T* s_shape_list;
		// �Q�Ƃ���}�`�V�[�g
		static ShapeSheet* s_shape_sheet;
		// ���삷��}�`
		Shape* m_shape;

		// �����j������.
		virtual ~Undo() {}
		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept { return false; }
		// ��������s����.
		virtual void exec(void) {}
		// ������f�[�^���C�^�[����ǂݍ���.
		virtual void read(DataReader const& /*dt_reader*/) {}
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		virtual bool refer_to(const Shape* s) const noexcept { return m_shape == s; };
		// �Q�Ƃ���}�`���X�g�Ɨp���}�`���i�[����.
		static void set(S_LIST_T* s_list, ShapeSheet* s_sheet) noexcept;
		// ���삷��}�`�𓾂�.
		Shape* shape(void) const noexcept { return m_shape; }
		// ������쐬����.
		Undo(Shape* s) : m_shape(s) {}
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& /*dt_writer*/) {}
	};

	//------------------------------
	// �}�`�̕��ʂ̑���
	//------------------------------
	struct UndoAnchor : Undo {
		uint32_t m_anchor;	// ���삳���}�`�̕���
		D2D1_POINT_2F m_anchor_pos;	// �ύX�O��, �}�`�̕��ʂ̈ʒu

		// ��������s����ƒl���ς�邩���肷��.
		bool changed(void) const noexcept;
		// ��������s����.
		void exec(void);
		// ������f�[�^���[�_�[����ǂݍ���.
		UndoAnchor(DataReader const& dt_reader);
		// �}�`�̕��ʂ�ۑ�����.
		UndoAnchor(Shape* const s, const uint32_t anchor);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`�̏��Ԃ����ւ��鑀��
	//------------------------------
	struct UndoArrange2 : Undo {
		Shape* m_dst_shape;	// ����ւ���̐}�`

		// ��������s����ƒl���ς�邩���肷��.
		bool changed(void) const noexcept { return m_shape != m_dst_shape; }
		// ����ւ����̐}�`�𓾂�.
		Shape* const dest(void) const noexcept { return m_dst_shape; }
		// ��������s����.
		void exec(void);
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		bool refer_to(const Shape* s) const noexcept { return Undo::refer_to(s) || m_dst_shape == s; };
		// �}�`�̏��Ԃ����ւ���.
		UndoArrange2(Shape* const s, Shape* const t);
		// ������f�[�^���[�_�[����ǂݍ���.
		UndoArrange2(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`�̒l��ۑ����ĕύX���鑀��.
	//------------------------------
	template <UNDO_OP U>
	struct UndoAttr : Undo, U_TYPE<U> {
		U_TYPE<U>::type m_value;	// // �ύX�����O�̒l

		~UndoAttr() {};
		// ��������s����ƒl���ς�邩���肷��.
		bool changed(void) const noexcept;
		// ��������s����.
		void exec(void);
		// �l��}�`���瓾��.
		static bool GET(const Shape* s, U_TYPE<U>::type& value) noexcept;
		// �l��}�`�Ɋi�[����.
		static void SET(Shape* const s, const U_TYPE<U>::type& value);
		// �f�[�^���[�_�[���瑀���ǂݍ���ō쐬����.
		UndoAttr(DataReader const& dt_reader);
		// �}�`�̒l��ۑ�����.
		UndoAttr(Shape* s);
		// �}�`�̒l��ۑ����ĕύX����.
		UndoAttr(Shape* s, const U_TYPE<U>::type& value);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`�����X�g�ɑ}���܂��͍폜���鑀��.
	//------------------------------
	struct UndoList : Undo {
		bool m_insert;	// �}���t���O
		Shape* m_shape_at;	// �ύX�O��, ���삳���ʒu�ɂ������}�`

		// ��������s����ƒl���ς�邩���肷��.
		bool changed(void) const noexcept { return true; }
		// ��������s����.
		void exec(void);
		// ���삪�}�������肷��.
		bool is_insert(void) const noexcept { return m_insert; }
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		bool refer_to(const Shape* s) const noexcept { return Undo::refer_to(s) || m_shape_at == s; };
		// ���삳���ʒu�ɂ������}�`�𓾂�.
		Shape* const shape_at(void) const noexcept { return m_shape_at; }
		// ������f�[�^���[�_�[����ǂݍ���.
		UndoList(DataReader const& dt_reader);
		// �}�`�����X�g�����菜��.
		UndoList(Shape* const s, const bool dont_exec = false);
		// �}�`�����X�g�ɑ}������.
		UndoList(Shape* const s, Shape* const s_pos, const bool dont_exec = false);
		// ������f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`���O���[�v�ɒǉ��܂��͍폜���鑀��.
	//------------------------------
	struct UndoListGroup : UndoList {
		// ���삷��O���[�v
		ShapeGroup* m_shape_group;

		// ��������s����.
		void exec(void);
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		bool refer_to(const Shape* s) const noexcept { return UndoList::refer_to(s) || m_shape_group == s; };
		// ������f�[�^���[�_�[����ǂݍ���.
		UndoListGroup(DataReader const& dt_reader);
		// �}�`���O���[�v����폜����.
		UndoListGroup(ShapeGroup* const g, Shape* const s);
		// �}�`���O���[�v�ɒǉ�����.
		UndoListGroup(ShapeGroup* const g, Shape* const s, Shape* const s_pos);
		// ������f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`�̑I���𔽓]���鑀��
	//------------------------------
	struct UndoSelect : Undo {
		// ��������s����ƒl���ς�邩���肷��.
		bool changed(void) const noexcept { return true; }
		// ��������s����.
		void exec(void);
		// ������f�[�^���[�_�[����ǂݍ���.
		UndoSelect(DataReader const& dt_reader);
		// �}�`�̑I���𔽓]����.
		UndoSelect(Shape* const s);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	// ������̑����j������.
	template <> UndoAttr<UNDO_OP::TEXT_CONTENT>::~UndoAttr() 
	{
		if (m_value != nullptr) {
			delete[] m_value;
			m_value = nullptr;
		}
	}
}