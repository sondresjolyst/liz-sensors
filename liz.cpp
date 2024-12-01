#include "liz.h"
#include <boost/asio.hpp>
#include <iostream>

namespace liz
{
    std::string getPilot(const char *ipaddress, int port)
    {
        std::cout << "getPilot...\n";
        std::string response;

        try
        {
            boost::asio::io_service io_service;

            boost::asio::ip::udp::socket socket(io_service);
            boost::asio::ip::udp::endpoint remote_endpoint;

            socket.open(boost::asio::ip::udp::v4());

            remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ipaddress), port);

            boost::system::error_code err;
            std::string message = "{\"method\":\"getPilot\"}";

            socket.send_to(boost::asio::buffer(message), remote_endpoint, 0, err);

            if (err)
            {
                std::cerr << err.message() << '\n';
            }
            else
            {
                char reply[1024];
                size_t reply_length = socket.receive_from(boost::asio::buffer(reply, 1024), remote_endpoint);
                response = std::string(reply, reply_length);
                std::cout << "Received: " << response << '\n';
                socket.close();
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }

        return response;
    }
}
