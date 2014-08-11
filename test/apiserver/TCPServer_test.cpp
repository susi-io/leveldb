#include "gtest/gtest.h"

#include "apiserver/TCPServer.h"
#include "apiserver/TCPServer.h"
#include "apiserver/TCPClient.h"

#include <mutex>
#include <condition_variable>

class TCPServerClientTest : public ::testing::Test {
protected:
	class EchoServer : public Susi::Api::TCPServer {
	public:
		std::mutex mutex;
		std::condition_variable cond;

		bool onConnectCalled = false;
		bool onDataCalled = false;
		bool onCloseCalled = false;

		EchoServer(unsigned short port) : Susi::Api::TCPServer{port} {}

		virtual void onConnect(std::string & id) override {
			onConnectCalled = true;
		}
		virtual void onData(std::string & id, std::string & data) override {
			onDataCalled = true;
			send(id,data);
		}
		virtual void onClose(std::string & id) override {
			onCloseCalled = true;
			cond.notify_all();
			stop();
		}
	};

	class EchoClient : public Susi::Api::TCPClient {
	public:
		bool onConnectCalled = false;
		bool onDataCalled = false;
		bool onCloseCalled = false;

		std::mutex mutex;
		std::condition_variable cond;

		EchoClient(std::string host, unsigned short port) : Susi::Api::TCPClient{host,port} {}

		virtual void onConnect() override{
			onConnectCalled = true;
			std::string data = "foobar";
			send(data);
		}
		virtual void onData(std::string & data) override{
			onDataCalled = true;
			std::string should = "foobar";
			EXPECT_EQ(should,data);
			close();
		}
		virtual void onClose() override{
			onCloseCalled = true;
			cond.notify_all();
		}
	};
};

TEST_F(TCPServerClientTest,Echo){
	EchoServer server{12345};
	EchoClient client{"localhost",12345};
	{
		std::unique_lock<std::mutex> lock(client.mutex);
		client.cond.wait(lock,[&client](){return client.onCloseCalled;}); 
	}
	{
		std::unique_lock<std::mutex> lock(server.mutex);
		server.cond.wait(lock,[&server](){return server.onCloseCalled;}); 
	}
	EXPECT_TRUE(server.onConnectCalled);
	EXPECT_TRUE(server.onDataCalled);
	EXPECT_TRUE(server.onCloseCalled);

	EXPECT_TRUE(client.onConnectCalled);
	EXPECT_TRUE(client.onDataCalled);
	EXPECT_TRUE(client.onCloseCalled);
}