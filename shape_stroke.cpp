#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	/*
	// �}�`���͂ޗ̈�𓾂�.
	// a_lt	���̗̈�̍���ʒu.
	// a_rb	���̗̈�̉E���ʒu.
	// b_lt	�͂ޗ̈�̍���ʒu.
	// b_rb	�͂ޗ̈�̉E���ʒu.
	void ShapeStroke::get_bound(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept
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
	*/

	// �[�̌`���𓾂�.
	// val	����ꂽ�l
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_cap(CAP_STYLE& val) const noexcept
	{
		val = m_stroke_cap;
		return true;
	}

	// ���̌����𓾂�.
	// val	����ꂽ�l
	// �߂�l	�˂� true
	bool ShapeStroke::get_dash_cap(D2D1_CAP_STYLE& val) const noexcept
	{
		val = m_dash_cap;
		return true;
	}

	// �j���̔z�u�𓾂�.
	// val	����ꂽ�l
	// �߂�l	�˂� true
	bool ShapeStroke::get_dash_patt(DASH_PATT& val) const noexcept
	{
		val = m_dash_patt;
		return true;
	}

	// ���g�̌`���𓾂�.
	// val	����ꂽ�l
	// �߂�l	�˂� true
	bool ShapeStroke::get_dash_style(D2D1_DASH_STYLE& val) const noexcept
	{
		val = m_dash_style;
		return true;
	}

	// �����̌����̃}�C�^�[�����𓾂�.
	// val	����ꂽ�l
	// �߂�l	�˂� true
	bool ShapeStroke::get_join_miter_limit(float& val) const noexcept
	{
		val = m_join_miter_limit;
		return true;
	}

	// �����̌����𓾂�.
	// val	����ꂽ�l
	// �߂�l	�˂� true
	bool ShapeStroke::get_join_style(D2D1_LINE_JOIN& val) const noexcept
	{
		val = m_join_style;
		return true;
	}

	// ���g�̐F�𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_color(D2D1_COLOR_F& val) const noexcept
	{
		val = m_stroke_color;
		return true;
	}

	// ���g�̑����𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_width(float& val) const noexcept
	{
		val = m_stroke_width;
		return true;
	}

	// �ʒu���܂ނ����肷��.
	// �߂�l	�˂� ANC_PAGE
	uint32_t ShapeStroke::hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept
	{
		return ANC_TYPE::ANC_PAGE;
	}

	// D2D �X�g���[�N�X�^�C�����쐬����.
	void ShapeStroke::create_stroke_style(ID2D1Factory* const factory)
	{
		UINT32 d_cnt;	// �j���̔z�u�̗v�f��
		FLOAT d_arr[6];	// �j���̔z�u
		FLOAT* d_ptr;
		D2D1_DASH_STYLE d_style;

		// �������[�������肷��.
		if (m_stroke_width < FLT_MIN) {
			return;
		}
		if (m_dash_style == D2D1_DASH_STYLE::D2D1_DASH_STYLE_DOT) {
			d_arr[0] = m_dash_patt.m_[2] / m_stroke_width;
			d_arr[1] = m_dash_patt.m_[3] / m_stroke_width;
			d_ptr = d_arr;
			d_cnt = 2;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
		}
		else if (m_dash_style == D2D1_DASH_STYLE_DASH) {
			d_arr[0] = m_dash_patt.m_[0] / m_stroke_width;
			d_arr[1] = m_dash_patt.m_[1] / m_stroke_width;
			d_ptr = d_arr;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 2;
		}
		else if (m_dash_style == D2D1_DASH_STYLE_DASH_DOT) {
			d_arr[0] = m_dash_patt.m_[0] / m_stroke_width;
			d_arr[1] = m_dash_patt.m_[1] / m_stroke_width;
			d_arr[2] = m_dash_patt.m_[2] / m_stroke_width;
			d_arr[3] = m_dash_patt.m_[3] / m_stroke_width;
			d_ptr = d_arr;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 4;
		}
		else if (m_dash_style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
			d_arr[0] = m_dash_patt.m_[0] / m_stroke_width;
			d_arr[1] = m_dash_patt.m_[1] / m_stroke_width;
			d_arr[2] = m_dash_patt.m_[2] / m_stroke_width;
			d_arr[3] = m_dash_patt.m_[3] / m_stroke_width;
			d_arr[4] = m_dash_patt.m_[4] / m_stroke_width;
			d_arr[5] = m_dash_patt.m_[5] / m_stroke_width;
			d_ptr = d_arr;
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM;
			d_cnt = 6;
		}
		else {
			d_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;
			d_ptr = nullptr;
			d_cnt = 0;
		}
		const D2D1_STROKE_STYLE_PROPERTIES s_prop{
			m_stroke_cap.m_start,	// startCap
			m_stroke_cap.m_end,	// endCap
			m_dash_cap,	// dashCap
			m_join_style,	// lineJoin
			m_join_miter_limit,	// miterLimit
			d_style,	// dashStyle
			0.0f,
		};
		winrt::check_hresult(factory->CreateStrokeStyle(s_prop, d_ptr, d_cnt, m_d2d_stroke_style.put()));
	}

	// �l��[�̌`���Ɋi�[����.
	bool ShapeStroke::set_stroke_cap(const CAP_STYLE& val) noexcept
	{
		if (!equal(m_stroke_cap, val)) {
			m_stroke_cap = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l��j���̒[�̌`���Ɋi�[����.
	bool ShapeStroke::set_dash_cap(const D2D1_CAP_STYLE& val) noexcept
	{
		if (!equal(m_dash_cap, val)) {
			m_dash_cap = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l��j���̔z�u�Ɋi�[����.
	// val	�i�[����l
	bool ShapeStroke::set_dash_patt(const DASH_PATT& val) noexcept
	{
		if (!equal(m_dash_patt, val)) {
			m_dash_patt = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l����g�̌`���Ɋi�[����.
	// val	�i�[����l
	bool ShapeStroke::set_dash_style(const D2D1_DASH_STYLE val) noexcept
	{
		if (m_dash_style != val) {
			m_dash_style = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l������̌����̃}�C�^�[�����Ɋi�[����.
	// val	�i�[����l
	bool ShapeStroke::set_join_miter_limit(const float& val) noexcept
	{
		if (!equal(m_join_miter_limit, val)) {
			m_join_miter_limit = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// �l������̌����Ɋi�[����.
	// val	�i�[����l
	bool ShapeStroke::set_join_style(const D2D1_LINE_JOIN& val)  noexcept
	{
		if (m_join_style != val) {
			m_join_style = val;
			if (m_d2d_stroke_style != nullptr) {
				m_d2d_stroke_style = nullptr;
			}
			return true;
		}
		return false;
	}

	// ���g�̐F�Ɋi�[����.
	bool ShapeStroke::set_stroke_color(const D2D1_COLOR_F& val) noexcept
	{
		if (!equal(m_stroke_color, val)) {
			m_stroke_color = val;
			return true;
		}
		return false;
	}

	// �l����g�̑����Ɋi�[����.
	// val	�i�[����l
	bool ShapeStroke::set_stroke_width(const float val) noexcept
	{
		if (!equal(m_stroke_width, val)) {
			m_stroke_width = val;
			if (m_dash_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID) {
				if (m_d2d_stroke_style != nullptr) {
					m_d2d_stroke_style = nullptr;
				}
			}
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// page	�ݒ�
	ShapeStroke::ShapeStroke(const ShapePage* page) :
		m_dash_cap(page->m_dash_cap),
		m_stroke_cap(page->m_stroke_cap),
		m_stroke_color(page->m_stroke_color),
		m_dash_patt(page->m_dash_patt),
		m_dash_style(page->m_dash_style),
		m_join_miter_limit(page->m_join_miter_limit),
		m_join_style(page->m_join_style),
		m_stroke_width(page->m_stroke_width),
		m_d2d_stroke_style(nullptr)
	{}

	/*
	static std::vector<D2D1_POINT_2F> dt_read_vec(DataReader const& dt_reader)
	{
		const size_t vec_cnt = dt_reader.ReadUInt32();	// �v�f��
		std::vector<D2D1_POINT_2F> vec(vec_cnt);
		for (size_t i = 0; i < vec_cnt; i++) {
			vec[i].x = dt_reader.ReadSingle();
			vec[i].y = dt_reader.ReadSingle();
		}
		return vec;
	}
	*/

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeStroke::ShapeStroke(const ShapePage& page, DataReader const& dt_reader) :
		// �ǂݍ��ޏ��Ԃ͒�`���ꂽ��
		ShapeSelect(dt_reader),
		m_stroke_cap(CAP_STYLE{
			static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32()),
			static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32())
		}),
		m_stroke_color(D2D1_COLOR_F{
			dt_reader.ReadSingle(), 
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
		}),
		m_stroke_width(dt_reader.ReadSingle()),
		m_dash_cap(static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32())),
		m_dash_patt(DASH_PATT{
			{
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle(),
			dt_reader.ReadSingle()
			}
		}),
		m_dash_style(static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32())),
		m_join_miter_limit(dt_reader.ReadSingle()),
		m_join_style(static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32())),
		m_d2d_stroke_style(nullptr)
	{
		if ((m_stroke_cap.m_start != D2D1_CAP_STYLE_FLAT &&
			m_stroke_cap.m_start != D2D1_CAP_STYLE_ROUND &&
			m_stroke_cap.m_start != D2D1_CAP_STYLE_SQUARE &&
			m_stroke_cap.m_start != D2D1_CAP_STYLE_TRIANGLE) ||
			m_stroke_cap.m_start != m_stroke_cap.m_end) {
			m_stroke_cap = page.m_stroke_cap;
		}
		if (m_stroke_color.r < 0.0f || m_stroke_color.r > 1.0f ||
			m_stroke_color.g < 0.0f || m_stroke_color.g > 1.0f ||
			m_stroke_color.b < 0.0f || m_stroke_color.b > 1.0f ||
			m_stroke_color.a < 0.0f || m_stroke_color.a > 1.0f) {
			m_stroke_color = page.m_stroke_color;
		}
		if (m_dash_cap != D2D1_CAP_STYLE_FLAT &&
			m_dash_cap != D2D1_CAP_STYLE_ROUND &&
			m_dash_cap != D2D1_CAP_STYLE_SQUARE &&
			m_dash_cap != D2D1_CAP_STYLE_TRIANGLE) {
			m_dash_cap = page.m_dash_cap;
		}
		if (m_dash_style != D2D1_DASH_STYLE_SOLID &&
			m_dash_style != D2D1_DASH_STYLE_CUSTOM) {
			m_dash_style = page.m_dash_style;
		}
		if (m_join_style != D2D1_LINE_JOIN_BEVEL &&
			m_join_style != D2D1_LINE_JOIN_ROUND &&
			m_join_style != D2D1_LINE_JOIN_MITER &&
			m_join_style != D2D1_LINE_JOIN_MITER_OR_BEVEL) {
			m_join_style = page.m_join_style;
		}
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	void ShapeStroke::write(DataWriter const& dt_writer) const
	{
		ShapeSelect::write(dt_writer);
		/*
		// �J�n�ʒu
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);

		// ���̈ʒu�ւ̍���
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_vec.size()));
		for (const D2D1_POINT_2F vec : m_vec) {
			dt_writer.WriteSingle(vec.x);
			dt_writer.WriteSingle(vec.y);
		}
		*/
		// ���̒[�̌`��
		dt_writer.WriteUInt32(m_stroke_cap.m_start);
		dt_writer.WriteUInt32(m_stroke_cap.m_end);
		// ���E�g�̐F
		dt_writer.WriteSingle(m_stroke_color.r);
		dt_writer.WriteSingle(m_stroke_color.g);
		dt_writer.WriteSingle(m_stroke_color.b);
		dt_writer.WriteSingle(m_stroke_color.a);
		// ���E�g�̑���
		dt_writer.WriteSingle(m_stroke_width);
		//dt_writer.WriteUInt32(m_dash_cap);
		dt_writer.WriteUInt32(m_dash_cap);
		dt_writer.WriteSingle(m_dash_patt.m_[0]);
		dt_writer.WriteSingle(m_dash_patt.m_[1]);
		dt_writer.WriteSingle(m_dash_patt.m_[2]);
		dt_writer.WriteSingle(m_dash_patt.m_[3]);
		dt_writer.WriteSingle(m_dash_patt.m_[4]);
		dt_writer.WriteSingle(m_dash_patt.m_[5]);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_dash_style));
		dt_writer.WriteSingle(m_join_miter_limit);
		dt_writer.WriteUInt32(m_join_style);
	}

}