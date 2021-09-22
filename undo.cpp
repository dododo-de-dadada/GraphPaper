#include "pch.h"
#include "undo.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr auto INDEX_NIL = static_cast<uint32_t>(-2);	// ヌル図形の添え字
	constexpr auto INDEX_SHEET = static_cast<uint32_t>(-1);	// 用紙図形の添え字

	// 部位の位置を得る.
	static D2D1_POINT_2F undo_get_pos_anch(const Shape* s, const uint32_t anch) noexcept;
	// データリーダーから位置を読み込む.
	static D2D1_POINT_2F undo_read_pos(DataReader const& dt_reader);
	// データリーダーから位置を読み込む.
	static D2D1_RECT_F undo_read_rect(DataReader const& dt_reader);
	// データリーダーから添え字を読み込んで図形を得る.
	static Shape* undo_read_shape(DataReader const& dt_reader);
	// データリーダーから位置を読み込む.
	static D2D1_SIZE_F undo_read_size(DataReader const& dt_reader);
	// 図形をデータライターに書き込む.
	static void undo_write_shape(Shape* const s, DataWriter const& dt_writer);

	// 部位の位置を得る.
	static D2D1_POINT_2F undo_get_pos_anch(const Shape* s, const uint32_t anch) noexcept
	{
		D2D1_POINT_2F pos;
		s->get_pos_anch(anch, pos);
		return pos;
	}

	// データリーダーから位置を読み込む.
	static D2D1_POINT_2F undo_read_pos(DataReader const& dt_reader)
	{
		D2D1_POINT_2F pos;
		dt_read(pos, dt_reader);
		return pos;
	}

	// データリーダーから位置を読み込む.
	static D2D1_RECT_F undo_read_rect(DataReader const& dt_reader)
	{
		return D2D1_RECT_F{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
	}

	// データリーダーから添え字を読み込んで図形を得る.
	static Shape* undo_read_shape(DataReader const& dt_reader)
	{
		Shape* s = static_cast<Shape*>(nullptr);
		const uint32_t i = dt_reader.ReadUInt32();
		if (i == INDEX_SHEET) {
			s = Undo::s_shape_sheet;
		}
		else if (i == INDEX_NIL) {
			s = nullptr;
		}
		else {
			slist_match<const uint32_t, Shape*>(*Undo::s_shape_list, i, s);
		}
		return s;
	}

	// データリーダーから位置を読み込む.
	static D2D1_SIZE_F undo_read_size(DataReader const& dt_reader)
	{
		D2D1_SIZE_F size;
		dt_read(size, dt_reader);
		return size;
	}

	// 図形をデータライターに書き込む.
	static void undo_write_shape(Shape* const s, DataWriter const& dt_writer)
	{
		if (s == Undo::s_shape_sheet) {
			dt_writer.WriteUInt32(INDEX_SHEET);
		}
		else if (s == nullptr) {
			dt_writer.WriteUInt32(INDEX_NIL);
		}
		else {
			uint32_t i = 0;
			slist_match<Shape* const, uint32_t>(*Undo::s_shape_list, s, i);
			dt_writer.WriteUInt32(i);
		}
	}

	SHAPE_LIST* Undo::s_shape_list = nullptr;
	ShapeSheet* Undo::s_shape_sheet = nullptr;

	// 操作が参照するための図形リストと用紙図形を格納する.
	void Undo::set(SHAPE_LIST* slist, ShapeSheet* s_layout) noexcept
	{
		s_shape_list = slist;
		s_shape_sheet = s_layout;
	}

	// 操作を実行すると値が変わるか判定する.
	bool UndoAnchor::changed(void) const noexcept
	{
		using winrt::GraphPaper::implementation::equal;

		D2D1_POINT_2F pos;
		m_shape->get_pos_anch(m_anch, pos);
		return !equal(pos, m_pos);
	}

	// 元に戻す操作を実行する.
	void UndoAnchor::exec(void)
	{
		D2D1_POINT_2F pos;
		m_shape->get_pos_anch(m_anch, pos);
		m_shape->set_pos_anch(m_pos, m_anch, 0.0f, false);
		m_pos = pos;
	}

	// データリーダーから操作を読み込む.
	UndoAnchor::UndoAnchor(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader)),
		m_anch(static_cast<ANCH_TYPE>(dt_reader.ReadUInt32())),
		m_pos(undo_read_pos(dt_reader))
	{}

	// 図形の, 指定された部位の位置を保存する.
	UndoAnchor::UndoAnchor(Shape* const s, const uint32_t anch) :
		Undo(s),
		m_anch(anch),
		m_pos(undo_get_pos_anch(s, anch))
	{}

	// データライターに書き込む.
	void UndoAnchor::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::POS_ANCH));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_anch));
		dt_write(m_pos, dt_writer);
	}

	// 操作を実行する.
	void UndoArrange::exec(void)
	{
		if (m_shape == m_dst_shape) {
			return;
		}
		auto it_beg{ s_shape_list->begin() };
		auto it_end{ s_shape_list->end() };
		auto it_src{ std::find(it_beg, it_end, m_shape) };
		if (it_src == it_end) {
			return;
		}
		auto it_dst{ std::find(it_beg, it_end, m_dst_shape) };
		if (it_dst == it_end) {
			return;
		}
		*it_src = m_dst_shape;
		*it_dst = m_shape;
	}

	// データリーダーから操作を読み込む.
	UndoArrange::UndoArrange(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader)),
		m_dst_shape(undo_read_shape(dt_reader))
	{
		//m_dst_shape = undo_read_shape(dt_reader);
	}

	// 操作を作成する.
	UndoArrange::UndoArrange(Shape* const s, Shape* const t) :
		Undo(s),
		m_dst_shape(t)
	{
		//m_dst_shape = t;
		exec();
	}

	// 操作をデータライターに書き込む.
	void UndoArrange::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::ARRANGE));
		undo_write_shape(m_shape, dt_writer);
		undo_write_shape(m_dst_shape, dt_writer);
	}

	// 操作を実行すると値が変わるか判定する.
	template <UNDO_OP U>
	bool UndoAttr<U>::changed(void) const noexcept
	{
		using winrt::GraphPaper::implementation::equal;
		U_TYPE<U>::type value{};
		return GET(m_shape, value) && equal(value, m_value) != true;
	}

	// 操作を実行する.
	template <UNDO_OP U>
	void UndoAttr<U>::exec(void)
	{
		U_TYPE<U>::type old_value{};
		if (GET(m_shape, old_value)) {
			SET(m_shape, m_value);
			m_value = old_value;
		}
	}

	// 図形の属性値を保存する.
	template <UNDO_OP U>
	UndoAttr<U>::UndoAttr(Shape* s) :
		Undo(s)
	{
		GET(m_shape, m_value);
	}

	// 図形の属性値を保存したあと値を格納する.
	template <UNDO_OP U>
	UndoAttr<U>::UndoAttr(Shape* s, U_TYPE<U>::type const& value) :
		UndoAttr(s)
	{
		UndoAttr<U>::SET(m_shape, value);
	}
	template UndoAttr<UNDO_OP::ARROW_SIZE>::UndoAttr(Shape* s, const ARROW_SIZE& value);
	template UndoAttr<UNDO_OP::ARROW_STYLE>::UndoAttr(Shape* s, const ARROW_STYLE& value);
	template UndoAttr<UNDO_OP::DASH_CAP>::UndoAttr(Shape* s, const D2D1_CAP_STYLE& value);
	template UndoAttr<UNDO_OP::DASH_PATT>::UndoAttr(Shape* s, const DASH_PATT& value);
	template UndoAttr<UNDO_OP::DASH_STYLE>::UndoAttr(Shape* s, const D2D1_DASH_STYLE& value);
	template UndoAttr<UNDO_OP::FILL_COLOR>::UndoAttr(Shape* s, const D2D1_COLOR_F& value);
	template UndoAttr<UNDO_OP::FONT_COLOR>::UndoAttr(Shape* s, const D2D1_COLOR_F& value);
	template UndoAttr<UNDO_OP::FONT_FAMILY>::UndoAttr(Shape* s, wchar_t* const& value);
	template UndoAttr<UNDO_OP::FONT_SIZE>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::FONT_STRETCH>::UndoAttr(Shape* s, const DWRITE_FONT_STRETCH& value);
	template UndoAttr<UNDO_OP::FONT_STYLE>::UndoAttr(Shape* s, const DWRITE_FONT_STYLE& value);
	template UndoAttr<UNDO_OP::FONT_WEIGHT>::UndoAttr(Shape* s, const DWRITE_FONT_WEIGHT& value);
	template UndoAttr<UNDO_OP::GRID_BASE>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::GRID_COLOR>::UndoAttr(Shape* s, const D2D1_COLOR_F& value);
	template UndoAttr<UNDO_OP::GRID_EMPH>::UndoAttr(Shape* s, const GRID_EMPH& value);
	template UndoAttr<UNDO_OP::GRID_SHOW>::UndoAttr(Shape* s, const GRID_SHOW& value);
	template UndoAttr<UNDO_OP::IMAGE_OPAC>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::JOIN_LIMIT>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::JOIN_STYLE>::UndoAttr(Shape* s, const D2D1_LINE_JOIN& value);
	template UndoAttr<UNDO_OP::POS_START>::UndoAttr(Shape* s, const D2D1_POINT_2F& value);
	template UndoAttr<UNDO_OP::SHEET_COLOR>::UndoAttr(Shape* s, const D2D1_COLOR_F& value);
	template UndoAttr<UNDO_OP::SHEET_SIZE>::UndoAttr(Shape* s, const D2D1_SIZE_F& value);
	template UndoAttr<UNDO_OP::STROKE_CAP>::UndoAttr(Shape* s, const CAP_STYLE& value);
	template UndoAttr<UNDO_OP::STROKE_COLOR>::UndoAttr(Shape* s, const D2D1_COLOR_F& value);
	template UndoAttr<UNDO_OP::STROKE_WIDTH>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::TEXT_ALIGN_P>::UndoAttr(Shape* s, const DWRITE_PARAGRAPH_ALIGNMENT& value);
	template UndoAttr<UNDO_OP::TEXT_ALIGN_T>::UndoAttr(Shape* s, const DWRITE_TEXT_ALIGNMENT& value);
	template UndoAttr<UNDO_OP::TEXT_CONTENT>::UndoAttr(Shape* s, wchar_t* const& value);
	template UndoAttr<UNDO_OP::TEXT_LINE_SP>::UndoAttr(Shape* s, const float& value);
	template UndoAttr<UNDO_OP::TEXT_MARGIN>::UndoAttr(Shape* s, const D2D1_SIZE_F& value);
	template UndoAttr<UNDO_OP::TEXT_RANGE>::UndoAttr(Shape* s, const DWRITE_TEXT_RANGE& value);

	template <UNDO_OP U> UndoAttr<U>::UndoAttr(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader)),
		m_value()
	{
		if constexpr (
			U == UNDO_OP::FONT_SIZE ||
			U == UNDO_OP::GRID_BASE ||
			U == UNDO_OP::IMAGE_OPAC ||
			U == UNDO_OP::JOIN_LIMIT ||
			U == UNDO_OP::STROKE_WIDTH ||
			U == UNDO_OP::TEXT_LINE_SP) {
			m_value = dt_reader.ReadSingle();
		}
		else if constexpr (
			U == UNDO_OP::ARROW_STYLE ||
			U == UNDO_OP::DASH_CAP ||
			U == UNDO_OP::DASH_STYLE ||
			U == UNDO_OP::JOIN_STYLE ||
			U == UNDO_OP::FONT_STRETCH ||
			U == UNDO_OP::FONT_STYLE ||
			U == UNDO_OP::FONT_WEIGHT ||
			U == UNDO_OP::GRID_SHOW ||
			U == UNDO_OP::TEXT_ALIGN_P ||
			U == UNDO_OP::TEXT_ALIGN_T) {
			m_value = static_cast<U_TYPE<U>::type>(dt_reader.ReadUInt32());
		}
		//else if constexpr (U == UNDO_OP::IMAGE_ASPECT) {
		//	m_value = dt_reader.ReadBoolean();
		//}
		else {
			dt_read(m_value, dt_reader);
		}
	}

	template UndoAttr<UNDO_OP::ARROW_SIZE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::ARROW_STYLE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::DASH_CAP>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::DASH_PATT>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::DASH_STYLE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FILL_COLOR>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_COLOR>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_FAMILY>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_SIZE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_STRETCH>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_STYLE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::FONT_WEIGHT>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::GRID_BASE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::GRID_COLOR>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::GRID_EMPH>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::GRID_SHOW>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::IMAGE_OPAC>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::JOIN_LIMIT>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::JOIN_STYLE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::POS_START>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::SHEET_COLOR>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::SHEET_SIZE>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::STROKE_CAP>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::STROKE_COLOR>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::STROKE_WIDTH>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_ALIGN_T>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_ALIGN_P>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_CONTENT>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_LINE_SP>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_MARGIN>::UndoAttr(DataReader const& dt_reader);
	template UndoAttr<UNDO_OP::TEXT_RANGE>::UndoAttr(DataReader const& dt_reader);

	// 図形の属性値に値を格納する.
	template <UNDO_OP U> void UndoAttr<U>::SET(Shape* const s, const U_TYPE<U>::type& value)
	{
		throw winrt::hresult_not_implemented();
	}

	void UndoAttr<UNDO_OP::ARROW_SIZE>::SET(Shape* const s, const ARROW_SIZE& value)
	{
		s->set_arrow_size(value);
	}

	void UndoAttr<UNDO_OP::ARROW_STYLE>::SET(Shape* const s, const ARROW_STYLE& value)
	{
		s->set_arrow_style(value);
	}

	void UndoAttr<UNDO_OP::DASH_CAP>::SET(Shape* const s, const D2D1_CAP_STYLE& value)
	{
		s->set_dash_cap(value);
	}

	void UndoAttr<UNDO_OP::DASH_PATT>::SET(Shape* const s, const DASH_PATT& value)
	{
		s->set_dash_patt(value);
	}

	void UndoAttr<UNDO_OP::DASH_STYLE>::SET(Shape* const s, const D2D1_DASH_STYLE& value)
	{
		s->set_dash_style(value);
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

	void UndoAttr<UNDO_OP::GRID_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& value)
	{
		s->set_grid_color(value);
	}

	void UndoAttr<UNDO_OP::GRID_EMPH>::SET(Shape* const s, const GRID_EMPH& value)
	{
		s->set_grid_emph(value);
	}

	void UndoAttr<UNDO_OP::GRID_SHOW>::SET(Shape* const s, const GRID_SHOW& value)
	{
		s->set_grid_show(value);
	}

	void UndoAttr<UNDO_OP::IMAGE_OPAC>::SET(Shape* const s, const float& value)
	{
		s->set_image_opacity(value);
	}

	void UndoAttr<UNDO_OP::JOIN_LIMIT>::SET(Shape* const s, const float& value)
	{
		s->set_join_limit(value);
	}

	void UndoAttr<UNDO_OP::JOIN_STYLE>::SET(Shape* const s, const D2D1_LINE_JOIN& value)
	{
		s->set_join_style(value);
	}

	void UndoAttr<UNDO_OP::POS_START>::SET(Shape* const s, const D2D1_POINT_2F& value)
	{
		s->set_pos_start(value);
	}

	void UndoAttr<UNDO_OP::SHEET_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& value)
	{
		s->set_sheet_color(value);
	}

	void UndoAttr<UNDO_OP::SHEET_SIZE>::SET(Shape* const s, const D2D1_SIZE_F& value)
	{
		s->set_sheet_size(value);
	}

	void UndoAttr<UNDO_OP::STROKE_CAP>::SET(Shape* const s, const CAP_STYLE& value)
	{
		s->set_stroke_cap(value);
	}

	void UndoAttr<UNDO_OP::STROKE_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& value)
	{
		s->set_stroke_color(value);
	}

	void UndoAttr<UNDO_OP::STROKE_WIDTH>::SET(Shape* const s, const float& value)
	{
		s->set_stroke_width(value);
	}

	void UndoAttr<UNDO_OP::TEXT_ALIGN_P>::SET(Shape* const s, const DWRITE_PARAGRAPH_ALIGNMENT& value)
	{
		s->set_text_align_p(value);
	}

	void UndoAttr<UNDO_OP::TEXT_ALIGN_T>::SET(Shape* const s, const DWRITE_TEXT_ALIGNMENT& value)
	{
		s->set_text_align_t(value);
	}

	void UndoAttr<UNDO_OP::TEXT_CONTENT>::SET(Shape* const s, wchar_t* const& value)
	{
		s->set_text_content(value);
	}

	void UndoAttr<UNDO_OP::TEXT_LINE_SP>::SET(Shape* const s, const float& value)
	{
		s->set_text_line_sp(value);
	}

	void UndoAttr<UNDO_OP::TEXT_MARGIN>::SET(Shape* const s, const D2D1_SIZE_F& value)
	{
		s->set_text_padding(value);
	}

	void UndoAttr<UNDO_OP::TEXT_RANGE>::SET(Shape* const s, const DWRITE_TEXT_RANGE& value)
	{
		s->set_text_range(value);
	}

	template <UNDO_OP U> bool UndoAttr<U>::GET(const Shape* s, U_TYPE<U>::type& value) noexcept
	{
		return false;
	}

	bool UndoAttr<UNDO_OP::ARROW_SIZE>::GET(const Shape* s, ARROW_SIZE& value) noexcept
	{
		return s->get_arrow_size(value);
	}

	bool UndoAttr<UNDO_OP::ARROW_STYLE>::GET(const Shape* s, ARROW_STYLE& value) noexcept
	{
		return s->get_arrow_style(value);
	}

	bool UndoAttr<UNDO_OP::DASH_CAP>::GET(const Shape* s, D2D1_CAP_STYLE& value) noexcept
	{
		return s->get_dash_cap(value);
	}

	bool UndoAttr<UNDO_OP::DASH_PATT>::GET(const Shape* s, DASH_PATT& value) noexcept
	{
		return s->get_dash_patt(value);
	}

	bool UndoAttr<UNDO_OP::DASH_STYLE>::GET(const Shape* s, D2D1_DASH_STYLE& value) noexcept
	{
		return s->get_dash_style(value);
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

	bool UndoAttr<UNDO_OP::GRID_COLOR>::GET(const Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_grid_color(value);
	}

	bool UndoAttr<UNDO_OP::GRID_EMPH>::GET(const Shape* s, GRID_EMPH& value) noexcept
	{
		return s->get_grid_emph(value);
	}

	bool UndoAttr<UNDO_OP::GRID_SHOW>::GET(const Shape* s, GRID_SHOW& value) noexcept
	{
		return s->get_grid_show(value);
	}

	bool UndoAttr<UNDO_OP::IMAGE_OPAC>::GET(const Shape* s, float& value) noexcept
	{
		return s->get_image_opacity(value);
	}

	bool UndoAttr<UNDO_OP::JOIN_LIMIT>::GET(const Shape* s, float& value) noexcept
	{
		return s->get_join_limit(value);
	}

	bool UndoAttr<UNDO_OP::JOIN_STYLE>::GET(const Shape* s, D2D1_LINE_JOIN& value) noexcept
	{
		return s->get_join_style(value);
	}

	bool UndoAttr<UNDO_OP::POS_START>::GET(const Shape* s, D2D1_POINT_2F& value) noexcept
	{
		return s->get_pos_start(value);
	}

	bool UndoAttr<UNDO_OP::SHEET_COLOR>::GET(const Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_sheet_color(value);
	}

	bool UndoAttr<UNDO_OP::SHEET_SIZE>::GET(const Shape* s, D2D1_SIZE_F& value) noexcept
	{
		return s->get_sheet_size(value);
	}

	bool UndoAttr<UNDO_OP::STROKE_CAP>::GET(const Shape* s, CAP_STYLE& value) noexcept
	{
		return s->get_stroke_cap(value);
	}

	bool UndoAttr<UNDO_OP::STROKE_COLOR>::GET(const Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_stroke_color(value);
	}

	bool UndoAttr<UNDO_OP::STROKE_WIDTH>::GET(const Shape* s, float& value) noexcept
	{
		return s->get_stroke_width(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_ALIGN_P>::GET(const Shape* s, DWRITE_PARAGRAPH_ALIGNMENT& value) noexcept
	{
		return s->get_text_align_p(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_ALIGN_T>::GET(const Shape* s, DWRITE_TEXT_ALIGNMENT& value) noexcept
	{
		return s->get_text_align_t(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_CONTENT>::GET(const Shape* s, wchar_t*& value) noexcept
	{
		return s->get_text_content(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_LINE_SP>::GET(const Shape* s, float& value) noexcept
	{
		return s->get_text_line_sp(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_MARGIN>::GET(const Shape* s, D2D1_SIZE_F& value) noexcept
	{
		return s->get_text_padding(value);
	}

	bool UndoAttr<UNDO_OP::TEXT_RANGE>::GET(const Shape* s, DWRITE_TEXT_RANGE& value) noexcept
	{
		return s->get_text_range(value);
	}

	// データライターに書き込む.
	template <UNDO_OP U> void UndoAttr<U>::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(U));
		undo_write_shape(m_shape, dt_writer);
		if constexpr (
			U == UNDO_OP::FONT_SIZE ||
			U == UNDO_OP::GRID_BASE ||
			U == UNDO_OP::IMAGE_OPAC ||
			U == UNDO_OP::JOIN_LIMIT ||
			U == UNDO_OP::STROKE_WIDTH ||
			U == UNDO_OP::TEXT_LINE_SP
			) {
			dt_writer.WriteSingle(m_value);
		}
		else if constexpr (
			U == UNDO_OP::ARROW_STYLE ||
			U == UNDO_OP::DASH_CAP ||
			U == UNDO_OP::DASH_STYLE ||
			U == UNDO_OP::FONT_STRETCH ||
			U == UNDO_OP::FONT_STYLE ||
			U == UNDO_OP::FONT_WEIGHT ||
			U == UNDO_OP::GRID_SHOW ||
			U == UNDO_OP::JOIN_STYLE ||
			U == UNDO_OP::TEXT_ALIGN_P ||
			U == UNDO_OP::TEXT_ALIGN_T
			) {
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_value));
		}
		//else if constexpr (U == UNDO_OP::IMAGE_ASPECT) {
		//	dt_writer.WriteBoolean(m_value);
		//}
		else {
			dt_write(m_value, dt_writer);
		}
	}

	// 操作を実行すると値が変わるか判定する.
	bool UndoImage::changed(void) const noexcept
	{
		//using winrt::GraphPaper::implementation::equal;

		const auto pos = static_cast<ShapeImage*>(m_shape)->m_pos;
		const auto view = static_cast<ShapeImage*>(m_shape)->m_view;
		const auto rect = static_cast<ShapeImage*>(m_shape)->m_rect;
		const auto ratio = static_cast<ShapeImage*>(m_shape)->m_ratio;
		const auto opac = static_cast<ShapeImage*>(m_shape)->m_opac;
		return !equal(pos, m_pos) || !equal(view, m_view) || !equal(rect, m_rect) || !equal(ratio, m_ratio) || !eqaul(opac, m_opac);
	}

	// 元に戻す操作を実行する.
	void UndoImage::exec(void)
	{
		ShapeImage* s = static_cast<ShapeImage*>(m_shape);
		const auto pos = s->m_pos;
		const auto view = s->m_view;
		const auto rect = s->m_rect;
		const auto ratio = s->m_ratio;
		const auto opac = s->m_opac;
		s->m_pos = m_pos;
		s->m_view = m_view;
		s->m_rect = m_rect;
		s->m_ratio = m_ratio;
		s->m_opac = m_opac;
		m_pos = pos;
		m_view = view;
		m_rect = rect;
		m_ratio = ratio;
		m_opac = opac;
	}

	// データリーダーから操作を読み込む.
	UndoImage::UndoImage(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader)),
		m_pos(undo_read_pos(dt_reader)),
		m_view(undo_read_size(dt_reader)),
		m_rect(undo_read_rect(dt_reader)),
		m_ratio(undo_read_size(dt_reader)),
		m_opac(undo_read_opac(dt_reader))
	{}

	UndoImage::UndoImage(ShapeImage* const s) :
		Undo(s),
		m_pos(s->m_pos),
		m_view(s->m_view),
		m_rect(s->m_rect),
		m_ratio(s->m_ratio),
		m_opac(s->m_opac)
	{}

	// データライターに書き込む.
	void UndoImage::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::IMAGE));
		undo_write_shape(m_shape, dt_writer);
		dt_write(m_pos, dt_writer);
		dt_write(m_view, dt_writer);
		dt_write(m_rect, dt_writer);
		dt_write(m_ratio, dt_writer);
		dt_write(m_opac, dt_writer);
	}

	// 操作を実行する.
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

	// データリーダーから操作を読み込む.
	UndoList::UndoList(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		m_insert = dt_reader.ReadBoolean();
		m_shape_at = undo_read_shape(dt_reader);
	}

	// 図形をリストから取り除く.
	// s	取り除く図形
	// dont_exec	初期化のみで操作を実行しない.
	UndoList::UndoList(Shape* const s, const bool dont_exec) :
		Undo(s),
		m_insert(false),
		m_shape_at(static_cast<Shape*>(nullptr))
	{
		if (dont_exec != true) {
			exec();
		}
	}

	// 図形をリストに挿入する
	// s	挿入する図形
	// p	挿入する位置にある図形
	// dont_exec	初期化のみで操作を実行しない.
	UndoList::UndoList(Shape* const s, Shape* const p, const bool dont_exec) :
		Undo(s),
		m_insert(true),
		m_shape_at(p)
	{
		if (dont_exec != true) {
			exec();
		}
	}

	// 操作をデータライターに書き込む.
	void UndoList::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::LIST));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_shape_at, dt_writer);
	}

	// 操作を実行する.
	void UndoListGroup::exec(void)
	{
		if (m_insert) {
			auto it_del{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape) };
			if (it_del != s_shape_list->end()) {
				s_shape_list->erase(it_del);
			}
			SHAPE_LIST& list_grouped = m_shape_group->m_list_grouped;
			auto it_ins{ std::find(list_grouped.begin(), list_grouped.end(), m_shape_at) };
			list_grouped.insert(it_ins, m_shape);
			m_shape->set_delete(false);
		}
		else {
			m_shape->set_delete(true);
			SHAPE_LIST& list_grouped = m_shape_group->m_list_grouped;
			auto it_del{ std::find(list_grouped.begin(), list_grouped.end(), m_shape) };
			auto it_pos{ list_grouped.erase(it_del) };
			m_shape_at = (it_pos == list_grouped.end() ? nullptr : *it_pos);
			s_shape_list->push_front(m_shape);
		}
		m_insert = !m_insert;
	}

	// データリーダーから操作を読み込む.
	UndoListGroup::UndoListGroup(DataReader const& dt_reader) :
		UndoList(dt_reader),
		m_shape_group(static_cast<ShapeGroup*>(undo_read_shape(dt_reader)))
	{}

	// 図形をグループから取り除く.
	// s	取り除く図形
	UndoListGroup::UndoListGroup(ShapeGroup* const g, Shape* const s) :
		UndoList(s, true),
		m_shape_group(g)
	{
		exec();
	}

	// 図形をグループに追加する.
	// s	取り除く図形
	UndoListGroup::UndoListGroup(ShapeGroup* const g, Shape* const s, Shape* const s_pos) :
		UndoList(s, s_pos, true),
		m_shape_group(g)
	{
		exec();
	}

	// 操作をデータライターに書き込む.
	void UndoListGroup::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::GROUP));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_shape_at, dt_writer);
		undo_write_shape(m_shape_group, dt_writer);
	}

	// 操作を実行する.
	void UndoSelect::exec(void)
	{
		m_shape->set_select(!m_shape->is_selected());
	}

	// データリーダーから操作を読み込む.
	UndoSelect::UndoSelect(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{}

	// 図形の選択を反転する.
	UndoSelect::UndoSelect(Shape* const s) :
		Undo(s)
	{
		exec();
	}

	// データライターに書き込む.
	void UndoSelect::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::SELECT));
		undo_write_shape(m_shape, dt_writer);
	}

}