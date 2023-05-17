//------------------------------
// shape_bezier.cpp
// �x�W�F�Ȑ�
//------------------------------
#include "pch.h"
#include "shape.h"
#include "shape_bezier.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �Z�O�����g����؂鏕�ϐ��̒l
	constexpr double T0 = 0.0;	// ��Ԃ̊J�n
	constexpr double T1 = 1.0 / 3.0;	// 1 �Ԗڂ̋�؂�
	constexpr double T2 = 2.0 / 3.0;	// 2 �Ԗڂ̋�؂�
	constexpr double T3 = 1.0;	// ��Ԃ̏I�[

	// �Ȑ��̖�邵�̃W�I���g�����쐬����.
	static HRESULT bezi_create_arrow_geom(ID2D1Factory3* const factory, const D2D1_POINT_2F start, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_STYLE a_style, const ARROW_SIZE a_size, ID2D1PathGeometry** a_geo) noexcept;

	// �_�̔z������Ƃɂ��������ׂĊ܂ޓʕ�����߂�.
	static void bezi_get_convex(const uint32_t p_cnt, const POINT_2D p[], uint32_t& c_cnt, POINT_2D c[]);

	// �Ȑ��̒[���_���܂ނ����肷��.
	template<D2D1_CAP_STYLE S>
	static bool bezi_hit_test_cap(const D2D1_POINT_2F& t, const D2D1_POINT_2F c[4], const D2D1_POINT_2F s[3], const double e_width);

	// �_���ʕ�Ɋ܂܂�邩���肷��.
	static bool bezi_in_convex(const double tx, const double ty, const size_t c_cnt, const POINT_2D c[]) noexcept;

	// �Ȑ���̏��ϐ������ƂɈʒu�����߂�.
	static inline void bezi_point_by_param(const POINT_2D c[4], const double t, POINT_2D& p) noexcept;

	//------------------------------
	// ���̕Ԃ��Ɛ�[�̓_�𓾂�
	//------------------------------
	bool ShapeBezier::bezi_get_pos_arrow(
		const D2D1_POINT_2F b_start, const D2D1_BEZIER_SEGMENT& b_seg,	// �Ȑ��̎n�_�Ɛ���_
		const ARROW_SIZE a_size,	// ��邵�̐��@
		D2D1_POINT_2F arrow[3]	// ���̕Ԃ��Ɛ�[�̓_
	) noexcept
	{
		POINT_2D seg[3]{};
		POINT_2D c[4]{};	// ����_

		// ����_��z��Ɋi�[����.
		seg[0] = b_seg.point1;
		seg[1] = b_seg.point2;
		seg[2] = b_seg.point3;

		// ���W�l�ɂ��덷�����Ȃ��ł���, �Ǝv����̂�,
		// �x�W�F�Ȑ����n�_�����_�ƂȂ�悤�ɕ��s�ړ�.
		c[3] = 0.0;
		c[2] = seg[0] - b_start;
		c[1] = seg[1] - b_start;
		c[0] = seg[2] - b_start;
		auto b_len = bezi_len_by_param(c, 0.0, 1.0, SIMPSON_CNT);
		if (b_len >= FLT_MIN) {

			// ��邵�̐�[�̈ʒu��, �Ȑ��̒�����, �ǂ��炩�Z������, ���ϐ������߂�.
			const auto t = bezi_param_by_len(c, min(b_len, a_size.m_offset));

			// ���ϐ������ƂɋȐ��̐ڐ��x�N�g���𓾂�.
			POINT_2D v;
			bezi_tvec_by_param(c, t, v);

			// ��邵�̕Ԃ��̈ʒu���v�Z����
			get_pos_barbs(-v, sqrt(v * v), a_size.m_width, a_size.m_length, arrow);

			// ���ϐ��ŋȐ���̈ʒu�𓾂�.
			POINT_2D tip;	// �I�_�����_�Ƃ���, ��邵�̐�[�̈ʒu
			bezi_point_by_param(c, t, tip);

			// �Ȑ���̈ʒu���邵�̐�[�Ƃ�, �Ԃ��̈ʒu�����s�ړ�����.
			pt_add(arrow[0], tip.x, tip.y, arrow[0]);
			pt_add(arrow[1], tip.x, tip.y, arrow[1]);
			arrow[2] = tip;
			pt_add(arrow[0], b_start, arrow[0]);
			pt_add(arrow[1], b_start, arrow[1]);
			pt_add(arrow[2], b_start, arrow[2]);
			return true;
		}
		return false;
	}

	//------------------------------
	// �Ȑ��̖�邵�̃W�I���g�����쐬����.
	//------------------------------
	static HRESULT bezi_create_arrow_geom(
		ID2D1Factory3* const factory,	// D2D �t�@�N�g��
		const D2D1_POINT_2F b_start, const D2D1_BEZIER_SEGMENT& b_seg,	// �Ȑ��̎n�_�Ɛ���_
		const ARROW_STYLE a_style,	// ��邵�̎��
		const ARROW_SIZE a_size,	// ��邵�̐��@
		ID2D1PathGeometry** a_geom	// ��邵�̃W�I���g��
	) noexcept
	{
		D2D1_POINT_2F barb[3];	// ��邵�̕Ԃ��̒[�_	
		winrt::com_ptr<ID2D1GeometrySink> sink;
		HRESULT hr = S_OK;
		if (!ShapeBezier::bezi_get_pos_arrow(b_start, b_seg, a_size, barb)) {
			hr = E_FAIL;
		}
		if (hr == S_OK) {
			// �W�I���g���V���N�ɒǉ�����.
			hr = factory->CreatePathGeometry(a_geom);
		}
		if (hr == S_OK) {
			hr = (*a_geom)->Open(sink.put());
		}
		if (hr == S_OK) {
			const D2D1_FIGURE_BEGIN f_begin = a_style == ARROW_STYLE::ARROW_FILLED ?
				D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
				D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW;
			const D2D1_FIGURE_END f_end = a_style == ARROW_STYLE::ARROW_FILLED ?
				D2D1_FIGURE_END::D2D1_FIGURE_END_CLOSED :
				D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN;
			sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(barb[0], f_begin);
			sink->AddLine(barb[2]);
			sink->AddLine(barb[1]);
			sink->EndFigure(f_end);
		}
		if (hr == S_OK) {
			hr = sink->Close();
		}
		sink = nullptr;
		return hr;
	}

	//------------------------------
	// �_�̔z������Ƃɂ��������ׂĊ܂ޓʕ�����߂�.
	// �M�t�g��@����������.
	// p_cnt	�_�̐�
	// p	�_�̔z��
	// c_cnt	�ʕ�̒��_�̐�
	// c	�ʕ�̒��_�̔z��
	//------------------------------
	static void bezi_get_convex(
		const uint32_t p_cnt, const POINT_2D p[], uint32_t& c_cnt, POINT_2D c[])
	{
		// e �̂���, y �l���ł��������_�̏W������, x �l���ł��������_�̓Y���� k �𓾂�.
		uint32_t k = 0;
		double ex = p[0].x;
		double ey = p[0].y;
		for (uint32_t i = 1; i < p_cnt; i++) {
			if (p[i].y < ey || (p[i].y == ey && p[i].x < ex)) {
				ex = p[i].x;
				ey = p[i].y;
				k = i;
			}
		}
		// p[k] �� a �Ɋi�[����.
		POINT_2D a = p[k];
		c_cnt = 0;
		do {
			// c �� a ��ǉ�����.
			c[c_cnt++] = a;
			POINT_2D b = p[0];
			for (uint32_t i = 1; i < p_cnt; i++) {
				const POINT_2D d = p[i];
				if (b == a) {
					b = d;
				}
				else {
					const POINT_2D ab = b - a;
					const POINT_2D ad = d - a;
					const double v = ab.opro(ad);
					if (v > 0.0 || (fabs(v) < FLT_MIN && ad * ad > ab * ab)) {
						b = d;
					}
				}
			}
			a = b;
		} while (c_cnt < p_cnt && a != c[0]);
	}

	//------------------------------
	// �Ȑ��̒[���_���܂ނ����肷��.
	// �߂�l	�܂ނȂ� true
	//------------------------------
	template<D2D1_CAP_STYLE S> static bool bezi_hit_test_cap(
		const D2D1_POINT_2F& t,	// ���肳���_	
		const D2D1_POINT_2F c[4],	// �ʕ� (�l�ӌ`) �̒��_�̔z��
		const D2D1_POINT_2F s[3],	// �ʕ�̕ӂ̃x�N�g��
		const double ew	// �ʕ�̕ӂ̔����̑���
	)
	{
		size_t i;
		for (i = 0; i < 3; i++) {
			const double s_abs = pt_abs2(s[i]);
			if (s_abs >= FLT_MIN) {
				D2D1_POINT_2F s_dir;	// �ӂ̕����x�N�g��
				pt_mul(s[i], -ew / sqrt(s_abs), s_dir);
				D2D1_POINT_2F s_orth{ s_dir.y, -s_dir.x };	// �ӂ̒����x�N�g��
				D2D1_POINT_2F q[4];	// �[�_�̎l�ӌ`.
				pt_add(c[i], s_orth, q[0]);
				pt_sub(c[i], s_orth, q[1]);
				if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
					pt_add(q[1], s_dir, q[2]);
					pt_add(q[0], s_dir, q[3]);
					if (pt_in_poly(t, 4, q)) {
						return true;
					}
				}
				else if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
					pt_add(q[i], s_dir, q[2]);
					if (pt_in_poly(t, 3, q)) {
						return true;
					}
				}
				break;
			}
		}
		if (i == 3) {
			D2D1_POINT_2F q[4]{};
			if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
				pt_add(c[0], -ew, q[0]);
				pt_add(c[0], ew, -ew, q[1]);
				pt_add(c[0], ew, q[2]);
				pt_add(c[0], -ew, ew, q[3]);
				if (pt_in_poly(t, 4, q)) {
					return true;
				}
			}
			else if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
				pt_add(c[0], 0.0, -ew, q[0]);
				pt_add(c[0], -ew, 0.0, q[1]);
				pt_add(c[0], 0.0, ew, q[2]);
				pt_add(c[0], ew, 0.0, q[3]);
				if (pt_in_poly(t, 4, q)) {
					return true;
				}
			}
		}
		else {
			for (size_t j = 3; j > 0; j--) {
				const double s_abs = pt_abs2(s[j - 1]);
				if (s_abs >= FLT_MIN) {
					D2D1_POINT_2F s_dir;	// �ӂ̕����x�N�g��
					pt_mul(s[j - 1], ew / sqrt(s_abs), s_dir);
					D2D1_POINT_2F s_orth{ s_dir.y, -s_dir.x };	// �ӂ̒����x�N�g��
					D2D1_POINT_2F q[4];
					pt_add(c[j], s_orth, q[0]);
					pt_sub(c[j], s_orth, q[1]);
					if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE) {
						pt_add(q[1], s_dir, q[2]);
						pt_add(q[0], s_dir, q[3]);
						if (pt_in_poly(t, 4, q)) {
							return true;
						}
					}
					else if constexpr (S == D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE) {
						pt_add(c[j], s_dir, q[2]);
						if (pt_in_poly(t, 3, q)) {
							return true;
						}
					}
					break;
				}
			}
		}
		return false;
	}

	//------------------------------
	// �_ { tx, ty } ���ʕ�Ɋ܂܂�邩���肷��.
	// �����������p����.
	// c_cnt	�ʕ�̒��_�̐�
	// c	�ʕ�̒��_�̔z��
	// �߂�l	�܂܂��Ȃ� true ��, �܂܂�Ȃ��Ȃ� false ��Ԃ�.
	//------------------------------
	static bool bezi_in_convex(
		const double tx, const double ty, const size_t c_cnt, const POINT_2D c[]) noexcept
	{
		int k = 0;	// �_���Ƃ��鐅�������ʕ�̕ӂƌ��������.
		for (size_t i = c_cnt - 1, j = 0; j < c_cnt; i = j++) {
			// ���[�� 1. ������̕�. �_�����������ɂ���, �ӂ̎n�_�ƏI�_�̊Ԃɂ���. �������A�I�_�͊܂܂Ȃ�.
			// ���[�� 2. �������̕�. �_�����������ɂ���, �ӂ̎n�_�ƏI�_�̊Ԃɂ���. �������A�n�_�͊܂܂Ȃ�.
			// ���[�� 3. �_���Ƃ��鐅�����ƕӂ������łȂ�.
			// ���[�� 1. ���[�� 2 ���m�F���邱�Ƃ�, ���[�� 3 ���m�F�ł��Ă���.
			if ((c[i].y <= ty && c[j].y > ty) || (c[i].y > ty && c[j].y <= ty)) {
				// ���[�� 4. �ӂ͓_�����E���ɂ���. ������, �d�Ȃ�Ȃ�.
				// �ӂ��_�Ɠ��������ɂȂ�ʒu����肵, ���̎��̐��������̒l�Ɠ_�̂��̒l�Ƃ��r����.
				if (tx < c[i].x + (ty - c[i].y) / (c[j].y - c[i].y) * (c[j].x - c[i].x)) {
					// ���[�� 1 �܂��̓��[�� 2, �����[�� 4 �𖞂����Ȃ�, �_���Ƃ��鐅�����͓ʕ�ƌ�������.
					k++;
				}
			}
		}
		// ��������񐔂�������肷��.
		// ��Ȃ��, �_�͓ʕ�Ɋ܂܂�̂� true ��Ԃ�.
		return static_cast<bool>(k & 1);
	}

	//------------------------------
	// �Ȑ���̏��ϐ������ƂɈʒu�����߂�.
	// c	����_
	// t	���ϐ�
	// p	���܂����ʒu
	//------------------------------
	static inline void bezi_point_by_param(
		const POINT_2D c[4], const double t, POINT_2D& p) noexcept
	{
		const double s = 1.0 - t;
		const double ss = s * s;
		const double tt = t * t;
		p = c[0] * s * ss + c[1] * 3.0 * ss * t + c[2] * 3.0 * s * tt + c[3] * t * tt;
	}

	//------------------------------
	// �}�`��\������.
	//------------------------------
	void ShapeBezier::draw(void) noexcept
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();
		ID2D1Factory* factory;
		m_d2d_target->GetFactory(&factory);
		bool b_flag = false;
		D2D1_BEZIER_SEGMENT b_seg{};
		HRESULT hr = S_OK;
		const bool exist_stroke = (!equal(m_stroke_width, 0.0f) && is_opaque(m_stroke_color));
		if (exist_stroke && m_d2d_stroke_style == nullptr) {
			if (hr == S_OK) {
				hr = create_stroke_style(factory);
			}
		}
		if (exist_stroke && m_arrow_style != ARROW_STYLE::ARROW_NONE && m_d2d_arrow_stroke == nullptr) {
			if (hr == S_OK) {
				hr = create_arrow_stroke();
			}
		}
		if ((exist_stroke || is_opaque(m_fill_color)) && m_d2d_path_geom == nullptr) {
			if (!b_flag) {
				pt_add(m_start, m_pos[0], b_seg.point1);
				pt_add(b_seg.point1, m_pos[1], b_seg.point2);
				pt_add(b_seg.point2, m_pos[2], b_seg.point3);
				b_flag = true;
			}
			if (hr == S_OK) {
				hr = factory->CreatePathGeometry(m_d2d_path_geom.put());
			}
			winrt::com_ptr<ID2D1GeometrySink> sink;
			if (hr == S_OK) {
				hr = m_d2d_path_geom->Open(sink.put());
			}
			if (hr == S_OK) {
				sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
				const auto f_begin = (is_opaque(m_fill_color) ?
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_FILLED :
					D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
				sink->BeginFigure(m_start, f_begin);
				sink->AddBezier(b_seg);
				sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
				hr = sink->Close();
			}
			sink = nullptr;
		}
		if (exist_stroke && m_arrow_style != ARROW_STYLE::ARROW_NONE && m_d2d_arrow_geom == nullptr) {
			if (!b_flag) {
				pt_add(m_start, m_pos[0], b_seg.point1);
				pt_add(b_seg.point1, m_pos[1], b_seg.point2);
				pt_add(b_seg.point2, m_pos[2], b_seg.point3);
				b_flag = true;
			}
			hr = bezi_create_arrow_geom(static_cast<ID2D1Factory3*>(factory), m_start, b_seg, m_arrow_style, m_arrow_size,
				m_d2d_arrow_geom.put());
		}
		if (is_opaque(m_fill_color)) {
			brush->SetColor(m_fill_color);
			target->FillGeometry(m_d2d_path_geom.get(), brush, nullptr);
		}
		if (exist_stroke) {
			brush->SetColor(m_stroke_color);
			target->DrawGeometry(m_d2d_path_geom.get(), brush, m_stroke_width, m_d2d_stroke_style.get());
			if (m_arrow_style == ARROW_STYLE::ARROW_FILLED) {
				target->FillGeometry(m_d2d_arrow_geom.get(), brush, nullptr);
				target->DrawGeometry(m_d2d_arrow_geom.get(), brush, m_stroke_width, m_d2d_arrow_stroke.get());
			}
			else if (m_arrow_style == ARROW_STYLE::ARROW_OPENED) {
				target->DrawGeometry(m_d2d_arrow_geom.get(), brush, m_stroke_width, m_d2d_arrow_stroke.get());
			}
		}
		if (m_loc_show && is_selected()) {
			if (!b_flag) {
				pt_add(m_start, m_pos[0], b_seg.point1);
				pt_add(b_seg.point1, m_pos[1], b_seg.point2);
				pt_add(b_seg.point2, m_pos[2], b_seg.point3);
				b_flag = true;
			}
			// �⏕����`��
			if (m_stroke_width >= Shape::m_loc_square_inner) {
				brush->SetColor(COLOR_WHITE);
				target->DrawGeometry(m_d2d_path_geom.get(), brush, 2.0f * m_aux_width, nullptr);
				brush->SetColor(COLOR_BLACK);
				target->DrawGeometry(m_d2d_path_geom.get(), brush, m_aux_width, m_aux_style.get());
			}
			// ����_�ւ̕⏕����`��.
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(m_start, b_seg.point1, brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(m_start, b_seg.point1, brush, m_aux_width, m_aux_style.get());
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(b_seg.point1, b_seg.point2, brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(b_seg.point1, b_seg.point2, brush, m_aux_width, m_aux_style.get());
			brush->SetColor(COLOR_WHITE);
			target->DrawLine(b_seg.point2, b_seg.point3, brush, m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawLine(b_seg.point2, b_seg.point3, brush, m_aux_width, m_aux_style.get());
			// �}�`�̕��ʂ�`��.
			loc_draw_circle(b_seg.point2, target, brush);
			loc_draw_circle(b_seg.point1, target, brush);
			loc_draw_square(m_start, target, brush);
			loc_draw_square(b_seg.point3, target, brush);
		}
	}

	// �}�`���_���܂ނ����肷��.
	// �߂�l	�_���܂ޕ���
	uint32_t ShapeBezier::hit_test(const D2D1_POINT_2F pt, const bool/*ctrl_key*/) const noexcept
	{
		const auto f_opaque = is_opaque(m_fill_color);
		bool f_test = false;	// �ʒu���h��Ԃ��Ɋ܂܂�邩����
		const auto ew = max(max(static_cast<double>(m_stroke_width), m_loc_width) * 0.5, 0.5);	// ���g�̑����̔����̒l
		const D2D1_POINT_2F tp{
			pt.x - m_start.x, pt.y - m_start.y
		};
		//pt_sub(pt, m_start, tp);
		// ���肳���_�ɂ���Đ��x�������Ȃ��悤, �Ȑ��̎n�_�����_�ƂȂ�悤���s�ړ���, ����_�𓾂�.
		D2D1_POINT_2F cp[4];
		cp[0].x = cp[0].y = 0.0;
		pt_add(cp[0], m_pos[0], cp[1]);
		pt_add(cp[1], m_pos[1], cp[2]);
		pt_add(cp[2], m_pos[2], cp[3]);
		if (loc_hit_test(tp, cp[3], m_loc_width)) {
			return LOC_TYPE::LOC_P0 + 3;
		}
		if (loc_hit_test(tp, cp[2], m_loc_width)) {
			return LOC_TYPE::LOC_P0 + 2;
		}
		if (loc_hit_test(tp, cp[1], m_loc_width)) {
			return LOC_TYPE::LOC_P0 + 1;
		}
		if (loc_hit_test(tp, cp[0], m_loc_width)) {
			return LOC_TYPE::LOC_P0 + 0;
		}
		if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND)) {
		//if (equal(m_stroke_cap, CAP_STYLE_ROUND)) {
			if (pt_in_circle(tp.x, tp.y, ew)) {
				return LOC_TYPE::LOC_STROKE;
			}
			if (pt_in_circle(tp, cp[3], ew)) {
				return LOC_TYPE::LOC_STROKE;
			}
		}
		else if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE)) {
			if (bezi_hit_test_cap<D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE>(
				tp, cp, m_pos.data(), ew)) {
				return LOC_TYPE::LOC_STROKE;
			}
		}
		else if (equal(m_stroke_cap, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE)) {
			if (bezi_hit_test_cap<D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE>(
				tp, cp, m_pos.data(), ew)) {
				return LOC_TYPE::LOC_STROKE;
			}
		}
		// �ŏ��̐���_�̑g���v�b�V������.
		// �S�̓_�̂����[�_��, ���ɂ܂��g�Ƌ��L����̂�, 1 + 3 * D_MAX �̔z����m�ۂ���.
		constexpr int32_t D_MAX = 64;	// ��������[���̍ő�l
		POINT_2D s[1 + D_MAX * 3] {};	// ����_�̃X�^�b�N
		int32_t s_cnt = 0;	// �X�^�b�N�ɐς܂ꂽ�_�̐�
		s[0] = cp[0];
		s[1] = cp[1];
		s[2] = cp[2];
		s[3] = cp[3];
		s_cnt += 4;
		// �X�^�b�N�� ��g�ȏ�̐���_���c���Ă��邩���肷��.
		while (s_cnt >= 4) {
			// ����_���|�b�v����.
			POINT_2D c[10];
			c[3] = s[s_cnt - 1];
			c[2] = s[s_cnt - 2];
			c[1] = s[s_cnt - 3];
			c[0] = s[s_cnt - 4];
			// �[�_�͋��L�Ȃ̂Ńs�[�N����;
			s_cnt -= 4 - 1;
			// ����_�̑g����ʕ� c0 �𓾂� (���ۂ͕��`�ő�p����).
			// ����_�̑g����, �d��������̂��������_�̏W���𓾂�.
			POINT_2D c0_lt = c[0];	// �ʕ� c0 (���܂ޕ��`�̍���_)
			POINT_2D c0_rb = c[0];	// �ʕ� c0 (���܂ޕ��`�̉E���_)
			POINT_2D d[4];	// �d�����Ȃ��_�̏W��.
			uint32_t d_cnt = 0;	// �d�����Ȃ��_�̏W���̗v�f��
			d[d_cnt++] = c[0];
			for (uint32_t i = 1; i < 4; i++) {
				if (d[d_cnt - 1] != c[i]) {
					d[d_cnt++] = c[i];
					c[i].exp(c0_lt, c0_rb);
				}
			}
			// �d�����Ȃ��_�̏W���̗v�f���� 2 ���������肷��.
			if (d_cnt < 2) {
				// ����_�̑g�� 1 �_�ɏW�܂��Ă���̂�, ���f����.
				continue;
			}

			// �g���E�������ꂽ�����𓾂�.
			//   e[i][0]             e[i][1]
			//        + - - - - - - - - - +
			//        |           orth|
			//   d[i] +---------------+---> dir
			//        |           d[i+1]
			//        + - - - - - - - - - +
			//   e[i][3]             e[i][2]
			POINT_2D e[3 * 4];	// �g���E�������ꂽ�����̔z��
			for (uint32_t i = 0, j = 0; i < d_cnt - 1; i++, j += 4) {
				auto e_dir = d[i + 1] - d[i];	// ���̒��_�ւ̈ʒu�x�N�g��
				e_dir = e_dir * (ew / std::sqrt(e_dir * e_dir));
				const POINT_2D e_orth{ e_dir.y, -e_dir.x };	// �����̒����x�N�g��

				// �@���x�N�g���ɂ����Đ��t�̕����ɐ������g������.
				e[j + 0] = d[i] + e_orth;
				e[j + 1] = d[i + 1] + e_orth;
				e[j + 2] = d[i + 1] - e_orth;
				e[j + 3] = d[i] - e_orth;
				if (i > 0) {
					// �ŏ��̐���_�ȊO��, �����x�N�g���̕����ɉ�������.
					e[j + 0] = e[j + 0] - e_dir;
					e[j + 3] = e[j + 3] - e_dir;
				}
				if (i + 1 < d_cnt - 1) {
					// �Ō�̐���_�ȊO��, �����x�N�g���̋t�����ɉ�������.
					e[j + 1] = e[j + 1] + e_dir;
					e[j + 2] = e[j + 2] + e_dir;
				}
			}
			// �g���E�������ꂽ��������, �ʕ� c1 �𓾂�.
			uint32_t c1_cnt;
			POINT_2D c1[3 * 4];
			bezi_get_convex((d_cnt - 1) * 4, e, c1_cnt, c1);
			// �_���ʕ� c1 �Ɋ܂܂�Ȃ������肷��.
			if (!bezi_in_convex(tp.x, tp.y, c1_cnt, c1)) {
				// ����ȏケ�̐���_�̑g�𕪊�����K�v�͂Ȃ�.
				// �X�^�b�N�Ɏc�������̐���_�̑g������.
				continue;
			}

			// �ʕ� c0 �̑傫���� 1 �ȉ������肷��.
			POINT_2D c0 = c0_rb - c0_lt;
			if (c0.x <= 1.0 && c0.y <= 1.0) {
				// ���݂̐���_�̑g (�ʕ� c0) ������ȏ㕪������K�v�͂Ȃ�.
				// �ʕ� c1 �͔��肳���_���܂�ł���̂�, �}�`�̕��ʂ�Ԃ�.
				return LOC_TYPE::LOC_STROKE;
			}

			// �X�^�b�N���I�o�[�t���[���邩���肷��.
			if (s_cnt + 6 > 1 + D_MAX * 3) {
				// ���݂̐���_�̑g (�ʕ� c0) ������ȏ㕪�����邱�Ƃ͂ł��Ȃ�.
				// �ʕ� c1�͔��肳���_���܂�ł���̂�, �}�`�̕��ʂ�Ԃ�.
				return LOC_TYPE::LOC_STROKE;
			}

			// ����_�̑g�� 2 ��������.
			// c[0,1,2,3] �̒��_�� c[4,5,6] ��, c[4,5,6] �̒��_�� c[7,8] ��, c[7,8] �̒��_�� c[9] �Ɋi�[����.
			// ������������_�̑g�͂��ꂼ�� c[0,4,7,9] �� c[9,8,6,3] �ɂȂ�.
			c[4] = (c[0] + c[1]) * 0.5;
			c[5] = (c[1] + c[2]) * 0.5;
			c[6] = (c[2] + c[3]) * 0.5;
			c[7] = (c[4] + c[5]) * 0.5;
			c[8] = (c[5] + c[6]) * 0.5;
			c[9] = (c[7] + c[8]) * 0.5;
			if (f_opaque && !f_test) {
				// �������ꂽ�ʕ�̂������ɂł����O�p�`��, �h��Ԃ��̗̈�.
				// ���̗̈�ɓ_���܂܂�邩, �������邽�тɔ��肷��.
				// ������ 1 �x�ł��܂܂��Ȃ�, ����ȏ�̔���͕K�v�Ȃ�.
				const POINT_2D tri[3]{ 
					c[0], c[9], c[3] 
				};
				f_test = bezi_in_convex(tp.x, tp.y, 3, tri);
			}
			// ����̑g���v�b�V������.
			// �n�_ (0) �̓X�^�b�N�Ɏc���Ă���̂�, 
			// �c��� 3 �̐���_���v�b�V������.
			s[s_cnt] = c[4];
			s[s_cnt + 1] = c[7];
			s[s_cnt + 2] = c[9];
			// ��������̑g���v�b�V������.
			// �n�_ (9) �̓v�b�V���ς݂Ȃ̂�,
			// �c��� 3 �̐���_���v�b�V������.
			s[s_cnt + 3] = c[8];
			s[s_cnt + 4] = c[6];
			s[s_cnt + 5] = c[3];
			s_cnt += 6;
		}
		if (f_opaque && f_test) {
			return LOC_TYPE::LOC_FILL;
		}
		return LOC_TYPE::LOC_PAGE;
	}

	//------------------------------
	// ��`�Ɋ܂܂�邩���肷��.
	// ���̑����͍l������Ȃ�.
	// lt	�͈͂̍���ʒu
	// rb	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	//------------------------------
	bool ShapeBezier::is_inside(
		const D2D1_POINT_2F lt, const D2D1_POINT_2F rb) const noexcept
	{
		// �v�Z���x���Ȃ�ׂ��ς��Ȃ��悤,
		// �͈͂̍��オ���_�ƂȂ�悤���s�ړ���������_�𓾂�.
		const double w = static_cast<double>(rb.x) - lt.x;
		const double h = static_cast<double>(rb.y) - lt.y;
		D2D1_POINT_2F cp[4];
		pt_sub(m_start, lt, cp[0]);
		pt_add(cp[0], m_pos[0], cp[1]);
		pt_add(cp[1], m_pos[1], cp[2]);
		pt_add(cp[2], m_pos[2], cp[3]);
		// �ŏ��̐���_�̑g���v�b�V������.
		constexpr auto D_MAX = 52;	// ��������ő��
		POINT_2D s_arr[1 + D_MAX * 3] = {};
		int32_t s_cnt = 4;
		s_arr[0] = cp[0];
		s_arr[1] = cp[1];
		s_arr[2] = cp[2];
		s_arr[3] = cp[3];
		while (s_cnt >= 4) {
			// �X�^�b�N����łȂ��Ȃ�, ����_�̑g���|�b�v����.
			// �[�_�͋��L�Ȃ̂Ńs�[�N����.
			POINT_2D c[10];
			c[3] = s_arr[s_cnt - 1];
			c[2] = s_arr[s_cnt - 2];
			c[1] = s_arr[s_cnt - 3];
			c[0] = s_arr[s_cnt - 4];
			s_cnt -= 3;
			// �n�_���͈͂̊O�ɂ���Ȃ�Ȑ��͔͈͂Ɋ܂܂�Ȃ�.
			if (c[0].x < 0.0 || w < c[0].x) {
				return false;
			}
			if (c[0].y < 0.0 || h < c[0].y) {
				return false;
			}
			// �I�_���͈͂̊O�ɂ���Ȃ�Ȑ��͔͈͂Ɋ܂܂�Ȃ�.
			if (c[3].x < 0.0 || w < c[3].x) {
				return false;
			}
			if (c[3].y < 0.0 || h < c[3].y) {
				return false;
			}
			// ���� 2 �̐���_���͈͓��Ȃ�, �Ȑ��̂��̕����͔͈͂Ɋ܂܂��.
			// ����ɕ�������K�v�͂Ȃ��̂�, �X�^�b�N�̎c��̑g�ɂ��Ĕ��肷��.
			if (0.0 <= c[1].x && c[1].x <= w && 0.0 <= c[1].y && c[1].y <= h) {
				if (0.0 <= c[2].x && c[2].x <= w && 0.0 <= c[2].y && c[2].y <= h) {
					continue;
				}
			}
			// ����_���܂ޗ̈�𓾂�.
			POINT_2D b_lt = c[0];
			POINT_2D b_rb = c[0];
			c[1].exp(b_lt, b_rb);
			c[2].exp(b_lt, b_rb);
			c[3].exp(b_lt, b_rb);
			POINT_2D d = b_rb - b_lt;
			if (d.x <= 1.0 && d.y <= 1.0) {
				// �̈�̊e�ӂ̑傫���� 1 �ȉ��Ȃ��, 
				// ����ȏ㕪������K�v�͂Ȃ�.
				// ����_�̏��Ȃ��Ƃ� 1 ���͈͂Ɋ܂܂�ĂȂ��̂����� false ��Ԃ�.
				return false;
			}
			if (s_cnt + 6 > 1 + D_MAX * 3) {
				// �X�^�b�N�I�o�[�t���[�Ȃ炱��ȏ㕪���ł��Ȃ�.
				// ����_�̏��Ȃ��Ƃ� 1 ���͈͂Ɋ܂܂�Ă��Ȃ��̂Ȃ� false ��Ԃ�.
				return false;
			}
			// ����_�̑g�� 2 ��������.
			c[4] = (c[0] + c[1]) * 0.5;
			c[5] = (c[1] + c[2]) * 0.5;
			c[6] = (c[2] + c[3]) * 0.5;
			c[7] = (c[4] + c[5]) * 0.5;
			c[8] = (c[5] + c[6]) * 0.5;
			c[9] = (c[7] + c[8]) * 0.5;
			// ����̑g���v�b�V������.
			// �n�_ (0) �̓X�^�b�N�Ɏc���Ă���̂�, 
			// �c��� 3 �̐���_���v�b�V������.
			s_arr[s_cnt] = c[4];
			s_arr[s_cnt + 1] = c[7];
			s_arr[s_cnt + 2] = c[9];
			// ��������̑g���v�b�V������.
			// �n�_ (9) �̓v�b�V���ς݂Ȃ̂�,
			// �c��� 3 �̐���_���v�b�V������.
			s_arr[s_cnt + 3] = c[8];
			s_arr[s_cnt + 4] = c[6];
			s_arr[s_cnt + 5] = c[3];
			s_cnt += 6;
		}
		return true;
	}

	// �}�`���쐬����.
	ShapeBezier::ShapeBezier(
		const D2D1_POINT_2F start,	// �n�_
		const D2D1_POINT_2F pos,	// �I�_�ւ̈ʒu�x�N�g��
		const Shape* prop	// ����
	) :
		ShapePath::ShapePath(prop, false)
	{
		m_start = start;
		m_pos.resize(3);
		m_pos.shrink_to_fit();
		m_pos[0] = D2D1_POINT_2F{ pos.x , 0.0f };
		m_pos[1] = D2D1_POINT_2F{ -pos.x , pos.y };
		m_pos[2] = D2D1_POINT_2F{ pos.x , 0.0f };
	}

	//------------------------------
	// �f�[�^���[�_�[����}�`��ǂݍ���.
	// dt_reader	�f�[�^���[�_�[
	//------------------------------
	ShapeBezier::ShapeBezier(DataReader const& dt_reader) :
		ShapePath::ShapePath(dt_reader)
	{}

	void ShapeBezier::write(const DataWriter& dt_writer) const
	{
		ShapePath::write(dt_writer);
	}

	/*
	static uint32_t clipping(const POINT_2D& p, const POINT_2D& q, double* t)
	{
		POINT_2D bz[10];
		POINT_2D br[2];
		POINT_2D pr[2];

		uint32_t t_cnt;
		POINT_2D br_mid;
		POINT_2D pr_next;
		double dist;
		POINT_2D pq;
		POINT_2D pb;
		double a, b, c, d;
		double f0, f1, f2, f3;
		double s;
		double t_min, t_max;
		std::list<POINT_2D> st;

		// �v�Z���x���Ȃ�ׂ����ɂ��邽��, �x�W�F����_ bz ��, ���̎n�_�����_�ƂȂ�悤���s�ړ�����.
		bz[0] = 0.0;
		bz[1].x = m_pos.x;
		bz[1].y = m_pos.y;
		bz[2].x = bz[1].x + m_pos_1.x;
		bz[2].y = bz[1].y + m_pos_1.y;
		bz[3].x = bz[2].x + m_pos_2.x;
		bz[3].y = bz[2].y + m_pos_2.y;
		// ���� pq �ɊO�ڂ�����` pr �����߂�.
		pb.x = p.x - m_start.x;
		pb.y = p.y - m_start.y;
		bezi_bound(p, q, pr);
		pr[0].x -= m_start.x;
		pr[0].y -= m_start.y;
		pr[1].x -= m_start.x;
		pr[1].y -= m_start.y;
		pr_next = pr[0].nextafter(1.0);
		pb = p - m_start;
		pq = q - pb;
		a = pq.y / pq.x;
		b = -1.0;
		c = -a * pb.x + pb.y;
		d = std::sqrt(a * a + b * b);
		// 0 �����ϐ��̌��Ɋi�[����.
		t_cnt = 0;
		// �x�W�F����_ b ���X�^�b�N�Ƀv�b�V������.
		bezi_push(st, bz);
		do {
			bezi_pop(st, bz);
			bezi_bound(bz, br);
			// ���` br �͕��` pr �̏��Ȃ��Ƃ��ꕔ�Əd�Ȃ� ?
			if (pr[1].x >= br[0].x && pr[0].x <= br[1].x
				&& pr[1].y >= br[0].y && pr[0].y <= br[1].y) {
				if (!bezi_dividable(br, br_mid)) {
					// ���� pq �� y ���ɂقڕ��s ?
					if (pr[1].x < pr_next.x) {
						dist = br_mid.x - p.x;
					}
					// ���� pq �� x ���ɂقڕ��s ?
					else if (pr[1].y < pr_next.y) {
						// ���_�ƒ��� pq �� Y �����̍����𒷂��Ɋi�[����.
						dist = br_mid.y - p.y;
					}
					else {
						// ���_�ƒ��� pq �̒��������߂�.
						dist = bezi_shortest_dist(br_mid, a, b, c, d);
					}
					if (dist < 1.0) {
						s = nearest(pb, pq, br_mid);
						if (s >= 0.0 && s <= 1.0) {
							if (t != nullptr) {
								t[t_cnt] = s;
							}
							t_cnt++;
						}
					}
				}
				else {
					// ���� pq �� y ���ɂقڕ��s ?
					if (pr[1].x < pr_next.x) {
						f0 = bz[0].x - pb.x;
						f1 = bz[1].x - pb.x;
						f2 = bz[2].x - pb.x;
						f3 = bz[3].x - pb.x;
					}
					// ���� pq �� x ���ɂقڕ��s ?
					else if (pr[1].y < pr_next.y) {
						// ���_�ƒ��� pq �� Y �����̍����𒷂��Ɋi�[����.
						f0 = bz[0].y - pb.y;
						f1 = bz[1].y - pb.y;
						f2 = bz[2].y - pb.y;
						f3 = bz[3].y - pb.y;
					}
					else {
						f0 = bezi_shortest_dist(bz[0], a, b, c, d);
						f1 = bezi_shortest_dist(bz[1], a, b, c, d);
						f2 = bezi_shortest_dist(bz[2], a, b, c, d);
						f3 = bezi_shortest_dist(bz[3], a, b, c, d);
					}
					// �ʕ� f �̏��Ȃ��Ƃ� 1 �� 0 �ȉ� ?
					// (�ϊ����ꂽ�ʕ�� t ���ƌ���� ?)
					if ((f0 <= 0.0
						|| f1 <= 0.0
						|| f2 <= 0.0
						|| f3 <= 0.0)
						// �ʕ� f �̏��Ȃ��Ƃ� 1 �� 0�ȏ� ?
						&& (f0 >= 0.0
							|| f1 >= 0.0
							|| f2 >= 0.0
							|| f3 >= 0.0)) {
						// 1 ����Ԃ̍ŏ��l tMin ��, 0 ���ő�l tMax �Ɋi�[����.
						t_min = T3;
						t_max = T0;
						// �ʕ�� t ���̌�_������, ��� tMin,tMax ���X�V.
						segment(T0, f0, T1, f1, t_min, t_max);
						segment(T0, f0, T2, f2, t_min, t_max);
						segment(T0, f0, T3, f3, t_min, t_max);
						segment(T1, f1, T2, f2, t_min, t_max);
						segment(T1, f1, T3, f3, t_min, t_max);
						segment(T2, f2, T3, f3, t_min, t_max);
						// ��Ԃ̍����� 0.5 �ȏ� ?
						if (t_max - t_min >= 0.5f) {
							// �Ȑ��𔼕��ɕ�����, 2 �g�̐���_�����߂�.
							bz[4] = (bz[0] + bz[1]) * 0.5;
							bz[5] = (bz[1] + bz[2]) * 0.5;
							bz[6] = (bz[2] + bz[3]) * 0.5;
							bz[7] = (bz[4] + bz[5]) * 0.5;
							bz[8] = (bz[5] + bz[6]) * 0.5;
							bz[9] = (bz[7] + bz[8]) * 0.5;
							// ����̑g�̐���_���X�^�b�N�Ƀv�b�V������.
							bezi_push(st, bz, 0, 4, 7, 9);
							// ��������̑g�̐���_���X�^�b�N�Ƀv�b�V������.
							bezi_push(st, bz, 9, 8, 6, 3);
						}
						// ��Ԃ̍����͂ق� 0 ?
						// tMax �� tMin �̕��������ŕ\���\�Ȏ��̐���菬 ?
						else if (t_max <= std::nextafter(t_min, t_min + 1.0)) {
							// ��Ԃ̎��ʂ����l���������ċȐ���̓_ bzP �����߂�.
							POINT_2D tp;
							bezi_point_by_param(bz, t_min, tp);
							/* ���� pq ��ɂ�����, �_ bzP �ɍł��߂��_�̏��ϐ��𓾂�.
							s = nearest(pb, pq, tp);
							// ���ϐ��� 0 �ȏ� 1 �ȉ� ?
							// (�_�͐�����ɂ��� ?)
							if (s >= 0.0 && s <= 1.0) {
								// �z�� t �� NULL �łȂ� ?
								if (t != nullptr) {
									// ���ϐ���z��� tCnt �ԖڂɊi�[����.
									t[t_cnt] = s;
								}
								// ���ϐ��̌� tCnt ���C���N�������g����.
								t_cnt++;
							}
						}
						/* �ő�l tMax �͂ق� 0 ���� ?
						else if (t_max > DBL_MIN) {
							// �ő�l tMax �ȉ��̐���_�����߂�.
							bz[4] = bz[0] + (bz[1] - bz[0]) * t_max;
							bz[5] = bz[1] + (bz[2] - bz[1]) * t_max;
							bz[6] = bz[2] + (bz[3] - bz[2]) * t_max;
							bz[7] = bz[4] + (bz[5] - bz[4]) * t_max;
							bz[8] = bz[5] + (bz[6] - bz[5]) * t_max;
							bz[9] = bz[7] + (bz[8] - bz[7]) * t_max;
							// �ŏ��l tMin ���ő�l tMax �ŏ����ĕ␳����.
							t_min /= t_max;
							// �␳���� tMin �ȏ�̐���_�����߂�.
							bz[1] = bz[0] + (bz[4] - bz[0]) * t_max;
							bz[2] = bz[4] + (bz[7] - bz[4]) * t_max;
							bz[3] = bz[7] + (bz[9] - bz[7]) * t_max;
							bz[5] = bz[1] + (bz[2] - bz[1]) * t_max;
							bz[6] = bz[2] + (bz[3] - bz[2]) * t_max;
							bz[8] = bz[5] + (bz[6] - bz[5]) * t_max;
							// ���߂�����_���X�^�b�N�Ƀv�b�V������.
							bezi_push(st, bz, 8, 6, 3, 9);
						}
					}
				}
			}
		} while (!st.empty());
		return t_cnt;
	}
	*/

	// �_�ƒ����̊Ԃ̍ŒZ���������߂�.
	/*
	static double bezi_shortest_dist(const POINT_2D& p, const double a, const double b, const double c, const double d) noexcept
	{
		return (a * p.x + b * p.y + c) / d;
	}
	*/

	// ��_���͂ޕ��`�𓾂�.
	/*
	static void bezi_bound(const POINT_2D& p, const POINT_2D& q, POINT_2D r[2])
	{
		if (p.x < q.x) {
			r[0].x = p.x;
			r[1].x = q.x;
		}
		else {
			r[0].x = q.x;
			r[1].x = p.x;
		}
		if (p.y < q.y) {
			r[0].y = p.y;
			r[1].y = q.y;
		}
		else {
			r[0].y = q.y;
			r[1].y = p.y;
		}
	}
	*/

	// �l�̓_���͂ޕ��`�𓾂�.
	/*
	static void bezi_bound(const POINT_2D bz[4], POINT_2D br[2])
	{
		bezi_bound(bz[0], bz[1], br);
		br[0].x = min(br[0].x, bz[2].x);
		br[0].y = min(br[0].y, bz[2].y);
		br[0].x = min(br[0].x, bz[3].x);
		br[0].y = min(br[0].y, bz[3].y);
		br[1].x = max(br[1].x, bz[2].x);
		br[1].y = max(br[1].y, bz[2].y);
		br[1].x = max(br[1].x, bz[3].x);
		br[1].y = max(br[1].y, bz[3].y);
	}
	*/

}