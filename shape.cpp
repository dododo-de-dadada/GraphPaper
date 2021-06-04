//------------------------------
// shape.cpp
// �}�`�̂ЂȌ^, ���̑�
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
#if defined(_DEBUG)
	uint32_t debug_leak_cnt = 0;
	uint32_t debug_shape_cnt = 0;
	uint32_t debug_deleted_cnt = 0;
#endif

	ID2D1Factory3* Shape::s_d2d_factory = nullptr;	// D2D1 �t�@�N�g��
	IDWriteFactory3* Shape::s_dwrite_factory = nullptr;	// DWRITE �t�@�N�g��
	float Shape::s_anch_len = 6.0f;
	D2D1_COLOR_F Shape::m_range_background = RNG_BACK;	// �����͈͂̔w�i�F
	D2D1_COLOR_F Shape::m_range_foreground = RNG_TEXT;	// �����͈͂̕����F
	D2D1_COLOR_F Shape::m_theme_background = S_WHITE;	// �O�i�F (�A���J�[�̔w�i�F)
	D2D1_COLOR_F Shape::m_theme_foreground = S_BLACK;	// �w�i�F (�A���J�[�̑O�i�F)

	// ������ '0'...'9' �܂��� 'A'...'F', 'a'...'f' �����肷��.
	static bool is_hex(const wchar_t w, uint32_t& x) noexcept;

	// �}�`�̕��� (���`) ��\������.
	// a_pos	���ʂ̈ʒu
	// dx		�}�`�̕`���
	void anchor_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		D2D1_POINT_2F r_min;
		pt_add(a_pos, -0.5 * Shape::s_anch_len, r_min);
		D2D1_POINT_2F r_max;
		pt_add(r_min, Shape::s_anch_len, r_max);
		const D2D1_RECT_F r{ r_min.x, r_min.y, r_max.x, r_max.y };

		dx.m_shape_brush->SetColor(Shape::m_theme_background);
		dx.m_d2dContext->DrawRectangle(r, dx.m_shape_brush.get(), 2.0, nullptr);
		dx.m_shape_brush->SetColor(Shape::m_theme_foreground);
		dx.m_d2dContext->FillRectangle(r, dx.m_shape_brush.get());
	}

	// �}�`�̕��ʁi�~�`�j��\������.
	// a_pos	���ʂ̈ʒu
	// dx		�}�`�̕`���
	void anchor_draw_ellipse(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		const FLOAT rad = static_cast<FLOAT>(Shape::s_anch_len * 0.5 + 1.0);
		dx.m_shape_brush->SetColor(Shape::m_theme_background);
		dx.m_d2dContext->FillEllipse({ a_pos, rad, rad }, dx.m_shape_brush.get());
		dx.m_shape_brush->SetColor(Shape::m_theme_foreground);
		dx.m_d2dContext->FillEllipse({ a_pos, rad - 1.0F, rad - 1.0F }, dx.m_shape_brush.get());
	}

	// ��邵�̕Ԃ��̈ʒu�����߂�.
	// a_vec	��x�N�g��.
	// a_len	��x�N�g���̒���
	// h_width	��邵�̕� (�Ԃ��̊Ԃ̒���)
	// h_len	��邵�̒��� (��[����Ԃ��܂ł̎��x�N�g����ł̒���)
	// barbs[2]	�v�Z���ꂽ��邵�̕Ԃ��̈ʒu (��[����̃I�t�Z�b�g)
	void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F barbs[]) noexcept
	{
		if (a_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z{ 0.0f, 0.0f };
			barbs[0] = Z;
			barbs[1] = Z;
		}
		else {
			const double hf = h_width * 0.5;	// ��邵�̕��̔����̑傫��
			const double sx = a_vec.x * -h_len;	// ��x�N�g�����邵�̒��������]
			const double sy = a_vec.x * hf;
			const double tx = a_vec.y * -h_len;
			const double ty = a_vec.y * hf;
			const double ax = 1.0 / a_len;
			barbs[0].x = static_cast<FLOAT>((sx - ty) * ax);
			barbs[0].y = static_cast<FLOAT>((tx + sy) * ax);
			barbs[1].x = static_cast<FLOAT>((sx + ty) * ax);
			barbs[1].y = static_cast<FLOAT>((tx - sy) * ax);
		}
	}

	// ��_�ň͂܂ꂽ���`�𓾂�.
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept
	{
		if (a.x < b.x) {
			r_min.x = a.x;
			r_max.x = b.x;
		}
		else {
			r_min.x = b.x;
			r_max.x = a.x;
		}
		if (a.y < b.y) {
			r_min.y = a.y;
			r_max.y = b.y;
		}
		else {
			r_min.y = b.y;
			r_max.y = a.y;
		}
	}

	// ���~�ɂ��ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// c_pos	���~�̒��S
	// rad	���~�̌a
	// �߂�l	�܂ޏꍇ true
	bool pt_in_elli(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept
	{
		const double dx = static_cast<double>(t_pos.x) - static_cast<double>(c_pos.x);
		const double dy = static_cast<double>(t_pos.y) - static_cast<double>(c_pos.y);
		const double rxrx = rad_x * rad_x;
		const double ryry = rad_y * rad_y;
		return dx * dx * ryry + dy * dy * rxrx <= rxrx * ryry;
	}

	// �������ʒu���܂ނ�, �������l�����Ĕ��肷��.
	// t_pos	���肷��ʒu
	// s_pos	�����̎n�[
	// e_pos	�����̏I�[
	// s_width	�����̑���
	// s_cap	�����̒[�_
	// �߂�l	�܂ޏꍇ true
	bool pt_in_line(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept
	{
		const double e_width = max(s_width * 0.5, 0.5);
		if (equal(s_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_SQUARE })) {
			D2D1_POINT_2F diff;	// ���������̃x�N�g��
			pt_sub(e_pos, s_pos, diff);
			const double abs2 = pt_abs2(diff);
			pt_mul(
				abs2 > FLT_MIN ? diff : D2D1_POINT_2F { 0.0f, static_cast<FLOAT>(e_width) },
				abs2 > FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				diff);
			const double dx = diff.x;
			const double dy = diff.y;
			const double ox = dy;
			const double oy = -dx;
			D2D1_POINT_2F e_side[4];
			pt_add(s_pos, -dx + ox, -dy + oy, e_side[0]);
			pt_add(s_pos, -dx - ox, -dy - oy, e_side[1]);
			pt_add(e_pos, dx - ox, dy - oy, e_side[2]);
			pt_add(e_pos, dx + ox, dy + oy, e_side[3]);
			return pt_in_poly(t_pos, 4, e_side);
		}
		else if (equal(s_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE, D2D1_CAP_STYLE::D2D1_CAP_STYLE_TRIANGLE })) {
			D2D1_POINT_2F diff;	// ���������̃x�N�g��
			pt_sub(e_pos, s_pos, diff);
			const double abs2 = pt_abs2(diff);
			pt_mul(
				abs2 > FLT_MIN ? diff : D2D1_POINT_2F{ 0.0f, static_cast<FLOAT>(e_width) },
				abs2 > FLT_MIN ? e_width / sqrt(abs2) : 1.0f,
				diff);
			const double dx = diff.x;
			const double dy = diff.y;
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
			if (equal(s_cap, CAP_STYLE{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE::D2D1_CAP_STYLE_ROUND })) {
				if (pt_in_circle(t_pos, s_pos, e_width) || pt_in_circle(t_pos, e_pos, e_width)) {
					return true;
				}
			}
			D2D1_POINT_2F diff;	// �����x�N�g��
			pt_sub(e_pos, s_pos, diff);
			const double abs2 = pt_abs2(diff);
			if (abs2 > FLT_MIN) {
				pt_mul(diff, e_width / sqrt(abs2), diff);
				const double ox = diff.y;
				const double oy = -diff.x;
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

	// ���p�`���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// v_cnt	���_�̐�
	// v_pos	���_�̔z�� [v_cnt]
	// �߂�l	�܂ޏꍇ true
	// ���p�`�̊e�ӂ�, �w�肳�ꂽ�_���J�n�_�Ƃ��鐅�������������鐔�����߂�.
	bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[]) noexcept
	{
		const double tx = t_pos.x;
		const double ty = t_pos.y;
		int i_cnt;	// ��_�̐�
		int i;

		double px = v_pos[v_cnt - 1].x;
		double py = v_pos[v_cnt - 1].y;
		i_cnt = 0;
		for (i = 0; i < v_cnt; i++) {
			double qx = v_pos[i].x;
			double qy = v_pos[i].y;
			// ���[�� 1. ������̕�. �_�� y �������ɂ��āA�n�_�ƏI�_�̊Ԃɂ��� (�������A�I�_�͊܂܂Ȃ�).
			// ���[�� 2. �������̕�. �_�� y �������ɂ��āA�n�_�ƏI�_�̊Ԃɂ��� (�������A�n�_�͊܂܂Ȃ�).
			if ((py <= ty && qy > ty) || (py > ty && qy <= ty)) {
				// ���[�� 3. �_��ʂ鐅�������ӂƏd�Ȃ� (���[�� 1, ���[�� 2 ���m�F���邱�Ƃ�, ���[�� 3 ���m�F�ł��Ă���).
				// ���[�� 4. �ӂ͓_�����E���ɂ���. ������, �d�Ȃ�Ȃ�.
				// �ӂ��_�Ɠ��������ɂȂ�ʒu����肵, ���̎���x�̒l�Ɠ_��x�̒l���r����.
				if (tx < px + (ty - py) / (qy - py) * (qx - px)) {
					i_cnt++;
				}
			}
			px = qx;
			py = qy;
		}
		return static_cast<bool>(i_cnt & 1);
	}
	/*
	bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t v_cnt, const D2D1_POINT_2F v_pos[]) noexcept
	{
		D2D1_POINT_2F p_pos;
		int i_cnt;	// ��_�̐�
		int i;

		i_cnt = 0;
		for (p_pos = v_pos[v_cnt - 1], i = 0; i < v_cnt; p_pos = v_pos[i++]) {
			// ���[�� 1. ������̕�. �_�� y �������ɂ��āA�n�_�ƏI�_�̊Ԃɂ��� (�������A�I�_�͊܂܂Ȃ�).
			// ���[�� 2. �������̕�. �_�� y �������ɂ��āA�n�_�ƏI�_�̊Ԃɂ��� (�������A�n�_�͊܂܂Ȃ�).
			if ((p_pos.y <= t_pos.y && v_pos[i].y > t_pos.y)
				|| (p_pos.y > t_pos.y && v_pos[i].y <= t_pos.y)) {
				// ���[�� 3. �_��ʂ鐅�������ӂƏd�Ȃ� (���[�� 1, ���[�� 2 ���m�F���邱�Ƃ�, ���[�� 3 ���m�F�ł��Ă���).
				// ���[�� 4. �ӂ͓_�����E���ɂ���. ������, �d�Ȃ�Ȃ�.
				// �ӂ��_�Ɠ��������ɂȂ�ʒu����肵, ���̎���x�̒l�Ɠ_��x�̒l���r����.
				if (t_pos.x < p_pos.x + (t_pos.y - p_pos.y) / (v_pos[i].y - p_pos.y) * (v_pos[i].x - p_pos.x)) {
					i_cnt++;
				}
			}
		}
		return static_cast<bool>(i_cnt & 1);
	}
	*/

	// ���`���ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// r_min	���`�̂����ꂩ�̒��_
	// r_max	���`�̂�������̒��_
	// �߂�l	�܂ޏꍇ true
	bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept
	{
		double min_x;
		double max_x;
		double min_y;
		double max_y;

		if (r_min.x < r_max.x) {
			min_x = r_min.x;
			max_x = r_max.x;
		}
		else {
			min_x = r_max.x;
			max_x = r_min.x;
		}
		if (r_min.y < r_max.y) {
			min_y = r_min.y;
			max_y = r_max.y;
		}
		else {
			min_y = r_max.y;
			max_y = r_min.y;
		}
		return min_x <= t_pos.x && t_pos.x <= max_x && min_y <= t_pos.y && t_pos.y <= max_y;
	}

	// �w�肵���ʒu���܂ނ悤, ���`���g�傷��.
	// a	�܂܂��ʒu
	// r_min	���̕��`�̍���ʒu, ����ꂽ����ʒu
	// r_max	���̕��`�̉E���ʒu, ����ꂽ�E���ʒu
	void pt_inc(const D2D1_POINT_2F a, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept
	{
		if (a.x < r_min.x) {
			r_min.x = a.x;
		}
		if (a.x > r_max.x) {
			r_max.x = a.x;
		}
		if (a.y < r_min.y) {
			r_min.y = a.y;
		}
		if (a.y > r_max.y) {
			r_max.y = a.y;
		}
	}

	// ��邵�̐��@���f�[�^���[�_�[����ǂݍ���.
	void dt_read(ARROW_SIZE& value, DataReader const& dt_reader)
	{
		value.m_width = dt_reader.ReadSingle();
		value.m_length = dt_reader.ReadSingle();
		value.m_offset = dt_reader.ReadSingle();
	}

	// ��邵�̐��@���f�[�^���[�_�[����ǂݍ���.
	void dt_read(CAP_STYLE& value, DataReader const& dt_reader)
	{
		value.m_start = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		value.m_end = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
	}

	// �F���f�[�^���[�_�[����ǂݍ���.
	void dt_read(D2D1_COLOR_F& value, DataReader const& dt_reader)
	{
		value.a = dt_reader.ReadSingle();
		value.r = dt_reader.ReadSingle();
		value.g = dt_reader.ReadSingle();
		value.b = dt_reader.ReadSingle();
		value.a = min(max(value.a, 0.0F), 1.0F);
		value.r = min(max(value.r, 0.0F), 1.0F);
		value.g = min(max(value.g, 0.0F), 1.0F);
		value.b = min(max(value.b, 0.0F), 1.0F);
	}

	// �ʒu���f�[�^���[�_�[����ǂݍ���.
	void dt_read(D2D1_POINT_2F& value, DataReader const& dt_reader)
	{
		value.x = dt_reader.ReadSingle();
		value.y = dt_reader.ReadSingle();
	}

	// ���@���f�[�^���[�_�[����ǂݍ���.
	void dt_read(D2D1_SIZE_F& value, DataReader const& dt_reader)
	{
		value.width = dt_reader.ReadSingle();
		value.height = dt_reader.ReadSingle();
	}

	// ������͈͂��f�[�^���[�_�[����ǂݍ���.
	void dt_read(DWRITE_TEXT_RANGE& value, DataReader const& dt_reader)
	{
		value.startPosition = dt_reader.ReadUInt32();
		value.length = dt_reader.ReadUInt32();
	}

	// �j���̔z�u���f�[�^���[�_�[����ǂݍ���.
	void dt_read(STROKE_DASH_PATT& value, DataReader const& dt_reader)
	{
		value.m_[0] = dt_reader.ReadSingle();
		value.m_[1] = dt_reader.ReadSingle();
		value.m_[2] = dt_reader.ReadSingle();
		value.m_[3] = dt_reader.ReadSingle();
		value.m_[4] = dt_reader.ReadSingle();
		value.m_[5] = dt_reader.ReadSingle();
	}

	// ���p�`�̃c�[�����f�[�^���[�_�[����ǂݍ���.
	void dt_read(POLY_TOOL& value, DataReader const& dt_reader)
	{
		value.m_vertex_cnt = dt_reader.ReadUInt32();
		value.m_regular = dt_reader.ReadBoolean();
		value.m_vertex_up = dt_reader.ReadBoolean();
		value.m_closed = dt_reader.ReadBoolean();
		value.m_clockwise = dt_reader.ReadBoolean();
	}

	// ��������f�[�^���[�_�[����ǂݍ���.
	void dt_read(wchar_t*& value, DataReader const& dt_reader)
	{
		const size_t n = dt_reader.ReadUInt32();	// ������
		if (n > 0) {
			value = new wchar_t[n + 1];
			if (value != nullptr) {
				for (size_t i = 0; i < n; i++) {
					value[i] = dt_reader.ReadUInt16();
				}
				value[n] = L'\0';
			}
		}
		else {
			value = nullptr;
		}
	}

	// �ʒu�z����f�[�^���[�_�[����ǂݍ���.
	void dt_read(std::vector<D2D1_POINT_2F>& value, DataReader const& dt_reader)
	{
		const size_t v_cnt = dt_reader.ReadUInt32();	// �v�f��
		value.resize(v_cnt);
		for (size_t i = 0; i < v_cnt; i++) {
			dt_read(value[i], dt_reader);
		}
	}

	// ����̋������f�[�^���[�_�[����ǂݍ���.
	void dt_read(GRID_EMPH& value, DataReader const& dt_reader)
	{
		value.m_gauge_1 = dt_reader.ReadUInt16();
		value.m_gauge_2 = dt_reader.ReadUInt16();
		if (equal(value, GRID_EMPH_0) || equal(value, GRID_EMPH_2) || equal(value, GRID_EMPH_3)) {
			return;
		}
		value = GRID_EMPH_0;
	}

	//	������𕡐�����.
	//	���̕����񂪃k���|�C���^�[, �܂��͌��̕������� 0 �̂Ƃ���,
	//	�k���|�C���^�[��Ԃ�.
	wchar_t* wchar_cpy(const wchar_t* const s)
	{
		const auto s_len = wchar_len(s);
		if (s_len == 0) {
			return nullptr;
		}
		auto t = new wchar_t[static_cast<size_t>(s_len) + 1];
		wcscpy_s(t, static_cast<size_t>(s_len) + 1, s);
		return t;
	}

	// ������ 0...9 �܂��� A...F, a...f �����肷��
	static bool is_hex(const wchar_t w, uint32_t& x) noexcept
	{
		if (isdigit(w)) {
			x = w - '0';
		}
		else if (w >= 'a' && w <= 'f') {
			x = w - 'a' + 10;
		}
		else if (w >= 'A' && w <= 'F') {
			x = w - 'A' + 10;
		}
		else {
			return false;
		}
		return true;
	}

	// ������𕡐�����.
	// �G�X�P�[�v������͕����R�[�h�ɕϊ�����.
	wchar_t* wchar_cpy_esc(const wchar_t* const s)
	{
		const auto s_len = wchar_len(s);
		if (s_len == 0) {
			return nullptr;
		}
		auto t = new wchar_t[static_cast<size_t>(s_len) + 1];
		auto st = 0;
		uint32_t j = 0;
		for (uint32_t i = 0; i < s_len && s[i] != '\0' && j < s_len; i++) {
			if (st == 0) {
				if (s[i] == '\\') {
					st = 1;
				}
				else {
					t[j++] = s[i];
				}
			}
			else if (st == 1) {
				// \0-9
				if (s[i] >= '0' && s[i] <= '8') {
					t[j] = s[i] - '0';
					st = 2;
				}
				// \x
				else if (s[i] == 'x') {
					st = 4;
				}
				// \u
				else if (s[i] == 'u') {
					st = 6;
				}
				// \a
				else if (s[i] == 'a') {
					t[j++] = '\a';
					st = 0;
				}
				// \b
				else if (s[i] == 'b') {
					t[j++] = '\b';
					st = 0;
				}
				// \f
				else if (s[i] == 'f') {
					t[j++] = '\f';
					st = 0;
				}
				// \n
				else if (s[i] == 'n') {
					t[j++] = '\n';
					st = 0;
				}
				// \r
				else if (s[i] == 'r') {
					t[j++] = '\r';
					st = 0;
				}
				// \s
				else if (s[i] == 's') {
					t[j++] = ' ';
					st = 0;
				}
				else if (s[i] == 't') {
					t[j++] = '\t';
					st = 0;
				}
				else if (s[i] == 'v') {
					t[j++] = '\v';
					st = 0;
				}
				else {
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 2) {
				if (s[i] >= '0' && s[i] <= '8') {
					t[j] = t[j] * 8 + s[i] - '0';
					st = 3;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 3) {
				if (s[i] >= '0' && s[i] <= '8') {
					t[j++] = t[j] * 8 + s[i] - '0';
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 4) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(x);
					st = 5;
				}
				else if (s[i] == '\\') {
					st = 1;
				}
				else {
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 5) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j++] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 6) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(x);
					st = 7;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 7) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 8;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 8) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 9;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
			else if (st == 9) {
				uint32_t x;
				if (is_hex(s[i], x)) {
					t[j++] = static_cast<wchar_t>(t[j] * 16 + x);
					st = 0;
				}
				else if (s[i] == '\\') {
					j++;
					st = 1;
				}
				else {
					j++;
					t[j++] = s[i];
					st = 0;
				}
			}
		}
		t[j] = '\0';
		return t;
	}

	// ������̒���.
	// �������k���|�C���^�̏ꍇ, 0 ��Ԃ�.
	uint32_t wchar_len(const wchar_t* const t) noexcept
	{
		return (t == nullptr || t[0] == '\0') ? 0 : static_cast<uint32_t>(wcslen(t));
	}

	// ��邵�̐��@���f�[�^���C�^�[�ɏ�������.
	void dt_write(const ARROW_SIZE& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_width);
		dt_writer.WriteSingle(value.m_length);
		dt_writer.WriteSingle(value.m_offset);
	}

	// �����̒[�_���f�[�^���C�^�[�ɏ�������.
	void dt_write(const CAP_STYLE& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(value.m_start));
		dt_writer.WriteUInt32(static_cast<uint32_t>(value.m_end));
	}

	// �F���f�[�^���C�^�[�ɏ�������.
	void dt_write(const D2D1_COLOR_F& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.a);
		dt_writer.WriteSingle(value.r);
		dt_writer.WriteSingle(value.g);
		dt_writer.WriteSingle(value.b);
	}

	// �ʒu���f�[�^���C�^�[�ɏ�������.
	void dt_write(const D2D1_POINT_2F value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.x);
		dt_writer.WriteSingle(value.y);
	}

	// ���@���f�[�^���C�^�[�ɏ�������.
	void dt_write(const D2D1_SIZE_F value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.width);
		dt_writer.WriteSingle(value.height);
	}

	// ������͈͂��f�[�^���C�^�[�ɏ�������.
	void dt_write(const DWRITE_TEXT_RANGE value, DataWriter const& dt_writer)
	{
		dt_writer.WriteInt32(value.startPosition);
		dt_writer.WriteInt32(value.length);
	}

	// ����̌`�����f�[�^���C�^�[�ɏ�������.
	void dt_write(const GRID_EMPH value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt16(value.m_gauge_1);
		dt_writer.WriteUInt16(value.m_gauge_2);
	}

	// �j���̔z�u���f�[�^���C�^�[�ɏ�������.
	void dt_write(const STROKE_DASH_PATT& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_[0]);
		dt_writer.WriteSingle(value.m_[1]);
		dt_writer.WriteSingle(value.m_[2]);
		dt_writer.WriteSingle(value.m_[3]);
		dt_writer.WriteSingle(value.m_[4]);
		dt_writer.WriteSingle(value.m_[5]);
	}

	// ���p�`�̃c�[�����f�[�^���C�^�[�ɏ�������.
	void dt_write(const POLY_TOOL& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(value.m_vertex_cnt));
		dt_writer.WriteBoolean(value.m_regular);
		dt_writer.WriteBoolean(value.m_vertex_up);
		dt_writer.WriteBoolean(value.m_closed);
		dt_writer.WriteBoolean(value.m_clockwise);
	}

	// ��������f�[�^���C�^�[�ɏ�������
	void dt_write(const wchar_t* value, DataWriter const& dt_writer)
	{
		const uint32_t len = wchar_len(value);

		dt_writer.WriteUInt32(len);
		for (uint32_t i = 0; i < len; i++) {
			dt_writer.WriteUInt16(value[i]);
		}
	}

	// �ʒu�z����f�[�^���C�^�[�ɏ�������
	void dt_write(const std::vector<D2D1_POINT_2F>& value, DataWriter const& dt_writer)
	{
		const size_t n = value.size();

		dt_writer.WriteUInt32(static_cast<uint32_t>(n));
		for (uint32_t i = 0; i < n; i++) {
			dt_write(value[i], dt_writer);
		}
	}

}
