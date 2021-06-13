//------------------------------
// Shape_path.cpp
// �܂���̂ЂȌ^
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ���������ړ�����.
	// d_vec	����
	bool ShapePath::move(const D2D1_POINT_2F d_vec)
	{
		if (ShapeStroke::move(d_vec)) {
			create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	// value	�i�[����l
	// anch	�}�`�̕���
	bool ShapePath::set_anch_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		if (ShapeStroke::set_anch_pos(value, anch)) {
			create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// ��邵�̌`���Ɋi�[����.
	bool ShapePath::set_arrow_size(const ARROW_SIZE& value)
	{
		if (!equal(m_arrow_size, value)) {
			m_arrow_size = value;
			create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// ��邵�̌`���Ɋi�[����.
	bool ShapePath::set_arrow_style(const ARROW_STYLE value)
	{
		if (m_arrow_style != value) {
			m_arrow_style = value;
			create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	// value	�i�[����l
	bool ShapePath::set_start_pos(const D2D1_POINT_2F value)
	{
		if (ShapeStroke::set_start_pos(value)) {
			create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapePath::write(DataWriter const& dt_writer) const
	{
		ShapeLineA::write(dt_writer);
	}

}