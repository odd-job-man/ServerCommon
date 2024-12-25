#pragma once
#include <cpp_redis/cpp_redis>
struct RedisClientWrapper
{
	cpp_redis::client* pClient_;
	RedisClientWrapper()
		:pClient_{ new cpp_redis::client }
	{
		pClient_->connect();
	}

	~RedisClientWrapper()
	{
		//����� ������ ���ŷ�ȴ�
		pClient_->disconnect(true);
		// �ڲ� �Ҹ��ڿ��� ���ŷ�Ǽ� �ϴ� ���Ḹ ���� ����
		//delete pClient_;
		//pClient_ = nullptr;
	}
};

inline cpp_redis::client* GetRedisClient()
{
	thread_local RedisClientWrapper client;
	return client.pClient_;
}
