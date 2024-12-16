#include "RingBuffer.h"
#include <memory.h>

#pragma warning(disable : 4700)

// Ư�̻��� : ���� ť�̱� ������ �Ҵ� ũ�Ⱑ BUFFER_SIZE ���� 1 Ŀ����
#pragma warning(disable : 26495)
RingBuffer::RingBuffer(void)
{
	iOutPos_ = iInPos_ = 0;
}
#pragma warning(default : 26495)

RingBuffer::~RingBuffer(void)
{
}

// Return:  (int) ���� Buffer���� Enqueue�Ǿ��ִ� ũ��
int RingBuffer::GetUseSize(void)
{
	int iRet;
	int in = iInPos_;
	int out = iOutPos_;
	GetUseSize_MACRO(in, out, iRet);
	return iRet;
}

// Return:(int)���� Buffer�� �߰��� Enequeue ������ ũ��
int RingBuffer::GetFreeSize(void)
{
	int iRet;
	int in = iInPos_;
	int out = iOutPos_;
	GetFreeSize_MACRO(in, out, iRet);
	return iRet;
}

//--------------------------------------------------------------------
// ���: Ring Buffer�� Data ����
// Parameters:	IN (char *) Ring Buffer�� Data�� ���� ��� Buffer
//				IN (int) ������ ũ��
// Return:		(int) Ring Buffer�� �� ũ��(��ǻ� 0 Ȥ�� sizeToPut ���� �ϳ���)
// Ư�̻��� : Enqueue��å�� sizeToPut() > GetFreeSize()�̸� 0�� ��ȯ�Ѵ�.
//--------------------------------------------------------------------
int RingBuffer::Enqueue(const char* pSource, int iSizeToPut)
{
	int iFreeSize;
	int iDirectEnqueueSize;
	int iFirstSize;
	int iSecondSize;
	char* pInStartPos;

	int in = iInPos_;
	int out = iOutPos_;

	GetFreeSize_MACRO(in, out, iFreeSize);
	if (iSizeToPut > iFreeSize)
	{
		// ��ȯ�ϴ� �ʿ����� ������ �����������.
		return 0;
	}
	GetDirectEnqueueSize_MACRO(in, out, iDirectEnqueueSize);

	// �������� ��ť �����ѻ���� �������� ������� ũ�ų� ������ �ѹ��� ����
	if (iDirectEnqueueSize >= iSizeToPut) 
		iFirstSize = iSizeToPut;
	else // �ι��� ������ �����ؾ���.
		iFirstSize = iDirectEnqueueSize;

	GetInStartPtr_MACRO(in, pInStartPos);
	memcpy(pInStartPos, pSource, iFirstSize);
	MoveInOrOutPos_MACRO(in, iFirstSize);

	iSecondSize = iSizeToPut - iFirstSize;
	if (iSecondSize <= 0)
	{
		iInPos_ = in;
		return iFirstSize;
	}

	GetInStartPtr_MACRO(in, pInStartPos);
	memcpy(pInStartPos, pSource + iFirstSize, iSecondSize);
	MoveInOrOutPos_MACRO(in, iSecondSize);
	iInPos_ = in;

	return iFirstSize + iSecondSize;
}

//--------------------------------------------------------------------
// ���: Ring Buffer���� Data ����
// Parameters:	OUT (char *) Ring Buffer�� Data�� ���� ��� Buffer
//				IN (int) ������ ũ��
// Return:		(int) Ring Buffer���� ������ pDest�� ������ �������� ũ��
// Ư�̻��� : Dequeue��å�� sizeToRead > GetUseSize()�̸� Dequeue�� ����� ���������ʰ� 0�� ��ȯ�Ѵ�.
//--------------------------------------------------------------------
int RingBuffer::Dequeue(char* pOutDest, int iSizeToOut)
{
	int iUseSize;
	int iDirectDequeueSize;
	int iFirstSize;
	int iSecondSize;
	char* pOutStartPos;

	int in = iInPos_;
	int out = iOutPos_;

	GetUseSize_MACRO(in, out, iUseSize);
	if (iSizeToOut > iUseSize)
	{
		// ����ִ� �����ͺ��� ���� �����͸� �������� �ϸ� �׳� ��ȯ�Ѵ�.
		return 0;
	}
	GetDirectDequeueSize_MACRO(in, out, iDirectDequeueSize);

	// �������� ��ť �����ѻ���� �������� ������� ũ�ų� ������ �ѹ��� ����
	if (iDirectDequeueSize > iSizeToOut) 
		iFirstSize = iSizeToOut;
	else // �ι��� ������ �����ؾ���
		iFirstSize = iDirectDequeueSize;


	GetOutStartPtr_MACRO(out, pOutStartPos);
	memcpy(pOutDest, pOutStartPos, iFirstSize);
	MoveInOrOutPos_MACRO(out, iFirstSize);

	iSecondSize = iSizeToOut - iFirstSize;
	if (iSecondSize <= 0)
	{
		iOutPos_ = out;
		return iFirstSize;
	}

	GetOutStartPtr_MACRO(out, pOutStartPos);
	memcpy(pOutDest + iFirstSize, pOutStartPos, iSecondSize);
	MoveInOrOutPos_MACRO(out, iSecondSize);
	iOutPos_ = out;

	return iFirstSize + iSecondSize;
}

// �ش��Լ��� Dequeue�� ���� ������ ���縸 �����ϰ� front�� ��ġ�� �ٲ��� �ʴ´�. 
int RingBuffer::Peek(char* pOutTarget, int iSizeToPeek)
{
	int iUseSize;
	int iDirectPeekSize;
	int iFirstSize;
	int iSecondSize;
	char* pPeekStartPos;

	int in = iInPos_;
	int out = iOutPos_;

	GetUseSize_MACRO(in, out, iUseSize);
	if (iSizeToPeek > iUseSize)
	{
		// ����ִ� �����ͺ��� ���� �����͸� �������� �ϸ� �׳� ��ȯ�Ѵ�.
		return 0;
	}

	GetDirectDequeueSize_MACRO(in, out, iDirectPeekSize);
	if (iDirectPeekSize > iSizeToPeek) // �߷��� �ι��� ���ļ� ����
		iFirstSize = iSizeToPeek;
	else // �ѹ��� ����
		iFirstSize = iDirectPeekSize;

	GetOutStartPtr_MACRO(out, pPeekStartPos);
	memcpy(pOutTarget, pPeekStartPos, iFirstSize);

	iSecondSize = iSizeToPeek - iFirstSize;
	if (iSecondSize <= 0)
		return iFirstSize;

	memcpy(pOutTarget + iFirstSize, Buffer_, iSecondSize);
	return iFirstSize + iSecondSize;
}

int RingBuffer::PeekAt(char* pOutTarget, int iOut, int iSizeToPeek)
{
	int iUseSize;
	int iDirectPeekSize;
	int iFirstSize;
	int iSecondSize;
	char* pPeekStartPos;

	int in = iInPos_;
	GetUseSize_MACRO(in, iOut, iUseSize);
	if (iSizeToPeek > iUseSize)
	{
		// ����ִ� �����ͺ��� ���� �����͸� �������� �ϸ� �׳� ��ȯ�Ѵ�.
		return 0;
	}

	GetDirectDequeueSize_MACRO(in, iOut, iDirectPeekSize);
	if (iDirectPeekSize > iSizeToPeek) // �߷��� �ι��� ���ļ� ����
		iFirstSize = iSizeToPeek;
	else // �ѹ��� ����
		iFirstSize = iDirectPeekSize;

	GetOutStartPtr_MACRO(iOut, pPeekStartPos);
	memcpy(pOutTarget, pPeekStartPos, iFirstSize);

	iSecondSize = iSizeToPeek - iFirstSize;
	if (iSecondSize <= 0)
		return iFirstSize;

	memcpy(pOutTarget + iFirstSize, Buffer_, iSecondSize);
	return iFirstSize + iSecondSize;
}

void RingBuffer::ClearBuffer(void)
{
	iInPos_ = iOutPos_ = 0;
}


// ��� : rear_ + 1���� ť�� �Ǿ����� �̵����� �ʰ� Enqueue������ �ִ� ũ�⸦ ��ȯ��.
// �� rear+ 1���� front_ - 1������ �Ÿ� Ȥ�� rear_ + 1���� 
int RingBuffer::DirectEnqueueSize(void)
{
	int iRet;
	int in = iInPos_;
	int out = iOutPos_;
	GetDirectEnqueueSize_MACRO(in,out,iRet);
	return iRet;
}

int RingBuffer::DirectDequeueSize(void)
{
	int iRet;
	int in = iInPos_;
	int out = iOutPos_;
	GetDirectDequeueSize_MACRO(in, out, iRet);
	return iRet;
}

// sizeToMove��ŭ rear_�� �̵���Ű�� ���� rear_�� ��ȯ��.
int RingBuffer::MoveInPos(int iSizeToMove)
{
	MoveInOrOutPos_MACRO(iInPos_, iSizeToMove);
	return iInPos_;
}

// sizeToMove��ŭ front_�̵���Ű�� ���� rear_�� ��ȯ��.
int RingBuffer::MoveOutPos(int iSizeToMove)
{
	int OutPrev = iOutPos_;
	MoveInOrOutPos_MACRO(iOutPos_, iSizeToMove);
	int OutAfter = iOutPos_;
	return iOutPos_;
}

char* RingBuffer::GetWriteStartPtr(void)
{
	char* pRet;
	int in = iInPos_;
	GetInStartPtr_MACRO(in,pRet);
	return pRet;
}

char* RingBuffer::GetReadStartPtr(void)
{
	char* pRet;
	int out = iOutPos_;
	GetOutStartPtr_MACRO(out, pRet);
	return pRet;
}


