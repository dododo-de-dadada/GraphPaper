//------------------------------
// Shape_path.cpp
// �܂���̂ЂȌ^
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataWriter;

	// �h��Ԃ��F�𓾂�.
	// val	����ꂽ�l
	// �߂�l	����ꂽ�Ȃ� true
	bool ShapePath::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// ���������ړ�����.
	// d_vec	����
	bool ShapePath::move(const D2D1_POINT_2F d_vec) noexcept
	{
		if (ShapeLine::move(d_vec)) {
			m_d2d_path_geom = nullptr;
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
		if (ShapeLine::set_pos_anc(val, anc, limit, keep_aspect)) {
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	// ��邵�̌`���Ɋi�[����.
	bool ShapePath::set_arrow_size(const ARROW_SIZE& val) noexcept
	{
		if (!equal(m_arrow_size, val)) {
			m_arrow_size = val;
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	// ��邵�̌`���Ɋi�[����.
	bool ShapePath::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		if (m_arrow_style != val) {
			m_arrow_style = val;
			if (m_d2d_path_geom != nullptr) {
				m_d2d_path_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// �h��Ԃ��̐F�Ɋi�[����.
	bool ShapePath::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			m_d2d_path_geom = nullptr;
			m_d2d_arrow_geom = nullptr;
			return true;
		}
		return false;
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	// val	�i�[����l
	bool ShapePath::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		if (ShapeLine::set_pos_start(val)) {
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	ShapePath::ShapePath(const ShapePage& page, const DataReader& dt_reader) :
		ShapeLine::ShapeLine(page, dt_reader)
	{
		const D2D1_COLOR_F fill_color{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		};
		m_fill_color.r = min(max(fill_color.r, 0.0f), 1.0f);
		m_fill_color.g = min(max(fill_color.g, 0.0f), 1.0f);
		m_fill_color.b = min(max(fill_color.b, 0.0f), 1.0f);
		m_fill_color.a = min(max(fill_color.a, 0.0f), 1.0f);
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapePath::write(const DataWriter& dt_writer) const
	{
		ShapeLine::write(dt_writer);
		dt_writer.WriteSingle(m_fill_color.r);
		dt_writer.WriteSingle(m_fill_color.g);
		dt_writer.WriteSingle(m_fill_color.b);
		dt_writer.WriteSingle(m_fill_color.a);
	}

}