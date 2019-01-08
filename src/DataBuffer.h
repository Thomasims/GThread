#pragma once

#include <stdint.h>
#include <vector>

enum Location {
	LOC_START,
	LOC_CUR,
	LOC_END
};

class DataBuffer
{
public:

	DataBuffer();
	DataBuffer(const DataBuffer&);
	DataBuffer(uint32_t blocksize);
	~DataBuffer();

	void Allocate(uint64_t pos);
	void PrepareWrite(uint64_t length);
	void Wrote(uint64_t length);
	
	void WriteUInt(uint64_t data, uint8_t bits);
	void WriteData(void* data, uint32_t bytes);

	uint64_t SeekWrite(Location location, int64_t dist);
	uint64_t SeekRead(Location location, int64_t dist);

private:

	const size_t m_blocksize;
	std::vector<char*> m_blocks;

	uint64_t m_writehead;
	uint64_t m_readhead;
	uint64_t m_written;
};

