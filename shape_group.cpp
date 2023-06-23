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
	void ShapeGroup::draw(void) noexcept
	{
		// 選択フラグが立ってるか判定する.
		if (m_loc_show && is_selected()) {
			D2D1_POINT_2F b_lt { FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_rb{ -FLT_MAX, -FLT_MAX };
			// グループ化された各図形について以下を繰り返す.
			for (const auto s : m_list_grouped) {
				// 消去フラグが立っているか判定する.
				if (s->is_deleted()) {
					continue;
				}
				s->draw();
				s->get_bbox(b_lt, b_rb, b_lt, b_rb);
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

	// 境界矩形を得る.
	void ShapeGroup::get_bbox(
		const D2D1_POINT_2F a_lt,	// 元の矩形の左上位置.
		const D2D1_POINT_2F a_rb,	// 元の矩形の右下位置.
		D2D1_POINT_2F& b_lt,	// 得られた矩形の左上位置.
		D2D1_POINT_2F& b_rb	// 得られた矩形の右下位置.
	) const noexcept
	{
		b_lt = a_lt;
		b_rb = a_rb;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bbox(b_lt, b_rb, b_lt, b_rb);
		}
	}

	// 境界矩形の左上点を得る.
	void ShapeGroup::get_bbox_lt(D2D1_POINT_2F& val) const noexcept
	{
		get_pos_start(val);
	}

	//------------------------------
	// 始点を得る.
	// val	始点
	// グループ図形の場合, 始点は図形を囲む矩形の左上点.
	//------------------------------
	bool ShapeGroup::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		auto flag = false;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			D2D1_POINT_2F lt;
			s->get_bbox_lt(lt);
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

	// 図形が点を含むか判定する.
	// test_pt	判定される点
	// 戻り値	点を含む部位
	uint32_t ShapeGroup::hit_test(const D2D1_POINT_2F test_pt, const bool/*ctrl_key*/) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->hit_test(test_pt, false) != LOCUS_TYPE::LOCUS_SHEET) {
				return LOCUS_TYPE::LOCUS_FILL;
			}
		}
		return LOCUS_TYPE::LOCUS_SHEET;
	}

	//------------------------------
	// 矩形に含まれるか判定する.
	// lt	矩形の左上位置
	// rb	矩形の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	//------------------------------
	bool ShapeGroup::is_inside(const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (!s->is_inside(lt, rb)) {
				return false;
			}
		}
		return true;
	}

	//------------------------------
	// 位置を移動する.
	// to	移動先へのベクトル
	//------------------------------
	bool ShapeGroup::move(const D2D1_POINT_2F to) noexcept
	{
		return slist_move_selected(m_list_grouped, to);
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
	// 値を始点に格納する. 他の部位の位置も動く.
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
	ShapeGroup::ShapeGroup(DataReader const& dt_reader)
	{
		slist_read(m_list_grouped, dt_reader);
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