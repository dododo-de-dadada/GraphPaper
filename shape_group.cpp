//------------------------------
// Shape_group.cpp
// グループ図形
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	//------------------------------
	// 図形を表示する.
	//------------------------------
	void ShapeGroup::draw(void)
	{
		// 選択フラグが立ってるか判定する.
		if (m_anc_show && is_selected()) {
			D2D1_POINT_2F b_lt { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_rb{ -FLT_MAX, -FLT_MAX };
			// グループ化された各図形について以下を繰り返す.
			for (const auto s : m_list_grouped) {
				// 消去フラグが立っているか判定する.
				if (s->is_deleted()) {
					continue;
				}
				s->draw();
				s->get_bound(b_lt, b_rb, b_lt, b_rb);
			}
			ID2D1RenderTarget* const target = Shape::m_d2d_target;
			ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
			target->DrawRectangle(D2D1_RECT_F{ b_lt.x, b_lt.y, b_rb.x, b_rb.y }, brush,
				m_aux_width, m_aux_style.get());
		}
		else {
			for (const auto s : m_list_grouped) {
				if (s->is_deleted()) {
					continue;
				}
				s->draw();
			}
		}
	}

	//------------------------------
	// 図形を囲む領域を得る.
	// a_lt	元の領域の左上位置.
	// a_rb	元の領域の右下位置.
	// b_lt	得られた領域の左上位置.
	// b_rb	得られた領域の右下位置.
	//------------------------------
	void ShapeGroup::get_bound(
		const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt,
		D2D1_POINT_2F& b_rb) const noexcept
	{
		b_lt = a_lt;
		b_rb = a_rb;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_lt, b_rb, b_lt, b_rb);
		}
	}

	//------------------------------
	// 図形を囲む領域の左上位置を得る.
	// val	領域の左上位置
	//------------------------------
	void ShapeGroup::get_bound_lt(D2D1_POINT_2F& val) const noexcept
	{
		get_pos_start(val);
	}

	//------------------------------
	// 開始位置を得る.
	// val	開始位置
	// グループ図形の場合, 開始位置は図形を囲む領域の左上位置.
	//------------------------------
	bool ShapeGroup::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		auto flag = false;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			D2D1_POINT_2F lt;
			s->get_bound_lt(lt);
			if (!flag) {
				val = lt;
				flag = true;
			}
			else {
				val.x = lt.x < val.x ? lt.x : val.x;
				val.y = lt.y < val.y ? lt.y : val.y;
			}
		}
		return flag;
	}

	//------------------------------
	// 文字列図形を含むか判定する.
	//------------------------------
	bool ShapeGroup::has_text(void) noexcept
	{
		std::list<SHAPE_LIST::iterator> stack;
		stack.push_back(m_list_grouped.begin());
		stack.push_back(m_list_grouped.end());
		while (!stack.empty()) {
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

	//------------------------------
	// 位置を含むか判定する.
	// test	判定する位置
	// 戻り値	位置を含む図形の部位
	//------------------------------
	uint32_t ShapeGroup::hit_test(const D2D1_POINT_2F test) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->hit_test(test) != ANC_TYPE::ANC_PAGE) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_PAGE;
	}

	//------------------------------
	// 範囲に含まれるか判定する.
	// area_lt	範囲の左上位置
	// area_rb	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	//------------------------------
	bool ShapeGroup::in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (!s->in_area(area_lt, area_rb)) {
				return false;
			}
		}
		return true;
	}

	//------------------------------
	// 位置を移動する
	// pos	移動する位置ベクトル
	//------------------------------
	bool ShapeGroup::move(const D2D1_POINT_2F pos) noexcept
	{
		return slist_move_selected(m_list_grouped, pos);
	}

	//------------------------------
	// 値を消去フラグに格納する.
	// val	格納する値
	// 戻り値	変更されたなら true
	//------------------------------
	bool ShapeGroup::set_delete(const bool val) noexcept
	{
		bool flag = false;
		for (Shape* s : m_list_grouped) {
			if (s->set_delete(val) && !flag) {
				flag = true;
			}
		}
		return flag;
	}

	//------------------------------
	// 値を開始位置に格納する. 他の部位の位置も動く.
	// val	格納する値
	// 戻り値	変更されたなら true
	//------------------------------
	bool ShapeGroup::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F old_val;
		if (get_pos_start(old_val) && !equal(val, old_val)) {
			D2D1_POINT_2F pos;
			pt_sub(val, old_val, pos);
			move(pos);
			return true;
		}
		return false;
	}

	//------------------------------
	// 値を選択フラグに格納する.
	// val	格納する値
	// 戻り値	変更されたなら true
	//------------------------------
	bool ShapeGroup::set_select(const bool val) noexcept
	{
		bool flag = false;
		for (Shape* s : m_list_grouped) {
			if (s->set_select(val) && !flag) {
				flag = true;
			}
		}
		return flag;
	}

	//------------------------------
	// 図形をデータリーダーから読み込む.
	//------------------------------
	ShapeGroup::ShapeGroup(const Shape& page, DataReader const& dt_reader)
	{
		slist_read(m_list_grouped, page, dt_reader);
	}

	//------------------------------
	// 図形をデータライターに書き込む.
	//------------------------------
	void ShapeGroup::write(DataWriter const& dt_writer) const
	{
		constexpr bool REDUCED = true;
		slist_write<!REDUCED>(m_list_grouped, dt_writer);
	}

}