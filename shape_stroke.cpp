#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	// D2D �X�g���[�N�������쐬����.
	static void create_stroke_dash_style(ID2D1Factory3* const d_factory, const D2D1_DASH_STYLE d_style, const STROKE_DASH_PATT& s_patt, const D2D1_LINE_JOIN line_join, const double miter_limit, ID2D1StrokeStyle** s_style);

	// D2D �X�g���[�N�������쐬����.
	// d_style	�j���̎��
	// s_patt	�j���̔z�u�z��
	// s_style	�쐬���ꂽ�X�g���[�N����
	// j_style	�p�̌`��
	// j_limit	�p�̃}�C�^�[����
	static void create_stroke_dash_style(ID2D1Factory3* const d_factory, const D2D1_DASH_STYLE d_style, const STROKE_DASH_PATT& s_patt, const D2D1_LINE_JOIN j_style, const double j_limit, ID2D1StrokeStyle** s_style)
	{
		UINT32 d_cnt;	// �j���̔z�u�z��̗v�f��
		const FLOAT* d_arr;	// �j���̔z�u�z����w���|�C���^

		if (d_style != D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID) {
			const D2D1_STROKE_STYLE_PROPERTIES prop{
				D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// startCap
				D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// endCap
				D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT,	// dashCap
				j_style,	// lineJoin
				static_cast<FLOAT>(j_limit),	// miterLimit
				D2D1_DASH_STYLE::D2D1_DASH_STYLE_CUSTOM,	// dashStyle
				0.0f
			};
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
				d_factory->CreateStrokeStyle(prop, d_arr, d_cnt, s_style)
			);
		}
		else {
			const D2D1_STROKE_STYLE_PROPERTIES prop{
				D2D1_CAP_STYLE_FLAT,	// startCap
				D2D1_CAP_STYLE_FLAT,	// endCap
				D2D1_CAP_STYLE_FLAT,	// dashCap
				j_style,	// lineJoin
				static_cast<FLOAT>(j_limit),	// miterLimit
				D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID,	// dashStyle
				0.0f	// dashOffset
			};
			winrt::check_hresult(
				d_factory->CreateStrokeStyle(prop, nullptr, 0, s_style)
			);
		}
	}

	// �}�`��j������.
	ShapeStroke::~ShapeStroke(void)
	{
		m_d2d_stroke_dash_style = nullptr;
	}

	// �}�`���͂ޗ̈�𓾂�.
	// a_min	���̗̈�̍���ʒu.
	// a_man	���̗̈�̉E���ʒu.
	// b_min	����ꂽ�̈�̍���ʒu.
	// b_max	����ꂽ�̈�̉E���ʒu.
	void ShapeStroke::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		const size_t d_cnt = m_diff.size();	// �����̐�
		D2D1_POINT_2F e_pos = m_pos;
		b_min = a_min;
		b_max = a_max;
		pt_inc(e_pos, b_min, b_max);
		for (size_t i = 0; i < d_cnt; i++) {
			pt_add(e_pos, m_diff[i], e_pos);
			pt_inc(e_pos, b_min, b_max);
		}
	}

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	// value	�̈�̍���ʒu
	void ShapeStroke::get_min_pos(D2D1_POINT_2F& value) const noexcept
	{
		const size_t n = m_diff.size();	// �����̐�
		D2D1_POINT_2F v_pos = m_pos;	// ���_�̈ʒu
		value = m_pos;
		for (size_t i = 0; i < n; i++) {
			pt_add(v_pos, m_diff[i], v_pos);
			pt_min(value, v_pos, value);
		}

		//value.x = m_diff[0].x >= 0.0f ? m_pos.x : m_pos.x + m_diff[0].x;
		//value.y = m_diff[0].y >= 0.0f ? m_pos.y : m_pos.y + m_diff[0].y;
	}

	// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
	void ShapeStroke::get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept
	{
		if (anch == ANCH_TYPE::ANCH_SHEET || anch == ANCH_TYPE::ANCH_P0) {
			// �}�`�̕��ʂ��u�O���v�܂��́u�J�n�_�v�Ȃ��, �J�n�ʒu�𓾂�.
			value = m_pos;
		}
		else if (anch > ANCH_TYPE::ANCH_P0) {
			const size_t m = m_diff.size() + 1;		// ���_�̐� (�����̐� + 1)
			if (anch < ANCH_TYPE::ANCH_P0 + m) {
				value = m_pos;
				for (size_t i = 0; i < anch - ANCH_TYPE::ANCH_P0; i++) {
					pt_add(value, m_diff[i], value);
				}
			}
		}
		//value = m_pos;
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

	// ���g�̃}�C�^�[�����̔䗦�𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_join_limit(float& value) const noexcept
	{
		value = m_stroke_join_limit;
		return true;
	}

	// ���g�̘A���𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_join_style(D2D1_LINE_JOIN& value) const noexcept
	{
		value = m_stroke_join_style;
		return true;
	}

	// �j���̔z�u�𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_dash_patt(STROKE_DASH_PATT& value) const noexcept
	{
		value = m_stroke_dash_patt;
		return true;
	}

	// ���g�̌`���𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_dash_style(D2D1_DASH_STYLE& value) const noexcept
	{
		value = m_stroke_dash_style;
		return true;
	}

	// ���g�̑����𓾂�.
	// �߂�l	�˂� true
	bool ShapeStroke::get_stroke_width(float& value) const noexcept
	{
		value = m_stroke_width;
		return true;
	}

	// �ʒu���܂ނ����肷��.
	// �߂�l	�˂� ANCH_SHEET
	uint32_t ShapeStroke::hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept
	{
		return ANCH_TYPE::ANCH_SHEET;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// �߂�l	�˂� false
	bool ShapeStroke::in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept
	{
		return false;
	}

	// ���������ړ�����.
	void ShapeStroke::move(const D2D1_POINT_2F diff)
	{
		D2D1_POINT_2F s_pos;
		pt_add(m_pos, diff, s_pos);
		set_start_pos(s_pos);
	}

	// �n�_�ɒl���i�[����. ���̕��ʂ̈ʒu������.
	void ShapeStroke::set_start_pos(const D2D1_POINT_2F value)
	{
		D2D1_POINT_2F s_pos;
		pt_round(value, PT_ROUND, s_pos);
		m_pos = s_pos;
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
	void ShapeStroke::write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, const ARROWHEAD_STYLE a_style, DataWriter const& dt_writer) const
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
		if (a_style == ARROWHEAD_STYLE::FILLED) {
			write_svg(m_stroke_color, "fill", dt_writer);
		}
		else {
			write_svg("fill=\"none\" ", dt_writer);
		}
		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		write_svg(" />" SVG_NEW_LINE, dt_writer);
	}

	// �l���}�C�^�[�����̔䗦�Ɋi�[����.
	void ShapeStroke::set_stroke_join_limit(const float& value)
	{
		if (equal(m_stroke_join_limit, value)) {
			return;
		}
		m_stroke_join_limit = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �l����g�̘A���Ɋi�[����.
	void ShapeStroke::set_stroke_join_style(const D2D1_LINE_JOIN& value)
	{
		if (equal(m_stroke_join_style, value)) {
			return;
		}
		m_stroke_join_style = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �l��j���̔z�u�Ɋi�[����.
	void ShapeStroke::set_stroke_dash_patt(const STROKE_DASH_PATT& value)
	{
		if (equal(m_stroke_dash_patt, value)) {
			return;
		}
		m_stroke_dash_patt = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �l����g�̌`���Ɋi�[����.
	void ShapeStroke::set_stroke_dash_style(const D2D1_DASH_STYLE value)
	{
		if (m_stroke_dash_style == value) {
			return;
		}
		m_stroke_dash_style = value;
		m_d2d_stroke_dash_style = nullptr;
		create_stroke_dash_style(s_d2d_factory, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �l����g�̑����Ɋi�[����.
	void ShapeStroke::set_stroke_width(const float value) noexcept
	{
		m_stroke_width = value;
	}

	// �}�`���쐬����.
	// d_cnt	�����̌� (�ő�l�� N_GON_MAX - 1)
	// s_attr	�����l
	ShapeStroke::ShapeStroke(const size_t d_cnt, const ShapeSheet* s_attr) :
		m_diff(d_cnt <= N_GON_MAX - 1 ? d_cnt : N_GON_MAX - 1),
		m_stroke_color(s_attr->m_stroke_color),
		m_stroke_dash_patt(s_attr->m_stroke_dash_patt),
		m_stroke_dash_style(s_attr->m_stroke_dash_style),
		m_stroke_join_limit(s_attr->m_stroke_join_limit),
		m_stroke_join_style(s_attr->m_stroke_join_style),
		m_stroke_width(s_attr->m_stroke_width),
		m_d2d_stroke_dash_style(nullptr)
	{
		create_stroke_dash_style(s_d2d_factory, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���.
	ShapeStroke::ShapeStroke(DataReader const& dt_reader) :
		m_d2d_stroke_dash_style(nullptr)
	{
		using winrt::GraphPaper::implementation::read;

		set_delete(dt_reader.ReadBoolean());
		set_select(dt_reader.ReadBoolean());
		read(m_pos, dt_reader);
		read(m_diff, dt_reader);
		read(m_stroke_color, dt_reader);
		m_stroke_join_limit = dt_reader.ReadSingle();
		m_stroke_join_style = static_cast<D2D1_LINE_JOIN>(dt_reader.ReadUInt32());
		read(m_stroke_dash_patt, dt_reader);
		m_stroke_dash_style = static_cast<D2D1_DASH_STYLE>(dt_reader.ReadUInt32());
		m_stroke_width = dt_reader.ReadSingle();
		create_stroke_dash_style(s_d2d_factory, m_stroke_dash_style, m_stroke_dash_patt, m_stroke_join_style, m_stroke_join_limit, m_d2d_stroke_dash_style.put());
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
		dt_writer.WriteSingle(m_stroke_join_limit);
		dt_writer.WriteUInt32(m_stroke_join_style);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[0]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[1]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[2]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[3]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[4]);
		dt_writer.WriteSingle(m_stroke_dash_patt.m_[5]);
		dt_writer.WriteUInt32(static_cast<uint32_t>(m_stroke_dash_style));
		dt_writer.WriteSingle(m_stroke_width);
	}

	// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
	void ShapeStroke::write_svg(DataWriter const& dt_writer) const
	{
		using winrt::GraphPaper::implementation::write_svg;

		write_svg(m_stroke_color, "stroke", dt_writer);
		write_svg(m_stroke_dash_style, m_stroke_dash_patt, m_stroke_width, dt_writer);
		write_svg(m_stroke_width, "stroke-width", dt_writer);
		write_svg("stroke-linejoin=\"miter\" stroke-miterlimit=\"1\" ", dt_writer);
	}

}