#pragma once
#include <utility>
#include <new>
#include "CAddressTranslator.h"
#include "MultithreadProfiler.h"

template<typename T, BOOL bPlacementNew>
struct Bucket
{
	static constexpr int size = 1000000;
	struct NODE
	{
		T data;
		Bucket* pBucket;
	};

	// placementNew �� Malloc���� ������� Cnt,MetaNext�� �ʱ�ȭ�Ѵ�
	// bPlamentNew�� false�̸鼭 Malloc�ǰ� ���� ������ �ʾѴٸ� ��� ��忡 ���� ���� 0���� �⺻ ������ ȣ��
	// bPlacementNew�� true��� �����ڴ� �Ź� ȣ��������ϱ⿡ ���⼭�� ȣ�����
	template<bool isMalloced>
	inline void Init()
	{
		AllocCnt_ = FreeCnt_ = 0;
		metaNext = 0;

		if constexpr (isMalloced) // ���� �Ҵ�� ��Ŷ �ּ� ����
		{
			for (int i = 0; i < size; ++i)
			{
				nodeArr_[i].pBucket = this;
			}

			if constexpr (!bPlacementNew) // bPlacementNew == FALSE (��ǥ���� ���ð� ����ȭ����) �϶��� �����ڸ� ó���� �ѹ� ȣ��
			{
				for (int i = 0; i < size; ++i)
				{
					new(&nodeArr_[i].data)T{};
				}
			}
		}
	}

	static inline Bucket* DataToBucket(T* pData)
	{
		return *(Bucket**)((char*)pData + offsetof(NODE, pBucket) - offsetof(NODE, data));
	}

	// ��Ŷ���� Node�� �Ҵ�ǥ���ϰ� ��ȯ�Ѵ�
	// �ش� �Լ� ȣ��� ���� ��Ŷ�ȿ� ���̻� �Ҵ��� ��尡 ���ԵǸ� Out parameter�� true�� ��������.
	inline NODE* AllocNode(bool* pbOutMustClearTls)
	{
		NODE* pRet = &nodeArr_[AllocCnt_++];
		if (AllocCnt_ == size)
		{
			*pbOutMustClearTls = true;
		}
		else
		{
			*pbOutMustClearTls = false;
		}
		return pRet;
	}

	// ��Ŷ�� Node�� FREEǥ���Ѵ�
	// �ش� �Լ� ȣ��� ���� ��Ŷ���� ��尡 ��� FREE�� ���¶�� Out Parameter�� true�� ��������. 
	inline bool RETURN_NODE_TO_BUCKET_AND_CHECK_BUCKET_HAVETO_FREE()
	{
		if (_InterlockedIncrement(&FreeCnt_) == size)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	NODE nodeArr_[size];
	long AllocCnt_;
	long FreeCnt_;
	uintptr_t metaNext;
};


template<typename T, bool bPlacementNew>
class CTlsObjectPool
{
private:
	using Bucket = Bucket<T, bPlacementNew>;
	DWORD dwTlsIdx_;

	alignas(64) uintptr_t metaTop_ = 0;
	alignas(64) long capacity_ = 0;
	alignas(64) long size_ = 0;
	alignas(64) size_t metaCnt_ = 0;
	long AllocSize_ = 0;


	// �������� Tls�� ���� �Ҵ� Ȥ�� �ٽἭ ��Ŷ�� ���ٸ� ������ ���ñ����� ��Ŷ�� �Ҵ�޴´�.
	Bucket* AllocBucket()
	{
		uintptr_t metaTop;
		Bucket* pRealTop;
		uintptr_t newMetaTop;

		do
		{
			metaTop = metaTop_;
			if (!metaTop)
			{
				InterlockedIncrement(&capacity_);
				Bucket* pBucket = (Bucket*)malloc(sizeof(Bucket));
				pBucket->Init<true>();
				return pBucket;
			}
			pRealTop = (Bucket*)CAddressTranslator::GetRealAddr(metaTop);
			newMetaTop = pRealTop->metaNext;
		} while (InterlockedCompareExchange(&metaTop_, newMetaTop, metaTop) != metaTop);

		pRealTop->Init<false>();
		InterlockedDecrement(&size_);
		return pRealTop;
	}

public:
	CTlsObjectPool()
	{
		dwTlsIdx_ = TlsAlloc();
		if (dwTlsIdx_ == TLS_OUT_OF_INDEXES)
		{
			DWORD dwErrCode = GetLastError();
			__debugbreak();
		}
	}

	template<typename... Types> requires (bPlacementNew || (sizeof...(Types) == 0))
	T* Alloc(Types&&... args)
	{
		Bucket* pBucket = (Bucket*)TlsGetValue(dwTlsIdx_);
		if (!pBucket)
		{
			pBucket = AllocBucket();
			TlsSetValue(dwTlsIdx_, pBucket);
		}

		bool bMustFreeBucket;
		auto* pNode = pBucket->AllocNode(&bMustFreeBucket);
		if (bMustFreeBucket)
		{
			TlsSetValue(dwTlsIdx_, nullptr);
		}
		if constexpr (bPlacementNew)
		{
			new(&pNode->data)T{ std::forward<Types>(args)... };
		}

		InterlockedIncrement(&AllocSize_);
		return &pNode->data;
	}


	void Free(T* pData)
	{
		uintptr_t metaTop;
		Bucket* pBucket = Bucket::DataToBucket(pData);

		if constexpr (bPlacementNew)
		{
			pData->~T();
		}

		if (!pBucket->RETURN_NODE_TO_BUCKET_AND_CHECK_BUCKET_HAVETO_FREE())
		{
			InterlockedDecrement(&AllocSize_);
			return;
		}

		uintptr_t newMetaTop = CAddressTranslator::GetMetaAddr
		(
			CAddressTranslator::GetCnt(&metaCnt_),
			(uintptr_t)pBucket
		);

		do
		{
			metaTop = metaTop_;
			pBucket->metaNext = metaTop;
		} while (InterlockedCompareExchange(&metaTop_, newMetaTop, metaTop) != metaTop);

		InterlockedIncrement(&size_);
		InterlockedDecrement(&AllocSize_);
	}
};
