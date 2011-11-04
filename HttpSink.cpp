// RtspSession.cpp

#include "ppbox/httpd/Common.h"


#include "ppbox/httpd/HttpSink.h"

#include <ppbox/mux/rtp/RtpPacket.h>

#include "util/protocol/http/HttpSocket.h"


FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("HttpSink", 0)

namespace ppbox
{
    namespace httpd
    {
        HttpSink::HttpSink(util::protocol::HttpSocket& sock):socket_(sock)
        {
        }

        HttpSink::~HttpSink()
        {
        }

        //工作线程调用
        boost::system::error_code HttpSink::write( ppbox::demux::Sample& tag)
        {
            boost::system::error_code ec;
            for(boost::uint32_t i = 0; i < tag.data.size(); ++i) {
                boost::asio::const_buffer & buf = tag.data.at(i);
                boost::uint8_t const * data = boost::asio::buffer_cast<boost::uint8_t const *>(buf);
                boost::uint32_t size = boost::asio::buffer_size(buf);
                boost::asio::write(
                    socket_, 
                    boost::asio::buffer(data, size), 
                    boost::asio::transfer_all(), 
                    ec);
                if (ec) {
                    break;
                }
            }
            return ec;
        }

    } // namespace httpd
} // namespace ppbox
