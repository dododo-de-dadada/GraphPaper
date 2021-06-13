//------------------------------
// Shape_path.cpp
// Ü‚êü‚Ì‚Ğ‚ÈŒ^
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ·•ª‚¾‚¯ˆÚ“®‚·‚é.
	// d_vec	·•ª
	bool ShapePath::move(const D2D1_POINT_2F d_vec)
	{
		if (ShapeStroke::move(d_vec)) {
			create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// ’l‚ğ, •”ˆÊ‚ÌˆÊ’u‚ÉŠi”[‚·‚é. ‘¼‚Ì•”ˆÊ‚ÌˆÊ’u‚Í“®‚©‚È‚¢. 
	// value	Ši”[‚·‚é’l
	// anch	}Œ`‚Ì•”ˆÊ
	bool ShapePath::set_anch_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		if (ShapeStroke::set_anch_pos(value, anch)) {
			create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// –î‚¶‚é‚µ‚ÌŒ`®‚ÉŠi”[‚·‚é.
	bool ShapePath::set_arrow_size(const ARROW_SIZE& value)
	{
		if (!equal(m_arrow_size, value)) {
			m_arrow_size = value;
			create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// –î‚¶‚é‚µ‚ÌŒ`®‚ÉŠi”[‚·‚é.
	bool ShapePath::set_arrow_style(const ARROW_STYLE value)
	{
		if (m_arrow_style != value) {
			m_arrow_style = value;
			create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// n“_‚É’l‚ğŠi”[‚·‚é. ‘¼‚Ì•”ˆÊ‚ÌˆÊ’u‚à“®‚­.
	// value	Ši”[‚·‚é’l
	bool ShapePath::set_start_pos(const D2D1_POINT_2F value)
	{
		if (ShapeStroke::set_start_pos(value)) {
			create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// ƒf[ƒ^ƒ‰ƒCƒ^[‚É‘‚«‚Ş.
	void ShapePath::write(DataWriter const& dt_writer) const
	{
		ShapeLineA::write(dt_writer);
	}

}