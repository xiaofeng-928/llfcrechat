// GateServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "const.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "MysqlMgr.h"
#include "AsioIOServicePool.h"

void TestMysqlMgr() {
	// 简化后的测试：RegUser只接受name和pwd两个参数
	// 取消注释以测试注册功能
	// int id = MysqlMgr::GetInstance()->RegUser("testuser", "123456");
	// std::cout << "id  is " << id << std::endl;
}

int main()
{
	try
	{
		MysqlMgr::GetInstance();
		auto & gCfgMgr = ConfigMgr::Inst();
		std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
		unsigned short gate_port = atoi(gate_port_str.c_str());
		net::io_context ioc{ 1 };
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {

			if (error) {
				return;
			}
			ioc.stop();
			});
		std::make_shared<CServer>(ioc, gate_port)->Start();
		std::cout << "Gate Server listen on port: " << gate_port << std::endl;
		ioc.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

}

