#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// D2D �X�g���[�N�������쐬����.
	static void create_stroke_style(const D2D1_DASH_STYLE d_style, const STROKE_PATT& s_patt, ID2D1StrokeStyle** s_style);

	// D2D �X�g���[�N�������쐬����.
	// d_style	�j���̎��
	// s_patt	�j���̔z�u�z��
	// s_style	�쐬���ꂽ�X�g���[�N����
	static void create_stroke_style(const D2D1_DASH_STYLE d_style, const STROKE_PATT& s_patt, ID2D1StrokeStyle** s_style)
	{
		D2D1_STROKE_STYLE_PROPERTIES prop{
			D2D1_CAP_STYLE_SQUARE,	// startCap
			D2D1_CAP_STYLE_SQUARE,	// endCap
			D2D1_CAP_STYLE_FLAT,	// dashCap
			D2D1_LINE_JOIN_MITER,	// lineJoin
			1.0f,					// miterLimit
			D2D1_DASH_STYLE_CUSTOM,	// dashStyle
			0.0f
		};
		UINT32 d_cnt;
		const FLOAT* d_arr;

		if (d_style != D2D1_DASH_STYLE_SOLID) {
			if (d_style == D2D1_DASH_STYLE_DOT) {
				d_arr = s_patt.m_ + 2;
				d_cnt = 2;
			}
			else {
				d_arr = s_patt.m_;
				if (d_style == D2D1_DASH_STYLE_DASH) {
					d_cnt = 2;
				}
				else if (d_style == D2D1_DASH_STYLE_DASH_DOT) {
					d_cnt = 4;
				}
				else if (d_style == D2D1_DASH_STYLE_DASH_DOT_DOT) {
					d_cnt = 6;
				}
				else {
					d_cnt = 0;
				}
			}
			winrt::check_hresult(
				Shape::s_d2d_factory->CreateStrokeStyle(prop, d_arr, d_cnt, s_style)
			);
		}
		else {
			*s_style = nullptr;
		}
	}

	// �}�`��j������.
	ShapePoly::~ShapePoly(void)
	{
		m_poly_geom = nullptr;
	}

	// �}�`���͂ޗ̈�𓾂�.
	// b_min	�̈�̍���ʒu.
	// b_max	�̈�̉E���ʒu.
	void ShapePoly::get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		D2D1_POINT_2F e_pos;
		pt_inc(m_pos, b_min, b_max);
		pt_add(m_pos, m_diff, e_pos);
		pt_inc(e_pos, b_min, b_max);
		pt_add(e_pos, m_diff_1, e_pos);
		pt_inc(e_pos, b_min, b_max);
		pt_add(e_pos, m_diff_2, e_pos);
		pt_inc(e_pos, b_min, b_max);
	}

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	// value	�̈�̍���ʒu
	void ShapePoly::get_min_pos(D2D1_POINT_2F& value) const noexcept
	{
		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_diff, e_pos);
		pt_min(m_pos, e_pos, value);
		pt_add(e_pos, m_diff_1, e_pos);
		pt_min(value, e_pos, value);
		pt_add(e_pos, m_diff_2, e_pos);
		pt_min(value, e_pos, value);
	}

	// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
	void ShapePoly::get_pos(const ANCH_WHICH a, D2D1_POINT_2F& value) const noexcept
	{
		switch (a) {
		case ANCH_WHICH::ANCH_OUTSIDE:
			value = m_pos;
			break;
		case ANCH_WHICH::ANCH_R_NW:
			value = m_pos;
			break;
		case ANCH_WHICH::ANCH_R_NE:
			pt_add(m_pos, m_diff, value);
			break;
		case ANCH_WHICH::ANCH_R_SW:
			pt_add(m_pos, m_diff, value);
			pt_add(value, m_diff_1, value);
			break;
		case ANCH_WHICH::ANCH_R_SE:
			pt_add(m_pos, m_diff, value);
			pt_add(value, m_diff_1, value);
			pt_add(value, m_diff_2, value);
			break;
		default:
			return;
		}
	}

	// ���������ړ�����.
	void ShapePoly::move(const D2D1_POINT_2F diff)
	{
		ShapeStroke::move(diff);
		create_path_geometry();
	}

	// �l���w�肵�����ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
	void ShapePoly::set_pos(const D2D1_POINT_2F value, const ANCH_WHICH a)
	{
		D2D1_POINT_2F a_pos;
		D2D1_POINT_2F diff;

		switch (a) {
		case ANCH_WHICH::ANCH_OUTSIDE:
			m_pos = value;
			break;
		case ANCH_WHICH::ANCH_R_NW:
			pt_sub(value, m_pos, diff);
			m_pos = value;
			pt_sub(m_diff, diff, m_diff);
			break;
		case ANCH_WHICH::ANCH_R_NE:
			get_pos(ANCH_WHICH::ANCH_R_NE, a_pos);
			pt_sub(value, a_pos, diff);
			pt_add(m_diff, diff, m_diff);
			pt_sub(m_diff_1, diff, m_diff_1);
			break;
		case ANCH_WHICH::ANCH_R_SW:
			get_pos(ANCH_WHICH::ANCH_R_SW, a_pos);
			pt_sub(value, a_pos, diff);
			pt_add(m_diff_1, diff, m_diff_1);
			pt_sub(m_diff_2, diff, m_diff_2);
			break;
		case ANCH_WHICH::ANCH_R_SE:
			get_pos(ANCH_WHICH::ANCH_R_SE, a_pos);
			pt_sub(value, a_pos, diff);
			pt_add(m_diff_2, diff, m_diff_2);
			break;
		default:
			return;
		}
		create_path_geometry();
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	void ShapePoly::set_start_pos(const D2D1_POINT_2F value)
	{
		ShapeStroke::set_start_pos(value);
		create_path_geometry();
	}

	// �}�`���쐬����.
	ShapePoly::ShapePoly(const ShapeSheet* attr) :
		ShapeStroke::ShapeStroke(attr)
	{}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapePoly::ShapePoly(DataReader const& dt_reader) :
		ShapeStroke::ShapeStroke(dt_reader)
	{
		using winrt::GraphPaper::implementation::read;

		read(m_diff_1, dt_reader);
		read(m_diff_2, dt_reader);
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapePoly::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		ShapeStroke::write(dt_writer);
		write(m_diff_1, dt_writer);
		write(m_diff_2, dt_writer);
	}

	// �}�`��j������.
	ShapeStroke::~ShapeStroke(void)
	{
		m_d2d_stroke_style = nullptr;
	}

	// �}�`���͂ޗ̈�𓾂�.
	// b_min	�̈�̍���ʒu
	// b_max	�̈�̉E���ʒu
	void ShapeStroke::get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		pt_inc(m_pos, b_min, b_max);
		D2D1_POINT_2F e_pos;
		pt_add(m_pos, m_diff, e_pos);
		pt_inc(e_pos, b_min, b_max);
	}

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	// value	�̈�̍���ʒu
	void ShapeStroke::get_min_pos(D2D1_POINT_2F& value) const noexcept
	{
		value.x = m_diff.x >= 0.0f ? m_pos.x : m_pos.x + m_diff.x;
		value.y = m_diff.y >= 0.0f ? m_pos.y : m_pos.y + m_diff.y;
	}

	// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
	// �߂�l	�˂� true
	void ShapeStroke::get_pos(const ANCH_WHICH /*a*/, D2D1_POINT_2F& value) const noexcept
	{
		value = m_pos;
	}

	// �J�n�ʒu�𓾂�
	// �߂�l	�˂� true
	bool ShapeStroke::get_start_pos(D2D1_POINT_2F& value) const noexcept
	{
		value = m_pos;
		return true;
	}

	// ���g�̐F�𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_color(D2D1_COLOR_F& value) const noexcept
	{
		value = m_stroke_color;
		return true;
	}

	// �j���̔z�u�𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_patt(STROKE_PATT& value) const noexcept
	{
		value = m_stroke_patt;
		return true;
	}

	// ���g�̌`���𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_style(D2D1_DASH_STYLE& value) const noexcept
	{
		value = m_stroke_style;
		return true;
	}

	// ���g�̑����𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_width(double& value) const noexcept
	{
		value = m_stroke_width;
		return true;
	}

	// �ʒu���܂ނ����ׂ�.
	// �߂�l	�˂� ANCH_OUTSIDE
	ANCH_WHICH ShapeStroke::hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept
	{
		return ANCH_WHICH::ANCH_OUTSIDE;
	}

	// �͈͂Ɋ܂܂�邩���ׂ�.
	// �߂�l	�˂� false
	bool ShapeStroke::in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept
	{
		return false;
	}

	// ���������ړ�����.
	void ShapeStroke::move(const D2D1_POINT_2F diff)
	{
		pt_add(m_pos, diff, m_pos);
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	void ShapeStroke::set_start_pos(const D2D1_POINT_2F value)
	{
		m_pos = value;
	}

	// ���g�̐F�Ɋi�[����.
	void ShapeStroke::set_stroke_color(const D2D1_COLOR_F& value) noexcept
	{
		m_stroke_color = value;
	}

	// �����f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	// barbs	���̗��[�̈ʒu
	// tip_pos	���̐�[�̈ʒu
	// a_style	���̌`��
	// dt_writer	�f�[�^���C�^�[
	void ShapeStroke::write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, const ARROW_STYLE a_style, DataWriter const& dt_writer) const
	{
		using  winrt::GraphPaper::implementation::write_svg;

		write_svg("<path d=\"", dt_writer);
		write_svg("M", dt_writer);
		write_svg(barbs[0].x, dt_writer);
		write_svg(barbs[0].y, dt_writer);
		write_svg("L", dt_writer);
		write_svg(tip_pos.x, dt_writer);
		write_svg(tip_pos.y, dt_writer);
		write_svg("L", dt_writer);
		write_svg(barbs[1].x, dt_writer);
		write_svg(barbs[1].y, dt_writer);
		write_svg("\" ", dt_writer);
		if (a_style == ARROW_STYLE::FILLED) {
			write_svg(m_stroke_color, "fill", dt_writer);
		}
		else {
			write_svg("fill=\"none\" ", dt_writer);
		}
		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		write_svg(" />" SVG_NL, dt_writer);
	}

	// �l��j���̔z�u�Ɋi�[����.
	void ShapeStroke::set_stroke_patt(const STROKE_PATT& value)
	{
		if (equal(m_stroke_patt, value)) {
			return;
		}
		m_stroke_patt = value;
		m_d2d_stroke_style = nullptr;
		create_stroke_style(m_stroke_style, m_stroke_patt, m_d2d_stroke_style.put());
	}

	// �l����g�̌`���Ɋi�[����.
	void ShapeStroke::set_stroke_style(const D2D1_DASH_STYLE value)
	{
		if (equal(m_stroke_style, value)) {
			return;
		}
		m_stroke_style = value;
		m_d2d_stroke_style = nullptr;
		create_stroke_style(m_stroke_style, m_stroke_patt, m_d2d_stroke_style.put());
	}

	// �l����g�̑����Ɋi�[����.
	void ShapeStroke::set_stroke_width(const double value) noexcept
	{
		m_stroke_width = value;
	}

	// �}�`���쐬����.
	ShapeStroke::ShapeStroke(const ShapeSheet* attr) :
		m_stroke_color(attr->m_stroke_color),
		m_stroke_patt(attr->m_stroke_patt),
		m_stroke_style(attr->m_stroke_style),
		m_stroke_width(attr->m_stroke_width),
		m_d2d_stroke_style(nullptr)
	{
		create_stroke_style(m_stroke_style, m_stroke_patt, m_d2d_stroke_style.put());
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeStroke::ShapeStroke(DataReader const& dt_reader) :
		m_d2d_stroke_style(nullptr)
	{
		using winrt::GraphPaper::implementation::read;

		set_delete(dt_reader.ReadBoolean());
		set_select(dt_reader.ReadBoolean());
		read(m_pos, dt_reader);
		read(m_diff, dt_reader);
		read(m_stroke_color, dt_reader);
		read(m_stroke_patt, dt_reader);
		read(m_stroke_style, dt_reader);
		m_stroke_width = dt_reader.ReadDouble();
		create_stroke_style(m_stroke_style, m_stroke_patt, m_d2d_stroke_style.put());
	}

	// �f�[�^���C�^�[�ɏ�������.
	void ShapeStroke::write(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write;

		dt_writer.WriteBoolean(is_deleted());
		dt_writer.WriteBoolean(is_selected());
		write(m_pos, dt_writer);
		write(m_diff, dt_writer);
		write(m_stroke_color, dt_writer);
		dt_writer.WriteSingle(m_stroke_patt.m_[0]);
		dt_writer.WriteSingle(m_stroke_patt.m_[1]);
		dt_writer.WriteSingle(m_stroke_patt.m_[2]);
		dt_writer.WriteSingle(m_stroke_patt.m_[3]);
		dt_writer.WriteSingle(m_stroke_patt.m_[4]);
		dt_writer.WriteSingle(m_stroke_patt.m_[5]);
		dt_writer.WriteInt32(static_cast<int32_t>(m_stroke_style));
		dt_writer.WriteDouble(m_stroke_width);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeStroke::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_style, m_stroke_patt, m_stroke_width, dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
	}

}