#include "pch.h"
#include "undo.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	constexpr auto INDEX_PAGE = static_cast<uint32_t>(-1);	// ページ図形の添え字
	constexpr auto INDEX_NULL = static_cast<uint32_t>(-2);	// ヌル図形の添え字

	S_LIST_T* Undo::s_shape_list = nullptr;
	ShapePanel* Undo::s_shape_page = nullptr;

	// 添え字をデータリーダーから読み込んで図形を得る.
	static Shape* undo_read_shape(DataReader const& dt_reader)
	{
		Shape* s = nullptr;
		auto i = dt_reader.ReadUInt32();
		if (i == INDEX_PAGE) {
			s = Undo::s_shape_page;
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
		if (s == Undo::s_shape_page) {
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
	void Undo::set(S_LIST_T* s_list, ShapePanel* s_page) noexcept
	{
		s_shape_list = s_list;
		s_shape_page = s_page;
	}

	// 操作を実行する.
	void UndoArrange2::exec(void)
	{
		if (m_shape == m_s_dst) {
			return;
		}
		auto it_begin{ s_shape_list->begin() };
		auto it_end{ s_shape_list->end() };
		auto it_src{ std::find(it_begin, it_end, m_shape) };
		if (it_src == it_end) {
			return;
		}
		auto it_dst{ std::find(it_begin, it_end, m_s_dst) };
		if (it_dst == it_end) {
			return;
		}
		*it_src = m_s_dst;
		*it_dst = m_shape;
	}

	// 操作をデータリーダーから読み込む.
	UndoArrange2::UndoArrange2(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		m_s_dst = undo_read_shape(dt_reader);
	}

	// 操作を作成する.
	UndoArrange2::UndoArrange2(Shape* s, Shape* t) :
		Undo(s)
	{
		m_s_dst = t;
		exec();
	}

	// 操作をデータライターに書き込む.
	void UndoArrange2::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(U_OP::ARRANGE));
		undo_write_shape(m_shape, dt_writer);
		undo_write_shape(m_s_dst, dt_writer);
	}

	// 操作を実行する.
	void UndoList::exec(void)
	{
		if (m_insert) {
			auto it_del{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape) };
			if (it_del != s_shape_list->end()) {
				s_shape_list->erase(it_del);
			}
			auto it_ins{ std::find(s_shape_list->begin(), s_shape_list->end(), m_s_pos) };
			s_shape_list->insert(it_ins, m_shape);
			m_shape->set_delete(false);
		}
		else {
			m_shape->set_delete(true);
			auto it_del{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape) };
			auto it_pos{ s_shape_list->erase(it_del) };
			m_s_pos = (it_pos == s_shape_list->end() ? nullptr : *it_pos);
			s_shape_list->push_front(m_shape);
		}
		m_insert = !m_insert;
	}

	// 操作をデータリーダーから読み込む.
	UndoList::UndoList(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		m_insert = dt_reader.ReadBoolean();
		m_s_pos = undo_read_shape(dt_reader);
	}

	// 図形をリストから取り除く.
	// s	取り除く図形
	// dont	初期化のみで操作を実行しない.
	UndoList::UndoList(Shape* s, const bool dont) :
		Undo(s),
		m_insert(false),
		m_s_pos(static_cast<Shape*>(nullptr))
	{
		if (dont == false) {
			exec();
		}
	}

	// 図形をリストに挿入する
	// s	挿入する図形
	// s_pos	挿入する位置
	// dont	初期化のみで操作を実行しない.
	UndoList::UndoList(Shape* s, Shape* s_pos, const bool dont) :
		Undo(s),
		m_insert(true),
		m_s_pos(s_pos)
	{
		if (dont == false) {
			exec();
		}
	}

	// 操作をデータライターに書き込む.
	void UndoList::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(U_OP::LIST));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_s_pos, dt_writer);
	}

	// 操作を実行する.
	void UndoListG::exec(void)
	{
		if (m_insert) {
			auto it_del{ std::find(s_shape_list->begin(), s_shape_list->end(), m_shape) };
			if (it_del != s_shape_list->end()) {
				s_shape_list->erase(it_del);
			}
			S_LIST_T& grp_list = m_g_pos->m_grp_list;
			auto it_ins{ std::find(grp_list.begin(), grp_list.end(), m_s_pos) };
			grp_list.insert(it_ins, m_shape);
			m_shape->set_delete(false);
		}
		else {
			m_shape->set_delete(true);
			S_LIST_T& grp_list = m_g_pos->m_grp_list;
			auto it_del{ std::find(grp_list.begin(), grp_list.end(), m_shape) };
			auto it_pos{ grp_list.erase(it_del) };
			m_s_pos = (it_pos == grp_list.end() ? nullptr : *it_pos);
			s_shape_list->push_front(m_shape);
		}
		m_insert = !m_insert;
	}

	// 操作をデータリーダーから読み込む.
	UndoListG::UndoListG(DataReader const& dt_reader) :
		UndoList(dt_reader),
		m_g_pos(static_cast<ShapeGroup*>(undo_read_shape(dt_reader)))
	{}

	// 図形をグループから取り除く.
	// s	取り除く図形
	UndoListG::UndoListG(ShapeGroup* g, Shape* s) :
		UndoList(s, true),
		m_g_pos(g)
	{
		exec();
	}

	// 図形をグループに追加する.
	// s	取り除く図形
	UndoListG::UndoListG(ShapeGroup* g, Shape* s, Shape* s_pos) :
		UndoList(s, s_pos, true),
		m_g_pos(g)
	{
		exec();
	}

	// 操作をデータライターに書き込む.
	void UndoListG::write(DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(U_OP::GROUP));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteBoolean(m_insert);
		undo_write_shape(m_s_pos, dt_writer);
		undo_write_shape(m_g_pos, dt_writer);
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
		dt_writer.WriteUInt32(static_cast<uint32_t>(U_OP::SELECT));
		undo_write_shape(m_shape, dt_writer);
	}

	// 操作を実行すると値が変わるか調べる.
	bool UndoForm::changed(void) const noexcept
	{
		using winrt::GraphPaper::implementation::equal;
		D2D1_POINT_2F a_pos;

		m_shape->get_pos(m_anchor, a_pos);
		return !equal(a_pos, m_a_pos);
	}

	// 元に戻す操作を実行する.
	void UndoForm::exec(void)
	{
		D2D1_POINT_2F a_pos;

		m_shape->get_pos(m_anchor, a_pos);
		m_shape->set_pos(m_a_pos, m_anchor);
		m_a_pos = a_pos;
	}

	// 操作をデータリーダーから読み込む.
	UndoForm::UndoForm(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		using winrt::GraphPaper::implementation::read;

		m_anchor = static_cast<ANCH_WHICH>(dt_reader.ReadUInt32());
		read(m_a_pos, dt_reader);
	}

	// 図形の, 指定された部位の位置を保存する.
	UndoForm::UndoForm(Shape* s, const ANCH_WHICH a) :
		Undo(s)
	{
		m_shape = s;
		m_anchor = a;
		s->get_pos(a, m_a_pos);
	}

	// データライターに書き込む.
	void UndoForm::write(DataWriter const& dt_writer)
	{
		using winrt::GraphPaper::implementation::write;

		dt_writer.WriteUInt32(static_cast<uint32_t>(U_OP::FORM));
		undo_write_shape(m_shape, dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_anchor));
		write(m_a_pos, dt_writer);
	}

	// 操作を実行すると値が変わるか調べる.
	template <U_OP U>
	bool UndoSet<U>::changed(void) const noexcept
	{
		using winrt::GraphPaper::implementation::equal;
		U_TYPE<U>::type val{};
		return GET(m_shape, val) && !equal(val, m_val);
	}
	template bool UndoSet<U_OP::ARROW_SIZE>::changed(void) const noexcept;
	template bool UndoSet<U_OP::ARROW_STYLE>::changed(void) const noexcept;
	template bool UndoSet<U_OP::FILL_COLOR>::changed(void) const noexcept;
	template bool UndoSet<U_OP::FONT_COLOR>::changed(void) const noexcept;
	template bool UndoSet<U_OP::FONT_FAMILY>::changed(void) const noexcept;
	template bool UndoSet<U_OP::FONT_SIZE>::changed(void) const noexcept;
	template bool UndoSet<U_OP::FONT_STRETCH>::changed(void) const noexcept;
	template bool UndoSet<U_OP::FONT_STYLE>::changed(void) const noexcept;
	template bool UndoSet<U_OP::FONT_WEIGHT>::changed(void) const noexcept;
	template bool UndoSet<U_OP::GRID_LEN>::changed(void) const noexcept;
	template bool UndoSet<U_OP::GRID_OPAC>::changed(void) const noexcept;
	template bool UndoSet<U_OP::GRID_SHOW>::changed(void) const noexcept;
	//template bool UndoSet<U_OP::GRID_SNAP>::changed(void) const noexcept;
	template bool UndoSet<U_OP::TEXT_LINE>::changed(void) const noexcept;
	template bool UndoSet<U_OP::TEXT_MARGIN>::changed(void) const noexcept;
	template bool UndoSet<U_OP::PAGE_COLOR>::changed(void) const noexcept;
	//template bool UndoSet<U_OP::PAGE_ZOOM>::changed(void) const noexcept;
	template bool UndoSet<U_OP::PAGE_SIZE>::changed(void) const noexcept;
	//template bool UndoSet<U_OP::PAGE_UNIT>::changed(void) const noexcept;
	template bool UndoSet<U_OP::TEXT_ALIGN_P>::changed(void) const noexcept;
	template bool UndoSet<U_OP::START_POS>::changed(void) const noexcept;
	template bool UndoSet<U_OP::STROKE_COLOR>::changed(void) const noexcept;
	template bool UndoSet<U_OP::STROKE_PATTERN>::changed(void) const noexcept;
	template bool UndoSet<U_OP::STROKE_STYLE>::changed(void) const noexcept;
	template bool UndoSet<U_OP::STROKE_WIDTH>::changed(void) const noexcept;
	template bool UndoSet<U_OP::TEXT>::changed(void) const noexcept;
	template bool UndoSet<U_OP::TEXT_ALIGN_T>::changed(void) const noexcept;
	template bool UndoSet<U_OP::TEXT_RANGE>::changed(void) const noexcept;

	// 操作を実行する.
	template <U_OP U>
	void UndoSet<U>::exec(void)
	{
		U_TYPE<U>::type t_val{};
		if (GET(m_shape, t_val)) {
			SET(m_shape, m_val);
			m_val = t_val;
		}
	}
	template void UndoSet<U_OP::ARROW_SIZE>::exec(void);
	template void UndoSet<U_OP::ARROW_STYLE>::exec(void);
	template void UndoSet<U_OP::FILL_COLOR>::exec(void);
	template void UndoSet<U_OP::FONT_COLOR>::exec(void);
	template void UndoSet<U_OP::FONT_FAMILY>::exec(void);
	template void UndoSet<U_OP::FONT_SIZE>::exec(void);
	template void UndoSet<U_OP::FONT_STRETCH>::exec(void);
	template void UndoSet<U_OP::FONT_STYLE>::exec(void);
	template void UndoSet<U_OP::FONT_WEIGHT>::exec(void);
	template void UndoSet<U_OP::GRID_LEN>::exec(void);
	template void UndoSet<U_OP::GRID_OPAC>::exec(void);
	template void UndoSet<U_OP::GRID_SHOW>::exec(void);
	//template void UndoSet<U_OP::GRID_SNAP>::exec(void);
	template void UndoSet<U_OP::TEXT_LINE>::exec(void);
	template void UndoSet<U_OP::TEXT_MARGIN>::exec(void);
	template void UndoSet<U_OP::PAGE_COLOR>::exec(void);
	template void UndoSet<U_OP::PAGE_SIZE>::exec(void);
	template void UndoSet<U_OP::TEXT_ALIGN_P>::exec(void);
	template void UndoSet<U_OP::START_POS>::exec(void);
	template void UndoSet<U_OP::STROKE_COLOR>::exec(void);
	template void UndoSet<U_OP::STROKE_PATTERN>::exec(void);
	template void UndoSet<U_OP::STROKE_STYLE>::exec(void);
	template void UndoSet<U_OP::STROKE_WIDTH>::exec(void);
	template void UndoSet<U_OP::TEXT>::exec(void);
	template void UndoSet<U_OP::TEXT_ALIGN_T>::exec(void);
	template void UndoSet<U_OP::TEXT_RANGE>::exec(void);
	//template void UndoSet<U_OP::PAGE_UNIT>::exec(void);
	//template void UndoSet<U_OP::PAGE_ZOOM>::exec(void);

	// 図形の属性値を保存する.
	template <U_OP U>
	UndoSet<U>::UndoSet(Shape* s) :
		Undo(s)
	{
		GET(m_shape, m_val);
	}
	template UndoSet<U_OP::ARROW_SIZE>::UndoSet(Shape* s);
	template UndoSet<U_OP::ARROW_STYLE>::UndoSet(Shape* s);
	template UndoSet<U_OP::FILL_COLOR>::UndoSet(Shape* s);
	template UndoSet<U_OP::FONT_COLOR>::UndoSet(Shape* s);
	template UndoSet<U_OP::FONT_FAMILY>::UndoSet(Shape* s);
	template UndoSet<U_OP::FONT_SIZE>::UndoSet(Shape* s);
	template UndoSet<U_OP::FONT_STRETCH>::UndoSet(Shape* s);
	template UndoSet<U_OP::FONT_STYLE>::UndoSet(Shape* s);
	template UndoSet<U_OP::FONT_WEIGHT>::UndoSet(Shape* s);
	template UndoSet<U_OP::GRID_LEN>::UndoSet(Shape* s);
	template UndoSet<U_OP::GRID_OPAC>::UndoSet(Shape* s);
	template UndoSet<U_OP::GRID_SHOW>::UndoSet(Shape* s);
	//template UndoSet<U_OP::GRID_SNAP>::UndoSet(Shape* s);
	template UndoSet<U_OP::TEXT_LINE>::UndoSet(Shape* s);
	template UndoSet<U_OP::TEXT_MARGIN>::UndoSet(Shape* s);
	template UndoSet<U_OP::PAGE_COLOR>::UndoSet(Shape* s);
	template UndoSet<U_OP::PAGE_SIZE>::UndoSet(Shape* s);
	template UndoSet<U_OP::TEXT_ALIGN_P>::UndoSet(Shape* s);
	template UndoSet<U_OP::START_POS>::UndoSet(Shape* s);
	template UndoSet<U_OP::STROKE_COLOR>::UndoSet(Shape* s);
	template UndoSet<U_OP::STROKE_PATTERN>::UndoSet(Shape* s);
	template UndoSet<U_OP::STROKE_STYLE>::UndoSet(Shape* s);
	template UndoSet<U_OP::STROKE_WIDTH>::UndoSet(Shape* s);
	template UndoSet<U_OP::TEXT>::UndoSet(Shape* s);
	template UndoSet<U_OP::TEXT_ALIGN_T>::UndoSet(Shape* s);
	template UndoSet<U_OP::TEXT_RANGE>::UndoSet(Shape* s);
	//template UndoSet<U_OP::PAGE_UNIT>::UndoSet(Shape* s);
	//template UndoSet<U_OP::PAGE_ZOOM>::UndoSet(Shape* s);

	// 図形の属性値を保存したあと値を格納する.
	template <U_OP U>
	UndoSet<U>::UndoSet(Shape* s, U_TYPE<U>::type const& t_val) :
		UndoSet(s)
	{
		UndoSet<U>::SET(m_shape, t_val);
	}
	template UndoSet<U_OP::ARROW_SIZE>::UndoSet(Shape* s, const ARROW_SIZE& val);
	template UndoSet<U_OP::ARROW_STYLE>::UndoSet(Shape* s, const ARROW_STYLE& val);
	template UndoSet<U_OP::FILL_COLOR>::UndoSet(Shape* s, const D2D1_COLOR_F& val);
	template UndoSet<U_OP::FONT_COLOR>::UndoSet(Shape* s, const D2D1_COLOR_F& val);
	template UndoSet<U_OP::FONT_FAMILY>::UndoSet(Shape* s, wchar_t* const& val);
	template UndoSet<U_OP::FONT_SIZE>::UndoSet(Shape* s, const double& val);
	template UndoSet<U_OP::FONT_STRETCH>::UndoSet(Shape* s, const DWRITE_FONT_STRETCH& val);
	template UndoSet<U_OP::FONT_STYLE>::UndoSet(Shape* s, const DWRITE_FONT_STYLE& val);
	template UndoSet<U_OP::FONT_WEIGHT>::UndoSet(Shape* s, const DWRITE_FONT_WEIGHT& val);
	template UndoSet<U_OP::GRID_LEN>::UndoSet(Shape* s, const double& val);
	template UndoSet<U_OP::GRID_OPAC>::UndoSet(Shape* s, const double& val);
	template UndoSet<U_OP::GRID_SHOW>::UndoSet(Shape* s, const GRID_SHOW& val);
	//template UndoSet<U_OP::GRID_SNAP>::UndoSet(Shape* s, const bool& val);
	template UndoSet<U_OP::TEXT_LINE>::UndoSet(Shape* s, const double& val);
	template UndoSet<U_OP::TEXT_MARGIN>::UndoSet(Shape* s, const D2D1_SIZE_F& val);
	template UndoSet<U_OP::PAGE_COLOR>::UndoSet(Shape* s, const D2D1_COLOR_F& val);
	template UndoSet<U_OP::PAGE_SIZE>::UndoSet(Shape* s, const D2D1_SIZE_F& val);
	template UndoSet<U_OP::TEXT_ALIGN_P>::UndoSet(Shape* s, const DWRITE_PARAGRAPH_ALIGNMENT& val);
	template UndoSet<U_OP::START_POS>::UndoSet(Shape* s, const D2D1_POINT_2F& val);
	template UndoSet<U_OP::STROKE_COLOR>::UndoSet(Shape* s, const D2D1_COLOR_F& val);
	template UndoSet<U_OP::STROKE_PATTERN>::UndoSet(Shape* s, const STROKE_PATTERN& val);
	template UndoSet<U_OP::STROKE_STYLE>::UndoSet(Shape* s, const D2D1_DASH_STYLE& val);
	template UndoSet<U_OP::STROKE_WIDTH>::UndoSet(Shape* s, const double& val);
	template UndoSet<U_OP::TEXT>::UndoSet(Shape* s, wchar_t* const& val);
	template UndoSet<U_OP::TEXT_ALIGN_T>::UndoSet(Shape* s, const DWRITE_TEXT_ALIGNMENT& val);
	template UndoSet<U_OP::TEXT_RANGE>::UndoSet(Shape* s, const DWRITE_TEXT_RANGE& val);
	//template UndoSet<U_OP::PAGE_UNIT>::UndoSet(Shape* s, const UNIT& val);
	//template UndoSet<U_OP::PAGE_ZOOM>::UndoSet(Shape* s, const double& val);

	template <U_OP U>
	UndoSet<U>::UndoSet(DataReader const& dt_reader) :
		Undo(undo_read_shape(dt_reader))
	{
		using winrt::GraphPaper::implementation::read;
		read(m_val, dt_reader);
	}
	template UndoSet<U_OP::ARROW_SIZE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::ARROW_STYLE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::FILL_COLOR>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::FONT_COLOR>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::FONT_FAMILY>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::FONT_SIZE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::FONT_STRETCH>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::FONT_STYLE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::FONT_WEIGHT>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::GRID_LEN>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::GRID_OPAC>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::GRID_SHOW>::UndoSet(DataReader const& dt_reader);
	//template UndoSet<U_OP::GRID_SNAP>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::TEXT_LINE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::TEXT_MARGIN>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::PAGE_COLOR>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::PAGE_SIZE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::TEXT_ALIGN_P>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::START_POS>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::STROKE_COLOR>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::STROKE_PATTERN>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::STROKE_STYLE>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::STROKE_WIDTH>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::TEXT>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::TEXT_ALIGN_T>::UndoSet(DataReader const& dt_reader);
	template UndoSet<U_OP::TEXT_RANGE>::UndoSet(DataReader const& dt_reader);
	//template UndoSet<U_OP::PAGE_UNIT>::UndoSet(DataReader const& dt_reader);
	//template UndoSet<U_OP::PAGE_ZOOM>::UndoSet(DataReader const& dt_reader);

	// 図形の属性値に値を格納する.
	template <U_OP U>
	void UndoSet<U>::SET(Shape* s, const U_TYPE<U>::type& t_val)
	{
		throw winrt::hresult_not_implemented();
	}
	void UndoSet<U_OP::ARROW_SIZE>::SET(Shape* s, const ARROW_SIZE& t_val)
	{
		s->set_arrow_size(t_val);
	}
	void UndoSet<U_OP::ARROW_STYLE>::SET(Shape* s, const ARROW_STYLE& t_val)
	{
		s->set_arrow_style(t_val);
	}
	void UndoSet<U_OP::FILL_COLOR>::SET(Shape* s, const D2D1_COLOR_F& t_val)
	{
		s->set_fill_color(t_val);
	}
	void UndoSet<U_OP::FONT_COLOR>::SET(Shape* s, const D2D1_COLOR_F& t_val)
	{
		s->set_font_color(t_val);
	}
	void UndoSet<U_OP::FONT_FAMILY>::SET(Shape* s, wchar_t* const& t_val)
	{
		s->set_font_family(t_val);
	}
	void UndoSet<U_OP::FONT_SIZE>::SET(Shape* s, const double& t_val)
	{
		s->set_font_size(t_val);
	}
	void UndoSet<U_OP::FONT_STRETCH>::SET(Shape* s, const DWRITE_FONT_STRETCH& t_val)
	{
		s->set_font_stretch(t_val);
	}
	void UndoSet<U_OP::FONT_STYLE>::SET(Shape* s, const DWRITE_FONT_STYLE& t_val)
	{
		s->set_font_style(t_val);
	}
	void UndoSet<U_OP::FONT_WEIGHT>::SET(Shape* s, const DWRITE_FONT_WEIGHT& t_val)
	{
		s->set_font_weight(t_val);
	}
	void UndoSet<U_OP::GRID_LEN>::SET(Shape* s, const double& t_val)
	{
		s->set_grid_len(t_val);
	}
	void UndoSet<U_OP::GRID_OPAC>::SET(Shape* s, const double& t_val)
	{
		s->set_grid_opac(t_val);
	}
	void UndoSet<U_OP::GRID_SHOW>::SET(Shape* s, const GRID_SHOW& t_val)
	{
		s->set_grid_show(t_val);
	}
	//void UndoSet<U_OP::GRID_SNAP>::SET(Shape* s, const bool& t_val)
	//{
	//	s->set_grid_snap(t_val);
	//}
	void UndoSet<U_OP::TEXT_LINE>::SET(Shape* s, const double& t_val)
	{
		s->set_text_line(t_val);
	}
	void UndoSet<U_OP::TEXT_MARGIN>::SET(Shape* s, const D2D1_SIZE_F& t_val)
	{
		s->set_text_margin(t_val);
	}
	void UndoSet<U_OP::PAGE_COLOR>::SET(Shape* s, const D2D1_COLOR_F& t_val)
	{
		s->set_page_color(t_val);
	}
	//void UndoSet<U_OP::PAGE_ZOOM>::SET(Shape* s, const double& t_val)
	//{
	//	s->set_page_scale(t_val);
	//}
	void UndoSet<U_OP::PAGE_SIZE>::SET(Shape* s, const D2D1_SIZE_F& t_val)
	{
		s->set_page_size(t_val);
	}
	//void UndoSet<U_OP::PAGE_UNIT>::SET(Shape* s, const UNIT& t_val)
	//{
	//	s->set_page_unit(t_val);
	//}
	void UndoSet<U_OP::TEXT_ALIGN_P>::SET(Shape* s, const DWRITE_PARAGRAPH_ALIGNMENT& t_val)
	{
		s->set_text_align_p(t_val);
	}
	void UndoSet<U_OP::START_POS>::SET(Shape* s, const D2D1_POINT_2F& t_val)
	{
		s->set_start_pos(t_val);
	}
	void UndoSet<U_OP::STROKE_COLOR>::SET(Shape* s, const D2D1_COLOR_F& t_val)
	{
		s->set_stroke_color(t_val);
	}
	void UndoSet<U_OP::STROKE_PATTERN>::SET(Shape* s, const STROKE_PATTERN& t_val)
	{
		s->set_stroke_pattern(t_val);
	}
	void UndoSet<U_OP::STROKE_STYLE>::SET(Shape* s, const D2D1_DASH_STYLE& t_val)
	{
		s->set_stroke_style(t_val);
	}
	void UndoSet<U_OP::STROKE_WIDTH>::SET(Shape* s, const double& t_val)
	{
		s->set_stroke_width(t_val);
	}
	void UndoSet<U_OP::TEXT>::SET(Shape* s, wchar_t* const& t_val)
	{
		s->set_text(t_val);
	}
	void UndoSet<U_OP::TEXT_ALIGN_T>::SET(Shape* s, const DWRITE_TEXT_ALIGNMENT& t_val)
	{
		s->set_text_align_t(t_val);
	}
	void UndoSet<U_OP::TEXT_RANGE>::SET(Shape* s, const DWRITE_TEXT_RANGE& t_val)
	{
		s->set_text_range(t_val);
	}

	template <U_OP U>
	bool UndoSet<U>::GET(Shape* s, U_TYPE<U>::type& t_val) noexcept
	{
		return false;
	}
	bool UndoSet<U_OP::ARROW_SIZE>::GET(Shape* s, ARROW_SIZE& t_val) noexcept
	{
		return s->get_arrow_size(t_val);
	}
	bool UndoSet<U_OP::ARROW_STYLE>::GET(Shape* s, ARROW_STYLE& t_val) noexcept
	{
		return s->get_arrow_style(t_val);
	}
	bool UndoSet<U_OP::FILL_COLOR>::GET(Shape* s, D2D1_COLOR_F& t_val) noexcept
	{
		return s->get_fill_color(t_val);
	}
	bool UndoSet<U_OP::FONT_COLOR>::GET(Shape* s, D2D1_COLOR_F& t_val) noexcept
	{
		return s->get_font_color(t_val);
	}
	bool UndoSet<U_OP::FONT_FAMILY>::GET(Shape* s, wchar_t*& t_val) noexcept
	{
		return s->get_font_family(t_val);
	}
	bool UndoSet<U_OP::FONT_SIZE>::GET(Shape* s, double& t_val) noexcept
	{
		return s->get_font_size(t_val);
	}
	bool UndoSet<U_OP::FONT_STRETCH>::GET(Shape* s, DWRITE_FONT_STRETCH& t_val) noexcept
	{
		return s->get_font_stretch(t_val);
	}
	bool UndoSet<U_OP::FONT_STYLE>::GET(Shape* s, DWRITE_FONT_STYLE& t_val) noexcept
	{
		return s->get_font_style(t_val);
	}
	bool UndoSet<U_OP::FONT_WEIGHT>::GET(Shape* s, DWRITE_FONT_WEIGHT& t_val) noexcept
	{
		return s->get_font_weight(t_val);
	}
	bool UndoSet<U_OP::GRID_LEN>::GET(Shape* s, double& t_val) noexcept
	{
		return s->get_grid_len(t_val);
	}
	bool UndoSet<U_OP::GRID_OPAC>::GET(Shape* s, double& t_val) noexcept
	{
		return s->get_grid_opac(t_val);
	}
	bool UndoSet<U_OP::GRID_SHOW>::GET(Shape* s, GRID_SHOW& t_val) noexcept
	{
		return s->get_grid_show(t_val);
	}
	//bool UndoSet<U_OP::GRID_SNAP>::GET(Shape* s, bool& t_val) noexcept
	//{
	//	return s->get_grid_snap(t_val);
	//}
	bool UndoSet<U_OP::TEXT_LINE>::GET(Shape* s, double& t_val) noexcept
	{
		return s->get_text_line(t_val);
	}
	bool UndoSet<U_OP::TEXT_MARGIN>::GET(Shape* s, D2D1_SIZE_F& t_val) noexcept
	{
		return s->get_text_margin(t_val);
	}
	bool UndoSet<U_OP::PAGE_COLOR>::GET(Shape* s, D2D1_COLOR_F& t_val) noexcept
	{
		return s->get_page_color(t_val);
	}
	bool UndoSet<U_OP::PAGE_SIZE>::GET(Shape* s, D2D1_SIZE_F& t_val) noexcept
	{
		return s->get_page_size(t_val);
	}
	//bool UndoSet<U_OP::PAGE_UNIT>::GET(Shape* s, UNIT& t_val) noexcept
	//{
	//	return s->get_page_unit(t_val);
	//}
	//bool UndoSet<U_OP::PAGE_ZOOM>::GET(Shape* s, double& t_val) noexcept
	//{
	//	return s->get_page_scale(t_val);
	//}
	bool UndoSet<U_OP::TEXT_ALIGN_P>::GET(Shape* s, DWRITE_PARAGRAPH_ALIGNMENT& t_val) noexcept
	{
		return s->get_text_align_p(t_val);
	}
	bool UndoSet<U_OP::START_POS>::GET(Shape* s, D2D1_POINT_2F& t_val) noexcept
	{
		return s->get_start_pos(t_val);
	}
	bool UndoSet<U_OP::STROKE_COLOR>::GET(Shape* s, D2D1_COLOR_F& t_val) noexcept
	{
		return s->get_stroke_color(t_val);
	}
	bool UndoSet<U_OP::STROKE_PATTERN>::GET(Shape* s, STROKE_PATTERN& t_val) noexcept
	{
		return s->get_stroke_pattern(t_val);
	}
	bool UndoSet<U_OP::STROKE_STYLE>::GET(Shape* s, D2D1_DASH_STYLE& t_val) noexcept
	{
		return s->get_stroke_style(t_val);
	}
	bool UndoSet<U_OP::STROKE_WIDTH>::GET(Shape* s, double& t_val) noexcept
	{
		return s->get_stroke_width(t_val);
	}
	bool UndoSet<U_OP::TEXT>::GET(Shape* s, wchar_t*& t_val) noexcept
	{
		return s->get_text(t_val);
	}
	bool UndoSet<U_OP::TEXT_ALIGN_T>::GET(Shape* s, DWRITE_TEXT_ALIGNMENT& t_val) noexcept
	{
		return s->get_text_align_t(t_val);
	}
	bool UndoSet<U_OP::TEXT_RANGE>::GET(Shape* s, DWRITE_TEXT_RANGE& t_val) noexcept
	{
		return s->get_text_range(t_val);
	}

	// データライターに書き込む.
	template <U_OP U>
	void UndoSet<U>::write(DataWriter const& dt_writer)
	{
		using winrt::GraphPaper::implementation::write;
		dt_writer.WriteUInt32(static_cast<uint32_t>(U));
		undo_write_shape(m_shape, dt_writer);
		write(m_val, dt_writer);
	}
	template void UndoSet<U_OP::ARROW_SIZE>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::ARROW_STYLE>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::FILL_COLOR>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::FONT_COLOR>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::FONT_FAMILY>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::FONT_SIZE>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::FONT_STRETCH>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::FONT_STYLE>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::FONT_WEIGHT>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::GRID_LEN>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::GRID_OPAC>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::GRID_SHOW>::write(DataWriter const& dt_writer);
	//template void UndoSet<U_OP::GRID_SNAP>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::TEXT_LINE>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::TEXT_MARGIN>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::PAGE_COLOR>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::PAGE_SIZE>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::TEXT_ALIGN_P>::write(DataWriter const& dt_writer);
	//template void UndoSet<U_OP::PAGE_UNIT>::write(DataWriter const& dt_writer);
	//template void UndoSet<U_OP::PAGE_ZOOM>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::START_POS>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::STROKE_COLOR>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::STROKE_PATTERN>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::STROKE_STYLE>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::STROKE_WIDTH>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::TEXT>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::TEXT_ALIGN_T>::write(DataWriter const& dt_writer);
	template void UndoSet<U_OP::TEXT_RANGE>::write(DataWriter const& dt_writer);

}