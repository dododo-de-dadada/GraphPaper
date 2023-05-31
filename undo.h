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
		SHEET_COLOR,	// �p���̐F�̑���
		SHEET_SIZE,	// �p���̑傫���̑���
		SHEET_PAD,	// �p���̓��]���̑���
		POLY_END,	// ���p�`�̒[�̑���
		REVERSE_PATH,	// ���̕����𔽓]���鑀��
		STROKE_CAP,	// �[�̌`���̑���
		STROKE_COLOR,	// ���g�̐F�̑���
		STROKE_WIDTH,	// ���g�̑����̑���
		TEXT_ALIGN_P,	// �i���̐���̑���
		TEXT_ALIGN_T,	// ������̐���̑���
		TEXT_CONTENT,	// ������̑���
		TEXT_DEL,	// ������̍폜�̑���
		TEXT_INS,	// ������̑}���̑���
		TEXT_LINE_SP,	// �s�Ԃ̑���
		TEXT_PAD,	// ������̗]���̑���
		TEXT_WRAP,	// ������̐܂�Ԃ��̑���
		//TEXT_RANGE,	// ������I���͈̔͂̑���
		TEXT_SELECT	// ������I���͈̔͂̑���
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
	template <> struct U_TYPE<UNDO_T::SHEET_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::SHEET_SIZE> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_T::SHEET_PAD> { using type = D2D1_RECT_F; };
	template <> struct U_TYPE<UNDO_T::POLY_END> { using type = D2D1_FIGURE_END; };
	template <> struct U_TYPE<UNDO_T::STROKE_CAP> { using type = D2D1_CAP_STYLE; };
	template <> struct U_TYPE<UNDO_T::STROKE_COLOR> { using type = D2D1_COLOR_F; };
	template <> struct U_TYPE<UNDO_T::STROKE_WIDTH> { using type = float; };
	template <> struct U_TYPE<UNDO_T::TEXT_ALIGN_P> { using type = DWRITE_PARAGRAPH_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_T::TEXT_ALIGN_T> { using type = DWRITE_TEXT_ALIGNMENT; };
	template <> struct U_TYPE<UNDO_T::TEXT_CONTENT> { using type = wchar_t*; };
	template <> struct U_TYPE<UNDO_T::TEXT_LINE_SP> { using type = float; };
	template <> struct U_TYPE<UNDO_T::TEXT_PAD> { using type = D2D1_SIZE_F; };
	template <> struct U_TYPE<UNDO_T::TEXT_WRAP> { using type = DWRITE_WORD_WRAPPING; };
	//template <> struct U_TYPE<UNDO_T::TEXT_RANGE> { using type = DWRITE_TEXT_RANGE; };

	constexpr auto UNDO_SHAPE_NIL = static_cast<uint32_t>(-2);	// �k���}�`�̓Y����
	constexpr auto UNDO_SHAPE_SHEET = static_cast<uint32_t>(-1);	// �p���}�`�̓Y����

	//------------------------------
	// ����̂ЂȌ^
	//------------------------------
	struct Undo {
		//static SHAPE_LIST* undo_slist;	// �Q�Ƃ���}�`���X�g
		static ShapeSheet* undo_sheet;	// �Q�Ƃ���p��

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
		static void begin(/*SHAPE_LIST* slist,*/ ShapeSheet* page) noexcept
		{
			//undo_slist = slist;
			undo_sheet = page;
		}
		// ���삷��}�`�𓾂�.
		Shape* shape(void) const noexcept { return m_shape; }
		// ������쐬����.
		Undo(Shape* s) : m_shape(s) {}
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
		// �f�[�^���[�_�[����Y������ǂݍ���Ő}�`�𓾂�.
		static Shape* undo_read_shape(DataReader const& dt_reader)
		{
			Shape* s = static_cast<Shape*>(nullptr);
			const uint32_t i = dt_reader.ReadUInt32();
			if (i == UNDO_SHAPE_SHEET) {
				s = Undo::undo_sheet;
			}
			else if (i == UNDO_SHAPE_NIL) {
				s = nullptr;
			}
			else {
				auto& slist = Undo::undo_sheet->m_shape_list;
				slist_match<const uint32_t, Shape*>(slist, i, s);
			}
			return s;
		}
		// �}�`���f�[�^���C�^�[�ɏ�������.
		static void undo_write_shape(
			Shape* const s,	// �������܂��}�`
			DataWriter const& dt_writer	// �f�[�^���[�_�[
		)
		{
			// �}�`���p���}�`�Ȃ�, �p�����Ӗ�����Y��������������.
			if (s == Undo::undo_sheet) {
				dt_writer.WriteUInt32(UNDO_SHAPE_SHEET);
			}
			// �}�`���k���Ȃ�, �k�����Ӗ�����Y��������������.
			else if (s == nullptr) {
				dt_writer.WriteUInt32(UNDO_SHAPE_NIL);
			}
			// ����ȊO�Ȃ�, ���X�g���ł̐}�`�̓Y��������������.
			// ���X�g���ɐ}�`���Ȃ���� UNDO_SHAPE_NIL ���������܂��.
			else {
				uint32_t i = UNDO_SHAPE_NIL;
				auto& slist = Undo::undo_sheet->m_shape_list;
				slist_match<Shape* const, uint32_t>(slist, s, i);
				dt_writer.WriteUInt32(i);
			}
		}
	};

	struct UndoReverse : Undo {
		UndoReverse(Shape* s) :
			Undo(s)
		{
			exec();
		}
		UndoReverse(DataReader const& dt_reader) :
			Undo(undo_read_shape(dt_reader))
		{}
		virtual bool changed(void) const noexcept final override
		{
			return true;
		}
		// ���ɖ߂���������s����.
		virtual void exec(void) noexcept final override
		{
			m_shape->reverse_path();
		}
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& dt_writer) const final override
		{
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::REVERSE_PATH));
			undo_write_shape(m_shape, dt_writer);
		}

	};

	// �}�`��ό`���鑀��
	struct UndoDeform : Undo {
		uint32_t m_loc;	// �ό`����镔��
		D2D1_POINT_2F m_pt;	// �ό`�O�̕��ʂ̓_

		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept final override
		{
			using winrt::GraphPaper::implementation::equal;
			D2D1_POINT_2F p;
			m_shape->get_pos_loc(m_loc, p);
			return !equal(p, m_pt);
		}
		// ���ɖ߂���������s����.
		virtual void exec(void) noexcept final override
		{
			D2D1_POINT_2F pt;
			m_shape->get_pos_loc(m_loc, pt);
			m_shape->set_pos_loc(m_pt, m_loc, 0.0f, false);
			m_pt = pt;
		}
		// �f�[�^���[�_�[���瑀���ǂݍ���.
		UndoDeform(DataReader const& dt_reader) :
			Undo(undo_read_shape(dt_reader)),
			m_loc(static_cast<LOC_TYPE>(dt_reader.ReadUInt32())),
			m_pt(D2D1_POINT_2F{ dt_reader.ReadSingle(), dt_reader.ReadSingle()})
		{}

		// �w�肵�����ʂ̓_��ۑ�����.
		UndoDeform::UndoDeform(Shape* const s, const uint32_t loc) :
			Undo(s),
			m_loc(loc),
			m_pt([](Shape* const s, const uint32_t loc)->D2D1_POINT_2F {
				D2D1_POINT_2F pt;
				s->get_pos_loc(loc, pt);
				return pt;
			}(s, loc))
		{}

		// �}�`�̌`�̑�����f�[�^���C�^�[�ɏ�������.
		void UndoDeform::write(DataWriter const& dt_writer) const final override
		{
			dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::DEFORM));
			undo_write_shape(m_shape, dt_writer);
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_loc));
			dt_writer.WriteSingle(m_pt.x);
			dt_writer.WriteSingle(m_pt.y);
		}

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
			return GET(m_shape, val) && !equal(static_cast<const U_TYPE<U>::type>(val), m_value);
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

	struct UndoTextSelect : Undo {
		int m_start;
		int m_end;
		bool m_is_trail;
		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept override
		{
			return true;
		}
		UndoTextSelect(Shape* const s, const int start, const int end, const bool is_trail) :
			Undo(s)
		{
			m_start = undo_sheet->m_select_start;
			m_end = undo_sheet->m_select_end;
			m_is_trail = undo_sheet->m_select_trail;
			undo_sheet->m_select_start = start;
			undo_sheet->m_select_end = end;
			undo_sheet->m_select_trail = is_trail;
		}
		virtual void exec(void) noexcept final override
		{
			const auto start = undo_sheet->m_select_start;
			const auto end = undo_sheet->m_select_end;
			const auto is_trail = undo_sheet->m_select_trail;
			undo_sheet->m_select_start = m_start;
			undo_sheet->m_select_end = m_end;
			undo_sheet->m_select_trail = m_is_trail;
			m_start = start;
			m_end = end;
			m_is_trail = is_trail;
		}
		UndoTextSelect(DataReader const& dt_reader);
		virtual void write(DataWriter const& dt_writer) const final override;
	};

	struct UndoText2 : Undo {
		uint32_t m_start = 0;
		uint32_t m_end = 0;
		bool m_trail = false;
		wchar_t* m_text = nullptr;	// �ۑ����ꂽ������

		// ��������s����ƒl���ς�邩���肷��.
		virtual bool changed(void) const noexcept override
		{
			if (m_start != (m_trail ? m_end + 1 : m_end)) {
				return true;
			}
			return wchar_len(m_text) > 0;
		}
		~UndoText2(void)
		{
			delete[] m_text;
		}

		// ������̑I��͈͂��폜��, �����Ɏw�肵���������}������.
		void edit(Shape* s, const wchar_t* ins_text) noexcept
		{
			wchar_t* old_text = static_cast<ShapeText*>(s)->m_text;
			const auto old_len = wchar_len(old_text);

			// �I��͈͂ƍ폜���镶�����ۑ�����.
			m_start = undo_sheet->m_select_start;
			m_end = undo_sheet->m_select_end;
			m_trail = undo_sheet->m_select_trail;
			const auto end = min(m_trail ? m_end + 1 : m_end, old_len);
			const auto start = min(m_start, old_len);
			const auto m = min(start, end);
			const auto n = max(start, end);
			const auto del_len = n - m;	// �폜���镶����.
			if (del_len > 0) {
				m_text = new wchar_t[del_len + 1];
				memcpy(m_text, old_text + m, del_len * 2);
				//for (int i = 0; i < del_len; i++) {
				//	m_text[i] = old_text[m + i];
				//}
				m_text[del_len] = L'\0';
			}
			else {
				m_text = nullptr;
			}

			// �}����̕����������̕������𒴂���Ȃ�, �V�������������m�ۂ���, �}�������ʒu���O�����R�s�[����.
			const auto ins_len = wchar_len(ins_text);
			const auto new_len = old_len + ins_len - del_len;
			wchar_t* new_text;
			if (new_len > old_len) {
				new_text = new wchar_t[new_len + 1];
				//for (uint32_t i = 0; i < m; i++) {
				//	new_text[i] = old_text[i];
				//}
				memcpy(new_text, old_text, m * 2);
			}

			// �}����̕����������̕������ȉ��Ȃ�, �V�������������m�ۂ���, ���̕�����𗘗p����.
			else {
				new_text = old_text;
			}

			// �}�����镶������R�s�[����.
			//for (uint32_t i = 0; i < ins_len; i++) {
			//	new_text[m + i] = ins_text[i];
			//}
			memcpy(new_text + m, ins_text, ins_len * 2);

			// ���̕�����̑I��͈͂̏I���ʒu��������R�s�[����.
			//for (uint32_t i = 0; i < old_len - n; i++) {
			//	new_text[m + ins_len + i] = old_text[n + i];
			//}
			memmove(new_text + m + ins_len, old_text + n, (old_len - n) * 2);
			new_text[new_len] = L'\0';

			// �}����̕����������̕������𒴂���Ȃ�, ���̕�����̃��������������, �V������������i�[����.
			if (new_len > old_len) {
				delete[] old_text;
				static_cast<ShapeText*>(s)->m_text = new_text;
			}
			static_cast<ShapeText*>(s)->m_text_len = new_len;

			// �ҏW��̑I��͈͂�, �}�����ꂽ������ɂȂ�.
			undo_sheet->m_select_start = m;
			undo_sheet->m_select_end = m + ins_len;
			undo_sheet->m_select_trail = false;
			static_cast<ShapeText*>(s)->m_dwrite_text_layout = nullptr;
		}
		// �}�`�̕������ҏW����.
		UndoText2(Shape* s, const wchar_t* ins_text) :
			Undo(s)
		{
			edit(s, ins_text);
		}
		virtual void exec(void) noexcept final override
		{
			// �ۑ����ꂽ������͕������ edit �֐����Ŋm�ۂ��ꂽ������ (�܂��̓k��) �ŕK���㏑�������.
			// edit �֐���͕K�v�Ȃ��Ȃ��Ă�̂ŉ������.
			wchar_t* ins_text = m_text;
			edit(m_shape, ins_text);
			delete[] ins_text;
		}
		UndoText2(DataReader const& dt_reader);
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