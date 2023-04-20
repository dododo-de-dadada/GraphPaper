//------------------------------
// Shape_path.cpp
// �܂���̂ЂȌ^
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �ߖT�̒��_��������.
	// pos	����ʒu
	// dd	�ߖT�Ƃ݂Ȃ����� (�̓��l), �����藣�ꂽ���_�͋ߖT�Ƃ݂͂Ȃ��Ȃ�.
	// val	����ʒu�̋ߖT�ɂ��钸�_
	// �߂�l	���������� true
	bool ShapePath::get_pos_nearest(const D2D1_POINT_2F p, double& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool done = false;
		D2D1_POINT_2F r;
		pt_sub(m_start, p, r);
		float r_abs = static_cast<float>(pt_abs2(r));
		if (r_abs < dd) {
			dd = r_abs;
			val = m_start;
			if (!done) {
				done = true;
			}
		}
		D2D1_POINT_2F q{ m_start };	// ���̓_
		for (const D2D1_POINT_2F pos : m_pos) {
			pt_add(q, pos, q);
			pt_sub(q, p, r);
			r_abs = static_cast<float>(pt_abs2(r));
			if (r_abs < dd) {
				dd = r_abs;
				val = q;
				if (!done) {
					done = true;
				}
			}
		}
		return done;
	}

	// �l��, �w�肵�����ʂ̓_�Ɋi�[����.
	bool ShapePath::set_pos_loc(
		const D2D1_POINT_2F val,	// �l
		const uint32_t loc,	// ����
		const float snap_point,	// 
		const bool /*keep_aspect*/
	) noexcept
	{
		bool flag = false;
		// �ύX���钸�_���ǂ̒��_�����肷��.
		const size_t d_cnt = m_pos.size();	// �����̐�
		if (loc >= LOC_TYPE::LOC_P0 && loc <= LOC_TYPE::LOC_P0 + d_cnt) {
			D2D1_POINT_2F p[N_GON_MAX];	// ���_�̈ʒu
			const size_t l_cnt = loc - LOC_TYPE::LOC_P0;	// �ύX����_�̓Y����
			// �ύX���钸�_�܂ł�, �e���_�̈ʒu�𓾂�.
			p[0] = m_start;
			for (size_t i = 0; i < l_cnt; i++) {
				pt_add(p[i], m_pos[i], p[i + 1]);
			}
			// �l����ύX�O�̈ʒu������, �ύX���鍷���𓾂�.
			D2D1_POINT_2F d;
			pt_sub(val, p[l_cnt], d);
			pt_round(d, PT_ROUND, d);
			// �����̒������[�����傫�������肷��.
			if (pt_abs2(d) >= FLT_MIN) {
				// �ύX���钸�_���ŏ��̒��_�����肷��.
				if (l_cnt == 0) {
					// �ŏ��̒��_�̈ʒu�ɕύX����������.
					pt_add(m_start, d, m_start);
				}
				else {
					// ���_�̒��O�̍����ɕύX����������.
					pt_add(m_pos[l_cnt - 1], d, m_pos[l_cnt - 1]);
				}
				// �ύX����̂��Ō�̒��_�ȊO�����肷��.
				if (l_cnt < d_cnt) {
					// ���̒��_�������Ȃ��悤��,
					// �ύX���钸�_�̎��̒��_�ւ̍�������ύX��������.
					pt_sub(m_pos[l_cnt], d, m_pos[l_cnt]);
				}
				if (!flag) {
					flag = true;
				}
			}
			// ���E�������[���łȂ������肷��.
			if (snap_point >= FLT_MIN) {
				// �c��̒��_�̈ʒu�𓾂�.
				for (size_t i = l_cnt; i < d_cnt; i++) {
					pt_add(p[i], m_pos[i], p[i + 1]);
				}
				const double dd = static_cast<double>(snap_point) * static_cast<double>(snap_point);
				for (size_t i = 0; i < d_cnt + 1; i++) {
					// ���_��, �ύX���钸�_�����肷��.
					if (i == l_cnt) {
						continue;
					}
					// ���_�ƕύX���钸�_�Ƃ̋��������E�����ȏォ���肷��.
					pt_sub(p[i], p[l_cnt], d);
					if (pt_abs2(d) >= dd) {
						continue;
					}
					// �ύX����̂��ŏ��̒��_�����肷��.
					if (l_cnt == 0) {
						pt_add(m_start, d, m_start);
					}
					else {
						pt_add(m_pos[l_cnt - 1], d, m_pos[l_cnt - 1]);
					}
					// �ύX����̂��Ō�̒��_�ȊO�����肷��.
					if (l_cnt < d_cnt) {
						pt_sub(m_pos[l_cnt], d, m_pos[l_cnt]);
					}
					if (!flag) {
						flag = true;
					}
					break;
				}
			}
		}
		if (flag) {
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
		}
		return flag;
	}

	// ���_�𓾂�.
	size_t ShapePath::get_verts(D2D1_POINT_2F p[]) const noexcept
	{
		const size_t p_cnt = m_pos.size();
		p[0] = m_start;
		for (size_t i = 0; i < p_cnt; i++) {
			p[i + 1].x = p[i].x + m_pos[i].x;
			p[i + 1].y = p[i].y + m_pos[i].y;
		}
		return p_cnt + 1;
	}

	// ���E��`�𓾂�.
	void ShapePath::get_bbox(
		const D2D1_POINT_2F a_lt,	// a_lt	���̗̈�̍���ʒu.
		const D2D1_POINT_2F a_rb,	// a_rb	���̗̈�̉E���ʒu.
		D2D1_POINT_2F& b_lt,	// b_lt	�͂ޗ̈�̍���ʒu.
		D2D1_POINT_2F& b_rb	// b_rb	�͂ޗ̈�̉E���ʒu.
	) const noexcept
	{
		b_lt.x = m_start.x < a_lt.x ? m_start.x : a_lt.x;
		b_lt.y = m_start.y < a_lt.y ? m_start.y : a_lt.y;
		b_rb.x = m_start.x > a_rb.x ? m_start.x : a_rb.x;
		b_rb.y = m_start.y > a_rb.y ? m_start.y : a_rb.y;
		const size_t d_cnt = m_pos.size();	// �����̐�
		D2D1_POINT_2F pos = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(pos, m_pos[i], pos);
			if (pos.x < b_lt.x) {
				b_lt.x = pos.x;
			}
			if (pos.x > b_rb.x) {
				b_rb.x = pos.x;
			}
			if (pos.y < b_lt.y) {
				b_lt.y = pos.y;
			}
			if (pos.y > b_rb.y) {
				b_rb.y = pos.y;
			}
		}
	}

	// ���E��`�̍���ʒu�𓾂�.
	void ShapePath::get_bbox_lt(D2D1_POINT_2F& val) const noexcept
	{
		const size_t p_cnt = m_pos.size();	// �ʒu�̐�
		D2D1_POINT_2F p = m_start;	// ���_
		D2D1_POINT_2F lt;	// ����ʒu
		lt = m_start;
		for (size_t i = 0; i < p_cnt; i++) {
			pt_add(p, m_pos[i], p);
			if (lt.x > p.x) {
				lt.x = p.x;
			}
			if (lt.y > p.y) {
				lt.y = p.y;
			}
		}
		val = lt;
	}


	// �w�肵�����ʂ̓_�𓾂�.
	void ShapePath::get_pos_loc(
		const uint32_t loc,	// ����
		D2D1_POINT_2F& val	// ����ꂽ�l
	) const noexcept
	{
		// �}�`�̕��ʂ��u�}�`�̊O���v�܂��́u�n�_�v�Ȃ��, �n�_�𓾂�.
		if (loc == LOC_TYPE::LOC_PAGE || loc == LOC_TYPE::LOC_P0) {
			val = m_start;
		}
		else if (loc > LOC_TYPE::LOC_P0) {
			const size_t  l_cnt = loc - LOC_TYPE::LOC_P0;
			if (l_cnt < m_pos.size() + 1) {
				val = m_start;
				for (size_t i = 0; i < l_cnt; i++) {
					pt_add(val, m_pos[i], val);
				}
			}
		}
	}

	// �h��Ԃ��F�𓾂�.
	// val	����ꂽ�l
	// �߂�l	����ꂽ�Ȃ� true
	bool ShapePath::get_fill_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_fill_color;
		return true;
	}

	// �ʒu���ړ�����.
	// pos	�ʒu�x�N�g��
	bool ShapePath::move(const D2D1_POINT_2F pos) noexcept
	{
		D2D1_POINT_2F start;
		pt_add(m_start, pos, start);
		if (set_pos_start(start)) {
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	// �h��Ԃ��F�Ɋi�[����.
	bool ShapePath::set_fill_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_fill_color, val)) {
			m_fill_color = val;
			return true;
		}
		return false;
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	// val	�i�[����l
	bool ShapePath::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F p;
		pt_round(val, PT_ROUND, p);
		if (!equal(m_start, p)) {
			m_start = p;
			m_d2d_arrow_geom = nullptr;
			m_d2d_path_geom = nullptr;
			return true;
		}
		return false;
	}

	ShapePath::ShapePath(const DataReader& dt_reader) :
		ShapeArrow(dt_reader)
	{
		m_start.x = dt_reader.ReadSingle();
		m_start.y = dt_reader.ReadSingle();
		const size_t vec_cnt = dt_reader.ReadUInt32();	// �v�f��
		m_pos.resize(vec_cnt);
		for (size_t i = 0; i < vec_cnt; i++) {
			m_pos[i].x = dt_reader.ReadSingle();
			m_pos[i].y = dt_reader.ReadSingle();
		}
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
		ShapeArrow::write(dt_writer);
		// �n�_
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);
		// ���̓_�ւ̈ʒu�x�N�g��
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_pos.size()));
		for (const D2D1_POINT_2F vec : m_pos) {
			dt_writer.WriteSingle(vec.x);
			dt_writer.WriteSingle(vec.y);
		}
		// �h��Ԃ��F
		dt_writer.WriteSingle(m_fill_color.r);
		dt_writer.WriteSingle(m_fill_color.g);
		dt_writer.WriteSingle(m_fill_color.b);
		dt_writer.WriteSingle(m_fill_color.a);
	}

}