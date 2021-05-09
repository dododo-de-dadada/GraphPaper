//------------------------------
// Shape_poly.cpp
// ���p�`
//------------------------------
#include "pch.h"
#include <corecrt_math.h>
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ���p�`�̊e�ӂ̖@���x�N�g���𓾂�.
	static bool poly_get_nor(const size_t m, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept;

	// ���s����x�N�g���𓾂�.
	static D2D1_POINT_2F poly_pt_orth(const D2D1_POINT_2F vec) { return { -vec.y, vec.x }; }

	// ���p�`�̊e�ӂ��ʒu���܂ނ����ׂ�.
	static bool poly_test_side(const D2D1_POINT_2F t_pos, const size_t m, const D2D1_POINT_2F q_pos[], const D2D1_POINT_2F n_vec[], const double exp, D2D1_POINT_2F q_exp[]) noexcept;

	// ���p�`�̊e�p���ʒu���܂ނ����ׂ�.
	static bool poly_test_corner(const D2D1_POINT_2F t_pos, const size_t m, const D2D1_POINT_2F exp[], const D2D1_POINT_2F n_vec[], const double ext) noexcept;

	//	���p�`�̊e�ӂ̖@���x�N�g���𓾂�.
	//	m	���_�̐�
	//	v_pos	���_�̔z�� [m]
	//	n_vec	�e�ӂ̖@���x�N�g���̔z�� [m]
	//	�߂�l	�@���x�N�g���𓾂��Ȃ� true, ���ׂĂ̒��_���d�Ȃ��Ă����Ȃ� false
	static bool poly_get_nor(const size_t m, const D2D1_POINT_2F v_pos[], D2D1_POINT_2F n_vec[]) noexcept
	{
		// ���p�`�̊e�ӂ̒����Ɩ@���x�N�g��, 
		// �d�����Ȃ����_�̐������߂�.
		std::vector<double> s_len(m);	// �e�ӂ̒���
		int q_cnt = 1;
		for (size_t i = 0; i < m; i++) {
			// ���̒��_�Ƃ̍��������߂�.
			D2D1_POINT_2F q_sub;
			pt_sub(v_pos[(i + 1) % m], v_pos[i], q_sub);
			// �����̒��������߂�.
			s_len[i] = sqrt(pt_abs2(q_sub));
			if (s_len[i] > FLT_MIN) {
				// �����̒����� 0 ���傫���Ȃ�, 
				// �d�����Ȃ����_�̐����C���N�������g����.
				q_cnt++;
			}
			// �����ƒ��s����x�N�g���𐳋K�����Ė@���x�N�g���Ɋi�[����.
			pt_scale(poly_pt_orth(q_sub), 1.0 / s_len[i], n_vec[i]);
		}
		if (q_cnt == 1) {
			// ���ׂĂ̒��_���d�Ȃ����Ȃ�, false ��Ԃ�.
			return false;
		}
		for (size_t i = 0; i < m; i++) {
			if (s_len[i] <= FLT_MIN) {
				// �ӂ̒����� 0 �Ȃ��,
				// �ӂɗאڂ���O��̕ӂ̒�����
				// ������ 0 �łȂ��ӂ�T��,
				// �����̖@���x�N�g����������, 
				// ���� 0 �̕ӂ̖@���x�N�g���Ƃ���.
				size_t prev;
				for (size_t j = 1; s_len[prev = ((i - j) % m)] < FLT_MIN; j++);
				size_t next;
				for (size_t j = 1; s_len[next = ((i + j) % m)] < FLT_MIN; j++);
				pt_add(n_vec[prev], n_vec[next], n_vec[i]);
				auto len = sqrt(pt_abs2(n_vec[i]));
				if (len > FLT_MIN) {
					pt_scale(n_vec[i], 1.0 / len, n_vec[i]);
					continue;
				}
				// �����x�N�g�����[���x�N�g���ɂȂ�Ȃ�,
				// ���O�̖@���x�N�g���ɒ�������x�N�g����@���x�N�g���Ƃ���.
				n_vec[i] = poly_pt_orth(n_vec[prev]);
			}
		}
		return true;
	}

	// ���p�`�̊e�ӂ��ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// m	���_�̐�
	// v_pos	���_�̔z�� [m]
	// n_vec	�e�ӂ̖@���x�N�g���̔z�� [m]
	// s_width	�ӂ̑���
	// exp_side	�g�������ӂ̔z�� [m �~ 4]
	static bool poly_test_side(const D2D1_POINT_2F t_pos, const size_t m, const D2D1_POINT_2F v_pos[], const D2D1_POINT_2F n_vec[], const double s_width, D2D1_POINT_2F exp_side[]) noexcept
	{
		for (size_t i = 0; i < m; i++) {
			// �ӂ̕Е��̒[�_��@���x�N�g���ɂ�����
			// �����̔��������ړ������ʒu��
			// �g�������ӂɊi�[����.
			D2D1_POINT_2F nor;
			pt_scale(n_vec[i], s_width, nor);
			const auto j = i * 4;
			pt_add(v_pos[i], nor, exp_side[j + 0]);
			// �t�����ɂ��ړ���, �g�������ӂɊi�[����.
			pt_sub(v_pos[i], nor, exp_side[j + 1]);
			// �ӂ̂�������̒[�_��@���x�N�g���ɂ�����
			// �����̔��������ړ������ʒu��
			// �g�������ӂɊi�[����.
			const auto k = (i + 1) % m;
			pt_sub(v_pos[k], nor, exp_side[j + 2]);
			// �t�����ɂ��ړ���, �g�������ӂɊi�[����.
			pt_add(v_pos[k], nor, exp_side[j + 3]);
			// �ʒu���g�������ӂɊ܂܂�邩���ׂ�.
			if (pt_in_poly(t_pos, 4, exp_side + j)) {
				// �܂܂��Ȃ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	//	���p�`�̊e�p���ʒu���܂ނ����ׂ�.
	//	t_pos	���ׂ�ʒu
	// 	m	�ӂ̐�
	//	exp_side	�g�������ӂ̔z�� [m �~ 4]
	//	n_vec	�e�ӂ̖@���x�N�g���̔z�� [m]
	//	ext_len	�p�𒴂��ĉ������钷��
	static bool poly_test_corner(const D2D1_POINT_2F t_pos, const size_t m, const D2D1_POINT_2F exp_side[], const D2D1_POINT_2F n_vec[], const double ext_len) noexcept
	{
		D2D1_POINT_2F ext_side[4];	// �g�������ӂ�����ɉ���������
		D2D1_POINT_2F p_vec;	// ���s�ȃx�N�g��

		for (size_t i = 0, j = m - 1; i < m; j = i++) {
			// ���钸�_�ɗאڂ���ӂɂ���.
			// �g�������ӂ̒[��, ���������ӂ̒[�Ɋi�[����.
			ext_side[0] = exp_side[m * j + 3];
			ext_side[1] = exp_side[m * j + 2];
			// �@���x�N�g���ƒ��s����x�N�g����,
			// �������钷���̕������{��,
			// ���s�ȃx�N�g���Ɋi�[����.
			pt_scale(poly_pt_orth(n_vec[j]), ext_len, p_vec);
			// �i�[�����ʒu�𕽍s�ȃx�N�g���ɉ����ĉ�����,
			// ���������ӂ̂�������̒[�Ɋi�[����.
			pt_sub(ext_side[1], p_vec, ext_side[2]);
			pt_sub(ext_side[0], p_vec, ext_side[3]);
			// �ʒu�����������ӂɊ܂܂�邩���ׂ�.
			if (pt_in_poly(t_pos, 4, ext_side) != true) {
				// �܂܂�Ȃ��Ȃ�p������.
				continue;
			}
			// �אڂ�������Е��̕ӂɂ���.
			// �g�������ӂ̒[��, ���������ӂ̒[�Ɋi�[����.
			ext_side[2] = exp_side[m * i + 1];
			ext_side[3] = exp_side[m * i + 0];
			// �@���x�N�g���ƒ��s����x�N�g�� (��قǂƂ͋t����) �𓾂�,
			// �p���������钷���̕������{��,
			// ���s�ȃx�N�g���Ɋi�[����.
			pt_scale(poly_pt_orth(n_vec[i]), ext_len, p_vec);
			// �i�[�����ʒu�𕽍s�ȃx�N�g���ɉ����ĉ�����,
			// ���������ӂ̂�������̒[�Ɋi�[����.
			pt_add(ext_side[3], p_vec, ext_side[0]);
			pt_add(ext_side[2], p_vec, ext_side[1]);
			// �ʒu�����������ӂɊ܂܂�邩���ׂ�.
			if (pt_in_poly(t_pos, 4, ext_side)) {
				// �܂܂��Ȃ� true ��Ԃ�.
				return true;
			}
		}
		return false;
	}

	// �p�X�W�I���g�����쐬����.
	void ShapePoly::create_path_geometry(void)
	{
		const size_t m = m_diff.size() + 1;	// ���_�̐� (�����̐� + 1)
		std::vector<D2D1_POINT_2F> v_pos(m);	// ���_�̔z��

		m_poly_geom = nullptr;
		v_pos[0] = m_pos;
		for (size_t i = 1; i < m; i++) {
			pt_add(v_pos[i - 1], m_diff[i - 1], v_pos[i]);
		}
		winrt::com_ptr<ID2D1GeometrySink> sink;
		winrt::check_hresult(
			s_d2d_factory->CreatePathGeometry(m_poly_geom.put())
		);
		m_poly_geom->Open(sink.put());
		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		const auto figure_begin = is_opaque(m_fill_color)
			? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
			: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
		sink->BeginFigure(v_pos[0], figure_begin);
		for (size_t i = 1; i < m; i++) {
			sink->AddLine(v_pos[i]);
		}
		// Shape ��Ŏn�_�ƏI�_���d�˂��Ƃ�,
		// �p�X�Ɏn�_�������Ȃ���, LINE_JOINT ���ւ�Ȃ��ƂɂȂ�.
		sink->AddLine(v_pos[0]);
		sink->EndFigure(D2D1_FIGURE_END_CLOSED);
		sink->Close();
		sink = nullptr;
	}

	// �}�`��\������.
	// dx	�}�`�̕`���
	void ShapePoly::draw(SHAPE_DX& dx)
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
		const size_t n = m_diff.size();	// �����̐�
		for (size_t i = 1; i < n; i++) {
			pt_add(a_pos, m_diff[i], a_pos);
			anchor_draw_rect(a_pos, dx);
		}
	}

	// �h��Ԃ��F�𓾂�.
	// value	����ꂽ�l
	// �߂�l	����ꂽ�Ȃ� true
	bool ShapePoly::get_fill_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_fill_color;
		return true;
	}

	// �ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapePoly::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const size_t m = m_diff.size() + 1;	// ���_�̐� (�����̐� + 1)
		constexpr D2D1_POINT_2F PZ{ 0.0f, 0.0f };	// ��_
		size_t j = static_cast<size_t>(-1);	// �_���܂ޒ��_�̓Y����
		size_t k = 0;	// �d���������_�����������_�̐�
		std::vector<D2D1_POINT_2F> v_pos(m);	// ���_�̔z��
		// ���ׂ�ʒu�����_�ƂȂ�悤���s�ړ������l�ւ�`�̊e���_�𓾂�.
		pt_sub(m_pos, t_pos, v_pos[k++]);
		if (pt_in_anch(v_pos[0], a_len)) {
			j = 0;
		}
		for (size_t i = 1; i < m; i++) {
			pt_add(v_pos[k - 1], m_diff[i - 1], v_pos[k]);
			if (pt_in_anch(v_pos[i], a_len)) {
				j = i;
			}
			if (pt_abs2(m_diff[i - 1]) > FLT_MIN) {
				k++;
			}
		}
		if (j != -1) {
			const auto anch = ANCH_WHICH::ANCH_P0 + j;
			return static_cast<uint32_t>(anch);
		}
		if (is_opaque(m_stroke_color) && k > 0) {
			// �ӂ��s�����Ȃ�, ���̑��������Ƃ�, �������ӂ��v�Z����.
			const auto width = max(max(m_stroke_width, a_len) * 0.5, 0.5);	// ��

			// �e�ӂ̖@���x�N�g���𓾂�.
			std::vector<D2D1_POINT_2F> n_vec(k);	// �@���x�N�g��
			poly_get_nor(k, v_pos.data(), n_vec.data());

			//	���p�`�̊e�ӂ��ʒu���܂ނ����ׂ�.
			std::vector<D2D1_POINT_2F> q_exp(k * 4);	// ��������
			if (poly_test_side(PZ, k, v_pos.data(), n_vec.data(), width, q_exp.data())) {
				// �܂ނȂ� ANCH_FRAME ��Ԃ�.
				return ANCH_WHICH::ANCH_FRAME;
			}

			//	���p�`�̊e�p���ʒu���܂ނ����ׂ�.
			//	�p�𒴂��ĉ������钷���͕ӂ̑����� 5 �{.
			//	���������, D2D �̕`��ƈ�v����.
			const auto ext_len = m_stroke_width * 5.0;	// �������钷��
			if (poly_test_corner(PZ, k, q_exp.data(), n_vec.data(), ext_len)) {
				// �܂ނȂ� ANCH_FRAME ��Ԃ�.
				return ANCH_WHICH::ANCH_FRAME;
			}
		}
		// �ӂ��s����, �܂��͈ʒu���ӂɊ܂܂�Ă��Ȃ��Ȃ�,
		// �h��Ԃ��F���s���������ׂ�.
		if (is_opaque(m_fill_color)) {
			// �s�����Ȃ�, �ʒu�����p�`�Ɋ܂܂�邩���ׂ�.
			if (pt_in_poly(PZ, k, v_pos.data())) {
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
	bool ShapePoly::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		if (!pt_in_rect(m_pos, a_min, a_max)) {
			return false;
		}
		const size_t n = m_diff.size();	// �����̐�
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
	void ShapePoly::set_fill_color(const D2D1_COLOR_F& value) noexcept
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
	// attr	�����l
	ShapePoly::ShapePoly(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeSheet* attr) :
		ShapePath::ShapePath(4, attr)
	{
		m_pos.x = static_cast<FLOAT>(s_pos.x + 0.5 * diff.x);
		m_pos.y = s_pos.y;
		pt_scale(diff, 0.5, m_diff[0]);
		m_diff[1].x = -m_diff[0].x;
		m_diff[1].y = m_diff[0].y;
		m_diff[2].x = m_diff[1].x;
		m_diff[2].y = -m_diff[0].y;
m_diff[3].x = 0.0;
m_diff[3].y = 0.0;
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
	ShapePoly::ShapePoly(DataReader const& dt_reader) :
		ShapePath::ShapePath(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_fill_color, dt_reader);
		create_path_geometry();
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapePoly::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapePath::write(dt_writer);
		write(m_fill_color, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapePoly::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg(m_pos, "M", dt_writer);
		const size_t n = m_diff.size();	// �����̐�
		for (size_t i = 0; i < n; i++) {
			write_svg(m_diff[i], "l", dt_writer);
		}
		write_svg("Z\" ", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg(m_fill_color, "fill", dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
	}

}