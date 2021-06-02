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
	// diff	����
	bool ShapePath::move(const D2D1_POINT_2F diff)
	{
		if (ShapeStroke::move(diff)) {
			create_path_geometry(s_d2d_factory);
			return true;
		}
		return false;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	// value	�i�[����l
	// anch	�}�`�̕���
	bool ShapePath::set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		if (ShapeStroke::set_anchor_pos(value, anch)) {
			create_path_geometry(s_d2d_factory);
			return true;
		}
		return false;
	}

	// ���̌`���Ɋi�[����.
	bool ShapePath::set_arrow_size(const ARROWHEAD_SIZE& value)
	{
		if (!equal(m_arrow_size, value)) {
			m_arrow_size = value;
			create_path_geometry(s_d2d_factory);
			return true;
		}
		return false;
	}

	// ���̌`���Ɋi�[����.
	bool ShapePath::set_arrow_style(const ARROWHEAD_STYLE value)
	{
		if (m_arrow_style != value) {
			m_arrow_style = value;
			create_path_geometry(s_d2d_factory);
			return true;
		}
		return false;
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	// value	�i�[����l
	bool ShapePath::set_start_pos(const D2D1_POINT_2F value)
	{
		if (ShapeStroke::set_start_pos(value)) {
			create_path_geometry(s_d2d_factory);
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// d_cnt	�����̐�
	// s_attr	����
	ShapePath::ShapePath(const size_t d_cnt, const ShapeSheet* s_attr, const bool closed) :
		ShapeLine::ShapeLine(d_cnt, s_attr, closed)
	{}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapePath::ShapePath(DataReader const& dt_reader) :
		ShapeLine::ShapeLine(dt_reader)
	{
		//m_arrow_style = static_cast<ARROWHEAD_STYLE>(dt_reader.ReadUInt32());
		//read(m_arrow_size, dt_reader);
		// �R���X�g���N�^�̒��ł� (�f�X�g���N�^�̒��ł�) ���z�֐��͖��Ӗ�.
		//create_path_geometry(s_d2d_factory);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapePath::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeLine::write(dt_writer);
		//dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
		//write(m_arrow_size, dt_writer);
	}

}