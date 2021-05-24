//------------------------------
// Shape_path.cpp
// �܂���̂ЂȌ^
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �}�`��j������.
	ShapePath::~ShapePath(void)
	{
		m_d2d_path_geom = nullptr;
		m_d2d_arrow_geom = nullptr;
	}

	// ���̐��@�𓾂�.
	bool ShapePath::get_arrow_size(ARROWHEAD_SIZE& value) const noexcept
	{
		value = m_arrow_size;
		return true;
	}

	// ���̌`���𓾂�.
	bool ShapePath::get_arrow_style(ARROWHEAD_STYLE& value) const noexcept
	{
		value = m_arrow_style;
		return true;
	}

	// �ʒu��, �܂���̐}�`�̕��ʂ��܂ނ����肷��.
	// t_pos	���肷��ʒu
	// a_len	���ʂ̑傫��
	// p_cnt	�܂���̒��_�̐�
	// p_pos	�܂���̒��_
	// �߂�l	�ʒu���܂ސ}�`�̕���
	/*
	uint32_t ShapePath::hit_test_anchor(const D2D1_POINT_2F t_pos, const double a_len, const size_t p_cnt, D2D1_POINT_2F p_pos[], size_t& k) const noexcept
	{
		//const size_t m = m_diff.size() + 1;	// ���_�̐� (�����̐� + 1)
		size_t j = static_cast<size_t>(-1);	// �_���܂ޒ��_�̓Y����

		// ���肷��ʒu�����_�ƂȂ�悤���s�ړ������l�ւ�`�̊e���_�𓾂�.
		k = 0;
		pt_sub(m_pos, t_pos, p_pos[k++]);
		if (pt_in_anch(p_pos[0], a_len)) {
			j = 0;
		}
		for (size_t i = 1; i < p_cnt; i++) {
			pt_add(p_pos[k - 1], m_diff[i - 1], p_pos[k]);
			if (pt_in_anch(p_pos[i], a_len)) {
				j = i;
			}
			if (pt_abs2(m_diff[i - 1]) > FLT_MIN) {
				k++;
			}
		}
		if (j != -1) {
			const auto anch = ANCH_TYPE::ANCH_P0 + j;
			return static_cast<uint32_t>(anch);
		}
		return ANCH_TYPE::ANCH_SHEET;
	}
	*/

	// ���������ړ�����.
	// diff	����
	void ShapePath::move(const D2D1_POINT_2F diff)
	{
		ShapeStroke::move(diff);
		create_path_geometry(s_d2d_factory);
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	// value	�i�[����l
	// anch	�}�`�̕���
	void ShapePath::set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		// �}�`�̕��ʂ����_�ȊO�����肷��.
		const size_t d_cnt = m_diff.size();	// �����̐�
		if (anch < ANCH_TYPE::ANCH_P0 || anch > ANCH_TYPE::ANCH_P0 + d_cnt) {
			return;
		}
		// �}�`�̕��ʂ��n�_�����肷��.
		if (anch == ANCH_TYPE::ANCH_P0) {
			D2D1_POINT_2F diff;
			pt_sub(value, m_pos, diff);
			pt_round(diff, PT_ROUND, diff);
			pt_add(m_pos, diff, m_pos);
			pt_sub(m_diff[0], diff, m_diff[0]);
		}
		else {
			D2D1_POINT_2F a_pos;
			get_anch_pos(anch, a_pos);
			D2D1_POINT_2F diff;
			pt_sub(value, a_pos, diff);
			pt_round(diff, PT_ROUND, diff);
			const size_t i = anch - ANCH_TYPE::ANCH_P0;
			pt_add(m_diff[i - 1], diff, m_diff[i - 1]);
			// �}�`�̕��ʂ��I�_�ȊO�����肷��.
			if (anch != ANCH_TYPE::ANCH_P0 + d_cnt) {
				pt_sub(m_diff[i], diff, m_diff[i]);
			}
		}
		create_path_geometry(s_d2d_factory);
	}

	// ���̌`���Ɋi�[����.
	void ShapePath::set_arrow_size(const ARROWHEAD_SIZE& value)
	{
		if (!equal(m_arrow_size, value)) {
			m_arrow_size = value;
			create_path_geometry(s_d2d_factory);
		}
	}

	// ���̌`���Ɋi�[����.
	void ShapePath::set_arrow_style(const ARROWHEAD_STYLE value)
	{
		if (m_arrow_style != value) {
			m_arrow_style = value;
			create_path_geometry(s_d2d_factory);
		}
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	// value	�i�[����l
	void ShapePath::set_start_pos(const D2D1_POINT_2F value)
	{
		ShapeStroke::set_start_pos(value);
		create_path_geometry(s_d2d_factory);
	}

	// �}�`���쐬����.
	// d_cnt	�����̐�
	// s_attr	����
	ShapePath::ShapePath(const size_t d_cnt, const ShapeSheet* s_attr) :
		ShapeStroke(d_cnt, s_attr),
		m_arrow_style(s_attr->m_arrow_style),
		m_arrow_size(s_attr->m_arrow_size)
	{}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapePath::ShapePath(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		m_arrow_style = static_cast<ARROWHEAD_STYLE>(dt_reader.ReadUInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
		// �R���X�g���N�^�̒��ł� (�f�X�g���N�^�̒��ł�) ���z�֐��͖��Ӗ�.
		//create_path_geometry(s_d2d_factory);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapePath::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
	}

}