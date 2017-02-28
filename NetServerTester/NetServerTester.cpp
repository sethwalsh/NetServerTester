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
		boost::asio::async_read_until(socket_, buf_, '\n', 
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
			Packet:
				1 byte Packet Type Flag
				4 byte Packet Size
				X byte Packet Payload

			Flag:
				1 Request Program List
				2 Submit Data

			Size:
				0-65,535 bits (16384 bytes)

			Payload:
				0-16384 bytes of data
			*/
			
			std::string STR(boost::asio::buffers_begin(buf_.data()), boost::asio::buffers_begin(buf_.data()) + bytes_transferred);

			if(STR.length() < 5)//if (strlen((char*)data_) < 5)
			{
				/*
				sprintf(data_, "-1");

				boost::asio::async_write(socket_,
					boost::asio::buffer(data_, 1),
					boost::bind(&session::handle_write, this,
						boost::asio::placeholders::error));
				*/
				int x = 33;
			}
			else
			{				
				unsigned char PACKET_FLAG_ = data_[0];
				uint16_t PACKET_SIZE_ = 0;
				char c = data_[1];
				PACKET_SIZE_ = PACKET_SIZE_ | (strtol(&c, NULL, 16) << 12);
				c = data_[2];
				PACKET_SIZE_ = PACKET_SIZE_ | (strtol(&c, NULL, 16) << 8);
				c = data_[3];
				PACKET_SIZE_ = PACKET_SIZE_ | (strtol(&c, NULL, 16) << 4);
				c = data_[4];
				PACKET_SIZE_ = PACKET_SIZE_ | strtol(&c, NULL, 16);
				unsigned char *PACKET_PAYLOAD_ = &data_[5];

				// If packet size != bytes_transferred
				if(PACKET_SIZE_ != bytes_transferred)
					boost::asio::async_write(socket_,
						boost::asio::buffer("-1\n", 2),
						boost::bind(&session::handle_write, this,
							boost::asio::placeholders::error));

				std::vector<char> payload_;
				for (int i = 0; i < (PACKET_SIZE_ - 5); i++)
				{
					payload_.push_back(*PACKET_PAYLOAD_);
					++PACKET_PAYLOAD_;
				}
				//std::string s(PACKET_PAYLOAD_);

				sprintf((char*)data_, "%d%s", PACKET_SIZE_,"\n");

				boost::asio::async_write(socket_,
					boost::asio::buffer("hi"),
					boost::bind(&session::handle_write, this,
						boost::asio::placeholders::error));
				int x = 33;

			}			
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
			std::cout << "handle_write: " << data_ << std::endl;

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