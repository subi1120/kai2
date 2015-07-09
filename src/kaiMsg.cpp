#include "kaiMsg.h"

using namespace std;


kaiMsg::kaiMsg() : _buf(0), _pos(0), _bufend(0), _header(), _bAllocated(false)
{
	
}

kaiMsg::kaiMsg(const kaiMsg &msg)
{
	if(msg._bAllocated)
	{
		allocateMemory(msg.dataSize());
		memcpy(_buf, msg.buffer(), msg.dataSize());
		_header = msg._header;
		_bAllocated = msg._bAllocated;
	}
	else
	{
		_header = msg._header;
		_buf = msg._buf;
		_pos = msg._pos;
		_bufend = msg._bufend;
		_bAllocated = msg._bAllocated;
	}
}

kaiMsg::~kaiMsg()
{
	if (_bAllocated)
		delete[] _buf;
}

void kaiMsg::reset()
{
	_pos = _buf;
	memset(&_header, 0, Size_kaiHEADER);
}

void kaiMsg::allocateMemory(kaiMsgSize size)
{
	_buf = new char[size];
	_bufend = _buf +  size;
	_pos = _buf;
	_bAllocated = true;
}

void kaiMsg::setBuffer(char* buf, int size)
{
	_buf = buf;
	_bufend = _buf +  size;
	_pos = _buf;
	_bAllocated = false;
}

void kaiMsg::begin()
{
	_pos = _buf+sizeof(kaiHeader);
}

void kaiMsg::end()
{
	_header.size = (kaiMsgSize)(_pos - (_buf+sizeof(kaiHeader)));	
	memcpy(_buf, &_header, sizeof(kaiHeader));
}

kaiMsgID kaiMsg::id() const
{
	return _header.id;
}

kaiMsgSize kaiMsg::size() const
{
	return _header.size;
}

kaiFlag	 kaiMsg::flag() const
{
	return _header.flag;
}

void kaiMsg::id(kaiMsgID id)
{
	_header.id = id;
}

void kaiMsg::size(kaiMsgSize size)
{
	_header.size = size;
}

void kaiMsg::flag(kaiFlag flag)
{
	_header.flag = flag;
}

const char* kaiMsg::buffer() const 
{
	return _buf;
}

kaiMsgSize kaiMsg::getReadSize() const
{
	return (kaiMsgSize)(_pos - (_buf + Size_kaiHEADER));
}

void kaiMsg::writeData(const void* data, kaiMsgSize size)
{
	if (_pos + size > _bufend)
		return;		
	memcpy(_pos, data, size);
	_pos += size;
}

void kaiMsg::readData(void* data, kaiMsgSize size)
{
	if (_pos + size > _bufend)
		return;

	memcpy(data, _pos, size);
	_pos += size;
}

unsigned kaiMsg::dataSize() const
{
	return (_header.size + Size_kaiHEADER);
}

int kaiMsg::writeFloat(float val)
{
	size_t size = sizeof(float);
	if (_pos + size > _bufend)
		return 0;

	memcpy(_pos, &val, size);
	_pos += size;
	return size;
}

int kaiMsg::readFloat(float& val)
{
	size_t size = sizeof(float);
	if (_pos + size > _bufend)
		return 0;

	memcpy(&val, _pos, size);
	_pos += size;
	return size;
}

int kaiMsg::writeInt(int val)
{
	size_t size = sizeof(int);
	if (_pos + size > _bufend)
		return 0;

	memcpy(_pos, &val, size);
	_pos += size;
	return size;
}

int kaiMsg::readInt(int& val)
{
	size_t size = sizeof(int);
	if (_pos + size > _bufend)
		return 0;

	memcpy(&val, _pos, size);
	_pos += size;
	return size;
}

int kaiMsg::writeShort(short val)
{
	size_t size = sizeof(short);
	if (_pos + size > _bufend)
		return 0;

	memcpy(_pos, &val, size);
	_pos += size;
	return size;
}

int kaiMsg::readShort(short& val)
{
	size_t size = sizeof(short);
	if (_pos + size > _bufend)
		return 0;

	memcpy(&val, _pos, size);
	_pos += size;
	return size;
}

int kaiMsg::writeByte(unsigned char val)
{
	size_t size = sizeof(unsigned char);
	if (_pos + size > _bufend)
		return 0;

	memcpy(_pos, &val, size);
	_pos += size;
	return size;
}

int kaiMsg::readByte(unsigned char& val)
{
	size_t size = sizeof(unsigned char);
	if (_pos + size > _bufend)
		return 0;

	memcpy(&val, _pos, size);
	_pos += size;
	return size;
}
