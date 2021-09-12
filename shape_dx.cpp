//------------------------------
// shape_dx.cpp
// �}�`�̕`���
//------------------------------
#include "pch.h"
#include <winrt/Windows.UI.Core.h>
#include <windows.ui.xaml.media.dxinterop.h>
#include "shape_dx.h"

using namespace winrt;
using namespace D2D1;
using namespace DirectX;

#define CALLBACK_CONTEXT_ANY	0

namespace DisplayMetrics
{
	// ���𑜓x�f�B�X�v���C�́A�����_�����O�ɑ����� GPU �ƃo�b�e���d�͂�K�v�Ƃ��܂��B
	// �Q�[�������S�ȍČ������ێ����Ė��b 60 �t���[���Ń����_�����O���悤�Ƃ���ƁA
	// ���𑜓x�̌g�ѓd�b�Ȃǂł̓o�b�e���̎����̒Z���ɔY�܂����ꍇ������܂��B
	// ���ׂẴv���b�g�t�H�[���ƃt�H�[�� �t�@�N�^�[�ɂ킽���Ċ��S�ȍČ������ێ����Ẵ����_�����O�́A
	// �T�d�Ɍ������Č��肷��K�v������܂��B
	static const bool SupportHighResolutions = true;

	// "���𑜓x" �f�B�X�v���C���`�������̂������l.
	// �������l�𒴂��ASupportHighResolutions �� false �̏ꍇ��,
	// �𑜓x�� 50 % �X�P�[���_�E��������.
	static const float DpiThreshold = 192.0f;		// �W���̃f�X�N�g�b�v�� 200% �\���B
	static const float WidthThreshold = 1920.0f;	// �� 1080p�B
	static const float HeightThreshold = 1080.0f;	// ���� 1080p�B
};

// ��ʂ̉�]�̌v�Z�Ɏg�p����萔�B
namespace ScreenRotation
{
	static const XMFLOAT4X4 Rotation0	// 0 �x Z ��]
	(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	static const XMFLOAT4X4 Rotation90	// 90 �x Z ��]
	(
		0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	static const XMFLOAT4X4 Rotation180	// 180 �x Z ��]
	(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	static const XMFLOAT4X4 Rotation270	// 270 �x Z ��]
	(
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
};

namespace winrt::GraphPaper::implementation
{
	static winrt::com_ptr<ID2D1Factory3> create_d2d_factory(void);
	static winrt::com_ptr<IDWriteFactory3> create_dwrite_factory(void);

	winrt::com_ptr<ID2D1Factory3> SHAPE_DX::m_d2dFactory{ create_d2d_factory() };
	winrt::com_ptr<IDWriteFactory3> SHAPE_DX::m_dwriteFactory{ create_dwrite_factory() };

	// �f�o�C�X�Ɉˑ����Ȃ��s�N�Z���P�� (DIP) �̒����𕨗��I�ȃs�N�Z���̒����ɕϊ����܂��B
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // �ł��߂������l�Ɋۂ߂܂��B
	}

	// D2D �t�@�N�g��������������.
	// �߂�l�Ƃ��� com_ptr ��Ԃ��֐��͎Q�ƕs��.
	static winrt::com_ptr<ID2D1Factory3> create_d2d_factory(void)
	{
		winrt::com_ptr<ID2D1Factory3> d2d_factory;
		// �v���W�F�N�g���f�o�b�O �r���h�Ȃ�,
		// �t�@�N�g���I�v�V������ D2D1_DEBUG_LEVEL_INFORMATION ���w�肷��.
		// �t�@�N�g���[�^�C�v�� D2D1_FACTORY_TYPE_SINGLE_THREADED ���w�肷��.
		D2D1_FACTORY_OPTIONS options;
		ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));
#if defined(_DEBUG)
		options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
		winrt::check_hresult(
			D2D1CreateFactory(
				D2D1_FACTORY_TYPE_SINGLE_THREADED,
				__uuidof(ID2D1Factory3),
				&options,
				d2d_factory.put_void()
			)
		);
		return d2d_factory;
	}

	// DWrite �t�@�N�g��������������.
	// �߂�l�Ƃ��� com_ptr ��Ԃ��֐��͎Q�ƕs��.
	static winrt::com_ptr<IDWriteFactory3> create_dwrite_factory(void)
	{
		winrt::com_ptr<IDWriteFactory3> dw_factory;
		// �t�@�N�g���^�C�v�ɂ� DWRITE_FACTORY_TYPE_SHARED ���w�肷��.
		winrt::check_hresult(
			DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(IDWriteFactory3),
				reinterpret_cast<::IUnknown**>(dw_factory.put())
			)
		);
		return dw_factory;
	};

	static DXGI_MODE_ROTATION get_display_rotation(const DisplayOrientations native, const DisplayOrientations current)
	{
		// �\���f�o�C�X�̊���̕����ƌ��݂̕��������Ƃ�, �\���f�o�C�X�̉�]�����肷��.
		// �K��l��, DXGI_MODE_ROTATION_UNSPECIFIED.
		// ����: DisplayOrientations �񋓌^�ɑ��̒l�������Ă��ANativeOrientation �Ƃ��Ďg�p�ł���̂́A
		// Landscape �܂��� Portrait �̂ǂ��炩�̂�.
		auto displayRotation = DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_IDENTITY;
		constexpr auto L = DisplayOrientations::Landscape;
		constexpr auto P = DisplayOrientations::Portrait;
		constexpr auto LF = DisplayOrientations::LandscapeFlipped;
		constexpr auto PF = DisplayOrientations::PortraitFlipped;
		// ���肪���R���������݂����R����, �܂��͊��肪�^�e���������݂��^�e���������肷��.
		if ((native == L && current == L)
			|| (native == P && current == P)) {
			// DXGI_MODE_ROTATION_IDENTITY. 
			displayRotation = DXGI_MODE_ROTATION_IDENTITY;
		}
		// ���肪���R���������݂��^�e����, �܂��͊��肪�^�e���������݂������R���������肷��.
		else if ((native == L && current == P)
			|| (native == P && current == LF)) {
			// DXGI_MODE_ROTATION_ROTATE270.
			displayRotation = DXGI_MODE_ROTATION_ROTATE270;
		}
		// ���肪���R���������݂������R����, �܂��͊��肪�^�e���������݂����^�e���������肷��.
		else if ((native == L && current == LF)
			|| (native == P && current == PF)) {
			// DXGI_MODE_ROTATION_ROTATE180.
			displayRotation = DXGI_MODE_ROTATION_ROTATE180;
		}
		// ���肪���R���������݂����^�e����, �܂��͊��肪�^�e���������݂����R���������肷��.
		else if ((native == L && current == PF)
			|| (native == P && current == L)) {
			// DXGI_MODE_ROTATION_ROTATE90.
			displayRotation = DXGI_MODE_ROTATION_ROTATE90;
		}
		return displayRotation;
	}

	// �X���b�v �`�F�[���̓K�؂ȕ�����ݒ肵�A��]���ꂽ�X���b�v �`�F�[���Ƀ����_�����O���邽�߂� 2D �����
	// 3D �}�g���b�N�X�ϊ��𐶐����܂��B
	// 2D ����� 3D �ϊ��̉�]�p�x�͈قȂ�܂��B
	// ����͍��W��Ԃ̈Ⴂ�ɂ��܂��B����ɁA
	// �ۂ߃G���[��������邽�߂� 3D �}�g���b�N�X�������I�Ɏw�肳��܂��B
	static void set_display_rotation(
		IDXGISwapChain3* dxgi_swap_chain,
		Matrix3x2F& trans2D,
		DirectX::XMFLOAT4X4& trans3D,
		const DXGI_MODE_ROTATION displayRotation,
		const float logical_width,
		const float logical_height)
	{
		switch (displayRotation)
		{
		case DXGI_MODE_ROTATION_IDENTITY:
			trans2D = Matrix3x2F::Identity();
			trans3D = ScreenRotation::Rotation0;
			break;

		case DXGI_MODE_ROTATION_ROTATE90:
			trans2D =
				Matrix3x2F::Rotation(90.0f) *
				Matrix3x2F::Translation(logical_height, 0.0f);
			trans3D = ScreenRotation::Rotation270;
			break;

		case DXGI_MODE_ROTATION_ROTATE180:
			trans2D =
				Matrix3x2F::Rotation(180.0f) *
				Matrix3x2F::Translation(logical_width, logical_height);
			trans3D = ScreenRotation::Rotation180;
			break;

		case DXGI_MODE_ROTATION_ROTATE270:
			trans2D =
				Matrix3x2F::Rotation(270.0f) *
				Matrix3x2F::Translation(0.0f, logical_width);
			trans3D = ScreenRotation::Rotation90;
			break;

		default:
			throw winrt::hresult_invalid_argument();
		}
		winrt::check_hresult(dxgi_swap_chain->SetRotation(displayRotation));
	}

	// �E�B���h�E�T�C�Y�ˑ��̃��\�[�X���쐬����.
	// �����̃��\�[�X�́A�E�B���h�E�T�C�Y���ύX����邽�тɍč쐬����K�v������.
	void SHAPE_DX::CreateWindowSizeDependentResources()
	{
		using winrt::Windows::UI::Core::CoreDispatcherPriority;
		using winrt::Windows::UI::Core::DispatchedHandler;

		// �O�̃E�B���h�E�T�C�Y�ɌŗL�̃R���e�L�X�g���N���A���邽��,
		// �����_�[�^�[�Q�b�g�� D3D �R���e�L�X�g�̏o�̓}�[�W (OM) �X�e�[�W�Ɋi�[����.
		// �����_�[�^�[�Q�b�g�͋�̃r���[, �[�x/�X�e���V���o�b�t�@�[�̓k�����w�肷��.
		ID3D11RenderTargetView* nullViews[] = { nullptr };
		m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
		// ��^�[�Q�b�g���@D2D �R���e�L�X�g�Ɋi�[����.
		m_d2dContext->SetTarget(nullptr);
		// �k���� D2D �r�b�g�}�b�v�Ɋi�[����.
		m_d2dTargetBitmap = nullptr;
		// GPU �̃R�}���h�o�b�t�@����̃C�x���g�N�G���Ńt���b�V������.
		m_d3dContext->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

		// �_�� DPI �ƍ����{���̒l��, �L���� DPI �ƗL���ȍ����{���Ɋi�[����.
		m_effectiveDpi = m_logical_dpi;
		m_effectiveCompositionScaleX = m_compositionScaleX;
		m_effectiveCompositionScaleY = m_compositionScaleY;

		// (���𑜓x�̃f�o�C�X�̃o�b�e���������グ�邽�߂ɂ́A��菬���������_�[ �^�[�Q�b�g�Ƀ����_�����O����
		// �o�͂��񎦂��ꂽ�ꍇ�� GPU �ŏo�͂��X�P�[�����O�ł���悤����.)
		if constexpr (DisplayMetrics::SupportHighResolutions != true
			&& m_logical_dpi > DisplayMetrics::DpiThreshold) {
			// ���𑜓x���T�|�[�g���Ȃ�, ���_�� DPI �͂����l�𒴂���Ȃ�,
			// �_���I�ȕ��ƍ�����, �_�� DPI ����������, 
			// �s�N�Z���P�ʂ̕��ƍ����ɕϊ�����.
			const float width = ConvertDipsToPixels(m_logical_width, m_logical_dpi);
			const float height = ConvertDipsToPixels(m_logical_height, m_logical_dpi);

			// �ϊ����ꂽ���ƍ������������l�𒴂��邩���肷��.
			if (max(width, height) > DisplayMetrics::WidthThreshold&& min(width, height) > DisplayMetrics::HeightThreshold)
			{
				// �L���� DPI �ƗL���ȍ����{���� 1/2 ����.
				m_effectiveDpi *= 0.5f;
				m_effectiveCompositionScaleX *= 0.5f;
				m_effectiveCompositionScaleY *= 0.5f;
			}
		}

		// �L���� DPI ��p����, �_���I�ȕ��ƍ������s�N�Z���P�ʂɕϊ���, 
		// �o�͗̈�̕��ƍ����Ɋi�[����.
		// �T�C�Y 0 �� DirectX �R���e���c���쐬����邱�Ƃ�h�~���邽��,
		// �o�͗̈�̕��ƍ����͍Œ�ł� 1px �Ƃ���.
		m_output_width = ConvertDipsToPixels(m_logical_width, m_effectiveDpi);
		m_output_height = ConvertDipsToPixels(m_logical_height, m_effectiveDpi);
		m_output_width = max(m_output_width, 1);
		m_output_height = max(m_output_height, 1);

		// �\���f�o�C�X�̊���̕����ƌ��݂̕��������Ƃ�, �\���f�o�C�X�̉�]�����肷��.
		// �K��l��, DXGI_MODE_ROTATION_UNSPECIFIED.
		// ����: DisplayOrientations �񋓌^�ɑ��̒l�������Ă��ANativeOrientation �Ƃ��Ďg�p�ł���̂́A
		// Landscape �܂��� Portrait �̂ǂ��炩�̂�.
		auto displayRotation = get_display_rotation(m_nativeOrientation, m_currentOrientation);
		// �\���f�o�C�X�̉�]�� 90�� �܂��� 270�� �����肷��.
		if (displayRotation == DXGI_MODE_ROTATION_ROTATE90
			|| displayRotation == DXGI_MODE_ROTATION_ROTATE270) {
			// �Ȃ��, �o�͗̈�̕��ƍ��������ւ���, D3D �^�[�Q�b�g�̕��ƍ����Ɋi�[����.
			m_d3d_target_width = m_output_height;
			m_d3d_target_height = m_output_width;
		}
		else {
			// �����łȂ����, �o�͗̈�̕��ƍ��������̂܂� D3D �^�[�Q�b�g�̕��ƍ����Ɋi�[����.
			m_d3d_target_width = m_output_width;
			m_d3d_target_height = m_output_height;
		}

		// DXGI �X���b�v�`�F�[��������������Ă��邩���肷��.
		if (m_dxgi_swap_chain != nullptr) {
			// DXGI �X���b�v�`�F�[���̃o�b�t�@�T�C�Y��ύX��, ���̌��ʂ𓾂�.
			// �o�b�t�@�̑傫���ɂ� D3D �^�[�Q�b�g�̕��ƍ�����,
			// �o�b�t�@�̐��ɂ� 2 (�_�u�� �o�b�t�@�[) ��, 
			// �t�H�[�}�b�g�ɂ� DXGI_FORMAT_B8G8R8A8_UNORM ���w�肷��.
			HRESULT hr = m_dxgi_swap_chain->ResizeBuffers(
				2,
				lround(m_d3d_target_width),
				lround(m_d3d_target_height),
				DXGI_FORMAT_B8G8R8A8_UNORM,
				0
			);

			// ���ʂ� DXGI_ERROR_DEVICE_REMOVED �܂��� DXGI_ERROR_DEVICE_RESET �����肷��.
			if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
				// ���ׂẴf�o�C�X ���\�[�X���č쐬��, ���݂̏�ԂɍĐݒ肷��.
				HandleDeviceLost();
				return;
			}
			else {
				winrt::check_hresult(hr);
			}
		}
		else {
			// ����ȊO�̏ꍇ�́A������ Direct3D �f�o�C�X�Ɠ����A�_�v�^�[���g�p���āA�V�K�쐬����.
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
			// DXGI �X���b�v�`�F�[���̏ڍׂ�����������.
			// ���ɂ� D3D �^�[�Q�b�g�̕�,
			// �����ɂ� D3D �^�[�Q�b�g�̍���,
			// �t�H�[�}�b�g�ɂ� DXGI_FORMAT_B8G8R8A8_UNORM,
			// �X�e���I�ɂ� false,
			// �}���`�T���v�����O�̃s�N�Z�����ɂ� 1,
			// �}���`�T���v�����O�̕i�����x���ɂ� 0,
			// �o�b�t�@�̎g�p���@�ɂ� DXGI_USAGE_RENDER_TARGET_OUTPUT,
			// �o�b�t�@�̐��ɂ� 2,
			// �t���O�ɂ� 0 �����ꂼ��w�肷��.
			swapChainDesc.Width = lround(m_d3d_target_width);		// �E�B���h�E�̃T�C�Y�ƈ�v�����܂��B
			swapChainDesc.Height = lround(m_d3d_target_height);
			swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;				// ����́A�ł���ʓI�ȃX���b�v �`�F�[���̃t�H�[�}�b�g�ł��B
			swapChainDesc.Stereo = false;
			swapChainDesc.SampleDesc.Count = 1;								// �}���`�T���v�����O�͎g���܂���B
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = 2;	// �x�����ŏ����ɗ}����ɂ̓_�u�� �o�b�t�@�[���g�p���܂��B
			swapChainDesc.Flags = 0;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			// �X�P�[�����O�ɂ� DXGI_SCALING_STRETCH ���w�肷��.
			// �����͂悭�킩��Ȃ���, ���Ƃ��o�b�t�@�T�C�Y����v���Ă����Ƃ��Ă� DXGI_SCALING_NONE ���w�肷��ƃG���[�ɂȂ�.
			//if (DisplayMetrics::SupportHighResolutions
			// || m_logical_dpi <= DisplayMetrics::DpiThreshold) {
			//	swapChainDesc.Scaling = DXGI_SCALING_NONE;
			//}
			//else {
				swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
			//}
			// D3D �h���C�o�[�� D3D_DRIVER_TYPE_HARDWARE �����肷��.
			if (m_d3d_driver_type == D3D_DRIVER_TYPE_HARDWARE) {
				// �؂�ւ����ʂ� DXGI_SWAP_EFFECT_FLIP_DISCARD ���i�[����.
				// ���̌��ʂ��w�肷���, Present1 �Ăяo����Ɏc�����o�b�t�@�͔j�������.
				// �j������ĂȂ��o�b�t�@���c���Ă���ꍇ, �n�[�h�E�F�A�h���C�o�[�͂����\�����Ă��܂�.
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			}
			else {
				// D3D �h���C�o�[�� D3D_DRIVER_TYPE_HARDWARE �ȊO�Ȃ�,
				// �؂�ւ����ʂ� DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL ���i�[����.
				// ���̌��ʂ��w�肷���, Present1 �Ăяo����Ɏc�����o�b�t�@�͕ێ������.
				// �j������ĂȂ��o�b�t�@���c���Ă���ꍇ�ł�, WARP �h���C�o�[�͐���ɕ\������.
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// ���ׂĂ� Windows �X�g�A �A�v���́A_FLIP_ SwapEffects ���g�p����K�v������܂��B
			}

			// ���̃V�[�P���X�́A��� Direct3D �f�o�C�X���쐬����ۂɎg�p���ꂽ DXGI �t�@�N�g�����擾����.
			winrt::com_ptr<IDXGIDevice3> dxgiDevice;
			winrt::check_hresult(m_d3dDevice.try_as(dxgiDevice));

			winrt::com_ptr<IDXGIAdapter> dxgiAdapter;
			winrt::check_hresult(dxgiDevice->GetAdapter(dxgiAdapter.put()));

			winrt::com_ptr<IDXGIFactory4> dxgiFactory;
			winrt::check_hresult(dxgiAdapter->GetParent(_uuidof(&dxgiFactory), dxgiFactory.put_void()));

			// XAML ���݉^�p�@�\�̎g�p����, �����p�ɃX���b�v �`�F�[�����쐬����K�v����.
			winrt::com_ptr<IDXGISwapChain1> swapChain;
			winrt::check_hresult(
				dxgiFactory->CreateSwapChainForComposition(
					m_d3dDevice.get(),
					&swapChainDesc,
					nullptr,
					swapChain.put()
				)
			);

			winrt::check_hresult(swapChain.try_as(m_dxgi_swap_chain));

			// �X���b�v �`�F�[���� SwapChainPanel ���֘A�t����.
			// UI �̕ύX�́AUI �X���b�h�Ƀf�B�X�p�b�`���Ė߂��K�v����.
			const auto _{ m_swapChainPanel.Dispatcher().RunAsync(
				CoreDispatcherPriority::High,
				[=]()
				{
					// SwapChainPanel �̃l�C�e�B�u �C���^�[�t�F�C�X�ɖ߂�.
					winrt::com_ptr<ISwapChainPanelNative> scpNative{ nullptr };
					winrt::check_hresult(winrt::get_unknown(m_swapChainPanel)->QueryInterface(__uuidof(scpNative), scpNative.put_void()));
					winrt::check_hresult(scpNative->SetSwapChain(m_dxgi_swap_chain.get()));
				}
			) };
			// DXGI �� 1 �x�ɕ����̃t���[�����L���[�������Ă��Ȃ����Ƃ��m�F���܂��B����ɂ��A�x�����������A
			// �A�v���P�[�V�������e VSync �̌�ł̂݃����_�����O���邱�Ƃ��ۏ؂���A����d�͂��ŏ����ɗ}�����܂��B
			winrt::check_hresult(dxgiDevice->SetMaximumFrameLatency(1));
		}

		// �X���b�v �`�F�[���̓K�؂ȕ�����ݒ肵�A��]���ꂽ�X���b�v �`�F�[���Ƀ����_�����O���邽�߂�
		// 2D ����� 3D �}�g���b�N�X�ϊ��𐶐����܂��B
		// 2D ����� 3D �ϊ��̉�]�p�x�͈قȂ�܂��B����͍��W��Ԃ̈Ⴂ�ɂ��܂��B
		// ����ɁA�ۂ߃G���[��������邽�߂� 3D �}�g���b�N�X�������I�Ɏw�肳��܂��B
		set_display_rotation(m_dxgi_swap_chain.get(), m_orientationTransform2D, m_orientationTransform3D, displayRotation, m_logical_width, m_logical_height);

		// �X���b�v �`�F�[���ɋt�g�嗦��ݒ肵�܂�
		DXGI_MATRIX_3X2_F inverseScale{ 0 };
		inverseScale._11 = 1.0f / m_effectiveCompositionScaleX;
		inverseScale._22 = 1.0f / m_effectiveCompositionScaleY;
		winrt::com_ptr<IDXGISwapChain2> spSwapChain2;
		winrt::check_hresult(m_dxgi_swap_chain.try_as(spSwapChain2));//As<IDXGISwapChain2>(&spSwapChain2));		
		winrt::check_hresult(spSwapChain2->SetMatrixTransform(&inverseScale));

		// �X���b�v �`�F�[���̃o�b�N �o�b�t�@�[�̃����_�����O �^�[�Q�b�g �r���[���쐬���܂��B
		winrt::com_ptr<ID3D11Texture2D1> backBuffer;
		winrt::check_hresult(
			m_dxgi_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D1), backBuffer.put_void())//IID_PPV_ARGS(&backBuffer))
		);
		// �X���b�v �`�F�[�� �o�b�N �o�b�t�@�[�Ɋ֘A�t����ꂽ Direct2D �^�[�Q�b�g �r�b�g�}�b�v���쐬���A
		// ��������݂̃^�[�Q�b�g�Ƃ��Đݒ肵�܂��B
		D2D1_BITMAP_PROPERTIES1 bitmapProperties =
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
				m_logical_dpi,
				m_logical_dpi
			);

		winrt::com_ptr<IDXGISurface2> dxgiBackBuffer;
		winrt::check_hresult(
			m_dxgi_swap_chain->GetBuffer(0, __uuidof(IDXGISurface2), dxgiBackBuffer.put_void())//IID_PPV_ARGS(&dxgiBackBuffer))
		);
		winrt::check_hresult(
			m_d2dContext->CreateBitmapFromDxgiSurface(
				dxgiBackBuffer.get(),
				&bitmapProperties,
				m_d2dTargetBitmap.put()
			)
		);
		m_d2dContext->SetTarget(m_d2dTargetBitmap.get());
		m_d2dContext->SetDpi(m_effectiveDpi, m_effectiveDpi);

		// ���ׂĂ� Windows �X�g�A �A�v���ŁA�O���[�X�P�[�� �e�L�X�g�̃A���`�G�C���A�V���O�������߂��܂��B
		m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	}

	// ���ׂẴf�o�C�X ���\�[�X���č쐬���A���݂̏�ԂɍĐݒ肷��.
	void SHAPE_DX::HandleDeviceLost()
	{
		m_dxgi_swap_chain = nullptr;

		if (m_deviceNotify != nullptr) {
			m_deviceNotify->OnDeviceLost();
		}

		m_d2dContext->SetDpi(m_logical_dpi, m_logical_dpi);
		CreateWindowSizeDependentResources();

		if (m_deviceNotify != nullptr) {
			m_deviceNotify->OnDeviceRestored();
		}
	}

	// �X���b�v�`�F�[���̓��e����ʂɕ\������.
	void SHAPE_DX::Present()
	{
		DXGI_PRESENT_PARAMETERS param{ 0 };
		HRESULT hr = m_dxgi_swap_chain->Present1(1, 0, &param);
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
			HandleDeviceLost();
		}
		else {
			winrt::check_hresult(hr);
		}
	}

	// �f�o�C�X������ꂽ�Ƃ��ƍ쐬���ꂽ�Ƃ��ɒʒm���󂯂�悤�ɁADeviceNotify ��o�^����.
	void SHAPE_DX::RegisterDeviceNotify(IDeviceNotify* deviceNotify)
	{
		m_deviceNotify = deviceNotify;
	}

	// �`����ɍ����{����ݒ肷��.
	// ���̃��\�b�h�́ACompositionScaleChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
	void SHAPE_DX::SetCompositionScale(float compositionScaleX, float compositionScaleY)
	{
		if (m_compositionScaleX != compositionScaleX || m_compositionScaleY != compositionScaleY) {
			m_compositionScaleX = compositionScaleX;
			m_compositionScaleY = compositionScaleY;
			CreateWindowSizeDependentResources();
		}
	}

	// �`����Ƀf�o�C�X�̌�����ݒ肷��.
	// ���̃��\�b�h�́AOrientationChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
	void SHAPE_DX::SetCurrentOrientation(DisplayOrientations currentOrientation)
	{
		if (m_currentOrientation != currentOrientation) {
			m_currentOrientation = currentOrientation;
			CreateWindowSizeDependentResources();
		}
	}

	// �`����� DPI ��ݒ肷��.
	// ���̃��\�b�h�́ADpiChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
	void SHAPE_DX::SetDpi(float dpi)
	{
		if (dpi == m_logical_dpi) {
			return;
		}
		m_logical_dpi = dpi;
		m_d2dContext->SetDpi(m_logical_dpi, m_logical_dpi);
		CreateWindowSizeDependentResources();
	}

	// �`����ɕ\���̈�̑傫����ݒ肷��.
	// ���̃��\�b�h�́ASizeChanged �C�x���g�n���h���[�̒��ŌĂяo�����.
	void SHAPE_DX::SetLogicalSize2(const D2D1_SIZE_F size)
	{
		if (m_logical_width == size.width && m_logical_height == size.height) {
			return;
		}
		m_logical_width = size.width;
		m_logical_height = size.height;
		CreateWindowSizeDependentResources();
	}

	// �`����� XAML �X���b�v�`�F�[���p�l����ݒ肷��.
	// ���̃��\�b�h��, UI �R���g���[�����쐬 (�܂��͍č쐬) ���ꂽ�Ƃ��ɌĂяo�����.
	void SHAPE_DX::SetSwapChainPanel(SwapChainPanel const& xaml_scp)
	{
		using winrt::Windows::Graphics::Display::DisplayInformation;

		// ���݂̕\����� DisplayInfomation �𓾂�.
		// �\����Ԃ����Ƃ�, �\���f�o�C�X�̊���̌����ƌ��݂̌���, 
		// �_�� DPI �𓾂�. 
		// D2D �R���e�L�X�g�ɘ_�� DPI ���i�[����.
		// �p�l���ւ̕ێ����ꂽ�Q�ƂɎw�肳�ꂽ�X���b�v�`�F�[���p�l����ۑ�����.
		// �p�l�������Ƃ�, ���ۓI�ȕ��ƍ���, �g�嗦�𓾂�, 
		// ���ꂼ��_���I�ȍ����ƕ�, �����{���Ɋi�[����.
		DisplayInformation di = DisplayInformation::GetForCurrentView();
		m_nativeOrientation = di.NativeOrientation();
		m_currentOrientation = di.CurrentOrientation();
		m_logical_dpi = di.LogicalDpi();
		m_d2dContext->SetDpi(m_logical_dpi, m_logical_dpi);
		m_swapChainPanel = xaml_scp;
		m_compositionScaleX = xaml_scp.CompositionScaleX();
		m_compositionScaleY = xaml_scp.CompositionScaleY();
		m_logical_width = static_cast<FLOAT>(xaml_scp.ActualWidth());
		m_logical_height = static_cast<FLOAT>(xaml_scp.ActualHeight());
		if (m_logical_width > 0.0 && m_logical_height > 0.0) {
			CreateWindowSizeDependentResources();
		}
	}

	// D3D �f�o�C�X���\����, D3D �Ƒ��݉^�p����� D2D �f�o�C�X���쐬����.
	SHAPE_DX::SHAPE_DX(void)
	{
		D3D_FEATURE_LEVEL m_d3dFeatureLevel{ D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_1 };

		// D3D11_CREATE_DEVICE_BGRA_SUPPORT ���쐬�t���O�Ɋi�[����.
		// D2D �Ƃ̌݊�����ێ����邽�߂ɕK�v.
		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
		// �R���p�C�����f�o�b�O�r���h�Ȃ�,
		// D3D �f�o�C�X���쐬�ł��邩���肵, ���̌��ʂ𓾂�.
		// �h���C�o�[�^�C�v�� D3D_DRIVER_TYPE_NULL ��, 
		// �t���O�� D3D11_CREATE_DEVICE_DEBUG ��,
		// SDK �o�[�W������ D3D11_SDK_VERSION ��,
		// ���̑��̈����� nullptr �܂��� 0 ���w�肷��.
		// D3D_DRIVER_TYPE_NULL ��, ���ۂ̃n�[�h�E�F�A�f�o�C�X���쐬���Ȃ�.
		// D3D11_CREATE_DEVICE_DEBUG ��, SDK ���C���[�̊m�F.
		// D3D11_SDK_VERSION ��, Windows �X�g�A�A�v���ł͏�ɕK�v.
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,
			0,
			D3D11_CREATE_DEVICE_DEBUG,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			nullptr,
			nullptr,
			nullptr
		);
		if (SUCCEEDED(hr)) {
			// �쐬�ł���Ȃ�,
			// D3D11_CREATE_DEVICE_DEBUG ���쐬�t���O�ɒǉ�����.
			creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
		}
#endif
		// D3D_FEATURE_LEVEL_9_1 �ȍ~���@�\���x���z��Ɋi�[����.
		constexpr D3D_FEATURE_LEVEL featureLevels[] = {
			// ���̔z��ł́A�������ۑ�����邱�Ƃɒ��ӂ���.
			// �A�v���P�[�V�����̍Œ���K�v�ȋ@�\���x�������̐����Ő錾���邱�Ƃ�Y��Ȃ��ł��������B
			// ���ɋL�ڂ��Ȃ�����A���ׂẴA�v���P�[�V������ 9.1 ���T�|�[�g���邱�Ƃ��z�肳��܂��B
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};
		// D3D �f�o�C�X�� D3D �R���e�L�X�g���쐬�����ʂ𓾂�.
		// �����Ƃ���, �쐬�t���O�Ƌ@�\���x���z��ɉ���, 
		// �h���C�o�[�^�C�v�� D3D_DRIVER_TYPE_HARDWARE ��,
		// SDK �o�[�W������ D3D11_SDK_VERSION ��,
		// ���̑��� 0 �܂��� nullptr ���w�肷��.
		winrt::com_ptr<ID3D11Device> device;
		winrt::com_ptr<ID3D11DeviceContext> context;
#if !defined(_DEBUG)
		HRESULT
#endif
		hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			creationFlags,
			featureLevels,
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,
			device.put(),
			&m_d3dFeatureLevel,
			context.put()
		);
		if (SUCCEEDED(hr)) {
			// �쐬�ł����Ȃ�, D3D_DRIVER_TYPE_HARDWARE �� D3D �h���C�o�[�^�C�v�Ɋi�[����.
			m_d3d_driver_type = D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE;
		}
		else {
			// �쐬�ł��Ȃ��Ȃ� WARP �f�o�C�X���쐬����.
			// �h���C�o�[�^�C�v�� D3D_DRIVER_TYPE_WARP ���w�肷��.
			// �쐬�ł��Ȃ��Ȃ�, ���f����.
			// �쐬�����Ȃ�, D3D_DRIVER_TYPE_WARP �� D3D �h���C�o�[�^�C�v�Ɋi�[����.
			winrt::check_hresult(
				D3D11CreateDevice(
					nullptr,
					D3D_DRIVER_TYPE_WARP,
					0,
					creationFlags,
					featureLevels,
					ARRAYSIZE(featureLevels),
					D3D11_SDK_VERSION,
					device.put(),
					&m_d3dFeatureLevel,
					context.put()
				)
			);
			m_d3d_driver_type = D3D_DRIVER_TYPE_WARP;
		}
		winrt::check_hresult(device.try_as(m_d3dDevice));
		winrt::check_hresult(context.try_as(m_d3dContext));

		// D3D �f�o�C�X�� DXGI �f�o�C�X�Ɋi�[����.
		winrt::com_ptr<IDXGIDevice3> dxgiDevice;
		winrt::check_hresult(m_d3dDevice.try_as(dxgiDevice));
		// DXGI �f�o�C�X�����Ƃ� D2D �f�o�C�X���쐬����.
		m_d2dDevice = nullptr;
		winrt::check_hresult(
			m_d2dFactory->CreateDevice(dxgiDevice.get(), m_d2dDevice.put())
		);
		// D2D �f�o�C�X�����Ƃ� D2D �R���e�L�X�g���쐬����.
		// �I�v�V������ D2D1_DEVICE_CONTEXT_OPTIONS_NONE ���w�肷��.
		m_d2dContext = nullptr;
		winrt::check_hresult(
			m_d2dDevice->CreateDeviceContext(
				D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
				m_d2dContext.put()
			)
		);
		// D2D �R���e�L�X�g�����Ƃ�, �}�`�̐F�u���V, ���ʂ̐F�u���V,
		// �⏕���̐F�u���V, �����͈͂̕����̐F�u���V, �⏕���̌`��,
		// �`���Ԃ̕ۑ��u���b�N���쐬����.
		m_shape_brush = nullptr;
		winrt::check_hresult(
			m_d2dContext->CreateSolidColorBrush({}, m_shape_brush.put())
		);
		m_range_brush = nullptr;
		winrt::check_hresult(
			m_d2dContext->CreateSolidColorBrush({}, m_range_brush.put())
		);
		m_aux_style = nullptr;
		winrt::check_hresult(
			m_d2dFactory->CreateStrokeStyle(AUX_STYLE, AUX_DASHES, AUX_DASHES_CONT, m_aux_style.put())
		);
		m_state_block = nullptr;
		winrt::check_hresult(
			m_d2dFactory->CreateDrawingStateBlock(m_state_block.put())
		);

	};

	// �o�b�t�@�[������ł��邱�Ƃ�, �h���C�o�[�Ɏ���.
	// ���̃��\�b�h��, �A�v������~/���f�����Ƃ��ɌĂяo�����.
	void SHAPE_DX::Trim()
	{
		if (m_d3dDevice == nullptr) {
			return;
		}
		winrt::com_ptr<IDXGIDevice3> dxgiDevice;
		m_d3dDevice.as(dxgiDevice);
		dxgiDevice->Trim();
	}

	// �\���f�o�C�X���L���ɂȂ���.
	// ���̃��\�b�h�́ADisplayContentsInvalidated �C�x���g�p�̃C�x���g �n���h���[�̒��ŌĂяo����܂��B
	void SHAPE_DX::ValidateDevice(void)
	{
		//�f�o�C�X���쐬���ꂽ��Ɋ���̃A�_�v�^�[���ύX���ꂽ�A
		// �܂��͂��̃f�o�C�X���폜���ꂽ�ꍇ�́AD3D �f�o�C�X���L���łȂ��Ȃ�܂��B

		// �܂��A�f�o�C�X���쐬���ꂽ���_����A����̃A�_�v�^�[�Ɋւ�������擾���܂��B

		winrt::com_ptr<IDXGIDevice3> dxgiDevice;
		winrt::check_hresult(m_d3dDevice.try_as(dxgiDevice));

		winrt::com_ptr<IDXGIAdapter> deviceAdapter;
		winrt::check_hresult(dxgiDevice->GetAdapter(deviceAdapter.put()));

		winrt::com_ptr<IDXGIFactory2> deviceFactory;
		winrt::check_hresult(deviceAdapter->GetParent(__uuidof(IDXGIFactory2), deviceFactory.put_void()));//IID_PPV_ARGS(&deviceFactory)));

		winrt::com_ptr<IDXGIAdapter1> previousDefaultAdapter;
		winrt::check_hresult(deviceFactory->EnumAdapters1(0, previousDefaultAdapter.put()));

		DXGI_ADAPTER_DESC1 previousDesc;
		winrt::check_hresult(previousDefaultAdapter->GetDesc1(&previousDesc));

		// ���ɁA���݂̊���̃A�_�v�^�[�̏����擾���܂��B

		winrt::com_ptr<IDXGIFactory4> currentFactory;
		winrt::check_hresult(CreateDXGIFactory1(__uuidof(IDXGIFactory4), currentFactory.put_void()));// IID_PPV_ARGS(&currentFactory)));

		winrt::com_ptr<IDXGIAdapter1> currentDefaultAdapter;
		winrt::check_hresult(currentFactory->EnumAdapters1(0, currentDefaultAdapter.put()));// &currentDefaultAdapter));

		DXGI_ADAPTER_DESC1 currentDesc;
		winrt::check_hresult(currentDefaultAdapter->GetDesc1(&currentDesc));

		// �A�_�v�^�[�� LUID ����v���Ȃ��A�܂��̓f�o�C�X�� LUID ���폜���ꂽ�Ƃ̕񍐂��������ꍇ�́A
		// �V���� D3D �f�o�C�X���쐬����K�v������܂��B

		if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
			previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
			FAILED(m_d3dDevice->GetDeviceRemovedReason()))
		{
			// �Â��f�o�C�X�Ɋ֘A�������\�[�X�ւ̎Q�Ƃ�������܂��B
			dxgiDevice = nullptr;
			deviceAdapter = nullptr;
			deviceFactory = nullptr;
			previousDefaultAdapter = nullptr;

			// ���ׂẴf�o�C�X ���\�[�X���č쐬���A���݂̏�ԂɍĐݒ肷��.
			HandleDeviceLost();
		}
	}

}