#pragma once
#include <string>
#include <cstdint>

class Buffer {
public:
	Buffer();
	~Buffer();
	
	enum HeadType {
		HEAD_W,
		HEAD_R
	};
	
	enum Location {
		LOC_START,
		LOC_CUR,
		LOC_END
	};

	size_t Seek( HeadType head, Location location, ptrdiff_t bytes );
	
	void WriteBytes( const void* data, size_t bytes );
	
	std::string ReadBytes( size_t bytes );
	size_t ReadBytes( void* dst, size_t bytes );
private:
	std::string m_container;
	size_t m_writehead;
	size_t m_readhead;
	size_t m_written;
};