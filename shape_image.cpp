#include <time.h>
#include "pch.h"
#include "shape.h"

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
	using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
	using winrt::Windows::Graphics::Imaging::BitmapBufferAccessMode;
	using winrt::Windows::Graphics::Imaging::BitmapEncoder;
	using winrt::Windows::Storage::Streams::RandomAccessStreamReference;

	// �_�ɍł��߂�, ������̓_�����߂�.
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, 
		const D2D1_POINT_2F b, /*--->*/D2D1_POINT_2F& q) noexcept;

	winrt::com_ptr<IWICImagingFactory2> ShapeImage::wic_factory{	// WIC �t�@�N�g��
		[]() {
			winrt::com_ptr<IWICImagingFactory2> factory;
			winrt::check_hresult(
				CoCreateInstance(
					CLSID_WICImagingFactory,
					nullptr,
					CLSCTX_INPROC_SERVER,
					__uuidof(IWICImagingFactory2),
					factory.put_void()
					//IID_PPV_ARGS(&factory)
				)
			);
			return factory;
		}()
	};

	// �_�ɍł��߂�, ������̓_�����߂�.
	// p	�_
	// a	�����̎n�_
	// b	�����̏I�_
	// q	�ł��߂��_
	static void image_get_pos_on_line(const D2D1_POINT_2F p, const D2D1_POINT_2F a, const D2D1_POINT_2F b, /*--->*/D2D1_POINT_2F& q) noexcept
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

	// �r�b�g�}�b�v���X�g���[���Ɋi�[����.
	// C	true �Ȃ�N���b�s���O����. false �Ȃ炵�Ȃ�.
	// enc_id	����������
	// stream	�摜���i�[����X�g���[��
	// �߂�l	�i�[�ł����� true
	template <bool C>
	IAsyncOperation<bool> ShapeImage::copy(const winrt::guid enc_id, IRandomAccessStream& stream) const
	{
		// �N���b�v�{�[�h�Ɋi�[����Ȃ�, ���f�[�^�̂܂�.
		// �`�悷��Ƃ��� DrawBitmap �ŃN���b�s���O�����.
		// �G�N�X�|�[�g����Ȃ�, �N���b�s���O�����f�[�^.
		// SVG �ɂ�, �摜���N���b�s���O���ĕ\������@�\���Ȃ���, �����Ă��ώG.
		// �Ђ���Ƃ�����, �N���b�s���O��`�̏����_�̂����Ŕ����ɈقȂ�摜�ɂȂ邩������Ȃ�.
		// PDF �ɂ�, 3 �o�C�g RGB ��񋟂ł��Ȃ��̂�, �g���Ȃ�.
		bool ret = false;	// �Ԓl
		const int32_t c_left = [=]() {	// �N���b�s���O���`�̍��̒l
			if constexpr (C) {
				return static_cast<int32_t>(std::floorf(m_clip.left)); 
			}
			else {
				return 0;
			}
		}();
		const int32_t c_top = [=]() {	// �N���b�s���O���`�̏�̒l
			if constexpr (C) { 
				return static_cast<int32_t>(std::floorf(m_clip.top)); 
			}
			else {
				return 0;
			}
		}(); 
		const int32_t c_right = [=]() {	// �N���b�s���O���`�̉E�̒l
			if constexpr (C) {
				return static_cast<int32_t>(std::ceilf(m_clip.right));
			}
			else {
				return m_orig.width;
			}
		}(); 
		const int32_t c_bottom = [=]() {	// �N���b�s���O���`�̉��̒l
			if constexpr (C) {
				return static_cast<int32_t>(std::ceilf(m_clip.bottom));
			}
			else {
				return m_orig.height;
			}
		}();
		if (c_left >= 0 && c_top >= 0 && c_right > c_left && c_bottom > c_top) {
			// �o�b�N�O���E���h�ɐؑւ����Ď��s����.
			// UI �X���b�h�̂܂܂ł�, SoftwareBitmap ����������Ƃ��G���[���N����.
			winrt::apartment_context context;
			co_await winrt::resume_background();

			// SoftwareBitmap ���쐬����.
			// Windows �ł�, SoftwareBitmap �ɂ��� WIC �ɂ���
			// 3 �o�C�g�� RGB �̓T�|�[�g����ĂȂ�.
			const int32_t c_width = c_right - c_left;
			const int32_t c_height = c_bottom - c_top;
			SoftwareBitmap soft_bmp{
				SoftwareBitmap(BitmapPixelFormat::Bgra8, c_width, c_height, BitmapAlphaMode::Straight)
			};

			// �r�b�g�}�b�v�̃o�b�t�@�����b�N����.
			auto soft_bmp_buf{
				soft_bmp.LockBuffer(BitmapBufferAccessMode::ReadWrite)
			};
			// �o�b�t�@�̎Q�Ƃ𓾂�.
			auto soft_bmp_ref{
				soft_bmp_buf.CreateReference()
			};
			// �Q�Ƃ��������o�b�t�@�A�N�Z�X�ɕϊ�����.
			winrt::com_ptr<IMemoryBufferByteAccess> soft_bmp_mem{
				soft_bmp_ref.as<IMemoryBufferByteAccess>()
			};

			// �������o�b�t�@�A�N�Z�X���烍�b�N���ꂽ���o�b�t�@�[�𓾂�.
			BYTE* soft_bmp_ptr = nullptr;
			UINT32 capacity = 0;
			if (SUCCEEDED(soft_bmp_mem->GetBuffer(&soft_bmp_ptr, &capacity)))
			{
				// �}�`�̉摜�f�[�^��
				// SoftwareBitmap �̃o�b�t�@�ɃR�s�[����.
				if constexpr (C) {
					// �N���b�s���O����
					for (size_t y = c_top; y < c_bottom; y++) {
						memcpy(soft_bmp_ptr + 4ull * c_width * y, 
							m_bgra + 4ull * m_orig.width * y + c_left,
							4ull * c_width);
					}
				}
				else {
					// �N���b�s���O�Ȃ�
					memcpy(soft_bmp_ptr, m_bgra, capacity);
				}

				// �o�b�t�@���������.
				// SoftwareBitmap �����b�N���ꂽ�܂܂ɂȂ�̂�,
				// IMemoryBufferByteAccess �� Release() ����K�v������.
				soft_bmp_buf.Close();
				soft_bmp_buf = nullptr;
				soft_bmp_ref.Close();
				soft_bmp_ref = nullptr;
				soft_bmp_mem->Release();
				soft_bmp_mem = nullptr;

				// �r�b�g�}�b�v��������쐬����.
				BitmapEncoder bmp_enc{
					co_await BitmapEncoder::CreateAsync(enc_id, stream)
				};
				// �������, �V�����T���l�C���𐶐�����悤�ݒ肷��.
				//bmp_enc.IsThumbnailGenerated(true);
				// ������Ƀ\�t�g�E�F�A�r�b�g�}�b�v���i�[����.
				bmp_enc.SetSoftwareBitmap(soft_bmp);
				try {
					// ���������ꂽ�r�b�g�}�b�v���X�g���[���ɏ����o��.
					co_await bmp_enc.FlushAsync();
					ret = true;
				}
				catch (const winrt::hresult_error& e) {
					// �T���l�C���̎����������o���Ȃ��Ȃ�, false ���i�[����.
					//if (err.code() == WINCODEC_ERR_UNSUPPORTEDOPERATION) {
					//	bmp_enc.IsThumbnailGenerated(false);
					//}
					//else
					ret = e.code();
				}
				//if (!bmp_enc.IsThumbnailGenerated()) {
					// �ēx��蒼��.
				//	co_await bmp_enc.FlushAsync();
				//}
				// ��������������.
				bmp_enc = nullptr;
			}
			// �r�b�g�}�b�v��j������.
			soft_bmp.Close();
			soft_bmp = nullptr;

			// �X���b�h�R���e�L�X�g�𕜌�����.
			co_await context;
		}
		co_return ret;
	}
	template IAsyncOperation<bool> ShapeImage::copy<true>(const winrt::guid enc_id, IRandomAccessStream& stream) const;
	template IAsyncOperation<bool> ShapeImage::copy<false>(const winrt::guid enc_id, IRandomAccessStream& stream) const;

	// �}�`��\������.
	void ShapeImage::draw(void)
	{
		ID2D1RenderTarget* const target = Shape::m_d2d_target;
		ID2D1SolidColorBrush* const brush = Shape::m_d2d_color_brush.get();

		if (m_d2d_bitmap == nullptr) {
			//const D2D1_BITMAP_PROPERTIES1 b_prop{
			//	D2D1::BitmapProperties1(
			//		D2D1_BITMAP_OPTIONS_NONE,
			//		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			//	)
			//};
			const D2D1_BITMAP_PROPERTIES b_prop{
				D2D1::BitmapProperties(
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
				)
			};
			const UINT32 pitch = 4u * m_orig.width;
			winrt::com_ptr<ID2D1Bitmap> bitmap;
			target->CreateBitmap(m_orig, static_cast<void*>(m_bgra), pitch, b_prop, bitmap.put());
			m_d2d_bitmap = bitmap.as<ID2D1Bitmap1>();
			if (m_d2d_bitmap == nullptr) {
				return;
			}
		}
		const D2D1_RECT_F dest_rect{
			m_start.x,
			m_start.y,
			m_start.x + m_view.width,
			m_start.y + m_view.height
		};
		target->DrawBitmap(m_d2d_bitmap.get(), dest_rect, m_opac, D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, m_clip);

		if (m_anc_show && is_selected()) {
			const D2D1_POINT_2F v_pos[4]{
				m_start,
				{ m_start.x + m_view.width, m_start.y },
				{ m_start.x + m_view.width, m_start.y + m_view.height },
				{ m_start.x, m_start.y + m_view.height },
			};
			anc_draw_square(v_pos[0], target, brush);
			anc_draw_square(v_pos[1], target, brush);
			anc_draw_square(v_pos[2], target, brush);
			anc_draw_square(v_pos[3], target, brush);
			brush->SetColor(COLOR_WHITE);
			target->DrawRectangle(dest_rect, brush, Shape::m_aux_width, nullptr);
			brush->SetColor(COLOR_BLACK);
			target->DrawRectangle(dest_rect, brush, Shape::m_aux_width, Shape::m_aux_style.get());
		}
	}

	// �}�`���͂ޗ̈�𓾂�.
	void ShapeImage::get_bound(const D2D1_POINT_2F a_lt, const D2D1_POINT_2F a_rb, D2D1_POINT_2F& b_lt, D2D1_POINT_2F& b_rb) const noexcept
	{
		D2D1_POINT_2F b_pos[2]{
			m_start,
			{ m_start.x + m_view.width, m_start.y + m_view.height }
		};
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
		b_lt.x = a_lt.x < b_pos[0].x ? a_lt.x : b_pos[0].x;
		b_lt.y = a_lt.y < b_pos[0].y ? a_lt.y : b_pos[0].y;
		b_rb.x = a_rb.x > b_pos[1].x ? a_rb.x : b_pos[1].x;
		b_rb.y = a_rb.y > b_pos[1].y ? a_rb.y : b_pos[1].y;
	}

	// �摜�̕s�����x�𓾂�.
	bool ShapeImage::get_image_opacity(float& val) const noexcept
	{
		val = m_opac;
		return true;
	}

	// ��f�̐F�𓾂�.
	// pos	�y�[�W���W�ł̈ʒu
	// val	��f�̐F
	// �߂�l	�F�𓾂�ꂽ�Ȃ� true, �����łȂ���� false.
	bool ShapeImage::get_pixcel(const D2D1_POINT_2F pos, D2D1_COLOR_F& val) const noexcept
	{
		// �y�[�W���W�ł̈ʒu��, ���摜�ł̈ʒu�ɕϊ�����.
		const double fx = round(m_clip.left + (pos.x - m_start.x) * (m_clip.right - m_clip.left) / m_view.width);
		const double fy = round(m_clip.top + (pos.y - m_start.y) * (m_clip.bottom - m_clip.top) / m_view.height);
		// �ϊ����ꂽ�ʒu��, �摜�Ɏ��܂�Ȃ�,
		if (fx >= 0.0 && fx <= m_orig.width && fy >= 0.0 && fy <= m_orig.height) {
			// ���f�[�^�ł̉�f������̓Y�����ɕϊ�.
			const size_t i = m_orig.width * static_cast<size_t>(fy) + static_cast<size_t>(fx);
			// �F������ D2D1_COLOR_F �ɕϊ�.
			const uint8_t b = m_bgra[i * 4 + 0];
			const uint8_t g = m_bgra[i * 4 + 1];
			const uint8_t r = m_bgra[i * 4 + 2];
			const uint8_t a = m_bgra[i * 4 + 3];
			val.b = static_cast<float>(b) / 255.0f;
			val.g = static_cast<float>(g) / 255.0f;
			val.r = static_cast<float>(r) / 255.0f;
			val.a = static_cast<float>(a) / 255.0f;
			return true;
		}
		return false;
	}

	// ���ʂ̈ʒu�𓾂�.
	void ShapeImage::get_pos_anc(const uint32_t anc, D2D1_POINT_2F& val) const noexcept
	{
		if (anc == ANC_TYPE::ANC_NW) {
			val = m_start;
		}
		else if (anc == ANC_TYPE::ANC_NORTH) {
			val.x = m_start.x + m_view.width * 0.5f;
			val.y = m_start.y;
		}
		else if (anc == ANC_TYPE::ANC_NE) {
			val.x = m_start.x + m_view.width;
			val.y = m_start.y;
		}
		else if (anc == ANC_TYPE::ANC_EAST) {
			val.x = m_start.x + m_view.width;
			val.y = m_start.y + m_view.height * 0.5f;
		}
		else if (anc == ANC_TYPE::ANC_SE) {
			val.x = m_start.x + m_view.width;
			val.y = m_start.y + m_view.height;
		}
		else if (anc == ANC_TYPE::ANC_SOUTH) {
			val.x = m_start.x + m_view.width * 0.5f;
			val.y = m_start.y + m_view.height;
		}
		else if (anc == ANC_TYPE::ANC_SW) {
			val.x = m_start.x;
			val.y = m_start.y + m_view.height;
		}
		else if (anc == ANC_TYPE::ANC_WEST) {
			val.x = m_start.x;
			val.y = m_start.y + m_view.height * 0.5f;
		}
	}

	// �}�`���͂ޗ̈�̍���ʒu�𓾂�.
	void ShapeImage::get_pos_lt(D2D1_POINT_2F& val) const noexcept
	{
		const float ax = m_start.x;
		const float ay = m_start.y;
		const float bx = m_start.x + m_view.width;
		const float by = m_start.y + m_view.height;
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
		val = m_start;
		return true;
	}

	// ���_�𓾂�.
	size_t ShapeImage::get_verts(D2D1_POINT_2F v_pos[]) const noexcept
	{
		v_pos[0] = m_start;
		v_pos[1] = D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y };
		v_pos[2] = D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y + m_view.height };
		v_pos[3] = D2D1_POINT_2F{ m_start.x, m_start.y + m_view.height };
		return 4;
	}

	// �ʒu���܂ނ����肷��.
	// t_pos	���肷��ʒu
	// �߂�l	�ʒu���܂ސ}�`�̕���. �܂܂Ȃ��Ƃ��́u�}�`�̊O���v��Ԃ�.
	uint32_t ShapeImage::hit_test(const D2D1_POINT_2F t_pos) const noexcept
	{
		D2D1_POINT_2F v_pos[4];
		// 0---1
		// |   |
		// 3---2
		get_verts(v_pos);
		if (pt_in_anc(t_pos, v_pos[2], m_anc_width)) {
			return ANC_TYPE::ANC_SE;
		}
		else if (pt_in_anc(t_pos, v_pos[3], m_anc_width)) {
			return ANC_TYPE::ANC_SW;
		}
		else if (pt_in_anc(t_pos, v_pos[1], m_anc_width)) {
			return ANC_TYPE::ANC_NE;
		}
		else if (pt_in_anc(t_pos, v_pos[0], m_anc_width)) {
			return ANC_TYPE::ANC_NW;
		}
		else {
			const auto e_width = m_anc_width * 0.5;
			D2D1_POINT_2F e_pos[2];
			e_pos[0].x = v_pos[0].x;
			e_pos[0].y = static_cast<FLOAT>(v_pos[0].y - e_width);
			e_pos[1].x = v_pos[1].x;
			e_pos[1].y = static_cast<FLOAT>(v_pos[1].y + e_width);
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_NORTH;
			}
			e_pos[0].x = static_cast<FLOAT>(v_pos[1].x - e_width);
			e_pos[0].y = v_pos[1].y;
			e_pos[1].x = static_cast<FLOAT>(v_pos[2].x + e_width);
			e_pos[1].y = v_pos[2].y;
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_EAST;
			}
			e_pos[0].x = v_pos[3].x;
			e_pos[0].y = static_cast<FLOAT>(v_pos[3].y - e_width);
			e_pos[1].x = v_pos[2].x;
			e_pos[1].y = static_cast<FLOAT>(v_pos[2].y + e_width);
			if (pt_in_rect(t_pos, e_pos[0], e_pos[1])) {
				return ANC_TYPE::ANC_SOUTH;
			}
			e_pos[0].x = static_cast<FLOAT>(v_pos[0].x - e_width);
			e_pos[0].y = v_pos[0].y;
			e_pos[1].x = static_cast<FLOAT>(v_pos[3].x + e_width);
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
		return ANC_TYPE::ANC_PAGE;
	}

	// �͈͂Ɋ܂܂�邩���肷��.
	// area_lt	�͈͂̍���ʒu
	// area_rb	�͈͂̉E���ʒu
	// �߂�l	�܂܂��Ȃ� true
	// ���̑����͍l������Ȃ�.
	bool ShapeImage::in_area(const D2D1_POINT_2F area_lt, const D2D1_POINT_2F area_rb) const noexcept
	{
		// �n�_�ƏI�_�Ƃ��͈͂Ɋ܂܂�邩���肷��.
		return pt_in_rect(m_start, area_lt, area_rb) &&
			pt_in_rect(D2D1_POINT_2F{ m_start.x + m_view.width, m_start.y + m_view.height }, area_lt, area_rb);
	}

	// ���������ړ�����.
	bool ShapeImage::move(const D2D1_POINT_2F val) noexcept
	{
		pt_add(m_start, val, m_start);
		return true;
	}

	// ���摜�ɖ߂�.
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
	// keep_aspect	�摜�̏c������ێ�
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
				const D2D1_POINT_2F s_pos{ m_start };	// �n�_ (�}�`�̒��_)
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
					const float page_w = m_view.width - v_vec.x;
					const float page_h = m_view.height - v_vec.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_start = pos;
						m_ratio.width = page_w / image_w;
						m_ratio.height = page_h / image_h;
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
				const D2D1_POINT_2F s_pos{ m_start.x + m_view.width, m_start.y };
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
					const float page_w = pos.x - m_start.x;
					const float page_h = m_start.y + m_view.height - pos.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_start.y = pos.y;
						m_ratio.width = page_w / image_w;
						m_ratio.height = page_h / image_h;
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
				const D2D1_POINT_2F s_pos{ m_start.x + m_view.width, m_start.y + m_view.height };
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
					const float page_w = pos.x - m_start.x;
					const float page_h = pos.y - m_start.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_ratio.width = page_w / image_w;
						m_ratio.height = page_h / image_h;
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
				const D2D1_POINT_2F s_pos{ m_start.x, m_start.y + m_view.height };
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
					const float page_w = m_start.x + m_view.width - pos.x;
					const float page_h = pos.y - m_start.y;
					if (page_w >= 1.0f && page_h >= 1.0f) {
						m_view.width = page_w;
						m_view.height = page_h;
						m_start.x = pos.x;
						m_ratio.width = page_w / image_w;
						m_ratio.height = page_h / image_h;
						if (!flag) {
							flag = true;
						}
					}
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_NORTH) {
			// �ύX���鍷�������߂�.
			const float dy = (new_pos.y - m_start.y);
			if (fabs(dy) >= FLT_MIN) {
				const float rect_top = min(m_clip.top + dy / m_ratio.height, m_clip.bottom - 1.0f);
				m_clip.top = max(rect_top, 0.0f);
				m_view.height = (m_clip.bottom - m_clip.top) * m_ratio.height;
				m_start.y = new_pos.y;
				if (!flag) {
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_EAST) {
			const float dx = (new_pos.x - (m_start.x + m_view.width));
			if (fabs(dx) >= FLT_MIN) {
				const float rect_right = max(m_clip.right + dx / m_ratio.width, m_clip.left + 1.0f);
				m_clip.right = min(rect_right, m_orig.width);
				m_view.width = (m_clip.right - m_clip.left) * m_ratio.width;
				m_start.x = new_pos.x - m_view.width;
				if (!flag) {
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_SOUTH) {
			const float dy = (new_pos.y - (m_start.y + m_view.height));
			if (fabs(dy) >= FLT_MIN) {
				const float rect_bottom = max(m_clip.bottom + dy / m_ratio.height, m_clip.top + 1.0f);
				m_clip.bottom = min(rect_bottom, m_orig.height);
				m_view.height = (m_clip.bottom - m_clip.top) * m_ratio.height;
				m_start.y = new_pos.y - m_view.height;
				if (!flag) {
					flag = true;
				}
			}
		}
		else if (anc == ANC_TYPE::ANC_WEST) {
			const float dx = (new_pos.x - m_start.x);
			if (fabs(dx) >= FLT_MIN) {
				const float rect_left = min(m_clip.left + dx / m_ratio.width, m_clip.right - 1.0f);
				m_clip.left = max(rect_left, 0.0f);
				m_view.width = (m_clip.right - m_clip.left) * m_ratio.width;
				m_start.x = new_pos.x;
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
		if (!equal(m_start, new_pos)) {
			m_start = new_pos;
			return true;
		}
		return false;
	}

	// �}�`���쐬����.
	// pos	����ʒu
	// view	�\�������傫��
	// bmp	�r�b�g�}�b�v
	// opac	�s�����x
	ShapeImage::ShapeImage(const D2D1_POINT_2F pos, const D2D1_SIZE_F view, 
		const SoftwareBitmap& bmp, const float opac)
	{
		const uint32_t image_w = bmp.PixelWidth();
		const uint32_t image_h = bmp.PixelHeight();

		m_orig.width = image_w;
		m_orig.height = image_h;
		m_start = pos;
		m_view = view;
		m_clip.left = m_clip.top = 0;
		m_clip.right = static_cast<FLOAT>(image_w);
		m_clip.bottom = static_cast<FLOAT>(image_h);
		m_opac = opac < 0.0f ? 0.0f : (opac > 1.0f ? 1.0f : opac);
		const size_t bgra_size = 4ull * image_w * image_h;
		if (bgra_size > 0) {
			m_bgra = new uint8_t[bgra_size];
			// SoftwareBitmap �̃o�b�t�@�����b�N����.
			auto image_buf{
				bmp.LockBuffer(BitmapBufferAccessMode::ReadWrite)
			};
			auto image_ref{
				image_buf.CreateReference()
			};
			winrt::com_ptr<IMemoryBufferByteAccess> image_mem = 
				image_ref.as<IMemoryBufferByteAccess>();
			BYTE* image_data = nullptr;
			UINT32 capacity = 0;
			if (SUCCEEDED(image_mem->GetBuffer(&image_data, &capacity))) {
				// ���b�N�����o�b�t�@�ɉ摜�f�[�^���R�s�[����.
				memcpy(m_bgra, image_data, capacity);
				// ���b�N�����o�b�t�@���������.
				image_buf.Close();
				image_buf = nullptr;
			}
			image_mem = nullptr;
		}
		else {
			m_bgra = nullptr;
		}
	}

	// �}�`���f�[�^���[�_�[����ǂݍ���
	// dt_reader	�f�[�^���[�_�[
	ShapeImage::ShapeImage(DataReader const& dt_reader) :
		ShapeSelect(dt_reader),
		m_start(D2D1_POINT_2F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_view(D2D1_SIZE_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_clip(D2D1_RECT_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle(), 
			dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_orig(D2D1_SIZE_U{ dt_reader.ReadUInt32(), dt_reader.ReadUInt32() }),
		m_ratio(D2D1_SIZE_F{ dt_reader.ReadSingle(), dt_reader.ReadSingle() }),
		m_opac(dt_reader.ReadSingle())
	{
		if (m_opac < 0.0f || m_opac > 1.0f) {
			m_opac = 1.0f;
		}
		const size_t pitch = 4ull * m_orig.width;
		m_bgra = new uint8_t[pitch * m_orig.height];
		std::vector<uint8_t> line_buf(pitch);
		const size_t height = m_orig.height;
		for (size_t i = 0; i < height; i++) {
			dt_reader.ReadBytes(line_buf);
			memcpy(m_bgra + pitch * i, line_buf.data(), pitch);
		}
	}

	// �}�`���f�[�^���C�^�[�ɏ�������.
	// dt_writer	�f�[�^���C�^�[
	void ShapeImage::write(DataWriter const& dt_writer) const
	{
		ShapeSelect::write(dt_writer);
		dt_writer.WriteSingle(m_start.x);
		dt_writer.WriteSingle(m_start.y);
		dt_writer.WriteSingle(m_view.width);
		dt_writer.WriteSingle(m_view.height);
		dt_writer.WriteSingle(m_clip.left);
		dt_writer.WriteSingle(m_clip.top);
		dt_writer.WriteSingle(m_clip.right);
		dt_writer.WriteSingle(m_clip.bottom);
		dt_writer.WriteUInt32(m_orig.width);
		dt_writer.WriteUInt32(m_orig.height);
		dt_writer.WriteSingle(m_ratio.width);
		dt_writer.WriteSingle(m_ratio.height);
		dt_writer.WriteSingle(m_opac);

		const size_t pitch = 4ull * m_orig.width;
		std::vector<uint8_t> line_buf(pitch);
		const size_t height = m_orig.height;
		for (size_t i = 0; i < height; i++) {
			memcpy(line_buf.data(), m_bgra + pitch * i, pitch);
			dt_writer.WriteBytes(line_buf);
		}
	}

}