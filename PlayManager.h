//PlayManager.h

#ifndef      _PPBOX_HTTP_PLAYMANAGER_
#define      _PPBOX_HTTP_PLAYMANAGER_

#include <ppbox/ppbox/IAdapter.h>
#include <ppbox/ppbox/IDemuxer.h>

#include <framework/thread/MessageQueue.h>
#include <boost/function/function2.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <queue>
#include <list>

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
        typedef boost::function2<void, 
            boost::system::error_code const &, 
            std::pair<std::size_t, std::size_t> const &
        > response_type;

        typedef enum
        {
            NODEFINE,
            START,
            PLAY,
            CALL,
            INFO,
            MEDIAINFO,
            SEEK,
            PAUSE,
            RESUME,
            ERR,
        } Option;

        struct OpenState
        {
            enum Enum
            {
                closed,
                opening,
                cancel,
                opened,
            };
        };

        struct MsgInfo
        {
            MsgInfo()
                : seek_time(0)
            {
            };

            ~MsgInfo() {};

            MsgInfo & operator=(MsgInfo const & info)
            {
                option = info.option;
                ec     = info.ec;
                url    = info.url;
                format = info.format;
                type   = info.type;
                write_socket = info.write_socket;
                //resp.swap(info.resp);
                resp = info.resp;
                seek_time    = info.seek_time;
                gid          = info.gid;
                pid          = info.pid;
                auth         = info.auth;
                return *this;
            }

            Option      option;
            boost::system::error_code ec;
            std::string url;
            std::string format;
            std::string type;
            util::protocol::HttpSocket * write_socket;
            response_type resp;
            boost::int32_t seek_time;
            std::string gid;
            std::string pid;
            std::string auth;
        };

        struct OpenInfo
        {
            OpenInfo()
               : open_state(OpenState::closed)
            {};

            ~OpenInfo() {};

            OpenInfo & operator=(OpenInfo const & info)
            {
                playlink      =  info.playlink;
                format        =  info.format;
                type          =  info.type;
                open_state    =  info.open_state;
                plays         = info.plays;
                mediainfo     = info.mediainfo;
                return *this;
            }

            std::string         playlink;
            std::string         format;
            std::string         type;
            OpenState::Enum     open_state;
            std::list<MsgInfo> plays;
            std::list<MsgInfo> mediainfo;
        };

        class PlayManager
        {

        public:
            enum WorkState
            {
                command, 
                play, 
            };

        public:
            PlayManager(boost::asio::io_service & io_srv);
            ~PlayManager();

            void start(void);
            void stop(void);

        private:
            void thread_function(void);

            boost::system::error_code open_impl(
                boost::system::error_code & ec);

            void async_open_impl(Adapter_Open_Callback callback);

            static void open_callback(PP_int32 lec);

            bool is_open(void);

            bool is_play(void);

            boost::system::error_code start_impl(
                boost::system::error_code & ec);

            boost::system::error_code mediainfo_impl(
                boost::system::error_code & ec);

            boost::system::error_code seek_impl(
                boost::system::error_code & ec);

            boost::system::error_code pause_impl(
                boost::system::error_code & ec);

            boost::system::error_code resume_impl(
                boost::system::error_code & ec);

            boost::system::error_code callback_impl(
                boost::system::error_code & ec);

            boost::system::error_code info_impl(
                boost::system::error_code & ec);

            boost::system::error_code error_impl(
                boost::system::error_code & ec);

            void handle_all(
                OpenInfo & open_info,
                boost::system::error_code const & ec);

            void handle_one(
                MsgInfo & msg_info,
                boost::system::error_code const & ec);

            boost::system::error_code handle_seek(
                boost::system::error_code & ec);

            void merge(OpenInfo & open_info);

            void handle_mediainfo(std::list<MsgInfo> & mediainfo);

            void on_timer(
                boost::system::error_code const & ec);

            void dump_stat(Adapter_PlayStatistic &msg);

            bool is_live_url(std::string const & type);

            bool insert_to_openlist(void);

            bool is_same_movie(OpenInfo const & open_info);

            static boost::system::error_code ec_translate(PP_int32 lec);

        public:
            static boost::system::error_code write_mediainfo_to_client(
                util::protocol::HttpSocket * handle,
                Adapter_Mediainfo const & media_info,
                boost::system::error_code & ec);

            static boost::system::error_code write_playing_info_to_client(
                util::protocol::HttpSocket * handle,
                Adapter_PlayStatistic const & msg,
                PP_uint64 time,
                boost::system::error_code & ec);

            static boost::system::error_code write_error_info_to_client(
                util::protocol::HttpSocket * handle,
                boost::system::error_code const & last_error,
                boost::system::error_code & ec);

            static boost::system::error_code write_error_respone(
                util::protocol::HttpSocket * handle,
                boost::uint32_t  code,
                std::string const & error_msg,
                std::string const & module,
                boost::system::error_code & ec);

            static boost::system::error_code write_success_respone(
                util::protocol::HttpSocket * handle,
                std::string const & module,
                boost::system::error_code & ec);

        private:
            boost::thread * work_thread_;
            bool is_certify_;
            bool stop_;
            bool pause_;
            bool blocked_;
            boost::uint32_t seek_time_;
            boost::uint32_t buffer_percent_;
            boost::asio::io_service & io_srv_;
            MsgInfo msg_info_;
            std::pair<std::size_t, std::size_t> size_pair;
            WorkState work_state_;
            std::queue<OpenInfo> open_task_queue_;
            boost::system::error_code last_error_;

        public:
            static framework::thread::MessageQueue< MsgInfo > s_msg_queue;
        };

    } // namespace httpd

} // namespace ppbox

#endif // _PPBOX_HTTP_PLAYMANAGER_
