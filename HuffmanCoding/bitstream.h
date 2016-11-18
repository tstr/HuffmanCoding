/*
 	Bitstream class
 
 	A class for manipulating streams of bits instead of streams of bytes,
 	bits are stored from left->right instead of right->left
*/

#pragma once

#include <algorithm>
#include <vector>
#include <ostream>

//////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct bitSizeOf
{
	static const size_t value = (sizeof(T) * (size_t)CHAR_BIT);
};

class BitStream
{
public:

	typedef unsigned char byte_t;
	typedef bool bit_t;
	typedef size_t bitpos_t;

	//Width of a single byte, in bits
	enum { bytewidth = CHAR_BIT };

	BitStream() :
		BitStream::BitStream(10 * bytewidth)
	{}

	//Reserve a certain number of bits
	BitStream(size_t reservebits) :
		m_bufferSize(0) //zero initialize the buffersize
	{
		m_buffer.resize(calcByteCount(reservebits));
	}

	BitStream(const byte_t* bytes, size_t numbits) :
		BitStream::BitStream(numbits)
	{
		m_bufferSize = numbits;
		m_buffer = std::vector<byte_t>(bytes, bytes + calcByteCount(numbits));
	}

	//Write part of a byte to the bitstream
	template<typename type_t>
	void write(type_t data, bitpos_t bitoffset = 0, bitpos_t bitcount = bitSizeOf<type_t>::value)
	{
		bitpos_t width = bitSizeOf<type_t>::value;

		bitpos_t bitof = (bitoffset >= width) ? 0 : bitoffset;
		bitpos_t bitsz = (bitcount > width) ? width : bitcount;

		for (bitpos_t bit = bitof; bit < bitsz; bit++)
		{
			this->writebit((data & (type_t)(1 << (width - (bit + 1)))) != 0);
		}
	}

	//Read part of the bitstream to a byte
	bool read(byte_t& data, bitpos_t bitpos = 0, bitpos_t bitcount = bitSizeOf<byte_t>::value)
	{
		for (size_t i = bitpos; i < bitcount; i++)
		{
			bit_t bit = 0;
			if (!readbit(bit))
				return false;

			data |= (bitpos_t(bit) << (bytewidth - (i + 1)));
		}

		return true;
	}

	//Writes a single bit to the bitstream
	void writebit(bit_t bit)
	{
		//reallocate the bitstream if there is not enough space
		if ((m_bufferWrite + 1) > (m_buffer.size() * bytewidth))
			realloc();

		bitpos_t& writeptr = m_bufferWrite;

		//Offset of the write pointer in bytes from the start of the buffer
		const size_t index = writeptr / bytewidth;
		//Offset of the write pointer in bits from the start of the nearest byte
		const size_t offset = bytewidth - ((writeptr % bytewidth) + 1);

		m_buffer[index] |= ((size_t)bit * (1 << offset));

		//Increment capacity and the write pointer
		writeptr++;
		m_bufferSize++;
	}

	bool readbit(bit_t& bit)
	{
		bitpos_t& readptr = m_bufferRead;

		if (readptr >= (m_buffer.size() * bytewidth))
			return false;

		//Offset of the read pointer in bytes from the start of the buffer
		const size_t index = readptr / bytewidth;
		//Offset of the read pointer in bits from the start of the nearest byte
		const size_t offset = bytewidth - ((readptr % bytewidth) + 1);

		bit = (m_buffer[index] & (1 << offset)) != 0;

		readptr++;

		return true;
	}

	const byte_t* getBitBuffer() const { return &m_buffer[0]; }
	size_t getBitCount() const { return m_bufferSize; }
	size_t getByteCount() const { return calcByteCount(m_bufferSize); }

	void copyBitBuffer(std::ostream& stream) const
	{
		stream.write((const char*)&m_buffer[0], getByteCount());
	}

	void clear()
	{
		m_buffer.resize(1);
		resetWrite();
		resetRead();
	}

	//Get read/write pointers
	bitpos_t getWrite() const { return m_bufferWrite; }
	bitpos_t getRead() const { return m_bufferRead; }

private:

	void resetWrite() { m_bufferWrite = 0; }
	void resetRead() { m_bufferRead = 0; }

	size_t calcByteCount(size_t bitCount) const
	{
		size_t bytecount = (bitCount / bytewidth);
		if (bitCount % bytewidth)
			bytecount++;

		return bytecount;
	}

	void realloc()
	{
		//Double the capacity
		m_buffer.resize(m_buffer.size() * 2);
	}

	bitpos_t m_bufferWrite = 0;	//Write offset in bits, from the start of the buffer
	bitpos_t m_bufferRead = 0;	//Read offset in bits, from the start of the buffer
	bitpos_t m_bufferSize = 0; //Number of bits in the buffer, this does not necessarily equal m_buffer.size() * bytewidth
	std::vector<byte_t> m_buffer;
};
