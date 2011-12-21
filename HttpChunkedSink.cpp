// RtspSession.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/HttpChunkedSink.h"

#include <ppbox/mux/MuxerBase.h>

#include <util/protocol/http/HttpSocket.h>
using namespace framework::logger;

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("HttpChunkedSink", 0)

namespace ppbox
{
    namespace httpd
    {

        HttpChunkedSink::HttpChunkedSink(
            util::protocol::HttpSocket& sock)
            : socket_(sock)
        {
        }

        HttpChunkedSink::~HttpChunkedSink()
        {
        }

        boost::system::error_code HttpChunkedSink::on_finish(
            boost::system::error_code const & ec)
        {
            boost::system::error_code ec1;
            socket_.send(util::protocol::HttpChunkedSocket<util::protocol::HttpSocket>::eof(),0,ec1);
            LOG_S(Logger::kLevelEvent, "[on_finish] ec:"<<ec1.message());
            return ec1;
        }

        //工作线程调用
        boost::system::error_code HttpChunkedSink::write(
            boost::posix_time::ptime const & time_send, 
            ppbox::demux::Sample & sample)
        {
            boost::system::error_code ec;
            boost::asio::write(
                socket_, 
                sample.data, 
                boost::asio::transfer_all(), 
                ec);
            return ec;
        }

    } // namespace httpd
} // namespace ppbox
