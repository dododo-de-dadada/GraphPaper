#include "pch.h"
#include "zlib.h"
#include "MainPage.h"

using namespace::Zlib::implementation;

// PDF �t�H�[�}�b�g
// https://aznote.jakou.com/prog/pdf/index.html
// �ڍ�PDF���� �[ �������Ċw�ڂ��IPDF�t�@�C���̍\���Ƃ��̏������ǂݕ�
// https://itchyny.hatenablog.com/entry/2015/09/16/100000
// PDF����u�g����v�e�L�X�g�����o���i��1��j
// https://golden-lucky.hatenablog.com/entry/2019/12/01/001701
// PDF �\�����
// https://www.pdf-tools.trustss.co.jp/Syntax/parsePdfProc.html#proc
// ���č���Ċw�ԁAPDF�t�@�C���̊�{�\��
// https://techracho.bpsinc.jp/west/2018_12_07/65062
// �O���t�ƃO���t�̎��s
// https://learn.microsoft.com/ja-JP/windows/win32/directwrite/glyphs-and-glyph-runs

using namespace winrt;

namespace winrt::GraphPaper::implementation
{
	using winrt::Windows::Storage::FileAccessMode;
	using winrt::Windows::Storage::CachedFileManager;
	using winrt::Windows::Storage::Provider::FileUpdateStatus;

}

// �O���t�ɂ�������W�l�╝���w�肷��ꍇ�APDF ���ł́A��� 1 em = 1000 �ł�����̂Ƃ��āA
// �l��ݒ肵�܂��B���ۂ̃t�H���g�� 1 em = 1024 �ȂǂƂȂ��Ă���ꍇ�́An / 1024 * 1000 
// �Ƃ����悤�ɂ��āA�l�� 1 em = 1000 �ɍ��킹�܂�.
// 
// PDF �� FontBBox (Black Box) �̃C���[�W
//     y
//     ^
//     |
//   +-+--------+
//   | |   /\   |
//   | |  /__\  |
//   | | /    \ |
// --+-+--------+--> x
//   | |        |
//   +-+--------+
//     |