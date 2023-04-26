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
	// ����̎��
	enum struct UNDO_T : uint32_t {
		END = static_cast<uint32_t>(-1),	// ����X�^�b�N�̏I�[ (�t�@�C���ǂݏ����Ŏg�p)
		NIL = 0,	// ����̋�؂� (�t�@�C���ǂݏ����Ŏg�p)
		ARC_DIR,	// �~�ʂ̕����̑���
		ARC_END,	// �~�ʂ̏I�_�̑���
		ARC_ROT,	// �~�ʂ̌X���̑���
		ARC_START,	// �~�ʂ̎n�_�̑���
		ARROW_CAP,	// ��邵�̕Ԃ��̌`���̑���
		ARROW_JOIN,	// ��邵�̐�[�̌`���̑���
		ARROW_SIZE,	// ��邵�̑傫���̑���
		ARROW_STYLE,	// ��邵�̌`���̑���
		//DASH_CAP,	// �j���̒[�̌`���̑���
		DASH_PAT,	// �j���̔z�u�̑���
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
		PAGE_PAD,	// �y�[�W�̓��]���̑���
		POLY_END,	// ���p�`�̒[�̑���
		STROKE_CAP,	// �[�̌`���̑���
		STROKE_COLOR,	// ���g�̐F�̑���
		STROKE_WIDTH,	// ���g�̑����̑���
		TEXT_ALIGN_P,	// �i���̐���̑���
		TEXT_ALIGN_T,	// ������̐���̑���
		TEXT_CONTENT,	// ������̑���
		TEXT_LINE_SP,	// �s�Ԃ̑���
		TEXT_PAD,	// ������̗]���̑���
		TEXT_RANGE,	// �I�����ꂽ�����͈͂̑���
	};

	// ����X�^�b�N
	using UNDO_STACK = std::list<struct Undo*>;	// ����X�^�b�N

	// ���삩��l�̌^�𓾂�e���v���[�g
	template <UNDO_T U> struct U_TYPE { using type = int; };
	template <> struct U_TYPE<UNDO_T::ARC_START> { using type = float; };
	template <> struct U_TYPE<UNDO_T::ARC_DIR> { using type = D2D1_SWEEP_DIRECTION; };
	template <> struct U_TYPE<UNDO_T::ARC_END> { using type = float; };
	template <> struct U_TYPE<UNDO_T::ARC_ROT> { using type = float; };
	template <> struct U_TYPE<UNDO_T::ARROW_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_T::ARROW_JOIN> { using type = D2D1_LINE_JOIN; };
	template <> struct U_TYPE<UNDO_T::ARROW_SIZE> { using type = ARROW_SIZE; };
	template <> struct U_TYPE<UNDO_T::ARROW_STYLE> { using type = ARROW_STYLE; };
	//template <> struct U_TYPE<UNDO_T::DASH_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_T::DASH_PAT> { using type = DASH_PAT; };
	template <> struct U_TYPE<UNDO_T::DASH_STYLE> { using type = D2D1_DASH_STYLE; };
	template <> struct U_TYPE<UNDO_T::FILL_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::FONT_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::FONT_FAMILY> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_T::FONT_SIZE> { using type = float; };
	template <> struct U_TYPE<UNDO_T::FONT_STRETCH> { using type = DWRITE_FONT_STRETCH; };
	template <> struct U_TYPE<UNDO_T::FONT_STYLE> { using type = DWRITE_FONT_STYLE; };
	template <> struct U_TYPE<UNDO_T::FONT_WEIGHT> { using type = DWRITE_FONT_WEIGHT; };
	template <> struct U_TYPE<UNDO_T::GRID_BASE> { using type = float; };
	template <> struct U_TYPE<UNDO_T::GRID_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::GRID_EMPH> { using type = GRID_EMPH; };
	template <> struct U_TYPE<UNDO_T::GRID_SHOW> { using type = GRID_SHOW; };
	template <> struct U_TYPE<UNDO_T::IMAGE_OPAC> { using type = float; };
	template <> struct U_TYPE<UNDO_T::JOIN_LIMIT> { using type = float; };
	template <> struct U_TYPE<UNDO_T::JOIN_STYLE> { using type = D2D1_LINE_JOIN; };
	template <> struct U_TYPE<UNDO_T::MOVE> { using type = D2D1_POINT_2F; };
	template <> struct U_TYPE<UNDO_T::PAGE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::PAGE_SIZE> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_T::PAGE_PAD> { using type = D2D1_RECT_F; };
	template <> struct U_TYPE<UNDO_T::POLY_END> { using type = D2D1_FIGURE_END; };
	template <> struct U_TYPE<UNDO_T::STROKE_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_T::STROKE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::STROKE_WIDTH> { using type = float; };
	template <> struct U_TYPE<UNDO_T::TEXT_ALIGN_P> { using type = DWRITE_PARAGRAPH_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_T::TEXT_ALIGN_T> { using type = DWRITE_TEXT_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_T::TEXT_CONTENT> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_T::TEXT_LINE_SP> { using type = float; };
	template <> struct U_TYPE<UNDO_T::TEXT_PAD> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_T::TEXT_RANGE> { using type = DWRITE_TEXT_RANGE; };

	//------------------------------
	// ����̂ЂȌ^
	//------------------------------
	struct Undo {
		static SHAPE_LIST* undo_slist;	// �Q�Ƃ���}�`���X�g
		static ShapePage* undo_page;	// �Q�Ƃ���y�[�W

		Shape* m_shape;	// ���삷��}�`

		// �����j������.
		virtual ~Undo() {}
		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept { return false; }
		// ��������s����.
		virtual void exec(void) = 0;
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		virtual bool refer_to(const Shape* s) const noexcept { return m_shape == s; };
		// ���삪�Q�Ƃ��邽�߂̐}�`���X�g�ƕ\���}�`���i�[����.
		static void begin(SHAPE_LIST* slist, ShapePage* page) noexcept
		{
			undo_slist = slist;
			undo_page = page;
		}
		// ���삷��}�`�𓾂�.
		Shape* shape(void) const noexcept { return m_shape; }
		// ������쐬����.
		Undo(Shape* s) : m_shape(s) {}
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
	};

	// �}�`��ό`���鑀��
	struct UndoDeform : Undo {
		uint32_t m_loc;	// �ό`����镔��
		D2D1_POINT_2F m_p;	// �ό`�O�̕��ʂ̓_

		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept final override
		{
			using winrt::GraphPaper::implementation::equal;
			D2D1_POINT_2F p;
			m_shape->get_pos_loc(m_loc, p);
			return !equal(p, m_p);
		}
		// ���ɖ߂���������s����.
		virtual void exec(void) noexcept final override
		{
			D2D1_POINT_2F p;
			m_shape->get_pos_loc(m_loc, p);
			m_shape->set_pos_loc(m_p, m_loc, 0.0f, false);
			m_p = p;
		}
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoDeform(DataReader const& dt_reader);
		// �w�肵�����ʂ̓_��ۑ�����.
		UndoDeform(Shape* const s, const uint32_t loc);
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	//------------------------------
	// �}�`�̏��Ԃ����ւ��鑀��
	//------------------------------
	struct UndoOrder : Undo {
		Shape* m_dst_shape;	// ����ւ���̐}�`

		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept final override { return m_shape != m_dst_shape; }
		// ����ւ����̐}�`�𓾂�.
		Shape* const dest(void) const noexcept { return m_dst_shape; }
		// ��������s����.
		virtual void exec(void) noexcept override;
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		bool refer_to(const Shape* s) const noexcept final override { return Undo::refer_to(s) || m_dst_shape == s; };
		// �}�`�̏��Ԃ����ւ���.
		UndoOrder(Shape* const s, Shape* const t) : Undo(s), m_dst_shape(t) { UndoOrder::exec(); }
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoOrder(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	//------------------------------
	// �}�`�̑����l��ۑ����ĕύX���鑀��.
	//------------------------------
	template <UNDO_T U>
	struct UndoValue : Undo, U_TYPE<U> {
		U_TYPE<U>::type m_value;	// // �ύX�����O�̒l

		~UndoValue() {};
		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept final override
		{
			using winrt::GraphPaper::implementation::equal;
			U_TYPE<U>::type val{};
			return GET(m_shape, val) && !equal(val, m_value);
		}
		// ��������s����.
		virtual void exec(void) noexcept final override;
		// �l��}�`���瓾��.
		static bool GET(const Shape* s, U_TYPE<U>::type& val) noexcept;
		// �l��}�`�Ɋi�[����.
		static void SET(Shape* const s, const U_TYPE<U>::type& val) noexcept;
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoValue(DataReader const& dt_reader);
		// �}�`�̒l��ۑ�����.
		UndoValue(Shape* s) : Undo(s) { GET(m_shape, m_value); }
		// �}�`�̒l��ۑ����ĕύX����.
		UndoValue(Shape* s, const U_TYPE<U>::type& val) : UndoValue(s) { SET(m_shape, val); }
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	//------------------------------
	// �摜�̈ʒu�Ƒ傫���̑���
	//------------------------------
	struct UndoImage : Undo {
		D2D1_POINT_2F m_start;	// ����_
		D2D1_SIZE_F m_view;	// �\������Ă����ʏ�̐��@
		D2D1_RECT_F m_clip;	// �\������Ă���摜��̋�`
		D2D1_SIZE_F m_ratio;	// �搡�@�ƌ���`�̏c����
		float m_opac;	// �s�����x

		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept final override;
		// ��������s����.
		virtual void exec(void) noexcept final override;
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoImage(DataReader const& dt_reader);
		// �}�`�̕��ʂ�ۑ�����.
		UndoImage(ShapeImage* const s);
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	//------------------------------
	// �}�`�����X�g�ɑ}���܂��͍폜���鑀��.
	//------------------------------
	struct UndoList : Undo {
		bool m_insert;	// �}���t���O
		Shape* m_shape_at;	// �ύX�O��, ���삳���ʒu�ɂ������}�`

		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept override { return true; }
		// ��������s����.
		virtual void exec(void) noexcept override;
		// ���삪�}�������肷��.
		bool is_insert(void) const noexcept { return m_insert; }
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		virtual bool refer_to(const Shape* s) const noexcept override { return Undo::refer_to(s) || m_shape_at == s; };
		// ���삳���ʒu�ɂ������}�`�𓾂�.
		Shape* const shape_at(void) const noexcept { return m_shape_at; }
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoList(DataReader const& dt_reader);
		// �}�`�����X�g����폜����.
		UndoList::UndoList(Shape* const s, const bool dont_exec) :
			Undo(s),
			m_insert(false),
			m_shape_at(static_cast<Shape*>(nullptr))
		{
			if (!dont_exec) {
				exec();
			}
		}
		// �}�`�����X�g�ɑ}������
		UndoList::UndoList(Shape* const s, Shape* const t, const bool dont_exec) :
			Undo(s),
			m_insert(true),
			m_shape_at(t)
		{
			if (!dont_exec) {
				exec();
			}
		}
		// ������f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const override;
	};

	//------------------------------
	// �}�`���O���[�v�ɒǉ��܂��͍폜���鑀��.
	//------------------------------
	struct UndoGroup : UndoList {
		ShapeGroup* m_shape_group;	// ���삷��O���[�v

		// ��������s����.
		virtual void exec(void) noexcept final override;
		// �}�`���Q�Ƃ��Ă��邩���肷��.
		bool refer_to(const Shape* s) const noexcept final override { return UndoList::refer_to(s) || m_shape_group == s; };
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoGroup(DataReader const& dt_reader);
		// �}�`���O���[�v����폜����.
		UndoGroup(ShapeGroup* const g, Shape* const s) : UndoList(s, true), m_shape_group(g) { exec(); }
		// �}�`���O���[�v�ɒǉ�����.
		UndoGroup(ShapeGroup* const g, Shape* const s, Shape* const s_pos) : UndoList(s, s_pos, true), m_shape_group(g) { exec(); }
		// ������f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	//------------------------------
	// �}�`�̑I���𔽓]���鑀��
	//------------------------------
	struct UndoSelect : Undo {
		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept final override { return true; }
		// ��������s����.
		virtual void exec(void) noexcept final override { m_shape->set_select(!m_shape->is_selected()); }
		// �}�`�̑I���𔽓]����.
		UndoSelect(DataReader const& dt_reader);
		// �}�`�̑I���𔽓]����.
		UndoSelect(Shape* const s) : Undo(s) { exec(); }
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	// ������̑����j������.
	template <> UndoValue<UNDO_T::TEXT_CONTENT>::~UndoValue() 
	{
		if (m_value != nullptr) {
			delete[] m_value;
			m_value = nullptr;
		}
	}

	// �}�`���O���[�v�ɒǉ�����.
#define UndoAppendG(g, s)	UndoGroup(static_cast<ShapeGroup* const>(g), static_cast<Shape* const>(s), static_cast<Shape* const>(nullptr))
// �}�`�����X�g�ɒǉ�����.
#define UndoAppend(s)	UndoList(static_cast<Shape* const>(s), static_cast<Shape* const>(nullptr), false)
// �}�`�����X�g�ɑ}������.
#define UndoInsert(s, p)	UndoList(static_cast<Shape* const>(s), static_cast<Shape* const>(p), false)
// �}�`���O���[�v����폜����.
#define UndoRemoveG(g, s)	UndoGroup(static_cast<ShapeGroup* const>(g), static_cast<Shape* const>(s))
// �}�`�����X�g����폜����.
#define UndoRemove(s)	UndoList(static_cast<Shape* const>(s), false)
}