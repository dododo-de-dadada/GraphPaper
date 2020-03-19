#include "pch.h"
#include "undo.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr auto INDEX_PAGE = static_cast<uint32_t>(-1);	// ページ図形の添え字
	constexpr auto INDEX_NULL = static_cast<uint32_t>(-2);	// ヌル図形の添え字

	S_LIST_T* Undo::s_shape_list = nullptr;
	ShapeLayout* Undo::s_shape_layout = nullptr;

	// 添え字をデータリーダーから読み込んで図形を得る.
	static Shape* undo_read_shape(DataReader const& dt_reader)
	{
		Shape* s = nullptr;
		auto i = dt_reader.ReadUInt32();
		if (i == INDEX_PAGE) {
			s = Undo::s_shape_layout;
		}
		else if (i == INDEX_NULL) {
			s = nullptr;
		}
		else {
			s_list_match<uint32_t, Shape*>(*Undo::s_shape_list, i, s);
		}
		return s;
	}

	// 図形をデータライターに書き込む.
	static void undo_write_shape(Shape* s, DataWriter const& dt_writer)
	{
		if (s == Undo::s_shape_layout) {
			dt_writer.WriteUInt32(INDEX_PAGE);
		}
		else if (s == nullptr) {
			dt_writer.WriteUInt32(INDEX_NULL);
		}
		else {
			uint32_t i = 0;
			s_list_match<Shape*, uint32_t>(*Undo::s_shape_list, s, i);
			dt_writer.WriteUInt32(i);
		}
	}

	// 操作が参照するための図形リストとページ図形を格納する.
	void Undo::set(S_LIST_T* s_list, ShapeLayout* s_layout) noexcept
	{
		s_shape_list = s_list;
		s_shape_layout = s_layout;
	}

	// 操作を実行する.
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

	// 操作をデータリーダーから読み込む.
	UndoArrange2::UndoArrange2(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		m_dst_shape = undo_read_shape(dt_reader);
	}

	// 操作を作成する.
	UndoArrange2::UndoArrange2(Shape* s, Shape* t) :
		Undo(s)
	{
		m_dst_shape = t;
		exec();
	}

	// 操作をデータライターに書き込む.
	void UndoArrange2::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::ARRANGE));
		undo_write_shape(m_shape, dt_writer);
		undo_write_shape(m_dst_shape, dt_writer);
	}

	// 操作を実行する.
	void UndoList::exec(void)
	{
		if (m_insert) {
			auto it_del{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape) };
			if (it_del != s_shape_list->end()) {
				s_shape_list->erase(it_del);
			}
			auto it_ins{ std::find(s_shape_list->begin(), s_shape_list->end(), m_item_pos) };
			s_shape_list->insert(it_ins, m_shape);
			m_shape->set_delete(false);
		}
		else {
			m_shape->set_delete(true);
			auto it_del{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape) };
			auto it_pos{ s_shape_list->erase(it_del) };
			m_item_pos = (it_pos == s_shape_list->end() ? nullptr : *it_pos);
			s_shape_list->push_front(m_shape);
		}
		m_insert = !m_insert;
	}

	// 操作をデータリーダーから読み込む.
	UndoList::UndoList(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		m_insert = dt_reader.ReadBoolean();
		m_item_pos = undo_read_shape(dt_reader);
	}

	// 図形をリストから取り除く.
	// s	取り除く図形
	// dont	初期化のみで操作を実行しない.
	UndoList::UndoList(Shape* s, const bool dont_exec) :
		Undo(s),
		m_insert(false),
		m_item_pos(static_cast<Shape*>(nullptr))
	{
		if (dont_exec == false) {
			exec();
		}
	}

	// 図形をリストに挿入する
	// s	挿入する図形
	// s_pos	挿入する位置
	// dont	初期化のみで操作を実行しない.
	UndoList::UndoList(Shape* s, Shape* s_pos, const bool dont_exec) :
		Undo(s),
		m_insert(true),
		m_item_pos(s_pos)
	{
		if (dont_exec == false) {
			exec();
		}
	}

	// 操作をデータライターに書き込む.
	void UndoList::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::LIST));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_item_pos, dt_writer);
	}

	// 操作を実行する.
	void UndoListG::exec(void)
	{
		if (m_insert) {
			auto it_del{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape) };
			if (it_del != s_shape_list->end()) {
				s_shape_list->erase(it_del);
			}
			S_LIST_T& list_grouped = m_shape_group->m_list_grouped;
			auto it_ins{ std::find(list_grouped.begin(), list_grouped.end(), m_item_pos) };
			list_grouped.insert(it_ins, m_shape);
			m_shape->set_delete(false);
		}
		else {
			m_shape->set_delete(true);
			S_LIST_T& list_grouped = m_shape_group->m_list_grouped;
			auto it_del{ std::find(list_grouped.begin(), list_grouped.end(), m_shape) };
			auto it_pos{ list_grouped.erase(it_del) };
			m_item_pos = (it_pos == list_grouped.end() ? nullptr : *it_pos);
			s_shape_list->push_front(m_shape);
		}
		m_insert = !m_insert;
	}

	// 操作をデータリーダーから読み込む.
	UndoListG::UndoListG(DataReader const& dt_reader) :
		UndoList(dt_reader),
		m_shape_group(static_cast<ShapeGroup*>(undo_read_shape(dt_reader)))
	{}

	// 図形をグループから取り除く.
	// s	取り除く図形
	UndoListG::UndoListG(ShapeGroup* g, Shape* s) :
		UndoList(s, true),
		m_shape_group(g)
	{
		exec();
	}

	// 図形をグループに追加する.
	// s	取り除く図形
	UndoListG::UndoListG(ShapeGroup* g, Shape* s, Shape* s_pos) :
		UndoList(s, s_pos, true),
		m_shape_group(g)
	{
		exec();
	}

	// 操作をデータライターに書き込む.
	void UndoListG::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::GROUP));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_item_pos, dt_writer);
		undo_write_shape(m_shape_group, dt_writer);
	}

	// 操作を実行する.
	void UndoSelect::exec(void)
	{
		m_shape->set_select(m_shape->is_selected() == false);
	}

	// 操作をデータリーダーから読み込む.
	UndoSelect::UndoSelect(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{}

	// 図形の選択を反転する.
	UndoSelect::UndoSelect(Shape* s) :
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

	// 操作を実行すると値が変わるか調べる.
	bool UndoForm::changed(void) const noexcept
	{
		using winrt::GraphPaper::implementation::equal;
		D2D1_POINT_2F a_pos;

		m_shape->get_pos(m_anchor, a_pos);
		return !equal(a_pos, m_anchor_pos);
	}

	// 元に戻す操作を実行する.
	void UndoForm::exec(void)
	{
		D2D1_POINT_2F a_pos;

		m_shape->get_pos(m_anchor, a_pos);
		m_shape->set_pos(m_anchor_pos, m_anchor);
		m_anchor_pos = a_pos;
	}

	// 操作をデータリーダーから読み込む.
	UndoForm::UndoForm(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		using winrt::GraphPaper::implementation::read;

		m_anchor = static_cast<ANCH_WHICH>(dt_reader.ReadUInt32());
		read(m_anchor_pos, dt_reader);
	}

	// 図形の, 指定された部位の位置を保存する.
	UndoForm::UndoForm(Shape* s, const ANCH_WHICH a) :
		Undo(s)
	{
		m_shape = s;
		m_anchor = a;
		s->get_pos(a, m_anchor_pos);
	}

	// データライターに書き込む.
	void UndoForm::write(DataWriter const& dt_writer)
	{
		using winrt::GraphPaper::implementation::write;

		dt_writer.WriteUInt32(static_cast<uint32_t>(UNDO_OP::ANCH_POS));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_anchor));
		write(m_anchor_pos, dt_writer);
	}

	// 操作を実行すると値が変わるか調べる.
	template <UNDO_OP U>
	bool UndoSet<U>::changed(void) const noexcept
	{
		using winrt::GraphPaper::implementation::equal;
		U_TYPE<U>::type value{};
		return GET(m_shape, value) && equal(value, m_value) == false;
	}
	template bool UndoSet<UNDO_OP::ARROW_SIZE>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::ARROW_STYLE>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::FILL_COLOR>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::FONT_COLOR>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::FONT_FAMILY>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::FONT_SIZE>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::FONT_STRETCH>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::FONT_STYLE>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::FONT_WEIGHT>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::GRID_BASE>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::GRID_GRAY>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::GRID_PATT>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::GRID_SHOW>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::TEXT_LINE>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::TEXT_MARGIN>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::PAGE_COLOR>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::PAGE_SIZE>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::TEXT_ALIGN_P>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::START_POS>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::STROKE_COLOR>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::STROKE_PATT>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::STROKE_STYLE>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::STROKE_WIDTH>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::TEXT_CONTENT>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::TEXT_ALIGN_T>::changed(void) const noexcept;
	template bool UndoSet<UNDO_OP::TEXT_RANGE>::changed(void) const noexcept;

	// 操作を実行する.
	template <UNDO_OP U>
	void UndoSet<U>::exec(void)
	{
		U_TYPE<U>::type old_value{};
		if (GET(m_shape, old_value)) {
			SET(m_shape, m_value);
			m_value = old_value;
		}
	}
	template void UndoSet<UNDO_OP::ARROW_SIZE>::exec(void);
	template void UndoSet<UNDO_OP::ARROW_STYLE>::exec(void);
	template void UndoSet<UNDO_OP::FILL_COLOR>::exec(void);
	template void UndoSet<UNDO_OP::FONT_COLOR>::exec(void);
	template void UndoSet<UNDO_OP::FONT_FAMILY>::exec(void);
	template void UndoSet<UNDO_OP::FONT_SIZE>::exec(void);
	template void UndoSet<UNDO_OP::FONT_STRETCH>::exec(void);
	template void UndoSet<UNDO_OP::FONT_STYLE>::exec(void);
	template void UndoSet<UNDO_OP::FONT_WEIGHT>::exec(void);
	template void UndoSet<UNDO_OP::GRID_BASE>::exec(void);
	template void UndoSet<UNDO_OP::GRID_GRAY>::exec(void);
	template void UndoSet<UNDO_OP::GRID_PATT>::exec(void);
	template void UndoSet<UNDO_OP::GRID_SHOW>::exec(void);
	//template void UndoSet<UNDO_OP::GRID_SNAP>::exec(void);
	template void UndoSet<UNDO_OP::TEXT_LINE>::exec(void);
	template void UndoSet<UNDO_OP::TEXT_MARGIN>::exec(void);
	template void UndoSet<UNDO_OP::PAGE_COLOR>::exec(void);
	template void UndoSet<UNDO_OP::PAGE_SIZE>::exec(void);
	template void UndoSet<UNDO_OP::TEXT_ALIGN_P>::exec(void);
	template void UndoSet<UNDO_OP::START_POS>::exec(void);
	template void UndoSet<UNDO_OP::STROKE_COLOR>::exec(void);
	template void UndoSet<UNDO_OP::STROKE_PATT>::exec(void);
	template void UndoSet<UNDO_OP::STROKE_STYLE>::exec(void);
	template void UndoSet<UNDO_OP::STROKE_WIDTH>::exec(void);
	template void UndoSet<UNDO_OP::TEXT_CONTENT>::exec(void);
	template void UndoSet<UNDO_OP::TEXT_ALIGN_T>::exec(void);
	template void UndoSet<UNDO_OP::TEXT_RANGE>::exec(void);

	// 図形の属性値を保存する.
	template <UNDO_OP U>
	UndoSet<U>::UndoSet(Shape* s) :
		Undo(s)
	{
		GET(m_shape, m_value);
	}
	template UndoSet<UNDO_OP::ARROW_SIZE>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::ARROW_STYLE>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::FILL_COLOR>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::FONT_COLOR>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::FONT_FAMILY>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::FONT_SIZE>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::FONT_STRETCH>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::FONT_STYLE>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::FONT_WEIGHT>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::GRID_BASE>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::GRID_GRAY>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::GRID_PATT>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::GRID_SHOW>::UndoSet(Shape* s);
	//template UndoSet<UNDO_OP::GRID_SNAP>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::TEXT_LINE>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::TEXT_MARGIN>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::PAGE_COLOR>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::PAGE_SIZE>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::TEXT_ALIGN_P>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::START_POS>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::STROKE_COLOR>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::STROKE_PATT>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::STROKE_STYLE>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::STROKE_WIDTH>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::TEXT_CONTENT>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::TEXT_ALIGN_T>::UndoSet(Shape* s);
	template UndoSet<UNDO_OP::TEXT_RANGE>::UndoSet(Shape* s);

	// 図形の属性値を保存したあと値を格納する.
	template <UNDO_OP U>
	UndoSet<U>::UndoSet(Shape* s, U_TYPE<U>::type const& value) :
		UndoSet(s)
	{
		UndoSet<U>::SET(m_shape, value);
	}
	template UndoSet<UNDO_OP::ARROW_SIZE>::UndoSet(Shape* s, const ARROW_SIZE& value);
	template UndoSet<UNDO_OP::ARROW_STYLE>::UndoSet(Shape* s, const ARROW_STYLE& value);
	template UndoSet<UNDO_OP::FILL_COLOR>::UndoSet(Shape* s, const D2D1_COLOR_F& value);
	template UndoSet<UNDO_OP::FONT_COLOR>::UndoSet(Shape* s, const D2D1_COLOR_F& value);
	template UndoSet<UNDO_OP::FONT_FAMILY>::UndoSet(Shape* s, wchar_t* const& value);
	template UndoSet<UNDO_OP::FONT_SIZE>::UndoSet(Shape* s, const double& value);
	template UndoSet<UNDO_OP::FONT_STRETCH>::UndoSet(Shape* s, const DWRITE_FONT_STRETCH& value);
	template UndoSet<UNDO_OP::FONT_STYLE>::UndoSet(Shape* s, const DWRITE_FONT_STYLE& value);
	template UndoSet<UNDO_OP::FONT_WEIGHT>::UndoSet(Shape* s, const DWRITE_FONT_WEIGHT& value);
	template UndoSet<UNDO_OP::GRID_BASE>::UndoSet(Shape* s, const double& value);
	template UndoSet<UNDO_OP::GRID_GRAY>::UndoSet(Shape* s, const double& value);
	template UndoSet<UNDO_OP::GRID_PATT>::UndoSet(Shape* s, const GRID_PATT& value);
	template UndoSet<UNDO_OP::GRID_SHOW>::UndoSet(Shape* s, const GRID_SHOW& value);
	template UndoSet<UNDO_OP::TEXT_LINE>::UndoSet(Shape* s, const double& value);
	template UndoSet<UNDO_OP::TEXT_MARGIN>::UndoSet(Shape* s, const D2D1_SIZE_F& value);
	template UndoSet<UNDO_OP::PAGE_COLOR>::UndoSet(Shape* s, const D2D1_COLOR_F& value);
	template UndoSet<UNDO_OP::PAGE_SIZE>::UndoSet(Shape* s, const D2D1_SIZE_F& value);
	template UndoSet<UNDO_OP::TEXT_ALIGN_P>::UndoSet(Shape* s, const DWRITE_PARAGRAPH_ALIGNMENT& value);
	template UndoSet<UNDO_OP::START_POS>::UndoSet(Shape* s, const D2D1_POINT_2F& value);
	template UndoSet<UNDO_OP::STROKE_COLOR>::UndoSet(Shape* s, const D2D1_COLOR_F& value);
	template UndoSet<UNDO_OP::STROKE_PATT>::UndoSet(Shape* s, const STROKE_PATT& value);
	template UndoSet<UNDO_OP::STROKE_STYLE>::UndoSet(Shape* s, const D2D1_DASH_STYLE& value);
	template UndoSet<UNDO_OP::STROKE_WIDTH>::UndoSet(Shape* s, const double& value);
	template UndoSet<UNDO_OP::TEXT_CONTENT>::UndoSet(Shape* s, wchar_t* const& value);
	template UndoSet<UNDO_OP::TEXT_ALIGN_T>::UndoSet(Shape* s, const DWRITE_TEXT_ALIGNMENT& value);
	template UndoSet<UNDO_OP::TEXT_RANGE>::UndoSet(Shape* s, const DWRITE_TEXT_RANGE& value);

	template <UNDO_OP U>
	UndoSet<U>::UndoSet(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		using winrt::GraphPaper::implementation::read;
		read(m_value, dt_reader);
	}
	template UndoSet<UNDO_OP::ARROW_SIZE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::ARROW_STYLE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::FILL_COLOR>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::FONT_COLOR>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::FONT_FAMILY>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::FONT_SIZE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::FONT_STRETCH>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::FONT_STYLE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::FONT_WEIGHT>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::GRID_BASE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::GRID_GRAY>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::GRID_PATT>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::GRID_SHOW>::UndoSet(DataReader const& dt_reader);
	//template UndoSet<UNDO_OP::GRID_SNAP>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::TEXT_LINE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::TEXT_MARGIN>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::PAGE_COLOR>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::PAGE_SIZE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::TEXT_ALIGN_P>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::START_POS>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::STROKE_COLOR>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::STROKE_PATT>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::STROKE_STYLE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::STROKE_WIDTH>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::TEXT_CONTENT>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::TEXT_ALIGN_T>::UndoSet(DataReader const& dt_reader);
	template UndoSet<UNDO_OP::TEXT_RANGE>::UndoSet(DataReader const& dt_reader);

	// 図形の属性値に値を格納する.
	template <UNDO_OP U>
	void UndoSet<U>::SET(Shape* s, const U_TYPE<U>::type& value)
	{
		throw winrt::hresult_not_implemented();
	}
	void UndoSet<UNDO_OP::ARROW_SIZE>::SET(Shape* s, const ARROW_SIZE& value)
	{
		s->set_arrow_size(value);
	}
	void UndoSet<UNDO_OP::ARROW_STYLE>::SET(Shape* s, const ARROW_STYLE& value)
	{
		s->set_arrow_style(value);
	}
	void UndoSet<UNDO_OP::FILL_COLOR>::SET(Shape* s, const D2D1_COLOR_F& value)
	{
		s->set_fill_color(value);
	}
	void UndoSet<UNDO_OP::FONT_COLOR>::SET(Shape* s, const D2D1_COLOR_F& value)
	{
		s->set_font_color(value);
	}
	void UndoSet<UNDO_OP::FONT_FAMILY>::SET(Shape* s, wchar_t* const& value)
	{
		s->set_font_family(value);
	}
	void UndoSet<UNDO_OP::FONT_SIZE>::SET(Shape* s, const double& value)
	{
		s->set_font_size(value);
	}
	void UndoSet<UNDO_OP::FONT_STRETCH>::SET(Shape* s, const DWRITE_FONT_STRETCH& value)
	{
		s->set_font_stretch(value);
	}
	void UndoSet<UNDO_OP::FONT_STYLE>::SET(Shape* s, const DWRITE_FONT_STYLE& value)
	{
		s->set_font_style(value);
	}
	void UndoSet<UNDO_OP::FONT_WEIGHT>::SET(Shape* s, const DWRITE_FONT_WEIGHT& value)
	{
		s->set_font_weight(value);
	}
	void UndoSet<UNDO_OP::GRID_BASE>::SET(Shape* s, const double& value)
	{
		s->set_grid_base(value);
	}
	void UndoSet<UNDO_OP::GRID_GRAY>::SET(Shape* s, const double& value)
	{
		s->set_grid_gray(value);
	}
	void UndoSet<UNDO_OP::GRID_PATT>::SET(Shape* s, const GRID_PATT& value)
	{
		s->set_grid_patt(value);
	}
	void UndoSet<UNDO_OP::GRID_SHOW>::SET(Shape* s, const GRID_SHOW& value)
	{
		s->set_grid_show(value);
	}
	void UndoSet<UNDO_OP::TEXT_LINE>::SET(Shape* s, const double& value)
	{
		s->set_text_line_height(value);
	}
	void UndoSet<UNDO_OP::TEXT_MARGIN>::SET(Shape* s, const D2D1_SIZE_F& value)
	{
		s->set_text_margin(value);
	}
	void UndoSet<UNDO_OP::PAGE_COLOR>::SET(Shape* s, const D2D1_COLOR_F& value)
	{
		s->set_page_color(value);
	}
	void UndoSet<UNDO_OP::PAGE_SIZE>::SET(Shape* s, const D2D1_SIZE_F& value)
	{
		s->set_page_size(value);
	}
	void UndoSet<UNDO_OP::TEXT_ALIGN_P>::SET(Shape* s, const DWRITE_PARAGRAPH_ALIGNMENT& value)
	{
		s->set_text_align_p(value);
	}
	void UndoSet<UNDO_OP::START_POS>::SET(Shape* s, const D2D1_POINT_2F& value)
	{
		s->set_start_pos(value);
	}
	void UndoSet<UNDO_OP::STROKE_COLOR>::SET(Shape* s, const D2D1_COLOR_F& value)
	{
		s->set_stroke_color(value);
	}
	void UndoSet<UNDO_OP::STROKE_PATT>::SET(Shape* s, const STROKE_PATT& value)
	{
		s->set_stroke_patt(value);
	}
	void UndoSet<UNDO_OP::STROKE_STYLE>::SET(Shape* s, const D2D1_DASH_STYLE& value)
	{
		s->set_stroke_style(value);
	}
	void UndoSet<UNDO_OP::STROKE_WIDTH>::SET(Shape* s, const double& value)
	{
		s->set_stroke_width(value);
	}
	void UndoSet<UNDO_OP::TEXT_CONTENT>::SET(Shape* s, wchar_t* const& value)
	{
		s->set_text(value);
	}
	void UndoSet<UNDO_OP::TEXT_ALIGN_T>::SET(Shape* s, const DWRITE_TEXT_ALIGNMENT& value)
	{
		s->set_text_align_t(value);
	}
	void UndoSet<UNDO_OP::TEXT_RANGE>::SET(Shape* s, const DWRITE_TEXT_RANGE& value)
	{
		s->set_text_range(value);
	}

	template <UNDO_OP U>
	bool UndoSet<U>::GET(Shape* s, U_TYPE<U>::type& value) noexcept
	{
		return false;
	}
	bool UndoSet<UNDO_OP::ARROW_SIZE>::GET(Shape* s, ARROW_SIZE& value) noexcept
	{
		return s->get_arrow_size(value);
	}
	bool UndoSet<UNDO_OP::ARROW_STYLE>::GET(Shape* s, ARROW_STYLE& value) noexcept
	{
		return s->get_arrow_style(value);
	}
	bool UndoSet<UNDO_OP::FILL_COLOR>::GET(Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_fill_color(value);
	}
	bool UndoSet<UNDO_OP::FONT_COLOR>::GET(Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_font_color(value);
	}
	bool UndoSet<UNDO_OP::FONT_FAMILY>::GET(Shape* s, wchar_t*& value) noexcept
	{
		return s->get_font_family(value);
	}
	bool UndoSet<UNDO_OP::FONT_SIZE>::GET(Shape* s, double& value) noexcept
	{
		return s->get_font_size(value);
	}
	bool UndoSet<UNDO_OP::FONT_STRETCH>::GET(Shape* s, DWRITE_FONT_STRETCH& value) noexcept
	{
		return s->get_font_stretch(value);
	}
	bool UndoSet<UNDO_OP::FONT_STYLE>::GET(Shape* s, DWRITE_FONT_STYLE& value) noexcept
	{
		return s->get_font_style(value);
	}
	bool UndoSet<UNDO_OP::FONT_WEIGHT>::GET(Shape* s, DWRITE_FONT_WEIGHT& value) noexcept
	{
		return s->get_font_weight(value);
	}
	bool UndoSet<UNDO_OP::GRID_BASE>::GET(Shape* s, double& value) noexcept
	{
		return s->get_grid_base(value);
	}
	bool UndoSet<UNDO_OP::GRID_GRAY>::GET(Shape* s, double& value) noexcept
	{
		return s->get_grid_gray(value);
	}
	bool UndoSet<UNDO_OP::GRID_PATT>::GET(Shape* s, GRID_PATT& value) noexcept
	{
		return s->get_grid_patt(value);
	}
	bool UndoSet<UNDO_OP::GRID_SHOW>::GET(Shape* s, GRID_SHOW& value) noexcept
	{
		return s->get_grid_show(value);
	}
	bool UndoSet<UNDO_OP::TEXT_LINE>::GET(Shape* s, double& value) noexcept
	{
		return s->get_text_line_height(value);
	}
	bool UndoSet<UNDO_OP::TEXT_MARGIN>::GET(Shape* s, D2D1_SIZE_F& value) noexcept
	{
		return s->get_text_margin(value);
	}
	bool UndoSet<UNDO_OP::PAGE_COLOR>::GET(Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_page_color(value);
	}
	bool UndoSet<UNDO_OP::PAGE_SIZE>::GET(Shape* s, D2D1_SIZE_F& value) noexcept
	{
		return s->get_page_size(value);
	}
	bool UndoSet<UNDO_OP::TEXT_ALIGN_P>::GET(Shape* s, DWRITE_PARAGRAPH_ALIGNMENT& value) noexcept
	{
		return s->get_text_align_p(value);
	}
	bool UndoSet<UNDO_OP::START_POS>::GET(Shape* s, D2D1_POINT_2F& value) noexcept
	{
		return s->get_start_pos(value);
	}
	bool UndoSet<UNDO_OP::STROKE_COLOR>::GET(Shape* s, D2D1_COLOR_F& value) noexcept
	{
		return s->get_stroke_color(value);
	}
	bool UndoSet<UNDO_OP::STROKE_PATT>::GET(Shape* s, STROKE_PATT& value) noexcept
	{
		return s->get_stroke_patt(value);
	}
	bool UndoSet<UNDO_OP::STROKE_STYLE>::GET(Shape* s, D2D1_DASH_STYLE& value) noexcept
	{
		return s->get_stroke_style(value);
	}
	bool UndoSet<UNDO_OP::STROKE_WIDTH>::GET(Shape* s, double& value) noexcept
	{
		return s->get_stroke_width(value);
	}
	bool UndoSet<UNDO_OP::TEXT_CONTENT>::GET(Shape* s, wchar_t*& value) noexcept
	{
		return s->get_text(value);
	}
	bool UndoSet<UNDO_OP::TEXT_ALIGN_T>::GET(Shape* s, DWRITE_TEXT_ALIGNMENT& value) noexcept
	{
		return s->get_text_align_t(value);
	}
	bool UndoSet<UNDO_OP::TEXT_RANGE>::GET(Shape* s, DWRITE_TEXT_RANGE& value) noexcept
	{
		return s->get_text_range(value);
	}

	// データライターに書き込む.
	template <UNDO_OP U>
	void UndoSet<U>::write(DataWriter const& dt_writer)
	{
		using winrt::GraphPaper::implementation::write;
		dt_writer.WriteUInt32(static_cast<uint32_t>(U));
		undo_write_shape(m_shape, dt_writer);
		write(m_value, dt_writer);
	}
	template void UndoSet<UNDO_OP::ARROW_SIZE>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::ARROW_STYLE>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::FILL_COLOR>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::FONT_COLOR>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::FONT_FAMILY>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::FONT_SIZE>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::FONT_STRETCH>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::FONT_STYLE>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::FONT_WEIGHT>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::GRID_BASE>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::GRID_GRAY>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::GRID_PATT>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::GRID_SHOW>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::TEXT_LINE>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::TEXT_MARGIN>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::PAGE_COLOR>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::PAGE_SIZE>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::TEXT_ALIGN_P>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::START_POS>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::STROKE_COLOR>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::STROKE_PATT>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::STROKE_STYLE>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::STROKE_WIDTH>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::TEXT_CONTENT>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::TEXT_ALIGN_T>::write(DataWriter const& dt_writer);
	template void UndoSet<UNDO_OP::TEXT_RANGE>::write(DataWriter const& dt_writer);

}