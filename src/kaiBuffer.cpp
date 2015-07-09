#include "kaiDefinition.h"
#include <memory.h>
#include "kaiBuffer.h"

kaiBuffer::kaiBuffer() : _buffer(0), _bufferInPtr(0),_bufferOutPtr(0),
						_bufferEnd(0), _bufferSize(0), _maxDataSize(0)
{

}

kaiBuffer::~kaiBuffer()
{
	delete[] _buffer;
}

void kaiBuffer::init(unsigned buffersize /* = kaiBUFFER_SIZE */, unsigned maxdatasize /* = kaiMAX_MSG_SIZE */)
{
	_buffer = new char[buffersize + maxdatasize];
    memset(_buffer, 0, buffersize + maxdatasize);
	_bufferSize = buffersize;
	_maxDataSize = maxdatasize;

	_bufferInPtr = _bufferOutPtr = _buffer;
	_bufferEnd = _buffer + _bufferSize;
}

void kaiBuffer::refresh()
{
	_bufferInPtr = _bufferOutPtr = _buffer;
}

char* kaiBuffer::bufferIn()
{
	return _bufferInPtr;
}

const char* kaiBuffer::bufferOut() const
{
	return _bufferOutPtr;
}



unsigned kaiBuffer::getDataSize()
{
	int size = _bufferInPtr - _bufferOutPtr;
	if (size < 0)
		size += _bufferSize;
	return size;
}

void kaiBuffer::put(const void* data, unsigned short size)
{
	if (size > kaiMAX_MSG_SIZE)
		return;
	
	memcpy(_bufferInPtr, data, size);
	_bufferInPtr += size;

	if (_bufferInPtr >= _bufferEnd)
	{
		unsigned extrasize = _bufferInPtr - _bufferEnd;
		memcpy(_buffer, _bufferEnd, extrasize);
		_bufferInPtr -= _bufferSize;
	}
}

void kaiBuffer::put(unsigned short size)
{
	if (size > kaiMAX_MSG_SIZE)
		return;

	_bufferInPtr += size;
	if (_bufferInPtr >= _bufferEnd)
	{
		unsigned extrasize = _bufferInPtr - _bufferEnd;
		memcpy(_buffer, _bufferEnd, extrasize);
		_bufferInPtr -= _bufferSize;
	}
}

void kaiBuffer::get(void* data, unsigned short size)
{
	if (size > kaiMAX_MSG_SIZE)
		return;
	
	int extrasize = _bufferOutPtr + size - _bufferEnd;
	if (extrasize > 0)
		memcpy(_bufferEnd, _bufferInPtr, extrasize);
	
	memcpy(data, _bufferOutPtr, size);
	_bufferOutPtr += size;

	if (extrasize >= 0)
		_bufferOutPtr -= _bufferSize;

}

void kaiBuffer::pop(unsigned short size)
{
	if (size > kaiMAX_MSG_SIZE)
		return;
	
	_bufferOutPtr += size;
	if (_bufferOutPtr >= _bufferEnd)
		_bufferOutPtr -= _bufferSize;
}