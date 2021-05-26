#include "pch.h"
#include "undo.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr auto INDEX_NULL = static_cast<uint32_t>(-2);	// �k���}�`�̓Y����
	constexpr auto INDEX_SHEET = static_cast<uint32_t>(-1);	// �p���}�`�̓Y����

	// �Y�������f�[�^���[�_�[����ǂݍ���Ő}�`�𓾂�.
	static Shape* undo_read_shape(DataReader const& dt_reader);
	// �}�`���f�[�^���C�^�[�ɏ�������.
	static void undo_write_shape(const Shape* s, DataWriter const& dt_writer);

	// �Y�������f�[�^���[�_�[����ǂݍ���Ő}�`�𓾂�.
	static Shape* undo_read_shape(DataReader const& dt_reader)
	{
		Shape* s = static_cast<Shape*>(nullptr);
		auto i = dt_reader.ReadUInt32();
		if (i == INDEX_SHEET) {
			s = Undo::s_shape_sheet;
		}
		else if (i == INDEX_NULL) {
			s = nullptr;
		}
		else {
			s_list_match<const uint32_t, Shape*>(*Undo::s_shape_list, i, s);
		}
		return s;
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	static void undo_write_shape(Shape* const s, DataWriter const& dt_writer)
	{
		if (s == Undo::s_shape_sheet) {
			dt_writer.WriteUInt32(INDEX_SHEET);
		}
		else if (s == nullptr) {
			dt_writer.WriteUInt32(INDEX_NULL);
		}
		else {
			uint32_t i = 0;
			s_list_match<Shape* const, uint32_t>(*Undo::s_shape_list, s, i);
			dt_writer.WriteUInt32(i);
		}
	}

	S_LIST_T* Undo::s_shape_list = nullptr;
	ShapeSheet* Undo::s_shape_sheet = nullptr;

	// ���삪�Q�Ƃ��邽�߂̐}�`���X�g�Ɨp���}�`���i�[����.
	void Undo::set(S_LIST_T* s_list, ShapeSheet* s_layout) noexcept
	{
		s_shape_list = s_list;
		s_shape_sheet = s_layout;
	}

	// ��������s����ƒl���ς�邩���肷��.
	bool UndoAnchor::changed(void) const noexcept
	{
		using winrt::GraphPaper::implementation::equal;
		D2D1_POINT_2F a_pos;

		m_shape->get_anch_pos(m_anchor, a_pos);
		return !equal(a_pos, m_anchor_pos);
	}

	// ���ɖ߂���������s����.
	void UndoAnchor::exec(void)
	{
		D2D1_POINT_2F a_pos;

		m_shape->get_anch_pos(m_anchor, a_pos);
		m_shape->set_anchor_pos(m_anchor_pos, m_anchor);
		m_anchor_pos = a_pos;
	}

	// ������f�[�^���[�_�[����ǂݍ���.
	UndoAnchor::UndoAnchor(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		using winrt::GraphPaper::implementation::read;

		m_anchor = static_cast<ANCH_TYPE>(dt_reader.ReadUInt32());
		read(m_anchor_pos, dt_reader);
	}

	// �}�`��, �w�肳�ꂽ���ʂ̈ʒu��ۑ�����.
	UndoAnchor::UndoAnchor(Shape* const s, const uint32_t anchor) :
		Undo(s)
	{
		m_shape = s;
		m_anchor = anchor;
		s->get_anch_pos(anchor, m_anchor_pos);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void UndoAnchor::write(DataWriter const& dt_writer)
	{
		using winrt::GraphPaper::implementation::write;

		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::ANCH_POS));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_anchor));
		write(m_anchor_pos, dt_writer);
	}

	// ��������s����.
	void UndoArrange2::exec(void)
	{
		if (m_shape == m_dst_shape) {
			return;
		}
		auto it_begin{ s_shape_list->begin() };
		auto it_end{ s_shape_list->end() };
		auto it_src{ std::find(it_begin, it_end, m_shape) };
		if (it_src == it_end) {
			return;
		}
		auto it_dst{ std::find(it_begin, it_end, m_dst_shape) };
		if (it_dst == it_end) {
			return;
		}
		*it_src = m_dst_shape;
		*it_dst = m_shape;
	}

	// ������f�[�^���[�_�[����ǂݍ���.
	UndoArrange2::UndoArrange2(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		m_dst_shape = undo_read_shape(dt_reader);
	}

	// ������쐬����.
	UndoArrange2::UndoArrange2(Shape* const s, Shape* const t) :
		Undo(s)
	{
		m_dst_shape = t;
		exec();
	}

	// ������f�[�^���C�^�[�ɏ�������.
	void UndoArrange2::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::ARRANGE));
		undo_write_shape(m_shape, dt_writer);
		undo_write_shape(m_dst_shape, dt_writer);
	}

	// ��������s����ƒl���ς�邩���肷��.
	template <UNDO_OP U> bool UndoAttr<U>::changed(void) const noexcept
	{
		using winrt::GraphPaper::implementation::equal;
		U_TYPE<U>::type value{};
		return GET(m_shape, value) && equal(value, m_value) != true;
	}

	// ��������s����.
	template <UNDO_OP U> void UndoAttr<U>::exec(void)
	{
		U_TYPE<U>::type old_value{};
		if (GET(m_shape, old_value)) {
			SET(m_shape, m_value);
			m_value = old_value;
		}
	}

	// �}�`�̑����l��ۑ�����.
	template <UNDO_OP U> UndoAttr<U>::UndoAttr(Shape* s) :
		Undo(s)
	{
		GET(m_shape, m_value);
	}

	// �}�`�̑����l��ۑ��������ƒl���i�[����.
	template <UNDO_OP U> UndoAttr<U>::UndoAttr(Shape* s, U_TYPE<U>::type const& value) :
		UndoAttr(s)
	{
		UndoAttr<U>::SET(m_shape, value);
	}
	template UndoAttr<UNDO_OP::ARROWHEAD_SIZE>::UndoAttr(Shape* s, const ARROWHEAD_SIZE& value);
	template UndoAttr<UNDO_OP::ARROWHEAD_STYLE>::UndoAttr(Shape* s, const ARROWHEAD_STYLE& value);
	template UndoAttr<UNDO_OP::FILL_COLOR>::UndoAttr(Shape* s, const D2D1_COLOR_F& value);
	template UndoAttr<UNDO_OP::FONT_COLOR>::UndoAttr(Shape* s, const D2D1_COLOR_F& value);
	template UndoAttr<UNDO_OP::FONT_FAMILY>::UndoAttr(Shape* s, wchar_t* const& value);
	template UndoAttr<UNDO_OP::FONT_SIZE>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::FONT_STRETCH>::UndoAttr(Shape* s, const DWRITE_FONT_STRETCH& value);
	template UndoAttr<UNDO_OP::FONT_STYLE>::UndoAttr(Shape* s, const DWRITE_FONT_STYLE& value);
	template UndoAttr<UNDO_OP::FONT_WEIGHT>::UndoAttr(Shape* s, const DWRITE_FONT_WEIGHT& value);
	template UndoAttr<UNDO_OP::GRID_BASE>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::GRID_GRAY>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::GRID_EMPH>::UndoAttr(Shape* s, const GRID_EMPH& value);
	template UndoAttr<UNDO_OP::GRID_SHOW>::UndoAttr(Shape* s, const GRID_SHOW& value);
	template UndoAttr<UNDO_OP::TEXT_LINE>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::TEXT_MARGIN>::UndoAttr(Shape* s, const D2D1_SIZE_F& value);
	template UndoAttr<UNDO_OP::SHEET_COLOR>::UndoAttr(Shape* s, const D2D1_COLOR_F& value);
	template UndoAttr<UNDO_OP::SHEET_SIZE>::UndoAttr(Shape* s, const D2D1_SIZE_F& value);
	template UndoAttr<UNDO_OP::TEXT_ALIGN_P>::UndoAttr(Shape* s, const DWRITE_PARAGRAPH_ALIGNMENT& value);
	template UndoAttr<UNDO_OP::START_POS>::UndoAttr(Shape* s, const D2D1_POINT_2F& value);
	template UndoAttr<UNDO_OP::STROKE_COLOR>::UndoAttr(Shape* s, const D2D1_COLOR_F& value);
	template UndoAttr<UNDO_OP::STROKE_DASH_PATT>::UndoAttr(Shape* s, const STROKE_DASH_PATT& value);
	template UndoAttr<UNDO_OP::STROKE_DASH_STYLE>::UndoAttr(Shape* s, const D2D1_DASH_STYLE& value);
	template UndoAttr<UNDO_OP::STROKE_JOIN_LIMIT>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::STROKE_JOIN_STYLE>::UndoAttr(Shape* s, const D2D1_LINE_JOIN& value);
	template UndoAttr<UNDO_OP::STROKE_WIDTH>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::TEXT_CONTENT>::UndoAttr(Shape* s, wchar_t* const& value);
	template UndoAttr<UNDO_OP::TEXT_ALIGN_T>::UndoAttr(Shape* s, const DWRITE_TEXT_ALIGNMENT& value);
	template UndoAttr<UNDO_OP::TEXT_RANGE>::UndoAttr(Shape* s, const DWRITE_TEXT_RANGE& value);

	template <UNDO_OP U> UndoAttr<U>::UndoAttr(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		using winrt::GraphPaper::implementation::read;

		if constexpr (U == UNDO_OP::FONT_SIZE
			|| U == UNDO_OP::STROKE_JOIN_LIMIT
			|| U == UNDO_OP::STROKE_WIDTH
			|| U == UNDO_OP::GRID_GRAY
			|| U == UNDO_OP::GRID_BASE
			|| U == UNDO_OP::TEXT_LINE) {
			m_value = dt_reader.ReadSingle();
		}
		else if constexpr (U == UNDO_OP::ARROWHEAD_STYLE
			|| U == UNDO_OP::STROKE_DASH_STYLE
			|| U == UNDO_OP::STROKE_JOIN_STYLE
			|| U == UNDO_OP::FONT_STRETCH
			|| U == UNDO_OP::FONT_STYLE
			|| U == UNDO_OP::FONT_WEIGHT
			|| U == UNDO_OP::GRID_SHOW
			|| U == UNDO_OP::TEXT_ALIGN_P
			|| U == UNDO_OP::TEXT_ALIGN_T) {
			m_value = static_cast<U_TYPE<U>::type>(dt_reader.ReadUInt32());
		}
		else {
			read(m_value, dt_reader);
		}
	}

	template UndoAttr<UNDO_OP::ARROWHEAD_SIZE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::ARROWHEAD_STYLE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FILL_COLOR>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_COLOR>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_FAMILY>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_SIZE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_STRETCH>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_STYLE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_WEIGHT>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::GRID_BASE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::GRID_GRAY>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::GRID_EMPH>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::GRID_SHOW>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_LINE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_MARGIN>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::SHEET_COLOR>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::SHEET_SIZE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_ALIGN_P>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::START_POS>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::STROKE_COLOR>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::STROKE_DASH_PATT>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::STROKE_DASH_STYLE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::STROKE_JOIN_LIMIT>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::STROKE_JOIN_STYLE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::STROKE_WIDTH>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_CONTENT>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_ALIGN_T>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_RANGE>::UndoAttr(DataReader const& dt_reader);

	// �}�`�̑����l�ɒl���i�[����.
	template <UNDO_OP U> void UndoAttr<U>::SET(Shape* const s, const U_TYPE<U>::type& value)
	{
		throw winrt::hresult_not_implemented();
	}

	void UndoAttr<UNDO_OP::ARROWHEAD_SIZE>::SET(Shape* const s, const ARROWHEAD_SIZE& value)
	{
		s->set_arrow_size(value);
	}

	void UndoAttr<UNDO_OP::ARROWHEAD_STYLE>::SET(Shape* const s, const ARROWHEAD_STYLE& value)
	{
		s->set_arrow_style(value);
	}

	void UndoAttr<UNDO_OP::FILL_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& value)
	{
		s->set_fill_color(value);
	}

	void UndoAttr<UNDO_OP::FONT_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& value)
	{
		s->set_font_color(value);
	}

	void UndoAttr<UNDO_OP::FONT_FAMILY>::SET(Shape* const s, wchar_t* const& value)
	{
		s->set_font_family(value);
	}

	void UndoAttr<UNDO_OP::FONT_SIZE>::SET(Shape* const s, const float& value)
	{
		s->set_font_size(value);
	}

	void UndoAttr<UNDO_OP::FONT_STRETCH>::SET(Shape* const s, const DWRITE_FONT_STRETCH& value)
	{
		s->set_font_stretch(value);
	}

	void UndoAttr<UNDO_OP::FONT_STYLE>::SET(Shape* const s, const DWRITE_FONT_STYLE& value)
	{
		s->set_font_style(value);
	}

	void UndoAttr<UNDO_OP::FONT_WEIGHT>::SET(Shape* const s, const DWRITE_FONT_WEIGHT& value)
	{
		s->set_font_weight(value);
	}

	void UndoAttr<UNDO_OP::GRID_BASE>::SET(Shape* const s, const float& value)
	{
		s->set_grid_base(value);
	}

	void UndoAttr<UNDO_OP::GRID_GRAY>::SET(Shape* const s, const float& value)
	{
		s->set_grid_gray(value);
	}

	void UndoAttr<UNDO_OP::GRID_EMPH>::SET(Shape* const s, const GRID_EMPH& value)
	{
		s->set_grid_emph(value);
	}

	void UndoAttr<UNDO_OP::GRID_SHOW>::SET(Shape* const s, const GRID_SHOW& value)
	{
		s->set_grid_show(value);
	}

	void UndoAttr<UNDO_OP::TEXT_LINE>::SET(Shape* const s, const float& value)
	{
		s->set_text_line(value);
	}

	void UndoAttr<UNDO_OP::TEXT_MARGIN>::SET(Shape* const s, const D2D1_SIZE_F& value)
	{
		s->set_text_margin(value);
	}

	void UndoAttr<UNDO_OP::SHEET_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& value)
	{
		s->set_sheet_color(value);
	}

	void UndoAttr<UNDO_OP::SHEET_SIZE>::SET(Shape* const s, const D2D1_SIZE_F& value)
	{
		s->set_sheet_size(value);
	}

	void UndoAttr<UNDO_OP::TEXT_ALIGN_P>::SET(Shape* const s, const DWRITE_PARAGRAPH_ALIGNMENT& value)
	{
		s->set_text_align_p(value);
	}

	void UndoAttr<UNDO_OP::START_POS>::SET(Shape* const s, const D2D1_POINT_2F& value)
	{
		s->set_start_pos(value);
	}

	void UndoAttr<UNDO_OP::STROKE_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& value)
	{
		s->set_stroke_color(value);
	}

	void UndoAttr<UNDO_OP::STROKE_JOIN_STYLE>::SET(Shape* const s, const D2D1_LINE_JOIN& value)
	{
		s->set_stroke_join_style(value);
	}

	void UndoAttr<UNDO_OP::STROKE_JOIN_LIMIT>::SET(Shape* const s, const float& value)
	{
		s->set_stroke_join_limit(value);
	}

	void UndoAttr<UNDO_OP::STROKE_DASH_PATT>::SET(Shape* const s, const STROKE_DASH_PATT& value)
	{
		s->set_stroke_dash_patt(value);
	}

	void UndoAttr<UNDO_OP::STROKE_DASH_STYLE>::SET(Shape* const s, const D2D1_DASH_STYLE& value)
	{
		s->set_stroke_dash_style(value);
	}

	void UndoAttr<UNDO_OP::STROKE_WIDTH>::SET(Shape* const s, const float& value)
	{
		s->set_stroke_width(value);
	}

	void UndoAttr<UNDO_OP::TEXT_CONTENT>::SET(Shape* const s, wchar_t* const& value)
	{
		s->set_text(value);
	}

	void UndoAttr<UNDO_OP::TEXT_ALIGN_T>::SET(Shape* const s, const DWRITE_TEXT_ALIGNMENT& value)
	{
		s->set_text_align_t(value);
	}

	void UndoAttr<UNDO_OP::TEXT_RANGE>::SET(Shape* const s, const DWRITE_TEXT_RANGE& value)
	{
		s->set_text_range(value);
	}

	template <UNDO_OP U> bool UndoAttr<U>::GET(const Shape* s, U_TYPE<U>::type& value) noexcept
	{
		return false;
	}

	bool UndoAttr<UNDO_OP::ARROWHEAD_SIZE>::GET(const Shape* s, ARROWHEAD_SIZE& value) noexcept
	{
		return s->get_arrow_size(value);
	}

	bool UndoAttr<UNDO_OP::ARROWHEAD_STYLE>::GET(const Shape* s, ARROWHEAD_STYLE& value) noexcept
	{
		return s->get_arrow_style(value);
	}

	bool UndoAttr<UNDO_OP::FILL_COLOR>::GET(const Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_fill_color(value);
	}

	bool UndoAttr<UNDO_OP::FONT_COLOR>::GET(const Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_font_color(value);
	}

	bool UndoAttr<UNDO_OP::FONT_FAMILY>::GET(const Shape* s, wchar_t*& value) noexcept
	{
		return s->get_font_family(value);
	}

	bool UndoAttr<UNDO_OP::FONT_SIZE>::GET(const Shape* s, float& value) noexcept
	{
		return s->get_font_size(value);
	}

	bool UndoAttr<UNDO_OP::FONT_STRETCH>::GET(const Shape* s, DWRITE_FONT_STRETCH& value) noexcept
	{
		return s->get_font_stretch(value);
	}

	bool UndoAttr<UNDO_OP::FONT_STYLE>::GET(const Shape* s, DWRITE_FONT_STYLE& value) noexcept
	{
		return s->get_font_style(value);
	}

	bool UndoAttr<UNDO_OP::FONT_WEIGHT>::GET(const Shape* s, DWRITE_FONT_WEIGHT& value) noexcept
	{
		return s->get_font_weight(value);
	}

	bool UndoAttr<UNDO_OP::GRID_BASE>::GET(const Shape* s, float& value) noexcept
	{
		return s->get_grid_base(value);
	}

	bool UndoAttr<UNDO_OP::GRID_GRAY>::GET(const Shape* s, float& value) noexcept
	{
		return s->get_grid_gray(value);
	}

	bool UndoAttr<UNDO_OP::GRID_EMPH>::GET(const Shape* s, GRID_EMPH& value) noexcept
	{
		return s->get_grid_emph(value);
	}

	bool UndoAttr<UNDO_OP::GRID_SHOW>::GET(const Shape* s, GRID_SHOW& value) noexcept
	{
		return s->get_grid_show(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_LINE>::GET(const Shape* s, float& value) noexcept
	{
		return s->get_text_line(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_MARGIN>::GET(const Shape* s, D2D1_SIZE_F& value) noexcept
	{
		return s->get_text_margin(value);
	}

	bool UndoAttr<UNDO_OP::SHEET_COLOR>::GET(const Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_sheet_color(value);
	}

	bool UndoAttr<UNDO_OP::SHEET_SIZE>::GET(const Shape* s, D2D1_SIZE_F& value) noexcept
	{
		return s->get_sheet_size(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_ALIGN_P>::GET(const Shape* s, DWRITE_PARAGRAPH_ALIGNMENT& value) noexcept
	{
		return s->get_text_align_p(value);
	}

	bool UndoAttr<UNDO_OP::START_POS>::GET(const Shape* s, D2D1_POINT_2F& value) noexcept
	{
		return s->get_start_pos(value);
	}

	bool UndoAttr<UNDO_OP::STROKE_COLOR>::GET(const Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_stroke_color(value);
	}

	bool UndoAttr<UNDO_OP::STROKE_JOIN_LIMIT>::GET(const Shape* s, float& value) noexcept
	{
		return s->get_stroke_join_limit(value);
	}

	bool UndoAttr<UNDO_OP::STROKE_JOIN_STYLE>::GET(const Shape* s, D2D1_LINE_JOIN& value) noexcept
	{
		return s->get_stroke_join_style(value);
	}

	bool UndoAttr<UNDO_OP::STROKE_DASH_PATT>::GET(const Shape* s, STROKE_DASH_PATT& value) noexcept
	{
		return s->get_stroke_dash_patt(value);
	}

	bool UndoAttr<UNDO_OP::STROKE_DASH_STYLE>::GET(const Shape* s, D2D1_DASH_STYLE& value) noexcept
	{
		return s->get_stroke_dash_style(value);
	}

	bool UndoAttr<UNDO_OP::STROKE_WIDTH>::GET(const Shape* s, float& value) noexcept
	{
		return s->get_stroke_width(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_CONTENT>::GET(const Shape* s, wchar_t*& value) noexcept
	{
		return s->get_text(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_ALIGN_T>::GET(const Shape* s, DWRITE_TEXT_ALIGNMENT& value) noexcept
	{
		return s->get_text_align_t(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_RANGE>::GET(const Shape* s, DWRITE_TEXT_RANGE& value) noexcept
	{
		return s->get_text_range(value);
	}

	// �f�[�^���C�^�[�ɏ�������.
	template <UNDO_OP U> void UndoAttr<U>::write(DataWriter const& dt_writer)
	{
		using winrt::GraphPaper::implementation::write;
		dt_writer.WriteUInt32(static_cast<uint32_t>(U));
		undo_write_shape(m_shape, dt_writer);
		if constexpr (U == UNDO_OP::FONT_SIZE 
			|| U == UNDO_OP::STROKE_JOIN_LIMIT
			|| U == UNDO_OP::STROKE_WIDTH
			|| U == UNDO_OP::GRID_GRAY 
			|| U == UNDO_OP::GRID_BASE
			|| U == UNDO_OP::TEXT_LINE) {
			dt_writer.WriteSingle(m_value);
		}
		else if constexpr (U == UNDO_OP::ARROWHEAD_STYLE
			|| U == UNDO_OP::STROKE_DASH_STYLE
			|| U == UNDO_OP::STROKE_JOIN_STYLE
			|| U == UNDO_OP::FONT_STRETCH
			|| U == UNDO_OP::FONT_STYLE
			|| U == UNDO_OP::FONT_WEIGHT
			|| U == UNDO_OP::GRID_SHOW
			|| U == UNDO_OP::TEXT_ALIGN_P
			|| U == UNDO_OP::TEXT_ALIGN_T) {
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_value));
		}
		else {
			write(m_value, dt_writer);
		}
	}

	// ��������s����.
	void UndoList::exec(void)
	{
		if (m_insert) {
			auto it_del{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape) };
			if (it_del != s_shape_list->end()) {
				s_shape_list->erase(it_del);
			}
			auto it_ins{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape_at) };
			s_shape_list->insert(it_ins, m_shape);
			m_shape->set_delete(false);
		}
		else {
			m_shape->set_delete(true);
			auto it_del{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape) };
			auto it_pos{ s_shape_list->erase(it_del) };
			m_shape_at = (it_pos == s_shape_list->end() ? nullptr : *it_pos);
			s_shape_list->push_front(m_shape);
		}
		m_insert = !m_insert;
	}

	// ������f�[�^���[�_�[����ǂݍ���.
	UndoList::UndoList(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		m_insert = dt_reader.ReadBoolean();
		m_shape_at = undo_read_shape(dt_reader);
	}

	// �}�`�����X�g�����菜��.
	// s	��菜���}�`
	// dont	�������݂̂ő�������s���Ȃ�.
	UndoList::UndoList(Shape* const s, const bool dont_exec) :
		Undo(s),
		m_insert(false),
		m_shape_at(static_cast<Shape*>(nullptr))
	{
		if (dont_exec != true) {
			exec();
		}
	}

	// �}�`�����X�g�ɑ}������
	// s	�}������}�`
	// s_pos	�}������ʒu
	// dont	�������݂̂ő�������s���Ȃ�.
	UndoList::UndoList(Shape* const s, Shape* const s_pos, const bool dont_exec) :
		Undo(s),
		m_insert(true),
		m_shape_at(s_pos)
	{
		if (dont_exec != true) {
			exec();
		}
	}

	// ������f�[�^���C�^�[�ɏ�������.
	void UndoList::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::LIST));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_shape_at, dt_writer);
	}

	// ��������s����.
	void UndoListGroup::exec(void)
	{
		if (m_insert) {
			auto it_del{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape) };
			if (it_del != s_shape_list->end()) {
				s_shape_list->erase(it_del);
			}
			S_LIST_T& list_grouped = m_shape_group->m_list_grouped;
			auto it_ins{ std::find(list_grouped.begin(), list_grouped.end(), m_shape_at) };
			list_grouped.insert(it_ins, m_shape);
			m_shape->set_delete(false);
		}
		else {
			m_shape->set_delete(true);
			S_LIST_T& list_grouped = m_shape_group->m_list_grouped;
			auto it_del{ std::find(list_grouped.begin(), list_grouped.end(), m_shape) };
			auto it_pos{ list_grouped.erase(it_del) };
			m_shape_at = (it_pos == list_grouped.end() ? nullptr : *it_pos);
			s_shape_list->push_front(m_shape);
		}
		m_insert = !m_insert;
	}

	// ������f�[�^���[�_�[����ǂݍ���.
	UndoListGroup::UndoListGroup(DataReader const& dt_reader) :
		UndoList(dt_reader),
		m_shape_group(static_cast<ShapeGroup*>(undo_read_shape(dt_reader)))
	{}

	// �}�`���O���[�v�����菜��.
	// s	��菜���}�`
	UndoListGroup::UndoListGroup(ShapeGroup* const g, Shape* const s) :
		UndoList(s, true),
		m_shape_group(g)
	{
		exec();
	}

	// �}�`���O���[�v�ɒǉ�����.
	// s	��菜���}�`
	UndoListGroup::UndoListGroup(ShapeGroup* const g, Shape* const s, Shape* const s_pos) :
		UndoList(s, s_pos, true),
		m_shape_group(g)
	{
		exec();
	}

	// ������f�[�^���C�^�[�ɏ�������.
	void UndoListGroup::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::GROUP));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_shape_at, dt_writer);
		undo_write_shape(m_shape_group, dt_writer);
	}

	// ��������s����.
	void UndoSelect::exec(void)
	{
		m_shape->set_select(m_shape->is_selected() != true);
	}

	// ������f�[�^���[�_�[����ǂݍ���.
	UndoSelect::UndoSelect(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{}

	// �}�`�̑I���𔽓]����.
	UndoSelect::UndoSelect(Shape* const s) :
		Undo(s)
	{
		exec();
	}

	// �f�[�^���C�^�[�ɏ�������.
	void UndoSelect::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::SELECT));
		undo_write_shape(m_shape, dt_writer);
	}

}