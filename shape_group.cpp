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
	// sh	表示する用紙
	//------------------------------
	void ShapeGroup::draw(void)
	{
		//ID2D1DeviceContext* const context = sheet.m_d2d.m_d2d_context.get();
		ID2D1RenderTarget* const context = Shape::s_target;
		ID2D1SolidColorBrush* const brush = Shape::s_color_brush;

		// 選択フラグが立ってるか判定する.
		if (is_selected()) {
			D2D1_POINT_2F b_min{ FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max{ -FLT_MAX, -FLT_MAX };
			// グループ化された各図形について以下を繰り返す.
			for (const auto s : m_list_grouped) {
				// 消去フラグが立っているか判定する.
				if (s->is_deleted()) {
					continue;
				}
				s->draw();
				s->get_bound(b_min, b_max, b_min, b_max);
			}
			context->DrawRectangle(D2D1_RECT_F{ b_min.x, b_min.y, b_max.x, b_max.y }, brush, 1.0f, Shape::m_aux_style.get());
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
	// a_min	元の領域の左上位置.
	// a_man	元の領域の右下位置.
	// b_min	得られた領域の左上位置.
	// b_max	得られた領域の右下位置.
	//------------------------------
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

	//------------------------------
	// 図形を囲む領域の左上位置を得る.
	// val	領域の左上位置
	//------------------------------
	void ShapeGroup::get_pos_min(D2D1_POINT_2F& val) const noexcept
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
		//if (m_list_grouped.empty()) {
		//	return false;
		//}
		auto flag = false;
		for (const auto s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			D2D1_POINT_2F pos;
			s->get_pos_min(pos);
			if (!flag) {
				val = pos;
				flag = true;
			}
			else {
				val.x = pos.x < val.x ? pos.x : val.x;
				val.y = pos.y < val.y ? pos.y : val.y;
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
	// t_pos	判定する位置
	// 戻り値	位置を含む図形の部位
	//------------------------------
	uint32_t ShapeGroup::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->hit_test(t_pos) != ANC_TYPE::ANC_SHEET) {
				return ANC_TYPE::ANC_FILL;
			}
		}
		return ANC_TYPE::ANC_SHEET;
	}

	//------------------------------
	// 範囲に含まれるか判定する.
	// area_min	範囲の左上位置
	// area_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	//------------------------------
	bool ShapeGroup::in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept
	{
		for (const Shape* s : m_list_grouped) {
			if (!s->in_area(area_min, area_max)) {
				return false;
			}
		}
		return true;
	}

	//------------------------------
	// 差分だけ移動する
	// d_vec	差分ベクトル
	//------------------------------
	bool ShapeGroup::move(const D2D1_POINT_2F d_vec) noexcept
	{
		return slist_move(m_list_grouped, d_vec);
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
		D2D1_POINT_2F b_min;
		if (get_pos_start(b_min) && !equal(val, b_min)) {
			D2D1_POINT_2F d_vec;
			pt_sub(val, b_min, d_vec);
			move(d_vec);
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