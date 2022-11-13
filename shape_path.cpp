//------------------------------
// Shape_path.cpp
// �܂���̂ЂȌ^
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::Streams::DataWriter;

	// ���������ړ�����.
	// d_vec	����
	bool ShapePath::move(const D2D1_POINT_2F d_vec) noexcept
	{
		if (ShapeStroke::move(d_vec)) {
			//m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			//create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	// val	�l
	// anc	�}�`�̕���
	// limit	���E���� (���̒��_�Ƃ̋��������̒l�����ɂȂ�Ȃ�, ���̒��_�Ɉʒu�ɍ��킹��)
	bool ShapePath::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept
	{
		if (ShapeStroke::set_pos_anc(val, anc, limit, keep_aspect)) {
			//m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			//create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// ��邵�̌`���Ɋi�[����.
	bool ShapePath::set_arrow_size(const ARROW_SIZE& val) noexcept
	{
		if (!equal(m_arrow_size, val)) {
			m_arrow_size = val;
			//m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			//create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// ��邵�̌`���Ɋi�[����.
	bool ShapePath::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		if (m_arrow_style != val) {
			m_arrow_style = val;
			//m_d2d_arrow_geom = nullptr;
			if (m_d2d_path_geom != nullptr) {
				m_d2d_path_geom = nullptr;
			}
			//create_path_geometry(Shape::s_d2d_factory);
			return true;
		}
		return false;
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	// val	�i�[����l
	bool ShapePath::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		if (ShapeStroke::set_pos_start(val)) {
			m_d2d_path_geom = nullptr;
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