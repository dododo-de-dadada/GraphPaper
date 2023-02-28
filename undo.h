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
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	//------------------------------
	// ����̎��ʎq
	//------------------------------
	enum struct UNDO_ID : uint32_t {
		END = static_cast<uint32_t>(-1),	// ����X�^�b�N�̏I�[ (�t�@�C���ǂݏ����Ŏg�p)
		NIL = 0,	// ����̋�؂� (�t�@�C���ǂݏ����Ŏg�p)
		ARROW_SIZE,	// ��邵�̑傫���̑���
		ARROW_STYLE,	// ��邵�̌`���̑���
		DASH_CAP,	// �j���̒[�̌`���̑���
		DASH_PATT,	// �j���̔z�u�̑���
		DASH_STYLE,	// �j���̌`���̑���
		FILL_COLOR,	// �h��Ԃ��̐F�̑���
		FONT_COLOR,	// ���̂̐F�̑���
		FONT_FAMILY,	// ���̖��̑���
		FONT_SIZE,	// ���̂̑傫���̑���
		FONT_STRETCH,	// ���̂̕��̑���
		FONT_STYLE,	// ���̂̎��̂̑���
		FONT_WEIGHT,	// ���̂̑����̑���
		GRID_BASE,	// ����̊�̑傳�̑���
		GRID_COLOR,	// ����̐F�̑���
		GRID_EMPH,	// ����̌`���̑���
		GRID_SHOW,	// ����̕\�����@�̑���
		GROUP,	// �O���[�v�̃��X�g����
		IMAGE,	// �摜�̑��� (�t�@�C���ǂݏ����Ŏg�p)
		//IMAGE_ASPECT,	// �摜�̏c���ێ��̑���
		IMAGE_OPAC,	// �摜�̕s�����x�̑���
		JOIN_LIMIT,	// ���̌����̐�萧���̑���
		JOIN_STYLE,	// �j�̌����̑���
		LIST,	// �}�`��}���܂��͍폜���鑀��
		ORDER,	// �}�`�̏��Ԃ̓���ւ�
		DEFORM,	// �}�`�̌` (���ʂ̈ʒu) �̑���
		MOVE,	// �}�`�̈ړ��̑���
		SELECT,	// �}�`�̑I����؂�ւ�
		PAGE_COLOR,	// �y�[�W�̐F�̑���
		PAGE_SIZE,	// �y�[�W�̑傫���̑���
		PAGE_PADD,	// �y�[�W�̓��]���̑���
		POLY_END,	// ���p�`�̏I�[�̑���
		DEG_START,
		DEG_END,
		DEG_ROT,	// �X���p�x�̑���
		STROKE_CAP,	// �[�̌`���̑���
		STROKE_COLOR,	// ���g�̐F�̑���
		STROKE_WIDTH,	// ���g�̑����̑���
		TEXT_PAR_ALIGN,	// �i���̐���̑���
		TEXT_ALIGN_T,	// ������̐���̑���
		TEXT_CONTENT,	// ������̑���
		TEXT_LINE_SP,	// �s�Ԃ̑���
		TEXT_PADDING,	// ������̗]���̑���
		TEXT_RANGE,	// �I�����ꂽ�����͈͂̑���
	};

	//------------------------------
	// ����X�^�b�N
	//------------------------------
	using UNDO_STACK = std::list<struct Undo*>;	// ����X�^�b�N

	//------------------------------
	// ���삩��l�̌^�𓾂�e���v���[�g
	//------------------------------
	template <UNDO_ID U> struct U_TYPE { using type = int; };
	template <> struct U_TYPE<UNDO_ID::ARROW_SIZE> { using type = ARROW_SIZE; };
	template <> struct U_TYPE<UNDO_ID::ARROW_STYLE> { using type = ARROW_STYLE; };
	template <> struct U_TYPE<UNDO_ID::DASH_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_ID::DASH_PATT> { using type = DASH_PATT; };
	template <> struct U_TYPE<UNDO_ID::DASH_STYLE> { using type = D2D1_DASH_STYLE; };
	template <> struct U_TYPE<UNDO_ID::FILL_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_ID::FONT_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_ID::FONT_FAMILY> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_ID::FONT_SIZE> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::FONT_STRETCH> { using type = DWRITE_FONT_STRETCH; };
	template <> struct U_TYPE<UNDO_ID::FONT_STYLE> { using type = DWRITE_FONT_STYLE; };
	template <> struct U_TYPE<UNDO_ID::FONT_WEIGHT> { using type = DWRITE_FONT_WEIGHT; };
	template <> struct U_TYPE<UNDO_ID::GRID_BASE> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::GRID_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_ID::GRID_EMPH> { using type = GRID_EMPH; };
	template <> struct U_TYPE<UNDO_ID::GRID_SHOW> { using type = GRID_SHOW; };
	//template <> struct U_TYPE<UNDO_ID::IMAGE_ASPECT> { using type = bool; };
	template <> struct U_TYPE<UNDO_ID::IMAGE_OPAC> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::JOIN_LIMIT> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::JOIN_STYLE> { using type = D2D1_LINE_JOIN; };
	template <> struct U_TYPE<UNDO_ID::MOVE> { using type = D2D1_POINT_2F; };
	template <> struct U_TYPE<UNDO_ID::PAGE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_ID::PAGE_SIZE> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_ID::PAGE_PADD> { using type = D2D1_RECT_F; };
	template <> struct U_TYPE<UNDO_ID::POLY_END> { using type = bool; };
	template <> struct U_TYPE<UNDO_ID::DEG_START> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::DEG_END> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::DEG_ROT> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::STROKE_CAP> { using type = CAP_STYLE; };
	template <> struct U_TYPE<UNDO_ID::STROKE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_ID::STROKE_WIDTH> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::TEXT_PAR_ALIGN> { using type = DWRITE_PARAGRAPH_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_ID::TEXT_ALIGN_T> { using type = DWRITE_TEXT_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_ID::TEXT_CONTENT> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_ID::TEXT_LINE_SP> { using type = float; };
	template <> struct U_TYPE<UNDO_ID::TEXT_PADDING> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_ID::TEXT_RANGE> { using type = DWRITE_TEXT_RANGE; };

	//------------------------------
	// ����̂ЂȌ^
	//------------------------------
	struct Undo {
		Shape* m_shape;	// ���삷��}�`

		// �����j������.
		virtual ~Undo() {}
		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept { return false; }
		// ��������s����.
		virtual void exec(void) {}
		// ������f�[�^���C�^�[����ǂݍ���.
		//virtual void read(DataReader const& /*dt_reader*/) {}
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		virtual bool refer_to(const Shape* s) const noexcept { return m_shape == s; };
		// �Q�Ƃ���}�`���X�g�Ɨp���}�`���i�[����.
		static void set(SHAPE_LIST* slist, ShapePage* page) noexcept;
		// ���삷��}�`�𓾂�.
		Shape* shape(void) const noexcept { return m_shape; }
		// ������쐬����.
		Undo(Shape* s) : m_shape(s) {}
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& /*dt_writer*/) {}
	};

	//------------------------------
	// �`�̑���
	//------------------------------
	struct UndoForm : Undo {
		uint32_t m_anc;	// ���삳��镔��
		D2D1_POINT_2F m_start;	// �ό`�O�̕��ʂ̈ʒu

		// ��������s����ƒl���ς�邩���肷��.
		bool changed(void) const noexcept;
		// ��������s����.
		void exec(void);
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoForm(DataReader const& dt_reader);
		// �}�`�̌`��ۑ�����.
		UndoForm(Shape* const s, const uint32_t anc);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`�̏��Ԃ����ւ��鑀��
	//------------------------------
	struct UndoOrder : Undo {
		Shape* m_dst_shape;	// ����ւ���̐}�`

		// ��������s����ƒl���ς�邩���肷��.
		bool changed(void) const noexcept { return m_shape != m_dst_shape; }
		// ����ւ����̐}�`�𓾂�.
		Shape* const dest(void) const noexcept { return m_dst_shape; }
		// ��������s����.
		void exec(void);
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		bool refer_to(const Shape* s) const noexcept final override { return Undo::refer_to(s) || m_dst_shape == s; };
		// �}�`�̏��Ԃ����ւ���.
		UndoOrder(Shape* const s, Shape* const t);
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoOrder(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`�̒l��ۑ����ĕύX���鑀��.
	//------------------------------
	template <UNDO_ID U>
	struct UndoValue : Undo, U_TYPE<U> {
		U_TYPE<U>::type m_value;	// // �ύX�����O�̒l

		~UndoValue() {};
		// ��������s����ƒl���ς�邩���肷��.
		bool changed(void) const noexcept;
		// ��������s����.
		void exec(void);
		// �l��}�`���瓾��.
		static bool GET(const Shape* s, U_TYPE<U>::type& val) noexcept;
		// �l��}�`�Ɋi�[����.
		static void SET(Shape* const s, const U_TYPE<U>::type& val);
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoValue(DataReader const& dt_reader);
		// �}�`�̒l��ۑ�����.
		UndoValue(Shape* s);
		// �}�`�̒l��ۑ����ĕύX����.
		UndoValue(Shape* s, const U_TYPE<U>::type& val);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �摜�̈ʒu�Ƒ傫���̑���
	//------------------------------
	struct UndoImage : Undo {
		D2D1_POINT_2F m_start;	// �ʒu
		D2D1_SIZE_F m_view;	// �\������Ă����ʏ�̐��@
		D2D1_RECT_F m_clip;	// �\������Ă���摜��̋�`
		D2D1_SIZE_F m_ratio;	// �搡�@�ƌ���`�̏c����
		float m_opac;	// �s�����x

		// ��������s����ƒl���ς�邩���肷��.
		bool changed(void) const noexcept;
		// ��������s����.
		void exec(void);
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoImage(DataReader const& dt_reader);
		// �}�`�̕��ʂ�ۑ�����.
		UndoImage(ShapeImage* const s);
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
		virtual bool refer_to(const Shape* s) const noexcept override { return Undo::refer_to(s) || m_shape_at == s; };
		// ���삳���ʒu�ɂ������}�`�𓾂�.
		Shape* const shape_at(void) const noexcept { return m_shape_at; }
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoList(DataReader const& dt_reader);
		// �}�`�����X�g�����菜��.
		UndoList(Shape* const s, const bool dont_exec = false);
		// �}�`�����X�g�ɑ}������.
		UndoList(Shape* const s, Shape* const p, const bool dont_exec = false);
		// ������f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �}�`���O���[�v�ɒǉ��܂��͍폜���鑀��.
	//------------------------------
	struct UndoGroup : UndoList {
		ShapeGroup* m_shape_group;	// ���삷��O���[�v

		// ��������s����.
		void exec(void);
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		bool refer_to(const Shape* s) const noexcept final override { return UndoList::refer_to(s) || m_shape_group == s; };
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoGroup(DataReader const& dt_reader);
		// �}�`���O���[�v����폜����.
		UndoGroup(ShapeGroup* const g, Shape* const s);
		// �}�`���O���[�v�ɒǉ�����.
		UndoGroup(ShapeGroup* const g, Shape* const s, Shape* const s_pos);
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
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoSelect(DataReader const& dt_reader);
		// �}�`�̑I���𔽓]����.
		UndoSelect(Shape* const s);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	// ������̑����j������.
	template <> UndoValue<UNDO_ID::TEXT_CONTENT>::~UndoValue() 
	{
		if (m_value != nullptr) {
			delete[] m_value;
			m_value = nullptr;
		}
	}
}