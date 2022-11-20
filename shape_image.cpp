#include <time.h>
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Foundation::IAsyncAction;
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapBufferAccessMode;
	using winrt::Windows::Graphics::Imaging::BitmapEncoder;
	using winrt::Windows::Graphics::Imaging::SoftwareBitmap;
	using winrt::Windows::Storage::Streams::DataReader;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::Storage::Streams::IRandomAccessStream;
	using winrt::Windows::Storage::Streams::RandomAccessStreamReference;

	// D2D1_RECT_F ���� D2D1_RECT_U ���쐬����.
	//static const D2D1_RECT_U image_conv_rect(const D2D1_RECT_F& f, const D2D1_SIZE_U s);
	// �_�ɍł��߂�, ������̓_�����߂�.
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& q) noexcept;

	// �_�ɍł��߂�, ������̓_�����߂�.
	// p	�_
	// a	�����̒[�_
	// b	�����̂�������̒[�_
	// q	������̓_
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, D2D1_POINT_2F& q) noexcept
	{
		// ���� ab �����������肷��.
		if (a.x == b.x) {
			q.x = a.x;
			q.y = p.y;
		}
		// ���� ab �����������肷��.
		else if (a.y == b.y) {
			q.x = p.x;
			q.y = a.y;
		}
		else {
			const double ap_x = static_cast<double>(p.x) - static_cast<double>(a.x);
			const double ap_y = static_cast<double>(p.y) - static_cast<double>(a.y);
			const double ab_x = static_cast<double>(b.x) - static_cast<double>(a.x);
			const double ab_y = static_cast<double>(b.y) - static_cast<double>(a.y);
			const double ap_ab = ap_x * ab_x + ap_y * ab_y;	// ap.ab
			const double ab_ab = ab_x * ab_x + ab_y * ab_y;	// ab.ab
			pt_mul_add(D2D1_POINT_2F{ static_cast<FLOAT>(ab_x), static_cast<FLOAT>(ab_y) }, ap_ab / ab_ab, a, q);	// a + ap.ab / (|ab|^2) ab -> q
		}
	}

	// �}�`��j������.
	ShapeImage::~ShapeImage(void)
	{
		if (m_data != nullptr) {
			delete m_data;
			m_data = nullptr;
		}
		if (m_d2d_bitmap != nullptr) {
			//m_d2d_bitmap->Release();
			m_d2d_bitmap = nullptr;
		}
	}

	// �X�g���[���Ɋi�[����.
	// enc_id	�摜�̌`�� (BitmapEncoder �ɒ�`����Ă���)
	// ra_stream	�摜���i�[����X�g���[��
	IAsyncAction ShapeImage::copy_to(const winrt::guid enc_id, IRandomAccessStream& ra_stream)
	{
		// SoftwareBitmap ���쐬����.
		SoftwareBitmap bmp{ SoftwareBitmap(BitmapPixelFormat::Bgra8,  m_orig.width, m_orig.height, BitmapAlphaMode::Straight) };

		// �r�b�g�}�b�v�̃o�b�t�@�����b�N����.
		auto bmp_buf{ bmp.LockBuffer(BitmapBufferAccessMode::ReadWrite) };
		auto bmp_ref{ bmp_buf.CreateReference() };
		winrt::com_ptr<IMemoryBufferByteAccess> bmp_mem = bmp_ref.as<IMemoryBufferByteAccess>();

		// ���b�N���ꂽ�o�b�t�@�[�𓾂�.
		BYTE* bmp_data = nullptr;
		UINT32 capacity = 0;
		if (SUCCEEDED(bmp_mem->GetBuffer(&bmp_data, &capacity)))
		{
			// �o�b�t�@�ɉ摜�f�[�^���R�s�[����.
			memcpy(bmp_data, m_data, capacity);

			// �o�b�t�@���������.
			bmp_buf.Close();
			bmp_buf = nullptr;
			bmp_mem->Release();
			bmp_mem = nullptr;

			// �r�b�g�}�b�v�G���R�[�_�[�Ƀr�b�g�}�b�v���i�[����.
			BitmapEncoder bmp_enc{ co_await BitmapEncoder::CreateAsync(enc_id, ra_stream) };
			// �V�����T���l�C���������I�ɐ�������悤 true ���i�[����.
			bmp_enc.IsThumbnailGenerated(true);
			bmp_enc.SetSoftwareBitmap(bmp);
			try {
				co_await bmp_enc.FlushAsync();
			}
			catch (winrt::hresult_error& err) {
				// �T���l�C���̎����������o���Ȃ��Ȃ�, false ���i�[����.
				if (err.code() == WINCODEC_ERR_UNSUPPORTEDOPERATION) {
					bmp_enc.IsThumbnailGenerated(false);
				}
			}
			if (!bmp_enc.IsThumbnailGenerated()) {
				// �ēx��蒼��.
				co_await bmp_enc.FlushAsync();
			}
			bmp_enc = nullptr;
		}
		bmp.Close();
		bmp = nullptr;
	}

	// �}�`��\������.
	// sh	�\������p��
	void ShapeImage::draw(ShapeSheet const& sh)
	{
		const D2D_UI& d2d = sh.m_d2d;
		if (m_d2d_bitmap == nullptr) {
			const D2D1_BITMAP_PROPERTIES1 b_prop{
				D2D1::BitmapProperties1(
					D2D1_BITMAP_OPTIONS_NONE,
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
				)
			};
			const UINT32 pitch = 4 * m_orig.width;
			winrt::check_hresult(d2d.m_d2d_context->CreateBitmap(m_orig, static_cast<void*>(m_data), pitch, b_prop, m_d2d_bitmap.put()));
			if (m_d2d_bitmap == nullptr) {
				return;
			}
		}
		const D2D1_RECT_F dest_rect{
			m_pos.x,
			m_pos.y,
			m_pos.x + m_view.width,
			m_pos.y + m_view.height
		};
		d2d.m_d2d_context->DrawBitmap(m_d2d_bitmap.get(), dest_rect, m_opac, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, m_clip);

		if (is_selected()) {
			sh.m_color_brush->SetColor(Shape::s_background_color);
			d2d.m_d2d_context->DrawRectangle(dest_rect, sh.m_color_brush.get(), 1.0f, nullptr);
			sh.m_color_brush->SetColor(Shape::s_foreground_color);
			d2d.m_d2d_context->DrawRectangle(dest_rect, sh.m_color_brush.get(), 1.0f, Shape::m_aux_style.get());

			const D2D1_POINT_2F v_pos[4]{
				m_pos,
				{ m_pos.x + m_view.width, m_pos.y },
				{ m_pos.x + m_view.width, m_pos.y + m_view.height },
				{ m_pos.x, m_pos.y + m_view.height },
			};

			anc_draw_rect(v_pos[0], sh);
			anc_draw_rect(v_pos[1], sh);
			anc_draw_rect(v_pos[2], sh);
			anc_draw_rect(v_pos[3], sh);
		}
	}

	// �}�`���͂ޗ̈�𓾂�.
	void ShapeImage::get_bound(const D2D1_POINT_2F a_min, const D2D1_POINT_2F a_max, D2D1_POINT_2F& b_min, D2D1_POINT_2F& b_max) const noexcept
	{
		D2D1_POINT_2F b_pos[2]{
			m_pos,
			{ m_pos.x + m_view.width, m_pos.y + m_view.height }
		};
		//pt_bound(b_pos[0], b_pos[1], b_pos[0], b_pos[1]);
		if (b_pos[0].x > b_pos[1].x) {
			const auto less_x = b_pos[1].x;
			b_pos[1].x = b_pos[0].x;
			b_pos[0].x = less_x;
		}
		if (b_pos[0].y > b_pos[1].y) {
			const auto less_y = b_pos[1].y;
			b_pos[1].y = b_pos[0].y;
			b_pos[0].y = less_y;
		}
		//pt_min(a_min, b_pos[0], b_min);
		b_min.x = a_min.x < b_pos[0].x ? a_min.x : b_pos[0].x;
		b_min.y = a_min.y < b_pos[0].y ? a_min.y : b_pos[0].y;
		//pt_max(a_max, b_pos[1], b_max);
		b_max.x = a_max.x > b_pos[1].x ? a_max.x : b_pos[1].x;
		b_max.y = a_max.y > b_pos[1].y ? a_max.y : b_pos[1].y;
	}

	// �摜�̕s�����x�𓾂�.
	bool ShapeImage::get_image_opacity(float& val) const noexcept
	{
		val = m_opac;
		return true;
	}

	// ���ʂ̈ʒu�𓾂�.
	void ShapeImage::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		if (anc == ANC_TYPE::ANC_NW) {
			val = m_pos;
		}
		else if (anc == ANC_TYPE::ANC_NORTH) {
			val.x = m_pos.x + m_view.width * 0.5f;
			val.y = m_pos.y;
		}
		else if (anc == ANC_TYPE::ANC_NE) {
			val.x = m_pos.x + m_view.width;
			val.y = m_pos.y;
		}
		else if (anc == ANC_TYPE::ANC_EAST) {
			val.x = m_pos.x + m_view.width;
			val.y = m_pos.y + m_view.height * 0.5f;
		}
		else if (anc == ANC_TYPE::ANC_SE) {
			val.x = m_pos.x + m_view.width;
			val.y = m_pos.y + m_view.height;
		}
		else if (anc == ANC_TYPE::ANC_SOUTH) {
			val.x = m_pos.x + m_view.width * 0.5f;
			val.y = m_pos.y + m_view.height;
		}
		else if (anc == ANC_TYPE::ANC_SW) {
			val.x = m_pos.x;
			val.y = m_pos.y + m_view.height;
		}
		else if (anc == ANC_TYPE::ANC_WEST) {
			val.x = m_pos.x;
			val.y = m_pos.y + m_view.height * 0.5f;
		}
	}

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	void ShapeImage::get_pos_min(D2D1_POINT_2F& val) const noexcept
	{
		const float ax = m_pos.x;
		const float ay = m_pos.y;
		const float bx = m_pos.x + m_view.width;
		const float by = m_pos.y + m_view.height;
		val.x = ax < bx ? ax : bx;
		val.y = ay < by ? ay : by;
	}

	// �ߖT�̒��_��������.
	// pos	����ʒu
	// dd	�ߖT�Ƃ݂Ȃ����� (�̓��l), �����藣�ꂽ���_�͋ߖT�Ƃ݂͂Ȃ��Ȃ�.
	// val	����ʒu�̋ߖT�ɂ��钸�_
	// �߂�l	���������� true
	bool ShapeImage::get_pos_nearest(const D2D1_POINT_2F pos, float& dd, D2D1_POINT_2F& val) const noexcept
	{
		bool found = false;
		D2D1_POINT_2F v_pos[4];
		get_verts(v_pos);
		for (size_t i = 0; i < 4; i++) {
			D2D1_POINT_2F vec;
			pt_sub(pos, v_pos[i], vec);
			const float vv = static_cast<float>(pt_abs2(vec));
			if (vv < dd) {
				dd = vv;
				val = v_pos[i];
				if (!found) {
					found = true;
				}
			}
		}
		return found;
	}

	// �J�n�ʒu�𓾂�.
	bool ShapeImage::get_pos_start(D2D1_POINT_2F& val) const noexcept
	{
		val = m_pos;
		return true;
	}

	// ���_�𓾂�.
	size_t ShapeImage::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_pos;
		v_pos[1] = D2D1_POINT_2F{ m_pos.x + m_view.width, m_pos.y };
		v_pos[2] = D2D1_POINT_2F{ m_pos.x + m_view.width, m_pos.y + m_view.height };
		v_pos[3] = D2D1_POINT_2F{ m_pos.x, m_pos.y + m_view.height };
		return 4;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���. �܂܂Ȃ��Ƃ��́u�}�`�̊O���v��Ԃ�.
	uint32_t ShapeImage::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		D2D1_POINT_2F v_pos[4];
		get_verts(v_pos);
		if (pt_in_anc(t_pos, v_pos[0])) {
			return ANC_TYPE::ANC_NW;
		}
		else if (pt_in_anc(t_pos, v_pos[1])) {
			return ANC_TYPE::ANC_NE;
		}
		else if (pt_in_anc(t_pos, v_pos[2])) {
			return ANC_TYPE::ANC_SE;
		}
		else if (pt_in_anc(t_pos, v_pos[3])) {
			return ANC_TYPE::ANC_SW;
		}
		else {
			const auto e_width = Shape::s_anc_len * 0.5f;
			D2D1_POINT_2F e_pos[2];
			e_pos[0].x = v_pos[0].x;
			e_pos[0].y = v_pos[0].y - e_width;
			e_pos[1].x = v_pos[1].x;
			e_pos[1].y = v_pos[1].y + e_width;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_NORTH;
			}
			e_pos[0].x = v_pos[1].x - e_width;
			e_pos[0].y = v_pos[1].y;
			e_pos[1].x = v_pos[2].x + e_width;
			e_pos[1].y = v_pos[2].y;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_EAST;
			}
			e_pos[0].x = v_pos[3].x;
			e_pos[0].y = v_pos[3].y - e_width;
			e_pos[1].x = v_pos[2].x;
			e_pos[1].y = v_pos[2].y + e_width;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_SOUTH;
			}
			e_pos[0].x = v_pos[0].x - e_width;
			e_pos[0].y = v_pos[0].y;
			e_pos[1].x = v_pos[3].x + e_width;
			e_pos[1].y = v_pos[3].y;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_WEST;
			}
		}
		//pt_bound(v_pos[0], v_pos[2], v_pos[0], v_pos[2]);
		if (v_pos[0].x > v_pos[2].x) {
			const float less_x = v_pos[2].x;
			v_pos[2].x = v_pos[0].x;
			v_pos[0].x = less_x;
		}
		if (v_pos[0].y > v_pos[2].y) {
			const float less_y = v_pos[2].y;
			v_pos[2].y = v_pos[0].y;
			v_pos[0].y = less_y;
		}
		if (v_pos[0].x <= t_pos.x && t_pos.x <= v_pos[2].x &&
			v_pos[0].y <= t_pos.y && t_pos.y <= v_pos[2].y) {
			return ANC_TYPE::ANC_FILL;
		}
		return ANC_TYPE::ANC_SHEET;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// area_min	�͈͂̍���ʒu
	// area_max	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeImage::in_area(const D2D1_POINT_2F area_min, const D2D1_POINT_2F area_max) const noexcept
	{
		// �n�_�ƏI�_�Ƃ��͈͂Ɋ܂܂�邩���肷��.
		return pt_in_rect(m_pos, area_min, area_max) &&
			pt_in_rect(D2D1_POINT_2F{ m_pos.x + m_view.width, m_pos.y + m_view.height }, area_min, area_max);
	}

	// ���������ړ�����.
	bool ShapeImage::move(const D2D1_POINT_2F val) noexcept
	{
		pt_add(m_pos, val, m_pos);
		return true;
	}

	// ���̉摜�ɖ߂�.
	void ShapeImage::revert(void) noexcept
	{
		const float src_w = static_cast<float>(m_orig.width);
		const float src_h = static_cast<float>(m_orig.height);
		m_view.width = src_w;
		m_view.height = src_h;
		m_clip.left = m_clip.top = 0.0f;
		m_clip.right = src_w;
		m_clip.bottom = src_h;
		m_ratio.width = m_ratio.height = 1.0f;
		m_opac = 1.0f;
	}

	// �l���摜�̕s�����x�Ɋi�[����.
	bool ShapeImage::set_image_opacity(const float val) noexcept
	{
		if (!equal(m_opac, val)) {
			m_opac = val;
			return true;
		}
		return false;
	}

	// �l��, ���ʂ̈ʒu�Ɋi�[����.
	// val	�l
	// anc	�}�`�̕���
	// limit	���̒��_�Ƃ̌��E���� (���̒l�����ɂȂ�Ȃ�, �}�`�̕��ʂ��w�肷�钸�_�𑼂̒��_�Ɉʒu�ɍ��킹��)
	// keep_aspect	�摜�}�`�̏c������ێ�
	bool ShapeImage::set_pos_anc(const D2D1_POINT_2F val, const uint32_t anc, const float /*limit*/, const bool keep_aspect) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_round(val, PT_ROUND, new_pos);
		bool flag = false;
		if (anc == ANC_TYPE::ANC_NW) {
			// �摜�̈ꕔ���ł��\������Ă��邩���肷��.
			// (�摜���܂������\������ĂȂ��ꍇ�̓X�P�[���̕ύX�͍s��Ȃ�.)
			const float image_w = m_clip.right - m_clip.left;	// �\������Ă���摜�̕� (����)
			const float image_h = m_clip.bottom - m_clip.top;	// �\������Ă���摜�̍��� (����)
			if (image_w > 1.0f && image_h > 1.0f) {
				const D2D1_POINT_2F s_pos{ m_pos };	// �n�_ (�}�`�̒��_)
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x + image_w, s_pos.y + image_h };	// �I�_ (�n�_�ƑΊp�ɂ���摜��̓_)
					image_get_pos_on_line(new_pos, s_pos, e_pos, pos);
				}
				else {
					pos = new_pos;
				}
				// �l�Ǝn�_�Ƃ̍���������, �������[�����傫�������肷��.
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					// �X�P�[���ύX��̕\���̐��@������, ���̏c���� 1 �s�N�Z���ȏ゠�邩���肷��.
					const float view_w = m_view.width - v_vec.x;
					const float view_h = m_view.height - v_vec.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view.width = view_w;
						m_view.height = view_h;
						m_pos = pos;
						m_ratio.width = view_w / image_w;
						m_ratio.height = view_h / image_h;
						if (!flag) {
							flag = true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_NE) {
			const float image_w = m_clip.right - m_clip.left;
			const float image_h = m_clip.bottom - m_clip.top;
			if (image_w > 1.0f && image_h > 1.0f) {
				const D2D1_POINT_2F s_pos{ m_pos.x + m_view.width, m_pos.y };
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x - image_w, s_pos.y + image_h };
					image_get_pos_on_line(new_pos, s_pos, e_pos, pos);
				}
				else {
					pos = new_pos;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float view_w = pos.x - m_pos.x;
					const float view_h = m_pos.y + m_view.height - pos.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view.width = view_w;
						m_view.height = view_h;
						m_pos.y = pos.y;
						m_ratio.width = view_w / image_w;
						m_ratio.height = view_h / image_h;
						if (!flag) {
							flag = true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_SE) {
			const float image_w = m_clip.right - m_clip.left;
			const float image_h = m_clip.bottom - m_clip.top;
			if (image_w > 1.0f && image_h > 1.0f) {
				const D2D1_POINT_2F s_pos{ m_pos.x + m_view.width, m_pos.y + m_view.height };
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x - image_w, s_pos.y - image_h };
					image_get_pos_on_line(new_pos, s_pos, e_pos, pos);
				}
				else {
					pos = new_pos;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float view_w = pos.x - m_pos.x;
					const float view_h = pos.y - m_pos.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view.width = view_w;
						m_view.height = view_h;
						m_ratio.width = view_w / image_w;
						m_ratio.height = view_h / image_h;
						if (!flag) {
							flag = true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_SW) {
			const float image_w = m_clip.right - m_clip.left;
			const float image_h = m_clip.bottom - m_clip.top;
			if (image_w > 1.0f && image_h > 1.0f) {
				const D2D1_POINT_2F s_pos{ m_pos.x, m_pos.y + m_view.height };
				D2D1_POINT_2F pos;
				if (keep_aspect) {
					const D2D1_POINT_2F e_pos{ s_pos.x + image_w, s_pos.y - image_h };
					image_get_pos_on_line(new_pos, s_pos, e_pos, pos);
				}
				else {
					pos = new_pos;
				}
				D2D1_POINT_2F v_vec;
				pt_sub(pos, s_pos, v_vec);
				if (pt_abs2(v_vec) >= FLT_MIN) {
					const float view_w = m_pos.x + m_view.width - pos.x;
					const float view_h = pos.y - m_pos.y;
					if (view_w >= 1.0f && view_h >= 1.0f) {
						m_view.width = view_w;
						m_view.height = view_h;
						m_pos.x = pos.x;
						m_ratio.width = view_w / image_w;
						m_ratio.height = view_h / image_h;
						if (!flag) {
							flag = true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_NORTH) {
			// �ύX���鍷�������߂�.
			const float dy = (new_pos.y - m_pos.y);
			if (fabs(dy) >= FLT_MIN) {
				const float rect_top = min(m_clip.top + dy / m_ratio.height, m_clip.bottom - 1.0f);
				m_clip.top = max(rect_top, 0.0f);
				m_view.height = (m_clip.bottom - m_clip.top) * m_ratio.height;
				m_pos.y = new_pos.y;
				if (!flag) {
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_EAST) {
			const float dx = (new_pos.x - (m_pos.x + m_view.width));
			if (fabs(dx) >= FLT_MIN) {
				const float rect_right = max(m_clip.right + dx / m_ratio.width, m_clip.left + 1.0f);
				m_clip.right = min(rect_right, m_orig.width);
				m_view.width = (m_clip.right - m_clip.left) * m_ratio.width;
				m_pos.x = new_pos.x - m_view.width;
				if (!flag) {
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_SOUTH) {
			const float dy = (new_pos.y - (m_pos.y + m_view.height));
			if (fabs(dy) >= FLT_MIN) {
				const float rect_bottom = max(m_clip.bottom + dy / m_ratio.height, m_clip.top + 1.0f);
				m_clip.bottom = min(rect_bottom, m_orig.height);
				m_view.height = (m_clip.bottom - m_clip.top) * m_ratio.height;
				m_pos.y = new_pos.y - m_view.height;
				if (!flag) {
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_WEST) {
			const float dx = (new_pos.x - m_pos.x);
			if (fabs(dx) >= FLT_MIN) {
				const float rect_left = min(m_clip.left + dx / m_ratio.width, m_clip.right - 1.0f);
				m_clip.left = max(rect_left, 0.0f);
				m_view.width = (m_clip.right - m_clip.left) * m_ratio.width;
				m_pos.x = new_pos.x;
				if (!flag) {
					flag = true;
				}
			}
		}
		return flag;
	}

	// �l���n�_�Ɋi�[����. ���̕��ʂ̈ʒu������.
	bool ShapeImage::set_pos_start(const D2D1_POINT_2F val) noexcept
	{
		D2D1_POINT_2F new_pos;
		pt_round(val, PT_ROUND, new_pos);
		if (!equal(m_pos, new_pos)) {
			m_pos = new_pos;
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// pos	����ʒu
	// view_size	�\�������傫��
	// bmp	�r�b�g�}�b�v
	// opac	�s�����x
	ShapeImage::ShapeImage(const D2D1_POINT_2F pos, const D2D1_SIZE_F view_size, const SoftwareBitmap& bmp, const float opac) :
		ShapeSelect()
	{
		const uint32_t image_w = bmp.PixelWidth();
		const uint32_t image_h = bmp.PixelHeight();

		m_orig.width = image_w;
		m_orig.height = image_h;
		m_pos = pos;
		m_view = view_size;
		m_clip.left = m_clip.top = 0;
		m_clip.right = static_cast<FLOAT>(image_w);
		m_clip.bottom = static_cast<FLOAT>(image_h);
		m_opac = opac < 0.0f ? 0.0f : (opac > 1.0f ? 1.0f : opac);
		const size_t data_size = 4ull * image_w * image_h;
		if (data_size > 0) {
			m_data = new uint8_t[data_size];
			// SoftwareBitmap �̃o�b�t�@�����b�N����.
			auto image_buf{ bmp.LockBuffer(BitmapBufferAccessMode::ReadWrite) };
			auto image_ref{ image_buf.CreateReference() };
			winrt::com_ptr<IMemoryBufferByteAccess> image_mem = image_ref.as<IMemoryBufferByteAccess>();
			BYTE* image_data = nullptr;
			UINT32 capacity = 0;
			if (SUCCEEDED(image_mem->GetBuffer(&image_data, &capacity))) {
				// ���b�N�����o�b�t�@�ɉ摜�f�[�^���R�s�[����.
				memcpy(m_data, image_data, capacity);
				// ���b�N�����o�b�t�@���������.
				image_buf.Close();
				image_buf = nullptr;
				image_mem->Release();
				image_mem = nullptr;
			}
		}
		else {
			m_data = nullptr;
		}
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���
	// dt_reader	�f�[�^���[�_�[
	ShapeImage::ShapeImage(DataReader const& dt_reader) :
		ShapeSelect(dt_reader),
		m_pos(D2D1_POINT_2F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_view(D2D1_SIZE_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_clip(D2D1_RECT_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_orig(D2D1_SIZE_U{ dt_reader.ReadUInt32(), dt_reader.ReadUInt32() }),
		m_ratio(D2D1_SIZE_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_opac(dt_reader.ReadSingle())
	{
		const size_t pitch = 4ull * m_orig.width;
		m_data = new uint8_t[pitch * m_orig.height];
		std::vector<uint8_t> line_buf(pitch);
		const size_t height = m_orig.height;
		for (size_t i = 0; i < height; i++) {
			dt_reader.ReadBytes(line_buf);
			memcpy(m_data + pitch * i, line_buf.data(), pitch);
		}
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	// dt_writer	�f�[�^���C�^�[
	void ShapeImage::write(DataWriter const& dt_writer) const
	{
		dt_writer.WriteBoolean(m_is_deleted);
		dt_writer.WriteBoolean(m_is_selected);
		dt_write(m_pos, dt_writer);
		dt_write(m_view, dt_writer);
		dt_write(m_clip, dt_writer);
		dt_write(m_orig, dt_writer);
		dt_write(m_ratio, dt_writer);
		dt_writer.WriteSingle(m_opac);

		const size_t pitch = 4ull * m_orig.width;
		std::vector<uint8_t> line_buf(pitch);
		const size_t height = m_orig.height;
		for (size_t i = 0; i < height; i++) {
			memcpy(line_buf.data(), m_data + pitch * i, pitch);
			dt_writer.WriteBytes(line_buf);
		}
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	// file_name	�摜�t�@�C����
	// dt_write		�f�[�^���C�^�[
	void ShapeImage::write_svg(const wchar_t file_name[], DataWriter const& dt_writer) const
	{
		dt_write_svg("<image href=\"", dt_writer);
		dt_write_svg(file_name, wchar_len(file_name), dt_writer);
		dt_write_svg("\" ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_view.width, "width", dt_writer);
		dt_write_svg(m_view.height, "height", dt_writer);
		dt_write_svg(m_opac, "opacity", dt_writer);
		dt_write_svg("/>" SVG_NEW_LINE, dt_writer);
	}

	// �f�[�^���C�^�[�� SVG �Ƃ��ď�������.
	// �摜�Ȃ��̏ꍇ.
	// dt_write		�f�[�^���C�^�[
	void ShapeImage::write_svg(DataWriter const& dt_writer) const
	{
		constexpr char RECT[] =
			"<rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" "
			"stroke=\"black\" fill=\"white\" />" SVG_NEW_LINE;
		constexpr char TEXT[] =
			"<text x=\"50%\" y=\"50%\" text-anchor=\"middle\" dominant-baseline=\"central\">"
			"NO IMAGE</text>" SVG_NEW_LINE;
		dt_write_svg("<!--" SVG_NEW_LINE, dt_writer);
		write_svg(nullptr, dt_writer);
		dt_write_svg("-->" SVG_NEW_LINE, dt_writer);
		dt_write_svg("<svg ", dt_writer);
		dt_write_svg(m_pos, "x", "y", dt_writer);
		dt_write_svg(m_view.width, "width", dt_writer);
		dt_write_svg(m_view.height, "height", dt_writer);
		dt_write_svg("viewBox=\"0 0 ", dt_writer);
		dt_write_svg(m_view.width, dt_writer);
		dt_write_svg(m_view.height, dt_writer);
		dt_write_svg("\">" SVG_NEW_LINE, dt_writer);
		dt_write_svg(RECT, dt_writer);
		dt_write_svg(TEXT, dt_writer);
		dt_write_svg("</svg>" SVG_NEW_LINE, dt_writer);
	}

}