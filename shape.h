//------------------------------
// shape.h
//	shape.cpp
//	shape_bezi.cpp	�x�W�F�Ȑ�
//	shape_dx.cpp	�}�`�̕`���
//	shape_elli.cpp	���~
//	shape_group.cpp	�O���[�v�}�`
//	shape_line.cpp	����
//	shape_list.cpp	�}�`���X�g
//	shape_panel.cpp	�p�l��
//	shape_quad.cpp	�l�ւ�`
//	shape.rect.cpp	���`
//	shape_rrect.cpp	�p�ە��`
//	shape_stroke.cpp	���g, �܂���̂ЂȌ^
//	shape_text.cpp	������}�`
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
// | ShapeStroke*| | ShapeGroup  | | ShapePanel  |
// +------+------+ +-------------+ +-------------+
//        |
//        +---------------+---------------+
//        |               |               |
// +------+------+ +------+------+ +------+------+
// | ShapePoly*  | | ShapeLine   | | ShapeRect   |
// +------+------+ +-------------+ +------+------+
//        |                               |
//        +---------------+               +---------------+---------------+---------------+
//        |               |               |               |               |               |
// +------+------+ +------+------+ +------+------+ +------+------+ +------+------+ +------+------+
// | ShapeQuad   | | ShapeBezi   | | ShapeElli   | | ShapeRRect  | | ShapeText   | | ShapeScale  |
// +-------------+ +-------------+ +-------------+ +-------------+ +-------------+ +-------------+

// SVG �̂��߂̃e�L�X�g���s�R�[�h
#define SVG_NL	"\n"

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::Point;
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::UI::Color;
	using winrt::Windows::UI::Xaml::Media::Brush;

#if defined(_DEBUG)
	extern uint32_t debug_leak_cnt;
#endif

	// �}�`�̕��� (�A���J�[)
	enum struct ANCH_WHICH {
		ANCH_OUTSIDE,		// �O�� (���J�[�\��)
		ANCH_INSIDE,		// ���� (�ړ��J�[�\��)
		ANCH_FRAME,		// ���g (�ړ��J�[�\��)
		ANCH_TEXT,		// ������
		ANCH_NW,		// ���`�̍���̒��_ (�k���쓌�J�[�\��)
		ANCH_SE,		// ���`�̉E���̒��_ (�k���쓌�J�[�\��)
		ANCH_NE,		// ���`�̉E��̒��_ (�k���쐼�J�[�\��)
		ANCH_SW,		// ���`�̍����̒��_ (�k���쐼�J�[�\��)
		ANCH_NORTH,		// ���`�̏�ӂ̒��_ (�㉺�J�[�\��)
		ANCH_SOUTH,		// ���`�̉��ӂ̒��_ (�㉺�J�[�\��)
		ANCH_EAST,		// ���`�̍��ӂ̒��_ (���E�J�[�\��)
		ANCH_WEST,		// ���`�̉E�ӂ̒��_ (���E�J�[�\��)
		ANCH_R_NW,		// �p�ۂ̍���̒��S�_ (�\���J�[�\��)
		ANCH_R_NE,		// �p�ۂ̉E��̒��S�_ (�\���J�[�\��)
		ANCH_R_SE,		// �p�ۂ̉E���̒��S�_ (�\���J�[�\��)
		ANCH_R_SW,		// �p�ۂ̍����̒��S�_ (�\���J�[�\��)
	};

	// ���`�̒��_�̔z��
	constexpr ANCH_WHICH ANCH_MIDDLE[4]{
		ANCH_WHICH::ANCH_SOUTH,
		ANCH_WHICH::ANCH_EAST,
		ANCH_WHICH::ANCH_WEST,
		ANCH_WHICH::ANCH_NORTH
	};

	// ���`�̒��_�̔z��
	constexpr ANCH_WHICH ANCH_CORNER[4]{
		ANCH_WHICH::ANCH_SE,
		ANCH_WHICH::ANCH_NE,
		ANCH_WHICH::ANCH_SW,
		ANCH_WHICH::ANCH_NW
	};

	// ���̌`��
	enum struct ARROW_STYLE {
		NONE,	// �Ȃ�
		OPENED,	// �J�������
		FILLED	// �������
	};

	// ����̕\��
	enum struct GRID_SHOW {
		HIDE,	// �\���Ȃ�
		BACK,	// �Ŕw�ʂɕ\��
		FRONT	// �őO�ʂɕ\��
	};

	// �}�`�̎��
	// �t�@�C���ւ̏������݂Ŏg�p����.
	enum SHAPE_TYPE {
		SHAPE_NULL,		// �k��
		SHAPE_BEZI,		// �Ȑ�
		SHAPE_ELLI,		// ���~
		SHAPE_LINE,		// ����
		SHAPE_QUAD,		// �l�ӌ`
		SHAPE_RECT,		// ���`
		SHAPE_RRECT,	// �p�ە��`
		SHAPE_TEXT,		// ������
		SHAPE_GROUP,	// �O���[�v
		SHAPE_SCALE		// �ڐ���
	};

	// ���̐��@
	struct ARROW_SIZE {
		float m_width = 7.0f;		// �Ԃ��̕�
		float m_length = 16.0f;		// �t���������[�܂ł̒���
		float m_offset = 0.0f;		// ��[�̃I�t�Z�b�g
	};

	// �j���̔z�u
	union STROKE_PATTERN {
		float m_[6];
	};

	// �F�����̍ő�l
	constexpr double COLOR_MAX = 255.0;
	// ��
	constexpr D2D1_COLOR_F S_WHITE{ 1.0f, 1.0f, 1.0f, 1.0f };
	// ��
	constexpr D2D1_COLOR_F S_BLACK{ 0.0f, 0.0f, 0.0f, 1.0f };
	// 1 �C���`������̃|�C���g��
	constexpr double PT_PER_INCH = 72.0;
	// 1 �C���`������̃~�����[�g����
	constexpr double MM_PER_INCH = 25.4;
	// ������̕s�����x
	constexpr float GRID_OPAC = 0.125f;
	// ����ʂ̕s�����x
	constexpr float ANCH_OPAC = 0.75f;

	//------------------------------
	// shape.cpp
	//------------------------------

	// UWP �̃u���V�� D2D1_COLOR_F �ɕϊ�����.
	bool cast_to(const Brush& a, D2D1_COLOR_F& b) noexcept;
	// UWP �̐F�� D2D1_COLOR_F �ɕϊ�����.
	void cast_to(const Color& a, D2D1_COLOR_F& b) noexcept;
	// ���� (���`) ��\������.
	void TOOL_anchor(const D2D1_POINT_2F a_pos, SHAPE_DX& dx);
	// ���� (�~) ��\������.
	void TOOL_anchor_rounded(const D2D1_POINT_2F& c_pos, SHAPE_DX& dx);
	// ���̐��@�����������ׂ�.
	bool equal(const ARROW_SIZE& a, const ARROW_SIZE& b) noexcept;
	// ���̌`�������������ׂ�.
	bool equal(const ARROW_STYLE a, const ARROW_STYLE b) noexcept;
	// �u�[���l�����������ׂ�.
	bool equal(const bool a, const bool b) noexcept;
	// �F�����������ׂ�.
	bool equal(const D2D1_COLOR_F& a, const D2D1_COLOR_F& b) noexcept;
	// �j���̌`�������������ׂ�.
	bool equal(const D2D1_DASH_STYLE a, const D2D1_DASH_STYLE b) noexcept;
	// �ʒu�����������ׂ�.
	bool equal(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept;
	// ���@�����������ׂ�.
	bool equal(const D2D1_SIZE_F a, const D2D1_SIZE_F b) noexcept;
	// �{���x�������������������ׂ�.
	bool equal(const double a, const double b) noexcept;
	// ���̂̕������������ׂ�.
	bool equal(const DWRITE_FONT_STRETCH a, const DWRITE_FONT_STRETCH b) noexcept;
	// ���̂̎��̂����������ׂ�.
	bool equal(const DWRITE_FONT_STYLE a, const DWRITE_FONT_STYLE b) noexcept;
	// ���̂̑��������������ׂ�.
	bool equal(const DWRITE_FONT_WEIGHT a, const DWRITE_FONT_WEIGHT b) noexcept;
	// �i���̂��낦�����������ׂ�.
	bool equal(const DWRITE_PARAGRAPH_ALIGNMENT a, const DWRITE_PARAGRAPH_ALIGNMENT b) noexcept;
	// ������̂��낦�����������ׂ�.
	bool equal(const DWRITE_TEXT_ALIGNMENT a, const DWRITE_TEXT_ALIGNMENT b) noexcept;
	// �����͈͂����������ׂ�.
	bool equal(const DWRITE_TEXT_RANGE a, const DWRITE_TEXT_RANGE b) noexcept;
	// �P���x�������������������ׂ�.
	bool equal(const float a, const float b) noexcept;
	// ������̕\�������������ׂ�.
	bool equal(const GRID_SHOW a, const GRID_SHOW b) noexcept;
	// �j���̔z�u�����������ׂ�.
	bool equal(const STROKE_PATTERN& a, const STROKE_PATTERN& b) noexcept;
	// 32 �r�b�g���������������ׂ�.
	bool equal(const uint32_t a, const uint32_t b) noexcept;
	// ���C�h�����񂪓��������ׂ�.
	bool equal(const wchar_t* a, const wchar_t* b) noexcept;
	// winrt �����񂪓��������ׂ�.
	bool equal(winrt::hstring const& a, const wchar_t* b) noexcept;
	// ���̎��Ɛ��@�����ƂɕԂ��̈ʒu���v�Z����.
	void get_arrow_barbs(const D2D1_POINT_2F axis, const double axis_len, const double barbWidth, const double barbLen, D2D1_POINT_2F barbs[]) noexcept;
	// �F���s���������ׂ�.
	bool is_opaque(const D2D1_COLOR_F& color) noexcept;
	// �x�N�g���̒��� (�̎���l) �𓾂�
	double pt_abs2(const D2D1_POINT_2F a) noexcept;
	// �ʒu�Ɉʒu�𑫂�
	void pt_add(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// �ʒu�ɃX�J���[�l�𑫂�
	void pt_add(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& c) noexcept;
	// �ʒu��X����Y���̒l�𑫂�
	void pt_add(const D2D1_POINT_2F a, const double x, const double y, D2D1_POINT_2F& c) noexcept;
	// ��_�̒��_�𓾂�.
	void pt_avg(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& c) noexcept;
	// ��_�ň͂܂ꂽ���`�𓾂�.
	void pt_bound(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) noexcept;
	// ��_�̓��ς𓾂�.
	double pt_dot(const D2D1_POINT_2F a, const D2D1_POINT_2F b) noexcept;
	// ���ʂ����_���܂ނ����ׂ�.
	bool pt_in_anch(const D2D1_POINT_2F a_pos, const double a_len) noexcept;
	// ���ʂ��ʒu���܂ނ����ׂ�.
	bool pt_in_anch(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F a_pos, const double a_len) noexcept;
	// ���~���ʒu���܂ނ����ׂ�.
	bool pt_in_elli(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F c_pos, const double rad_x, const double rad_y) noexcept;
	// �������ʒu���܂ނ�, �������l�����Ē��ׂ�.
	bool pt_in_line(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F s_pos, const D2D1_POINT_2F e_pos, const double s_width) noexcept;
	// �l�ւ�`���ʒu���܂ނ����ׂ�.
	bool pt_in_quad(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F q_pos[]) noexcept;
	// ���`���ʒu���܂ނ����ׂ�.
	bool pt_in_rect(const D2D1_POINT_2F t_pos, const D2D1_POINT_2F r_min, const D2D1_POINT_2F r_max) noexcept;
	// �w�肵���ʒu���܂ނ悤, ���`���g�傷��.
	void pt_inc(const D2D1_POINT_2F a, D2D1_POINT_2F& r_min, D2D1_POINT_2F& r_max) noexcept;
	// ��_���ׂĂ��ꂼ��̑傫���l�����ʒu�𓾂�.
	void pt_max(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min) noexcept;
	// ��_���ׂĂ��ꂼ��̏������l�����ʒu�𓾂�.
	void pt_min(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& r_min) noexcept;
	// �ʒu���X�J���[�{�Ɋۂ߂�.
	void pt_round(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& round) noexcept;
	// �ʒu�ɃX�J���[�l���|��, �ʂ̈ʒu�𑫂�.
	void pt_scale(const D2D1_POINT_2F a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& scale) noexcept;
	// �ʒu�ɃX�J���[�l���|����.
	void pt_scale(const D2D1_POINT_2F a, const double b, D2D1_POINT_2F& scale) noexcept;
	// ���@�ɒl���|����.
	void pt_scale(const D2D1_SIZE_F a, const double b, D2D1_SIZE_F& scale) noexcept;
	// �_�ɃX�J���[�l���|��, �ʂ̈ʒu�𑫂�.
	void pt_scale(const Point a, const double b, const D2D1_POINT_2F c, D2D1_POINT_2F& d) noexcept;
	// �ʒu����ʒu������.
	void pt_sub(const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& sub) noexcept;
	// �ʒu����傫��������.
	void pt_sub(const D2D1_POINT_2F a, const D2D1_SIZE_F b, D2D1_POINT_2F& sub) noexcept;
	// ���̐��@��ǂݍ���.
	void read(ARROW_SIZE& val, DataReader const& dt_reader);
	// ���̌`�����f�[�^���[�_�[����ǂݍ���.
	void read(ARROW_STYLE& val, DataReader const& dt_reader);
	// �u�[���l���f�[�^���[�_�[����ǂݍ���.
	void read(bool& val, DataReader const& dt_reader);
	// �{���x�����������f�[�^���[�_�[����ǂݍ���.
	void read(double& val, DataReader const& dt_reader);
	// �F���f�[�^���[�_�[����ǂݍ���.
	void read(D2D1_COLOR_F& val, DataReader const& dt_reader);
	// �j���̌`�����f�[�^���[�_�[����ǂݍ���.
	void read(D2D1_DASH_STYLE& val, DataReader const& dt_reader);
	// �ʒu���f�[�^���[�_�[����ǂݍ���.
	void read(D2D1_POINT_2F& val, DataReader const& dt_reader);
	// ���@���f�[�^���[�_�[����ǂݍ���.
	void read(D2D1_SIZE_F& val, DataReader const& dt_reader);
	// ���̂̎��̂��f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_FONT_STYLE& val, DataReader const& dt_reader);
	// ���̂̑������f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_FONT_WEIGHT& val, DataReader const& dt_reader);
	// ���̂̐L�k���f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_FONT_STRETCH& val, DataReader const& dt_reader);
	// �i���̂��낦���f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_PARAGRAPH_ALIGNMENT& val, DataReader const& dt_reader);
	// ������̂��낦���f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_TEXT_ALIGNMENT& val, DataReader const& dt_reader);
	// �����͈͂��f�[�^���[�_�[����ǂݍ���.
	void read(DWRITE_TEXT_RANGE& val, DataReader const& dt_reader);
	// ����̕\�����f�[�^���[�_�[����ǂݍ���.
	void read(GRID_SHOW& val, DataReader const& dt_reader);
	// �j���̔z�u���f�[�^���[�_�[����ǂݍ���.
	void read(STROKE_PATTERN& val, DataReader const& dt_reader);
	// 32 �r�b�g�������f�[�^���[�_�[����ǂݍ���.
	void read(uint32_t& val, DataReader const& dt_reader);
	// ��������f�[�^���[�_�[����ǂݍ���.
	void read(wchar_t*& val, DataReader const& dt_reader);
	// ������𕡐�����. ���̕����񂪃k���|�C���^�[, �܂��͌��̕������� 0 �̂Ƃ���, �k���|�C���^�[��Ԃ�.
	wchar_t* wchar_cpy(const wchar_t* const s);
	// ������̒���. �������k���|�C���^�̏ꍇ, 0 ��Ԃ�.
	uint32_t wchar_len(const wchar_t* const t) noexcept;
	// ���̐��@���f�[�^���C�^�[�ɏ�������.
	void write(const ARROW_SIZE& val, DataWriter const& dt_writer);
	// ���̌`�����f�[�^���C�^�[�ɏ�������.
	void write(const ARROW_STYLE val, DataWriter const& dt_writer);
	// �u�[���l���f�[�^���C�^�[�ɏ�������.
	void write(const bool val, DataWriter const& dt_writer);
	// �F���f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_COLOR_F& val, DataWriter const& dt_writer);
	// �j���̌`�����f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_DASH_STYLE val, DataWriter const& dt_writer);
	// �ʒu���f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_POINT_2F val, DataWriter const& dt_writer);
	// ���@���f�[�^���C�^�[�ɏ�������.
	void write(const D2D1_SIZE_F val, DataWriter const& dt_writer);
	// �{���x�����������f�[�^���C�^�[�ɏ�������.
	void write(const double val, DataWriter const& dt_writer);
	// ���̂̎��̂��f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_FONT_STYLE val, DataWriter const& dt_writer);
	// ���̂̐L�k���f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_FONT_STRETCH val, DataWriter const& dt_writer);
	// ���̂̑������f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_FONT_WEIGHT val, DataWriter const& dt_writer);
	// �i���̂��낦���f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_PARAGRAPH_ALIGNMENT val, DataWriter const& dt_writer);
	// ������̂��낦���f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_TEXT_ALIGNMENT val, DataWriter const& dt_writer);
	// ������͈͂��f�[�^���C�^�[�ɏ�������.
	void write(const DWRITE_TEXT_RANGE val, DataWriter const& dt_writer);
	// ����̕\�����f�[�^���C�^�[�ɏ�������.
	void write(const GRID_SHOW val, DataWriter const& dt_writer);
	// �j���̔z�u���f�[�^���C�^�[�ɏ�������.
	void write(const STROKE_PATTERN& val, DataWriter const& dt_writer);
	// 32 �r�b�g�������f�[�^���C�^�[�ɏ�������.
	void write(const uint32_t val, DataWriter const& dt_writer);
	// ��������f�[�^���C�^�[�ɏ�������.
	void write(const wchar_t* val, DataWriter const& dt_writer);
	// �V���O���o�C�g��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const char* val, DataWriter const& dt_writer);
	// �}���`�o�C�g��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const wchar_t* val, const uint32_t v_len, DataWriter const& dt_writer);
	// �������ƃV���O���o�C�g��������f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const char* val, const char* name, DataWriter const& dt_writer);
	// ���߂ƈʒu���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_POINT_2F val, const char* cmd, DataWriter const& dt_writer);
	// �������ƈʒu���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_POINT_2F val, const char* name_x, const char* name_y, DataWriter const& dt_writer);
	// �������ƐF���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_COLOR_F val, const char* name, DataWriter const& dt_writer);
	// �F���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_COLOR_F val, DataWriter const& dt_writer);
	// �����������f�[�^���C�^�[�ɏ�������
	void write_svg(const float val, DataWriter const& dt_writer);
	// �������ƕ��������l���f�[�^���C�^�[�� SVG �Ƃ��ď�������
	void write_svg(const double val, const char* name, DataWriter const& dt_writer);
	// �������� 32 �r�b�g���������f�[�^���C�^�[�� SVG �Ƃ��ď�������
	void write_svg(const uint32_t val, const char* name, DataWriter const& dt_writer);
	// �j���̌`���Ɣz�u���f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	void write_svg(const D2D1_DASH_STYLE style, const STROKE_PATTERN& pattern, const double width, DataWriter const& dt_writer);

	// �}�`�̂ЂȌ^
	struct Shape {
		// D2D �t�@�N�g���̃L���b�V��
		static ID2D1Factory3* s_d2d_factory;
		// DWRITE �t�@�N�g���̃L���b�V��
		static IDWriteFactory3* s_dwrite_factory;

		// �}�`��j������.
		virtual ~Shape() {}
		// �}�`��\������
		virtual void draw(SHAPE_DX& /*dx*/) {}
		// ���̐��@�𓾂�
		virtual bool get_arrow_size(ARROW_SIZE& /*val*/) const noexcept { return false; }
		// ���̌`���𓾂�.
		virtual bool get_arrow_style(ARROW_STYLE& /*val*/) const noexcept { return false; }
		// �}�`���͂ޗ̈�𓾂�.
		virtual void get_bound(D2D1_POINT_2F& /*b_min*/, D2D1_POINT_2F& /*b_max*/) const noexcept {}
		// �p�۔��a�𓾂�.
		virtual bool get_corner_radius(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// �h��Ԃ��F�𓾂�.
		virtual bool get_fill_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// ���̂̐F�𓾂�.
		virtual bool get_font_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// ���̖��𓾂�.
		virtual bool get_font_family(wchar_t*& /*val*/) const noexcept { return false; }
		// ���̂̑傫���𓾂�.
		virtual bool get_font_size(double& /*val*/) const noexcept { return false; }
		// ���̂̉����𓾂�.
		virtual bool get_font_stretch(DWRITE_FONT_STRETCH& /*val*/) const noexcept { return false; }
		// ���̂̎��̂𓾂�.
		virtual bool get_font_style(DWRITE_FONT_STYLE& /*val*/) const noexcept { return false; }
		// ���̂̑����𓾂�.
		virtual bool get_font_weight(DWRITE_FONT_WEIGHT& /*val*/) const noexcept { return false; }
		// ����̑傫���𓾂�.
		virtual bool get_grid_size(double& /*val*/) const noexcept { return false; }
		// ����̑傫���𓾂�.
		virtual bool get_grid_opac(double& /*val*/) const noexcept { return false; }
		// ����̕\���𓾂�.
		virtual bool get_grid_show(GRID_SHOW& /*val*/) const noexcept { return false; }
		// ����̕\���𓾂�.
		virtual bool get_grid_snap(bool& /*val*/) const noexcept { return false; }
		// �}�`���͂ޕ��`�̍���_�𓾂�.
		virtual void get_min_pos(D2D1_POINT_2F& /*val*/) const noexcept {}
		// �y�[�W�̐F�𓾂�.
		virtual bool get_page_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// �y�[�W�̊g�嗦�𓾂�.
		virtual bool get_page_scale(double& /*val*/) const noexcept { return false; }
		// �y�[�W�̑傫���𓾂�.
		virtual bool get_page_size(D2D1_SIZE_F& /*val*/) const noexcept { return false; }
		// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
		virtual	void get_pos(const ANCH_WHICH /*a*/, D2D1_POINT_2F&/*val*/) const noexcept {}
		// �n�_�𓾂�
		virtual bool get_start_pos(D2D1_POINT_2F& /*val*/) const noexcept { return false; }
		// ���g�̐F�𓾂�.
		virtual bool get_stroke_color(D2D1_COLOR_F& /*val*/) const noexcept { return false; }
		// �j���̔z�u�𓾂�.
		virtual bool get_stroke_pattern(STROKE_PATTERN& /*val*/) const noexcept { return false; }
		// �j���̌`���𓾂�.
		virtual bool get_stroke_style(D2D1_DASH_STYLE& /*val*/) const noexcept { return false; }
		// ���̂̑����𓾂�
		virtual bool get_stroke_width(double& /*val*/) const noexcept { return false; }
		// ������𓾂�.
		virtual bool get_text(wchar_t*& /*val*/) const noexcept { return false; }
		// �i���̂��낦�𓾂�.
		virtual bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& /*val*/) const noexcept { return false; }
		// ������̂��낦�𓾂�.
		virtual bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& /*val*/) const noexcept { return false; }
		// �s�̍����𓾂�.
		virtual bool get_text_line_height(double& /*val*/) const noexcept { return false; }
		// ������̎��̗͂]���𓾂�.
		virtual bool get_text_margin(D2D1_SIZE_F& /*val*/) const noexcept { return false; }
		// �����͈͂𓾂�
		virtual bool get_text_range(DWRITE_TEXT_RANGE& /*val*/) const noexcept { return false; }
		// �ʒu���܂ނ����ׂ�.
		virtual ANCH_WHICH hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept { return ANCH_WHICH::ANCH_OUTSIDE; }
		// �͈͂Ɋ܂܂�邩���ׂ�.
		virtual bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept { return false; }
		// �����t���O�𒲂ׂ�.
		virtual bool is_deleted(void) const noexcept { return false; }
		// �I���t���O�𒲂ׂ�.
		virtual bool is_selected(void) const noexcept { return false; }
		// ���������ړ�����.
		virtual	void move(const D2D1_POINT_2F /*d*/) {}
		// �l����̐��@�Ɋi�[����.
		virtual void set_arrow_size(const ARROW_SIZE& /*val*/) {}
		// �l����̌`���Ɋi�[����.
		virtual void set_arrow_style(const ARROW_STYLE /*val*/) {}
		// �l�������t���O�Ɋi�[����.
		virtual void set_delete(const bool /*val*/) noexcept {}
		// �l��h��Ԃ��F�Ɋi�[����.
		virtual void set_fill_color(const D2D1_COLOR_F& /*val*/) noexcept {}
		// �l�����̂̐F�Ɋi�[����.
		virtual void set_font_color(const D2D1_COLOR_F& /*val*/) noexcept {}
		// �l�����̖��Ɋi�[����.
		virtual void set_font_family(wchar_t* const /*val*/) {}
		// �l�����̂̑傫���Ɋi�[����.
		virtual void set_font_size(const double /*val*/) {}
		// �l�����̂̉����Ɋi�[����.
		virtual void set_font_stretch(const DWRITE_FONT_STRETCH /*val*/) {}
		// �l�����̂̎��̂Ɋi�[����.
		virtual void set_font_style(const DWRITE_FONT_STYLE /*val*/) {}
		// �l�����̂̑����Ɋi�[����.
		virtual void set_font_weight(const DWRITE_FONT_WEIGHT /*val*/) {}
		// �l�����̑傫���Ɋi�[����.
		virtual void set_grid_size(const double /*val*/) noexcept {}
		// �l�����̑傫���Ɋi�[����.
		virtual void set_grid_opac(const double /*val*/) noexcept {}
		// �l�����̕\���Ɋi�[����.
		virtual void set_grid_show(const GRID_SHOW /*val*/) noexcept {}
		// �l�����ւ̑����Ɋi�[����.
		virtual void set_grid_snap(const bool /*val*/) noexcept {}
		// �l���y�[�W�̐F�Ɋi�[����.
		virtual void set_page_color(const D2D1_COLOR_F& /*val*/) noexcept {}
		// �l���y�[�W�̊g�嗦�Ɋi�[����.
		virtual void set_page_scale(const double /*val*/) noexcept {}
		// �l���y�[�W�̑傫���Ɋi�[����.
		virtual void set_page_size(const D2D1_SIZE_F /*val*/) noexcept {}
		// �l���w�肵�����ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
		virtual void set_pos(const D2D1_POINT_2F /*val*/, const ANCH_WHICH /*a*/) {}
		// �l��I���t���O�Ɋi�[����.
		virtual void set_select(const bool /*val*/) noexcept {}
		// �l����g�̐F�Ɋi�[����.
		virtual void set_stroke_color(const D2D1_COLOR_F& /*val*/) noexcept {}
		// �l��j���̔z�u�Ɋi�[����.
		virtual void set_stroke_pattern(const STROKE_PATTERN& /*val*/) {}
		// �l����g�̌`���Ɋi�[����.
		virtual void set_stroke_style(const D2D1_DASH_STYLE /*val*/) {}
		// �l�����̂̑����Ɋi�[����.
		virtual void set_stroke_width(const double /*val*/) noexcept {}
		// �l�𕶎���Ɋi�[����.
		virtual void set_text(wchar_t* const /*val*/) {}
		// �l��i���̂��낦�Ɋi�[����.
		virtual void set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT /*val*/) {}
		// �l�𕶎���̂��낦�Ɋi�[����.
		virtual void set_text_align_t(const DWRITE_TEXT_ALIGNMENT /*val*/) {}
		// �l���s�ԂɊi�[����.
		virtual void set_text_line_height(const double /*val*/) {}
		// �l�𕶎���̗]���Ɋi�[����.
		virtual void set_text_margin(const D2D1_SIZE_F /*val*/) {}
		// �l�𕶎��͈͂Ɋi�[����.
		virtual void set_text_range(const DWRITE_TEXT_RANGE /*val*/) {}
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual void set_start_pos(const D2D1_POINT_2F /*val*/) {}
		// �f�[�^���C�^�[�ɏ�������.
		virtual void write(DataWriter const& /*dt_writer*/) const {}
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		virtual void write_svg(DataWriter const& /*dt_writer*/) const {}
	};

	//------------------------------
	// shape_list.cpp
	// �}�`���X�g�Ɋ֘A��������
	//------------------------------

	typedef std::list<struct Shape*>	S_LIST_T;
	// �Ō�̐}�`�����X�g���瓾��.
	Shape* s_list_back(S_LIST_T const& s_list) noexcept;
	// �}�`���X�g����������.
	void s_list_clear(S_LIST_T& s_list) noexcept;
	// �}�`�̃��X�g��ł̈ʒu�𓾂�.
	uint32_t s_list_distance(S_LIST_T const& s_list, const Shape* s) noexcept;
	// �ŏ��̐}�`�����X�g���瓾��.
	Shape* s_list_front(S_LIST_T const& s_list) noexcept;
	// �}�`�S�̗̂̈�����X�g���瓾��.
	void s_list_bound(S_LIST_T const& s_list, const D2D1_SIZE_F p_size, D2D1_POINT_2F& p_min, D2D1_POINT_2F& p_max) noexcept;
	// �ʒu���܂ސ}�`�Ƃ��̕��ʂ����X�g���瓾��.
	ANCH_WHICH s_list_hit_test(S_LIST_T const& s_list, const D2D1_POINT_2F t_pos, const double a_len, Shape*& s) noexcept;
	// �}�`�����X�g�ɑ}������.
	void s_list_insert(S_LIST_T& s_list, Shape* s_ins, const Shape* s_pos) noexcept;
	// �I���t���O�̗����ׂĂ̐}�`�����������ړ�����.
	void s_list_move(S_LIST_T const& s_list, const D2D1_POINT_2F d) noexcept;
	// ���̐}�`�����X�g���瓾��.
	Shape* s_list_next(S_LIST_T const& s_list, const Shape* s) noexcept;
	// �O�̐}�`�����X�g���瓾��.
	Shape* s_list_prev(S_LIST_T const& s_list, const Shape* s) noexcept;
	// �}�`���X�g���f�[�^���[�_�[����ǂݍ���.
	void s_list_read(S_LIST_T& s_list, DataReader const& dt_reader);
	// �}�`�����X�g����폜��, �폜�����}�`�̎��̐}�`�𓾂�.
	Shape* s_list_remove(S_LIST_T& s_list, const Shape* s) noexcept;
	// �I�����ꂽ�}�`�̃��X�g�𓾂�.
	template <typename S> void s_list_select(S_LIST_T const& s_list, S_LIST_T& sel_list) noexcept;
	// �}�`���X�g���f�[�^���C�^�[�ɏ�������. REDUCE �ꍇ�̏����t���O�̗��}�`�͖�������.
	template<bool REDUCE> void s_list_write(const S_LIST_T& s_list, DataWriter const& dt_writer);
	// ���X�g�̒��̐}�`�̏��Ԃ𓾂�.
	template <typename S, typename T> bool s_list_match(S_LIST_T const& s_list, S s, T& t);
	//	�}�`���X�g���當����𓾂�.
	winrt::hstring s_list_text(S_LIST_T const& s_list) noexcept;

	//------------------------------
	// �}�`�p�l��
	//------------------------------
	struct ShapePanel : Shape {

		// ����̑���
		D2D1_COLOR_F m_grid_color;	// ������̐F (MainPage �̃R���X�g���N�^�Őݒ�)
		double m_grid_size = 0.0;	// ����̑傫�� (�� -1 �����l)
		double m_grid_opac = GRID_OPAC;	// ������̕s�����x
		GRID_SHOW m_grid_show = GRID_SHOW::BACK;	// ������̕\��
		bool m_grid_snap = true;	// ����ɐ���

		// �y�[�W�̑���
		D2D1_COLOR_F m_page_color = S_WHITE;	// �w�i�F (MainPage �̃R���X�g���N�^�Őݒ�)
		double m_page_scale = 1.0;	// �y�[�W�̊g�嗦
		D2D1_SIZE_F	m_page_size{ 8.27f * 96.0f, 11.69f * 96.0f };	// �y�[�W�̑傫�� (MainPage �̃R���X�g���N�^�Őݒ�)

		// �}�`�̑���
		ARROW_SIZE m_arrow_size{ 7.0f, 16.0f, 0.0f };	// ���̐��@
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// ���̌`��
		D2D1_POINT_2F m_corner_rad{ 0.0f, 0.0f };	// �p�۔��a
		D2D1_COLOR_F m_fill_color = S_WHITE;	// �h��Ԃ��̐F
		D2D1_COLOR_F m_stroke_color = S_BLACK;	// ���g�̐F (MainPage �̃R���X�g���N�^�Őݒ�)
		STROKE_PATTERN m_stroke_pattern{ 4.0f, 3.0f, 1.0f, 3.0f };	// �j���̔z�u
		D2D1_DASH_STYLE m_stroke_style = D2D1_DASH_STYLE_SOLID;	// �j���̌`��
		double m_stroke_width = 1.0;	// ���g�̑���
		D2D1_COLOR_F m_font_color = S_BLACK;	// ���̂̐F (MainPage �̃R���X�g���N�^�Őݒ�)
		wchar_t* m_font_family = nullptr;	// ���̖�
		double m_font_size = 12.0 * 96.0 / 72.0;	// ���̂̑傫��
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH_UNDEFINED;	// ���̂̐L�k
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE_NORMAL;	// ���̂̎���
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT_NORMAL;	// ���̂̑���
		double m_text_line = 0.0;	// �s�� (DIPs 96dpi�Œ�)
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// �i���̑���
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT_LEADING;	// ������̑���
		D2D1_SIZE_F m_text_mar{ 4.0f, 4.0f };	// ������̍��E�Ə㉺�̗]��

		D2D1_COLOR_F m_anch_color;	// ���ʂ̐F
		D2D1_COLOR_F m_aux_color;	// �⏕���̐F

		//------------------------------
		// shape_panel.cpp
		//------------------------------

		// �Ȑ��̕⏕����\������.
		void TOOL_auxiliary_bezi(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// ���~�̕⏕����\������.
		void TOOL_auxiliary_elli(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// �����̕⏕����\������.
		void TOOL_auxiliary_line(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// ���`�̕⏕����\������.
		void TOOL_auxiliary_rect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// �l�ӌ`�̕⏕����\������.
		void TOOL_auxiliary_quad(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// �p�ە��`�̕⏕����\������.
		void TOOL_auxiliary_rrect(SHAPE_DX const& dx, const D2D1_POINT_2F p_pos, const D2D1_POINT_2F c_pos);
		// �������\������,
		void TOOL_grid_line(SHAPE_DX const& dx, const D2D1_POINT_2F offset);
		// ���̐��@�𓾂�.
		bool get_arrow_size(ARROW_SIZE& val) const noexcept;
		// ���̌`���𓾂�.
		bool get_arrow_style(ARROW_STYLE& val) const noexcept;
		// ����̑傫���𓾂�.
		bool get_grid_size(double& val) const noexcept;
		// ����̑傫���𓾂�.
		bool get_grid_opac(double& val) const noexcept;
		// ����̕\���̏�Ԃ𓾂�.
		bool get_grid_show(GRID_SHOW& val) const noexcept;
		// ����ւ̂��낦�𓾂�.
		bool get_grid_snap(bool& val) const noexcept;
		// �y�[�W�̐F�𓾂�.
		bool get_page_color(D2D1_COLOR_F& val) const noexcept;
		// �y�[�W�̐F�𓾂�.
		bool get_page_size(D2D1_SIZE_F& val) const noexcept;
		// �y�[�W�̊g�嗦�𓾂�.
		bool get_page_scale(double& val) const noexcept;
		// �p�۔��a�𓾂�.
		bool get_corner_radius(D2D1_POINT_2F& val) const noexcept;
		// �h��Ԃ��F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept;
		// ���̂̐F�𓾂�.
		bool get_font_color(D2D1_COLOR_F& val) const noexcept;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& val) const noexcept;
		// ���̂̑傫���𓾂�.
		bool get_font_size(double& val) const noexcept;
		// ���̂̉����𓾂�.
		bool get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept;
		// ���̂̎��̂𓾂�.
		bool get_font_style(DWRITE_FONT_STYLE& val) const noexcept;
		// ���̂̑����𓾂�.
		bool get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept;
		// �s�Ԃ𓾂�.
		bool get_text_line_height(double& val) const noexcept;
		// ������̎��̗͂]���𓾂�.
		bool get_text_margin(D2D1_SIZE_F& val) const noexcept;
		// �i���̂��낦�𓾂�.
		bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept;
		// ���g�̐F�𓾂�.
		bool get_stroke_color(D2D1_COLOR_F& val) const noexcept;
		// �j���̔z�u�𓾂�.
		bool get_stroke_pattern(STROKE_PATTERN& val) const noexcept;
		// �j���̌`���𓾂�.
		bool get_stroke_style(D2D1_DASH_STYLE& val) const noexcept;
		// ���̂̑����𓾂�
		bool get_stroke_width(double& val) const noexcept;
		// ������̂��낦�𓾂�.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept;
		// �f�[�^���[�_�[����ǂݍ���.
		void read(DataReader const& dt_reader);
		// �}�`�̑����l���i�[����.
		void set_to_shape(Shape* s) noexcept;
		// �l�����̑傫���Ɋi�[����.
		void set_grid_size(const double val) noexcept;
		// �l�����̕s�����x�Ɋi�[����.
		void set_grid_opac(const double val) noexcept;
		// �l�����̕\���Ɋi�[����.
		void set_grid_show(const GRID_SHOW val) noexcept;
		// �l�����ւ̂��낦�Ɋi�[����.
		void set_grid_snap(const bool val) noexcept;
		// �l���y�[�W�̐F�Ɋi�[����.
		void set_page_color(const D2D1_COLOR_F& val) noexcept;
		// �l���y�[�W�̐��@�Ɋi�[����.
		void set_page_size(const D2D1_SIZE_F val) noexcept;
		// �l���y�[�W�̊g�嗦�Ɋi�[����.
		void set_page_scale(const double val) noexcept;
		// �l����̐��@�Ɋi�[����.
		void set_arrow_size(const ARROW_SIZE& val);
		// �l����̌`���Ɋi�[����.
		void set_arrow_style(const ARROW_STYLE val);
		// �l��h��Ԃ��F�Ɋi�[����.
		void set_fill_color(const D2D1_COLOR_F& val) noexcept;
		// �l�����̂̐F�Ɋi�[����.
		void set_font_color(const D2D1_COLOR_F& val) noexcept;
		// ���̖��Ɋi�[����.
		void set_font_family(wchar_t* const val);
		// ���̂̑傫���Ɋi�[����.
		void set_font_size(const double val);
		// �l�����̂̐L�k�Ɋi�[����.
		void set_font_stretch(const DWRITE_FONT_STRETCH val);
		// �l�����̂̎��̂Ɋi�[����.
		void set_font_style(const DWRITE_FONT_STYLE val);
		// �l�����̂̑����Ɋi�[����.
		void set_font_weight(const DWRITE_FONT_WEIGHT val);
		// �l���s�ԂɊi�[����.
		void set_text_line_height(const double val);
		// �l�𕶎���̗]���Ɋi�[����.
		void set_text_margin(const D2D1_SIZE_F val);
		// �l��i���̂��낦�Ɋi�[����.
		void set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT val);
		// �l����g�̐F�Ɋi�[����.
		void set_stroke_color(const D2D1_COLOR_F& val) noexcept;
		// �l��j���̔z�u�Ɋi�[����.
		void set_stroke_pattern(const STROKE_PATTERN& val);
		// �l����g�̌`���Ɋi�[����.
		void set_stroke_style(const D2D1_DASH_STYLE val);
		// �l�����̂̑����Ɋi�[����.
		void set_stroke_width(const double val) noexcept;
		// �l�𕶎���̂��낦�Ɋi�[����.
		void set_text_align_t(const DWRITE_TEXT_ALIGNMENT val);
		// �f�[�^���[�_�[�ɏ�������.
		void write(DataWriter const& dt_writer);

	};

	//------------------------------
	// �O���[�v�}�`
	//------------------------------
	struct ShapeGroup : Shape {
		S_LIST_T m_grp_list;	// �O���[�v�Ɋ܂܂��}�`�̃��X�g

		// �}�`���쐬����.
		ShapeGroup(void) {};

		//------------------------------
		// shape_group.cpp
		//------------------------------

		// �}�`��j������.
		~ShapeGroup(void);
		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �}�`���͂ޕ��`�𓾂�.
		void get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// �}�`���͂ޕ��`�̍���_�𓾂�.
		void get_min_pos(D2D1_POINT_2F& val) const noexcept;
		// �n�_�𓾂�
		bool get_start_pos(D2D1_POINT_2F& val) const noexcept;
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// �͈͂Ɋ܂܂�邩���ׂ�.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �����t���O�𒲂ׂ�.
		bool is_deleted(void) const noexcept;
		// �I���t���O�𒲂ׂ�.
		bool is_selected(void) const noexcept;
		// ���������ړ�����
		void move(const D2D1_POINT_2F d);
		// �l�������t���O�Ɋi�[����.
		void set_delete(const bool val) noexcept;
		// �l��I���t���O�Ɋi�[����.
		void set_select(const bool val) noexcept;
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		void set_start_pos(const D2D1_POINT_2F s_pos);
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
		bool m_deleted = false;	// �����t���O
		bool m_selected = true;	// �I���t���O
		D2D1_POINT_2F m_pos{ 0.0f, 0.0f };	// �n�_
		D2D1_POINT_2F m_vec{ 0.0f, 0.0f };	// �I�_�x�N�g��
		D2D1_COLOR_F m_stroke_color;	// ���g�̐F
		STROKE_PATTERN m_stroke_pattern{};	// �j���̔z�u
		D2D1_DASH_STYLE m_stroke_style;	// �j���̌`��
		double m_stroke_width;	// ���g�̑���
		winrt::com_ptr<ID2D1StrokeStyle> m_d2d_stroke_style{};	// D2D �X�g���[�N�X�^�C��

		//------------------------------
		// shape_stroke.cpp
		//------------------------------

		// �}�`��j������.
		~ShapeStroke(void);

		// �}�`���͂ޕ��`�𓾂�.
		void get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// �}�`���͂ޕ��`�̍���_�𓾂�.
		virtual void get_min_pos(D2D1_POINT_2F& val) const noexcept;
		// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
		virtual	void get_pos(const ANCH_WHICH /*a*/, D2D1_POINT_2F& pos) const noexcept;
		// �n�_�𓾂�
		virtual bool get_start_pos(D2D1_POINT_2F& val) const noexcept;
		// ���g�̐F�𓾂�.
		bool get_stroke_color(D2D1_COLOR_F& color) const noexcept;
		// �j���̔z�u�𓾂�.
		bool get_stroke_pattern(STROKE_PATTERN& array) const noexcept;
		// �j���̌`���𓾂�.
		bool get_stroke_style(D2D1_DASH_STYLE& style) const noexcept;
		// ���g�̑����𓾂�.
		bool get_stroke_width(double& width) const noexcept;
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test(const D2D1_POINT_2F /*t_pos*/, const double /*a_len*/) const noexcept;
		// �͈͂Ɋ܂܂�邩���ׂ�.
		bool in_area(const D2D1_POINT_2F /*a_min*/, const D2D1_POINT_2F /*a_max*/) const noexcept;
		// �����t���O�𒲂ׂ�.
		bool is_deleted(void) const noexcept { return m_deleted; }
		// �I���t���O�𒲂ׂ�.
		bool is_selected(void) const noexcept { return m_selected; }
		// ���������ړ�����.
		virtual	void move(const D2D1_POINT_2F d);
		// �l��I���t���O�Ɋi�[����.
		void set_select(const bool val) noexcept { m_selected = val; }
		// �l�������t���O�Ɋi�[����.
		void set_delete(const bool val) noexcept { m_deleted = val; }
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		virtual void set_start_pos(const D2D1_POINT_2F s_pos);
		// �l����g�̐F�Ɋi�[����.
		void set_stroke_color(const D2D1_COLOR_F& color) noexcept;
		// �l��j���̔z�u�Ɋi�[����.
		void set_stroke_pattern(const STROKE_PATTERN& val);
		// �l����g�̌`���Ɋi�[����.
		void set_stroke_style(const D2D1_DASH_STYLE val);
		// �l����g�̑����Ɋi�[����.
		void set_stroke_width(const double width) noexcept;
		// �}�`���쐬����.
		ShapeStroke(const ShapePanel* attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeStroke(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �����f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(const D2D1_POINT_2F barbs[], const D2D1_POINT_2F tip_pos, const ARROW_STYLE a_style, DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ����
	//------------------------------
	struct ShapeLine : ShapeStroke {
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// ���̌`��
		ARROW_SIZE m_arrow_size{};	// ���̐��@
		winrt::com_ptr<ID2D1PathGeometry> m_d2d_arrow_geometry{};	// ���� D2D �p�X�W�I���g��

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// �}�`���쐬����.
		ShapeLine(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeLine(DataReader const& dt_reader);
		// �}�`��j������.
		~ShapeLine(void);
		// �\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// �͈͂Ɋ܂܂�邩���ׂ�.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// ���̐��@�𓾂�.
		bool get_arrow_size(ARROW_SIZE& size) const noexcept;
		// ���̌`���𓾂�.
		bool get_arrow_style(ARROW_STYLE& val) const noexcept;
		// �l����̐��@�Ɋi�[����.
		void set_arrow_size(const ARROW_SIZE& size);
		// �l����̌`���Ɋi�[����.
		void set_arrow_style(const ARROW_STYLE arrow_style);
		// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
		void get_pos(const ANCH_WHICH a, D2D1_POINT_2F& pos) const noexcept;
		// �l���w�肵�����ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
		void set_pos(const D2D1_POINT_2F val, const ANCH_WHICH a);
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		void set_start_pos(const D2D1_POINT_2F pos);
		// ���������ړ�����.
		void move(const D2D1_POINT_2F d);
		// �f�[�^���[�_�[����ǂݍ���.
		void read(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ���`
	//------------------------------
	struct ShapeRect : ShapeStroke {
		D2D1_COLOR_F m_fill_color;		// �h��Ԃ�

		//------------------------------
		// shape_rect.cpp
		//------------------------------

		// �}�`���쐬����.
		ShapeRect(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* s_attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeRect(DataReader const& dt_reader);
		// �\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test_anchor(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// �͈͂Ɋ܂܂�邩���ׂ�.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �h��Ԃ��̐F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept;
		// �l��h��Ԃ��̐F�Ɋi�[����.
		void set_fill_color(const D2D1_COLOR_F& color) noexcept;
		// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
		void get_pos(const ANCH_WHICH a, D2D1_POINT_2F& pos) const noexcept;
		// �l���w�肵�����ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
		void set_pos(const D2D1_POINT_2F val, const ANCH_WHICH a);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	//	�ڐ���
	//	�쐬�������Ƃŕ�����̑����̕ύX�͂ł��Ȃ�.
	//------------------------------
	struct ShapeScale : ShapeRect {
		double m_grid_size;	// ����̑傫�� (�� -1 �����l)
		winrt::com_ptr<IDWriteTextFormat> m_dw_text_format{};	// �e�L�X�g�t�H�[�}�b�g

		//------------------------------
		// shape_scale.cpp
		//------------------------------

		//	�}�`��j������.
		~ShapeScale(void);
		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		//	�}�`���쐬����.
		ShapeScale(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeScale(DataReader const& dt_reader);
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
		ShapeElli(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* attr) :
			ShapeRect::ShapeRect(pos, vec, attr)
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
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �p�ە��`
	//------------------------------
	struct ShapeRRect : ShapeRect {
		D2D1_POINT_2F m_corner_rad;		// �p�ە����̔��a

		//------------------------------
		// shape_rrect.cpp
		// �p�ە��`
		//------------------------------

		// �}�`���쐬����.
		ShapeRRect(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeRRect(DataReader const& dt_reader);
		// �}�`��\������.
		void draw(SHAPE_DX& dx);
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// �p�۔��a�𓾂�.
		bool get_corner_radius(D2D1_POINT_2F& val) const noexcept;
		// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
		void get_pos(const ANCH_WHICH a, D2D1_POINT_2F& pos) const noexcept;
		// �l���w�肵�����ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
		void set_pos(const D2D1_POINT_2F val, const ANCH_WHICH a);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �܂���̂ЂȌ^
	//------------------------------
	struct ShapePoly : ShapeStroke {
		D2D1_POINT_2F m_vec_1{ 0.0f, 0.0f };	// 3 �Ԗڂ̒��_�������x�N�g��
		D2D1_POINT_2F m_vec_2{ 0.0f, 0.0f };	// 4 �Ԗڂ̒��_�������x�N�g��
		winrt::com_ptr<ID2D1PathGeometry> m_poly_geom{};	// �l�ӌ`�̃p�X�W�I���g��

		//------------------------------
		// shape_stroke.cpp
		// �܂���̂ЂȌ^
		//------------------------------

		// �p�X�W�I���g�����쐬����.
		virtual void create_path_geometry(void) {}
		// �}�`���쐬����.
		ShapePoly(const ShapePanel* attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapePoly(DataReader const& dt_reader);
		// �}�`��j������.
		~ShapePoly(void);
		// �}�`���͂ޕ��`�̍���_�𓾂�.
		void get_min_pos(D2D1_POINT_2F& val) const noexcept;
		// �}�`���͂ޕ��`�𓾂�.
		void get_bound(D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept;
		// ���������ړ�����.
		void move(const D2D1_POINT_2F vec);
		// �w�肳�ꂽ���ʂ̈ʒu�𓾂�.
		void get_pos(const ANCH_WHICH a, D2D1_POINT_2F& pos) const noexcept;
		// �l���w�肵�����ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
		void set_pos(const D2D1_POINT_2F pos, const ANCH_WHICH a);
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		void set_start_pos(const D2D1_POINT_2F pos);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// �l�ւ�`
	//------------------------------
	struct ShapeQuad : ShapePoly {
		D2D1_COLOR_F m_fill_color;

		//------------------------------
		// shape_quad.cpp
		//------------------------------

		// �p�X�W�I���g�����쐬����.
		void create_path_geometry(void);
		// �\������
		void draw(SHAPE_DX& dx);
		// �h��Ԃ��F�𓾂�.
		bool get_fill_color(D2D1_COLOR_F& val) const noexcept;
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// �͈͂Ɋ܂܂�邩���ׂ�.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �l��h��Ԃ��F�Ɋi�[����.
		void set_fill_color(const D2D1_COLOR_F& color) noexcept;
		// �}�`���쐬����.
		ShapeQuad(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeQuad(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& /*dt_writer*/) const;
		// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
		void write_svg(DataWriter const& /*dt_writer*/) const;
	};

	//------------------------------
	// �Ȑ�
	//------------------------------
	struct ShapeBezi : ShapePoly {
		ARROW_STYLE m_arrow_style = ARROW_STYLE::NONE;	// ���̌`��
		ARROW_SIZE m_arrow_size{};	// ���̐��@
		winrt::com_ptr<ID2D1PathGeometry> m_arrow_geom{};	// ���� D2D �p�X�W�I���g��

		~ShapeBezi(void)
		{
			m_arrow_geom = nullptr;
		}

		//------------------------------
		// shape_bezi.cpp
		// �x�W�F�Ȑ�
		//------------------------------

		// �p�X�W�I���g�����쐬����.
		void create_path_geometry(void);
		// �\������.
		void draw(SHAPE_DX& dx);
		// ���̐��@�𓾂�
		bool get_arrow_size(ARROW_SIZE& val) const noexcept;
		// ���̌`���𓾂�.
		bool get_arrow_style(ARROW_STYLE& val) const noexcept;
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// �͈͂Ɋ܂܂�邩���ׂ�.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �l����̐��@�Ɋi�[����.
		void set_arrow_size(const ARROW_SIZE& val);
		// �l����̌`���Ɋi�[����.
		void set_arrow_style(const ARROW_STYLE val);
		// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
		void set_start_pos(const D2D1_POINT_2F pos);
		// �}�`���쐬����.
		ShapeBezi(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, const ShapePanel* attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeBezi(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

	//------------------------------
	// ������
	//------------------------------
	struct ShapeText : ShapeRect {
		static wchar_t** s_available_fonts;		// �L���ȏ��̖�

		DWRITE_TEXT_RANGE m_sel_range{ 0, 0 };	// �����͈�
		D2D1_COLOR_F m_font_color = S_BLACK;	// ���̂̐F
		wchar_t* m_font_family = nullptr;	// ���̖�
		double m_font_size = 0.0;	// ���̂̑傫��
		DWRITE_FONT_STRETCH m_font_stretch = DWRITE_FONT_STRETCH_UNDEFINED;	// ���̂̐L�k
		DWRITE_FONT_STYLE m_font_style = DWRITE_FONT_STYLE_NORMAL;	// ���̂̎���
		DWRITE_FONT_WEIGHT m_font_weight = DWRITE_FONT_WEIGHT_NORMAL;	// ���̂̑���
		double m_text_line = 0.0;	// �s�� (DIPs 96dpi�Œ�)
		wchar_t* m_text = nullptr;	// ������
		DWRITE_PARAGRAPH_ALIGNMENT m_text_align_p = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;	// �i�����낦
		DWRITE_TEXT_ALIGNMENT m_text_align_t = DWRITE_TEXT_ALIGNMENT_LEADING;	// ��������
		D2D1_SIZE_F m_text_mar{ 4.0f, 4.0f };	// ������̂܂��̏㉺�ƍ��E�̗]��

		winrt::com_ptr<IDWriteTextLayout> m_dw_text_layout{};	// �������\�����邽�߂̃��C�A�E�g
		DWRITE_HIT_TEST_METRICS* m_dw_test_metrics = nullptr;	// ������̌v��
		DWRITE_LINE_METRICS* m_dw_line_metrics = nullptr;	// ������̌v��
		UINT32 m_dw_linecnt = 0;	// ������̌v�ʂ̗v�f��
		double m_dw_descent = 0.0f;
		DWRITE_HIT_TEST_METRICS* m_dw_range_metrics = nullptr;	// �����͈͂̌v��
		UINT32 m_dw_range_linecnt = 0;	// �����͈͂̌v�ʂ̗v�f��

		// �}�`��j������.
		~ShapeText(void);
		// �e�L�X�g���C�A�E�g����v�ʂ̔z��𓾂�.
		void create_test_metrics(void);
		// �e�L�X�g���C�A�E�g��j�����č쐬����.
		void create_text_layout(void);
		// �v�ʂ�j�����č쐬����.
		void create_text_metrics(void);
		// �����̋󔒂���菜��.
		void delete_bottom_blank(void) noexcept;
		// �\������.
		void draw(SHAPE_DX& dx);
		// �L���ȏ��̖�����v�f�𓾂�.
		static wchar_t* get_available_font(const uint32_t i);
		// ���̂̐F�𓾂�.
		bool get_font_color(D2D1_COLOR_F& val) const noexcept;
		// ���̖��𓾂�.
		bool get_font_family(wchar_t*& val) const noexcept;
		// ���̂̑傫���𓾂�.
		bool get_font_size(double& val) const noexcept;
		// ���̂̐L�k�𓾂�.
		bool get_font_stretch(DWRITE_FONT_STRETCH& val) const noexcept;
		// ���̂̎��̂𓾂�.
		bool get_font_style(DWRITE_FONT_STYLE& val) const noexcept;
		// �s�Ԃ𓾂�.
		bool get_text_line_height(double& val) const noexcept;
		// ���̂̑����𓾂�.
		bool get_font_weight(DWRITE_FONT_WEIGHT& val) const noexcept;
		// ������̗]���𓾂�.
		bool get_text_margin(D2D1_SIZE_F& val) const noexcept;
		// �i���̂��낦�𓾂�.
		bool get_text_align_p(DWRITE_PARAGRAPH_ALIGNMENT& val) const noexcept;
		// ������𓾂�.
		bool get_text(wchar_t*& val) const noexcept;
		// ������̂��낦�𓾂�.
		bool get_text_align_t(DWRITE_TEXT_ALIGNMENT& val) const noexcept;
		// �����͈͂𓾂�.
		bool get_text_range(DWRITE_TEXT_RANGE& val) const noexcept;
		// �ʒu���܂ނ����ׂ�.
		ANCH_WHICH hit_test(const D2D1_POINT_2F t_pos, const double a_len) const noexcept;
		// �͈͂Ɋ܂܂�邩���ׂ�.
		bool in_area(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max) const noexcept;
		// �L���ȏ��̖�������, �L���Ȃ�, ���Ƃ��Ƃ̏��̖���j����, �L���ȏ��̖��v�f�ւ̃|�C���^�[�ƒu��������.
		static bool is_available_font(wchar_t*& font);
		//	�L���ȏ��̖��̔z���j������.
		static void release_available_fonts(void);
		// �L���ȏ��̖����擾����.
		static void set_available_fonts(void);
		// �l�����̂̐F�Ɋi�[����.
		void set_font_color(const D2D1_COLOR_F& val) noexcept;
		// �l�����̖��Ɋi�[����.
		void set_font_family(wchar_t* const val);
		// �l�����̂̑傫���Ɋi�[����.
		void set_font_size(const double val);
		// �l�����̂̐L�k�Ɋi�[����.
		void set_font_stretch(const DWRITE_FONT_STRETCH val);
		// �l�����̂̎��̂Ɋi�[����.
		void set_font_style(const DWRITE_FONT_STYLE val);
		// �l�����̂̑����Ɋi�[����.
		void set_font_weight(const DWRITE_FONT_WEIGHT val);
		// �l���s�ԂɊi�[����.
		void set_text_line_height(const double val);
		// �l�𕶎���̗]���Ɋi�[����.
		void set_text_margin(const D2D1_SIZE_F val);
		// �l��i���̂��낦�Ɋi�[����.
		void set_text_align_p(const DWRITE_PARAGRAPH_ALIGNMENT val);
		// �l���w�肵�����ʂ̈ʒu�Ɋi�[����. ���̕��ʂ̈ʒu�͓����Ȃ�. 
		void set_pos(const D2D1_POINT_2F val, const ANCH_WHICH a);
		// �l�𕶎���Ɋi�[����.
		void set_text(wchar_t* const text);
		// �l�𕶎���̂��낦�Ɋi�[����.
		void set_text_align_t(const DWRITE_TEXT_ALIGNMENT val);
		// �l�𕶎��͈͂Ɋi�[����.
		void set_text_range(const DWRITE_TEXT_RANGE val);
		// �}�`���쐬����.
		ShapeText(const D2D1_POINT_2F pos, const D2D1_POINT_2F vec, wchar_t* const text, const ShapePanel* attr);
		// �}�`���f�[�^���[�_�[����ǂݍ���.
		ShapeText(DataReader const& dt_reader);
		// �f�[�^���C�^�[�ɏ�������.
		void write(DataWriter const& dt_writer) const;
		// �f�[�^���C�^�[�� SVG �^�O�Ƃ��ď�������.
		void write_svg(DataWriter const& dt_writer) const;
	};

}