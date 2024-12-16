#include "CLinkedList.h"

LINKED_NODE::LINKED_NODE(const int offset)
	:offset_{ offset }, pNext_{ nullptr }, pPrev_{ nullptr }
{}

CLinkedList::CLinkedList(const int offset)
	:offset_{ offset }, pHead_{ nullptr }, pTail_{ nullptr }
{}

void CLinkedList::push_back(void* pNewData)
{
	LINKED_NODE* pNewLink = (LINKED_NODE*)((char*)pNewData + offset_);
#ifdef _DEBUG
	if (pNewLink->offset_ != offset_)
		__debugbreak();
#endif
	// ����մ� ����Ʈ�� ���� ���
	if (pHead_ == nullptr)
	{
		pTail_ = pHead_ = pNewLink;
		pHead_->pNext_ = nullptr;
		pHead_->pPrev_ = nullptr;
	}
	else
	{
		pNewLink->pPrev_ = pTail_;
		pTail_->pNext_ = pNewLink;
		pTail_ = pNewLink;
		pNewLink->pNext_ = nullptr;
	}
	++size_;
}

void* CLinkedList::remove(void* pVictim)
{
	LINKED_NODE* pVictimLink = (LINKED_NODE*)((char*)pVictim + offset_);
	void* pRet;
#ifdef _DEBUG
	if (pVictimLink->offset_ != offset_)
		__debugbreak();

	// �յ� ����� ���ռ� �Ǵ�
	if (pVictimLink->pPrev_ != nullptr && pVictimLink->pPrev_->pNext_ != pVictimLink)
		__debugbreak();

	if (pVictimLink->pNext_ != nullptr && pVictimLink->pNext_->pPrev_ != pVictimLink)
		__debugbreak();
#endif

	if (pVictimLink->pPrev_ != nullptr)
	{
		pVictimLink->pPrev_->pNext_ = pVictimLink->pNext_;
	}
	else
	{
#ifdef _DEBUG
		// �� �ճ���� �����޴µ� pHead_�� �ƴѰ��
		if (pVictimLink != pHead_)
			__debugbreak();
#endif
		// pVictim�� ����Ʈ�� �� �ճ���϶�
		pHead_ = pVictimLink->pNext_;
	}

	if (pVictimLink->pNext_ != nullptr)
	{
		pVictimLink->pNext_->pPrev_ = pVictimLink->pPrev_;
		pRet = (char*)(pVictimLink->pNext_) - offset_;
	}
	else
	{
#ifdef _DEBUG
		// �� �޳���� �����޴µ� pTail_�� �ƴѰ��
		if (pVictimLink != pTail_)
			__debugbreak();
#endif
		// pVictim�� ����Ʈ�� �� ������϶� 
		pTail_ = pVictimLink->pPrev_;
		pRet = nullptr;
	}
	pVictimLink->pNext_ = pVictimLink->pPrev_ = nullptr;
	--size_;
	return pRet;
}

void* CLinkedList::GetNext(void* pCur)
{
	LINKED_NODE* pNext = ((LINKED_NODE*)((char*)pCur + offset_))->pNext_;
	if (pNext == nullptr)
		return nullptr;

	return (char*)pNext - offset_;
}

void* CLinkedList::GetFirst()
{
	if(pHead_ == nullptr)
		return nullptr;

	return (char*)pHead_ - offset_;
}

int CLinkedList::FindElementNum(void* pElement)
{
	int ret = 0;
	void* pTarget = GetFirst();
	while (pTarget != nullptr)
	{
		if (pTarget == pElement)
			++ret;

		pTarget = GetNext(pTarget);
	}
	return ret;
}


