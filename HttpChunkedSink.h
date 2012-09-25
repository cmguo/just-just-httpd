#ifndef _PPBOX_HTTPD_HTTP_CHUNKED_SINK_H_
#define _PPBOX_HTTPD_HTTP_CHUNKED_SINK_H_

#include <util/stream/Sink.h>

#include <util/protocol/http/HttpChunkedSocket.h>

namespace util
{
    namespace protocol
    {
        class HttpSocket;
    }
}

namespace ppbox
{
    namespace httpd
    {

        class Transport;

        class HttpChunkedSink
            : public util::stream::Sink
        {
        public:
            HttpChunkedSink(
                 boost::asio::io_service & io_svc,
                util::protocol::HttpSocket& sock);

            virtual ~HttpChunkedSink();

        private:
            virtual std::size_t private_write_some(
                util::stream::StreamConstBuffers const & buffers,
                boost::system::error_code & ec);

        private:
            util::protocol::HttpChunkedSocket<util::protocol::HttpSocket> socket_;
        };

    } // namespace httpd
} // namespace ppbox

#endif // _PPBOX_HTTPD_HTTP_CHUNKED_SINK_H_
