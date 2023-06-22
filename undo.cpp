#include "pch.h"
#include "undo.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//SHAPE_LIST* Undo::undo_slist = nullptr;	// 操作が参照する図形リスト
	ShapeSheet* Undo::undo_sheet = nullptr;	// 操作が参照する用紙

	// 指定した部位の点を得る.
	static D2D1_POINT_2F undo_get_pos_loc(const Shape* s, const uint32_t loc) noexcept;
	// データリーダーから添え字を読み込んで図形を得る.
	static Shape* undo_read_shape(DataReader const& dt_reader);
	// データリーダーから位置を読み込む.
	//static D2D1_SIZE_F undo_read_size(DataReader const& dt_reader);
	// 図形をデータライターに書き込む.
	static void undo_write_shape(Shape* const s, DataWriter const& dt_writer);

	// 指定した部位の点を得る.
	static D2D1_POINT_2F undo_get_pos_loc(
		const Shape* s,	// 図形
		const uint32_t loc	// 部位
	) noexcept
	{
		D2D1_POINT_2F pos;
		s->get_pos_loc(loc, pos);
		return pos;
	}

	// 操作を実行する.
	void UndoOrder::exec(void) noexcept
	{
		if (m_shape == m_dst_shape) {
			return;
		}
		auto& slist = Undo::undo_sheet->m_shape_list;
		auto it_beg{ slist.begin() };
		auto it_end{ slist.end() };
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
	{}

	// 図形の入れ替え操作をデータライターに書き込む.
	void UndoOrder::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::ORDER));
		undo_write_shape(m_shape, dt_writer);
		undo_write_shape(m_dst_shape, dt_writer);
	}

	// 操作を実行する.
	template <UNDO_T U>
	void UndoValue<U>::exec(void) noexcept
	{
		U_TYPE<U>::type old_val{};
		if (UndoValue<U>::GET(m_shape, old_val)) {
			UndoValue<U>::SET(m_shape, m_value);
			m_value = old_val;
		}
	}

	template UndoValue<UNDO_T::ARC_DIR>::UndoValue(Shape* s, const D2D1_SWEEP_DIRECTION& val);
	template UndoValue<UNDO_T::ARC_END>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_T::ARC_ROT>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_T::ARC_START>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_T::ARROW_CAP>::UndoValue(Shape* s, const D2D1_CAP_STYLE& val);
	template UndoValue<UNDO_T::ARROW_JOIN>::UndoValue(Shape* s, const D2D1_LINE_JOIN& val);
	template UndoValue<UNDO_T::ARROW_SIZE>::UndoValue(Shape* s, const ARROW_SIZE& val);
	template UndoValue<UNDO_T::ARROW_STYLE>::UndoValue(Shape* s, const ARROW_STYLE& val);
	//template UndoValue<UNDO_T::DASH_CAP>::UndoValue(Shape* s, const D2D1_CAP_STYLE& val);
	template UndoValue<UNDO_T::DASH_PAT>::UndoValue(Shape* s, const DASH_PAT& val);
	template UndoValue<UNDO_T::DASH_STYLE>::UndoValue(Shape* s, const D2D1_DASH_STYLE& val);
	template UndoValue<UNDO_T::FILL_COLOR>::UndoValue(Shape* s, const D2D1_COLOR_F& val);
	template UndoValue<UNDO_T::FONT_COLOR>::UndoValue(Shape* s, const D2D1_COLOR_F& val);
	template UndoValue<UNDO_T::FONT_FAMILY>::UndoValue(Shape* s, wchar_t* const& val);
	template UndoValue<UNDO_T::FONT_SIZE>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_T::FONT_STRETCH>::UndoValue(Shape* s, const DWRITE_FONT_STRETCH& val);
	template UndoValue<UNDO_T::FONT_STYLE>::UndoValue(Shape* s, const DWRITE_FONT_STYLE& val);
	template UndoValue<UNDO_T::FONT_WEIGHT>::UndoValue(Shape* s, const DWRITE_FONT_WEIGHT& val);
	template UndoValue<UNDO_T::GRID_BASE>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_T::GRID_COLOR>::UndoValue(Shape* s, const D2D1_COLOR_F& val);
	template UndoValue<UNDO_T::GRID_EMPH>::UndoValue(Shape* s, const GRID_EMPH& val);
	template UndoValue<UNDO_T::GRID_SHOW>::UndoValue(Shape* s, const GRID_SHOW& val);
	template UndoValue<UNDO_T::IMAGE_OPAC>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_T::JOIN_LIMIT>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_T::JOIN_STYLE>::UndoValue(Shape* s, const D2D1_LINE_JOIN& val);
	template UndoValue<UNDO_T::MOVE>::UndoValue(Shape* s, const D2D1_POINT_2F& val);
	template UndoValue<UNDO_T::SHEET_COLOR>::UndoValue(Shape* s, const D2D1_COLOR_F& val);
	template UndoValue<UNDO_T::SHEET_SIZE>::UndoValue(Shape* s, const D2D1_SIZE_F& val);
	template UndoValue<UNDO_T::SHEET_PAD>::UndoValue(Shape* s, const D2D1_RECT_F& val);
	template UndoValue<UNDO_T::POLY_END>::UndoValue(Shape* s, const D2D1_FIGURE_END& val);
	template UndoValue<UNDO_T::STROKE_CAP>::UndoValue(Shape* s, const D2D1_CAP_STYLE& val);
	template UndoValue<UNDO_T::STROKE_COLOR>::UndoValue(Shape* s, const D2D1_COLOR_F& val);
	template UndoValue<UNDO_T::STROKE_WIDTH>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_T::TEXT_ALIGN_P>::UndoValue(Shape* s, const DWRITE_PARAGRAPH_ALIGNMENT& val);
	template UndoValue<UNDO_T::TEXT_ALIGN_T>::UndoValue(Shape* s, const DWRITE_TEXT_ALIGNMENT& val);
	template UndoValue<UNDO_T::TEXT_CONTENT>::UndoValue(Shape* s, wchar_t* const& val);
	template UndoValue<UNDO_T::TEXT_LINE_SP>::UndoValue(Shape* s, const float& val);
	template UndoValue<UNDO_T::TEXT_PAD>::UndoValue(Shape* s, const D2D1_SIZE_F& val);
	template UndoValue<UNDO_T::TEXT_WRAP>::UndoValue(Shape* s, const DWRITE_WORD_WRAPPING& val);
	template UndoValue<UNDO_T::CORE_TEXT_RANGE>::UndoValue(Shape* s, const CORE_TEXT_RANGE& val);

	template <UNDO_T U> 
	UndoValue<U>::UndoValue(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader)),
		m_value()
	{
		if constexpr (
			U == UNDO_T::ARC_END ||
			U == UNDO_T::ARC_ROT ||
			U == UNDO_T::ARC_START ||
			U == UNDO_T::FONT_SIZE ||
			U == UNDO_T::GRID_BASE ||
			U == UNDO_T::IMAGE_OPAC ||
			U == UNDO_T::JOIN_LIMIT ||
			U == UNDO_T::STROKE_WIDTH ||
			U == UNDO_T::TEXT_LINE_SP) {
			m_value = dt_reader.ReadSingle();
		}
		else if constexpr (
			U == UNDO_T::MOVE ||
			U == UNDO_T::SHEET_SIZE ||
			U == UNDO_T::TEXT_PAD) {
			m_value = U_TYPE<U>::type{
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle()
			};
		}
		else if constexpr (
			U == UNDO_T::ARROW_SIZE) {
			m_value = U_TYPE<U>::type{
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle()
			};
		}
		else if constexpr (
			U == UNDO_T::FILL_COLOR ||
			U == UNDO_T::FONT_COLOR ||
			U == UNDO_T::GRID_COLOR ||
			U == UNDO_T::SHEET_COLOR ||
			U == UNDO_T::STROKE_COLOR ||
			U == UNDO_T::SHEET_PAD
			) {
			m_value = U_TYPE<U>::type{
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle()
			};
		}
		else if constexpr (
			U == UNDO_T::DASH_PAT) {
			m_value = U_TYPE<U>::type{
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle(),
				dt_reader.ReadSingle()
			};
		}
		else if constexpr (
			U == UNDO_T::ARC_DIR ||
			U == UNDO_T::ARROW_CAP ||
			U == UNDO_T::ARROW_JOIN ||
			U == UNDO_T::ARROW_STYLE ||
			//U == UNDO_T::DASH_CAP ||
			U == UNDO_T::DASH_STYLE ||
			U == UNDO_T::FONT_STRETCH ||
			U == UNDO_T::FONT_STYLE ||
			U == UNDO_T::FONT_WEIGHT ||
			U == UNDO_T::GRID_SHOW ||
			U == UNDO_T::JOIN_STYLE ||
			U == UNDO_T::POLY_END ||
			U == UNDO_T::STROKE_CAP ||
			U == UNDO_T::TEXT_ALIGN_P ||
			U == UNDO_T::TEXT_ALIGN_T ||
			U == UNDO_T::TEXT_WRAP) {
			m_value = static_cast<U_TYPE<U>::type>(dt_reader.ReadUInt32());
		}
		else if constexpr (
			//U == UNDO_T::GRID_EMPH ||
			//U == UNDO_T::TEXT_RANGE) {
			U == UNDO_T::GRID_EMPH) {
				m_value = U_TYPE<U>::type{
				dt_reader.ReadUInt32(),
				dt_reader.ReadUInt32()
			};
		}
		else if constexpr (
			U == UNDO_T::CORE_TEXT_RANGE) {
			m_value = U_TYPE<U>::type{
			dt_reader.ReadUInt32(),
			dt_reader.ReadUInt32(),
			dt_reader.ReadBoolean()
			};
		}
		else if constexpr (
			U == UNDO_T::FONT_FAMILY ||
			U == UNDO_T::TEXT_CONTENT) {
			const size_t len = dt_reader.ReadUInt32();	// 文字数
			uint8_t* data = new uint8_t[2 * (len + 1)];
			dt_reader.ReadBytes(array_view(data, data + 2 * len));
			m_value = reinterpret_cast<wchar_t*>(data);
			m_value[len] = L'\0';
			if constexpr (U == UNDO_T::FONT_FAMILY) {
				ShapeText::is_available_font(m_value);
			}
		}
		else {
			throw winrt::hresult_not_implemented();
		}
	}
	template UndoValue<UNDO_T::ARC_DIR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::ARC_END>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::ARC_ROT>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::ARC_START>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::ARROW_CAP>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::ARROW_JOIN>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::ARROW_SIZE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::ARROW_STYLE>::UndoValue(DataReader const& dt_reader);
	//template UndoValue<UNDO_T::DASH_CAP>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::DASH_PAT>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::DASH_STYLE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::FILL_COLOR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::FONT_COLOR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::FONT_FAMILY>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::FONT_SIZE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::FONT_STRETCH>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::FONT_STYLE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::FONT_WEIGHT>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::GRID_BASE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::GRID_COLOR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::GRID_EMPH>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::GRID_SHOW>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::IMAGE_OPAC>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::JOIN_LIMIT>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::JOIN_STYLE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::MOVE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::SHEET_COLOR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::SHEET_SIZE>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::SHEET_PAD>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::POLY_END>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::STROKE_CAP>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::STROKE_COLOR>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::STROKE_WIDTH>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::TEXT_ALIGN_T>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::TEXT_ALIGN_P>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::TEXT_CONTENT>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::TEXT_LINE_SP>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::TEXT_PAD>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::TEXT_WRAP>::UndoValue(DataReader const& dt_reader);
	template UndoValue<UNDO_T::CORE_TEXT_RANGE>::UndoValue(DataReader const& dt_reader);

	// 図形の属性値に値を格納する.
	template <UNDO_T U> void UndoValue<U>::SET(Shape* const s, const U_TYPE<U>::type& val) noexcept
	{
		throw winrt::hresult_not_implemented();
	}

	void UndoValue<UNDO_T::ARC_DIR>::SET(Shape* const s, const D2D1_SWEEP_DIRECTION& val) noexcept
	{
		s->set_arc_dir(val);
	}

	void UndoValue<UNDO_T::ARC_END>::SET(Shape* const s, const float& val) noexcept
	{
		s->set_arc_end(val);
	}

	void UndoValue<UNDO_T::ARC_ROT>::SET(Shape* const s, const float& val) noexcept
	{
		s->set_arc_rot(val);
	}

	void UndoValue<UNDO_T::ARC_START>::SET(Shape* const s, const float& val) noexcept
	{
		s->set_arc_start(val);
	}

	void UndoValue<UNDO_T::ARROW_CAP>::SET(Shape* const s, const D2D1_CAP_STYLE& val) noexcept
	{
		s->set_arrow_cap(val);
	}

	void UndoValue<UNDO_T::ARROW_JOIN>::SET(Shape* const s, const D2D1_LINE_JOIN& val) noexcept
	{
		s->set_arrow_join(val);
	}

	void UndoValue<UNDO_T::ARROW_SIZE>::SET(Shape* const s, const ARROW_SIZE& val) noexcept
	{
		s->set_arrow_size(val);
	}

	void UndoValue<UNDO_T::ARROW_STYLE>::SET(Shape* const s, const ARROW_STYLE& val) noexcept
	{
		s->set_arrow_style(val);
	}

	//void UndoValue<UNDO_T::DASH_CAP>::SET(Shape* const s, const D2D1_CAP_STYLE& val) noexcept
	//{
	//	s->set_dash_cap(val);
	//}

	void UndoValue<UNDO_T::DASH_PAT>::SET(Shape* const s, const DASH_PAT& val) noexcept
	{
		s->set_stroke_dash_pat(val);
	}

	void UndoValue<UNDO_T::DASH_STYLE>::SET(Shape* const s, const D2D1_DASH_STYLE& val) noexcept
	{
		s->set_stroke_dash(val);
	}

	void UndoValue<UNDO_T::FILL_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& val) noexcept
	{
		s->set_fill_color(val);
	}

	void UndoValue<UNDO_T::FONT_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& val) noexcept
	{
		s->set_font_color(val);
	}

	void UndoValue<UNDO_T::FONT_FAMILY>::SET(Shape* const s, wchar_t* const& val) noexcept
	{
		s->set_font_family(val);
	}

	void UndoValue<UNDO_T::FONT_SIZE>::SET(Shape* const s, const float& val) noexcept
	{
		s->set_font_size(val);
	}

	void UndoValue<UNDO_T::FONT_STRETCH>::SET(Shape* const s, const DWRITE_FONT_STRETCH& val) noexcept
	{
		s->set_font_stretch(val);
	}

	void UndoValue<UNDO_T::FONT_STYLE>::SET(Shape* const s, const DWRITE_FONT_STYLE& val) noexcept
	{
		s->set_font_style(val);
	}

	void UndoValue<UNDO_T::FONT_WEIGHT>::SET(Shape* const s, const DWRITE_FONT_WEIGHT& val) noexcept
	{
		s->set_font_weight(val);
	}

	void UndoValue<UNDO_T::GRID_BASE>::SET(Shape* const s, const float& val) noexcept
	{
		s->set_grid_base(val);
	}

	void UndoValue<UNDO_T::GRID_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& val) noexcept
	{
		s->set_grid_color(val);
	}

	void UndoValue<UNDO_T::GRID_EMPH>::SET(Shape* const s, const GRID_EMPH& val) noexcept
	{
		s->set_grid_emph(val);
	}

	void UndoValue<UNDO_T::GRID_SHOW>::SET(Shape* const s, const GRID_SHOW& val) noexcept
	{
		s->set_grid_show(val);
	}

	void UndoValue<UNDO_T::IMAGE_OPAC>::SET(Shape* const s, const float& val) noexcept
	{
		s->set_image_opacity(val);
	}

	void UndoValue<UNDO_T::JOIN_LIMIT>::SET(Shape* const s, const float& val) noexcept
	{
		s->set_stroke_join_limit(val);
	}

	void UndoValue<UNDO_T::JOIN_STYLE>::SET(Shape* const s, const D2D1_LINE_JOIN& val) noexcept
	{
		s->set_stroke_join(val);
	}

	void UndoValue<UNDO_T::MOVE>::SET(Shape* const s, const D2D1_POINT_2F& val) noexcept
	{
		s->set_pos_start(val);
	}

	void UndoValue<UNDO_T::SHEET_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& val) noexcept
	{
		s->set_sheet_color(val);
	}

	void UndoValue<UNDO_T::SHEET_SIZE>::SET(Shape* const s, const D2D1_SIZE_F& val) noexcept
	{
		s->set_sheet_size(val);
	}

	void UndoValue<UNDO_T::SHEET_PAD>::SET(Shape* const s, const D2D1_RECT_F& val) noexcept
	{
		s->set_sheet_padding(val);
	}
	void UndoValue<UNDO_T::POLY_END>::SET(Shape* const s, const D2D1_FIGURE_END& val) noexcept
	{
		s->set_poly_end(val);
	}

	void UndoValue<UNDO_T::STROKE_CAP>::SET(Shape* const s, const D2D1_CAP_STYLE& val) noexcept
	{
		s->set_stroke_cap(val);
	}

	void UndoValue<UNDO_T::STROKE_COLOR>::SET(Shape* const s, const D2D1_COLOR_F& val) noexcept
	{
		s->set_stroke_color(val);
	}

	void UndoValue<UNDO_T::STROKE_WIDTH>::SET(Shape* const s, const float& val) noexcept
	{
		s->set_stroke_width(val);
	}

	void UndoValue<UNDO_T::TEXT_ALIGN_P>::SET(Shape* const s, const DWRITE_PARAGRAPH_ALIGNMENT& val) noexcept
	{
		s->set_text_align_vert(val);
	}

	void UndoValue<UNDO_T::TEXT_ALIGN_T>::SET(Shape* const s, const DWRITE_TEXT_ALIGNMENT& val) noexcept
	{
		s->set_text_align_horz(val);
	}

	void UndoValue<UNDO_T::TEXT_CONTENT>::SET(Shape* const s, wchar_t* const& val) noexcept
	{
		s->set_text_content(val);
	}

	void UndoValue<UNDO_T::TEXT_LINE_SP>::SET(Shape* const s, const float& val) noexcept
	{
		s->set_text_line_sp(val);
	}

	void UndoValue<UNDO_T::TEXT_PAD>::SET(Shape* const s, const D2D1_SIZE_F& val) noexcept
	{
		s->set_text_pad(val);
	}

	void UndoValue<UNDO_T::TEXT_WRAP>::SET(Shape* const s, const DWRITE_WORD_WRAPPING& val) noexcept
	{
		s->set_text_wrap(val);
	}

	void UndoValue<UNDO_T::CORE_TEXT_RANGE>::SET(Shape* const s, const CORE_TEXT_RANGE& val) noexcept
	{
		s->set_core_text_range(val);
	}

	template <UNDO_T U> bool UndoValue<U>::GET(const Shape* s, U_TYPE<U>::type& val) noexcept
	{
		return false;
	}

	bool UndoValue<UNDO_T::ARROW_CAP>::GET(const Shape* s, D2D1_CAP_STYLE& val) noexcept
	{
		return s->get_arrow_cap(val);
	}

	bool UndoValue<UNDO_T::ARROW_JOIN>::GET(const Shape* s, D2D1_LINE_JOIN& val) noexcept
	{
		return s->get_arrow_join(val);
	}

	bool UndoValue<UNDO_T::ARROW_SIZE>::GET(const Shape* s, ARROW_SIZE& val) noexcept
	{
		return s->get_arrow_size(val);
	}

	bool UndoValue<UNDO_T::ARROW_STYLE>::GET(const Shape* s, ARROW_STYLE& val) noexcept
	{
		return s->get_arrow_style(val);
	}

	//bool UndoValue<UNDO_T::DASH_CAP>::GET(const Shape* s, D2D1_CAP_STYLE& val) noexcept
	//{
	//	return s->get_dash_cap(val);
	//}

	bool UndoValue<UNDO_T::DASH_PAT>::GET(const Shape* s, DASH_PAT& val) noexcept
	{
		return s->get_stroke_dash_pat(val);
	}

	bool UndoValue<UNDO_T::DASH_STYLE>::GET(const Shape* s, D2D1_DASH_STYLE& val) noexcept
	{
		return s->get_stroke_dash(val);
	}

	bool UndoValue<UNDO_T::FILL_COLOR>::GET(const Shape* s, D2D1_COLOR_F& val) noexcept
	{
		return s->get_fill_color(val);
	}

	bool UndoValue<UNDO_T::FONT_COLOR>::GET(const Shape* s, D2D1_COLOR_F& val) noexcept
	{
		return s->get_font_color(val);
	}

	bool UndoValue<UNDO_T::FONT_FAMILY>::GET(const Shape* s, wchar_t*& val) noexcept
	{
		return s->get_font_family(val);
	}

	bool UndoValue<UNDO_T::FONT_SIZE>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_font_size(val);
	}

	bool UndoValue<UNDO_T::FONT_STRETCH>::GET(const Shape* s, DWRITE_FONT_STRETCH& val) noexcept
	{
		return s->get_font_stretch(val);
	}

	bool UndoValue<UNDO_T::FONT_STYLE>::GET(const Shape* s, DWRITE_FONT_STYLE& val) noexcept
	{
		return s->get_font_style(val);
	}

	bool UndoValue<UNDO_T::FONT_WEIGHT>::GET(const Shape* s, DWRITE_FONT_WEIGHT& val) noexcept
	{
		return s->get_font_weight(val);
	}

	bool UndoValue<UNDO_T::GRID_BASE>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_grid_base(val);
	}

	bool UndoValue<UNDO_T::GRID_COLOR>::GET(const Shape* s, D2D1_COLOR_F& val) noexcept
	{
		return s->get_grid_color(val);
	}

	bool UndoValue<UNDO_T::GRID_EMPH>::GET(const Shape* s, GRID_EMPH& val) noexcept
	{
		return s->get_grid_emph(val);
	}

	bool UndoValue<UNDO_T::GRID_SHOW>::GET(const Shape* s, GRID_SHOW& val) noexcept
	{
		return s->get_grid_show(val);
	}

	bool UndoValue<UNDO_T::IMAGE_OPAC>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_image_opacity(val);
	}

	bool UndoValue<UNDO_T::JOIN_LIMIT>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_stroke_join_limit(val);
	}

	bool UndoValue<UNDO_T::JOIN_STYLE>::GET(const Shape* s, D2D1_LINE_JOIN& val) noexcept
	{
		return s->get_stroke_join(val);
	}

	bool UndoValue<UNDO_T::MOVE>::GET(const Shape* s, D2D1_POINT_2F& val) noexcept
	{
		return s->get_pos_start(val);
	}

	bool UndoValue<UNDO_T::SHEET_COLOR>::GET(const Shape* s, D2D1_COLOR_F& val) noexcept
	{
		return s->get_sheet_color(val);
	}

	bool UndoValue<UNDO_T::SHEET_SIZE>::GET(const Shape* s, D2D1_SIZE_F& val) noexcept
	{
		return s->get_sheet_size(val);
	}

	bool UndoValue<UNDO_T::SHEET_PAD>::GET(const Shape* s, D2D1_RECT_F& val) noexcept
	{
		return s->get_sheet_padding(val);
	}

	bool UndoValue<UNDO_T::POLY_END>::GET(const Shape* s, D2D1_FIGURE_END& val) noexcept
	{
		return s->get_poly_end(val);
	}

	bool UndoValue<UNDO_T::ARC_DIR>::GET(const Shape* s, D2D1_SWEEP_DIRECTION& val) noexcept
	{
		return s->get_arc_dir(val);
	}

	bool UndoValue<UNDO_T::ARC_END>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_arc_end(val);
	}

	bool UndoValue<UNDO_T::ARC_ROT>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_arc_rot(val);
	}

	bool UndoValue<UNDO_T::ARC_START>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_arc_start(val);
	}

	bool UndoValue<UNDO_T::STROKE_CAP>::GET(const Shape* s, D2D1_CAP_STYLE& val) noexcept
	{
		return s->get_stroke_cap(val);
	}

	bool UndoValue<UNDO_T::STROKE_COLOR>::GET(const Shape* s, D2D1_COLOR_F& val) noexcept
	{
		return s->get_stroke_color(val);
	}

	bool UndoValue<UNDO_T::STROKE_WIDTH>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_stroke_width(val);
	}

	bool UndoValue<UNDO_T::TEXT_ALIGN_P>::GET(const Shape* s, DWRITE_PARAGRAPH_ALIGNMENT& val) noexcept
	{
		return s->get_text_align_vert(val);
	}

	bool UndoValue<UNDO_T::TEXT_ALIGN_T>::GET(const Shape* s, DWRITE_TEXT_ALIGNMENT& val) noexcept
	{
		return s->get_text_align_horz(val);
	}

	bool UndoValue<UNDO_T::TEXT_CONTENT>::GET(const Shape* s, wchar_t*& val) noexcept
	{
		return s->get_text_content(val);
	}

	bool UndoValue<UNDO_T::TEXT_LINE_SP>::GET(const Shape* s, float& val) noexcept
	{
		return s->get_text_line_sp(val);
	}

	bool UndoValue<UNDO_T::TEXT_PAD>::GET(const Shape* s, D2D1_SIZE_F& val) noexcept
	{
		return s->get_text_pad(val);
	}

	bool UndoValue<UNDO_T::TEXT_WRAP>::GET(const Shape* s, DWRITE_WORD_WRAPPING& val) noexcept
	{
		return s->get_text_wrap(val);
	}

	bool UndoValue<UNDO_T::CORE_TEXT_RANGE>::GET(const Shape* s, CORE_TEXT_RANGE& val) noexcept
	{
		return s->get_core_text_range(val);
	}

	// 図形の値の操作をデータライターに書き込む.
	template <UNDO_T U> void UndoValue<U>::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(U));
		undo_write_shape(m_shape, dt_writer);
		if constexpr (
			U == UNDO_T::FONT_SIZE ||
			U == UNDO_T::GRID_BASE ||
			U == UNDO_T::IMAGE_OPAC ||
			U == UNDO_T::JOIN_LIMIT ||
			U == UNDO_T::ARC_START ||
			U == UNDO_T::ARC_END ||
			U == UNDO_T::ARC_ROT ||
			U == UNDO_T::STROKE_WIDTH ||
			U == UNDO_T::TEXT_LINE_SP) {
			dt_writer.WriteSingle(m_value);
		}
		else if constexpr (
			U == UNDO_T::ARC_DIR ||
			U == UNDO_T::ARROW_CAP ||
			U == UNDO_T::ARROW_JOIN ||
			U == UNDO_T::ARROW_STYLE ||
			//U == UNDO_T::DASH_CAP ||
			U == UNDO_T::DASH_STYLE ||
			U == UNDO_T::FONT_STRETCH ||
			U == UNDO_T::FONT_STYLE ||
			U == UNDO_T::FONT_WEIGHT ||
			U == UNDO_T::GRID_SHOW ||
			U == UNDO_T::JOIN_STYLE ||
			U == UNDO_T::POLY_END ||
			U == UNDO_T::STROKE_CAP ||
			U == UNDO_T::TEXT_ALIGN_P ||
			U == UNDO_T::TEXT_ALIGN_T ||
			U == UNDO_T::TEXT_WRAP) {
			dt_writer.WriteUInt32(static_cast<uint32_t>(m_value));
		}
		else if constexpr (U == UNDO_T::ARROW_SIZE) {
			dt_writer.WriteSingle(static_cast<ARROW_SIZE>(m_value).m_width);
			dt_writer.WriteSingle(static_cast<ARROW_SIZE>(m_value).m_length);
			dt_writer.WriteSingle(static_cast<ARROW_SIZE>(m_value).m_offset);
		}
		else if constexpr (U == UNDO_T::DASH_PAT) {
			dt_writer.WriteSingle(static_cast<DASH_PAT>(m_value).m_[0]);
			dt_writer.WriteSingle(static_cast<DASH_PAT>(m_value).m_[1]);
			dt_writer.WriteSingle(static_cast<DASH_PAT>(m_value).m_[2]);
			dt_writer.WriteSingle(static_cast<DASH_PAT>(m_value).m_[3]);
			dt_writer.WriteSingle(static_cast<DASH_PAT>(m_value).m_[4]);
			dt_writer.WriteSingle(static_cast<DASH_PAT>(m_value).m_[5]);
		}
		else if constexpr (
			U == UNDO_T::FILL_COLOR ||
			U == UNDO_T::FONT_COLOR ||
			U == UNDO_T::GRID_COLOR ||
			U == UNDO_T::SHEET_COLOR ||
			U == UNDO_T::STROKE_COLOR
			) {
			dt_writer.WriteSingle(static_cast<D2D1_COLOR_F>(m_value).r);
			dt_writer.WriteSingle(static_cast<D2D1_COLOR_F>(m_value).g);
			dt_writer.WriteSingle(static_cast<D2D1_COLOR_F>(m_value).b);
			dt_writer.WriteSingle(static_cast<D2D1_COLOR_F>(m_value).a);
		}
		else if constexpr (U == UNDO_T::SHEET_PAD) {
			dt_writer.WriteSingle(static_cast<D2D1_RECT_F>(m_value).left);
			dt_writer.WriteSingle(static_cast<D2D1_RECT_F>(m_value).top);
			dt_writer.WriteSingle(static_cast<D2D1_RECT_F>(m_value).right);
			dt_writer.WriteSingle(static_cast<D2D1_RECT_F>(m_value).bottom);
		}
		else if constexpr (
			U == UNDO_T::FONT_FAMILY ||
			U == UNDO_T::TEXT_CONTENT) {
			const uint32_t len = wchar_len(m_value);
			dt_writer.WriteUInt32(len);
			const auto data = reinterpret_cast<const uint8_t*>(m_value);
			dt_writer.WriteBytes(array_view(data, data + 2 * static_cast<size_t>(len)));
		}
		else if constexpr (U == UNDO_T::GRID_EMPH) {
			dt_writer.WriteUInt32(m_value.m_gauge_1);
			dt_writer.WriteUInt32(m_value.m_gauge_2);
		}
		else if constexpr (U == UNDO_T::MOVE) {
			dt_writer.WriteSingle(m_value.x);
			dt_writer.WriteSingle(m_value.y);
		}
		else if constexpr (
			U == UNDO_T::TEXT_PAD ||
			U == UNDO_T::SHEET_SIZE) {
			dt_writer.WriteSingle(m_value.width);
			dt_writer.WriteSingle(m_value.height);
		}
		else {
			winrt::hresult_not_implemented();
		}
	}

	// 操作を実行すると値が変わるか判定する.
	bool UndoImage::changed(void) const noexcept
	{
		const ShapeImage* s = static_cast<const ShapeImage*>(m_shape);
		const auto pos = s->m_start;
		const auto view = s->m_view;
		const auto clip = s->m_clip;
		const auto opac = s->m_opac;
		return !equal(pos, m_start) || !equal(view, m_view) || !equal(clip, m_clip) || !equal(opac, m_opac);
	}

	// 元に戻す操作を実行する.
	void UndoImage::exec(void) noexcept
	{
		ShapeImage* s = static_cast<ShapeImage*>(m_shape);
		const auto pos = s->m_start;
		const auto view = s->m_view;
		const auto clip = s->m_clip;
		const auto opac = s->m_opac;
		s->m_start = m_start;
		s->m_view = m_view;
		s->m_clip = m_clip;
		s->m_opac = m_opac;
		m_start = pos;
		m_view = view;
		m_clip = clip;
		m_opac = opac;
	}

	// データリーダーから操作を読み込む.
	UndoImage::UndoImage(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader)),
		m_start(D2D1_POINT_2F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_view(D2D1_SIZE_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_clip(D2D1_RECT_F{
			dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle()
		}),
		m_opac(dt_reader.ReadSingle())
	{}

	UndoImage::UndoImage(ShapeImage* const s) :
		Undo(s),
		m_start(s->m_start),
		m_view(s->m_view),
		m_clip(s->m_clip),
		m_opac(s->m_opac)
	{}

	// 画像の操作をデータライターに書き込む.
	void UndoImage::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::IMAGE));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);
		dt_writer.WriteSingle(m_view.width);
		dt_writer.WriteSingle(m_view.height);
		dt_writer.WriteSingle(m_clip.left);
		dt_writer.WriteSingle(m_clip.top);
		dt_writer.WriteSingle(m_clip.right);
		dt_writer.WriteSingle(m_clip.bottom);
		dt_writer.WriteSingle(m_opac);
	}

	// 操作を実行する.
	void UndoList::exec(void) noexcept
	{
		if (m_insert) {
			// 図形リストから指定された図形を検索し, あれば図形を取りのぞく.
			// 図形リストから挿入する位置にある図形を探し, その位置に指定された図形を挿入する.
			// 指定された図形の削除フラグを解除する.
			// この操作が次に実行されたとき, 削除が行なわれるよう, 挿入フラグを下す.
			auto& slist = Undo::undo_sheet->m_shape_list;
			auto it_del{ std::find(slist.begin(), slist.end(), m_shape) };
			if (it_del != slist.end()) {
				slist.erase(it_del);
			}
			auto it_ins{ std::find(slist.begin(), slist.end(), m_shape_at) };
			slist.insert(it_ins, m_shape);
			m_shape->set_delete(false);
			m_insert = false;
		}
		else {
			// 図形リストから指定された図形を検索し, 図形を取りのぞき, リストの先頭に挿入する.
			// 指定された図形の削除フラグを立てる.
			// この操作が次に実行されたとき, 挿入が行なわれるよう, 挿入フラグを立てる.
			auto& slist = Undo::undo_sheet->m_shape_list;
			auto it_del{ std::find(slist.begin(), slist.end(), m_shape) };
			auto it_pos{ slist.erase(it_del) };
			m_shape_at = (it_pos == slist.end() ? nullptr : *it_pos);
			slist.push_front(m_shape);
			m_shape->set_delete(true);
			m_insert = true;
		}
	}

	// データリーダーから操作を読み込む.
	UndoList::UndoList(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		m_insert = dt_reader.ReadBoolean();
		m_shape_at = undo_read_shape(dt_reader);
	}

	// 追加と削除の操作をデータライターに書き込む.
	void UndoList::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::LIST));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_shape_at, dt_writer);
	}

	// 操作を実行する.
	void UndoGroup::exec(void) noexcept
	{
		if (m_insert) {
			auto& slist = Undo::undo_sheet->m_shape_list;
			auto it_del{ std::find(slist.begin(), slist.end(), m_shape) };
			if (it_del != slist.end()) {
				slist.erase(it_del);
			}
			SHAPE_LIST& list_grouped = m_shape_group->m_list_grouped;
			auto it_ins{ std::find(list_grouped.begin(), list_grouped.end(), m_shape_at) };
			list_grouped.insert(it_ins, m_shape);
			m_shape->set_delete(false);
		}
		else {
			auto& slist = Undo::undo_sheet->m_shape_list;
			m_shape->set_delete(true);
			SHAPE_LIST& list_grouped = m_shape_group->m_list_grouped;
			auto it_del{ std::find(list_grouped.begin(), list_grouped.end(), m_shape) };
			auto it_pos{ list_grouped.erase(it_del) };
			m_shape_at = (it_pos == list_grouped.end() ? nullptr : *it_pos);
			slist.push_front(m_shape);
		}
		m_insert = !m_insert;
	}

	// データリーダーから操作を読み込む.
	UndoGroup::UndoGroup(DataReader const& dt_reader) :
		UndoList(dt_reader),
		m_shape_group(static_cast<ShapeGroup*>(undo_read_shape(dt_reader)))
	{}

	// グループ操作をデータライターに書き込む.
	void UndoGroup::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::GROUP));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_shape_at, dt_writer);
		undo_write_shape(m_shape_group, dt_writer);
	}

	// データリーダーから操作を読み込む.
	UndoSelect::UndoSelect(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{}

	// 図形の選択操作をデータライターに書き込む.
	void UndoSelect::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::SELECT));
		undo_write_shape(m_shape, dt_writer);
	}

	/*
	UndoTextSelect::UndoTextSelect(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		m_start = dt_reader.ReadInt32();
		m_end = dt_reader.ReadInt32();
		m_is_trail = dt_reader.ReadBoolean();
	}

	void UndoTextSelect::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::TEXT_SELECT));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteInt32(static_cast<int32_t>(m_start));
		dt_writer.WriteInt32(static_cast<int32_t>(m_end));
		dt_writer.WriteBoolean(m_is_trail);
	}
	*/
	UndoText2::UndoText2(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		m_start = dt_reader.ReadUInt32();
		m_end = dt_reader.ReadUInt32();
		m_trail = dt_reader.ReadBoolean();
		const auto len = dt_reader.ReadUInt32();
		if (len > 0) {
			const auto a = reinterpret_cast<uint8_t*>(m_text);
			dt_reader.ReadBytes(array_view(a, a + 2 * static_cast<size_t>(len)));
		}
		else {
			m_text = nullptr;
		}
	}

	void UndoText2::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_T::TEXT_CONTENT));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteUInt32(m_start);
		dt_writer.WriteUInt32(m_end);
		dt_writer.WriteBoolean(m_trail);
		const auto len = wchar_len(m_text);
		if (len > 0) {
			const auto a = reinterpret_cast<const uint8_t*>(m_text);
			dt_writer.WriteBytes(array_view(a, a + 2 * static_cast<size_t>(len)));
		}
		else {
			dt_writer.WriteUInt32(0);
		}
	}

}