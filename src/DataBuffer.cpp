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

void DataBuffer::PrepareWrite(uint64_t length) {
	Allocate((m_writehead + length - 1) / (m_blocksize * 8));
}

void DataBuffer::Wrote(uint64_t length) {
	m_writehead += length;
	if (m_writehead > m_written)
		m_written = m_writehead;
}


void DataBuffer::WriteUInt(uint64_t data, uint8_t bits) {
	PrepareWrite(bits);

	Wrote(bits);
}

void DataBuffer::WriteData(void* data, uint32_t bytes) {
	if (m_writehead % 8)
		SeekWrite(Location::LOC_CUR, 8 - m_writehead % 8);
	PrepareWrite(bytes * 8);

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