#ifndef _PPBOX_HTTPD_HTTP_CHUNKED_SINK_H_
#define _PPBOX_HTTPD_HTTP_CHUNKED_SINK_H_

#include <ppbox/mux/tool/Sink.h>

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
            : public ppbox::mux::Sink
        {
        public:
            HttpChunkedSink(
                util::protocol::HttpSocket& sock);

            virtual ~HttpChunkedSink();

        private:
            virtual boost::system::error_code write(
                boost::posix_time::ptime const & time_send, 
                ppbox::demux::Sample & sample);

            boost::system::error_code on_finish(
                boost::system::error_code const & ec);

        private:
            util::protocol::HttpChunkedSocket<util::protocol::HttpSocket> socket_;
        };

    } // namespace httpd
} // namespace ppbox

#endif // _PPBOX_HTTPD_HTTP_CHUNKED_SINK_H_
