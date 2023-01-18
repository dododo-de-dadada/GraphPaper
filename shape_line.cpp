//------------------------------
// Shape_line.cpp
// �����Ɩ�邵
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// ��邵�� D2D1 �p�X�W�I���g�����쐬����.
	static void line_create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo);
	// ��邵�� D2D �X�g���[�N�������쐬����.
	static void line_create_arrow_style(ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, const D2D1_LINE_JOIN s_join_style, const double s_join_miter_limit, ID2D1StrokeStyle** s_arrow_style);
	// ��邵�̐�[�ƕԂ��̈ʒu�����߂�.
	//static bool line_get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept;
	// �������ʒu���܂ނ�, �������l�����Ĕ��肷��.
	static bool line_hit_test(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept;

	// ��邵�� D2D1 �p�X�W�I���g�����쐬����
	// d_factory	D2D �t�@�N�g���[
	// s_pos	���̊J�n�ʒu
	// d_vec	���̏I���ʒu�ւ̍���
	// style	��邵�̌`��
	// size	��邵�̐��@
	// geo	�쐬���ꂽ�p�X�W�I���g��
	static void line_create_arrow_geom(ID2D1Factory3* const d_factory, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F d_vec, ARROW_STYLE style, ARROW_SIZE& a_size, ID2D1PathGeometry** geo)
	{
		D2D1_POINT_2F barbs[2];	// ��邵�̕Ԃ��̒[�_
		D2D1_POINT_2F tip_pos;	// ��邵�̐�[�_
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (ShapeLine::line_get_arrow_pos(s_pos, d_vec, a_size, barbs, tip_pos)) {
			// �W�I���g���p�X���쐬����.
			winrt::check_hresult(d_factory->CreatePathGeometry(geo));
			winrt::check_hresult((*geo)->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(
				barbs[0],
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW
			);
			sink->AddLine(tip_pos);
			sink->AddLine(barbs[1]);
			sink->EndFigure(
				style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN
			);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
		}
	}

	// ��邵�� D2D �X�g���[�N�������쐬����.
	static void line_create_arrow_style(ID2D1Factory3* const d_factory, const CAP_STYLE s_cap_style, const D2D1_LINE_JOIN s_join_style, const double s_join_miter_limit, ID2D1StrokeStyle** s_arrow_style)
	{
		// ��邵�̔j���̌`���͂��Ȃ炸�\���b�h�Ƃ���.
		const D2D1_STROKE_STYLE_PROPERTIES s_prop{
			s_cap_style.m_start,	// startCap
			s_cap_style.m_end,	// endCap
			D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// dashCap
			s_join_style,	// lineJoin
			static_cast<FLOAT>(s_join_miter_limit),	// miterLimit
			D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
			0.0f	// dashOffset
		};
		winrt::check_hresult(
			d_factory->CreateStrokeStyle(s_prop, nullptr, 0, s_arrow_style)
		);
	}

	// ��邵�̐�[�ƕԂ��̈ʒu�����߂�.
	// a_pos	��̌�[�̈ʒu
	// a_vec	��̐�[�ւ̃x�N�g��
	// a_size	��邵�̐��@
	// barbs	�Ԃ��̈ʒu
	// tip		��[�̈ʒu
	bool ShapeLine::line_get_arrow_pos(const D2D1_POINT_2F a_pos, const D2D1_POINT_2F a_vec, const ARROW_SIZE& a_size, D2D1_POINT_2F barbs[2], D2D1_POINT_2F& tip) noexcept
	{
		const auto a_len = std::sqrt(pt_abs2(a_vec));	// ��̒���
		if (a_len >= FLT_MIN) {
			get_arrow_barbs(a_vec, a_len, a_size.m_width, a_size.m_length, barbs);
			if (a_size.m_offset >= a_len) {
				// ��邵�̐�[
				tip = a_pos;
			}
			else {
				pt_mul_add(a_vec, 1.0 - a_size.m_offset / a_len, a_pos, tip);
			}
			pt_add(barbs[0], tip, barbs[0]);
			pt_add(barbs[1], tip, barbs[1]);
			return true;
		}
		return false;
	}

	// �������ʒu���܂ނ�, �������l�����Ĕ��肷��.
	// t_pos	���肷��ʒu
	// s_pos	�����̎n�[
	// e_pos	�����̏I�[
	// s_width	�����̑���
	// s_cap	�����̒[�̌`��
	// �߂�l	�܂ޏꍇ true
	static bool line_hit_test(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept
	{
		const double e_width = max(s_width * 0.5, 0.5);
		if (equal(s_cap, CAP_SQUARE)) {
			D2D1_POINT_2F d_vec;	// ���������̃x�N�g��
			pt_sub(e_pos, s_pos, d_vec);
			const double abs2 = pt_abs2(d_vec);
			pt_mul(
				abs2 >= FLT_MIN ? d_vec : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				d_vec);
			const double dx = d_vec.x;
			const double dy = d_vec.y;
			const double ox = dy;
			const double oy = -dx;
			D2D1_POINT_2F e_side[4];
			pt_add(s_pos, -dx + ox, -dy + oy, e_side[0]);
			pt_add(s_pos, -dx - ox, -dy - oy, e_side[1]);
			pt_add(e_pos, dx - ox, dy - oy, e_side[2]);
			pt_add(e_pos, dx + ox, dy + oy, e_side[3]);
			return pt_in_poly(t_pos, 4, e_side);
		}
		else if (equal(s_cap, CAP_TRIANGLE)) {
			D2D1_POINT_2F d_vec;	// ���������̃x�N�g��
			pt_sub(e_pos, s_pos, d_vec);
			const double abs2 = pt_abs2(d_vec);
			pt_mul(
				abs2 >= FLT_MIN ? d_vec : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 >= FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				d_vec);
			const double dx = d_vec.x;
			const double dy = d_vec.y;
			const double ox = dy;
			const double oy = -dx;
			D2D1_POINT_2F e_side[6];
			pt_add(s_pos, ox, oy, e_side[0]);
			pt_add(s_pos, -dx, -dy, e_side[1]);
			pt_add(s_pos, -ox, -oy, e_side[2]);
			pt_add(e_pos, -ox, -oy, e_side[3]);
			pt_add(e_pos, dx, dy, e_side[4]);
			pt_add(e_pos, ox, oy, e_side[5]);
			return pt_in_poly(t_pos, 6, e_side);
		}
		else {
			if (equal(s_cap, CAP_ROUND)) {
				if (pt_in_circle(t_pos, s_pos, e_width) || pt_in_circle(t_pos, e_pos, e_width)) {
					return true;
				}
			}
			D2D1_POINT_2F d_vec;	// �����x�N�g��
			pt_sub(e_pos, s_pos, d_vec);
			const double abs2 = pt_abs2(d_vec);
			if (abs2 >= FLT_MIN) {
				pt_mul(d_vec, e_width / sqrt(abs2), d_vec);
				const double ox = d_vec.y;
				const double oy = -d_vec.x;
				D2D1_POINT_2F e_side[4];
				pt_add(s_pos, ox, oy, e_side[0]);
				pt_add(s_pos, -ox, -oy, e_side[1]);
				pt_add(e_pos, -ox, -oy, e_side[2]);
				pt_add(e_pos, ox, oy, e_side[3]);
				return pt_in_poly(t_pos, 4, e_side);
			}
		}
		return false;
	}

	// �}�`��\������.
	void ShapeLine::draw(void)
	{
		ID2D1Factory3* const factory = Shape::s_d2d_factory;
		ID2D1RenderTarget* const context = Shape::s_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::s_d2d_color_brush;

		if (m_d2d_stroke_style == nullptr) {
			create_stroke_style(factory);
		}

		brush->SetColor(m_stroke_color);
		const auto s_style = m_d2d_stroke_style.get();
		const auto s_width = m_stroke_width;

		D2D1_POINT_2F e_pos;
		pt_add(m_start, m_vec[0], e_pos);
		context->DrawLine(m_start, e_pos, brush, s_width, s_style);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			if (m_d2d_arrow_style == nullptr) {
				line_create_arrow_style(factory, m_stroke_cap, m_join_style, m_join_miter_limit, m_d2d_arrow_style.put());
			}
			if (m_d2d_arrow_geom == nullptr) {
				line_create_arrow_geom(factory, m_start, m_vec[0], m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
			}
			const auto a_geom = m_d2d_arrow_geom.get();
			if (m_d2d_arrow_geom != nullptr) {
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					context->FillGeometry(a_geom, brush, nullptr);
				}
				context->DrawGeometry(a_geom, brush, s_width, m_d2d_arrow_style.get());
			}
		}
		if (is_selected()) {
			D2D1_POINT_2F mid;
			pt_mul_add(m_vec[0], 0.5, m_start, mid);
			anc_draw_rect(m_start, context, brush);
			anc_draw_rect(mid, context, brush);
			anc_draw_rect(e_pos, context, brush);
		}
	}

	// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
	// anc	�}�`�̕���
	// val	����ꂽ�ʒu
	void ShapeLine::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		if (anc == ANC_TYPE::ANC_PAGE || anc == ANC_TYPE::ANC_P0) {
			// �}�`�̕��ʂ��u�O���v�܂��́u�J�n�_�v�Ȃ��, �J�n�ʒu�𓾂�.
			val = m_start;
		}
		else if (anc > ANC_TYPE::ANC_P0) {
			const size_t a_cnt = anc - ANC_TYPE::ANC_P0;
			if (a_cnt < m_vec.size() + 1) {
				val = m_start;
				for (size_t i = 0; i < a_cnt; i++) {
					pt_add(val, m_vec[i], val);
				}
			}
		}
	}

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	// val	�̈�̍���ʒu
	void ShapeLine::get_pos_lt(D2D1_POINT_2F& val) const noexcept
	{
		const size_t d_cnt = m_vec.size();	// �����̐�
		D2D1_POINT_2F v_pos = m_start;	// ���_�̈ʒu
		val = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(v_pos, m_vec[i], v_pos);
			val.x = val.x < v_pos.x ? val.x : v_pos.x;
			val.y = val.y < v_pos.y ? val.y : v_pos.y;
		}
	}

	// �J�n�ʒu�𓾂�
	// �߂�l	�˂� true
	bool ShapeLine::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_start;
		return true;
	}

	// ��邵�̐��@�𓾂�.
	bool ShapeLine::get_arrow_size(ARROW_SIZE& val) const noexcept
	{
		val = m_arrow_size;
		return true;
	}

	// ��邵�̌`���𓾂�.
	bool ShapeLine::get_arrow_style(ARROW_STYLE& val) const noexcept
	{
		val = m_arrow_style;
		return true;
	}

	// �}�`���͂ޗ̈�𓾂�.
// a_lt	���̗̈�̍���ʒu.
// a_rb	���̗̈�̉E���ʒu.
// b_lt	�͂ޗ̈�̍���ʒu.
// b_rb	�͂ޗ̈�̉E���ʒu.
	void ShapeLine::get_bound(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept
	{
		b_lt.x = m_start.x < a_lt.x ? m_start.x : a_lt.x;
		b_lt.y = m_start.y < a_lt.y ? m_start.y : a_lt.y;
		b_rb.x = m_start.x > a_rb.x ? m_start.x : a_rb.x;
		b_rb.y = m_start.y > a_rb.y ? m_start.y : a_rb.y;
		const size_t d_cnt = m_vec.size();	// �����̐�
		D2D1_POINT_2F pos = m_start;
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(pos, m_vec[i], pos);
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

	// ���_�𓾂�.
	size_t ShapeLine::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_start;
		const size_t d_cnt = m_vec.size();
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
		}
		return d_cnt + 1;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeLine::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_start, m_vec[0], e_pos);
		if (pt_in_anc(t_pos, e_pos)) {
			return ANC_TYPE::ANC_P0 + 1;
		}
		if (pt_in_anc(t_pos, m_start)) {
			return ANC_TYPE::ANC_P0;
		}
		const float s_width = static_cast<float>(max(static_cast<double>(m_stroke_width), Shape::s_anc_len));
		if (line_hit_test(t_pos, m_start, e_pos, s_width, m_stroke_cap)) {
			return ANC_TYPE::ANC_STROKE;
		}
		return ANC_TYPE::ANC_PAGE;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// area_lt	�͈͂̍���ʒu
	// area_rb	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeLine::in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept
	{
		if (pt_in_rect(m_start, area_lt, area_rb)) {
			D2D1_POINT_2F pos;
			pt_add(m_start, m_vec[0], pos);
			return pt_in_rect(pos, area_lt, area_rb);
		}
		return false;
	}

	// ���������ړ�����.
	/*
	bool ShapeLine::move(const D2D1_POINT_2F d_vec) noexcept
	{
		if (ShapeStroke::move(d_vec)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}
	*/

	// �l���邵�̐��@�Ɋi�[����.
	bool ShapeLine::set_arrow_size(const ARROW_SIZE& val) noexcept
	{
		if (!equal(m_arrow_size, val)) {
			m_arrow_size = val;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l���邵�̌`���Ɋi�[����.
	bool ShapeLine::set_arrow_style(const ARROW_STYLE val) noexcept
	{
		if (m_arrow_style != val) {
			m_arrow_style = val;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l������̌����̃}�C�^�[�����Ɋi�[����.
	bool ShapeLine::set_join_miter_limit(const float& val) noexcept
	{
		if (ShapeStroke::set_join_miter_limit(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l������̌����Ɋi�[����.
	bool ShapeLine::set_join_style(const D2D1_LINE_JOIN& val) noexcept
	{
		if (ShapeStroke::set_join_style(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����.
	// val	�l
	// anc	�}�`�̕���
	// limit	���E���� (���̒��_�Ƃ̋��������̒l�����ɂȂ�Ȃ�, ���̒��_�Ɉʒu�ɍ��킹��)
	bool ShapeLine::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool /*keep_aspect*/) noexcept
	{
		bool done = false;
		// �ύX���钸�_���ǂ̒��_�����肷��.
		const size_t d_cnt = m_vec.size();	// �����̐�
		if (anc >= ANC_TYPE::ANC_P0 && anc <= ANC_TYPE::ANC_P0 + d_cnt) {
			D2D1_POINT_2F v_pos[N_GON_MAX];	// ���_�̈ʒu
			const size_t a_cnt = anc - ANC_TYPE::ANC_P0;	// �ύX���钸�_
			// �ύX���钸�_�܂ł�, �e���_�̈ʒu�𓾂�.
			v_pos[0] = m_start;
			for (size_t i = 0; i < a_cnt; i++) {
				pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
			}
			// �l����ύX�O�̈ʒu������, �ύX���鍷���𓾂�.
			D2D1_POINT_2F vec;
			pt_sub(val, v_pos[a_cnt], vec);
			pt_round(vec, PT_ROUND, vec);
			// �����̒������[�����傫�������肷��.
			if (pt_abs2(vec) >= FLT_MIN) {
				// �ύX���钸�_���ŏ��̒��_�����肷��.
				if (a_cnt == 0) {
					// �ŏ��̒��_�̈ʒu�ɕύX����������.
					pt_add(m_start, vec, m_start);
				}
				else {
					// ���_�̒��O�̍����ɕύX����������.
					pt_add(m_vec[a_cnt - 1], vec, m_vec[a_cnt - 1]);
				}
				// �ύX����̂��Ō�̒��_�ȊO�����肷��.
				if (a_cnt < d_cnt) {
					// ���̒��_�������Ȃ��悤��,
					// �ύX���钸�_�̎��̒��_�ւ̍�������ύX��������.
					pt_sub(m_vec[a_cnt], vec, m_vec[a_cnt]);
				}
				if (!done) {
					done = true;
				}
			}
			// ���E�������[���łȂ������肷��.
			if (limit >= FLT_MIN) {
				// �c��̒��_�̈ʒu�𓾂�.
				for (size_t i = a_cnt; i < d_cnt; i++) {
					pt_add(v_pos[i], m_vec[i], v_pos[i + 1]);
				}
				for (size_t i = 0; i < d_cnt + 1; i++) {
					// ���_��, �ύX���钸�_�����肷��.
					if (i == a_cnt) {
						continue;
					}
					// ���_�ƕύX���钸�_�Ƃ̋��������E�����ȏォ���肷��.
					D2D1_POINT_2F v_vec;
					pt_sub(v_pos[i], v_pos[a_cnt], v_vec);
					const double d = static_cast<double>(limit);
					if (pt_abs2(v_vec) >= d * d) {
						continue;
					}
					// �ύX����̂��ŏ��̒��_�����肷��.
					if (a_cnt == 0) {
						pt_add(m_start, v_vec, m_start);
					}
					else {
						pt_add(m_vec[a_cnt - 1], v_vec, m_vec[a_cnt - 1]);
					}
					// �ύX����̂��Ō�̒��_�ȊO�����肷��.
					if (a_cnt < d_cnt) {
						pt_sub(m_vec[a_cnt], v_vec, m_vec[a_cnt]);
					}
					if (!done) {
						done = true;
					}
					break;
				}
			}
		}
		if (done && m_d2d_arrow_geom != nullptr) {
			m_d2d_arrow_geom = nullptr;
		}
		return done;
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	bool ShapeLine::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_round(val, PT_ROUND, new_pos);
		if (!equal(m_start, new_pos)) {
			m_start = new_pos;
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	/*
	// �l��, ���ʂ̈ʒu�Ɋi�[����. 
	// val	�l
	// anc	�}�`�̕���
	// limit	���E���� (���̒��_�Ƃ̋��������̒l�����ɂȂ�Ȃ�, ���̒��_�Ɉʒu�ɍ��킹��)
	bool ShapeLine::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float limit, const bool keep_aspect) noexcept
	{
		if (ShapeStroke::set_pos_anc(val, anc, limit, keep_aspect)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}
	*/

	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	/*
	bool ShapeLine::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		if (ShapeStroke::set_pos_start(val)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}
	*/

	// �l��[�̌`���Ɋi�[����.
	bool ShapeLine::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (ShapeStroke::set_stroke_cap(val)) {
			if (m_d2d_arrow_style != nullptr) {
				m_d2d_arrow_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// ���������ړ�����.
	// d_vec	�����x�N�g��
	bool ShapeLine::move(const D2D1_POINT_2F d_vec) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_add(m_start, d_vec, new_pos);
		if (set_pos_start(new_pos)) {
			if (m_d2d_arrow_geom != nullptr) {
				m_d2d_arrow_geom = nullptr;
			}
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_vec	�͂ޗ̈�̏I�_�ւ̍���
	// page	����̑����l
	ShapeLine::ShapeLine(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_vec, const ShapePage* page) :
		ShapeStroke::ShapeStroke(page),
		m_arrow_style(page->m_arrow_style),
		m_arrow_size(page->m_arrow_size),
		m_d2d_arrow_geom(nullptr),
		m_d2d_arrow_style(nullptr)
	{
		m_start = b_pos;
		m_vec.resize(1, b_vec);
		m_vec.shrink_to_fit();
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	// dt_reader	�ǂݍ��ރf�[�^���[�_�[
	ShapeLine::ShapeLine(const ShapePage& page, DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(page, dt_reader),
		m_d2d_arrow_style(nullptr),
		m_d2d_arrow_geom(nullptr)
	{
		m_start.x = dt_reader.ReadSingle();
		m_start.y = dt_reader.ReadSingle();
		const size_t vec_cnt = dt_reader.ReadUInt32();	// �v�f��
		m_vec.resize(vec_cnt);
		for (size_t i = 0; i < vec_cnt; i++) {
			m_vec[i].x = dt_reader.ReadSingle();
			m_vec[i].y = dt_reader.ReadSingle();
		}

		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeLine::write(DataWriter const& dt_writer) const
	{
		ShapeStroke::write(dt_writer);

		// �J�n�ʒu
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);

		// ���̈ʒu�ւ̍���
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_vec.size()));
		for (const D2D1_POINT_2F vec : m_vec) {
			dt_writer.WriteSingle(vec.x);
			dt_writer.WriteSingle(vec.y);
		}

		dt_writer.WriteInt32(static_cast<int32_t>(m_arrow_style));

		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
	}

	// �ߖT�̒��_��������.
// pos	����ʒu
// dd	�ߖT�Ƃ݂Ȃ����� (�̓��l), �����藣�ꂽ���_�͋ߖT�Ƃ݂͂Ȃ��Ȃ�.
// val	����ʒu�̋ߖT�ɂ��钸�_
// �߂�l	���������� true
	bool ShapeLine::get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool done = false;
		D2D1_POINT_2F vec;
		pt_sub(m_start, pos, vec);
		float vv = static_cast<float>(pt_abs2(vec));
		if (vv < dd) {
			dd = vv;
			val = m_start;
			if (!done) {
				done = true;
			}
		}
		D2D1_POINT_2F v_pos{ m_start };
		for (const auto d_vec : m_vec) {
			pt_add(v_pos, d_vec, v_pos);
			pt_sub(v_pos, pos, vec);
			vv = static_cast<float>(pt_abs2(vec));
			if (vv < dd) {
				dd = vv;
				val = v_pos;
				if (!done) {
					done = true;
				}
			}
		}
		return done;
	}

}