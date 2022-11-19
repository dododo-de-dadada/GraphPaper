#include "pch.h"
#include "undo.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;

	constexpr auto INDEX_NIL = static_cast<uint32_t>(-2);	// ヌル図形の添え字
	constexpr auto INDEX_SHEET = static_cast<uint32_t>(-1);	// 用紙図形の添え字

	// 部位の位置を得る.
	static D2D1_POINT_2F undo_get_pos_anc(const Shape* s, const uint32_t anc) noexcept;
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
	static D2D1_POINT_2F undo_get_pos_anc(const Shape* s, const uint32_t anc) noexcept
	{
		D2D1_POINT_2F pos;
		s->get_pos_anc(anc, pos);
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
		Undo::s_shape_list = slist;
		Undo::s_shape_sheet = s_layout;
	}

	// 操作を実行すると値が変わるか判定する.
	bool UndoForm::changed(void) const noexcept
	{
		using winrt::GraphPaper::implementation::equal;

		D2D1_POINT_2F pos;
		m_shape->get_pos_anc(m_anc, pos);
		return !equal(pos, m_pos);
	}

	// 元に戻す操作を実行する.
	void UndoForm::exec(void)
	{
		D2D1_POINT_2F pos;
		m_shape->get_pos_anc(m_anc, pos);
		m_shape->set_pos_anc(m_pos, m_anc, 0.0f, false);
		m_pos = pos;
	}

	// データリーダーから操作を読み込む.
	UndoForm::UndoForm(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader)),
		m_anc(static_cast<ANC_TYPE>(dt_reader.ReadUInt32())),
		m_pos(undo_read_pos(dt_reader))
	{}

	// 図形の形を保存する.
	UndoForm::UndoForm(Shape* const s, const uint32_t anc) :
		Undo(s),
		m_anc(anc),
		m_pos(undo_get_pos_anc(s, anc))
	{}

	// 図形の形の操作をデータライターに書き込む.
	void UndoForm::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::FORM));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_anc));
		dt_write(m_pos, dt_writer);
	}

	// 操作を実行する.
	void UndoOrder::exec(void)
	{
		if (m_shape == m_dst_shape) {
			return;
		}
		auto it_beg{ Undo::s_shape_list->begin() };
		auto it_end{ Undo::s_shape_list->end() };
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
	UndoOrder::UndoOrder(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader)),
		m_dst_shape(undo_read_shape(dt_reader))
	{
		//m_dst_shape = undo_read_shape(dt_reader);
	}

	// 操作を作成する.
	UndoOrder::UndoOrder(Shape* const s, Shape* const t) :
		Undo(s),
		m_dst_shape(t)
	{
		//m_dst_shape = t;
		exec();
	}

	// 図形の入れ替え操作をデータライターに書き込む.
	void UndoOrder::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::ORDER));
		undo_write_shape(m_shape, dt_writer);
		undo_write_shape(m_dst_shape, dt_writer);
	}

	// 操作を実行すると値が変わるか判定する.
	template <UNDO_OP U>
	bool UndoValue<U>::changed(void) const noexcept
	{
		using winrt::GraphPaper::implementation::equal;
		U_TYPE<U>::type val{};
		return GET(m_shape, val) && !equal(val, m_value);
	}

	// 操作を実行する.
	template <UNDO_OP U>
	void UndoValue<U>::exec(void)
	{
		U_TYPE<U>::type old_val{};
		if (GET(m_shape, old_val)) {
			SET(m_shape, m_value);
			m_value = old_val;
		}
	}

	// 図形の属性値を保存する.
	template <UNDO_OP U>
	UndoValue<U>::UndoValue(Shape* s) :
		Undo(s)
	{
		GET(m_shape, m_value);
	}

	// 図形の属性値を保存したあと値を格納する.
	template <UNDO_OP U>
	UndoValue<U>::UndoValue(Shape* s, U_TYPE<U>::type const& val) :
		UndoValue(s)
	{
		UndoValue<U>::SET(m_shape, val);
	}
	template UndoValue<UNDO_OP::ARROW_SIZE>::UndoValue(Shape* s, const ARROW_SIZE& val);
	template UndoValue<UNDO_OP::ARROW_STYLE>::UndoValue(Shape* s, const ARROW_STYLE& val);
	template UndoValue<UNDO_OP::DASH_CAP>::UndoValue(Shape* s, const D2D1_CAP_STYLE& val);
	template UndoValue<UNDO_OP::DASH_PATT>::UndoValue(Shape* s, const DASH_PATT& val);
	template UndoValue<UNDO_OP::DASH_STYLE>::UndoValue(Shape* s, const D2D1_DASH_STYLE& val);
	template UndoValue<UNDO_OP::FILL_COLOR>::UndoValue(Shape* s, const D2D1_COLOR_F& val);
	template UndoValue<UNDO_OP::FONT_COLOR>::UndoValue(Shape* s, const D2D1_COLOR_F& val);
	template UndoValue<UNDO_OP::FONT_FAMILY>::UndoValue(Shape* s, wchar_t* const& val);
	template UndoValue<UNDO_OP::FONT_SIZE>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_OP::FONT_STRETCH>::UndoValue(Shape* s, const DWRITE_FONT_STRETCH& val);
	template UndoValue<UNDO_OP::FONT_STYLE>::UndoValue(Shape* s, const DWRITE_FONT_STYLE& val);
	template UndoValue<UNDO_OP::FONT_WEIGHT>::UndoValue(Shape* s, const DWRITE_FONT_WEIGHT& val);
	template UndoValue<UNDO_OP::GRID_BASE>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_OP::GRID_COLOR>::UndoValue(Shape* s, const D2D1_COLOR_F& val);
	template UndoValue<UNDO_OP::GRID_EMPH>::UndoValue(Shape* s, const GRID_EMPH& val);
	template UndoValue<UNDO_OP::GRID_SHOW>::UndoValue(Shape* s, const GRID_SHOW& val);
	template UndoValue<UNDO_OP::IMAGE_OPAC>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_OP::JOIN_LIMIT>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_OP::JOIN_STYLE>::UndoValue(Shape* s, const D2D1_LINE_JOIN& val);
	template UndoValue<UNDO_OP::MOVE>::UndoValue(Shape* s, const D2D1_POINT_2F& val);
	template UndoValue<UNDO_OP::SHEET_COLOR>::UndoValue(Shape* s, const D2D1_COLOR_F& val);
	template UndoValue<UNDO_OP::SHEET_SIZE>::UndoValue(Shape* s, const D2D1_SIZE_F& val);
	template UndoValue<UNDO_OP::STROKE_CAP>::UndoValue(Shape* s, const CAP_STYLE& val);
	template UndoValue<UNDO_OP::STROKE_COLOR>::UndoValue(Shape* s, const D2D1_COLOR_F& val);
	template UndoValue<UNDO_OP::STROKE_WIDTH>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_OP::TEXT_ALIGN_P>::UndoValue(Shape* s, const DWRITE_PARAGRAPH_ALIGNMENT& val);
	template UndoValue<UNDO_OP::TEXT_ALIGN_T>::UndoValue(Shape* s, const DWRITE_TEXT_ALIGNMENT& val);
	template UndoValue<UNDO_OP::TEXT_CONTENT>::UndoValue(Shape* s, wchar_t* const& val);
	template UndoValue<UNDO_OP::TEXT_LINE_SP>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_OP::TEXT_MARGIN>::UndoValue(Shape* s, const D2D1_SIZE_F& val);
	template UndoValue<UNDO_OP::TEXT_SELECTED>::UndoValue(Shape* s, const DWRITE_TEXT_RANGE& val);

	template <UNDO_OP U> UndoValue<U>::UndoValue(DataReader const& dt_reader) :
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

	template UndoValue<UNDO_OP::ARROW_SIZE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::ARROW_STYLE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::DASH_CAP>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::DASH_PATT>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::DASH_STYLE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::FILL_COLOR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::FONT_COLOR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::FONT_FAMILY>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::FONT_SIZE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::FONT_STRETCH>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::FONT_STYLE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::FONT_WEIGHT>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::GRID_BASE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::GRID_COLOR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::GRID_EMPH>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::GRID_SHOW>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::IMAGE_OPAC>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::JOIN_LIMIT>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::JOIN_STYLE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::MOVE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::SHEET_COLOR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::SHEET_SIZE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::STROKE_CAP>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::STROKE_COLOR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::STROKE_WIDTH>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::TEXT_ALIGN_T>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::TEXT_ALIGN_P>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::TEXT_CONTENT>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::TEXT_LINE_SP>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::TEXT_MARGIN>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_OP::TEXT_SELECTED>::UndoValue(DataReader const& dt_reader);

	// 図形の属性値に値を格納する.
	template <UNDO_OP U> void UndoValue<U>::SET(Shape* const s, const U_TYPE<U>::type& val)
	{
		throw winrt::hresult_not_implemented();
	}

	void UndoValue<UNDO_OP::ARROW_SIZE>::SET(Shape* const s, const ARROW_SIZE& val)
	{
		s->set_arrow_size(val);
	}

	void UndoValue<UNDO_OP::ARROW_STYLE>::SET(Shape* const s, const ARROW_STYLE& val)
	{
		s->set_arrow_style(val);
	}

	void UndoValue<UNDO_OP::DASH_CAP>::SET(Shape* const s, const D2D1_CAP_STYLE& val)
	{
		s->set_dash_cap(val);
	}

	void UndoValue<UNDO_OP::DASH_PATT>::SET(Shape* const s, const DASH_PATT& val)
	{
		s->set_dash_patt(val);
	}

	void UndoValue<UNDO_OP::DASH_STYLE>::SET(Shape* const s, const D2D1_DASH_STYLE& val)
	{
		s->set_dash_style(val);
	}

	void UndoValue<UNDO_OP::FILL_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& val)
	{
		s->set_fill_color(val);
	}

	void UndoValue<UNDO_OP::FONT_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& val)
	{
		s->set_font_color(val);
	}

	void UndoValue<UNDO_OP::FONT_FAMILY>::SET(Shape* const s, wchar_t* const& val)
	{
		s->set_font_family(val);
	}

	void UndoValue<UNDO_OP::FONT_SIZE>::SET(Shape* const s, const float& val)
	{
		s->set_font_size(val);
	}

	void UndoValue<UNDO_OP::FONT_STRETCH>::SET(Shape* const s, const DWRITE_FONT_STRETCH& val)
	{
		s->set_font_stretch(val);
	}

	void UndoValue<UNDO_OP::FONT_STYLE>::SET(Shape* const s, const DWRITE_FONT_STYLE& val)
	{
		s->set_font_style(val);
	}

	void UndoValue<UNDO_OP::FONT_WEIGHT>::SET(Shape* const s, const DWRITE_FONT_WEIGHT& val)
	{
		s->set_font_weight(val);
	}

	void UndoValue<UNDO_OP::GRID_BASE>::SET(Shape* const s, const float& val)
	{
		s->set_grid_base(val);
	}

	void UndoValue<UNDO_OP::GRID_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& val)
	{
		s->set_grid_color(val);
	}

	void UndoValue<UNDO_OP::GRID_EMPH>::SET(Shape* const s, const GRID_EMPH& val)
	{
		s->set_grid_emph(val);
	}

	void UndoValue<UNDO_OP::GRID_SHOW>::SET(Shape* const s, const GRID_SHOW& val)
	{
		s->set_grid_show(val);
	}

	void UndoValue<UNDO_OP::IMAGE_OPAC>::SET(Shape* const s, const float& val)
	{
		s->set_image_opacity(val);
	}

	void UndoValue<UNDO_OP::JOIN_LIMIT>::SET(Shape* const s, const float& val)
	{
		s->set_join_limit(val);
	}

	void UndoValue<UNDO_OP::JOIN_STYLE>::SET(Shape* const s, const D2D1_LINE_JOIN& val)
	{
		s->set_join_style(val);
	}

	void UndoValue<UNDO_OP::MOVE>::SET(Shape* const s, const D2D1_POINT_2F& val)
	{
		s->set_pos_start(val);
	}

	void UndoValue<UNDO_OP::SHEET_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& val)
	{
		s->set_sheet_color(val);
	}

	void UndoValue<UNDO_OP::SHEET_SIZE>::SET(Shape* const s, const D2D1_SIZE_F& val)
	{
		s->set_sheet_size(val);
	}

	void UndoValue<UNDO_OP::STROKE_CAP>::SET(Shape* const s, const CAP_STYLE& val)
	{
		s->set_stroke_cap(val);
	}

	void UndoValue<UNDO_OP::STROKE_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& val)
	{
		s->set_stroke_color(val);
	}

	void UndoValue<UNDO_OP::STROKE_WIDTH>::SET(Shape* const s, const float& val)
	{
		s->set_stroke_width(val);
	}

	void UndoValue<UNDO_OP::TEXT_ALIGN_P>::SET(Shape* const s, const DWRITE_PARAGRAPH_ALIGNMENT& val)
	{
		s->set_text_align_p(val);
	}

	void UndoValue<UNDO_OP::TEXT_ALIGN_T>::SET(Shape* const s, const DWRITE_TEXT_ALIGNMENT& val)
	{
		s->set_text_align_t(val);
	}

	void UndoValue<UNDO_OP::TEXT_CONTENT>::SET(Shape* const s, wchar_t* const& val)
	{
		s->set_text_content(val);
	}

	void UndoValue<UNDO_OP::TEXT_LINE_SP>::SET(Shape* const s, const float& val)
	{
		s->set_text_line_sp(val);
	}

	void UndoValue<UNDO_OP::TEXT_MARGIN>::SET(Shape* const s, const D2D1_SIZE_F& val)
	{
		s->set_text_padding(val);
	}

	void UndoValue<UNDO_OP::TEXT_SELECTED>::SET(Shape* const s, const DWRITE_TEXT_RANGE& val)
	{
		s->set_text_selected(val);
	}

	template <UNDO_OP U> bool UndoValue<U>::GET(const Shape* s, U_TYPE<U>::type& val) noexcept
	{
		return false;
	}

	bool UndoValue<UNDO_OP::ARROW_SIZE>::GET(const Shape* s, ARROW_SIZE& val) noexcept
	{
		return s->get_arrow_size(val);
	}

	bool UndoValue<UNDO_OP::ARROW_STYLE>::GET(const Shape* s, ARROW_STYLE& val) noexcept
	{
		return s->get_arrow_style(val);
	}

	bool UndoValue<UNDO_OP::DASH_CAP>::GET(const Shape* s, D2D1_CAP_STYLE& val) noexcept
	{
		return s->get_dash_cap(val);
	}

	bool UndoValue<UNDO_OP::DASH_PATT>::GET(const Shape* s, DASH_PATT& val) noexcept
	{
		return s->get_dash_patt(val);
	}

	bool UndoValue<UNDO_OP::DASH_STYLE>::GET(const Shape* s, D2D1_DASH_STYLE& val) noexcept
	{
		return s->get_dash_style(val);
	}

	bool UndoValue<UNDO_OP::FILL_COLOR>::GET(const Shape* s, D2D1_COLOR_F& val) noexcept
	{
		return s->get_fill_color(val);
	}

	bool UndoValue<UNDO_OP::FONT_COLOR>::GET(const Shape* s, D2D1_COLOR_F& val) noexcept
	{
		return s->get_font_color(val);
	}

	bool UndoValue<UNDO_OP::FONT_FAMILY>::GET(const Shape* s, wchar_t*& val) noexcept
	{
		return s->get_font_family(val);
	}

	bool UndoValue<UNDO_OP::FONT_SIZE>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_font_size(val);
	}

	bool UndoValue<UNDO_OP::FONT_STRETCH>::GET(const Shape* s, DWRITE_FONT_STRETCH& val) noexcept
	{
		return s->get_font_stretch(val);
	}

	bool UndoValue<UNDO_OP::FONT_STYLE>::GET(const Shape* s, DWRITE_FONT_STYLE& val) noexcept
	{
		return s->get_font_style(val);
	}

	bool UndoValue<UNDO_OP::FONT_WEIGHT>::GET(const Shape* s, DWRITE_FONT_WEIGHT& val) noexcept
	{
		return s->get_font_weight(val);
	}

	bool UndoValue<UNDO_OP::GRID_BASE>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_grid_base(val);
	}

	bool UndoValue<UNDO_OP::GRID_COLOR>::GET(const Shape* s, D2D1_COLOR_F& val) noexcept
	{
		return s->get_grid_color(val);
	}

	bool UndoValue<UNDO_OP::GRID_EMPH>::GET(const Shape* s, GRID_EMPH& val) noexcept
	{
		return s->get_grid_emph(val);
	}

	bool UndoValue<UNDO_OP::GRID_SHOW>::GET(const Shape* s, GRID_SHOW& val) noexcept
	{
		return s->get_grid_show(val);
	}

	bool UndoValue<UNDO_OP::IMAGE_OPAC>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_image_opacity(val);
	}

	bool UndoValue<UNDO_OP::JOIN_LIMIT>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_join_limit(val);
	}

	bool UndoValue<UNDO_OP::JOIN_STYLE>::GET(const Shape* s, D2D1_LINE_JOIN& val) noexcept
	{
		return s->get_join_style(val);
	}

	bool UndoValue<UNDO_OP::MOVE>::GET(const Shape* s, D2D1_POINT_2F& val) noexcept
	{
		return s->get_pos_start(val);
	}

	bool UndoValue<UNDO_OP::SHEET_COLOR>::GET(const Shape* s, D2D1_COLOR_F& val) noexcept
	{
		return s->get_sheet_color(val);
	}

	bool UndoValue<UNDO_OP::SHEET_SIZE>::GET(const Shape* s, D2D1_SIZE_F& val) noexcept
	{
		return s->get_sheet_size(val);
	}

	bool UndoValue<UNDO_OP::STROKE_CAP>::GET(const Shape* s, CAP_STYLE& val) noexcept
	{
		return s->get_stroke_cap(val);
	}

	bool UndoValue<UNDO_OP::STROKE_COLOR>::GET(const Shape* s, D2D1_COLOR_F& val) noexcept
	{
		return s->get_stroke_color(val);
	}

	bool UndoValue<UNDO_OP::STROKE_WIDTH>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_stroke_width(val);
	}

	bool UndoValue<UNDO_OP::TEXT_ALIGN_P>::GET(const Shape* s, DWRITE_PARAGRAPH_ALIGNMENT& val) noexcept
	{
		return s->get_text_align_p(val);
	}

	bool UndoValue<UNDO_OP::TEXT_ALIGN_T>::GET(const Shape* s, DWRITE_TEXT_ALIGNMENT& val) noexcept
	{
		return s->get_text_align_t(val);
	}

	bool UndoValue<UNDO_OP::TEXT_CONTENT>::GET(const Shape* s, wchar_t*& val) noexcept
	{
		return s->get_text_content(val);
	}

	bool UndoValue<UNDO_OP::TEXT_LINE_SP>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_text_line_sp(val);
	}

	bool UndoValue<UNDO_OP::TEXT_MARGIN>::GET(const Shape* s, D2D1_SIZE_F& val) noexcept
	{
		return s->get_text_padding(val);
	}

	bool UndoValue<UNDO_OP::TEXT_SELECTED>::GET(const Shape* s, DWRITE_TEXT_RANGE& val) noexcept
	{
		return s->get_text_selected(val);
	}

	// 図形の値の操作をデータライターに書き込む.
	template <UNDO_OP U> void UndoValue<U>::write(DataWriter const& dt_writer)
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
		const ShapeImage* s = static_cast<const ShapeImage*>(m_shape);
		const auto pos = s->m_pos;
		const auto view = s->m_view;
		const auto clip = s->m_clip;
		const auto ratio = s->m_ratio;
		const auto opac = s->m_opac;
		return !equal(pos, m_pos) || !equal(view, m_view) || !equal(clip, m_clip) || !equal(ratio, m_ratio) || !equal(opac, m_opac);
	}

	// 元に戻す操作を実行する.
	void UndoImage::exec(void)
	{
		ShapeImage* s = static_cast<ShapeImage*>(m_shape);
		const auto pos = s->m_pos;
		const auto view = s->m_view;
		const auto clip = s->m_clip;
		const auto ratio = s->m_ratio;
		const auto opac = s->m_opac;
		s->m_pos = m_pos;
		s->m_view = m_view;
		s->m_clip = m_clip;
		s->m_ratio = m_ratio;
		s->m_opac = m_opac;
		m_pos = pos;
		m_view = view;
		m_clip = clip;
		m_ratio = ratio;
		m_opac = opac;
	}

	// データリーダーから操作を読み込む.
	UndoImage::UndoImage(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader)),
		m_pos(undo_read_pos(dt_reader)),
		m_view(undo_read_size(dt_reader)),
		m_clip(undo_read_rect(dt_reader)),
		m_ratio(undo_read_size(dt_reader)),
		m_opac(dt_reader.ReadSingle())
	{}

	UndoImage::UndoImage(ShapeImage* const s) :
		Undo(s),
		m_pos(s->m_pos),
		m_view(s->m_view),
		m_clip(s->m_clip),
		m_ratio(s->m_ratio),
		m_opac(s->m_opac)
	{}

	// 画像の操作をデータライターに書き込む.
	void UndoImage::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::IMAGE));
		undo_write_shape(m_shape, dt_writer);
		dt_write(m_pos, dt_writer);
		dt_write(m_view, dt_writer);
		dt_write(m_clip, dt_writer);
		dt_write(m_ratio, dt_writer);
		dt_writer.WriteSingle(m_opac);
	}

	// 操作を実行する.
	void UndoList::exec(void)
	{
		if (m_insert) {
			auto it_del{ std::find(Undo::s_shape_list->begin(), Undo::s_shape_list->end(), m_shape) };
			if (it_del != Undo::s_shape_list->end()) {
				Undo::s_shape_list->erase(it_del);
			}
			auto it_ins{ std::find(Undo::s_shape_list->begin(), Undo::s_shape_list->end(), m_shape_at) };
			Undo::s_shape_list->insert(it_ins, m_shape);
			m_shape->set_delete(false);
		}
		else {
			m_shape->set_delete(true);
			auto it_del{ std::find(Undo::s_shape_list->begin(), Undo::s_shape_list->end(), m_shape) };
			auto it_pos{ Undo::s_shape_list->erase(it_del) };
			m_shape_at = (it_pos == Undo::s_shape_list->end() ? nullptr : *it_pos);
			Undo::s_shape_list->push_front(m_shape);
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
		if (!dont_exec) {
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
		if (!dont_exec) {
			exec();
		}
	}

	// 追加と削除の操作をデータライターに書き込む.
	void UndoList::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::LIST));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_shape_at, dt_writer);
	}

	// 操作を実行する.
	void UndoGroup::exec(void)
	{
		if (m_insert) {
			auto it_del{ std::find(Undo::s_shape_list->begin(), Undo::s_shape_list->end(), m_shape) };
			if (it_del != Undo::s_shape_list->end()) {
				Undo::s_shape_list->erase(it_del);
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
			Undo::s_shape_list->push_front(m_shape);
		}
		m_insert = !m_insert;
	}

	// データリーダーから操作を読み込む.
	UndoGroup::UndoGroup(DataReader const& dt_reader) :
		UndoList(dt_reader),
		m_shape_group(static_cast<ShapeGroup*>(undo_read_shape(dt_reader)))
	{}

	// 図形をグループから取り除く.
	// s	取り除く図形
	UndoGroup::UndoGroup(ShapeGroup* const g, Shape* const s) :
		UndoList(s, true),
		m_shape_group(g)
	{
		exec();
	}

	// 図形をグループに追加する.
	// s	取り除く図形
	UndoGroup::UndoGroup(ShapeGroup* const g, Shape* const s, Shape* const s_pos) :
		UndoList(s, s_pos, true),
		m_shape_group(g)
	{
		exec();
	}

	// グループ操作をデータライターに書き込む.
	void UndoGroup::write(DataWriter const& dt_writer)
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

	// 図形の選択操作をデータライターに書き込む.
	void UndoSelect::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::SELECT));
		undo_write_shape(m_shape, dt_writer);
	}

}