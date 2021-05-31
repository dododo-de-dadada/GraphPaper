//------------------------------
// shape.cpp
// �}�`�̂ЂȌ^, ���̑�
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	ID2D1Factory3* Shape::s_d2d_factory = nullptr;	// D2D1 �t�@�N�g��
	IDWriteFactory3* Shape::s_dwrite_factory = nullptr;	// DWRITE �t�@�N�g��
#if defined(_DEBUG)
	uint32_t debug_leak_cnt = 0;
	uint32_t debug_shape_cnt = 0;
	uint32_t debug_deleted_cnt = 0;
#endif

	// ������ '0'...'9' �܂��� 'A'...'F', 'a'...'f' �����肷��.
	static bool is_hex(const wchar_t w, uint32_t& x) noexcept;

	// �}�`�̕��� (���`) ��\������.
	// a_pos	���ʂ̈ʒu
	// dx		�}�`�̕`���
	void anchor_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		D2D1_POINT_2F r_min;
		pt_add(a_pos, -0.5 * dx.m_anchor_len, r_min);
		D2D1_POINT_2F r_max;
		pt_add(r_min, dx.m_anchor_len, r_max);
		const D2D1_RECT_F r{ r_min.x, r_min.y, r_max.x, r_max.y };

		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->DrawRectangle(r, dx.m_shape_brush.get(), 2.0, nullptr);
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->FillRectangle(r, dx.m_shape_brush.get());
	}

	// �}�`�̕��ʁi�~�`�j��\������.
	// a_pos	���ʂ̈ʒu
	// dx		�}�`�̕`���
	void anchor_draw_ellipse(const D2D1_POINT_2F a_pos, SHAPE_DX& dx)
	{
		const FLOAT rad = static_cast<FLOAT>(dx.m_anchor_len * 0.5 + 1.0);
		dx.m_shape_brush->SetColor(dx.m_theme_background);
		dx.m_d2dContext->FillEllipse({ a_pos, rad, rad }, dx.m_shape_brush.get());
		dx.m_shape_brush->SetColor(dx.m_theme_foreground);
		dx.m_d2dContext->FillEllipse({ a_pos, rad - 1.0F, rad - 1.0F }, dx.m_shape_brush.get());
	}

	// ���̕Ԃ��̈ʒu�����߂�.
	// a_vec	��x�N�g��.
	// a_len	��x�N�g���̒���
	// h_width	���̕� (�Ԃ��̊Ԃ̒���)
	// h_len	���̒��� (��[����Ԃ��܂ł̎��x�N�g����ł̒���)
	// b_vec[2]	�v�Z���ꂽ���̕Ԃ��̈ʒu (��[����̃I�t�Z�b�g)
	void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F b_vec[]) noexcept
	{
		if (a_len <= DBL_MIN) {
			constexpr D2D1_POINT_2F Z{ 0.0f, 0.0f };
			b_vec[0] = Z;
			b_vec[1] = Z;
		}
		else {
			const double hf = h_width * 0.5;	// ���̕��̔����̑傫��
			const double sx = a_vec.x * -h_len;	// ��x�N�g������̒��������]
			const double sy = a_vec.x * hf;
			const double tx = a_vec.y * -h_len;
			const double ty = a_vec.y * hf;
			const double ax = 1.0 / a_len;
			b_vec[0].x = static_cast<FLOAT>((sx - ty) * ax);
			b_vec[0].y = static_cast<FLOAT>((tx + sy) * ax);
			b_vec[1].x = static_cast<FLOAT>((sx + ty) * ax);
			b_vec[1].y = static_cast<FLOAT>((tx - sy) * ax);
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

	// ��_�̓��ς𓾂�.
	//double pt_dot(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept
	//{
	//	return static_cast<double>(a.x) * b.x + static_cast<double>(a.y) * b.y;
	//}

	// �}�`�̕��ʂ��ʒu { 0,0 } ���܂ނ����肷��.
	// a_pos	���ʂ̈ʒu
	// a_len	���ʂ̈�ӂ̒���.
	// �߂�l	�܂ޏꍇ true
	// �A���J�[������ 0 ����łȂ���΂Ȃ�Ȃ�.
	bool pt_in_anch(const D2D1_POINT_2F a_pos, const double a_len) noexcept
	{
		D2D1_POINT_2F a_min;	// ���ʂ̈ʒu�𒆓_�Ƃ�����`�̍���_
		pt_add(a_pos, a_len * -0.5, a_min);
		return a_min.x <= 0.0f && 0.0f <= a_min.x + a_len && a_min.y <= 0.0f && 0.0f <= a_min.y + a_len;
	}

	// �}�`�̕��ʂ��ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// a_pos	���ʂ̈ʒu
	// a_len	���ʂ̈�ӂ̒���.
	// �߂�l	�܂ޏꍇ true
	// �A���J�[������ 0 ����łȂ���΂Ȃ�Ȃ�.
	bool pt_in_anch(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos, const double a_len) noexcept
	{
		D2D1_POINT_2F a_tran;
		pt_sub(a_pos, t_pos, a_tran);
		return pt_in_anch(a_tran, a_len);
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
		// ���S�_�����_�ɂȂ�悤���肷��ʒu���ړ�����.
		/*
		const double tx = t_pos.x;
		const double ty = t_pos.y;
		const double cx = c_pos.x;
		const double cy = c_pos.y;
		double px = tx - cx;
		double py = ty - cy;
		const double rx = fabs(rad_x);
		const double ry = fabs(rad_y);
		if (ry <= FLT_MIN) {
			return fabs(py) <= FLT_MIN && fabs(px) <= rx;
		}
		else if (rx <= FLT_MIN) {
			return fabs(py) <= ry && fabs(px) <= FLT_MIN;
		}
		px /= rx;
		py /= ry;
		return px * px + py * py <= 1.0;
		*/
	}

	// �������ʒu���܂ނ�, �������l�����Ĕ��肷��.
	// t_pos	���肷��ʒu
	// s_pos	�����̎n�[
	// e_pos	�����̏I�[
	// s_width	�����̑���
	// �߂�l	�܂ޏꍇ true
	bool pt_in_line(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width) noexcept
	{
		D2D1_POINT_2F diff;	// ���������̃x�N�g��
		pt_sub(e_pos, s_pos, diff);
		const double abs = pt_abs2(diff);
		if (abs <= FLT_MIN) {
			return equal(t_pos, s_pos);
		}
		// �����̖@���x�N�g�������߂�.
		// �@���x�N�g���̒�����, ���̑����̔����Ƃ���.
		// ������ 0.5 �����̏ꍇ��, 0.5 �Ƃ���.
		pt_mul(diff, max(s_width * 0.5, 0.5) / sqrt(abs), diff);
		const double nx = diff.y;
		const double ny = -diff.x;
		// �����̗��[����, �@���x�N�g���̕���, �܂��͂��̋t�̕����ɂ���_�����߂�.
		// ���߂� 4 �_����Ȃ�l�ӌ`���ʒu���܂ނ����肷��.
		D2D1_POINT_2F exp_side[4];
		pt_add(s_pos, nx, ny, exp_side[0]);
		pt_add(e_pos, nx, ny, exp_side[1]);
		pt_add(e_pos, -nx, -ny, exp_side[2]);
		pt_add(s_pos, -nx, -ny, exp_side[3]);
		return pt_in_poly(t_pos, 4, exp_side);
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

	// ���̐��@���f�[�^���[�_�[����ǂݍ���.
	void read(ARROWHEAD_SIZE& value, DataReader const& dt_reader)
	{
		value.m_width = dt_reader.ReadSingle();
		value.m_length = dt_reader.ReadSingle();
		value.m_offset = dt_reader.ReadSingle();
	}

	// �F���f�[�^���[�_�[����ǂݍ���.
	void read(D2D1_COLOR_F& value, DataReader const& dt_reader)
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
	void read(D2D1_POINT_2F& value, DataReader const& dt_reader)
	{
		value.x = dt_reader.ReadSingle();
		value.y = dt_reader.ReadSingle();
	}

	// ���@���f�[�^���[�_�[����ǂݍ���.
	void read(D2D1_SIZE_F& value, DataReader const& dt_reader)
	{
		value.width = dt_reader.ReadSingle();
		value.height = dt_reader.ReadSingle();
	}

	// ������͈͂��f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_TEXT_RANGE& value, DataReader const& dt_reader)
	{
		value.startPosition = dt_reader.ReadUInt32();
		value.length = dt_reader.ReadUInt32();
	}

	// �j���̔z�u���f�[�^���[�_�[����ǂݍ���.
	void read(STROKE_DASH_PATT& value, DataReader const& dt_reader)
	{
		value.m_[0] = dt_reader.ReadSingle();
		value.m_[1] = dt_reader.ReadSingle();
		value.m_[2] = dt_reader.ReadSingle();
		value.m_[3] = dt_reader.ReadSingle();
		value.m_[4] = dt_reader.ReadSingle();
		value.m_[5] = dt_reader.ReadSingle();
	}

	// ���p�`�̃c�[�����f�[�^���[�_�[����ǂݍ���.
	void read(TOOL_POLY& value, DataReader const& dt_reader)
	{
		value.m_vertex_cnt = dt_reader.ReadUInt32();
		value.m_regular = dt_reader.ReadBoolean();
		value.m_vertex_up = dt_reader.ReadBoolean();
		value.m_closed = dt_reader.ReadBoolean();
		value.m_clockwise = dt_reader.ReadBoolean();
	}

	// ��������f�[�^���[�_�[����ǂݍ���.
	void read(wchar_t*& value, DataReader const& dt_reader)
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
	void read(std::vector<D2D1_POINT_2F>& value, DataReader const& dt_reader)
	{
		const size_t cnt = dt_reader.ReadUInt32();	// �v�f��
		value.resize(cnt);
		for (size_t i = 0; i < cnt; i++) {
			read(value[i], dt_reader);
		}
	}

	// ����̋������f�[�^���[�_�[����ǂݍ���.
	void read(GRID_EMPH& value, DataReader const& dt_reader)
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

	// ���̐��@���f�[�^���C�^�[�ɏ�������.
	void write(const ARROWHEAD_SIZE& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_width);
		dt_writer.WriteSingle(value.m_length);
		dt_writer.WriteSingle(value.m_offset);
	}

	// �F���f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_COLOR_F& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.a);
		dt_writer.WriteSingle(value.r);
		dt_writer.WriteSingle(value.g);
		dt_writer.WriteSingle(value.b);
	}

	// �ʒu���f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_POINT_2F value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.x);
		dt_writer.WriteSingle(value.y);
	}

	// ���@���f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_SIZE_F value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.width);
		dt_writer.WriteSingle(value.height);
	}

	// ������͈͂��f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_TEXT_RANGE value, DataWriter const& dt_writer)
	{
		dt_writer.WriteInt32(value.startPosition);
		dt_writer.WriteInt32(value.length);
	}

	// ����̌`�����f�[�^���C�^�[�ɏ�������.
	void write(const GRID_EMPH value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt16(value.m_gauge_1);
		dt_writer.WriteUInt16(value.m_gauge_2);
	}

	// �j���̔z�u���f�[�^���C�^�[�ɏ�������.
	void write(const STROKE_DASH_PATT& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(value.m_[0]);
		dt_writer.WriteSingle(value.m_[1]);
		dt_writer.WriteSingle(value.m_[2]);
		dt_writer.WriteSingle(value.m_[3]);
		dt_writer.WriteSingle(value.m_[4]);
		dt_writer.WriteSingle(value.m_[5]);
	}

	// ���p�`�̃c�[�����f�[�^���C�^�[�ɏ�������.
	void write(const TOOL_POLY& value, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(value.m_vertex_cnt));
		dt_writer.WriteBoolean(value.m_regular);
		dt_writer.WriteBoolean(value.m_vertex_up);
		dt_writer.WriteBoolean(value.m_closed);
		dt_writer.WriteBoolean(value.m_clockwise);
	}

	// ��������f�[�^���C�^�[�ɏ�������
	void write(const wchar_t* value, DataWriter const& dt_writer)
	{
		const uint32_t len = wchar_len(value);

		dt_writer.WriteUInt32(len);
		for (uint32_t i = 0; i < len; i++) {
			dt_writer.WriteUInt16(value[i]);
		}
	}

	// �ʒu�z����f�[�^���C�^�[�ɏ�������
	void write(const std::vector<D2D1_POINT_2F>& value, DataWriter const& dt_writer)
	{
		const size_t n = value.size();

		dt_writer.WriteUInt32(static_cast<uint32_t>(n));
		for (uint32_t i = 0; i < n; i++) {
			write(value[i], dt_writer);
		}
	}

}
