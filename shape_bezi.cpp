//------------------------------
// Shape_bezi.cpp
// �x�W�F�Ȑ�
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �܂���̒��_�̔z��
	constexpr uint32_t ANCH_BEZI[4]{
		ANCH_WHICH::ANCH_P3,//ANCH_R_SE,
		ANCH_WHICH::ANCH_P2,//ANCH_R_SW,
		ANCH_WHICH::ANCH_P1,//ANCH_R_NE
		ANCH_WHICH::ANCH_P0	//ANCH_WHICH::ANCH_R_NW
	};

	// �Z�O�����g�̋�؂鏕�ϐ��̒l
	constexpr double T0 = 0.0;
	constexpr double T1 = 1.0 / 3.0;
	constexpr double T2 = 2.0 / 3.0;
	constexpr double T3 = 1.0;

	// �V���v�\���@�̉�
	constexpr uint32_t SIMPSON_N = 30;

	// double �^�̒l�����ʒu
	// ShapeBase ���ł̂ݎg�p����.
	struct BZP {
		double x;
		double y;

		inline double dist(const double a, const double b, const double c, const double d) const noexcept { return (a * x + b * y + c) / d; }
		inline void inc(BZP& r_min, BZP& r_max) const noexcept
		{
			if (x < r_min.x) {
				r_min.x = x;
			}
			if (x > r_max.x) {
				r_max.x = x;
			}
			if (y < r_min.y) {
				r_min.y = y;
			}
			if (y > r_max.y) {
				r_max.y = y;
			}
		}
		inline BZP nextafter(const double d) const noexcept { return { std::nextafter(x, x + d), std::nextafter(y, y + d) }; }
		inline operator D2D1_POINT_2F(void) const noexcept { return { static_cast<FLOAT>(x), static_cast<FLOAT>(y) }; }
		inline BZP operator -(const BZP& q) const noexcept { return { x - q.x, y - q.y }; }
		inline BZP operator -(const D2D1_POINT_2F q) const noexcept { return { x - q.x, y - q.y }; }
		inline BZP operator -(void) const noexcept { return { -x, -y }; }
		inline BZP operator *(const double s) const noexcept { return { x * s, y * s }; }
		inline double operator *(const BZP& q) const noexcept { return x * q.x + y * q.y; }
		inline BZP operator +(const BZP& q) const noexcept { return { x + q.x, y + q.y }; }
		inline BZP operator +(const D2D1_POINT_2F p) const noexcept { return { x + p.x, y + p.y }; }
		inline bool operator <(const BZP& q) const noexcept { return x < q.x && y < q.y; }
		inline BZP operator =(const D2D1_POINT_2F p) noexcept { return { x = p.x, y = p.y }; }
		inline BZP operator =(const double s) noexcept { return { x = s, y = s }; }
		inline bool operator >(const BZP& q) const noexcept { return x > q.x&& y > q.y; }
	};

	// �������珕�ϐ��𓾂�.
	static double bz_by_length(const BZP bz[4], const double len) noexcept;

	// �Ȑ���̓_�����߂�.
	static void bz_by_param(const BZP bz[4], const double t, BZP& p) noexcept;

	// ���ϐ������ƂɋȐ���̔����l�����߂�.
	static double bz_derivative(const BZP bz[4], const double t) noexcept;

	// �Ȑ���̋�Ԃ̒����𓾂�.
	static double bz_simpson(const BZP bz[4], const double t_min, const double t_max, const uint32_t sim_n) noexcept;

	// 2 �̏��ϐ������0-1�̊ԂŐ��������ׂ�.
	static bool bz_test_param(const double t_min, const double t_max) noexcept;

	// ���ϐ�����Ȑ���̐ڐ��x�N�g�������߂�.
	static void bz_tvec_by_param(const BZP bz[4], const double t, BZP& t_vec) noexcept;

	// ���̗��[�_���v�Z����.
	static bool bz_calc_arrowhead(const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_SIZE a_size, D2D1_POINT_2F barbs[3]) noexcept;

	// �Ȑ��̃W�I���g���V���N�ɖ���ǉ�����.
	static void bz_create_arrow_geometry(ID2D1Factory3* factory, const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_STYLE a_style, const ARROW_SIZE a_size, ID2D1PathGeometry** a_geo);

	// ��_���͂ޕ��`�𓾂�.
	/*
	static void bz_bound(const BZP& p, const BZP& q, BZP r[2])
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
	static void bz_bound(const BZP bz[4], BZP br[2])
	{
		bz_bound(bz[0], bz[1], br);
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

	// �������珕�ϐ��𓾂�.
	// bz	�Ȑ�
	// len	����
	// �߂�l	����ꂽ���ϐ��̒l
	static double bz_by_length(const BZP bz[4], const double len) noexcept
	{
		double t;	// ���ϐ�
		double d;	// ���ϐ��̕ϕ�
		double e;	// �덷

		/* ��Ԃ̒��Ԓl 0.5 �����ϐ��Ɋi�[����. */
		t = 0.5;
		/* 0.25 �����ϐ��̕ϕ��Ɋi�[����. */
		/* ���ϐ��̕ϕ��� 0.001953125 �ȏ� ? */
		for (d = 0.25; d >= 0.001953125; d *= 0.5) {
			/* 0-���ϐ��͈̔͂������V���v�\�������Őϕ���, �Ȑ��̒��������߂�. */
			/* ���߂������Ǝw�肳�ꂽ�����̍������덷�Ɋi�[����. */
			e = bz_simpson(bz, 0.0, t, SIMPSON_N) - len;
			/* �덷�̐�΂� 0.125 ��菬 ? */
			if (fabs(e) < 0.125) {
				break;
			}
			/* �덷�� 0 ���� ? */
			else if (e > 0.0) {
				/* �ϕ������ϐ��������. */
				t -= d;
			}
			else {
				/* �ϕ������ϐ��ɑ���. */
				t += d;
			}
		}
		return t;
	}

	// �x�W�F�Ȑ���̈ʒu�����߂�.
	static void bz_by_param(const BZP bz[4], const double t, BZP& p) noexcept
	{
		const double s = 1.0 - t;
		const double ss = s * s;
		const double tt = t * t;
		p = bz[0] * s * ss
			+ bz[1] * 3.0 * ss * t
			+ bz[2] * 3.0 * s * tt
			+ bz[3] * t * tt;
	}

	// ���̗��[�_���v�Z����.
	// b_pos	�Ȑ��̊J�n�ʒu
	// b_seg	�Ȑ��̐���_
	// a_size	���̐��@
	// barbs[3]	�v�Z���ꂽ���[�_�Ɛ�[�_
	static bool bz_calc_arrowhead(const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_SIZE a_size, D2D1_POINT_2F barbs[3]) noexcept
	{
		BZP pos = {};
		BZP seg[3] = {};
		BZP bz[4] = {};

		pos = b_pos;
		seg[0] = b_seg.point1;
		seg[1] = b_seg.point2;
		seg[2] = b_seg.point3;
		// ���W�l�ɂ��덷�����Ȃ��ł���, �Ǝv����̂�,
		// �x�W�F�Ȑ����n�_�����_�ƂȂ�悤�ɕ��s�ړ�.
		bz[3] = 0.0;
		bz[2] = seg[0] - pos;
		bz[1] = seg[1] - pos;
		bz[0] = seg[2] - pos;
		auto b_len = bz_simpson(bz, 0.0, 1.0, SIMPSON_N);
		if (b_len > FLT_MIN) {
			// ���̐�[�̃I�t�Z�b�g, �܂��͋Ȑ��̒���, 
			// �ǂ��炩�Z������, ���ϐ��𓾂�.
			const auto t = bz_by_length(bz, min(b_len, a_size.m_offset));
			// ���ϐ������ƂɋȐ��̐ڐ��x�N�g���𓾂�.
			BZP t_vec;
			bz_tvec_by_param(bz, t, t_vec);
			// �ڐ��x�N�g�������Ƃ�����̕Ԃ��̈ʒu���v�Z����
			get_arrow_barbs(-t_vec,
				/*D2D1_POINT_2F(-t_vec),*/ sqrt(t_vec * t_vec), a_size.m_width, a_size.m_length, barbs);
			// ���ϐ��ŋȐ���̈ʒu�𓾂�.
			BZP t_pos;	// �I�_�����_�Ƃ���, ���̐�[�̈ʒu
			bz_by_param(bz, t, t_pos);
			// �Ȑ���̈ʒu����̐�[�Ƃ�, �Ԃ��̈ʒu�����s�ړ�����.
			pt_add(barbs[0], t_pos.x, t_pos.y, barbs[0]);
			pt_add(barbs[1], t_pos.x, t_pos.y, barbs[1]);
			barbs[2] = t_pos;
			//barbs[2].x = static_cast<FLOAT>(t_pos.x);
			//barbs[2].y = static_cast<FLOAT>(t_pos.y);
			pt_add(barbs[0], b_pos, barbs[0]);
			pt_add(barbs[1], b_pos, barbs[1]);
			pt_add(barbs[2], b_pos, barbs[2]);
			return true;
		}
		return false;
	}

	// �Ȑ��̃W�I���g���V���N�ɖ���ǉ�����
	// factory	D2D �t�@�N�g��
	// b_pos	�Ȑ��̊J�n�ʒu
	// b_seg	�Ȑ��̐���_
	// a_style	���̎��
	// a_size	���̐��@
	// a_geo	��肪�ǉ����ꂽ�W�I���g��
	static void bz_create_arrow_geometry(ID2D1Factory3* factory, const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROW_STYLE a_style, const ARROW_SIZE a_size, ID2D1PathGeometry** a_geo)
	{
		D2D1_POINT_2F barbs[3];	// ���̕Ԃ��̒[�_	
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (bz_calc_arrowhead(b_pos, b_seg, a_size, barbs)) {
			// �W�I���g���V���N�ɒǉ�����.
			winrt::check_hresult(
				factory->CreatePathGeometry(a_geo)
			);
			winrt::check_hresult(
				(*a_geo)->Open(sink.put())
			);
			sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(
				barbs[0],
				a_style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_BEGIN_FILLED
				: D2D1_FIGURE_BEGIN_HOLLOW
			);
			sink->AddLine(barbs[2]);
			sink->AddLine(barbs[1]);
			sink->EndFigure(
				a_style == ARROW_STYLE::FILLED
				? D2D1_FIGURE_END_CLOSED
				: D2D1_FIGURE_END_OPEN
			);
			sink->Close();
			sink = nullptr;
		}
	}

	// ���ϐ������ƂɃx�W�F�Ȑ���̔����l�����߂�.
	static double bz_derivative(const BZP bz[4], const double t) noexcept
	{
		BZP t_vec;

		// ���ϐ������ƂɃx�W�F�Ȑ���̐ڐ��x�N�g��������,
		// ���̐ڐ��x�N�g���̒�����Ԃ�.
		bz_tvec_by_param(bz, t, t_vec);
		return sqrt(t_vec * t_vec);
	}

	// ���`�������\���ǂ������肵, ���̏d�S�_��Ԃ�.
	/*
	static bool bz_dividable(const BZP r[2], BZP& mid)
	{
		mid = (r[0] + r[1]) * 0.5;
		return r[0] < mid.nextafter(-1.0) && r[1] > mid.nextafter(1.0);
	}
	*/

	// �X�^�b�N�����菜��
	/*
	static void bz_pop(std::list<BZP>& stack, BZP bz[4])
	{
		bz[3] = stack.back();
		stack.pop_back();
		bz[2] = stack.back();
		stack.pop_back();
		bz[1] = stack.back();
		stack.pop_back();
		bz[0] = stack.back();
		stack.pop_back();
	}
	*/

	// �X�^�b�N�ɐς�.
	// i0 = 0	�ŏ��ɐςވʒu�̓Y����
	// i1 = 1	2 �Ԗڂɐςވʒu�̓Y����
	// i2 = 2	3 �Ԗڂɐςވʒu�̓Y����
	// i3 = 3	4 �Ԗڂɐςވʒu�̓Y����
	/*
	static void bz_push(std::list<BZP>& stack, const BZP bz[4], const uint32_t i0 = 0, const uint32_t i1 = 1, const uint32_t i2 = 2, const uint32_t i3 = 3)
	{
		stack.push_back(bz[i0]);
		stack.push_back(bz[i1]);
		stack.push_back(bz[i2]);
		stack.push_back(bz[i3]);
	}
	*/

	// �Ȑ���̋�Ԃ̒����𓾂�.
	static double bz_simpson(const BZP bz[4], const double t_min, const double t_max, const uint32_t sim_n) noexcept
	{
		double t_diff;
		uint32_t n;
		double h;
		double a, b;
		double t;
		double b0, b2;
		double s;

		/* �͈͂̏�������͐������ǂ������ׂ�. */
		/* ���� ? */
		if (bz_test_param(t_min, t_max)) {
			/* �͈͏�� tMax -�͈͉��� tMin ������ tDiff �Ɋi�[����. */
			t_diff = t_max - t_min;
			/* ��Ԃ̕����� simN �� tDiff ����Z����. */
			/* ���̌��ʂ�؂�グ�Đ����l����. */
			/* �����l����Ԃ̔��� n �Ɋi�[����. */
			n = (int)std::ceil(t_diff * (double)sim_n);
			/* tDiff��2n ���K�� h �Ɋi�[����. */
			h = t_diff / (2.0 * n);
			/* 0 ����Ԗڂ̕�����Ԃ̍��v�l a �Ɋi�[����. */
			a = 0.0;
			/* 0 �������Ԗڂ̕�����Ԃ̍��v�l b �Ɋi�[����. */
			b = 0.0;
			/* tMin+h �����ϐ� t �Ɋi�[����. */
			t = t_min + h;
			/* 1 ��Y���� i �Ɋi�[����. */
			/* i �� n ��菬 ? */
			for (uint32_t i = 1; i < n; i++) {
				/* 2i-1 �Ԗڂ̕�����Ԃ̔����l������, a �ɉ�����. */
				a += bz_derivative(bz, t);
				/* �K�� h �����ϐ� t �ɉ�����. */
				t += h;
				/* 2i �Ԗڂ̕�����Ԃ̔����l������, b �ɉ�����. */
				b += bz_derivative(bz, t);
				/* �K�� h �����ϐ� t �ɉ�����. */
				t += h;
				/* i ���C���N�������g����. */
			}
			/* 2n-1 �Ԗڂ̕�����Ԃ̔����l������, a �ɉ�����. */
			a += bz_derivative(bz, t);
			/* 0 �Ԗڂ̕�����Ԃł̔����l������, b0 �Ɋi�[����. */
			b0 = bz_derivative(bz, t_min);
			/* 2n �Ԗڂ̕�����Ԃł̔����l������, b2 �Ɋi�[����. */
			b2 = bz_derivative(bz, t_max);
			/* (b0+4�~a+2�~b+b2)�~h/3 ������, �ϕ��l s �Ɋi�[����. */
			s = (b0 + 4.0 * a + 2.0 * b + b2) * h / 3.0f;
		}
		else {
			/* 0 ��ϕ��l s �Ɋi�[����. */
			s = 0.0;
		}
		/* s ��Ԃ�. */
		return s;
	}

	// �x�W�F�Ȑ��ƕ��`������邩�ǂ���, �Ȑ��𕪊���, ���ׂ�.
	/*
	static bool bz_splitting(const BZP BZ[4], const BZP pr[2])
	{
		BZP bz[10];
		BZP br[2];
		BZP br_mid;
		std::list<BZP> st;

		bz[0] = BZ[0];
		bz[1] = BZ[1];
		bz[2] = BZ[2];
		bz[3] = BZ[3];
		bz_push(st, bz);
		do {
			bz_pop(st, bz);
			bz_bound(bz, br);
			if (pr[0].x <= br[0].x && br[1].x <= pr[1].x
				&& pr[0].y <= br[0].y && br[1].y <= pr[1].y) {
				return true;
			}
			// ���` br �͕��` pr �̏��Ȃ��Ƃ��ꕔ�Əd�Ȃ� ?
			else if (br[1].x >= pr[0].x && br[1].y >= pr[0].y
				&& br[0].x <= pr[1].x && br[0].y <= pr[1].y) {
				if (bz_dividable(br, br_mid) != true) {
					return true;
				}
				bz[4] = (bz[0] + bz[1]) * 0.5;
				bz[5] = (bz[1] + bz[2]) * 0.5;
				bz[6] = (bz[2] + bz[3]) * 0.5;
				bz[7] = (bz[4] + bz[5]) * 0.5;
				bz[8] = (bz[5] + bz[6]) * 0.5;
				bz[9] = (bz[7] + bz[8]) * 0.5;
				// �������ꂽ�x�W�F����_���X�^�b�N�Ƀv�b�V������.
				bz_push(st, bz, 0, 4, 7, 9);
				bz_push(st, bz, 9, 8, 6, 3);
			}
		} while (st.empty() != true);
		return false;
	}
	*/

	// 2 �̏��ϐ������0-1�̊ԂŐ��������ׂ�.
	static bool bz_test_param(const double t_min, const double t_max) noexcept
	{
		return -DBL_MIN < t_min
			// �͈͂̏�� tMax �� 1+DBL_EPSILON ��菬 ?
			&& t_max < 1.0 + DBL_EPSILON
			// tMin ���傫���čł��߂��l�� tMax ��菬 ?
			&& std::nextafter(t_min, t_min + 1.0) < t_max;
	}

	// ���ϐ�����Ȑ���̐ڐ��x�N�g�������߂�.
	// bz	�Ȑ�
	// t	���ϐ�
	// t_vec	�ڐ��x�N�g��
	static void bz_tvec_by_param(const BZP bz[4], const double t, BZP& t_vec) noexcept
	{
		const double a = -3.0 * (1.0 - t) * (1.0 - t);
		const double b = 3.0 * (1.0 - t) * (1.0 - 3.0 * t);
		const double c = 3.0 * t * (2.0 - 3.0 * t);
		const double d = (3.0 * t * t);
		t_vec = bz[0] * a + bz[1] * b + bz[2] * c + bz[3] * d;
	}

	// �p�X�W�I���g�����쐬����.
	void ShapeBezi::create_path_geometry(void)
	{
		D2D1_BEZIER_SEGMENT b_seg;
		winrt::com_ptr<ID2D1GeometrySink> sink;

		m_poly_geom = nullptr;
		m_arrow_geom = nullptr;
		pt_add(m_pos, m_diff[0], b_seg.point1);
		pt_add(b_seg.point1, m_diff[1], b_seg.point2);
		pt_add(b_seg.point2, m_diff[2], b_seg.point3);
		winrt::check_hresult(
			s_d2d_factory->CreatePathGeometry(m_poly_geom.put())
		);
		m_poly_geom->Open(sink.put());
		sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
		sink->BeginFigure(m_pos, D2D1_FIGURE_BEGIN_HOLLOW);
		sink->AddBezier(b_seg);
		sink->EndFigure(D2D1_FIGURE_END_OPEN);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			bz_create_arrow_geometry(s_d2d_factory,
				m_pos, b_seg, m_arrow_style, m_arrow_size,
				m_arrow_geom.put());
		}
		sink->Close();
		sink = nullptr;
	}

	// �}�`��\������.
	void ShapeBezi::draw(SHAPE_DX& dx)
	{
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F e_pos;

		if (is_opaque(m_stroke_color)) {
			const auto sw = static_cast<FLOAT>(m_stroke_width);
			auto sb = dx.m_shape_brush.get();
			auto ss = m_d2d_stroke_style.get();
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawGeometry(m_poly_geom.get(), sb, sw, ss);
			if (m_arrow_style != ARROW_STYLE::NONE) {
				auto geo = m_arrow_geom.get();
				dx.m_d2dContext->DrawGeometry(geo, sb, sw, nullptr);
				if (m_arrow_style == ARROW_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(geo, sb, nullptr);
				}
			}
		}
		if (is_selected() != true) {
			return;
		}
		D2D1_MATRIX_3X2_F tran;
		dx.m_d2dContext->GetTransform(&tran);
		const auto sw = static_cast<FLOAT>(1.0 / tran.m11);
		//auto sb = dx.m_anch_brush.get();
		//auto ss = dx.m_aux_style.get();

		anchor_draw_rect(m_pos, dx);
		s_pos = m_pos;
		pt_add(s_pos, m_diff[0], e_pos);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		anchor_draw_ellipse(e_pos, dx);

		s_pos = e_pos;
		pt_add(s_pos, m_diff[1], e_pos);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		anchor_draw_ellipse(e_pos, dx);

		s_pos = e_pos;
		pt_add(s_pos, m_diff[2], e_pos);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->DrawLine(s_pos, e_pos, dx.m_shape_brush.get(), sw, dx.m_aux_style.get());
		anchor_draw_rect(e_pos, dx);
	}

	// ���̐��@�𓾂�.
	bool ShapeBezi::get_arrow_size(ARROW_SIZE& value) const noexcept
	{
		value = m_arrow_size;
		return true;
	}

	// ���̌`���𓾂�.
	bool ShapeBezi::get_arrow_style(ARROW_STYLE& value) const noexcept
	{
		value = m_arrow_style;
		return true;
	}

	// �ʒu���܂ނ����ׂ�.
	// t_pos	���ׂ�ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���
	uint32_t ShapeBezi::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		// �v�Z���x���Ȃ�ׂ��ς��Ȃ��悤,
		// ���ׂ�ʒu�����_�ƂȂ�悤���s�ړ���, ����_�𓾂�.
		D2D1_POINT_2F a_pos[4];
		pt_sub(m_pos, t_pos, a_pos[3]);
		pt_add(a_pos[3], m_diff[0], a_pos[2]);
		pt_add(a_pos[2], m_diff[1], a_pos[1]);
		pt_add(a_pos[1], m_diff[2], a_pos[0]);
		// ����_�̊e���ʂɂ���,
		for (uint32_t i = 0; i < 4; i++) {
			// ���ʂ��ʒu���܂ނ����ׂ�.
			if (pt_in_anch(a_pos[i], a_len)) {
				// �܂ނȂ炻�̕��ʂ�Ԃ�.
				return ANCH_BEZI[i];
			}
		}
		const auto s_width = max(max(m_stroke_width, a_len) * 0.5, 0.5);
		// �ŏ��̐���_�̑g���v�b�V������.
		constexpr auto D_MAX = 52;	// ��������ő��
		BZP s_arr[1 + D_MAX * 3] = {};
		int32_t s_cnt = 4;
		s_arr[0] = a_pos[0];
		s_arr[1] = a_pos[1];
		s_arr[2] = a_pos[2];
		s_arr[3] = a_pos[3];
		while (s_cnt >= 4) {
			// �X�^�b�N����łȂ��Ȃ�, ����_�̑g���|�b�v����.
			// �[�_�͋��L�Ȃ̂Ńs�[�N����.
			BZP b_pos[10];
			b_pos[3] = s_arr[s_cnt - 1];
			b_pos[2] = s_arr[s_cnt - 2];
			b_pos[1] = s_arr[s_cnt - 3];
			b_pos[0] = s_arr[s_cnt - 4];
			s_cnt -= 3;
			// ����_�̑g�Ɋ܂܂��e�_�ɂ���,
			// �܂�, �ŏ��ƍŌ�̐���_���܂ޓʕ� b �� q �𓾂�.
			// �ʕ� b ��, ����_�̑g�������ł��邩���ׂ邽�ߎg�p����.
			// �ʕ� q ��, ���g�̑����𔽉f�����ʒu���܂ނ����ׂ邽�ߎg�p����.
			// �ȒP�ɂ��邽�ߓʕ�łȂ��A���`�ő�p����.
			BZP b_min = b_pos[0];
			BZP b_max = b_pos[3];
			BZP q_min = b_pos[0];
			BZP q_max = b_pos[3];
			for (int i = 0; i < 3; i++) {
				// ����_�Ǝ��̐���_�̊Ԃ̍����𓾂�.
				BZP b_vec = b_pos[i + 1] - b_pos[i];
				//double v = b_vec.abs();
				const auto v = b_vec * b_vec;
				if (v <= FLT_MIN) {
					// �����̒����� FLT_MIN �����Ȃ�, 
					// ���̐���_�͖������ēʕ�ɂ͉����Ȃ�.
					continue;
				}
				// ���̐���_��ʕ� b �ɉ�����.
				b_pos[i + 1].inc(b_min, b_max);
				// �����ƒ��s����@���x�N�g���𓾂�.
				// ����_�Ǝ��̐���_��@���ɂ����Đ��t�̗������ɂ��炷.
				// ����ꂽ 4 �_��ʕ� q �ɉ�����.
				b_vec = b_vec * (s_width / std::sqrt(v));
				BZP n_vec = { b_vec.y, -b_vec.x };
				BZP q_pos;
				q_pos = b_pos[i] + n_vec;
				q_pos.inc(q_min, q_max);
				q_pos = b_pos[i + 1] + n_vec;
				q_pos.inc(q_min, q_max);
				q_pos = b_pos[i + 1] - n_vec;
				q_pos.inc(q_min, q_max);
				q_pos = b_pos[i] - n_vec;
				q_pos.inc(q_min, q_max);
			}
			if (q_min.x > 0.0 || 0.0 > q_max.x || q_min.y > 0.0 || 0.0 > q_max.y) {
				// ���ׂ�ʒu���ʕ� q �Ɋ܂܂�Ȃ��Ȃ�,
				// ���̐���_�̑g�͈ʒu���܂܂Ȃ��̂Ŗ�������.
				continue;
			}
			BZP d = b_max - b_min;
			if (d.x <= 1.0 && d.y <= 1.0) {
				// �ʕ� b �̊e�ӂ̑傫���� 1 �ȉ��Ȃ��,
				// �Ȑ��͒��ׂ�ʒu���܂ނƂ݂Ȃ�.
				// (����ȏ㕪�����Ă����ʂɉe�����Ȃ�)
				// ANCH_FRAME ��Ԃ�.
				return ANCH_WHICH::ANCH_FRAME;
			}
			// �X�^�b�N���I�o�[�t���[����Ȃ�, �X�^�b�N�ɐς܂�Ă���
			// ���̐���_�̑g�𒲂ׂ�.
			if (s_cnt + 6 > 1 + D_MAX * 3) {
				continue;
			}
			// ����_�̑g�� 2 ��������.
			b_pos[4] = (b_pos[0] + b_pos[1]) * 0.5;
			b_pos[5] = (b_pos[1] + b_pos[2]) * 0.5;
			b_pos[6] = (b_pos[2] + b_pos[3]) * 0.5;
			b_pos[7] = (b_pos[4] + b_pos[5]) * 0.5;
			b_pos[8] = (b_pos[5] + b_pos[6]) * 0.5;
			b_pos[9] = (b_pos[7] + b_pos[8]) * 0.5;
			// ����̑g���v�b�V������.
			// �n�_ (0) �̓X�^�b�N�Ɏc���Ă���̂�, 
			// �c��� 3 �̐���_���v�b�V������.
			s_arr[s_cnt] = b_pos[4];
			s_arr[s_cnt + 1] = b_pos[7];
			s_arr[s_cnt + 2] = b_pos[9];
			// ��������̑g���v�b�V������.
			// �n�_ (9) �̓v�b�V���ς݂Ȃ̂�,
			// �c��� 3 �̐���_���v�b�V������.
			s_arr[s_cnt + 3] = b_pos[8];
			s_arr[s_cnt + 4] = b_pos[6];
			s_arr[s_cnt + 5] = b_pos[3];
			s_cnt += 6;
		}
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// �͈͂Ɋ܂܂�邩���ׂ�.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeBezi::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		// �v�Z���x���Ȃ�ׂ��ς��Ȃ��悤,
		// �͈͂̍��オ���_�ƂȂ�悤���s�ړ���������_�𓾂�.
		const auto w = static_cast<double>(a_max.x) - a_min.x;
		const auto h = static_cast<double>(a_max.y) - a_min.y;
		D2D1_POINT_2F a_pos[4];
		pt_sub(m_pos, a_min, a_pos[0]);
		pt_add(a_pos[0], m_diff[0], a_pos[1]);
		pt_add(a_pos[1], m_diff[1], a_pos[2]);
		pt_add(a_pos[2], m_diff[2], a_pos[3]);
		// �ŏ��̐���_�̑g���v�b�V������.
		constexpr auto D_MAX = 52;	// ��������ő��
		BZP s_arr[1 + D_MAX * 3] = {};
		int32_t s_cnt = 4;
		s_arr[0] = a_pos[0];
		s_arr[1] = a_pos[1];
		s_arr[2] = a_pos[2];
		s_arr[3] = a_pos[3];
		while (s_cnt >= 4) {
			// �X�^�b�N����łȂ��Ȃ�, ����_�̑g���|�b�v����.
			// �[�_�͋��L�Ȃ̂Ńs�[�N����.
			BZP b_pos[10];
			b_pos[3] = s_arr[s_cnt - 1];
			b_pos[2] = s_arr[s_cnt - 2];
			b_pos[1] = s_arr[s_cnt - 3];
			b_pos[0] = s_arr[s_cnt - 4];
			s_cnt -= 3;
			// �n�_���͈͂̊O�ɂ���Ȃ�Ȑ��͔͈͂Ɋ܂܂�Ȃ�.
			if (b_pos[0].x < 0.0 || w < b_pos[0].x) {
				return false;
			}
			if (b_pos[0].y < 0.0 || h < b_pos[0].y) {
				return false;
			}
			// �I�_���͈͂̊O�ɂ���Ȃ�Ȑ��͔͈͂Ɋ܂܂�Ȃ�.
			if (b_pos[3].x < 0.0 || w < b_pos[3].x) {
				return false;
			}
			if (b_pos[3].y < 0.0 || h < b_pos[3].y) {
				return false;
			}
			// ���� 2 �̐���_���͈͓��Ȃ�, �Ȑ��̂��̕����͔͈͂Ɋ܂܂��.
			// ����ɕ�������K�v�͂Ȃ��̂�, �X�^�b�N�̎c��̑g�ɂ��Ē��ׂ�.
			if (0.0 <= b_pos[1].x && b_pos[1].x <= w && 0.0 <= b_pos[1].y && b_pos[1].y <= h) {
				if (0.0 <= b_pos[2].x && b_pos[2].x <= w && 0.0 <= b_pos[2].y && b_pos[2].y <= h) {
					continue;
				}
			}
			// ����_���܂ޗ̈�𓾂�.
			BZP b_min = b_pos[0];
			BZP b_max = b_pos[0];
			b_pos[1].inc(b_min, b_max);
			b_pos[2].inc(b_min, b_max);
			b_pos[3].inc(b_min, b_max);
			BZP d = b_max - b_min;
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
			b_pos[4] = (b_pos[0] + b_pos[1]) * 0.5;
			b_pos[5] = (b_pos[1] + b_pos[2]) * 0.5;
			b_pos[6] = (b_pos[2] + b_pos[3]) * 0.5;
			b_pos[7] = (b_pos[4] + b_pos[5]) * 0.5;
			b_pos[8] = (b_pos[5] + b_pos[6]) * 0.5;
			b_pos[9] = (b_pos[7] + b_pos[8]) * 0.5;
			// ����̑g���v�b�V������.
			// �n�_ (0) �̓X�^�b�N�Ɏc���Ă���̂�, 
			// �c��� 3 �̐���_���v�b�V������.
			s_arr[s_cnt] = b_pos[4];
			s_arr[s_cnt + 1] = b_pos[7];
			s_arr[s_cnt + 2] = b_pos[9];
			// ��������̑g���v�b�V������.
			// �n�_ (9) �̓v�b�V���ς݂Ȃ̂�,
			// �c��� 3 �̐���_���v�b�V������.
			s_arr[s_cnt + 3] = b_pos[8];
			s_arr[s_cnt + 4] = b_pos[6];
			s_arr[s_cnt + 5] = b_pos[3];
			s_cnt += 6;
		}
		return true;
	}

	// ���̐��@�Ɋi�[����.
	void ShapeBezi::set_arrow_size(const ARROW_SIZE& value)
	{
		if (equal(m_arrow_size, value)) {
			return;
		}
		m_arrow_size = value;
		create_path_geometry();
	}

	// ���̌`���Ɋi�[����.
	void ShapeBezi::set_arrow_style(const ARROW_STYLE value)
	{
		if (m_arrow_style == value) {
			return;
		}
		m_arrow_style = value;
		create_path_geometry();
	}

	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	void ShapeBezi::set_start_pos(const D2D1_POINT_2F value)
	{
		ShapeStroke::set_start_pos(value);
		create_path_geometry();
	}

	// �}�`���쐬����.
	ShapeBezi::ShapeBezi(const D2D1_POINT_2F s_pos, const D2D1_POINT_2F diff, const ShapeSheet* attr) :
		ShapePoly::ShapePoly(3, attr)
	{
		m_pos = s_pos;
		m_diff[0].x = diff.x;
		m_diff[0].y = 0.0f;
		m_diff[1].x = -diff.x;
		m_diff[1].y = diff.y;
		m_diff[2].x = diff.x;
		m_diff[2].y = 0.0f;
		m_arrow_style = attr->m_arrow_style;
		m_arrow_size = attr->m_arrow_size;
		create_path_geometry();
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeBezi::ShapeBezi(DataReader const& dt_reader) :
		ShapePoly::ShapePoly(dt_reader)
	{
		m_arrow_style = static_cast<ARROW_STYLE>(dt_reader.ReadUInt32());
		m_arrow_size.m_width = dt_reader.ReadSingle();
		m_arrow_size.m_length = dt_reader.ReadSingle();
		m_arrow_size.m_offset = dt_reader.ReadSingle();
		create_path_geometry();
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeBezi::write(DataWriter const& dt_writer) const
	{
		//w.WriteInt32(static_cast<int32_t>(SHAPE_BEZI));
		ShapePoly::write(dt_writer);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
		dt_writer.WriteSingle(m_arrow_size.m_width);
		dt_writer.WriteSingle(m_arrow_size.m_length);
		dt_writer.WriteSingle(m_arrow_size.m_offset);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeBezi::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;
		D2D1_BEZIER_SEGMENT b_seg;

		pt_add(m_pos, m_diff[0], b_seg.point1);
		pt_add(b_seg.point1, m_diff[1], b_seg.point2);
		pt_add(b_seg.point2, m_diff[2], b_seg.point3);
		write_svg("<path d=\"", dt_writer);
		write_svg(m_pos, "M", dt_writer);
		write_svg(b_seg.point1, "C", dt_writer);
		write_svg(b_seg.point2, ",", dt_writer);
		write_svg(b_seg.point3, ",", dt_writer);
		write_svg("\" ", dt_writer);
		write_svg("none", "fill", dt_writer);
		ShapeStroke::write_svg(dt_writer);
		write_svg("/>" SVG_NL, dt_writer);
		if (m_arrow_style != ARROW_STYLE::NONE) {
			D2D1_POINT_2F barbs[3];
			bz_calc_arrowhead(m_pos, b_seg, m_arrow_size, barbs);
			ShapeStroke::write_svg(barbs, barbs[2], m_arrow_style, dt_writer);
		}
	}

	/*
	static uint32_t clipping(
		const BZP& p, const BZP& q, double* t)
	{
		BZP bz[10];
		BZP br[2];
		BZP pr[2];

		uint32_t t_cnt;
		BZP br_mid;
		BZP pr_next;
		double dist;
		BZP pq;
		BZP pb;
		double a, b, c, d;
		double f0, f1, f2, f3;
		double s;
		double t_min, t_max;
		std::list<BZP> st;

		// �v�Z���x���Ȃ�ׂ����ɂ��邽��, �x�W�F����_ bz ��, ���̎n�_�����_�ƂȂ�悤���s�ړ�����.
		bz[0] = 0.0;
		bz[1].x = m_diff.x;
		bz[1].y = m_diff.y;
		bz[2].x = bz[1].x + m_diff_1.x;
		bz[2].y = bz[1].y + m_diff_1.y;
		bz[3].x = bz[2].x + m_diff_2.x;
		bz[3].y = bz[2].y + m_diff_2.y;
		// ���� pq �ɊO�ڂ�����` pr �����߂�.
		pb.x = p.x - m_pos.x;
		pb.y = p.y - m_pos.y;
		bz_bound(p, q, pr);
		pr[0].x -= m_pos.x;
		pr[0].y -= m_pos.y;
		pr[1].x -= m_pos.x;
		pr[1].y -= m_pos.y;
		pr_next = pr[0].nextafter(1.0);
		pb = p - m_pos;
		pq = q - pb;
		a = pq.y / pq.x;
		b = -1.0;
		c = -a * pb.x + pb.y;
		d = std::sqrt(a * a + b * b);
		// 0 �����ϐ��̌��Ɋi�[����.
		t_cnt = 0;
		// �x�W�F����_ b ���X�^�b�N�Ƀv�b�V������.
		bz_push(st, bz);
		do {
			bz_pop(st, bz);
			bz_bound(bz, br);
			// ���` br �͕��` pr �̏��Ȃ��Ƃ��ꕔ�Əd�Ȃ� ?
			if (pr[1].x >= br[0].x && pr[0].x <= br[1].x
				&& pr[1].y >= br[0].y && pr[0].y <= br[1].y) {
				if (bz_dividable(br, br_mid) != true) {
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
						dist = br_mid.dist(a, b, c, d);
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
						f0 = bz[0].dist(a, b, c, d);
						f1 = bz[1].dist(a, b, c, d);
						f2 = bz[2].dist(a, b, c, d);
						f3 = bz[3].dist(a, b, c, d);
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
							bz_push(st, bz, 0, 4, 7, 9);
							// ��������̑g�̐���_���X�^�b�N�Ƀv�b�V������.
							bz_push(st, bz, 9, 8, 6, 3);
						}
						// ��Ԃ̍����͂ق� 0 ?
						// tMax �� tMin �̕��������ŕ\���\�Ȏ��̐���菬 ?
						else if (t_max <= std::nextafter(t_min, t_min + 1.0)) {
							// ��Ԃ̎��ʂ����l���������ċȐ���̓_ bzP �����߂�.
							BZP tp;
							bz_by_param(bz, t_min, tp);
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
							bz_push(st, bz, 8, 6, 3, 9);
						}
					}
				}
			}
		} while (st.empty() != true);
		return t_cnt;
	}
	*/

}