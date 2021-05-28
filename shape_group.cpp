//------------------------------
// Shape_group.cpp
// グループ図形
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	// 図形を破棄する.
	ShapeGroup::~ShapeGroup(void)
	{
		s_list_clear(m_list_grouped);
	}

	// 図形を表示する.
	void ShapeGroup::draw(SHAPE_DX& sc)
	{
		if (is_selected()) {
			// 選択フラグが立っている場合,
			D2D1_POINT_2F b_min{ FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max{ -FLT_MAX, -FLT_MAX };
			// グループ化された各図形について以下を繰り返す.
			for (const auto s : m_list_grouped) {
				if (s->is_deleted()) {
					// 消去フラグが立っている場合,
					// 無視する.
					continue;
				}
				s->draw(sc);
				s->get_bound(b_min, b_max, b_min, b_max);
			}
			const D2D1_RECT_F r{
				b_min.x, b_min.y,
				b_max.x, b_max.y
			};
			auto br = sc.m_shape_brush.get();
			auto ss = sc.m_aux_style.get();
			sc.m_d2dContext->DrawRectangle(r, br, 1.0f, ss);
		}
		else {
			for (const auto s : m_list_grouped) {
				if (s->is_deleted()) {
					continue;
				}
				s->draw(sc);
			}
		}
	}

	// 図形を囲む領域を得る.
	// a_min	元の領域の左上位置.
	// a_man	元の領域の右下位置.
	// b_min	得られた領域の左上位置.
	// b_max	得られた領域の右下位置.
	void ShapeGroup::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		b_min = a_min;
		b_max = a_max;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_min, b_max, b_min, b_max);
		}
	}

	// 図形を囲む領域の左上位置を得る.
	// value	領域の左上位置
	void ShapeGroup::get_min_pos(D2D1_POINT_2F& value) const noexcept
	{
		get_start_pos(value);
	}

	// 開始位置を得る.
	// value	開始位置
	// グループ図形の場合, 開始位置は図形を囲む領域の左上位置.
	bool ShapeGroup::get_start_pos(D2D1_POINT_2F& value) const noexcept
	{
		//if (m_list_grouped.empty()) {
		//	return false;
		//}
		auto flag = false;
		for (auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			D2D1_POINT_2F pos;
			s->get_min_pos(pos);
			if (flag != true) {
				value = pos;
				flag = true;
			}
			else {
				pt_min(pos, value, value);
			}
		}
		return flag;
	}

	// 文字列図形を含むか判定する.
	bool ShapeGroup::has_text(void) noexcept
	{
		std::list<S_LIST_T::iterator> stack;
		stack.push_back(m_list_grouped.begin());
		stack.push_back(m_list_grouped.end());
		while (stack.empty() != true) {
			auto j = stack.back();
			stack.pop_back();
			auto i = stack.back();
			stack.pop_back();
			while (i != j) {
				auto s = *i++;
				if (s == nullptr || s->is_deleted()) {
					continue;
				}
				if (typeid(*s) == typeid(ShapeText)) {
					return true;
				}
				else if (typeid(*s) == typeid(ShapeGroup)) {
					stack.push_back(i);
					stack.push_back(j);
					i = static_cast<ShapeGroup*>(s)->m_list_grouped.begin();
					j = static_cast<ShapeGroup*>(s)->m_list_grouped.end();
					continue;
				}
			}
		}
		return false;
	}

	// 位置を含むか判定する.
	// t_pos	判定する位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	uint32_t ShapeGroup::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->hit_test(t_pos, a_len) != ANCH_TYPE::ANCH_SHEET) {
				return ANCH_TYPE::ANCH_FILL;
			}
		}
		return ANCH_TYPE::ANCH_SHEET;
	}

	// 範囲に含まれるか判定する.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeGroup::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		for (const auto s : m_list_grouped) {
			if (s->in_area(a_min, a_max) != true) {
				return false;
			}
		}
		return true;
	}

	// 消去フラグを判定する.
	bool ShapeGroup::is_deleted(void) const noexcept
	{
		return m_list_grouped.size() == 0 || m_list_grouped.back()->is_deleted();
	}

	// 選択フラグを判定する.
	bool ShapeGroup::is_selected(void) const noexcept
	{
		// グループに含まれる図形が選択されてるか判定する.
		return m_list_grouped.size() > 0 && m_list_grouped.back()->is_selected();
	}

	// 差分だけ移動する
	void ShapeGroup::move(const D2D1_POINT_2F diff)
	{
		s_list_move(m_list_grouped, diff);
	}

	// 値を消去フラグに格納する.
	void ShapeGroup::set_delete(const bool value) noexcept
	{
		for (const auto s : m_list_grouped) {
			s->set_delete(value);
		}
	}

	// 値を選択フラグに格納する.
	void ShapeGroup::set_select(const bool value) noexcept
	{
		for (const auto s : m_list_grouped) {
			s->set_select(value);
		}
	}

	// 値を開始位置に格納する. 他の部位の位置も動く.
	void ShapeGroup::set_start_pos(const D2D1_POINT_2F value)
	{
		D2D1_POINT_2F b_min;
		if (get_start_pos(b_min) && !equal(value, b_min)) {
			D2D1_POINT_2F diff;
			pt_sub(value, b_min, diff);
			move(diff);
		}
	}

	// 図形をデータリーダーから作成する.
	ShapeGroup::ShapeGroup(DataReader const& dt_reader)
	{
		s_list_read(m_list_grouped, dt_reader);
	}

	// データライターに書き込む.
	void ShapeGroup::write(DataWriter const& dt_writer) const
	{
		constexpr bool REDUCED = true;
		s_list_write<!REDUCED>(m_list_grouped, dt_writer);
	}

	// データライターに SVG として書き込む.
	void ShapeGroup::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<g>" SVG_NEW_LINE, dt_writer);
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->write_svg(dt_writer);
		}
		write_svg("</g>" SVG_NEW_LINE, dt_writer);
	}
}