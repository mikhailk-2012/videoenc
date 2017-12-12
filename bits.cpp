//-----------------------------------------------------------------------------
//
//	Musepack Demuxer
//
//	Author : Igor Janos
//
//-----------------------------------------------------------------------------
#include "bits.h"

//-----------------------------------------------------------------------------
//
//	Bitstream class
//
//	podporuje citanie bitstreamu
//
//----------------------------------------------------------------------------

const int32_t Bitstream::EXP_GOLOMB_MAP[2][48] =
{
		{
				47, 31, 15,  0, 23, 27, 29, 30,
				7, 11, 13, 14, 39, 43, 45, 46,
				16,  3,  5, 10, 12, 19, 21, 26,
				28, 35, 37, 42, 44,  1,  2,  4,
				8, 17, 18, 20, 24,  6,  9, 22,
				25, 32, 33, 34, 36, 40, 38, 41
		},
		{
				0, 16,  1,  2,  4,  8, 32,  3,
				5, 10, 12, 15, 47,  7, 11, 13,
				14,  6,  9, 31, 35, 37, 42, 44,
				33, 34, 36, 40, 39, 43, 45, 46,
				17, 18, 20, 24, 19, 21, 26, 28,
				23, 27, 29, 30, 22, 25, 38, 41
		}
};

const int32_t Bitstream::EXP_GOLOMB_MAP_INV[2][48] =
{
	{
			3, 29, 30, 17, 31, 18, 37,  8,
			32, 38, 19,  9, 20, 10, 11,  2,
			16, 33, 34, 21, 35, 22, 39,  4,
			36, 40, 23,  5, 24,  6,  7,  1,
			41, 42, 43, 25, 44, 26, 46, 12,
			45, 47, 27, 13, 28, 14, 15,  0
	},
	{
			0,  2,  3,  7,  4,  8, 17, 13,
			5, 18,  9, 14, 10, 15, 16, 11,
			1, 32, 33, 36, 34, 37, 44, 40,
			35, 45, 38, 41, 39, 42, 43, 19,
			6, 24, 25, 20, 26, 21, 46, 28,
			27, 47, 22, 29, 23, 30, 31, 12 
	}
};


// Exp-Golomb Codes

const int32_t Bitstream::EXP_GOLOMB_SIZE[255] =
{
	1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
	11,11,11,11,11,11,11,11,11,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
	13,13,13,13,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
};

uint32_t Bitstream::Get_UE()
{
	int32_t len = 0;
	NeedBits24();
	while (!UGetBits(1)) len++;
	NeedBits24();
	return (len == 0 ? 0 : (1<<len)-1 + UGetBits(len));
}

int32_t Bitstream::Get_SE()
{
	int32_t len = 0;
	NeedBits24();
	while (!UGetBits(1)) len++;
	if (len == 0) return 0;
	NeedBits24();
	int32_t val = (1 << len) | UGetBits(len);
	return (val&1) ? -(val>>1) : (val>>1);
}

int32_t Bitstream::Get_ME(int32_t mode)
{
	// nacitame UE a potom podla mapovacej tabulky
	int32_t codeNum = Get_UE();
	if (codeNum >= 48) return -1;		// chyba
	return EXP_GOLOMB_MAP[mode][codeNum];
}

int32_t Bitstream::Get_TE(int32_t range)
{
	/* ISO/IEC 14496-10 - Section 9.1 */
	if (range > 1) {
		return Get_UE();
	} else {
		return (!UGetBits(1))&0x01;
	}
}

int32_t Bitstream::Get_Golomb(int k)
{
	int32_t l=0;
	NeedBits();
	while (UBits(8) == 0) {
		l += 8;
		DumpBits(8);
		NeedBits();
	}
	while (UGetBits(1) == 0) l++;
	NeedBits();
	return (l << k) | UGetBits(k);
}




int CBitStreamReader::StripEmulationBytes(uint8_t* buf, size_t bufLen)
{
	if (buf == NULL || bufLen < 4) return 0;

	int strippedBytes = 0;
	int lastBytesNull = 0;
	for (size_t i=0; i<bufLen; i++)
	{

		if (buf[i] == 0x03 && lastBytesNull == 2)
		{
			strippedBytes++;
			lastBytesNull = 0;
		}
		else
		{
			if (buf[i] == 0)
				lastBytesNull++;
			else
				lastBytesNull = 0;

			if (strippedBytes)
				buf[i-strippedBytes] = buf[i];
		}
	}

	return strippedBytes;
}

CBitStreamReader::CBitStreamReader(const uint8_t* buf, size_t size, bool skipEmulationBytes)
: m_start(buf), m_p(buf), m_end(buf + size), m_bitsLeft(8), m_skipEmulationBytes(skipEmulationBytes)
{
}

void CBitStreamReader::GotoNextByteIfNeeded()
{
	if (m_bitsLeft == 0)
	{
		m_p++;
		if (GetPos() > 2 && m_skipEmulationBytes)
		{
			if (*(m_p-2) == 0 && *(m_p-1) == 0 && *m_p == 0x03)
			{
				// skip emulation byte
				m_p++;
			}
		}

		m_bitsLeft = 8;
	}
}

uint32_t CBitStreamReader::ReadU1()
{
	uint32_t r = 0;
	if (IsEnd()) return 0;

	m_bitsLeft--;
	r = ((*(m_p)) >> m_bitsLeft) & 0x01;

	GotoNextByteIfNeeded();

	return r;
}

uint32_t CBitStreamReader::ReadU8()
{
	uint32_t r = 0;
	if (IsEnd()) return 0;

	if (m_bitsLeft == 8)
	{
		// optimized reading when byte aligned
		r = *(m_p);
		m_bitsLeft = 0;

		GotoNextByteIfNeeded();
	}
	else
		r = ReadU(8);

	return r;
}

void CBitStreamReader::SkipU1()
{
	if (IsEnd()) return;

	m_bitsLeft--;
	GotoNextByteIfNeeded();
}

void CBitStreamReader::SkipU8(int n=1)
{
	if (IsEnd()) return;

	if (m_p + n >= m_end)
	{
		m_p = m_end;
		m_bitsLeft = 0;
		return;
	}

	int bitsLeft = m_bitsLeft;

	for (int i=0; i<n; i++)
	{
		m_bitsLeft = 0;
		GotoNextByteIfNeeded();
	}

	m_bitsLeft = bitsLeft;
}

uint32_t CBitStreamReader::ReadU(int n)
{
	uint32_t r = 0;
	int i;
	for (i = 0; i < n; i++)
	{
		r |= ( ReadU1() << ( n - i - 1 ) );
	}
	return r;
}

void CBitStreamReader::SkipU(int n)
{
	for (int i=0; i<n; i++)
		SkipU1();
}

uint16_t CBitStreamReader::ReadU16()
{
	uint16_t r = ReadU(8) << 8;
	r |= ReadU8();
	return r;
}

uint32_t CBitStreamReader::ReadU32()
{
	uint32_t r = ReadU(8) << 8;
	r |= ReadU8();
	r <<= 8;
	r |= ReadU8();
	r <<= 8;
	r |= ReadU8();
	return r;
}

uint32_t CBitStreamReader::ReadUE()
{
	uint32_t r = 0;
	int i = 0;

	while( ReadU1() == 0 && i < 32 && !IsEnd() )
	{
		i++;
	}
	r = ReadU(i);
	r += (1 << i) - 1;
	return r;
}

int32_t CBitStreamReader::ReadSE()
{
	int32_t r = ReadUE();
	if (r & 0x01)
	{
		r = (r+1)/2;
	}
	else
	{
		r = -(r/2);
	}
	return r;
}

GUID CBitStreamReader::ReadGUID()
{
	if (GetRemaining() < sizeof(GUID))
		return GUID();

	GUID guid = { 0 };
	guid.Data1 = ReadU32();
	guid.Data2 = ReadU16();
	guid.Data3 = ReadU16();

	for (int i = 0; i < 8; i++)
		guid.Data4[i] = ReadU8();

	return guid;
}

uint32_t CBitStreamReader::PeekU1()
{
	int32_t r = 0;
	if (IsEnd())
	{
		// whast is with m_skipEmulationBytes in this case?
				r = ((*(m_p)) >> (m_bitsLeft-1)) & 0x01;
	}
	return r;
}

static unsigned int
va_swap32(unsigned int val)
{
    unsigned char *pval = (unsigned char *)&val;

    return ((pval[0] << 24)     |
            (pval[1] << 16)     |
            (pval[2] << 8)      |
            (pval[3] << 0));

}

void bitstream_start(wbitstream *bs)
{
    bs->max_size_in_dword = BITSTREAM_ALLOCATE_STEPPING;
    bs->buffer = (unsigned int *)calloc(bs->max_size_in_dword * sizeof(int), 1);
    bs->bit_offset = 0;
}

void bitstream_end(wbitstream *bs)
{
    int pos = (bs->bit_offset >> 5);
    int bit_offset = (bs->bit_offset & 0x1f);
    int bit_left = 32 - bit_offset;

    if (bit_offset) {
        bs->buffer[pos] = va_swap32((bs->buffer[pos] << bit_left));
    }
}

void bitstream_put_ui(wbitstream *bs, unsigned int val, int size_in_bits)
{
	int pos = (bs->bit_offset >> 5);
	int bit_offset = (bs->bit_offset & 0x1f);
	int bit_left = 32 - bit_offset;

	if (!size_in_bits)
		return;

	bs->bit_offset += size_in_bits;

	if (size_in_bits < 32)
		val &= (1<<size_in_bits) -1;


	if (bit_left > size_in_bits) {
		bs->buffer[pos] = ((bs->buffer[pos] << size_in_bits) | val);
	} else {
		size_in_bits -= bit_left;
		bs->buffer[pos] = (bs->buffer[pos] << bit_left) | (val >> size_in_bits);
		bs->buffer[pos] = va_swap32(bs->buffer[pos]);

		if (pos + 1 == bs->max_size_in_dword) {
			bs->max_size_in_dword += BITSTREAM_ALLOCATE_STEPPING;
			bs->buffer = (unsigned int *)realloc(bs->buffer, bs->max_size_in_dword * sizeof(unsigned int));
		}

		bs->buffer[pos + 1] = val & ((1<<size_in_bits) -1);
	}
}

void bitstream_put_ue(wbitstream *bs, unsigned int val)
{
    int size_in_bits = 0;
    int tmp_val = ++val;

    while (tmp_val) {
        tmp_val >>= 1;
        size_in_bits++;
    }

    bitstream_put_ui(bs, 0, size_in_bits - 1); // leading zero
    bitstream_put_ui(bs, val, size_in_bits);
}

void bitstream_put_se(wbitstream *bs, int val)
{
    unsigned int new_val;

    if (val <= 0)
        new_val = -2 * val;
    else
        new_val = 2 * val - 1;

    bitstream_put_ue(bs, new_val);
}

void bitstream_byte_aligning(wbitstream *bs, int bit)
{
    int bit_offset = (bs->bit_offset & 0x7);
    int bit_left = 8 - bit_offset;
    int new_val;

    if (!bit_offset)
        return;

    //assert(bit == 0 || bit == 1);

    if (bit)
        new_val = (1 << bit_left) - 1;
    else
        new_val = 0;

    bitstream_put_ui(bs, new_val, bit_left);
}

void rbsp_trailing_bits(wbitstream *bs)
{
    bitstream_put_ui(bs, 1, 1);
    bitstream_byte_aligning(bs, 0);
}
