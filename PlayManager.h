//PlayManager.h

#ifndef      _PPBOX_HTTP_PLAYMANAGER_
#define      _PPBOX_HTTP_PLAYMANAGER_

#include <ppbox/mux/MuxerModule.h>
#include <ppbox/common/CommonModuleBase.h>

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
            TSSEG,
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
               , is_stream_end(false)
            {};

            ~OpenInfo() {};

            OpenInfo & operator=(OpenInfo const & info)
            {
                playlink      =  info.playlink;
                format        =  info.format;
                type          =  info.type;
                open_state    =  info.open_state;
                is_stream_end =  info.is_stream_end;
                plays         = info.plays;
                mediainfo     = info.mediainfo;
                return *this;
            }

            std::string         playlink;
            std::string         format;
            std::string         type;
            OpenState::Enum     open_state;
            bool                is_stream_end;
            std::list<MsgInfo> plays;
            std::list<MsgInfo> mediainfo;
        };

        class PlayManager
            : public ppbox::common::CommonModuleBase<PlayManager>
        {
        public:
            virtual boost::system::error_code startup();

            virtual void shutdown();

        public:
            enum WorkState
            {
                undefine,
                command, 
                play, 
            };

        public:
            PlayManager(util::daemon::Daemon & daemon);

            ~PlayManager();

            void start(void);
            void stop(void);

        private:
            void thread_function(void);

            boost::system::error_code open_impl(
                boost::system::error_code & ec);

            void async_open_impl(void);

            void open_callback(
                boost::system::error_code const & ec,
                ppbox::mux::MuxerBase * muxer);

            bool is_open(void);

            bool is_play(void);

            boost::system::error_code start_impl(
                boost::system::error_code & ec);

            boost::system::error_code mediainfo_impl(
                boost::system::error_code & ec);

            boost::system::error_code seek_impl(
                boost::system::error_code & ec);

            boost::system::error_code tsseg_impl(
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

            //void dump_stat(Adapter_PlayStatistic &msg);

            bool is_live_url(std::string const & type);

            bool insert_to_openlist(void);

            bool is_same_movie(OpenInfo const & open_info);

        public:
            static boost::system::error_code write_mediainfo_to_client(
                util::protocol::HttpSocket * handle,
                ppbox::mux::MediaFileInfo const & media_info,
                boost::system::error_code & ec);

            static boost::system::error_code write_playing_info_to_client(
                util::protocol::HttpSocket * handle,
                boost::uint32_t buffer_time,
                boost::uint64_t time,
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
            boost::system::error_code write_data_to_client(
                ppbox::mux::MuxTagEx & tag,
                boost::system::error_code & ec);

            void clear();

        private:
            ppbox::mux::MuxerModule & muxer_mod_;
            ppbox::mux::MuxerBase * muxer_;
            ppbox::mux::MuxTagEx mux_tag_;
            bool segment_end_;
            size_t close_token_;
            boost::thread * work_thread_;
            bool stop_;
            bool pause_;
            bool blocked_;
            boost::uint32_t seek_time_;
            //boost::uint32_t buffer_percent_;
            boost::uint32_t buffer_time_;
            boost::uint32_t ts_seg_next_index_; // m3u8 format
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
