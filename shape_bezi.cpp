//------------------------------
// Shape_bezi.cpp
// �x�W�F�Ȑ�
//------------------------------
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// �Z�O�����g����؂鏕�ϐ��̒l
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
		inline bool operator >(const BZP& q) const noexcept { return x > q.x && y > q.y; }
		inline bool operator ==(const BZP& q) const noexcept { return x == q.x && y == q.y; }
		inline bool operator !=(const BZP& q) const noexcept { return x != q.x || y != q.y; }
		inline double opro(const BZP& q) const noexcept { return x * q.y - y * q.x; }
	};

	// �Ȑ��[�̖��̗��[�_�����߂�.
	static bool bz_calc_arrow(const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROWHEAD_SIZE a_size, D2D1_POINT_2F barbs[3]) noexcept;

	// �Ȑ��̃W�I���g���V���N�ɖ���ǉ�����.
	static void bz_create_arrow_geometry(ID2D1Factory3* const d_factory, const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROWHEAD_STYLE a_style, const ARROWHEAD_SIZE a_size, ID2D1PathGeometry** a_geo);

	// �Ȑ���̏��ϐ������Ƃɔ����l�����߂�.
	static double bz_deriv_by_param(const BZP b_pos[4], const double t) noexcept;

	// �_�̔z������Ƃɂ��������ׂĊ܂ޓʕ�����߂�.
	static void bz_get_convex(const uint32_t e_cnt, const BZP e_pos[], uint32_t& c_cnt, BZP c_pos[]);

	// �_�ƒ����̊Ԃ̍ŒZ���������߂�.
	//static double bz_shortest_dist(const BZP& p, const double a, const double b, const double c, const double d) noexcept;

	// �_���ʕ�Ɋ܂܂�邩���肷��.
	static bool bz_in_convex(const double tx, const double ty, const size_t c_cnt, const BZP c_pos[]) noexcept;

	// �Ȑ���̒��������Ƃɏ��ϐ������߂�.
	static double bz_param_by_len(const BZP b_pos[4], const double b_len) noexcept;

	// �Ȑ���̏��ϐ������ƂɈʒu�����߂�.
	static void bz_point_by_param(const BZP b_pos[4], const double t, BZP& p) noexcept;

	// �Ȑ���̏��ϐ��̋�Ԃ����Ƃɒ��������߂�.
	static double bz_len_by_param(const BZP b_pos[4], const double t_min, const double t_max, const uint32_t sim_n) noexcept;

	// 2 �̏��ϐ������ 0-1 �̊ԂŐ��������肷��.
	static bool bz_test_param(const double t_min, const double t_max) noexcept;

	// �Ȑ���̏��ϐ������Ƃɐڐ��x�N�g�������߂�.
	static void bz_tvec_by_param(const BZP b_pos[4], const double t, BZP& t_vec) noexcept;

	// ���̗��[�_���v�Z����.
	// b_start	�Ȑ��̊J�n�ʒu
	// b_seg	�Ȑ��̐���_
	// a_size	���̐��@
	// barbs[3]	�v�Z���ꂽ���[�_�Ɛ�[�_
	static bool bz_calc_arrow(const D2D1_POINT_2F b_start, const D2D1_BEZIER_SEGMENT& b_seg, const ARROWHEAD_SIZE a_size, D2D1_POINT_2F barbs[3]) noexcept
	{
		BZP seg[3]{};
		BZP b_pos[4]{};

		seg[0] = b_seg.point1;
		seg[1] = b_seg.point2;
		seg[2] = b_seg.point3;
		// ���W�l�ɂ��덷�����Ȃ��ł���, �Ǝv����̂�,
		// �x�W�F�Ȑ����n�_�����_�ƂȂ�悤�ɕ��s�ړ�.
		b_pos[3] = 0.0;
		b_pos[2] = seg[0] - b_start;
		b_pos[1] = seg[1] - b_start;
		b_pos[0] = seg[2] - b_start;
		auto b_len = bz_len_by_param(b_pos, 0.0, 1.0, SIMPSON_N);
		if (b_len > FLT_MIN) {
			// ���̐�[�̃I�t�Z�b�g, �܂��͋Ȑ��̒���, 
			// �ǂ��炩�Z������, ���ϐ������߂�.
			const auto t = bz_param_by_len(b_pos, min(b_len, a_size.m_offset));
			// ���ϐ������ƂɋȐ��̐ڐ��x�N�g���𓾂�.
			BZP t_vec;
			bz_tvec_by_param(b_pos, t, t_vec);
			// ���̕Ԃ��̈ʒu���v�Z����
			get_arrow_barbs(-t_vec, sqrt(t_vec * t_vec), a_size.m_width, a_size.m_length, barbs);
			// ���ϐ��ŋȐ���̈ʒu�𓾂�.
			BZP t_pos;	// �I�_�����_�Ƃ���, ���̐�[�̈ʒu
			bz_point_by_param(b_pos, t, t_pos);
			// �Ȑ���̈ʒu����̐�[�Ƃ�, �Ԃ��̈ʒu�����s�ړ�����.
			pt_add(barbs[0], t_pos.x, t_pos.y, barbs[0]);
			pt_add(barbs[1], t_pos.x, t_pos.y, barbs[1]);
			barbs[2] = t_pos;
			pt_add(barbs[0], b_start, barbs[0]);
			pt_add(barbs[1], b_start, barbs[1]);
			pt_add(barbs[2], b_start, barbs[2]);
			return true;
		}
		return false;
	}

	// �Ȑ��̃W�I���g���V���N�ɖ���ǉ�����
	// d_factory	D2D �t�@�N�g��
	// b_pos	�Ȑ��̊J�n�ʒu
	// b_seg	�Ȑ��̐���_
	// a_style	���̎��
	// a_size	���̐��@
	// a_geo	��肪�ǉ����ꂽ�W�I���g��
	static void bz_create_arrow_geometry(
		ID2D1Factory3* const d_factory,
		const D2D1_POINT_2F b_pos, const D2D1_BEZIER_SEGMENT& b_seg, const ARROWHEAD_STYLE a_style, const ARROWHEAD_SIZE a_size, 
		ID2D1PathGeometry** a_geo)
	{
		D2D1_POINT_2F barbs[3];	// ���̕Ԃ��̒[�_	
		winrt::com_ptr<ID2D1GeometrySink> sink;

		if (bz_calc_arrow(b_pos, b_seg, a_size, barbs)) {
			// �W�I���g���V���N�ɒǉ�����.
			winrt::check_hresult(d_factory->CreatePathGeometry(a_geo));
			winrt::check_hresult((*a_geo)->Open(sink.put()));
			sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
			sink->BeginFigure(barbs[0], a_style == ARROWHEAD_STYLE::FILLED ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
			sink->AddLine(barbs[2]);
			sink->AddLine(barbs[1]);
			sink->EndFigure(a_style == ARROWHEAD_STYLE::FILLED ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
			winrt::check_hresult(sink->Close());
			sink = nullptr;
		}
	}

	// �Ȑ���̏��ϐ������Ƃɔ����l�����߂�.
	// b_pos	����_
	// t	���ϐ�
	static double bz_deriv_by_param(const BZP b_pos[4], const double t) noexcept
	{
		BZP t_vec;

		// ���ϐ������ƂɃx�W�F�Ȑ���̐ڐ��x�N�g��������, ���̐ڐ��x�N�g���̒�����Ԃ�.
		bz_tvec_by_param(b_pos, t, t_vec);
		return sqrt(t_vec * t_vec);
	}

	// �_�̔z������Ƃɂ��������ׂĊ܂ޓʕ�����߂�.
	// �M�t�g��@����������.
	// e_cnt	�_�̐�
	// e_pos	�_�̔z��
	// c_cnt	�ʕ�̒��_�̐�
	// c_pos	�ʕ�̒��_�̔z��
	static void bz_get_convex(const uint32_t e_cnt, const BZP e_pos[], uint32_t& c_cnt, BZP c_pos[])
	{
		// e �̂���, y �l���ł��������_�̏W������, x �l���ł��������_�̓Y���� k �𓾂�.
		uint32_t k = 0;
		auto ex = e_pos[0].x;
		auto ey = e_pos[0].y;
		for (uint32_t i = 1; i < e_cnt; i++) {
			if (e_pos[i].y < ey || (e_pos[i].y == ey && e_pos[i].x < ex)) {
				ex = e_pos[i].x;
				ey = e_pos[i].y;
				k = i;
			}
		}
		// e[k] �� a �Ɋi�[����.
		auto a = e_pos[k];
		c_cnt = 0;
		do {
			// c �� a ��ǉ�����.
			c_pos[c_cnt++] = a;
			auto b = e_pos[0];
			for (uint32_t i = 1; i < e_cnt; i++) {
				const auto c = e_pos[i];
				if (b == a) {
					b = c;
				}
				else {
					const auto ab = b - a;
					const auto ac = c - a;
					const auto v = ab.opro(ac);
					if (v > 0.0 || (fabs(v) < FLT_MIN && ac * ac > ab * ab)) {
						b = c;
					}
				}
			}
			a = b;
		} while (c_cnt < e_cnt && a != c_pos[0]);
	}

	// �_�ƒ����̊Ԃ̍ŒZ���������߂�.
	/*
	static double bz_shortest_dist(const BZP& p, const double a, const double b, const double c, const double d) noexcept
	{
		return (a * p.x + b * p.y + c) / d;
	}
	*/

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

	// �_ { 0, 0 } ���ʕ�Ɋ܂܂�邩���肷��.
	// �����������p����.
	// c_cnt	�ʕ�̒��_�̐�
	// c_pos	�ʕ�̒��_�̔z��
	// �߂�l	�܂܂��Ȃ� true ��, �܂܂�Ȃ��Ȃ� false ��Ԃ�.
	static bool bz_in_convex(const double tx, const double ty, const size_t c_cnt, const BZP c_pos[]) noexcept
	{
		int k = 0;	// �_���Ƃ��鐅�������ʕ�̕ӂƌ��������.
		for (size_t i = c_cnt - 1, j = 0; j < c_cnt; i = j++) {
			// ���[�� 1. ������̕�. �_�����������ɂ���, �ӂ̎n�_�ƏI�_�̊Ԃɂ���. �������A�I�_�͊܂܂Ȃ�.
			// ���[�� 2. �������̕�. �_�����������ɂ���, �ӂ̎n�_�ƏI�_�̊Ԃɂ���. �������A�n�_�͊܂܂Ȃ�.
			// ���[�� 3. �_���Ƃ��鐅�����ƕӂ������łȂ�.
			// ���[�� 1. ���[�� 2 ���m�F���邱�Ƃ�, ���[�� 3 ���m�F�ł��Ă���.
			if ((c_pos[i].y <= ty && c_pos[j].y > ty) || (c_pos[i].y > ty && c_pos[j].y <= ty)) {
				// ���[�� 4. �ӂ͓_�����E���ɂ���. ������, �d�Ȃ�Ȃ�.
				// �ӂ��_�Ɠ��������ɂȂ�ʒu����肵, ���̎��̐��������̒l�Ɠ_�̂��̒l�Ƃ��r����.
				if (tx < c_pos[i].x + (ty - c_pos[i].y) / (c_pos[j].y - c_pos[i].y) * (c_pos[j].x - c_pos[i].x)) {
					// ���[�� 1 �܂��̓��[�� 2, �����[�� 4 �𖞂����Ȃ�, �_���Ƃ��鐅�����͓ʕ�ƌ�������.
					k++;
				}
			}
		}
		// ��������񐔂�������肷��.
		// ��Ȃ��, �_�͓ʕ�Ɋ܂܂�̂� true ��Ԃ�.
		return static_cast<bool>(k & 1);
	}

	// �Ȑ���̒��������Ƃɏ��ϐ������߂�.
	// b_pos	����_
	// b_len	����
	// �߂�l	����ꂽ���ϐ��̒l
	static double bz_param_by_len(const BZP b_pos[4], const double b_len) noexcept
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
			e = bz_len_by_param(b_pos, 0.0, t, SIMPSON_N) - b_len;
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

	// �Ȑ���̏��ϐ������ƂɈʒu�����߂�.
	// b_pos	����_
	// t	���ϐ�
	// p	���܂����ʒu
	static void bz_point_by_param(const BZP b_pos[4], const double t, BZP& p) noexcept
	{
		const double s = 1.0 - t;
		const double ss = s * s;
		const double tt = t * t;
		p = b_pos[0] * s * ss + b_pos[1] * 3.0 * ss * t + b_pos[2] * 3.0 * s * tt + b_pos[3] * t * tt;
	}

	// ���`�������\�����肵, ���̏d�S�_��Ԃ�.
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

	// �Ȑ���̏��ϐ��̋�Ԃ����Ƃɒ��������߂�.
	// �V���v�\���@��p����.
	// b_pos	����_
	// t_min	��Ԃ̎n�[
	// t_max	��Ԃ̏I�[
	static double bz_len_by_param(const BZP b_pos[4], const double t_min, const double t_max, const uint32_t sim_n) noexcept
	{
		double t_diff;
		uint32_t n;
		double h;
		double a, b;
		double t;
		double b0, b2;
		double s;

		/* �͈͂̏�������͐��������肷��. */
		/* ���� ? */
		if (bz_test_param(t_min, t_max)) {
			/* �͈͏�� t_max -�͈͉��� t_min ������ t_diff �Ɋi�[����. */
			t_diff = t_max - t_min;
			/* ��Ԃ̕����� sim_n �� t_diff ����Z����. */
			/* ���̌��ʂ�؂�グ�Đ����l����. */
			/* �����l����Ԃ̔��� n �Ɋi�[����. */
			n = (int)std::ceil(t_diff * (double)sim_n);
			/* t_diff��2n ���K�� h �Ɋi�[����. */
			h = t_diff / (2.0 * n);
			/* 0 ����Ԗڂ̕�����Ԃ̍��v�l a �Ɋi�[����. */
			a = 0.0;
			/* 0 �������Ԗڂ̕�����Ԃ̍��v�l b �Ɋi�[����. */
			b = 0.0;
			/* t_min+h �����ϐ� t �Ɋi�[����. */
			t = t_min + h;
			/* 1 ��Y���� i �Ɋi�[����. */
			/* i �� n ��菬 ? */
			for (uint32_t i = 1; i < n; i++) {
				/* 2i-1 �Ԗڂ̕�����Ԃ̔����l������, a �ɉ�����. */
				a += bz_deriv_by_param(b_pos, t);
				/* �K�� h �����ϐ� t �ɉ�����. */
				t += h;
				/* 2i �Ԗڂ̕�����Ԃ̔����l������, b �ɉ�����. */
				b += bz_deriv_by_param(b_pos, t);
				/* �K�� h �����ϐ� t �ɉ�����. */
				t += h;
				/* i ���C���N�������g����. */
			}
			/* 2n-1 �Ԗڂ̕�����Ԃ̔����l������, a �ɉ�����. */
			a += bz_deriv_by_param(b_pos, t);
			/* 0 �Ԗڂ̕�����Ԃł̔����l������, b0 �Ɋi�[����. */
			b0 = bz_deriv_by_param(b_pos, t_min);
			/* 2n �Ԗڂ̕�����Ԃł̔����l������, b2 �Ɋi�[����. */
			b2 = bz_deriv_by_param(b_pos, t_max);
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

	// �x�W�F�Ȑ��ƕ��`������邩, �Ȑ��𕪊���, ���肷��.
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

	// 2 �̏��ϐ������ 0-1 �̊ԂŐ��������肷��.
	// t_min	���������̏��ϐ�
	// t_max	�傫�����̏��ϐ�
	static bool bz_test_param(const double t_min, const double t_max) noexcept
	{
		// �͈͂̏�� t_max �� 1+DBL_EPSILON ��菬 ?
		// t_min ���傫���čł��߂��l�� t_max ��菬 ?
		return -DBL_MIN < t_min && t_max < 1.0 + DBL_EPSILON && std::nextafter(t_min, t_min + 1.0) < t_max;
	}

	// �Ȑ���̏��ϐ������Ƃɐڐ��x�N�g�������߂�.
	// b_pos	�Ȑ�
	// t	���ϐ�
	// t_vec	�ڐ��x�N�g��
	static void bz_tvec_by_param(const BZP b_pos[4], const double t, BZP& t_vec) noexcept
	{
		const double a = -3.0 * (1.0 - t) * (1.0 - t);
		const double b = 3.0 * (1.0 - t) * (1.0 - 3.0 * t);
		const double c = 3.0 * t * (2.0 - 3.0 * t);
		const double d = (3.0 * t * t);
		t_vec = b_pos[0] * a + b_pos[1] * b + b_pos[2] * c + b_pos[3] * d;
	}

	// �p�X�W�I���g�����쐬����.
	void ShapeBezi::create_path_geometry(ID2D1Factory3* const d_factory)
	{
		D2D1_BEZIER_SEGMENT b_seg;
		winrt::com_ptr<ID2D1GeometrySink> sink;

		m_d2d_path_geom = nullptr;
		m_d2d_arrow_geom = nullptr;
		pt_add(m_pos, m_diff[0], b_seg.point1);
		pt_add(b_seg.point1, m_diff[1], b_seg.point2);
		pt_add(b_seg.point2, m_diff[2], b_seg.point3);
		winrt::check_hresult(d_factory->CreatePathGeometry(m_d2d_path_geom.put()));
		m_d2d_path_geom->Open(sink.put());
		sink->SetFillMode(D2D1_FILL_MODE::D2D1_FILL_MODE_ALTERNATE);
		sink->BeginFigure(m_pos, D2D1_FIGURE_BEGIN::D2D1_FIGURE_BEGIN_HOLLOW);
		sink->AddBezier(b_seg);
		sink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);
		winrt::check_hresult(sink->Close());
		sink = nullptr;
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			bz_create_arrow_geometry(d_factory, m_pos, b_seg, m_arrow_style, m_arrow_size, m_d2d_arrow_geom.put());
		}
	}

	// �}�`��\������.
	void ShapeBezi::draw(SHAPE_DX& dx)
	{
		D2D1_POINT_2F s_pos;
		D2D1_POINT_2F e_pos;

		if (is_opaque(m_stroke_color)) {
			const auto s_width = static_cast<FLOAT>(m_stroke_width);
			const auto s_brush = dx.m_shape_brush.get();
			const auto s_style = m_d2d_stroke_style.get();
			dx.m_shape_brush->SetColor(m_stroke_color);
			dx.m_d2dContext->DrawGeometry(m_d2d_path_geom.get(), s_brush, s_width, s_style);
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				const auto a_geom = m_d2d_arrow_geom.get();
				if (m_arrow_style == ARROWHEAD_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(a_geom, s_brush, nullptr);
				}
				dx.m_d2dContext->DrawGeometry(a_geom, s_brush, s_width, s_style);
			}
			/*
			if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
				auto geo = m_d2d_arrow_geom.get();
				dx.m_d2dContext->DrawGeometry(geo, sb, sw, nullptr);
				if (m_arrow_style == ARROWHEAD_STYLE::FILLED) {
					dx.m_d2dContext->FillGeometry(geo, sb, nullptr);
				}
			}
			*/
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
	//bool ShapeBezi::get_arrow_size(ARROWHEAD_SIZE& value) const noexcept
	//{
	//	value = m_arrow_size;
	//	return true;
	//}

	// ���̌`���𓾂�.
	//bool ShapeBezi::get_arrow_style(ARROWHEAD_STYLE& value) const noexcept
	//{
	//	value = m_arrow_style;
	//	return true;
	//}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// a_len	���ʂ̑傫��
	// �߂�l	�ʒu���܂ސ}�`�̕���. �܂܂Ȃ��Ƃ��́u�}�`�̊O���v��Ԃ�.
	uint32_t ShapeBezi::hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept
	{
		const auto e_width = max(max(m_stroke_width, a_len) * 0.5, 0.5);	// ���g�̑����̔����̒l
		D2D1_POINT_2F tp;
		pt_sub(t_pos, m_pos, tp);
		// ���肷��ʒu�ɂ���Đ��x�������Ȃ��悤, �J�n�ʒu�����_�ƂȂ�悤���s�ړ���, ����_�𓾂�.
		D2D1_POINT_2F c_pos[4];
		c_pos[0].x = c_pos[0].y = 0.0;
		//pt_sub(m_pos, t_pos, c_pos[0]);
		pt_add(c_pos[0], m_diff[0], c_pos[1]);
		pt_add(c_pos[1], m_diff[1], c_pos[2]);
		pt_add(c_pos[2], m_diff[2], c_pos[3]);
		if (pt_in_anch(tp, c_pos[3], a_len)) {
			return ANCH_TYPE::ANCH_P0 + 3;
		}
		if (pt_in_anch(tp, c_pos[2], a_len)) {
			return ANCH_TYPE::ANCH_P0 + 2;
		}
		if (pt_in_anch(tp, c_pos[1], a_len)) {
			return ANCH_TYPE::ANCH_P0 + 1;
		}
		if (pt_in_anch(tp, c_pos[0], a_len)) {
			return ANCH_TYPE::ANCH_P0 + 0;
		}
		// �ŏ��̐���_�̑g���v�b�V������.
		// �S�̓_�̂����[�_��, ���ɂ܂��g�Ƌ��L����̂�, 1 + 3 * D_MAX �̔z����m�ۂ���.
		constexpr auto D_MAX = 64;	// ��������[���̍ő�l
		BZP s_arr[1 + D_MAX * 3] {};	// ����_�̃X�^�b�N
		int32_t s_cnt = 0;	// �X�^�b�N�ɐς܂ꂽ�_�̐�
		s_arr[0] = c_pos[0];
		s_arr[1] = c_pos[1];
		s_arr[2] = c_pos[2];
		s_arr[3] = c_pos[3];
		s_cnt += 4;
		// �X�^�b�N�� ��g�ȏ�̐���_���c���Ă��邩���肷��.
		while (s_cnt >= 4) {
			// ����_�̑g���|�b�v����.
			BZP b_pos[10];
			b_pos[3] = s_arr[s_cnt - 1];
			b_pos[2] = s_arr[s_cnt - 2];
			b_pos[1] = s_arr[s_cnt - 3];
			b_pos[0] = s_arr[s_cnt - 4];
			// �[�_�͋��L�Ȃ̂Ńs�[�N����;
			s_cnt -= 4 - 1;
			// ����_�̑g����ʕ� c0 �𓾂� (���ۂ͕��`�ő�p����).
			// ����_�̑g����, �d��������̂��������_�̏W���𓾂�.
			BZP c0_min = b_pos[0];	// �ʕ� c0 (���܂ޕ��`�̍���_)
			BZP c0_max = b_pos[0];	// �ʕ� c0 (���܂ޕ��`�̉E���_)
			BZP d_pos[4];	// �d�����Ȃ��_�̏W��.
			uint32_t d_cnt = 0;	// �d�����Ȃ��_�̏W���̗v�f��
			d_pos[d_cnt++] = b_pos[0];
			for (uint32_t i = 1; i < 4; i++) {
				if (d_pos[d_cnt - 1] != b_pos[i]) {
					d_pos[d_cnt++] = b_pos[i];
					b_pos[i].inc(c0_min, c0_max);
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
			//        |          d_nor|         
			//   d[i] +---------------+---> d_vec
			//        |           d[i+1]
			//        + - - - - - - - - - +
			//   e[i][3]             e[i][2]
			BZP e_pos[3 * 4];	// �g���E�������ꂽ�����̔z��
			for (uint32_t i = 0, j = 0; i < d_cnt - 1; i++, j += 4) {
				auto d_vec = d_pos[i + 1] - d_pos[i];	// �����̃x�N�g��
				// �����̃x�N�g���̒�����, �����̔����ɂ���.
				d_vec = d_vec * (e_width / std::sqrt(d_vec * d_vec));
				const BZP d_nor{ d_vec.y, -d_vec.x };	// �����̖@���x�N�g��

				// �@���x�N�g���ɂ����Đ��t�̕����ɐ������g������.
				e_pos[j + 0] = d_pos[i] + d_nor;
				e_pos[j + 1] = d_pos[i + 1] + d_nor;
				e_pos[j + 2] = d_pos[i + 1] - d_nor;
				e_pos[j + 3] = d_pos[i] - d_nor;
				if (i > 0) {
					// �ŏ��̐���_�ȊO��, �����x�N�g���̕����ɉ�������.
					e_pos[j + 0] = e_pos[j + 0] - d_vec;
					e_pos[j + 3] = e_pos[j + 3] - d_vec;
				}
				if (i + 1 < d_cnt - 1) {
					// �Ō�̐���_�ȊO��, �����x�N�g���̋t�����ɉ�������.
					e_pos[j + 1] = e_pos[j + 1] + d_vec;
					e_pos[j + 2] = e_pos[j + 2] + d_vec;
				}
			}
			// �g���E�������ꂽ��������, �ʕ� c1 �𓾂�.
			uint32_t c1_cnt;
			BZP c1_pos[3 * 4];
			bz_get_convex((d_cnt - 1) * 4, e_pos, c1_cnt, c1_pos);
			// �_���ʕ� c1 �Ɋ܂܂�Ȃ������肷��.
			if (!bz_in_convex(tp.x, tp.y, c1_cnt, c1_pos)) {
				// ����ȏケ�̐���_�̑g�𕪊�����K�v�͂Ȃ�.
				// �X�^�b�N�Ɏc�������̐���_�̑g������.
				continue;
			}

			// �ʕ� c0 �̑傫���� 1 �ȉ������肷��.
			BZP c0 = c0_max - c0_min;
			if (c0.x <= 1.0 && c0.y <= 1.0) {
				// ���݂̐���_�̑g (�ʕ� c0) ������ȏ㕪������K�v�͂Ȃ�.
				// �ʕ� c1 �͔��肷��ʒu���܂�ł���̂�, �}�`�̕��ʂ�Ԃ�.
				return ANCH_TYPE::ANCH_STROKE;
			}

			// �X�^�b�N���I�o�[�t���[���邩���肷��.
			if (s_cnt + 6 > 1 + D_MAX * 3) {
				// ���݂̐���_�̑g (�ʕ� c0) ������ȏ㕪�����邱�Ƃ͂ł��Ȃ�.
				// �ʕ� c1�͔��肷��ʒu���܂�ł���̂�, �}�`�̕��ʂ�Ԃ�.
				return ANCH_TYPE::ANCH_STROKE;
			}

			// ����_�̑g�� 2 ��������.
			// b[0,1,2,3] �̒��_�� b[4,5,6] ��, b[4,5,6] �̒��_�� b[7,8] ��, b[7,8] �̒��_�� b[9] �Ɋi�[����.
			// ������������_�̑g�͂��ꂼ�� b[0,4,7,9] �� b[9,8,6,3] �ɂȂ�.
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
		return ANCH_TYPE::ANCH_SHEET;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// ���̑����͍l������Ȃ�.
	// a_min	�͈͂̍���ʒu
	// a_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	bool ShapeBezi::in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept
	{
		// �v�Z���x���Ȃ�ׂ��ς��Ȃ��悤,
		// �͈͂̍��オ���_�ƂȂ�悤���s�ړ���������_�𓾂�.
		const auto w = static_cast<double>(a_max.x) - a_min.x;
		const auto h = static_cast<double>(a_max.y) - a_min.y;
		D2D1_POINT_2F c_pos[4];
		pt_sub(m_pos, a_min, c_pos[0]);
		pt_add(c_pos[0], m_diff[0], c_pos[1]);
		pt_add(c_pos[1], m_diff[1], c_pos[2]);
		pt_add(c_pos[2], m_diff[2], c_pos[3]);
		// �ŏ��̐���_�̑g���v�b�V������.
		constexpr auto D_MAX = 52;	// ��������ő��
		BZP s_arr[1 + D_MAX * 3] = {};
		int32_t s_cnt = 4;
		s_arr[0] = c_pos[0];
		s_arr[1] = c_pos[1];
		s_arr[2] = c_pos[2];
		s_arr[3] = c_pos[3];
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
			// ����ɕ�������K�v�͂Ȃ��̂�, �X�^�b�N�̎c��̑g�ɂ��Ĕ��肷��.
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
	//void ShapeBezi::set_arrow_size(const ARROWHEAD_SIZE& value)
	//{
	//	if (equal(m_arrow_size, value)) {
	//		return;
	//	}
	//	m_arrow_size = value;
	//	create_path_geometry(s_d2d_factory);
	//}

	// ���̌`���Ɋi�[����.
	//void ShapeBezi::set_arrow_style(const ARROWHEAD_STYLE value)
	//{
	//	if (m_arrow_style == value) {
	//		return;
	//	}
	//	m_arrow_style = value;
	//	create_path_geometry(s_d2d_factory);
	//}

	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	//void ShapeBezi::set_start_pos(const D2D1_POINT_2F value)
	//{
	//	ShapeStroke::set_start_pos(value);
	//	create_path_geometry(s_d2d_factory);
	//}

	// �}�`���쐬����.
	// b_pos	�͂ޗ̈�̎n�_
	// b_diff	�͂ޗ̈�̏I�_�ւ̍���
	// s_attr	����
	ShapeBezi::ShapeBezi(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr) :
		ShapePath::ShapePath(3, s_attr)
	{
		m_pos = b_pos;
		m_diff[0].x = b_diff.x;
		m_diff[0].y = 0.0f;
		m_diff[1].x = -b_diff.x;
		m_diff[1].y = b_diff.y;
		m_diff[2].x = b_diff.x;
		m_diff[2].y = 0.0f;
		create_path_geometry(s_d2d_factory);
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeBezi::ShapeBezi(DataReader const& dt_reader) :
		ShapePath::ShapePath(dt_reader)
	{
	//	m_arrow_style = static_cast<ARROWHEAD_STYLE>(dt_reader.ReadUInt32());
	//	m_arrow_size.m_width = dt_reader.ReadSingle();
	//	m_arrow_size.m_length = dt_reader.ReadSingle();
	//	m_arrow_size.m_offset = dt_reader.ReadSingle();
		create_path_geometry(s_d2d_factory);
	}

	// �f�[�^���C�^�[�ɏ�������.
	//void ShapeBezi::write(DataWriter const& dt_writer) const
	//{
	//	ShapePath::write(dt_writer);
	//	dt_writer.WriteUInt32(static_cast<uint32_t>(m_arrow_style));
	//	dt_writer.WriteSingle(m_arrow_size.m_width);
	//	dt_writer.WriteSingle(m_arrow_size.m_length);
	//	dt_writer.WriteSingle(m_arrow_size.m_offset);
	//}

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
		write_svg("/>" SVG_NEW_LINE, dt_writer);
		if (m_arrow_style != ARROWHEAD_STYLE::NONE) {
			D2D1_POINT_2F barbs[3];
			bz_calc_arrow(m_pos, b_seg, m_arrow_size, barbs);
			ShapeStroke::write_svg(barbs, barbs[2], m_arrow_style, dt_writer);
		}
	}

	/*
	static uint32_t clipping(const BZP& p, const BZP& q, double* t)
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
						dist = bz_shortest_dist(br_mid, a, b, c, d);
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
						f0 = bz_shortest_dist(bz[0], a, b, c, d);
						f1 = bz_shortest_dist(bz[1], a, b, c, d);
						f2 = bz_shortest_dist(bz[2], a, b, c, d);
						f3 = bz_shortest_dist(bz[3], a, b, c, d);
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
							bz_point_by_param(bz, t_min, tp);
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