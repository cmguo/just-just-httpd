// RtspSession.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpSink.h"

#include <util/protocol/http/HttpSocket.h>

#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox.httpd.HttpSink", Debug)

namespace ppbox
{
    namespace httpd
    {
        HttpSink::HttpSink(
            boost::asio::io_service & io_svc
            , util::protocol::HttpSocket& sock)
            : util::stream::Sink(io_svc)
            , socket_(sock)
        {
        }

        HttpSink::~HttpSink()
        {
        }

        //工作线程调用
        std::size_t HttpSink::private_write_some(
            util::stream::StreamConstBuffers const & buffers,
            boost::system::error_code & ec)
        {
            return boost::asio::write(
                socket_, 
                buffers, 
                boost::asio::transfer_all(), 
                ec);
        }

    } // namespace httpd
} // namespace ppbox
