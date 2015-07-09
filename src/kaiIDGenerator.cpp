#include "kaiDefinition.h"
#include "kaiIDGenerator.h"

kaiIDGenerator::kaiIDGenerator(unsigned short size)
{
	unsigned short i;

	_nodeMemory = new Node[size];
	_size = size;

	for (i=0; i<size-1; i++)
	{
		_nodeMemory[i].id = i;
		_nodeMemory[i].next = &_nodeMemory[i+1];

	}

	_nodeMemory[i].id = i;
	_nodeMemory[i].next = 0;

	_beginNode = &_nodeMemory[0];
	_endNode = &_nodeMemory[i];
}

kaiIDGenerator::~kaiIDGenerator()
{
	delete[] _nodeMemory;
}

unsigned short kaiIDGenerator::getID()
{
	unsigned short id = _beginNode->id;
	_beginNode = _beginNode->next;
	return id;
}

void kaiIDGenerator::releaseID(unsigned short id)
{
	if (id >= _size)
		return;

	Node* node = &_nodeMemory[id];
	_endNode->next = node;
	node->next = 0;

	_endNode = node;
}

