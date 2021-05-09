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
		m_poly_geom = nullptr;
	}

	// �}�`���͂ޗ̈�𓾂�.
	// b_min	�̈�̍���ʒu.
	// b_max	�̈�̉E���ʒu.
	void ShapePath::get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		const size_t n = m_diff.size();	// �����̐�
		D2D1_POINT_2F e_pos = m_pos;
		pt_inc(e_pos, b_min, b_max);
		for (size_t i = 0; i < n; i++) {
			pt_add(e_pos, m_diff[i], e_pos);
			pt_inc(e_pos, b_min, b_max);
		}
	}

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	// value	�̈�̍���ʒu
	void ShapePath::get_min_pos(D2D1_POINT_2F& value) const noexcept
	{
		const size_t n = m_diff.size();	// �����̐�
		D2D1_POINT_2F v_pos = m_pos;	// ���_�̈ʒu
		value = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(v_pos, m_diff[i], v_pos);
			pt_min(value, v_pos, value);
		}
	}

	// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
	// anch	�}�`�̕���
	// value	���ʂ̈ʒu
	void ShapePath::get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		if (anch == ANCH_WHICH::ANCH_OUTSIDE || anch == ANCH_WHICH::ANCH_P0) {
			// �}�`�̕��ʂ��u�O���v�܂��́u�J�n�_�v�Ȃ��, �J�n�ʒu�𓾂�.
			value = m_pos;
		}
		else if (anch > ANCH_WHICH::ANCH_P0) {
			const size_t m = m_diff.size() + 1;		// ���_�̐� (�����̐� + 1)
			if (anch < ANCH_WHICH::ANCH_P0 + m) {
				value = m_pos;
				for (size_t i = 0; i < anch - ANCH_WHICH::ANCH_P0; i++) {
					pt_add(value, m_diff[i], value);
				}
			}
		}
	}

	// ���������ړ�����.
	// diff	����
	void ShapePath::move(const D2D1_POINT_2F diff)
	{
		ShapeStroke::move(diff);
		create_path_geometry();
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	// value	�i�[����l
	// anch	�}�`�̕���
	void ShapePath::set_anch_pos(const D2D1_POINT_2F value, const uint32_t anch)
	{
		D2D1_POINT_2F a_pos;
		D2D1_POINT_2F diff;

		if (anch == ANCH_WHICH::ANCH_OUTSIDE) {
			m_pos = value;
		}
		else if (anch == ANCH_WHICH::ANCH_P0) {
			pt_sub(value, m_pos, diff);
			m_pos = value;
			pt_sub(m_diff[0], diff, m_diff[0]);
		}
		else {
			const size_t n = m_diff.size();	// �����̐�
			if (anch == ANCH_WHICH::ANCH_P0 + n) {
				get_anch_pos(anch, a_pos);
				pt_sub(value, a_pos, diff);
				pt_add(m_diff[n - 1], diff, m_diff[n - 1]);
			}
			else if (anch > ANCH_WHICH::ANCH_P0 && anch < ANCH_WHICH::ANCH_P0 + n) {
				get_anch_pos(anch, a_pos);
				pt_sub(value, a_pos, diff);
				const size_t i = anch - ANCH_WHICH::ANCH_P0;
				pt_add(m_diff[i - 1], diff, m_diff[i - 1]);
				pt_sub(m_diff[i], diff, m_diff[i]);
			}
			else {
				return;
			}
		}
		create_path_geometry();
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	// value	�i�[����l
	void ShapePath::set_start_pos(const D2D1_POINT_2F value)
	{
		ShapeStroke::set_start_pos(value);
		create_path_geometry();
	}

	// �}�`���쐬����.
	// n	�p��
	// attr	�����l
	ShapePath::ShapePath(const uint32_t n, const ShapeSheet* attr) :
		ShapeStroke::ShapeStroke(n, attr)
	{}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapePath::ShapePath(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_diff[1], dt_reader);
		read(m_diff[2], dt_reader);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapePath::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeStroke::write(dt_writer);
		write(m_diff[1], dt_writer);
		write(m_diff[2], dt_writer);
	}

}