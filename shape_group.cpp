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
		s_list_clear(m_grp_list);
	}

	// 図形を表示する.
	void ShapeGroup::draw(SHAPE_DX& sc)
	{
		if (is_selected()) {
			D2D1_POINT_2F b_min{ FLT_MAX, FLT_MAX };
			D2D1_POINT_2F b_max{ -FLT_MAX, -FLT_MAX };
			for (const auto s : m_grp_list) {
				if (s->is_deleted()) {
					continue;
				}
				s->draw(sc);
				s->get_bound(b_min, b_max);
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
			for (const auto s : m_grp_list) {
				if (s->is_deleted()) {
					continue;
				}
				s->draw(sc);
			}
		}
	}

	// 図形を囲む方形を得る.
	void ShapeGroup::get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		for (const auto s : m_grp_list) {
			if (s->is_deleted()) {
				continue;
			}
			s->get_bound(b_min, b_max);
		}
	}

	// 図形を囲む方形の左上点を得る.
	void ShapeGroup::get_min_pos(D2D1_POINT_2F& val) const noexcept
	{
		auto flag = true;
		for (const auto s : m_grp_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (flag == true) {
				flag = false;
				s->get_min_pos(val);
				continue;
			}
			D2D1_POINT_2F pos;
			s->get_min_pos(pos);
			pt_min(pos, val, val);
		}
	}

	// 始点を得る
	bool ShapeGroup::get_start_pos(D2D1_POINT_2F& val) const noexcept
	{
		get_min_pos(val);
		return true;
	}

	// 位置を含むか調べる.
	// t_pos	調べる位置
	// a_len	部位の大きさ
	// 戻り値	位置を含む図形の部位
	ANCH_WHICH ShapeGroup::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		for (const auto s : m_grp_list) {
			if (s->is_deleted()) {
				continue;
			}
			if (s->hit_test(t_pos, a_len) != ANCH_WHICH::ANCH_OUTSIDE) {
				return ANCH_WHICH::ANCH_INSIDE;
			}
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// 範囲に含まれるか調べる.
	// a_min	範囲の左上位置
	// a_max	範囲の右下位置
	// 戻り値	含まれるなら true
	// 線の太さは考慮されない.
	bool ShapeGroup::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		for (const auto s : m_grp_list) {
			if (s->in_area(a_min, a_max) == false) {
				return false;
			}
		}
		return true;
	}

	// 消去フラグを調べる.
	bool ShapeGroup::is_deleted(void) const noexcept
	{
		return m_grp_list.size() == 0 || m_grp_list.back()->is_deleted();
	}

	// 選択フラグを調べる.
	bool ShapeGroup::is_selected(void) const noexcept
	{
		// グループに含まれる図形が選択されてるか調べる.
		return m_grp_list.size() > 0 && m_grp_list.back()->is_selected();
	}

	// 差分だけ移動する
	void ShapeGroup::move(const D2D1_POINT_2F d)
	{
		s_list_move(m_grp_list, d);
	}

	// 値を消去フラグに格納する.
	void ShapeGroup::set_delete(const bool val) noexcept
	{
		for (const auto s : m_grp_list) {
			s->set_delete(val);
		}
	}

	// 値を選択フラグに格納する.
	void ShapeGroup::set_select(const bool val) noexcept
	{
		for (const auto s : m_grp_list) {
			s->set_select(val);
		}
	}

	// 値を始点に格納する. 他の部位の位置も動く.
	void ShapeGroup::set_start_pos(const D2D1_POINT_2F pos)
	{
		D2D1_POINT_2F b_min;
		D2D1_POINT_2F d;

		get_min_pos(b_min);
		if (equal(pos, b_min)) {
			return;
		}
		pt_sub(pos, b_min, d);
		move(d);
	}

	// 図形をデータリーダーから作成する.
	ShapeGroup::ShapeGroup(DataReader const& dt_reader)
	{
		s_list_read(m_grp_list, dt_reader);
	}

	// データライターに書き込む.
	void ShapeGroup::write(DataWriter const& dt_writer) const
	{
		constexpr bool REDUCED = true;
		s_list_write<!REDUCED>(m_grp_list, dt_writer);
	}

	// データライターに SVG として書き込む.
	void ShapeGroup::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<g>" SVG_NL, dt_writer);
		for (const auto s : m_grp_list) {
			if (s->is_deleted()) {
				continue;
			}
			s->write_svg(dt_writer);
		}
		write_svg("</g>" SVG_NL, dt_writer);
	}
}