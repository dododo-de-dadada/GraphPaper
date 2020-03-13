#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �܂���̒��_�̔z��
	constexpr ANCH_WHICH ANCH_QUAD[4]{
		ANCH_WHICH::ANCH_R_SE,
		ANCH_WHICH::ANCH_R_SW,
		ANCH_WHICH::ANCH_R_NE,
		ANCH_WHICH::ANCH_R_NW
	};

	// �l�ւ�`�̊e�ӂ̖@���x�N�g���𓾂�.
	static bool qd_get_nor(const D2D1_POINT_2F q_pos[4], D2D1_POINT_2F n_vec[4]) noexcept;
	// ���s����x�N�g���𓾂�.
	static D2D1_POINT_2F qd_orth(const D2D1_POINT_2F v) { return { -v.y, v.x }; }
	// �l�ւ�`�̕ӂ��ʒu���܂ނ����ׂ�.
	static bool qd_test_expanded(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_pos[], const D2D1_POINT_2F n_vec[], const double exp, D2D1_POINT_2F q_exp[][4]) noexcept;
	// �l�ւ�`�̊p���ʒu���܂ނ����ׂ�.
	static bool qd_test_extended(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F exp[4][4], const D2D1_POINT_2F n_vec[4], const double ext) noexcept;

	// �l�ւ�`�̊e�ӂ̖@���x�N�g���𓾂�.
	// q_pos	�l�ӌ`
	// q_nor	����ꂽ�e�ӂ̖@���x�N�g��.
	// �߂�l	�@���x�N�g���𓾂��Ȃ� true, ���ׂĂ̒��_���d�Ȃ��Ă����Ȃ� false
	static bool qd_get_nor(const D2D1_POINT_2F q_pos[4], D2D1_POINT_2F q_nor[4]) noexcept
	{
		// �l�ӌ`�̊e�ӂ̒����Ɩ@���x�N�g��, 
		// �d�����Ȃ����_�̐������߂�.
		double q_len[4];
		int q_cnt = 1;
		for (int i = 0; i < 4; i++) {
			// ���̒��_�Ƃ̍��������߂�.
			D2D1_POINT_2F q_sub;
			pt_sub(q_pos[(i + 1) & 3], q_pos[i], q_sub);
			// �����̒��������߂�.
			q_len[i] = sqrt(pt_abs2(q_sub));
			if (q_len[i] > FLT_MIN) {
				// �����̒����� 0 ���傫���Ȃ�, 
				// �d�����Ȃ����_�̐����C���N�������g����.
				q_cnt++;
			}
			// �����ƒ��s����x�N�g���𐳋K�����Ė@���x�N�g���Ɋi�[����.
			pt_scale(qd_orth(q_sub), 1.0 / q_len[i], q_nor[i]);
		}
		if (q_cnt == 1) {
			// ���ׂĂ̒��_���d�Ȃ����Ȃ�, false ��Ԃ�.
			return false;
		}
		for (int i = 0; i < 4; i++) {
			if (q_len[i] <= FLT_MIN) {
				// �ӂ̒����� 0 �Ȃ��,
				// �ӂɗאڂ���O��̕ӂ̒�����
				// ������ 0 �łȂ��ӂ�T��,
				// �����̖@���x�N�g����������, 
				// ���� 0 �̕ӂ̖@���x�N�g���Ƃ���.
				int prev;
				for (int j = 1; q_len[prev = ((i - j) & 3)] < FLT_MIN; j++);
				int next;
				for (int j = 1; q_len[next = ((i + j) & 3)] < FLT_MIN; j++);
				pt_add(q_nor[prev], q_nor[next], q_nor[i]);
				auto len = sqrt(pt_abs2(q_nor[i]));
				if (len > FLT_MIN) {
					pt_scale(q_nor[i], 1.0 / len, q_nor[i]);
					continue;
				}
				// �����x�N�g�����[���x�N�g���ɂȂ�Ȃ�,
				// ��������x�N�g����@���x�N�g���Ƃ���.
				q_nor[i] = qd_orth(q_nor[prev]);
			}
		}
		return true;
	}

	// �l�ւ�`�̕ӂ��ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// q_pos	�l�ւ�`
	// q_nor	�l�ւ�`�̊e�ӂ̖@���x�N�g��
	// exp	�ӂ̑���
	// q_exp	�������g�������e��
	static bool qd_test_expanded(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_pos[], const D2D1_POINT_2F q_nor[], const double exp, D2D1_POINT_2F q_exp[][4]) noexcept
	{
		for (int i = 0; i < 4; i++) {
			// �ӂ̕Е��̒[�_��@���x�N�g���ɂ�����
			// �����̔��������ړ������ʒu��
			// �g�������ӂɊi�[����.
			D2D1_POINT_2F nor;
			pt_scale(q_nor[i], exp, nor);
			pt_add(q_pos[i], nor, q_exp[i][0]);
			// �t�����ɂ��ړ���, �g�������ӂɊi�[����.
			pt_sub(q_pos[i], nor, q_exp[i][1]);
			// �ӂ̂�������̒[�_��@���x�N�g���ɂ�����
			// �����̔��������ړ������ʒu��
			// �g�������ӂɊi�[����.
			const auto j = (i + 1) & 3;
			pt_sub(q_pos[j], nor, q_exp[i][2]);
			// �t�����ɂ��ړ���, �g�������ӂɊi�[����.
			pt_add(q_pos[j], nor, q_exp[i][3]);
			// �ʒu���g�������ӂɊ܂܂�邩���ׂ�.
			if (pt_in_quad(t_pos, q_exp[i])) {
				// �܂܂��Ȃ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// �l�ւ�`�̊p���ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// q_exp	�l�ւ�`�̊g�����ꂽ�e��
	// q_nor	�l�ւ�`�̊e�ӂ̖@���x�N�g��
	// ext	�p���������钷��
	static bool qd_test_extended(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_exp[4][4], const D2D1_POINT_2F q_nor[4], const double ext) noexcept
	{
		for (int i = 0, j = 3; i < 4; j = i++) {
			D2D1_POINT_2F q_ext[4];	// ����������
			D2D1_POINT_2F vec;	// ���s�ȃx�N�g��
			// ���钸�_�ɗאڂ���ӂɂ���.
			// �g�������ӂ̒[��, ���������ӂ̒[�Ɋi�[����.
			q_ext[0] = q_exp[j][3];
			q_ext[1] = q_exp[j][2];
			// �@���x�N�g���ƒ��s����x�N�g����,
			// �������钷���̕������{��,
			// ���s�ȃx�N�g���Ɋi�[����.
			pt_scale(qd_orth(q_nor[j]), ext, vec);
			// �i�[�����ʒu�𕽍s�ȃx�N�g���ɉ����ĉ�����,
			// ���������ӂ̂�������̒[�Ɋi�[����.
			pt_sub(q_ext[1], vec, q_ext[2]);
			pt_sub(q_ext[0], vec, q_ext[3]);
			// �ʒu�����������ӂɊ܂܂�邩���ׂ�.
			if (pt_in_quad(t_pos, q_ext) == false) {
				// �܂܂�Ȃ��Ȃ�p������.
				continue;
			}
			// �אڂ�������Е��̕ӂɂ���.
			// �g�������ӂ̒[��, ���������ӂ̒[�Ɋi�[����.
			q_ext[2] = q_exp[i][1];
			q_ext[3] = q_exp[i][0];
			// �@���x�N�g���ƒ��s����x�N�g�� (��قǂƂ͋t����) �𓾂�,
			// �p���������钷���̕������{��,
			// ���s�ȃx�N�g���Ɋi�[����.
			pt_scale(qd_orth(q_nor[i]), ext, vec);
			// �i�[�����ʒu�𕽍s�ȃx�N�g���ɉ����ĉ�����,
			// ���������ӂ̂�������̒[�Ɋi�[����.
			pt_add(q_ext[3], vec, q_ext[0]);
			pt_add(q_ext[2], vec, q_ext[1]);
			// �ʒu�����������ӂɊ܂܂�邩���ׂ�.
			if (pt_in_quad(t_pos, q_ext)) {
				// �܂܂��Ȃ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// �p�X�W�I���g�����쐬����.
	void ShapeQuad::create_path_geometry(void)
	{
		D2D1_POINT_2F q_pos[4];

		m_poly_geom = nullptr;
		q_pos[0] = m_pos;
		pt_add(q_pos[0], m_diff, q_pos[1]);
		pt_add(q_pos[1], m_diff_1, q_pos[2]);
		pt_add(q_pos[2], m_diff_2, q_pos[3]);
		winrt::com_ptr<ID2D1GeometrySink> sink;
		winrt::check_hresult(
			s_d2d_factory->CreatePathGeometry(m_poly_geom.put())
		);
		m_poly_geom->Open(sink.put());
		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		const auto figure_begin = is_opaque(m_fill_color)
			? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
			: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
		sink->BeginFigure(q_pos[0], figure_begin);
		sink->AddLine(q_pos[1]);
		sink->AddLine(q_pos[2]);
		sink->AddLine(q_pos[3]);
		// Shape ��Ŏn�_�ƏI�_���d�˂��Ƃ�,
		// �p�X�Ɏn�_�������Ȃ���, LINE_JOINT ���ւ�Ȃ��ƂɂȂ�.
		sink->AddLine(q_pos[0]);
		sink->EndFigure(D2D1_FIGURE_END_CLOSED);
		sink->Close();
		sink = nullptr;
	}

	// �}�`��\������.
	// dx	�}�`�̕`���
	void ShapeQuad::draw(SHAPE_DX& dx)
	{
		if (is_opaque(m_fill_color)) {
			dx.m_shape_brush->SetColor(m_fill_color);
			dx.m_d2dContext->FillGeometry(m_poly_geom.get(), dx.m_shape_brush.get(), nullptr);
		}
		if (is_opaque(m_stroke_color)) {
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawGeometry(
				m_poly_geom.get(),
				dx.m_shape_brush.get(),
				static_cast<FLOAT>(m_stroke_width),
				m_d2d_stroke_style.get());
		}
		if (is_selected() == false) {
			return;
		}
		anchor_draw_rect(m_pos, dx);
		D2D1_POINT_2F a_pos;
		pt_add(m_pos, m_diff, a_pos);
		anchor_draw_rect(a_pos, dx);
		pt_add(a_pos, m_diff_1, a_pos);
		anchor_draw_rect(a_pos, dx);
		pt_add(a_pos, m_diff_2, a_pos);
		anchor_draw_rect(a_pos, dx);
	}

	// �h��Ԃ��F�𓾂�.
	// val	����ꂽ�l
	// �߂�l	����ꂽ�Ȃ� true
	bool ShapeQuad::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// �ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	ANCH_WHICH ShapeQuad::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		constexpr D2D1_POINT_2F ZP{ 0.0f, 0.0f };
		// ���ׂ�ʒu�����_�ƂȂ�悤���s�ړ������l�ւ�`�̊e���_�𓾂�.
		D2D1_POINT_2F q_pos[4];
		pt_sub(m_pos, t_pos, q_pos[3]);
		pt_add(q_pos[3], m_diff, q_pos[2]);
		pt_add(q_pos[2], m_diff_1, q_pos[1]);
		pt_add(q_pos[1], m_diff_2, q_pos[0]);
		for (int i = 0; i < 4; i++) {
			// �ʒu��, �l�ւ�`�̊e���� (���_) �Ɋ܂܂�邩���ׂ�.
			if (pt_in_anch(q_pos[i], a_len)) {
				// �܂܂��Ȃ�, �Y�����镔�ʂ�Ԃ�.
				return ANCH_QUAD[i];
			}
		}
		if (is_opaque(m_stroke_color)) {
			// �ӂ��s�����Ȃ�,
			// ���̑��������Ƃ�, �ӂ��g�����鑾�����v�Z����.
			const auto exp = max(max(m_stroke_width, a_len) * 0.5, 0.5);
			// �e�ӂ̖@���x�N�g���𓾂�.
			D2D1_POINT_2F q_nor[4];
			if (qd_get_nor(q_pos, q_nor) == false) {
				// �@���x�N�g�����Ȃ� (���ׂĂ̒��_���d�Ȃ��Ă���) �Ȃ�,
				// �ʒu��, �g�����鑾���𔼌a�Ƃ���~�Ɋ܂܂�邩���ׂ�.
				if (pt_abs2(q_pos[0]) <= exp * exp) {
					// �܂܂��Ȃ� ANCH_FRAME ��Ԃ�.
					return ANCH_WHICH::ANCH_FRAME;
				}
				// �����łȂ���� ANCH_NONE ��Ԃ�.
				return ANCH_WHICH::ANCH_OUTSIDE;
			}
			// �l�ӌ`�̕ӂ��ʒu���܂ނ����ׂ�.
			D2D1_POINT_2F q_exp[4][4];
			if (qd_test_expanded(ZP, q_pos, q_nor, exp, q_exp)) {
				// �܂ނȂ� ANCH_FRAME ��Ԃ�.
				return ANCH_WHICH::ANCH_FRAME;
			}
			// �l�ӌ`�̊p���ʒu���܂ނ����ׂ�.
			// �p���������钷���͕ӂ̑����� 5 �{.
			// ���������, D2D �̕`��ƈ�v����.
			const auto ext = m_stroke_width * 5.0;
			if (qd_test_extended(ZP, q_exp, q_nor, ext)) {
				// �܂ނȂ� ANCH_FRAME ��Ԃ�.
				return ANCH_WHICH::ANCH_FRAME;
			}
		}
		// �ӂ��s����, �܂��͈ʒu���ӂɊ܂܂�Ă��Ȃ��Ȃ�,
		// �h��Ԃ��F���s���������ׂ�.
		if (is_opaque(m_fill_color)) {
			// �s�����Ȃ�, �ʒu���l�ւ�`�Ɋ܂܂�邩���ׂ�.
			if (pt_in_quad(ZP, q_pos)) {
				// �܂܂��Ȃ� ANCH_INSIDE ��Ԃ�.
				return ANCH_WHICH::ANCH_INSIDE;
			}
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// �͈͂Ɋ܂܂�邩���ׂ�.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeQuad::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		D2D1_POINT_2F e_pos;	// ���̈ʒu
		if (pt_in_rect(m_pos, a_min, a_max)) {
			pt_add(m_pos, m_diff, e_pos);
			if (pt_in_rect(e_pos, a_min, a_max)) {
				pt_add(e_pos, m_diff_1, e_pos);
				if (pt_in_rect(e_pos, a_min, a_max)) {
					pt_add(e_pos, m_diff_2, e_pos);
					return pt_in_rect(e_pos, a_min, a_max);
				}
			}
		}
		return false;
	}

	// �h��Ԃ��̐F�Ɋi�[����.
	void ShapeQuad::set_fill_color(const D2D1_COLOR_F& value) noexcept
	{
		if (equal(m_fill_color, value)) {
			return;
		}
		m_fill_color = value;
		create_path_geometry();
	}

	// �}�`���쐬����.
	// s_pos	�J�n�ʒu
	// d_pos	�I���ʒu�ւ̍���
	// attr	����̑����l
	ShapeQuad::ShapeQuad(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_pos, const ShapePanel* attr) :
		ShapePoly::ShapePoly(attr)
	{
		m_pos.x = static_cast<FLOAT>(s_pos.x + 0.5 * d_pos.x);
		m_pos.y = s_pos.y;
		pt_scale(d_pos, 0.5, m_diff);
		m_diff_1.x = -m_diff.x;
		m_diff_1.y = m_diff.y;
		m_diff_2.x = m_diff_1.x;
		m_diff_2.y = -m_diff.y;
		m_fill_color = attr->m_fill_color;
		create_path_geometry();
		D2D1_POINT_2F q_pos[4];
		q_pos[0] = { 0.0f, 0.0f };
		q_pos[1] = m_diff;
		q_pos[2] = m_diff_1;
		q_pos[3] = m_diff_2;
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�f�[�^���[�_�[
	ShapeQuad::ShapeQuad(DataReader const& dt_reader) :
		ShapePoly::ShapePoly(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_fill_color, dt_reader);
		create_path_geometry();
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeQuad::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapePoly::write(dt_writer);
		write(m_fill_color, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeQuad::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg(m_pos, "M", dt_writer);
		write_svg(m_diff, "l", dt_writer);
		write_svg(m_diff_1, "l", dt_writer);
		write_svg(m_diff_2, "l", dt_writer);
		write_svg("Z\" ", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
	}

}