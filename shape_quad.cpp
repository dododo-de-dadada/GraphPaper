#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �܂���̒��_�̔z��
	constexpr uint32_t ANCH_QUAD[4]{
		ANCH_WHICH::ANCH_P3,//ANCH_R_SE,
		ANCH_WHICH::ANCH_P2,//ANCH_R_SW,
		ANCH_WHICH::ANCH_P1,//ANCH_R_NE
		ANCH_WHICH::ANCH_P0//ANCH_R_NW
	};

	// �l�ւ�`�̊e�ӂ̖@���x�N�g���𓾂�.
	static bool qd_get_nor(const size_t n, const D2D1_POINT_2F q_pos[], D2D1_POINT_2F n_vec[]) noexcept;
	// ���s����x�N�g���𓾂�.
	static D2D1_POINT_2F qd_orth(const D2D1_POINT_2F v) { return { -v.y, v.x }; }
	// �l�ւ�`�̕ӂ��ʒu���܂ނ����ׂ�.
	static bool qd_test_expanded(const size_t n, const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_pos[], const D2D1_POINT_2F n_vec[], const double exp, D2D1_POINT_2F q_exp[]) noexcept;
	// �l�ւ�`�̊p���ʒu���܂ނ����ׂ�.
	static bool qd_test_extended(const size_t n, const D2D1_POINT_2F t_pos, const D2D1_POINT_2F exp[], const D2D1_POINT_2F n_vec[], const double ext) noexcept;

	//	���p�`�̊e�ӂ̖@���x�N�g���𓾂�.
	//	n	���_�̐�
	//	q_pos	���p�`
	//	q_nor	����ꂽ�e�ӂ̖@���x�N�g��.
	//	�߂�l	�@���x�N�g���𓾂��Ȃ� true, ���ׂĂ̒��_���d�Ȃ��Ă����Ȃ� false
	static bool qd_get_nor(const size_t n, const D2D1_POINT_2F q_pos[], D2D1_POINT_2F q_nor[]) noexcept
	{
		// ���p�`�̊e�ӂ̒����Ɩ@���x�N�g��, 
		// �d�����Ȃ����_�̐������߂�.
		std::vector<double> q_len(n);
		int q_cnt = 1;
		for (size_t i = 0; i < n; i++) {
			// ���̒��_�Ƃ̍��������߂�.
			D2D1_POINT_2F q_sub;
			pt_sub(q_pos[(i + 1) % n], q_pos[i], q_sub);
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
		for (size_t i = 0; i < n; i++) {
			if (q_len[i] <= FLT_MIN) {
				// �ӂ̒����� 0 �Ȃ��,
				// �ӂɗאڂ���O��̕ӂ̒�����
				// ������ 0 �łȂ��ӂ�T��,
				// �����̖@���x�N�g����������, 
				// ���� 0 �̕ӂ̖@���x�N�g���Ƃ���.
				size_t prev;
				for (size_t j = 1; q_len[prev = ((i - j) % n)] < FLT_MIN; j++);
				size_t next;
				for (size_t j = 1; q_len[next = ((i + j) % n)] < FLT_MIN; j++);
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

	//	���p�`�̕ӂ��ʒu���܂ނ����ׂ�.
	//	n	���_�̐�
	//	t_pos	���ׂ�ʒu
	//	q_pos	���p�`
	//	q_nor	���p�`�̊e�ӂ̖@���x�N�g��
	//	exp	�ӂ̑���
	//	q_exp	�������g�������e��
	static bool qd_test_expanded(const size_t n, const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_pos[], const D2D1_POINT_2F q_nor[], const double exp, D2D1_POINT_2F q_exp[]) noexcept
	{
		for (size_t i = 0; i < n; i++) {
			// �ӂ̕Е��̒[�_��@���x�N�g���ɂ�����
			// �����̔��������ړ������ʒu��
			// �g�������ӂɊi�[����.
			D2D1_POINT_2F nor;
			pt_scale(q_nor[i], exp, nor);
			pt_add(q_pos[i], nor, q_exp[n * i + 0]);
			// �t�����ɂ��ړ���, �g�������ӂɊi�[����.
			pt_sub(q_pos[i], nor, q_exp[n * i + 1]);
			// �ӂ̂�������̒[�_��@���x�N�g���ɂ�����
			// �����̔��������ړ������ʒu��
			// �g�������ӂɊi�[����.
			const auto j = (i + 1) % 4;
			pt_sub(q_pos[j], nor, q_exp[n * i + 2]);
			// �t�����ɂ��ړ���, �g�������ӂɊi�[����.
			pt_add(q_pos[j], nor, q_exp[n * i + 3]);
			// �ʒu���g�������ӂɊ܂܂�邩���ׂ�.
			if (pt_in_quad(n, t_pos, q_exp + n * i)) {
				// �܂܂��Ȃ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// �l�ւ�`�̊p���ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// q_exp	�l�ւ�`�̊g�����ꂽ�e�� n �~ n
	// q_nor	�l�ւ�`�̊e�ӂ̖@���x�N�g�� n
	// ext	�p���������钷��
	static bool qd_test_extended(const size_t n, const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_exp[], const D2D1_POINT_2F q_nor[], const double ext) noexcept
	{
		std::vector<D2D1_POINT_2F> q_ext(n);	// ����������
		D2D1_POINT_2F vec;	// ���s�ȃx�N�g��

		for (size_t i = 0, j = n - 1; i < n; j = i++) {
			// ���钸�_�ɗאڂ���ӂɂ���.
			// �g�������ӂ̒[��, ���������ӂ̒[�Ɋi�[����.
			q_ext[0] = q_exp[n * j + 3];
			q_ext[1] = q_exp[n * j + 2];
			// �@���x�N�g���ƒ��s����x�N�g����,
			// �������钷���̕������{��,
			// ���s�ȃx�N�g���Ɋi�[����.
			pt_scale(qd_orth(q_nor[j]), ext, vec);
			// �i�[�����ʒu�𕽍s�ȃx�N�g���ɉ����ĉ�����,
			// ���������ӂ̂�������̒[�Ɋi�[����.
			pt_sub(q_ext[1], vec, q_ext[2]);
			pt_sub(q_ext[0], vec, q_ext[3]);
			// �ʒu�����������ӂɊ܂܂�邩���ׂ�.
			if (pt_in_quad(n, t_pos, q_ext.data()) != true) {
				// �܂܂�Ȃ��Ȃ�p������.
				continue;
			}
			// �אڂ�������Е��̕ӂɂ���.
			// �g�������ӂ̒[��, ���������ӂ̒[�Ɋi�[����.
			q_ext[2] = q_exp[n * i + 1];
			q_ext[3] = q_exp[n * i + 0];
			// �@���x�N�g���ƒ��s����x�N�g�� (��قǂƂ͋t����) �𓾂�,
			// �p���������钷���̕������{��,
			// ���s�ȃx�N�g���Ɋi�[����.
			pt_scale(qd_orth(q_nor[i]), ext, vec);
			// �i�[�����ʒu�𕽍s�ȃx�N�g���ɉ����ĉ�����,
			// ���������ӂ̂�������̒[�Ɋi�[����.
			pt_add(q_ext[3], vec, q_ext[0]);
			pt_add(q_ext[2], vec, q_ext[1]);
			// �ʒu�����������ӂɊ܂܂�邩���ׂ�.
			if (pt_in_quad(n, t_pos, q_ext.data())) {
				// �܂܂��Ȃ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// �p�X�W�I���g�����쐬����.
	void ShapeQuad::create_path_geometry(void)
	{
		const size_t n = m_diff.size() + 1;
		std::vector<D2D1_POINT_2F> q_pos(n);

		m_poly_geom = nullptr;
		q_pos[0] = m_pos;
		for (size_t i = 1; i < n; i++) {
			pt_add(q_pos[i - 1], m_diff[i - 1], q_pos[i]);
		}
		/*
		pt_add(q_pos[0], m_diff[0], q_pos[1]);
		pt_add(q_pos[1], m_diff[1], q_pos[2]);
		pt_add(q_pos[2], m_diff[2], q_pos[3]);
		*/
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
		for (size_t i = 1; i < n; i++) {
			sink->AddLine(q_pos[i]);
		}
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
		if (is_selected() != true) {
			return;
		}
		anchor_draw_rect(m_pos, dx);
		D2D1_POINT_2F a_pos;
		pt_add(m_pos, m_diff[0], a_pos);
		anchor_draw_rect(a_pos, dx);
		const size_t n = m_diff.size();
		for (size_t i = 1; i < n; i++) {
			pt_add(a_pos, m_diff[i], a_pos);
			anchor_draw_rect(a_pos, dx);
		}
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
	uint32_t ShapeQuad::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const size_t n = m_diff.size() + 1;
		constexpr D2D1_POINT_2F ZP{ 0.0f, 0.0f };
		// ���ׂ�ʒu�����_�ƂȂ�悤���s�ړ������l�ւ�`�̊e���_�𓾂�.
		std::vector<D2D1_POINT_2F> q_pos(n);
		pt_sub(m_pos, t_pos, q_pos[n - 1]);
		for (size_t i = 1; i < n; i++) {
			pt_add(q_pos[n - i], m_diff[i - 1], q_pos[n - 1 - i]);
		}
		//pt_add(q_pos[3], m_diff[0], q_pos[2]);
		//pt_add(q_pos[2], m_diff[1], q_pos[1]);
		//pt_add(q_pos[1], m_diff[2], q_pos[0]);
		for (size_t i = 0; i < n; i++) {
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
			std::vector<D2D1_POINT_2F> q_nor(n);
			//D2D1_POINT_2F q_nor[4];
			if (qd_get_nor(n, q_pos.data(), q_nor.data()) != true) {
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
			std::vector<D2D1_POINT_2F> q_exp(n * n);
			if (qd_test_expanded(n, ZP, q_pos.data(), q_nor.data(), exp, q_exp.data())) {
				// �܂ނȂ� ANCH_FRAME ��Ԃ�.
				return ANCH_WHICH::ANCH_FRAME;
			}
			// �l�ӌ`�̊p���ʒu���܂ނ����ׂ�.
			// �p���������钷���͕ӂ̑����� 5 �{.
			// ���������, D2D �̕`��ƈ�v����.
			const auto ext = m_stroke_width * 5.0;
			if (qd_test_extended(n, ZP, q_exp.data(), q_nor.data(), ext)) {
				// �܂ނȂ� ANCH_FRAME ��Ԃ�.
				return ANCH_WHICH::ANCH_FRAME;
			}
		}
		// �ӂ��s����, �܂��͈ʒu���ӂɊ܂܂�Ă��Ȃ��Ȃ�,
		// �h��Ԃ��F���s���������ׂ�.
		if (is_opaque(m_fill_color)) {
			// �s�����Ȃ�, �ʒu���l�ւ�`�Ɋ܂܂�邩���ׂ�.
			if (pt_in_quad(n, ZP, q_pos.data())) {
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
		if (!pt_in_rect(m_pos, a_min, a_max)) {
			return false;
		}
		const size_t n = m_diff.size();
		D2D1_POINT_2F e_pos = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(e_pos, m_diff[i], e_pos);	// ���̈ʒu
			if (!pt_in_rect(e_pos, a_min, a_max)) {
				return false;
			}
		}
		return true;
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
	// diff	�I���ʒu�ւ̍���
	// attr	����̑����l
	ShapeQuad::ShapeQuad(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeSheet* attr) :
		ShapePoly::ShapePoly(3, attr)
	{
		m_pos.x = static_cast<FLOAT>(s_pos.x + 0.5 * diff.x);
		m_pos.y = s_pos.y;
		pt_scale(diff, 0.5, m_diff[0]);
		m_diff[1].x = -m_diff[0].x;
		m_diff[1].y = m_diff[0].y;
		m_diff[2].x = m_diff[1].x;
		m_diff[2].y = -m_diff[0].y;
		m_fill_color = attr->m_fill_color;
		create_path_geometry();
		//D2D1_POINT_2F q_pos[4];
		//q_pos[0] = { 0.0f, 0.0f };
		//q_pos[1] = m_diff[0];
		//q_pos[2] = m_diff[1];
		//q_pos[3] = m_diff[2];
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
		const size_t n = m_diff.size();
		for (size_t i = 0; i < n; i++) {
			write_svg(m_diff[i], "l", dt_writer);
		}
		write_svg("Z\" ", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
	}

}