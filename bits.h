//-----------------------------------------------------------------------------
//
//	Bitstream classes.
//
//	Originally seen in Skal's mpeg-4 video codec. Modified by Igor Janos
//
//-----------------------------------------------------------------------------
#ifndef __bits_h__
#define __bits_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

typedef long int SSIZE_T;
typedef struct GUID {
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t  Data4[8];
} GUID;



//-------------------------------------------------------------------------
//
//	Bitstream class
//
//-------------------------------------------------------------------------

class Bitstream
{
public:
	uint32_t	bitbuf;				// bitbuffer
	uint8_t	*buf;				// byte buffer
	int32_t	bits;				// pocet bitov v bitbufferi

public:
	static const int32_t EXP_GOLOMB_MAP[2][48];
	static const int32_t EXP_GOLOMB_MAP_INV[2][48];
	static const int32_t EXP_GOLOMB_SIZE[255];

public:

	// Konstruktory
	Bitstream() : bitbuf(0), buf(NULL), bits(0) { };
	Bitstream(uint8_t *b) : bitbuf(0), buf(b), bits(0) { };
	Bitstream(const Bitstream &b) : bitbuf(b.bitbuf), buf(b.buf), bits(b.bits) { };

	// Operator priradenia = kopia stavu bitstreamu
	Bitstream &operator =(const Bitstream &b) { bitbuf = b.bitbuf; buf = b.buf; bits = b.bits; return *this; };
	Bitstream &operator =(const uint8_t *b) { bitbuf = 0; buf = (uint8_t*)b; bits = 0; return *this; };
	Bitstream &operator =(uint8_t *b) { bitbuf = 0; buf = b; bits = 0; return *this; };

	// Resetovanie stavu
	inline void Init(const uint8_t *b) { bitbuf = 0; buf = (uint8_t*)b; bits = 0; };
	inline void Init(uint8_t *b) { bitbuf = 0; buf = b; bits = 0; };

	// Zistenie stavu bitstreamu
	inline int32_t BitsLeft() { return bits; };
	inline uint32_t BitBuf() { return bitbuf; };
	inline uint8_t *Position() { return buf - (bits/8); };

	// Citanie z bitstreamu
	inline void DumpBits(int32_t n) { bitbuf <<= n; bits -= n; };
	inline uint32_t UBits(int32_t n) { return (uint32_t)(bitbuf >> (32-n)); };
	inline uint32_t UGetBits(int32_t n) { uint32_t val = (uint32_t)(bitbuf >> (32-n)); bitbuf <<= n; bits -= n; return val; };
	inline int32_t SBits(int32_t n) { return (int32_t)(bitbuf >> (32-n)); };
	inline int32_t SGetBits(int32_t n) { int32_t val = (int32_t)(bitbuf >> (32-n)); bitbuf <<= n; bits -= n; return val; };
	inline void Markerbit() { DumpBits(1); }

	// Reading variable length size field for Musepack SV8
	inline int64_t GetMpcSize() {
		int64_t ret=0;
		uint8_t tmp;
		do {
			NeedBits();
			tmp = UGetBits(8);
			ret = (ret<<7) | (tmp&0x7f);
		} while (tmp&0x80);
		return ret;
	}

	// AAC LATM
	inline int64_t LatmGetValue() {
		NeedBits();
		uint8_t bytesForValue = UGetBits(2);
		int64_t value = 0;
		for (int i=0; i<=bytesForValue; i++) {
			NeedBits();
			value <<= 8;
			value |= UGetBits(8);
		}
		return value;
	}

	// Byte alignment
	inline int32_t IsByteAligned() { return !(bits&0x07); };
	inline void ByteAlign() { if (bits&0x07) DumpBits(bits&0x07); };

	// Exp-Golomb Codes
	uint32_t Get_UE();
	int32_t Get_SE();
	int32_t Get_ME(int32_t mode);
	int32_t Get_TE(int32_t range);
	int32_t Get_Golomb(int k);

	inline int32_t Size_UE(uint32_t val)
	{
		if (val<255) return EXP_GOLOMB_SIZE[val];
		int32_t isize=0;
		val++;
		if (val >= 0x10000) { isize+= 32;	val = (val >> 16)-1; }
		if (val >= 0x100)	{ isize+= 16;	val = (val >> 8)-1;  }
		return EXP_GOLOMB_SIZE[val] + isize;
	}

	inline int32_t Size_SE(int32_t val)				{ return Size_UE(val <= 0 ? -val*2 : val*2 - 1); }
	inline int32_t Size_TE(int32_t range, int32_t v)  { if (range == 1) return 1; return Size_UE(v);	}

	// Loading bits...
	inline void NeedBits() { if (bits < 16) { bitbuf |= ((buf[0] << 8) | (buf[1])) << (16-bits); bits += 16; buf += 2; } };
	inline void NeedBits24() { while (bits<24) { bitbuf |= (buf[0] << (24-bits)); buf++; bits+= 8; } };
	inline void NeedBits32() { while (bits<32) { bitbuf |= (buf[0] << (24-bits)); buf++; bits+= 8; } };

	// Bitstream writing
	inline void PutBits(int32_t val, int32_t num) {
		bits += num;
		if (num < 32) val &= (1<<num)-1;
		if (bits > 32) {
			bits -= 32;
			bitbuf |= val >> (bits);
			*buf++ = ( bitbuf >> 24 ) & 0xff;
			*buf++ = ( bitbuf >> 16 ) & 0xff;
			*buf++ = ( bitbuf >>  8 ) & 0xff;
			*buf++ = ( bitbuf >>  0 ) & 0xff;
			bitbuf = val << (32 - bits);
		} else
			bitbuf |= val << (32 - bits);
	}
	inline void WriteBits()	{
		while (bits >= 8) {
			*buf++ = (bitbuf >> 24) & 0xff;
			bitbuf <<= 8;
			bits -= 8;
		}
	}
	inline void Put_ByteAlign_Zero() { int32_t bl=(bits)&0x07; if (bl<8) { PutBits(0,8-bl); } WriteBits(); }
	inline void Put_ByteAlign_One() { int32_t bl=(bits)&0x07; if (bl<8) {	PutBits(0xffffff,8-bl); } WriteBits(); }
};


class CBitStreamReader
{
public:
	// works direct on the buffer and returns the count of stripped bytes
	static int StripEmulationBytes(uint8_t* buf, size_t bufLen);

	CBitStreamReader(const uint8_t* buf, size_t size, bool skipEmulationBytes = true);
	inline uint32_t ByteAligned() const { return m_bitsLeft == 8 ? 1 : 0; }
	inline uint32_t IsEnd() const { return m_p >= m_end ? 1 : 0; }
	inline SSIZE_T GetPos() const { return (m_p - m_start); }
	inline void SetPos(SSIZE_T pos) { m_p = m_start + pos; m_bitsLeft = 8; }
	inline SSIZE_T GetRemaining() { return m_end - m_p; }

	uint32_t ReadU(int n);
	void SkipU(int n);
	uint32_t ReadU1();
	void SkipU1();
	void SkipU8(int n);
	uint32_t ReadU8();
	uint16_t ReadU16();
	uint32_t ReadU32();
	uint32_t ReadUE();
	int32_t ReadSE();

	GUID ReadGUID();

	uint32_t PeekU1();

	inline const uint8_t* Pointer() { return m_p; }
	inline void SkipToEnd() { m_p = m_end; }
	inline void ByteAlign() { if (m_bitsLeft != 8) m_bitsLeft = 0; GotoNextByteIfNeeded(); }

private:
	void GotoNextByteIfNeeded();

	const uint8_t* m_start;
	const uint8_t* m_p;
	const uint8_t* m_end;
	int m_bitsLeft;
	bool m_skipEmulationBytes;
};

#define BITSTREAM_ALLOCATE_STEPPING 4096
struct __bitstream {
    unsigned int *buffer;
    int bit_offset;
    int max_size_in_dword;
} ;
typedef struct __bitstream wbitstream;

void bitstream_start(wbitstream *bs);
void bitstream_end(wbitstream *bs);
void bitstream_put_ui(wbitstream *bs, unsigned int val, int size_in_bits);
void bitstream_put_ue(wbitstream *bs, unsigned int val);
void bitstream_put_se(wbitstream *bs, int val);
void bitstream_byte_aligning(wbitstream *bs, int bit);
void rbsp_trailing_bits(wbitstream *bs);

#endif
