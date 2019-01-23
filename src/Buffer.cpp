#include "Buffer.h"
#include <string.h>

Buffer::Buffer()
: m_writehead{ 0 }
, m_readhead{ 0 }
, m_written{ 0 }
{
}

Buffer::~Buffer()
{
}

void Buffer::WriteBytes( const void* data, size_t bytes ) {
	size_t diff = m_written - m_writehead;
	if ( bytes > diff ) {
		if ( diff )
			m_container.replace( m_writehead, diff, (const char*) data, diff );
		m_container.append( (const char*) data + diff, bytes - diff );
		m_writehead += bytes;
		m_written = m_writehead;
	}
	else {
		m_container.replace( m_writehead, bytes, (const char*) data, bytes );
		m_writehead += bytes;
	}
}

std::string Buffer::ReadBytes( size_t bytes ) {
	std::string str = m_container.substr( m_readhead, bytes );
	m_readhead += str.length();
	return str;
}

size_t Buffer::ReadBytes( void* dst, size_t bytes ) {
	if ( m_written < m_readhead + bytes ) return 0;
	memcpy( dst, m_container.data() + m_readhead, m_written - m_readhead );
	m_readhead += bytes;
	return bytes;
}

size_t Buffer::Seek( int head, int location, std::ptrdiff_t bytes ) {
	size_t* p_head = NULL;
	switch ( head ) {
	case 0:
		p_head = &m_writehead;
		break;
	case 1:
		p_head = &m_readhead;
		break;
	default:
		return 0;
	}

	switch ( location ) {
	case 0:
		if ( bytes < 0 )
			return *p_head = 0;
		else if ( bytes > std::ptrdiff_t( m_written ) )
			return *p_head = m_written;
		return *p_head = bytes;
	case 1:
		if ( -bytes > std::ptrdiff_t( *p_head ) )
			return *p_head = 0;
		else if ( bytes > std::ptrdiff_t( m_written - m_writehead ) )
			return *p_head = m_written;
		return *p_head = *p_head + bytes;
	case 2:
		if ( bytes > 0 )
			return *p_head = m_written;
		else if ( -bytes > std::ptrdiff_t( m_written ) )
			return *p_head = 0;
		return *p_head = m_written + bytes;
	default:
		return 0;
	}
}

size_t Buffer::Written() {
	return m_written;
}

void Buffer::Clear() {
	m_container.clear();
	m_readhead = 0;
	m_writehead = 0;
	m_written = 0;
}

size_t Buffer::Slice( size_t start, size_t len ) {
	if ( start > m_written ) start = m_written;

	m_container = m_container.substr( start, len );

	m_written = m_container.size();

	if ( m_writehead < start ) m_writehead = 0;
	else if ( (m_writehead -= start) > m_written ) m_writehead = m_written;

	if ( m_readhead < start ) m_readhead = 0;
	else if ( (m_readhead -= start) > m_written ) m_readhead = m_written;

	return m_written;
}