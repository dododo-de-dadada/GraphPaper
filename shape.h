//------------------------------
// shape.h
// shape.cpp	�}�`�̂ЂȌ^, ���̑�
// shape_bezi.cpp	�x�W�F�Ȑ�
// shape_dx.cpp	�}�`�̕`���
// shape_elli.cpp	���~
// shape_group.cpp	�O���[�v�}�`
// shape_line.cpp	����
// shape_list.cpp	�}�`���X�g
// shape_path.cpp	�܂���̂ЂȌ^
// shape_poly.cpp	���p�`
// shape.rect.cpp	���`
// shape_rrect.cpp	�p�ە��`
// shape_ruler.cpp	��K
// shape_sheet.cpp	�p��
// shape_stroke.cpp	���g�̂ЂȌ^
// shape_text.cpp	������
//------------------------------
#pragma once
#include <list>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include "shape_dx.h"
// +-------------+
// | SHAPE_DX    |
// +-------------+
//
// +-------------+
// | Shape*      |
// +------+------+
//        |
//        +---------------+---------------+
//        |               |               |
// +------+------+ +------+------+ +------+------+
// | ShapeStroke*| | ShapeGroup  | | ShapeSheet  |
// +------+------+ +-------------+ +-------------+
//        |
//        +-------------------------------+
//        |                               |
// +------+------+                        |
// | ShapeLineA  |                        |
// +------+------+                        |
//        |                               |
// +------+------+                 +------+------+
// | ShapePath*  |                 | ShapeRect   |
// +------+------+                 +------+------+
//        |                               |
//        +---------------+               +---------------+---------------+---------------+
//        |               |               |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+
// | ShapePoly   | | ShapeBezi   | | ShapeElli   | | ShapeRRect  | | ShapeText   | | ShapeRuler  |
// +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+

// SVG �̂��߂̃e�L�X�g���s�R�[�h
#define SVG_NEW_LINE	"\n"

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::Point;
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::UI::Color;
	using winrt::Windows::UI::Xaml::Media::Brush;

#if defined(_DEBUG)
	extern uint32_t debug_leak_cnt;
	constexpr wchar_t DEBUG_MSG[] = L"Memory leak occurs";
#endif
	constexpr double PT_ROUND = 1.0 / 16.0;

	// �O���錾
	struct Shape;

	// �A���J�[ (�}�`�̕���) �̎��
	// �܂���̒��_������킷����, enum struct �łȂ� enum ��p����.
	enum ANCH_TYPE {
		ANCH_SHEET,		// �}�`�̊O�� (���J�[�\��)
		ANCH_FILL,		// �}�`�̓��� (�ړ��J�[�\��)
		ANCH_STROKE,	// ���g (�ړ��J�[�\��)
		ANCH_TEXT,		// ������ (�ړ��J�[�\��)
		ANCH_NW,		// ���`�̍���̒��_ (�k���쓌�J�[�\��)
		ANCH_SE,		// ���`�̉E���̒��_ (�k���쓌�J�[�\��)
		ANCH_NE,		// ���`�̉E��̒��_ (�k���쐼�J�[�\��)
		ANCH_SW,		// ���`�̍����̒��_ (�k���쐼�J�[�\��)
		ANCH_NORTH,		// ���`�̏�ӂ̒��_ (�㉺�J�[�\��)
		ANCH_SOUTH,		// ���`�̉��ӂ̒��_ (�㉺�J�[�\��)
		ANCH_EAST,		// ���`�̍��ӂ̒��_ (���E�J�[�\��)
		ANCH_WEST,		// ���`�̉E�ӂ̒��_ (���E�J�[�\��)
		ANCH_R_NW,		// ����̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANCH_R_NE,		// �E��̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANCH_R_SE,		// �E���̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANCH_R_SW,		// �����̊p�ۂ̒��S�_ (�\���J�[�\��)
		ANCH_P0,	// �p�X�̊J�n�_ (�\���J�[�\��)
	};

	// ���`�̒��_�̔z��
	constexpr uint32_t ANCH_MIDDLE[4]{
		ANCH_TYPE::ANCH_SOUTH,
		ANCH_TYPE::ANCH_EAST,
		ANCH_TYPE::ANCH_WEST,
		ANCH_TYPE::ANCH_NORTH
	};

	// ���`�̒��_�̔z��
	constexpr uint32_t ANCH_CORNER[4]{
		ANCH_TYPE::ANCH_SE,
		ANCH_TYPE::ANCH_NE,
		ANCH_TYPE::ANCH_SW,
		ANCH_TYPE::ANCH_NW
	};

	// ��邵�̌`��
	enum struct ARROW_STYLE {
		NONE,	// �Ȃ�
		OPENED,	// �J������邵
		FILLED	// ������邵
	};

	// ����̕\��
	enum struct GRID_SHOW {
		HIDE,	// �\���Ȃ�
		BACK,	// �Ŕw�ʂɕ\��
		FRONT	// �őO�ʂɕ\��
	};

	// ��邵�̐��@
	struct ARROW_SIZE {
		float m_width;		// �Ԃ��̕�
		float m_length;		// ��[����t�����܂ł̒���
		float m_offset;		// ��[�̃I�t�Z�b�g
	};

	// �����̒P�_
	// (SVG ��, �n�_�I�_�̋�ʂ��ł��Ȃ�)
	struct CAP_STYLE {
		D2D1_CAP_STYLE m_start;	// �n�_
		D2D1_CAP_STYLE m_end;	// �I�_
	};

	// ���p�`�̃c�[��
	struct TOOL_POLY {
		uint32_t m_vertex_cnt;	// ���p�`�̒��_�̐�
		bool m_regular;	// �����p�`�ō�}���邩����
		bool m_vertex_up;	// ���_����ɍ�}���邩����
		bool m_closed;	// �ӂ���č�}���邩����
		bool m_clockwise;	// ���_�����v���ɍ�}���邩���肷��.
	};

	// ����̋���
	struct GRID_EMPH {
		uint16_t m_gauge_1;	// ��������Ԋu (����1)
		uint16_t m_gauge_2;	// ��������Ԋu (����2)
	};
	constexpr GRID_EMPH GRID_EMPH_0{ 0, 0 };	// �����Ȃ�
	constexpr GRID_EMPH GRID_EMPH_2{ 2, 0 };	// 2 �Ԗڂ�����
	constexpr GRID_EMPH GRID_EMPH_3{ 2, 10 };	// 2 �Ԗڂ� 10 �Ԗڂ�����

	// �j���̔z�u
	union STROKE_DASH_PATT {
		float m_[6];
	};

	constexpr float COLOR_MAX = 255.0f;	// �F�����̍ő�l
	constexpr double PT_PER_INCH = 72.0;	// 1 �C���`������̃|�C���g��
	constexpr double MM_PER_INCH = 25.4;	// 1 �C���`������̃~�����[�g����
	constexpr float GRID_GRAY_DEF = 0.25f;	// ����̔Z��
	constexpr float MITER_LIMIT_DEF = 10.0f;	// �}�C�^�[�����̔䗦
	constexpr STROKE_DASH_PATT STROKE_DASH_PATT_DEF{ { 4.0F, 3.0F, 1.0F, 3.0F, 1.0F, 3.0F } };
	constexpr ARROW_SIZE ARROW_SIZE_DEF{ 7.0, 16.0, 0.0 };
	constexpr TOOL_POLY TOOL_POLY_DEF{ 3, true, true, true, true };
	constexpr float FONT_SIZE_DEF = static_cast<float>(12.0 * 96.0 / 72.0);
	constexpr D2D1_SIZE_F TEXT_MARGIN_DEF{ FONT_SIZE_DEF / 4.0, FONT_SIZE_DEF / 4.0 };
	constexpr float GRID_LEN_DEF = 48.0f;
	constexpr size_t N_GON_MAX = 256;	// ���p�`�̒��_�̍ő吔 (�q�b�g����ŃX�^�b�N�𗘗p���邽��, �I�[�o�[�t���[���Ȃ��悤��������)

	//------------------------------
	// shape.cpp
	//------------------------------

	// �P���x�������������������肷��.
	inline bool equal(const float a, const float b) noexcept;
	// �{���x�������������������肷��.
	inline bool equal(const double a, const double b) noexcept;
	// �}�`�̕��� (���`) ��\������.
	void anchor_draw_rect(const D2D1_POINT_2F a_pos, SHAPE_DX& dx);
	// �}�`�̕��ʁi�~�`�j��\������.
	void anchor_draw_ellipse(const D2D1_POINT_2F c_pos, SHAPE_DX& dx);
	// ���l�����肷��.
	template<typename T> inline bool equal(const T a, const T b) noexcept { return a == b; };
	// ��邵�̐��@�����������肷��.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept;
	// ���̒P�_�����������肷��.
	inline bool equal(const CAP_STYLE& a, const CAP_STYLE& b) noexcept;
	// �F�����������肷��.
	inline bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept;
	// �ʒu�����������肷��.
	inline bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept;
	// ���@�����������肷��.
	inline bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept;
	// �����͈͂����������肷��.
	inline bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept;
	// ����̋��������������肷��.
	inline bool equal(const GRID_EMPH& a, const GRID_EMPH& b) noexcept;
	// �j���̔z�u�����������肷��.
	inline bool equal(const STROKE_DASH_PATT& a, const STROKE_DASH_PATT& b) noexcept;
	// ���C�h�����񂪓��������肷��.
	inline bool equal(const wchar_t* a, const wchar_t* b) noexcept;
	// winrt �����񂪓��������肷��.
	inline bool equal(winrt::hstring const& a, const wchar_t* b) noexcept;
	// �F�̐��������������肷��.
	inline bool equal_color_comp(const FLOAT a, const FLOAT b) noexcept;
	// ��邵�̕Ԃ��̈ʒu�����߂�.
	void get_arrow_barbs(const D2D1_POINT_2F a_vec, const double a_len, const double h_width, const double h_len, D2D1_POINT_2F barbs[]) noexcept;
	// �F���s���������肷��.
	inline bool is_opaque(const D2D1_COLOR_F& color) noexcept;
	// �x�N�g���̒��� (�̎���l) �𓾂�
	inline double pt_abs2(const D2D1_POINT_2F a) noexcept;
	// �ʒu�Ɉʒu�𑫂�
	inline void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// �ʒu�ɃX�J���[�l�𑫂�
	inline void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// �ʒu��X����Y���̒l�𑫂�
	inline void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& c) noexcept;
	// ��_�̒��_�𓾂�.
	inline void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// ��_�ň͂܂ꂽ���`�𓾂�.
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// �ʒu���}�`�̕��ʂɊ܂܂�邩���肷��.
	inline bool pt_in_anch(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos) noexcept;
	// �ʒu�����~�Ɋ܂܂�邩���肷��.
	bool pt_in_elli(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept;
	// �ʒu���~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_circle(const D2D1_POINT_2F t_vec, const double rad) noexcept;
	// �ʒu���~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_circle(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad) noexcept;
	// �������ʒu���܂ނ�, �������l�����Ĕ��肷��.
	bool pt_in_line(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width, const CAP_STYLE& s_cap) noexcept;
	// ���p�`���ʒu���܂ނ����肷��.
	bool pt_in_poly(const D2D1_POINT_2F t_pos, const size_t p_cnt, const D2D1_POINT_2F p_pos[]) noexcept;
	// ���`���ʒu���܂ނ����肷��.
	bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept;
	// �w�肵���ʒu���܂ނ悤, ���`���g�傷��.
	void pt_inc(const D2D1_POINT_2F a, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept;
	// ��_�̂��ꂼ��傫���l�����ʒu�𓾂�.
	inline void pt_max(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min) noexcept;
	// �ʒu�̕������t�ɂ���.
	inline void pt_neg(const D2D1_POINT_2F a, D2D1_POINT_2F& b) noexcept;
	// ��_�̂��ꂼ�ꏬ�����l�����ʒu�𓾂�.
	inline void pt_min(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min) noexcept;
	// �ʒu���X�J���[�{�Ɋۂ߂�.
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& round) noexcept;
	// �ʒu�ɃX�J���[�l���|��, �ʂ̈ʒu�𑫂�.
	inline void pt_mul(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// �ʒu�ɃX�J���[�l���|����.
	inline void pt_mul(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// ���@�ɒl���|����.
	inline void pt_mul(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept;
	// �_�ɃX�J���[�l���|��, �ʂ̈ʒu�𑫂�.
	inline void pt_mul(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// �ʒu����ʒu������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& sub) noexcept;
	// �ʒu����傫��������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& sub) noexcept;
	// ��邵�̐��@��ǂݍ���.
	void dt_read(ARROW_SIZE& value, DataReader const& dt_reader);
	// �����̒[�_��ǂݍ���.
	void dt_read(CAP_STYLE& value, DataReader const& dt_reader);
	// �F���f�[�^���[�_�[����ǂݍ���.
	void dt_read(D2D1_COLOR_F& value, DataReader const& dt_reader);
	// �ʒu���f�[�^���[�_�[����ǂݍ���.
	void dt_read(D2D1_POINT_2F& value, DataReader const& dt_reader);
	// ���@���f�[�^���[�_�[����ǂݍ���.
	void dt_read(D2D1_SIZE_F& value, DataReader const& dt_reader);
	// �����͈͂��f�[�^���[�_�[����ǂݍ���.
	void dt_read(DWRITE_TEXT_RANGE& value, DataReader const& dt_reader);
	// ����̌`�����f�[�^���[�_�[����ǂݍ���.
	void dt_read(GRID_EMPH& value, DataReader const& dt_reader);
	// �j���̔z�u���f�[�^���[�_�[����ǂݍ���.
	void dt_read(STROKE_DASH_PATT& value, DataReader const& dt_reader);
	// ���p�`�̃c�[�����f�[�^���[�_�[����ǂݍ���.
	void dt_read(TOOL_POLY& value, DataReader const& dt_reader);
	// ��������f�[�^���[�_�[����ǂݍ���.
	void dt_read(wchar_t*& value, DataReader const& dt_reader);
	// �ʒu�z����f�[�^���[�_�[����ǂݍ���.
	void dt_read(std::vector<D2D1_POINT_2F>& value, DataReader const& dt_reader);
	// ������𕡐�����. ���̕����񂪃k���|�C���^�[, �܂��͌��̕������� 0 �̂Ƃ���, �k���|�C���^�[��Ԃ�.
	wchar_t* wchar_cpy(const wchar_t* const s);
	// ������𕡐�����. �G�X�P�[�v������͕����R�[�h�ɕϊ�����.
	wchar_t* wchar_cpy_esc(const wchar_t* const s);
	// ������̒���. �������k���|�C���^�̏ꍇ, 0 ��Ԃ�.
	uint32_t wchar_len(const wchar_t* const t) noexcept;
	// ��邵�̐��@���f�[�^���C�^�[�ɏ�������.
	void dt_write(const ARROW_SIZE& value, DataWriter const& dt_writer);
	// �����̒[�_���f�[�^���C�^�[�ɏ�������.
	void dt_write(const CAP_STYLE& value, DataWriter const& dt_writer);
	// �F���f�[�^���C�^�[�ɏ�������.
	void dt_write(const D2D1_COLOR_F& value, DataWriter const& dt_writer);
	// �ʒu���f�[�^���C�^�[�ɏ�������.
	void dt_write(const D2D1_POINT_2F value, DataWriter const& dt_writer);
	// ���@���f�[�^���C�^�[�ɏ�������.
	void dt_write(const D2D1_SIZE_F value, DataWriter const& dt_writer);
	// ������͈͂��f�[�^���C�^�[�ɏ�������.
	void dt_write(const DWRITE_TEXT_RANGE value, DataWriter const& dt_writer);
	// ����̌`�����f�[�^���C�^�[�ɏ�������.
	void dt_write(const GRID_EMPH value, DataWriter const& dt_writer);
	// �j���̔z�u���f�[�^���C�^�[�ɏ�������.
	void dt_write(const STROKE_DASH_PATT& value, DataWriter const& dt_writer);
	// ���p�`�̃c�[�����f�[�^���C�^�[�ɏ�������.
	void dt_write(const TOOL_POLY& value, DataWriter const& dt_writer);
	// ��������f�[�^���C�^�[�ɏ�������.
	void dt_write(const wchar_t* value, DataWriter const& dt_writer);
	// �ʒu�z����f�[�^���[�_�[�ɏ�������.
	void dt_write(const std::vector<D2D1_POINT_2F>& value, DataWriter const& dt_writer);

	//------------------------------
	// shape_svg.cpp
	// SVG �t�@�C���ւ̏o��
	//------------------------------

	// �V���O���o�C�g��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const char* value, DataWriter const& dt_writer);
	// �}���`�o�C�g��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const wchar_t* value, const uint32_t v_len, DataWriter const& dt_writer);
	// �������ƃV���O���o�C�g��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const char* value, const char* name, DataWriter const& dt_writer);
	// ���߂ƈʒu���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_POINT_2F value, const char* cmd, DataWriter const& dt_writer);
	// �������ƈʒu���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_POINT_2F value, const char* name_x, const char* name_y, DataWriter const& dt_writer);
	// �������ƐF���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_COLOR_F value, const char* name, DataWriter const& dt_writer);
	// �F���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_COLOR_F value, DataWriter const& dt_writer);
	// �����������f�[�^���C�^�[�ɏ�������
	void write_svg(const float value, DataWriter const& dt_writer);
	// �������ƕ��������l���f�[�^���C�^�[�� SVG �Ƃ��ď�������
	void write_svg(const double value, const char* name, DataWriter const& dt_writer);
	// �������� 32 �r�b�g���������f�[�^���C�^�[�� SVG �Ƃ��ď�������
	void write_svg(const uint32_t value, const char* name, DataWriter const& dt_writer);
	// �j���̌`���Ɣz�u���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_DASH_STYLE style, const STROKE_DASH_PATT& patt, const double width, DataWriter const& dt_writer);

	//------------------------------
	// shape_list.cpp
	// �}�`���X�g�Ɋ֘A��������
	//------------------------------

	using SHAPE_LIST = std::list<struct Shape*>;

	// ���p�\�ȏ��̖������肵, ���p�ł��Ȃ����̂��������Ȃ�΂���𓾂�.
	bool slist_test_font(const SHAPE_LIST& slist, wchar_t*& unavailable_font) noexcept;

	// �Ō�̐}�`�𓾂�.
	Shape* slist_back(SHAPE_LIST const& slist) noexcept;

	// �}�`���X�g��������, �܂܂��}�`��j������.
	void slist_clear(SHAPE_LIST& slist) noexcept;

	// �}�`����ޕʂɐ�����.
	void slist_count(const SHAPE_LIST& slist, uint32_t& undeleted_cnt, uint32_t& selected_cnt, uint32_t& selected_group_cnt, uint32_t& runlength_cnt, uint32_t& selected_text_cnt, uint32_t& text_cnt, bool& fore_selected, bool& back_selected, bool& prev_selected) noexcept;

	// �擪����}�`�܂Ő�����.
	uint32_t slist_count(SHAPE_LIST const& slist, const Shape* s) noexcept;

	// �ŏ��̐}�`�����X�g���瓾��.
	Shape* slist_front(SHAPE_LIST const& slist) noexcept;

	// �}�`�Ɨp�����͂ޗ̈�𓾂�.
	void slist_bound(SHAPE_LIST const& slist, const D2D1_SIZE_F sheet_size, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;

	// �}�`���͂ޗ̈�����X�g���瓾��.
	void slist_bound(SHAPE_LIST const& slist, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;

	// �ʒu���܂ސ}�`�Ƃ��̕��ʂ𓾂�.
	uint32_t slist_hit_test(SHAPE_LIST const& slist, const D2D1_POINT_2F t_pos, Shape*& s) noexcept;

	// �}�`��}������.
	void slist_insert(SHAPE_LIST& slist, Shape* const s_ins, const Shape* s_at) noexcept;

	// �I�����ꂽ�}�`�����������ړ�����.
	bool slist_move(SHAPE_LIST const& slist, const D2D1_POINT_2F diff) noexcept;

	// �}�`�̂��̎��̐}�`�𓾂�.
	Shape* slist_next(SHAPE_LIST const& slist, const Shape* s) noexcept;

	// �}�`�̂��̑O�̐}�`�𓾂�.
	Shape* slist_prev(SHAPE_LIST const& slist, const Shape* s) noexcept;

	// �}�`���X�g���f�[�^���[�_�[����ǂݍ���.
	bool slist_read(SHAPE_LIST& slist, DataReader const& dt_reader);

	// �}�`�����X�g����폜��, �폜�����}�`�̎��̐}�`�𓾂�.
	Shape* slist_remove(SHAPE_LIST& slist, const Shape* s) noexcept;

	// �I�����ꂽ�}�`�̃��X�g�𓾂�.
	template <typename S> void slist_selected(SHAPE_LIST const& slist, SHAPE_LIST& t_list) noexcept;

	// �}�`���X�g���f�[�^���C�^�[�ɏ�������. REDUCE �ꍇ�̏����t���O�̗��}�`�͖�������.
	template <bool REDUCE> void slist_write(const SHAPE_LIST& slist, DataWriter const& dt_writer);

	// ���X�g�̒��̐}�`�̏��Ԃ𓾂�.
	template <typename S, typename T> bool slist_match(SHAPE_LIST const& slist, S s, T& t);

	// �I�����ꂽ������}�`����, ���������s�ŘA������������𓾂�.
	winrt::hstring slist_selected_all_text(SHAPE_LIST const& slist) noexcept;


	//------------------------------
	// �}�`�̂ЂȌ^
	//------------------------------
	struct Shape {
		// D2D �t�@�N�g���̃L���b�V��
		static ID2D1Factory3* s_d2d_factory;
		// DWRITE �t�@�N�g���̃L���b�V��
		static IDWriteFactory3* s_dwrite_factory;
		// �}�`�̕��ʂ̐F
		static D2D1_COLOR_F s_anch_color;
		// �}�`�̕��ʂ̑傫��
		static float s_anch_len;
		//static winrt::com_ptr<ID2D1StrokeStyle1> m_aux_style;	// �⏕���̌`��
		static D2D1_COLOR_F m_range_background;	// �����͈͂̔w�i�F
		static D2D1_COLOR_F m_range_foreground;	// �����͈͂̕����F
		static D2D1_COLOR_F m_theme_background;	// �O�i�F
		static D2D1_COLOR_F m_theme_foreground;	// �w�i�F
		//static winrt::com_ptr<ID2D1SolidColorBrush> m_range_brush;	// �����͈͂̕����F�u���V
		//static winrt::com_ptr<ID2D1SolidColorBrush> m_shape_brush;	// �}�`�̐F�u���V
		//static winrt::com_ptr<ID2D1DrawingStateBlock1> m_state_block;	// �`���Ԃ̕ۑ��u���b�N


		// �}�`��j������.
		virtual ~Shape(void) {}
		// �}�`��\������
		virtual void draw(SHAPE_DX& /*dx*/) {}
		// ���ʂ̈ʒu�𓾂�.
		virtual	void get_anch_pos(const uint32_t /*anch*/, D2D1_POINT_2F&/*value*/) const noexcept {}
		// ��邵�̐��@�𓾂�
		virtual bool get_arrow_size(ARROW_SIZE& /*value*/) const noexcept { return false; }
		// ��邵�̌`���𓾂�.
		virtual bool get_arrow_style(ARROW_STYLE& /*value*/) const noexcept { return false; }
		// �}�`���͂ޗ̈�𓾂�.
		virtual void get_bound(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/, D2D1_POINT_2F& /*b_min*/, D2D1_POINT_2F& /*b_max*/) const noexcept {}
		// �p�۔��a�𓾂�.
		virtual bool get_corner_radius(D2D1_POINT_2F& /*value*/) const noexcept { return false; }
		// �h��Ԃ��F�𓾂�.
		virtual bool get_fill_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// ���̂̐F�𓾂�.
		virtual bool get_font_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// ���̖��𓾂�.
		virtual bool get_font_family(wchar_t*& /*value*/) const noexcept { return false; }
		// ���̂̑傫���𓾂�.
		virtual bool get_font_size(float& /*value*/) const noexcept { return false; }
		// ���̂̉����𓾂�.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH& /*value*/) const noexcept { return false; }
		// ���̂̎��̂𓾂�.
		virtual bool get_font_style(DWRITE_FONT_STYLE& /*value*/) const noexcept { return false; }
		// ���̂̑����𓾂�.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT& /*value*/) const noexcept { return false; }
		// ����̊�̑傫���𓾂�.
		virtual bool get_grid_base(float& /*value*/) const noexcept { return false; }
		// ����̑傫���𓾂�.
		virtual bool get_grid_gray(float& /*value*/) const noexcept { return false; }
		// ����������𓾂�.
		virtual bool get_grid_emph(GRID_EMPH& /*value*/) const noexcept { return false; }
		// ����̕\���𓾂�.
		virtual bool get_grid_show(GRID_SHOW& /*value*/) const noexcept { return false; }
		// ����ɂ��낦��𓾂�.
		virtual bool get_grid_snap(bool& /*value*/) const noexcept { return false; }
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		virtual void get_min_pos(D2D1_POINT_2F& /*value*/) const noexcept {}
		// �p���̐F�𓾂�.
		virtual bool get_sheet_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// �p���̊g�嗦�𓾂�.
		virtual bool get_sheet_scale(float& /*value*/) const noexcept { return false; }
		// �p���̑傫���𓾂�.
		virtual bool get_sheet_size(D2D1_SIZE_F& /*value*/) const noexcept { return false; }
		// �J�n�ʒu�𓾂�.
		virtual bool get_start_pos(D2D1_POINT_2F& /*value*/) const noexcept { return false; }
		// ���̒[�_�𓾂�.
		virtual bool get_stroke_cap_style(CAP_STYLE& /*value*/) const noexcept { return false; }
		// ���g�̐F�𓾂�.
		virtual bool get_stroke_color(D2D1_COLOR_F& /*value*/) const noexcept { return false; }
		// �j���̒[�_�𓾂�.
		virtual bool get_stroke_dash_cap(D2D1_CAP_STYLE& /*value*/) const noexcept { return false; }
		// �j���̔z�u�𓾂�.
		virtual bool get_stroke_dash_patt(STROKE_DASH_PATT& /*value*/) const noexcept { return false; }
		// �j���̌`���𓾂�.
		virtual bool get_stroke_dash_style(D2D1_DASH_STYLE& /*value*/) const noexcept { return false; }
		// ���̃}�C�^�[�����̔䗦�𓾂�.
		virtual bool get_stroke_join_limit(float& /*value*/) const noexcept { return false; }
		// ���̂Ȃ���𓾂�.
		virtual bool get_stroke_join_style(D2D1_LINE_JOIN& /*value*/) const noexcept { return false; }
		// ���̂̑����𓾂�
		virtual bool get_stroke_width(float& /*value*/) const noexcept { return false; }
		// ������𓾂�.
		virtual bool get_text(wchar_t*& /*value*/) const noexcept { return false; }
		// �i���̂��낦�𓾂�.
		virtual bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& /*value*/) const noexcept { return false; }
		// ������̂��낦�𓾂�.
		virtual bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& /*value*/) const noexcept { return false; }
		// �s�̍����𓾂�.
		virtual bool get_text_line(float& /*value*/) const noexcept { return false; }
		// ������̎��̗͂]���𓾂�.
		virtual bool get_text_margin(D2D1_SIZE_F& /*value*/) const noexcept { return false; }
		// �����͈͂𓾂�
		virtual bool get_text_range(DWRITE_TEXT_RANGE& /*value*/) const noexcept { return false; }
		// �ʒu���܂ނ����肷��.
		virtual uint32_t hit_test(const D2D1_POINT_2F /*t_pos*/) const noexcept { return ANCH_TYPE::ANCH_SHEET; }
		// �͈͂Ɋ܂܂�邩���肷��.
		virtual bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept { return false; }
		// �����t���O�𔻒肷��.
		virtual bool is_deleted(void) const noexcept { return false; }
		// �I���t���O�𔻒肷��.
		virtual bool is_selected(void) const noexcept { return false; }
		// ���������ړ�����.
		virtual	bool move(const D2D1_POINT_2F /*value*/) { return false; }
		// �l���邵�̐��@�Ɋi�[����.
		virtual bool set_arrow_size(const ARROW_SIZE& /*value*/) { return false; }
		// �l���邵�̌`���Ɋi�[����.
		virtual bool set_arrow_style(const ARROW_STYLE /*value*/) { return false; }
		// �l���p�۔��a�Ɋi�[����.
		virtual bool set_corner_radius(const D2D1_POINT_2F& /*alue*/) noexcept { return false; }
		// �l�������t���O�Ɋi�[����.
		virtual bool set_delete(const bool /*value*/) noexcept { return false; }
		// �l��h��Ԃ��F�Ɋi�[����.
		virtual bool set_fill_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// �l�����̂̐F�Ɋi�[����.
		virtual bool set_font_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// �l�����̖��Ɋi�[����.
		virtual bool set_font_family(wchar_t* const /*value*/) { return false; }
		// �l�����̂̑傫���Ɋi�[����.
		virtual bool set_font_size(const float /*value*/) { return false; }
		// �l�����̂̉����Ɋi�[����.
		virtual bool set_font_stretch(const DWRITE_FONT_STRETCH /*value*/) { return false; }
		// �l�����̂̎��̂Ɋi�[����.
		virtual bool set_font_style(const DWRITE_FONT_STYLE /*value*/) { return false; }
		// �l�����̂̑����Ɋi�[����.
		virtual bool set_font_weight(const DWRITE_FONT_WEIGHT /*value*/) { return false; }
		// �l�����̑傫���Ɋi�[����.
		virtual bool set_grid_base(const float /*value*/) noexcept { return false; }
		// �l�����̔Z�W�Ɋi�[����.
		virtual bool set_grid_gray(const float /*value*/) noexcept { return false; }
		// �l�����̋����Ɋi�[����.
		virtual bool set_grid_emph(const GRID_EMPH& /*value*/) noexcept { return false; }
		// �l�����̕\���Ɋi�[����.
		virtual bool set_grid_show(const GRID_SHOW /*value*/) noexcept { return false; }
		// �l�����ɂ��낦��Ɋi�[����.
		virtual bool set_grid_snap(const bool /*value*/) noexcept { return false; }
		// �l��p���̐F�Ɋi�[����.
		virtual bool set_sheet_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// �l��p���̊g�嗦�Ɋi�[����.
		virtual bool set_sheet_scale(const float /*value*/) noexcept { return false; }
		// �l��p���̑傫���Ɋi�[����.
		virtual bool set_sheet_size(const D2D1_SIZE_F /*value*/) noexcept { return false; }
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		virtual bool set_anchor_pos(const D2D1_POINT_2F /*value*/, const uint32_t /*anch*/) { return false; }
		// �l��I���t���O�Ɋi�[����.
		virtual bool set_select(const bool /*value*/) noexcept { return false; }
		// �l����̒[�_�Ɋi�[����.
		virtual bool set_stroke_cap_style(const CAP_STYLE& /*value*/) { return false; }
		// �l����g�̐F�Ɋi�[����.
		virtual bool set_stroke_color(const D2D1_COLOR_F& /*value*/) noexcept { return false; }
		// �l��j���̒[�_�Ɋi�[����.
		virtual bool set_stroke_dash_cap(const D2D1_CAP_STYLE& /*value*/) { return false; }
		// �l��j���̔z�u�Ɋi�[����.
		virtual bool set_stroke_dash_patt(const STROKE_DASH_PATT& /*value*/) { return false; }
		// �l����g�̌`���Ɋi�[����.
		virtual bool set_stroke_dash_style(const D2D1_DASH_STYLE /*value*/) { return false; }
		// �l���}�C�^�[�����̔䗦�Ɋi�[����.
		virtual bool set_stroke_join_limit(const float& /*value*/) { return false; }
		// �l����̂Ȃ���Ɋi�[����.
		virtual bool set_stroke_join_style(const D2D1_LINE_JOIN& /*value*/) { return false; }
		// �l�����̂̑����Ɋi�[����.
		virtual bool set_stroke_width(const float /*value*/) noexcept { return false; }
		// �l�𕶎���Ɋi�[����.
		virtual bool set_text(wchar_t* const /*value*/) { return false; }
		// �l��i���̂��낦�Ɋi�[����.
		virtual bool set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT /*value*/) { return false; }
		// �l�𕶎���̂��낦�Ɋi�[����.
		virtual bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT /*value*/) { return false; }
		// �l���s�ԂɊi�[����.
		virtual bool set_text_line(const float /*value*/) { return false; }
		// �l�𕶎���̗]���Ɋi�[����.
		virtual bool set_text_margin(const D2D1_SIZE_F /*value*/) { return false; }
		// �l�𕶎��͈͂Ɋi�[����.
		virtual bool set_text_range(const DWRITE_TEXT_RANGE /*value*/) { return false; }
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual bool set_start_pos(const D2D1_POINT_2F /*value*/) { return false; }
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void write_svg(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// �p��
	//------------------------------
	struct ShapeSheet : Shape {

		// ����̐}�`����
		ARROW_SIZE m_arrow_size{ ARROW_SIZE_DEF };	// ��邵�̐��@
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// ��邵�̌`��
		D2D1_POINT_2F m_corner_rad{ GRID_LEN_DEF, GRID_LEN_DEF };	// �p�۔��a
		D2D1_COLOR_F m_fill_color{ S_WHITE };	// �h��Ԃ��̐F
		D2D1_COLOR_F m_font_color{ S_BLACK };	// ���̂̐F (MainPage �̃R���X�g���N�^�Őݒ�)
		wchar_t* m_font_family = nullptr;	// ���̖�
		float m_font_size = FONT_SIZE_DEF;	// ���̂̑傫��
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// ���̂̐L�k
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// ���̂̎���
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// ���̂̑���
		CAP_STYLE m_stroke_cap_style{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };	// �����̒[�_
		D2D1_COLOR_F m_stroke_color{ S_BLACK };	// ���g�̐F (MainPage �̃R���X�g���N�^�Őݒ�)
		D2D1_CAP_STYLE m_stroke_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// �j���̒[�_
		STROKE_DASH_PATT m_stroke_dash_patt{ STROKE_DASH_PATT_DEF };	// �j���̔z�u
		D2D1_DASH_STYLE m_stroke_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// �j���̌`��
		float m_stroke_join_limit = MITER_LIMIT_DEF;	// ���̂Ȃ���̃}�C�^�[�����̔䗦
		D2D1_LINE_JOIN m_stroke_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_MITER;	// ���g�̂Ȃ���
		float m_stroke_width = 1.0;	// ���g�̑���

		float m_text_line_h = 0.0f;	// �s�� (DIPs 96dpi�Œ�)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// �i���̑���
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// ������̑���
		D2D1_SIZE_F m_text_margin{ TEXT_MARGIN_DEF };	// ������̍��E�Ə㉺�̗]��

		// ����̑���
		float m_grid_gray = GRID_GRAY_DEF;	// ����̔Z��
		float m_grid_base = GRID_LEN_DEF - 1.0f;	// ����̊�̑傫�� (�� -1 �����l)
		GRID_SHOW m_grid_show = GRID_SHOW::BACK;	// ����̕\��
		GRID_EMPH m_grid_emph{ GRID_EMPH_0 };	// ����̋���
		bool m_grid_snap = true;	// ����ɐ���

		// �p���̑���
		D2D1_COLOR_F m_sheet_color{ S_WHITE };	// �w�i�F (MainPage �̃R���X�g���N�^�Őݒ�)
		float m_sheet_scale = 1.0f;	// �g�嗦
		D2D1_SIZE_F	m_sheet_size{ 0.0f, 0.0f };	// �傫�� (MainPage �̃R���X�g���N�^�Őݒ�)

		//------------------------------
		// shape_sheet.cpp
		//------------------------------

		// �Ȑ��̕⏕����\������.
		void draw_auxiliary_bezi(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// ���~�̕⏕����\������.
		void draw_auxiliary_elli(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// �����̕⏕����\������.
		void draw_auxiliary_line(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// ���`�̕⏕����\������.
		void draw_auxiliary_rect(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// ���p�`�̕⏕����\������.
		void draw_auxiliary_poly(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos, const TOOL_POLY& t_poly);
		// �p�ە��`�̕⏕����\������.
		void draw_auxiliary_rrect(SHAPE_DX const& dx, const D2D1_POINT_2F b_pos, const D2D1_POINT_2F c_pos);
		// �����\������,
		void draw_grid(SHAPE_DX const& dx, const D2D1_POINT_2F offset);
		// ��邵�̐��@�𓾂�.
		bool get_arrow_size(ARROW_SIZE& value) const noexcept;
		// ��邵�̌`���𓾂�.
		bool get_arrow_style(ARROW_STYLE& value) const noexcept;
		// ����̊�̑傫���𓾂�.
		bool get_grid_base(float& value) const noexcept;
		// ����̐F�𓾂�.
		void get_grid_color(D2D1_COLOR_F& value) const noexcept;
		// ����̑傫���𓾂�.
		bool get_grid_gray(float& value) const noexcept;
		// ����̋����𓾂�.
		bool get_grid_emph(GRID_EMPH& value) const noexcept;
		// ����̕\���̏�Ԃ𓾂�.
		bool get_grid_show(GRID_SHOW& value) const noexcept;
		// ����ɂ��낦��𓾂�.
		bool get_grid_snap(bool& value) const noexcept;
		// �p���̐F�𓾂�.
		bool get_sheet_color(D2D1_COLOR_F& value) const noexcept;
		// �p���̐F�𓾂�.
		bool get_sheet_size(D2D1_SIZE_F& value) const noexcept;
		// �p���̊g�嗦�𓾂�.
		bool get_sheet_scale(float& value) const noexcept;
		// �p�۔��a�𓾂�.
		bool get_corner_radius(D2D1_POINT_2F& value) const noexcept;
		// �h��Ԃ��F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// ���̂̐F�𓾂�.
		bool get_font_color(D2D1_COLOR_F& value) const noexcept;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& value) const noexcept;
		// ���̂̑傫���𓾂�.
		bool get_font_size(float& value) const noexcept;
		// ���̂̉����𓾂�.
		bool get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept;
		// ���̂̎��̂𓾂�.
		bool get_font_style(DWRITE_FONT_STYLE& value) const noexcept;
		// ���̂̑����𓾂�.
		bool get_font_weight(DWRITE_FONT_WEIGHT& value) const noexcept;
		// �s�Ԃ𓾂�.
		bool get_text_line(float& value) const noexcept;
		// ������̎��̗͂]���𓾂�.
		bool get_text_margin(D2D1_SIZE_F& value) const noexcept;
		// �i���̂��낦�𓾂�.
		bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& value) const noexcept;
		// ���̒[�_�𓾂�.
		bool get_stroke_cap_style(CAP_STYLE& value) const noexcept;
		// ���g�̐F�𓾂�.
		bool get_stroke_color(D2D1_COLOR_F& value) const noexcept;
		// ���̒[�_�𓾂�.
		bool get_stroke_dash_cap(D2D1_CAP_STYLE& value) const noexcept;
		// �j���̔z�u�𓾂�.
		bool get_stroke_dash_patt(STROKE_DASH_PATT& value) const noexcept;
		// �j���̌`���𓾂�.
		bool get_stroke_dash_style(D2D1_DASH_STYLE& value) const noexcept;
		// ���̃}�C�^�[�����̔䗦�𓾂�.
		bool get_stroke_join_limit(float& value) const noexcept;
		// ���̂Ȃ���𓾂�.
		bool get_stroke_join_style(D2D1_LINE_JOIN& value) const noexcept;
		// ���̂̑����𓾂�
		bool get_stroke_width(float& value) const noexcept;
		// ������̂��낦�𓾂�.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& value) const noexcept;
		// �f�[�^���[�_�[����ǂݍ���.
		void read(DataReader const& dt_reader);
		// �}�`�̑����l���i�[����.
		void set_attr_to(Shape* s) noexcept;
		// �l���p�۔��a�Ɋi�[����.
		bool set_corner_radius(const D2D1_POINT_2F& value) noexcept;
		// �l�����̊�̑傫���Ɋi�[����.
		bool set_grid_base(const float value) noexcept;
		// �l�����̔Z�W�Ɋi�[����.
		bool set_grid_gray(const float value) noexcept;
		// �l�����̋����Ɋi�[����.
		bool set_grid_emph(const GRID_EMPH& value) noexcept;
		// �l�����̕\���Ɋi�[����.
		bool set_grid_show(const GRID_SHOW value) noexcept;
		// �l�����ɂ��낦��Ɋi�[����.
		bool set_grid_snap(const bool value) noexcept;
		// �l��p���̐F�Ɋi�[����.
		bool set_sheet_color(const D2D1_COLOR_F& value) noexcept;
		// �l��p���̊g�嗦�Ɋi�[����.
		bool set_sheet_scale(const float value) noexcept;
		// �l��p���̐��@�Ɋi�[����.
		bool set_sheet_size(const D2D1_SIZE_F value) noexcept;
		// �l���邵�̐��@�Ɋi�[����.
		bool set_arrow_size(const ARROW_SIZE& value);
		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE value);
		// �l��h��Ԃ��F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// �l�����̂̐F�Ɋi�[����.
		bool set_font_color(const D2D1_COLOR_F& value) noexcept;
		// ���̖��Ɋi�[����.
		bool set_font_family(wchar_t* const value);
		// ���̂̑傫���Ɋi�[����.
		bool set_font_size(const float value);
		// �l�����̂̐L�k�Ɋi�[����.
		bool set_font_stretch(const DWRITE_FONT_STRETCH value);
		// �l�����̂̎��̂Ɋi�[����.
		bool set_font_style(const DWRITE_FONT_STYLE value);
		// �l�����̂̑����Ɋi�[����.
		bool set_font_weight(const DWRITE_FONT_WEIGHT value);
		// �l���s�ԂɊi�[����.
		bool set_text_line(const float value);
		// �l�𕶎���̗]���Ɋi�[����.
		bool set_text_margin(const D2D1_SIZE_F value);
		// �l��i���̂��낦�Ɋi�[����.
		bool set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value);
		// �l����̒[�_�Ɋi�[����.
		bool set_stroke_cap_style(const CAP_STYLE& value);
		// �l����g�̐F�Ɋi�[����.
		bool set_stroke_color(const D2D1_COLOR_F& value) noexcept;
		// �l��j���̒[�_�Ɋi�[����.
		bool set_stroke_dash_cap(const D2D1_CAP_STYLE& value);
		// �l��j���̔z�u�Ɋi�[����.
		bool set_stroke_dash_patt(const STROKE_DASH_PATT& value);
		// �l����g�̌`���Ɋi�[����.
		bool set_stroke_dash_style(const D2D1_DASH_STYLE value);
		// �l���}�C�^�[�����̔䗦�Ɋi�[����.
		bool set_stroke_join_limit(const float& value);
		// �l����̂Ȃ���Ɋi�[����.
		bool set_stroke_join_style(const D2D1_LINE_JOIN& value);
		// �l�����̂̑����Ɋi�[����.
		bool set_stroke_width(const float value) noexcept;
		// �l�𕶎���̂��낦�Ɋi�[����.
		bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT value);
		// �f�[�^���[�_�[�ɏ�������.
		void write(DataWriter const& dt_writer);
	};

	//------------------------------
	// �O���[�v�}�`
	//------------------------------
	struct ShapeGroup : Shape {
		SHAPE_LIST m_list_grouped;	// �O���[�v�����ꂽ�}�`�̃��X�g

		// �}�`���쐬����.
		ShapeGroup(void) {};

		//------------------------------
		// shape_group.cpp
		//------------------------------

		// �}�`��j������.
		~ShapeGroup(void);
		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �}�`���͂ޗ̈�𓾂�.
		void get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		void get_min_pos(D2D1_POINT_2F& value) const noexcept;
		// �J�n�ʒu�𓾂�.
		bool get_start_pos(D2D1_POINT_2F& value) const noexcept;
		// ������}�`���܂ނ����肷��.
		bool has_text(void) noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �����t���O�𔻒肷��.
		bool is_deleted(void) const noexcept;
		// �I���t���O�𔻒肷��.
		bool is_selected(void) const noexcept;
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F value);
		// �l�������t���O�Ɋi�[����.
		bool set_delete(const bool value) noexcept;
		// �l��I���t���O�Ɋi�[����.
		bool set_select(const bool value) noexcept;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		bool set_start_pos(const D2D1_POINT_2F value);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeGroup(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ���g�̂ЂȌ^
	//------------------------------
	struct ShapeStroke : Shape {
		bool m_flag_delete = false;	// �����t���O
		bool m_flag_select = false;	// �I���t���O
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// �J�n�ʒu
		std::vector<D2D1_POINT_2F> m_diff;	// ���̈ʒu�ւ̍���
		CAP_STYLE m_stroke_cap_style{ D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT };	// �����̒[�_
		D2D1_COLOR_F m_stroke_color{ S_BLACK };	// ���g�̐F
		D2D1_CAP_STYLE m_stroke_dash_cap = D2D1_CAP_STYLE::D2D1_CAP_STYLE_FLAT;	// �j���̒[�_
		STROKE_DASH_PATT m_stroke_dash_patt{ STROKE_DASH_PATT_DEF };	// �j���̔z�u
		D2D1_DASH_STYLE m_stroke_dash_style = D2D1_DASH_STYLE::D2D1_DASH_STYLE_SOLID;	// �j���̌`��
		float m_stroke_join_limit = MITER_LIMIT_DEF;		// ���̂Ȃ���̃}�C�^�[�����̔䗦
		D2D1_LINE_JOIN m_stroke_join_style = D2D1_LINE_JOIN::D2D1_LINE_JOIN_BEVEL;	// ���̂Ȃ���
		float m_stroke_width = 1.0f;	// ���g�̑���

		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_stroke_style{};	// D2D �X�g���[�N�X�^�C��

		//------------------------------
		// shape_stroke.cpp
		//------------------------------

		// �}�`��j������.
		~ShapeStroke(void);
		// �}�`���͂ޗ̈�𓾂�.
		void get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
		virtual void get_min_pos(D2D1_POINT_2F& value) const noexcept;
		// ���ʂ̈ʒu�𓾂�.
		virtual	void get_anch_pos(const uint32_t /*anch*/, D2D1_POINT_2F& value) const noexcept;
		// �J�n�ʒu�𓾂�.
		virtual bool get_start_pos(D2D1_POINT_2F& value) const noexcept;
		// ���̒[�_�𓾂�.
		bool get_stroke_cap_style(CAP_STYLE& value) const noexcept;
		// ���g�̐F�𓾂�.
		bool get_stroke_color(D2D1_COLOR_F& value) const noexcept;
		// ���̒[�_�𓾂�.
		bool get_stroke_dash_cap(D2D1_CAP_STYLE& value) const noexcept;
		// �j���̔z�u�𓾂�.
		bool get_stroke_dash_patt(STROKE_DASH_PATT& value) const noexcept;
		// �j���̌`���𓾂�.
		bool get_stroke_dash_style(D2D1_DASH_STYLE& value) const noexcept;
		// ���g�̃}�C�^�[�����̔䗦�𓾂�.
		bool get_stroke_join_limit(float& value) const noexcept;
		// ���̂Ȃ���𓾂�.
		bool get_stroke_join_style(D2D1_LINE_JOIN& value) const noexcept;
		// ���g�̑����𓾂�.
		bool get_stroke_width(float& value) const noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos, const size_t d_cnt, const D2D1_POINT_2F diff[], const bool s_close, const bool f_opa) const noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept;
		// �����t���O�𔻒肷��.
		bool is_deleted(void) const noexcept { return m_flag_delete; }
		// �I���t���O�𔻒肷��.
		bool is_selected(void) const noexcept { return m_flag_select; }
		// ���������ړ�����.
		virtual	bool move(const D2D1_POINT_2F value);
		// �l��I���t���O�Ɋi�[����.
		bool set_select(const bool value) noexcept { if (m_flag_select != value) { m_flag_select = value; return true; } return false; }
		// �l�������t���O�Ɋi�[����.
		bool set_delete(const bool value) noexcept { if (m_flag_delete != value) { m_flag_delete = value;  return true; } return false; }
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual bool set_start_pos(const D2D1_POINT_2F value);
		// �l����̒[�_�Ɋi�[����.
		bool set_stroke_cap_style(const CAP_STYLE& value);
		// �l����g�̐F�Ɋi�[����.
		bool set_stroke_color(const D2D1_COLOR_F& value) noexcept;
		// �l��j���̒[�_�Ɋi�[����.
		bool set_stroke_dash_cap(const D2D1_CAP_STYLE& value);
		// �l��j���̔z�u�Ɋi�[����.
		bool set_stroke_dash_patt(const STROKE_DASH_PATT& value);
		// �l����g�̌`���Ɋi�[����.
		bool set_stroke_dash_style(const D2D1_DASH_STYLE value);
		// �l���}�C�^�[�����̔䗦�Ɋi�[����.
		bool set_stroke_join_limit(const float& value);
		// �l����̂Ȃ���Ɋi�[����.
		bool set_stroke_join_style(const D2D1_LINE_JOIN& value);
		// �l����g�̑����Ɋi�[����.
		bool set_stroke_width(const float value) noexcept;
		// �}�`���쐬����.
		ShapeStroke(const size_t d_cnt, const ShapeSheet* s_attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeStroke(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// ��邵���f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, const ARROW_STYLE h_style, DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ��������
	//------------------------------
	struct ShapeLineA : ShapeStroke {
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// ��邵�̌`��
		ARROW_SIZE m_arrow_size{ ARROW_SIZE_DEF };	// ��邵�̐��@
		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_arrow_style{ nullptr };	// ��邵�� D2D �X�g���[�N�X�^�C��
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_arrow_geom{ nullptr };	// ��邵�� D2D �p�X�W�I���g��

		//------------------------------
		// shape_line.cpp
		//------------------------------

		// �R���X�g���N�^
		ShapeLineA(const size_t d_cnt, const ShapeSheet* s_attr, const bool a_none = false) :
			ShapeStroke(d_cnt, s_attr),
			m_arrow_style(a_none ? ARROW_STYLE::NONE : s_attr->m_arrow_style),
			m_arrow_size(s_attr->m_arrow_size),
			m_d2d_arrow_geom(nullptr),
			m_d2d_arrow_style(nullptr)
		{}
		// �}�`���쐬����.
		ShapeLineA(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeLineA(DataReader const& dt_reader);
		// �}�`��j������.
		~ShapeLineA(void);
		// �\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// ��邵�̐��@�𓾂�.
		bool get_arrow_size(ARROW_SIZE& size) const noexcept;
		// ��邵�̌`���𓾂�.
		bool get_arrow_style(ARROW_STYLE& value) const noexcept;
		// �l���邵�̐��@�Ɋi�[����.
		bool set_arrow_size(const ARROW_SIZE& value);
		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE value);
		//	�l��, ���ʂ̈ʒu�Ɋi�[����. 
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// �l���n�_�Ɋi�[����.
		bool set_start_pos(const D2D1_POINT_2F value);
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F value);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
		// 
		void write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, DataWriter const& dt_writer) const;
		bool set_stroke_cap_style(const CAP_STYLE& value);
		bool set_stroke_join_limit(const float& value);
		bool set_stroke_join_style(const D2D1_LINE_JOIN& value);
	};

	//------------------------------
	// ���`
	//------------------------------
	struct ShapeRect : ShapeStroke {
		D2D1_COLOR_F m_fill_color{ S_WHITE };		// �h��Ԃ��F

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// �}�`���쐬����.
		ShapeRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeRect(DataReader const& dt_reader);
		// �\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �}�`�̕��ʂ��ʒu���܂ނ����肷��.
		uint32_t hit_test_anchor(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �h��Ԃ��̐F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// �l��h��Ԃ��̐F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// ���ʂ̈ʒu�𓾂�.
		void get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept;
		//	�l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ��K
	// �쐬�������Ƃŕ�����̑����̕ύX�͂ł��Ȃ�.
	//------------------------------
	struct ShapeRuler : ShapeRect {
		float m_grid_base = GRID_LEN_DEF - 1.0f;	// ����̑傫�� (�� -1 �����l)
		winrt::com_ptr<IDWriteTextFormat> m_dw_text_format{};	// �e�L�X�g�t�H�[�}�b�g

		//------------------------------
		// shape_ruler.cpp
		//------------------------------

		// �}�`��j������.
		~ShapeRuler(void);
		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �}�`���쐬����.
		ShapeRuler(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeRuler(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// ���~
	//------------------------------
	struct ShapeElli : ShapeRect {
		// �}�`���쐬����.
		ShapeElli(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr) :
			ShapeRect::ShapeRect(b_pos, b_diff, s_attr)
		{}
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeElli(DataReader const& dt_reader) :
			ShapeRect::ShapeRect(dt_reader)
		{}

		//------------------------------
		// shape_elli.cpp
		//------------------------------

		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �p�ە��`
	//------------------------------
	struct ShapeRRect : ShapeRect {
		D2D1_POINT_2F m_corner_rad{ GRID_LEN_DEF, GRID_LEN_DEF };		// �p�ە����̔��a

		//------------------------------
		// shape_rrect.cpp
		// �p�ە��`
		//------------------------------

		// �}�`���쐬����.
		ShapeRRect(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeRRect(DataReader const& dt_reader);
		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �p�۔��a�𓾂�.
		bool get_corner_radius(D2D1_POINT_2F& value) const noexcept;
		// ���ʂ̈ʒu�𓾂�.
		void get_anch_pos(const uint32_t anch, D2D1_POINT_2F& value) const noexcept;
		//	�l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �܂���̂ЂȌ^
	//------------------------------
	struct ShapePath : ShapeLineA {
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_path_geom{};	// �܂���� D2D �p�X�W�I���g��

		//------------------------------
		// shape_path.cpp
		// �܂���̂ЂȌ^
		//------------------------------

		// �p�X�W�I���g�����쐬����.
		virtual void create_path_geometry(ID2D1Factory3* const/*d_factory*/) {}
		// �}�`���쐬����.
		ShapePath(const size_t d_cnt, const ShapeSheet* s_attr, const bool s_closed) :
			ShapeLineA::ShapeLineA(d_cnt, s_attr, s_closed), m_d2d_path_geom(nullptr) {}
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapePath(DataReader const& dt_reader) :
			ShapeLineA::ShapeLineA(dt_reader), m_d2d_path_geom(nullptr) {}
		// �}�`��j������.
		~ShapePath(void) { if (m_d2d_path_geom != nullptr) m_d2d_path_geom = nullptr; }
		// ���������ړ�����.
		bool move(const D2D1_POINT_2F value);
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// �l���邵�̐��@�Ɋi�[����.
		bool set_arrow_size(const ARROW_SIZE& value);
		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE value);
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		bool set_start_pos(const D2D1_POINT_2F value);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ���p�`
	//------------------------------
	struct ShapePoly : ShapePath {
		bool m_end_closed;	// �ӂ����Ă��邩����
		D2D1_COLOR_F m_fill_color;

		//------------------------------
		// shape_poly.cpp
		//------------------------------

		// ���E���`���݂������p�`���쐬����.
		static void create_poly_by_bbox(const D2D1_POINT_2F b_min, const D2D1_POINT_2F b_max, const size_t v_cnt, const bool v_up, const bool v_reg, const bool v_clock, D2D1_POINT_2F v_pos[]) noexcept;
		// �p�X�W�I���g�����쐬����.
		void create_path_geometry(ID2D1Factory3* const d_factory);
		// �\������
		void draw(SHAPE_DX& dx);
		// �h��Ԃ��F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& value) const noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �l���邵�̌`���Ɋi�[����.
		bool set_arrow_style(const ARROW_STYLE value);
		// �l��h��Ԃ��F�Ɋi�[����.
		bool set_fill_color(const D2D1_COLOR_F& value) noexcept;
		// �}�`���쐬����.
		ShapePoly(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr, const TOOL_POLY& poly);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapePoly(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& /*dt_writer*/) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& /*dt_writer*/) const;
	};

	//------------------------------
	// �Ȑ�
	//------------------------------
	struct ShapeBezi : ShapePath {

		//------------------------------
		// shape_bezi.cpp
		// �x�W�F�Ȑ�
		//------------------------------

		// �p�X�W�I���g�����쐬����.
		void create_path_geometry(ID2D1Factory3* const d_factory);
		// �\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �}�`���쐬����.
		ShapeBezi(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, const ShapeSheet* s_attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeBezi(DataReader const& dt_reader);
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ������
	//------------------------------
	struct ShapeText : ShapeRect {
		static wchar_t** s_available_fonts;		// �L���ȏ��̖�

		DWRITE_TEXT_RANGE m_select_range{ 0, 0 };	// �I��͈�
		D2D1_COLOR_F m_font_color{ S_BLACK };	// ���̂̐F
		wchar_t* m_font_family = nullptr;	// ���̖�
		float m_font_size = FONT_SIZE_DEF;	// ���̂̑傫��
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL;	// ���̂̐L�k
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL;	// ���̂̎���
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL;	// ���̂̑���
		wchar_t* m_text = nullptr;	// ������
		float m_text_line_h = 0.0f;	// �s�� (DIPs 96dpi�Œ�)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// �i�����낦
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING;	// ��������
		D2D1_SIZE_F m_text_margin{ TEXT_MARGIN_DEF };	// ������̂܂��̏㉺�ƍ��E�̗]��

		winrt::com_ptr<IDWriteTextLayout> m_dw_layout{};	// �������\�����邽�߂̃��C�A�E�g
		float m_dw_descent = 0.0f;	// �f�B�Z���g
		UINT32 m_dw_line_cnt = 0;	// �s�̌v�ʂ̗v�f��
		DWRITE_LINE_METRICS* m_dw_line_metrics = nullptr;	// �s�̌v��
		UINT32 m_dw_selected_cnt = 0;	// �I��͈͂̌v�ʂ̗v�f��
		DWRITE_HIT_TEST_METRICS* m_dw_selected_metrics = nullptr;	// �I��͈͂̌v��
		UINT32 m_dw_test_cnt = 0;	// �ʒu�̌v�ʂ̗v�f��
		DWRITE_HIT_TEST_METRICS* m_dw_test_metrics = nullptr;	// �ʒu�̌v��

		// �}�`��j������.
		~ShapeText(void);
		// �̈�̑傫����, ���ꂪ�������菬�����Ȃ�Ȃ��悤��, ��������. 
		bool adjust_bbox(const D2D1_SIZE_F& bound = D2D1_SIZE_F{ 0.0F, 0.0F });
		// �e�L�X�g���C�A�E�g��j�����č쐬����.
		void create_text_layout(IDWriteFactory3* d_factory);
		// �v�ʂ�j�����č쐬����.
		void create_text_metrics(IDWriteFactory3* d_factory);
		// �����̋󔒂���菜��.
		void delete_bottom_blank(void) noexcept;
		// �\������.
		void draw(SHAPE_DX& dx);
		// �I�����ꂽ�����͈͂�h��.
		void fill_range(SHAPE_DX& dx, const D2D1_POINT_2F t_min);
		// ������̘g��\������.
		void draw_frame(SHAPE_DX& dx, const D2D1_POINT_2F t_min);
		// �L���ȏ��̖�����v�f�𓾂�.
		static wchar_t* get_available_font(const uint32_t i);
		// ���̂̐F�𓾂�.
		bool get_font_color(D2D1_COLOR_F& value) const noexcept;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& value) const noexcept;
		// ���̂̑傫���𓾂�.
		bool get_font_size(float& value) const noexcept;
		// ���̂̐L�k�𓾂�.
		bool get_font_stretch(DWRITE_FONT_STRETCH& value) const noexcept;
		// ���̂̎��̂𓾂�.
		bool get_font_style(DWRITE_FONT_STYLE& value) const noexcept;
		// �s�Ԃ𓾂�.
		bool get_text_line(float& value) const noexcept;
		// ���̂̑����𓾂�.
		bool get_font_weight(DWRITE_FONT_WEIGHT& value) const noexcept;
		// ������̗]���𓾂�.
		bool get_text_margin(D2D1_SIZE_F& value) const noexcept;
		// �i���̂��낦�𓾂�.
		bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& value) const noexcept;
		// ������𓾂�.
		bool get_text(wchar_t*& value) const noexcept;
		// ������̂��낦�𓾂�.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& value) const noexcept;
		// �����͈͂𓾂�.
		bool get_text_range(DWRITE_TEXT_RANGE& value) const noexcept;
		// �ʒu���܂ނ����肷��.
		uint32_t hit_test(const D2D1_POINT_2F t_pos) const noexcept;
		// �͈͂Ɋ܂܂�邩���肷��.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �L���ȏ��̖������肵, �L���Ȃ���Ƃ̏��̖���j����, �L���ȏ��̖��v�f�ւ̃|�C���^�[�ƒu��������.
		static bool is_available_font(wchar_t*& font);
		// �L���ȏ��̖��̔z���j������.
		static void release_available_fonts(void);
		// �L���ȏ��̖��̔z���ݒ肷��.
		static void set_available_fonts(void);
		// �l�����̂̐F�Ɋi�[����.
		bool set_font_color(const D2D1_COLOR_F& value) noexcept;
		// �l�����̖��Ɋi�[����.
		bool set_font_family(wchar_t* const value);
		// �l�����̂̑傫���Ɋi�[����.
		bool set_font_size(const float value);
		// �l�����̂̐L�k�Ɋi�[����.
		bool set_font_stretch(const DWRITE_FONT_STRETCH value);
		// �l�����̂̎��̂Ɋi�[����.
		bool set_font_style(const DWRITE_FONT_STYLE value);
		// �l�����̂̑����Ɋi�[����.
		bool set_font_weight(const DWRITE_FONT_WEIGHT value);
		// �l���s�ԂɊi�[����.
		bool set_text_line(const float value);
		// �l�𕶎���̗]���Ɋi�[����.
		bool set_text_margin(const D2D1_SIZE_F value);
		// �l��i���̂��낦�Ɋi�[����.
		bool set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT value);
		// �l��, ���ʂ̈ʒu�Ɋi�[����.
		bool set_anchor_pos(const D2D1_POINT_2F value, const uint32_t anch);
		// �l�𕶎���Ɋi�[����.
		bool set_text(wchar_t* const value);
		// �l�𕶎���̂��낦�Ɋi�[����.
		bool set_text_align_t(const DWRITE_TEXT_ALIGNMENT value);
		// �l�𕶎��͈͂Ɋi�[����.
		bool set_text_range(const DWRITE_TEXT_RANGE value);
		// �}�`���쐬����.
		ShapeText(const D2D1_POINT_2F b_pos, const D2D1_POINT_2F b_diff, wchar_t* const text, const ShapeSheet* s_attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeText(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	// ��邵�̐��@�����������肷��.
	inline bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept
	{
		return equal(a.m_width, b.m_width) && equal(a.m_length, b.m_length) && equal(a.m_offset, b.m_offset);
	}

	// ���̒[�_�����������肷��.
	inline bool equal(const CAP_STYLE& a, const CAP_STYLE& b) noexcept
	{
		return a.m_start == b.m_start && a.m_end == b.m_end;
	}

	// �F�����������肷��.
	inline bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept
	{
		return equal_color_comp(a.a, b.a) && equal_color_comp(a.r, b.r) && equal_color_comp(a.g, b.g) && equal_color_comp(a.b, b.b);
	}

	// �ʒu�����������肷��.
	inline bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept
	{
		return equal(a.x, b.x) && equal(a.y, b.y);
	}

	// ���@�����������肷��.
	inline bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept
	{
		return equal(a.width, b.width) && equal(a.height, b.height);
	}

	// �{���x�������������������肷��.
	inline bool equal(const double a, const double b) noexcept
	{
		return fabs(a - b) <= FLT_EPSILON * fmax(1.0, fmax(fabs(a), fabs(b)));
	}

	// �����͈͂����������肷��.
	inline bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept
	{
		return a.startPosition == b.startPosition && a.length == b.length;
	}

	// �P���x�������������������肷��.
	inline bool equal(const float a, const float b) noexcept
	{
		return fabs(a - b) <= FLT_EPSILON * fmax(1.0f, fmax(fabs(a), fabs(b)));
	}

	// ����̌`�������������肷��.
	inline bool equal(const GRID_EMPH& a, const GRID_EMPH& b) noexcept
	{
		return a.m_gauge_1 == b.m_gauge_1 && a.m_gauge_2 == b.m_gauge_2;
	}

	// �j���̔z�u�����������肷��.
	inline bool equal(const STROKE_DASH_PATT& a, const STROKE_DASH_PATT& b) noexcept
	{
		return equal(a.m_[0], b.m_[0]) && equal(a.m_[1], b.m_[1]) && equal(a.m_[2], b.m_[2]) && equal(a.m_[3], b.m_[3]) && equal(a.m_[4], b.m_[4]) && equal(a.m_[5], b.m_[5]);
	}

	// �����񂪓��������肷��.
	inline bool equal(const wchar_t* a, const wchar_t* b) noexcept
	{
		return a == b || (a != nullptr && b != nullptr && wcscmp(a, b) == 0);
	}

	// �����񂪓��������肷��.
	inline bool equal(winrt::hstring const& a, const wchar_t* b) noexcept
	{
		return a == (b == nullptr ? L"" : b);
	}

	// �F�̐��������������肷��.
	inline bool equal_color_comp(const FLOAT a, const FLOAT b) noexcept
	{
		return fabs(b - a) * 128.0f < 1.0f;
	}

	// �x�N�g���̒��� (�̎���l) �𓾂�
	// a	�x�N�g��
	// �߂�l	���� (�̎���l) 
	inline double pt_abs2(const D2D1_POINT_2F a) noexcept
	{
		const double ax = a.x;
		const double ay = a.y;
		return ax * ax + ay * ay;
	}

	// �ʒu���ʒu�ɑ���
	inline void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x + b.x;
		c.y = a.y + b.y;
	}

	// �X�J���[�l���ʒu�ɑ���
	inline void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x + b);
		c.y = static_cast<FLOAT>(a.y + b);
	}

	// 2 �̒l���ʒu�ɑ���
	inline void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& b) noexcept
	{
		b.x = static_cast<FLOAT>(a.x + x);
		b.y = static_cast<FLOAT>(a.y + y);
	}

	// ��_�Ԃ̒��_�����߂�.
	inline void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>((a.x + b.x) * 0.5);
		c.y = static_cast<FLOAT>((a.y + b.y) * 0.5);
	}

	// �}�`�̕��ʂ��ʒu���܂ނ����肷��.
	inline bool pt_in_anch(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos) noexcept
	{
		const double a = Shape::s_anch_len * 0.5;
		const double dx = t_pos.x - a_pos.x;
		const double dy = t_pos.y - a_pos.y;
		return -a <= dx && dx <= a && -a <= dy && dy <= a;
	}

	// �ʒu���~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_circle(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double r) noexcept
	{
		const double dx = static_cast<double>(t_pos.x) - static_cast<double>(c_pos.x);
		const double dy = static_cast<double>(t_pos.y) - static_cast<double>(c_pos.y);
		return dx * dx + dy * dy <= r * r;
	}

	// �ʒu���~�Ɋ܂܂�邩���肷��.
	inline bool pt_in_circle(const D2D1_POINT_2F t_vec, const double r) noexcept
	{
		const double dx = t_vec.x;
		const double dy = t_vec.y;
		return dx * dx + dy * dy <= r * r;
	}

	// ��_�̈ʒu���ׂĂ��ꂼ��傫���l�����߂�.
	// a	����̈ʒu
	// b	��������̈ʒu
	// c	����
	inline void pt_max(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x > b.x ? a.x : b.x;
		c.y = a.y > b.y ? a.y : b.y;
	}

	// ��_�̈ʒu���ׂĂ��ꂼ�ꏬ�����l�����߂�.
	// a	����̈ʒu
	// b	��������̈ʒu
	// c	����
	inline void pt_min(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x < b.x ? a.x : b.x;
		c.y = a.y < b.y ? a.y : b.y;
	}

	// �ʒu�̕����𔽑΂ɂ���.
	// a	�ʒu
	// b	����
	inline void pt_neg(const D2D1_POINT_2F a, D2D1_POINT_2F& b) noexcept
	{
		b.x = -a.x;
		b.y = -a.y;
	}

	// �ʒu���X�J���[�{�Ɋۂ߂�.
	// a	�ʒu
	// b	�X�J���[�l
	// c	����
	inline void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(std::round(a.x / b) * b);
		c.y = static_cast<FLOAT>(std::round(a.y / b) * b);
	}

	// �ʒu�ɃX�J���[���|����, �ʒu��������.
	// a	�ʒu
	// b	�X�J���[�l
	// c	������ʒu
	// d	����
	inline void pt_mul(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.x * b + c.x);
		d.y = static_cast<FLOAT>(a.y * b + c.y);
	}

	// �ʒu�ɃX�J���[���|����.
	// a	�ʒu
	// b	�X�J���[�l
	// c	����
	inline void pt_mul(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept
	{
		c.x = static_cast<FLOAT>(a.x * b);
		c.y = static_cast<FLOAT>(a.y * b);
	}

	// ���@�ɃX�J���[�l���|����.
	// a	���@
	// b	�X�J���[�l
	// c	����
	inline void pt_mul(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& c) noexcept
	{
		c.width = static_cast<FLOAT>(a.width * b);
		c.height = static_cast<FLOAT>(a.height * b);
	}

	// �_�ɃX�J���[���|����, �ʒu��������.
	// a	�ʒu
	// b	�X�J���[�l
	// c	������ʒu
	// d	����
	inline void pt_mul(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept
	{
		d.x = static_cast<FLOAT>(a.X * b + c.x);
		d.y = static_cast<FLOAT>(a.Y * b + c.y);
	}

	// �ʒu����ʒu������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.x;
		c.y = a.y - b.y;
	}

	// �ʒu����傫��������.
	inline void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& c) noexcept
	{
		c.x = a.x - b.width;
		c.y = a.y - b.height;
	}

	// �F���s���������肷��.
	// value	�F
	// �߂�l	�s�����Ȃ�� true, �����Ȃ�� false.
	inline bool is_opaque(const D2D1_COLOR_F& value) noexcept
	{
		const uint32_t a = static_cast<uint32_t>(round(value.a * 255.0f));
		return (a & 0xff) > 0;
	}

}