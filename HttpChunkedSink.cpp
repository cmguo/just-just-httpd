// RtspSession.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpChunkedSink.h"

#include <ppbox/mux/MuxBase.h>

#include <util/protocol/http/HttpSocket.h>

#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("HttpChunkedSink", 0)

namespace ppbox
{
    namespace httpd
    {

        HttpChunkedSink::HttpChunkedSink(
            boost::asio::io_service & io_svc,
            util::protocol::HttpSocket& sock)
            : util::stream::Sink(io_svc)
            , socket_(sock)
        {
        }

        HttpChunkedSink::~HttpChunkedSink()
        {
        }

        std::size_t HttpChunkedSink::private_write_some(
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
