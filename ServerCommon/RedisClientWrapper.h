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
		//끊기기 전까지 블라킹된다
		pClient_->disconnect(true);
		// 자꾸 소멸자에서 블라킹되서 일단 연결만 끊고 종료
		//delete pClient_;
		//pClient_ = nullptr;
	}
};

inline cpp_redis::client* GetRedisClient()
{
	thread_local RedisClientWrapper client;
	return client.pClient_;
}
