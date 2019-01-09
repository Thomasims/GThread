#include "DataBuffer.h"



DataBuffer::DataBuffer()
	: m_blocksize(1024)
	, m_writehead(0)
	, m_readhead(0)
	, m_written(0)
{
}

DataBuffer::DataBuffer(const DataBuffer& other)
	: m_blocksize(other.m_blocksize)
	, m_writehead(other.m_writehead)
	, m_readhead(other.m_readhead)
	, m_written(other.m_written)
{
	for (const char* ptr : other.m_blocks) {
		char* newptr = new char[m_blocksize];
		memcpy(newptr, ptr, m_blocksize);
		m_blocks.push_back(newptr);
	}
}

DataBuffer::DataBuffer(uint32_t blocksize)
	: m_blocksize(blocksize)
	, m_writehead(0)
	, m_readhead(0)
	, m_written(0)
{
}

DataBuffer::~DataBuffer()
{
	for (char* ptr : m_blocks) {
		delete ptr;
	}
}


void DataBuffer::Allocate(uint64_t pos) {
	for (uint64_t i = m_blocks.size(); i <= pos; i++) {
		char* newptr = new char[m_blocksize];
		memset(newptr, 0, m_blocksize);
		m_blocks.push_back(newptr);
	}
}

uint64_t DataBuffer::PrepareWrite(uint64_t length) {
	Allocate((m_writehead + length - 1) / m_blocksizebits);
	return m_writehead / m_blocksizebits;
}

void DataBuffer::Wrote(uint64_t length) {
	m_writehead += length;
	if (m_writehead > m_written)
		m_written = m_writehead;
}

// This function is insane, I should really use a lib.
void DataBuffer::WriteUInt(uint64_t data, uint8_t bits) {
	size_t localhead = m_writehead % m_blocksizebits;
	size_t spaceinblock = m_blocksizebits - localhead;
	if ( spaceinblock < bits ) {
		uint8_t leftover = bits - spaceinblock;
		WriteUInt( data >> leftover, spaceinblock );
		WriteUInt( data, leftover );
		return;
	}
	uint64_t blockid = PrepareWrite(bits);
	char* cstart = m_blocks[blockid] + localhead / 8;

	uint8_t soffset = (8 - localhead) % 8;
	if ( soffset > bits ) {
		*cstart = (*cstart & (-1 << soffset)) | data << (soffset - bits) | (*cstart & ~(-1 << (soffset - bits)));
		return Wrote( bits );
	}
	if ( soffset ) {
		*cstart = (*cstart & (-1 << soffset)) | data >> bits - soffset;
		++cstart;
		bits -= soffset;
	}
	uint8_t wholebytes = bits / 8;
	uint8_t eoffset = bits - (wholebytes * 8);
	if ( wholebytes ) {
		uint64_t temp = data >> eoffset;
		memcpy( cstart, &temp, wholebytes );
		cstart += wholebytes;
	}
	if ( eoffset ) {
		eoffset = 8 - eoffset;
		*cstart = *cstart & ~(-1 << eoffset) | data << eoffset;
	}

	Wrote(bits);
}

void DataBuffer::WriteData(void* data, uint32_t bytes) {
	uint8_t offset = m_writehead % 8;
	if (offset)
		SeekWrite(Location::LOC_CUR, 8 - offset);
	uint64_t blockid = PrepareWrite(bytes * 8);
	size_t localhead = ( m_writehead % m_blocksizebits ) / 8;
	size_t spaceinblock = m_blocksize - localhead;
	char* cstart = m_blocks[blockid] + localhead;

	while ( bytes ) {
		if ( bytes < spaceinblock ) {
			memcpy( cstart, data, bytes );
			bytes = 0;
		} else {
			memcpy( cstart, data, spaceinblock );
			bytes -= spaceinblock;
			data = (char*) data + spaceinblock;
			cstart = m_blocks[++blockid];
			spaceinblock = m_blocksize;
		}
	}

	Wrote(bytes * 8);
}


uint64_t DataBuffer::SeekWrite(Location location, int64_t dist) {
	switch (location) {
	case Location::LOC_START:
		return m_writehead = dist;
	case Location::LOC_CUR:
		return m_writehead += dist;
	case Location::LOC_END:
		return m_writehead = (m_written + dist);
	}
	return m_writehead;
}

uint64_t DataBuffer::SeekRead(Location location, int64_t dist) {
	switch (location) {
	case Location::LOC_START:
		return m_readhead = dist;
	case Location::LOC_CUR:
		return m_readhead += dist;
	case Location::LOC_END:
		return m_readhead = (m_written + dist);
	}
	return m_readhead;
}