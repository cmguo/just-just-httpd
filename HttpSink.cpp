// RtspSession.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpSink.h"

#include <ppbox/mux/MuxerBase.h>

#include <util/protocol/http/HttpSocket.h>

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("HttpSink", 0)

namespace ppbox
{
    namespace httpd
    {
        HttpSink::HttpSink(
            util::protocol::HttpSocket& sock)
            :socket_(sock)
        {
        }

        HttpSink::~HttpSink()
        {
        }

        //工作线程调用
        boost::system::error_code HttpSink::write(
            boost::posix_time::ptime const & time_send, 
            ppbox::demux::Sample& tag)
        {

            boost::system::error_code ec;
            boost::asio::write(
                socket_, 
                tag.data, 
                boost::asio::transfer_all(), 
                ec);
            return ec;
        }

    } // namespace httpd
} // namespace ppbox
