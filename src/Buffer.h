#ifndef BUFFER_H
#define BUFFER_H

#include <string>
#include <cstdint>

class Buffer {
public:
	Buffer();
	~Buffer();

	size_t Seek( int head, int location, std::ptrdiff_t bytes );
	
	void WriteBytes( const void* data, size_t bytes );
	
	std::string ReadBytes( size_t bytes );
	size_t ReadBytes( void* dst, size_t bytes );

	size_t Written();
	void Clear();
	size_t Slice( size_t start, size_t len );
private:
	std::string m_container;
	size_t m_writehead;
	size_t m_readhead;
	size_t m_written;
};

#endif // BUFFER_H