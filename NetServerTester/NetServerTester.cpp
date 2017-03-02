//
// main.cpp
// ~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
//#include <boost/asio/ssl.hpp>

/**
 - Create Socket
 - Listen
 - Accept (specific socket connection)
 - Get data
 - Shutdown clients socket FD
 - Close (accepted socket File Descriptor)
 - DONT close Listen FD (until we're ready to shut down the server...this createst leaks)
*/
using boost::asio::ip::tcp;

class session
{
public:
	session(boost::asio::io_service& io_service)
		: socket_(io_service)
	{
	}

	tcp::socket& socket()
	{
		return socket_;
	}

	void start()
	{
		boost::asio::async_read_until(socket_, buf_, "\n\n", 
			boost::bind(&session::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
		);

				
		//socket_.async_read_some(boost::asio::buffer(data_, max_length),
		//	boost::bind(&session::handle_read, this,
		//		boost::asio::placeholders::error,
		//		boost::asio::placeholders::bytes_transferred));
	}

private:
	void handle_read(const boost::system::error_code& error,
		size_t bytes_transferred)
	{
		if (!error)
		{
			/*
			Flag:
				1 Sending Data

			Eflag:
				3 Event Data

			Psize:
				Payload size (EVENT + Timestamp + Username)

			Usize:
				Username length

			Tsize:
				Length of Timestamp

			MD5:
				32 bytes
			*/
			
			
			unsigned char* output = (unsigned char*)malloc(buf_.size());
			memcpy(output, boost::asio::buffer_cast<const void*>(buf_.data()), buf_.size());
			uint16_t flag = output[0] | uint16_t(output[1]) << 8;
			uint16_t eflag = output[2] | uint16_t(output[3]) << 8;
			uint16_t psize = output[4] | uint16_t(output[5]) << 8;
			uint16_t usize = output[6] | uint16_t(output[7]) << 8;
			uint16_t tsize = output[8] | uint16_t(output[9]) << 8;

			std::string PAYLOAD(reinterpret_cast<char const*>(&output[10]), (psize-tsize-usize));	
			std::string TIME(reinterpret_cast<char const*>(&output[10 + (psize - tsize - usize)]), tsize);
			std::string USER(reinterpret_cast<char const*>(&output[10 + psize - usize]), usize);
			std::string MD5(reinterpret_cast<char const*>(&output[10 + psize]), 32);

			/// Compute the pre-hash string of unsigned chars
			std::vector<unsigned char> preHash;
			for (int i = 0; i < bytes_transferred - 32 - 2; i++)
			{
				preHash.push_back(output[i]);
			}

			
			/// Compute MD5 hash of everything minus the incoming MD5 Hash, compare, and report results
			//if(hashMD5String(preHash) == MD5)
			//{
				boost::asio::async_write(socket_,
					boost::asio::buffer("0\n\n", 3),
					boost::bind(&session::handle_write, this,
						boost::asio::placeholders::error));
			//}
			
			int x = 33;						
		}
		else
		{
			delete this;
		}
	}

	void handle_write(const boost::system::error_code& error)
	{
		if (!error)
		{
			std::cout << "handle_write: "<< std::endl;

			//socket_.async_read_some(boost::asio::buffer(data_, max_length),
			//	boost::bind(&session::handle_read, this,
			//		boost::asio::placeholders::error,
			//		boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			delete this;
		}
	}

	tcp::socket socket_;
	enum { max_length = 4096 };
	unsigned char * data_;//unsigned char data_[max_length];
	boost::asio::streambuf buf_;
	uint16_t BYTES_SENT;
	
};

class server
{
public:
	server(boost::asio::io_service& io_service, short port)
		: io_service_(io_service),
		acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
	{
		start_accept();
	}

private:
	void start_accept()
	{
		session* new_session = new session(io_service_);
		acceptor_.async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, this, new_session,
				boost::asio::placeholders::error));
	}

	void handle_accept(session* new_session,
		const boost::system::error_code& error)
	{
		if (!error)
		{
			new_session->start();
		}
		else
		{
			delete new_session;
		}

		start_accept();
	}

	boost::asio::io_service& io_service_;
	tcp::acceptor acceptor_;	
};

int main(int argc, char* argv[])
{
	try
	{
		//if (argc != 2)
		//{
		//	std::cerr << "Usage: async_tcp_echo_server <port>\n";
		//	return 1;
		//}

		boost::asio::io_service io_service;

		using namespace std; // For atoi.
		server s(io_service, 16000);

		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}