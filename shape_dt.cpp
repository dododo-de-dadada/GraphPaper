//------------------------------
// shape_dt.cpp
// �ǂݍ���, ��������.
//------------------------------
#include "pch.h"
#include "shape.h"

namespace winrt::GraphPaper::implementation
{
	//using winrt::Windows::Storage::Streams::DataReader;
	//using winrt::Windows::Storage::Streams::DataWriter;

	// �f�[�^���[�_�[�����邵�̐��@��ǂݍ���.
	void dt_read(ARROW_SIZE& val, DataReader const& dt_reader)
	{
		val.m_width = dt_reader.ReadSingle();
		val.m_length = dt_reader.ReadSingle();
		val.m_offset = dt_reader.ReadSingle();
	}

	// �f�[�^���[�_�[�����邵�̌`����ǂݍ���.
	void dt_read(CAP_STYLE& val, DataReader const& dt_reader)
	{
		val.m_start = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
		val.m_end = static_cast<D2D1_CAP_STYLE>(dt_reader.ReadUInt32());
	}

	// �f�[�^���[�_�[����F��ǂݍ���.
	void dt_read(D2D1_COLOR_F& val, DataReader const& dt_reader)
	{
		val.r = dt_reader.ReadSingle();
		val.g = dt_reader.ReadSingle();
		val.b = dt_reader.ReadSingle();
		val.a = dt_reader.ReadSingle();
		val.r = min(max(val.r, 0.0F), 1.0F);
		val.g = min(max(val.g, 0.0F), 1.0F);
		val.b = min(max(val.b, 0.0F), 1.0F);
		val.a = min(max(val.a, 0.0F), 1.0F);
	}

	// �f�[�^���[�_�[����ʒu��ǂݍ���.
	void dt_read(D2D1_POINT_2F& val, DataReader const& dt_reader)
	{
		val.x = dt_reader.ReadSingle();
		val.y = dt_reader.ReadSingle();
	}

	// �f�[�^���[�_�[���琡�@��ǂݍ���.
	void dt_read(D2D1_RECT_F& val, DataReader const& dt_reader)
	{
		val.left = dt_reader.ReadSingle();
		val.top = dt_reader.ReadSingle();
		val.right = dt_reader.ReadSingle();
		val.bottom = dt_reader.ReadSingle();
	}

	// �f�[�^���[�_�[���琡�@��ǂݍ���.
	void dt_read(D2D1_SIZE_F& val, DataReader const& dt_reader)
	{
		val.width = dt_reader.ReadSingle();
		val.height = dt_reader.ReadSingle();
	}

	// �f�[�^���[�_�[���琡�@��ǂݍ���.
	void dt_read(D2D1_SIZE_U& val, DataReader const& dt_reader)
	{
		val.width = dt_reader.ReadUInt32();
		val.height = dt_reader.ReadUInt32();
	}

	// �f�[�^���[�_�[����j���̔z�u��ǂݍ���.
	void dt_read(DASH_PATT& val, DataReader const& dt_reader)
	{
		val.m_[0] = dt_reader.ReadSingle();
		val.m_[1] = dt_reader.ReadSingle();
		val.m_[2] = dt_reader.ReadSingle();
		val.m_[3] = dt_reader.ReadSingle();
		val.m_[4] = dt_reader.ReadSingle();
		val.m_[5] = dt_reader.ReadSingle();
	}

	// �f�[�^���[�_�[���當����͈͂�ǂݍ���.
	void dt_read(DWRITE_TEXT_RANGE& val, DataReader const& dt_reader)
	{
		val.startPosition = dt_reader.ReadUInt32();
		val.length = dt_reader.ReadUInt32();
	}

	// �f�[�^���[�_�[�������̋�����ǂݍ���.
	void dt_read(GRID_EMPH& val, DataReader const& dt_reader)
	{
		val.m_gauge_1 = dt_reader.ReadUInt16();
		val.m_gauge_2 = dt_reader.ReadUInt16();
		if (equal(val, GRID_EMPH_0) || equal(val, GRID_EMPH_2) || equal(val, GRID_EMPH_3)) {
			return;
		}
		val = GRID_EMPH_0;
	}

	// �f�[�^���[�_�[���瑽�p�`�̑I������ǂݍ���.
	/*
	void dt_read(POLY_OPTION& val, DataReader const& dt_reader)
	{
		val.m_vertex_cnt = dt_reader.ReadUInt32();
		val.m_regular = dt_reader.ReadBoolean();
		val.m_vertex_up = dt_reader.ReadBoolean();
		val.m_end_closed = dt_reader.ReadBoolean();
		val.m_clockwise = dt_reader.ReadBoolean();
	}
	*/

	// �f�[�^���[�_�[����ʒu�z���ǂݍ���.
	void dt_read(std::vector<D2D1_POINT_2F>& val, DataReader const& dt_reader)
	{
		const size_t v_cnt = dt_reader.ReadUInt32();	// �v�f��
		val.resize(v_cnt);
		for (size_t i = 0; i < v_cnt; i++) {
			dt_read(val[i], dt_reader);
		}
	}

	// �f�[�^���[�_�[���當�����ǂݍ���.
	void dt_read(wchar_t*& val, DataReader const& dt_reader)
	{
		const size_t n = dt_reader.ReadUInt32();	// ������
		//if (n > -1) {
			val = new wchar_t[n + 1];
			if (val != nullptr) {
				for (size_t i = 0; i < n; i++) {
					val[i] = dt_reader.ReadUInt16();
				}
				val[n] = L'\0';
			}
		//}
		//else {
		//	val = nullptr;
		//}
	}

	// �f�[�^���C�^�[�ɖ�邵�̐��@����������.
	void dt_write(const ARROW_SIZE& val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.m_width);
		dt_writer.WriteSingle(val.m_length);
		dt_writer.WriteSingle(val.m_offset);
	}

	// �f�[�^���C�^�[�ɒ[�̌`������������.
	void dt_write(const CAP_STYLE& val, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(val.m_start));
		dt_writer.WriteUInt32(static_cast<uint32_t>(val.m_end));
	}

	// �f�[�^���C�^�[�ɐF����������.
	void dt_write(const D2D1_COLOR_F& val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.r);
		dt_writer.WriteSingle(val.g);
		dt_writer.WriteSingle(val.b);
		dt_writer.WriteSingle(val.a);
	}

	// �f�[�^���C�^�[�Ɉʒu����������.
	void dt_write(const D2D1_POINT_2F val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.x);
		dt_writer.WriteSingle(val.y);
	}

	// �f�[�^���C�^�[�ɕ��`����������.
	void dt_write(const D2D1_RECT_F val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.left);
		dt_writer.WriteSingle(val.top);
		dt_writer.WriteSingle(val.right);
		dt_writer.WriteSingle(val.bottom);
	}

	// �f�[�^���C�^�[�ɐ��@����������.
	void dt_write(const D2D1_SIZE_F val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.width);
		dt_writer.WriteSingle(val.height);
	}

	// �f�[�^���C�^�[�ɐ��@����������.
	void dt_write(const D2D1_SIZE_U val, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(val.width);
		dt_writer.WriteUInt32(val.height);
	}

	// �f�[�^���C�^�[�ɔj���̔z�u����������.
	void dt_write(const DASH_PATT& val, DataWriter const& dt_writer)
	{
		dt_writer.WriteSingle(val.m_[0]);
		dt_writer.WriteSingle(val.m_[1]);
		dt_writer.WriteSingle(val.m_[2]);
		dt_writer.WriteSingle(val.m_[3]);
		dt_writer.WriteSingle(val.m_[4]);
		dt_writer.WriteSingle(val.m_[5]);
	}

	// �f�[�^���C�^�[�ɕ�����͈͂���������.
	void dt_write(const DWRITE_TEXT_RANGE val, DataWriter const& dt_writer)
	{
		dt_writer.WriteInt32(val.startPosition);
		dt_writer.WriteInt32(val.length);
	}

	// �f�[�^���C�^�[�ɕ���̌`������������.
	void dt_write(const GRID_EMPH val, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt16(val.m_gauge_1);
		dt_writer.WriteUInt16(val.m_gauge_2);
	}

	// �f�[�^���C�^�[�ɑ��p�`�̑I��������������.
	/*
	void dt_write(const POLY_OPTION& val, DataWriter const& dt_writer)
	{
		dt_writer.WriteUInt32(static_cast<uint32_t>(val.m_vertex_cnt));
		dt_writer.WriteBoolean(val.m_regular);
		dt_writer.WriteBoolean(val.m_vertex_up);
		dt_writer.WriteBoolean(val.m_end_closed);
		dt_writer.WriteBoolean(val.m_clockwise);
	}
	*/
	// �f�[�^���C�^�[�Ɉʒu�z�����������.
	void dt_write(const std::vector<D2D1_POINT_2F>& val, DataWriter const& dt_writer)
	{
		const size_t n = val.size();

		dt_writer.WriteUInt32(static_cast<uint32_t>(n));
		for (uint32_t i = 0; i < n; i++) {
			dt_write(val[i], dt_writer);
		}
	}

	// �f�[�^���C�^�[�ɕ��������������
	void dt_write(const wchar_t* val, DataWriter const& dt_writer)
	{
		const uint32_t len = wchar_len(val);

		dt_writer.WriteUInt32(len);
		for (uint32_t i = 0; i < len; i++) {
			dt_writer.WriteUInt16(val[i]);
		}
	}

	// �f�[�^���C�^�[�ɕ��������������
	// �߂�l	�������񂾃o�C�g��.
	size_t dt_write(const char val[], DataWriter const& dt_writer)
	{
		size_t i = 0;
		for (; val[i] != '\0'; i++) {
			dt_writer.WriteByte(val[i]);
		}
		return i;
	}

}