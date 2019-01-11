#include "Buffer.hpp"

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

size_t Buffer::Seek( HeadType head, Location location, ptrdiff_t bytes ) {
	size_t* p_head = NULL;
	switch ( head ) {
	case HeadType::HEAD_W:
		p_head = &m_writehead;
		break;
	case HeadType::HEAD_R:
		p_head = &m_readhead;
		break;
	default:
		return 0;
	}

	switch ( location ) {
	case Location::LOC_START:
		if ( bytes < 0 )
			return *p_head = 0;
		else if ( bytes > ptrdiff_t( m_written ) )
			return *p_head = m_written;
		return *p_head = bytes;
	case Location::LOC_CUR:
		if ( -bytes > ptrdiff_t( *p_head ) )
			return *p_head = 0;
		else if ( bytes > ptrdiff_t( m_written - m_writehead ) )
			return *p_head = m_written;
		return *p_head = *p_head + bytes;
	case Location::LOC_END:
		if ( bytes > 0 )
			return *p_head = m_written;
		else if ( -bytes > ptrdiff_t( m_written ) )
			return *p_head = 0;
		return *p_head = m_written + bytes;
	default:
		return 0;
	}
}