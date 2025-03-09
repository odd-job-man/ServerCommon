#pragma once
#include <cpp_redis/cpp_redis>
#include <Windows.h>


struct RedisClientWrapper
{
	cpp_redis::client* pClient_;
	static inline CRITICAL_SECTION cs;
	RedisClientWrapper()
	{
		pClient_ = new cpp_redis::client ;
		pClient_->connect();
	}

	~RedisClientWrapper()
	{
		//����� ������ ���ŷ�ȴ�
		// �ڲ� �Ҹ��ڿ��� ���ŷ�Ǽ� �ϴ� ���Ḹ ���� ����
		pClient_->disconnect(true);
	}
};

inline cpp_redis::client* GetRedisClient()
{
	thread_local RedisClientWrapper client;
	return client.pClient_;
}
