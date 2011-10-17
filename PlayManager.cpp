 //PlayManager.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/PlayManager.h"
#include "ppbox/httpd/M3U8Protocol.h"
#include "ppbox/demux/DemuxerError.h"
#include "ppbox/demux/DemuxerBase.h"
using namespace ppbox::mux;

#include <util/protocol/http/HttpSocket.h>
using namespace util::protocol;

#include <framework/string/Url.h>
#include <framework/configure/Config.h>
#include <framework/logger/LoggerStreamRecord.h>
#include <framework/string/Format.h>
#include <framework/string/Algorithm.h>
using namespace framework::network;
using namespace framework::string;
using namespace framework::logger;

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
using namespace boost::system;
using namespace boost::asio;

#include <tinyxml/tinyxml.h>

#include <iostream>
#ifdef   _WRITE_MUX_FILE_
#include <fstream>
#endif

FRAMEWORK_LOGGER_DECLARE_MODULE("Httpd");

namespace ppbox
{
    namespace httpd
    {

#ifdef   _WRITE_MUX_FILE_
        std::ofstream out_mux_file;
#endif

        framework::thread::MessageQueue< MsgInfo > PlayManager::s_msg_queue;

        PlayManager::PlayManager(util::daemon::Daemon & daemon)
            : ppbox::common::CommonModuleBase<PlayManager>(daemon, "playmanager")
            , muxer_mod_(util::daemon::use_module<ppbox::mux::MuxerModule>(daemon))
            , muxer_(NULL)
            , close_token_(0)
            , work_thread_(NULL)
            , segment_end_(false)
            , stop_(false)
            , pause_(false)
            , blocked_(false)
            , seek_time_(0)
            //, buffer_percent_(0)
            , buffer_time_(3000) // 3s
            , ts_seg_next_index_(0)
            , io_srv_(daemon.io_svc())
            , work_state_(PlayManager::command)
            , last_error_(error::httpd_not_authed)
        {
            size_pair.first = 0;
            size_pair.second = 0;
        }

        PlayManager::~PlayManager()
        {
        }

        error_code PlayManager::startup()
        {
            error_code ec;
            start();
            return ec;
        }

        void PlayManager::shutdown()
        {
            stop();
        }

        void PlayManager::start(void)
        {
            if (!work_thread_) {
                work_thread_ = new boost::thread(
                    boost::bind(&PlayManager::thread_function, this));
            }
            work_thread_->detach();
        }

        void PlayManager::stop(void)
        {
            stop_ = true;
        }

        void PlayManager::thread_function(void)
        {
            boost::uint32_t sum = 0;
            boost::uint32_t buffer_time = 0;
            error_code ec;
            error_code playing_ec;

            while (!stop_) {
                while(work_state_ == PlayManager::command) {
                    std::cout << "In command loop" << std::endl;
                    if (s_msg_queue.pop(msg_info_)) {
                        switch (msg_info_.option) {
                        case START:
                            start_impl(ec);
                            if (ec) {
                                last_error_ = ec;
                            } else {
                                last_error_ = error::httpd_not_open;
                            }
                            break;
                        case PLAY:
                            open_impl(ec);
                            break;
                        case CALL:
                            callback_impl(ec);
                            break;
                        case MEDIAINFO:
                            mediainfo_impl(ec);
                            break;
                        case SEEK:
                            seek_impl(ec);
                            break;
                        case TSSEG:
                            tsseg_impl(ec);
                            break;
                        case PAUSE:
                            pause_impl(ec);
                            break;
                        case RESUME:
                            resume_impl(ec);
                            break;
                        case INFO:
                            info_impl(ec);
                            break;
                        case ERR:
                            error_impl(ec);
                            break;
                        default:
                            LOG_S(Logger::kLevelEvent, "[thread_fuction], unknown option: " << msg_info_.option);
                            break;
                        }
                    }
                }

                while (work_state_ == PlayManager::play) {
                    if (!s_msg_queue.empty()) {
                        work_state_ = PlayManager::command;
                        break;
                    }

                    if (blocked_) {
                        muxer_->get_buffer_time(buffer_time, playing_ec);
                        if (!playing_ec) {
                            if (buffer_time < buffer_time_) {
                                continue;
                            }
                        } else if (playing_ec == boost::asio::error::would_block) {
                            std::cout << "buffering..." << std::endl;
                            boost::this_thread::sleep(boost::posix_time::milliseconds(500));
                            continue;
                        } else if (playing_ec == boost::asio::error::eof) {
                            // 影片数据下载结束
                        } else {
                            last_error_ = playing_ec;
                            muxer_mod_.close(close_token_, ec);
                            if (!open_task_queue_.front().plays.front().resp.empty()) {
                                open_task_queue_.front().plays.front().resp(playing_ec, size_pair);
                                open_task_queue_.front().plays.front().resp.clear();
                            }
                            open_task_queue_.pop();
                            blocked_ = false;
                            work_state_ = PlayManager::command;
                            break;
                        }
                        blocked_ = false;
                    }

<<<<<<< .working
                    lec = Adapter_Read(buffer, 524288, &read_size);
                    if (lec == ppbox_success) {
                        // 限速设计
                        ++sum;
                        if (sum >= 300) {
                            sum = 0;
                            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
=======
                    if (ts_seg_next_index_ == 0 && open_task_queue_.front().format == "m3u8") {
                        //ts_seg_next_index_ = 1;
                        M3U8Protocol msu8(10, muxer_->media_info().duration);
                        boost::asio::write(
                            *open_task_queue_.front().plays.front().write_socket, 
                            boost::asio::buffer(msu8.read_buffer()), 
                            boost::asio::transfer_all(), 
                            playing_ec);
                        open_task_queue_.front().plays.front().resp(playing_ec, size_pair);
                        open_task_queue_.front().plays.front().resp.clear();
                        work_state_ = PlayManager::command;
                    } else {
                        if (!segment_end_) {
                            muxer_->read(&mux_tag_, playing_ec);
                            if (!playing_ec) {
                                ++sum;
                                if (sum >= 300) {
                                    sum = 0;
                                    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
                                }
                                if (ts_seg_next_index_ > 0
                                    && open_task_queue_.front().format == "m3u8"
                                    && mux_tag_.is_sync
                                    && mux_tag_.itrack == muxer_->video_track_index()
                                    && mux_tag_.time/1000 >= 10*(ts_seg_next_index_-1) ) {
                                        open_task_queue_.front().plays.front().resp(playing_ec, size_pair);
                                        open_task_queue_.front().plays.front().resp.clear();
                                        work_state_ = PlayManager::command;
                                        segment_end_ = true;
                                } else {
                                    write_data_to_client(mux_tag_, playing_ec);
                                    if (playing_ec) {
                                        LOG_S(Logger::kLevelEvent, "write socket error: ec = " << playing_ec.message());
                                        playing_ec = error::httpd_client_closed;
                                        work_state_ = PlayManager::command;
                                    }
                                }
                            } else {
                                sum = 0;
                                if (playing_ec == boost::asio::error::would_block) {
                                    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
                                    std::cout << "pptv would block" << std::endl;
                                    boost::uint64_t current_play_time =  muxer_->current_time();
                                    if (current_play_time > (seek_time_ + 10)) {
                                        blocked_ = true;
                                    }
                                    continue;
                                } else if (playing_ec == ppbox::demux::error::no_more_sample) {
                                    work_state_ = PlayManager::command;
                                    playing_ec = error::httpd_stream_end;
                                    open_task_queue_.front().is_stream_end = true;
                                    open_task_queue_.front().plays.front().resp(playing_ec, size_pair);
                                    open_task_queue_.front().plays.front().resp.clear();
                                    LOG_S(Logger::kLevelEvent, "stream end value: " << ec.value()
                                        << ", message: " << ec.message());
                                } else {
                                    open_task_queue_.front().plays.front().resp(playing_ec, size_pair);
                                    open_task_queue_.front().plays.front().resp.clear();
                                    work_state_ = PlayManager::command;
                                }
                            }
                        } else {
                            segment_end_ = false;
                            if (open_task_queue_.front().plays.front().format == "m3u8") {
                                boost::uint32_t head_size = 0;
                                unsigned char const * head_buffer = muxer_->get_head(head_size);
                                boost::asio::write(
                                    *open_task_queue_.front().plays.front().write_socket, 
                                    boost::asio::buffer(head_buffer, head_size), 
                                    boost::asio::transfer_all(), 
                                    playing_ec);
                            }
                            if (!playing_ec) {
                                write_data_to_client(mux_tag_, playing_ec);
                                if (playing_ec) {
                                    LOG_S(Logger::kLevelEvent, "write socket error: ec = " << playing_ec.message());
                                    playing_ec = error::httpd_client_closed;
                                    work_state_ = PlayManager::command;
                                }
                            }
                        }
                    }

                    if (work_state_ == PlayManager::command) {
                        // failed or finish
                        last_error_ = playing_ec;
                        MsgInfo msg;
                        if (s_msg_queue.timed_pop(msg, boost::posix_time::milliseconds(300000))) {
                            s_msg_queue.push(msg);
                        } else {
                            muxer_mod_.close(close_token_, ec);
                            muxer_ = NULL;
                            if (!open_task_queue_.front().plays.front().resp.empty()) {
                                open_task_queue_.front().plays.front().resp(playing_ec, size_pair);
                                open_task_queue_.front().plays.front().resp.clear();
                            }
                            open_task_queue_.pop();
                        }
                    }
                } // End while (work_state_ == PlayManager::play)
            } // End while
        }

        error_code PlayManager::write_data_to_client(
            MuxTagEx & tag,
            error_code & ec)
        {
            boost::asio::write(
                *open_task_queue_.front().plays.front().write_socket, 
                boost::asio::buffer(tag.tag_header_buffer, tag.tag_header_length), 
                boost::asio::transfer_all(), 
                ec);
            if (!ec) {
                boost::asio::write(
                    *open_task_queue_.front().plays.front().write_socket, 
                    boost::asio::buffer(tag.tag_data_buffer, tag.tag_data_length), 
                    boost::asio::transfer_all(), 
                    ec);
                if (!ec) {
                    boost::asio::write(
                        *open_task_queue_.front().plays.front().write_socket, 
                        boost::asio::buffer(tag.tag_size_buffer, tag.tag_size_length), 
                        boost::asio::transfer_all(), 
                        ec);
                }
            }
            return ec;
        }

        bool PlayManager::insert_to_openlist(void)
        {
            bool res = false;
            LOG_S(Logger::kLevelEvent, "[insert_to_openlist], open list size: "
                << open_task_queue_.size());
            OpenInfo open_info;
            error_code lec = error::httpd_operation_canceled;
            if (open_task_queue_.size() == 2) {
                if (is_same_movie(open_task_queue_.back())) {
                    merge(open_task_queue_.back());
                } else {
                    handle_all(open_task_queue_.back(), lec);
                    open_info.playlink = msg_info_.url;
                    open_info.format   = msg_info_.format;
                    open_info.type     = msg_info_.type;
                    if (PLAY == msg_info_.option) {
                        open_info.plays.push_back(msg_info_);
                    } else if (MEDIAINFO == msg_info_.option) {
                        open_info.mediainfo.push_back(msg_info_);
                    }
                    open_task_queue_.back() = open_info;
                }
            } else if (open_task_queue_.size() == 1) {
                if (is_same_movie(open_task_queue_.front())) {
                    // merge
                    merge(open_task_queue_.front());
                    ts_seg_next_index_ = 0;
                    if (is_play()) {
                        work_state_ = PlayManager::play;
                        // seek失败继续播放
                        handle_seek(last_error_);
                    }
                } else {
                    open_info.playlink = msg_info_.url;
                    open_info.format   = msg_info_.format;
                    open_info.type     = msg_info_.type;
                    if (PLAY == msg_info_.option) {
                        open_info.plays.push_back(msg_info_);
                    } else if (MEDIAINFO == msg_info_.option) {
                        open_info.mediainfo.push_back(msg_info_);
                    }
                    open_task_queue_.push(open_info);
                    res = true;
                }
            } else if (open_task_queue_.empty() ) {
                open_info.playlink = msg_info_.url;
                open_info.format   = msg_info_.format;
                open_info.type     = msg_info_.type;
                if (PLAY == msg_info_.option) {
                    open_info.plays.push_back(msg_info_);
                } else if(MEDIAINFO == msg_info_.option) {
                    open_info.mediainfo.push_back(msg_info_);
                }
                open_task_queue_.push(open_info);
                res = true;
            }
            return res;
        }

        bool PlayManager::is_same_movie(OpenInfo const & open_info)
        {
            bool res = false;
            if (strncasecmp(msg_info_.url, open_info.playlink)
                && strncasecmp(msg_info_.format, open_info.format)
                && strncasecmp(msg_info_.type, open_info.type)) {
                res = true;
            }
            return res;
        }

        void PlayManager::merge(OpenInfo & open_info)
        {
            // merge
            if (MEDIAINFO == msg_info_.option) {
                open_info.mediainfo.push_back(msg_info_);
            } else if (PLAY == msg_info_.option) {
                if (!open_info.plays.empty()) {
                    error_code ec = error::httpd_operation_canceled;
                    handle_one(open_info.plays.front(), ec);
                    open_info.plays.pop_front();
                }
                open_info.plays.push_back(msg_info_);
            } else {
                assert(0);
            }
        }

        void PlayManager::handle_mediainfo(std::list<MsgInfo> & mediainfo)
        {
            assert(muxer_ != NULL);
            error_code ec;
            if (!mediainfo.empty()) {
                MediaFileInfo const & media_info = muxer_->media_info();
                while (!mediainfo.empty()) {
                    write_mediainfo_to_client(mediainfo.front().write_socket, media_info, ec);
                    mediainfo.front().resp(ec, size_pair);
                    mediainfo.pop_front();
                }
            }
        }

        void PlayManager::async_open_impl()
        {
            LOG_S(Logger::kLevelEvent, "[async_open_impl], open: " << msg_info_.url
                << ", seek time: " << msg_info_.seek_time
                << ", format: " << msg_info_.format
                << ", type: " << msg_info_.type);
            if (open_task_queue_.size() == 1) {
                open_task_queue_.front().open_state = OpenState::opening;
                seek_time_ = 0;
                muxer_mod_.async_open(
                    open_task_queue_.front().type+std::string("://")+
                    open_task_queue_.front().playlink,
                    open_task_queue_.front().format == "m3u8" ? "ts" : open_task_queue_.front().format,
                    close_token_,
                    boost::bind(&PlayManager::open_callback, this, _1, _2));
            } else if (open_task_queue_.size() == 2) {
                assert(open_task_queue_.front().open_state == OpenState::opened ||
                    open_task_queue_.front().open_state == OpenState::opening);
                error_code ec;
                muxer_mod_.close(close_token_, ec);
                muxer_ = NULL;
                // 如果已经打开一个
                if (open_task_queue_.front().open_state == OpenState::opened) {
                    if (!open_task_queue_.front().plays.empty()) {
                        error_code ec = error::httpd_operation_canceled;
                        if (!open_task_queue_.front().plays.front().resp.empty()) {
                            open_task_queue_.front().plays.front().resp(ec, size_pair);
                            open_task_queue_.front().plays.front().resp.clear();
                        }
                        open_task_queue_.front().plays.pop_front();
                    }
                    open_task_queue_.pop();
                    open_task_queue_.front().open_state = OpenState::opening;
                    seek_time_ = 0;
                    muxer_mod_.async_open(
                        open_task_queue_.front().type+std::string("://")+
                        open_task_queue_.front().playlink,
                        open_task_queue_.front().format == "m3u8" ? "ts" : open_task_queue_.front().format,
                        close_token_,
                        boost::bind(&PlayManager::open_callback, this, _1, _2));
                }
            } else {
                assert(0);
            }
        }

        void PlayManager::open_callback(
            error_code const & ec,
            MuxerBase * muxer)
        {
            LOG_SECTION();
            MsgInfo msg_info;
            msg_info.ec = ec;
            if (!ec) {
                muxer_ = muxer;
            }
            msg_info.option = CALL;
            s_msg_queue.push(msg_info);
        }

        bool PlayManager::is_open(void)
        {
            bool res = false;
            if (!open_task_queue_.empty() &&
                open_task_queue_.front().open_state == OpenState::opened) {
                res = true;
            }
            return res;
        }

        bool PlayManager::is_play(void)
        {
            bool res = false;
            if (!open_task_queue_.empty() &&
                !open_task_queue_.front().plays.empty() &&
                open_task_queue_.front().open_state == OpenState::opened) {
                    res = true;
            }
            return res;
        }

        error_code PlayManager::open_impl(error_code & ec)
        {
            LOG_S(Logger::kLevelEvent, "[open_impl], open: " << msg_info_.url
                << ", seek time: " << msg_info_.seek_time
                << ", format: " << msg_info_.format
                << ", type: " << msg_info_.type);
            if (false) {
                last_error_ = error::httpd_not_authed;
                write_error_respone(msg_info_.write_socket,
                    last_error_.value(), last_error_.message(), "open", ec);
                LOG_S(Logger::kLevelAlarm, "[open_impl], ec = " << last_error_.message());
                msg_info_.resp(ec, size_pair);
            } else {
                if (insert_to_openlist()) {
                    async_open_impl();
                }
            }
            return ec;
        }

        boost::system::error_code PlayManager::start_impl(
            boost::system::error_code & ec)
        {
            ec.clear();
            //PP_int32 lec = ppbox_success;
            //if (!is_certify_) {
            //    lec = PPBOX_StartP2PEngine(msg_info_.gid.c_str(),
            //        msg_info_.pid.c_str(),
            //        msg_info_.auth.c_str());
            //    if (lec != ppbox_success) {
            //        is_certify_ = false;
            //        ec = httpd_auth_failed;
            //        last_error_ = ec_translate(lec);
            //    } else {
            //        is_certify_ = true;
            //    }
            //    LOG_S(Logger::kLevelEvent, "[start_impl], value = " << ec.value()
            //        << ", msg = " << ec.message());
            //    if (ec) {
            //        write_error_respone(msg_info_.write_socket,
            //            ec.value(), ec.message(), "start", ec);
            //    } else {
            //        write_success_respone(msg_info_.write_socket, "start", ec);
            //    }
            //} else {
            //    write_success_respone(msg_info_.write_socket, "start", ec);
            //}
            //msg_info_.resp(ec, size_pair);
            return ec;
        }

        boost::system::error_code PlayManager::mediainfo_impl(
            boost::system::error_code & ec)
        {
            ec.clear();
            LOG_S(Logger::kLevelEvent, "[mediainfo_impl], open: " << msg_info_.url
                << ", format: " << msg_info_.format
                << ", type: " << msg_info_.type);
            if (is_open() && is_same_movie(open_task_queue_.front())) {
                if (is_play() && !pause_) {
                    work_state_ = PlayManager::play;
                }
                MediaFileInfo const & media_info = muxer_->media_info();
                write_mediainfo_to_client(msg_info_.write_socket, media_info, ec);
                msg_info_.resp(ec, size_pair);
            } else {
                open_impl(ec);
            }
            return ec;
        }

        boost::system::error_code PlayManager::seek_impl(
            boost::system::error_code & ec)
        {
            if (!is_play()) {
                ec = error::httpd_not_open;
            } else {
                if (!pause_) {
                    work_state_ = PlayManager::play; 
                }
                muxer_->seek(msg_info_.seek_time*1000, ec);
                if (ec) {
                    last_error_ = ec;
                }
            }
            LOG_S(Logger::kLevelEvent, "[seek_impl], value = " << ec.value()
                << ", msg = " << ec.message());
            if (ec) {
                write_error_respone(msg_info_.write_socket,
                    ec.value(), ec.message(), "seek", ec);

            } else {
                write_success_respone(msg_info_.write_socket, "seek", ec);
            }
            msg_info_.resp(ec, size_pair);
            return ec;
        }

        error_code PlayManager::tsseg_impl(
            error_code & ec)
        {
            boost::uint32_t cur_index = boost::uint32_t(-1);
            if (!is_play()) {
                ec = error::httpd_not_open;
            } else {
                cur_index = atoi(msg_info_.url.c_str());
                open_task_queue_.front().plays.front().write_socket = msg_info_.write_socket;
                open_task_queue_.front().plays.front().resp = msg_info_.resp;
                if (ts_seg_next_index_ != cur_index) {
                    muxer_->seek((cur_index-1)*10*1000, ec);
                    if (!ec || ec == boost::asio::error::would_block) {
                        open_task_queue_.front().is_stream_end = false;
                        ts_seg_next_index_ = cur_index+1;
                        work_state_ = PlayManager::play;
                    } else {
                        open_task_queue_.front().plays.front().resp(ec, size_pair);
                        open_task_queue_.front().plays.front().resp.clear();
                    }
                } else {
                    ts_seg_next_index_++;
                    if (open_task_queue_.front().is_stream_end) {
                        ec = boost::asio::error::eof;
                        //error_code lec;
                        //write_error_respone(msg_info_.write_socket,
                        //    ec.value(), ec.message(), "tsseg_impl", lec);
                        open_task_queue_.front().plays.front().resp(ec, size_pair);
                        open_task_queue_.front().plays.front().resp.clear();
                    } else {
                        work_state_ = PlayManager::play;
                    }
                }
            }
            LOG_S(Logger::kLevelEvent, "[tsseg_impl], value = " << ec.value()
                << ", msg = " << ec.message() << ", index = " << cur_index);
            return ec;
        }

        error_code PlayManager::pause_impl(
            error_code & ec)
        {
            if (!is_play()) {
                ec = error::httpd_not_open;
            } else {
                if (is_live_url(open_task_queue_.front().type)) {
                    ec = error::httpd_logic_error; // not support
                    work_state_ = PlayManager::play;
                } else {
                    muxer_->pause(ec);
                    if (ec) {
                        work_state_ = PlayManager::play;
                        last_error_ = ec;
                    } else {
                        work_state_ = PlayManager::command;
                        pause_ = true;
                    }
                }
            }
            LOG_S(Logger::kLevelEvent, "[pause_impl], value = " << ec.value()
                << ", msg = " << ec.message());
            if (ec) {
                write_error_respone(msg_info_.write_socket,
                    ec.value(), ec.message(), "pause", ec);
            } else {
                write_success_respone(msg_info_.write_socket, "pause", ec);
            }
            msg_info_.resp(ec, size_pair);
            return ec;
        }

        error_code PlayManager::resume_impl(
            error_code & ec)
        {
            if (!is_play()) {
                ec = error::httpd_not_open;
            } else {
                if (is_live_url(open_task_queue_.front().type)) {
                    ec = error::httpd_logic_error; // not support
                    work_state_ = PlayManager::play;
                } else {
                    muxer_->resume(ec);
                    if (ec) {
                        work_state_ = PlayManager::command;
                        last_error_ = ec;
                    } else {
                        pause_ = false;
                        work_state_ = PlayManager::play;
                    }
                }
            }
            LOG_S(Logger::kLevelEvent, "[resume_impl], value = " << ec.value()
                << ", msg = " << ec.message());
            if (ec) {
                write_error_respone(msg_info_.write_socket,
                    ec.value(), ec.message(), "resume", ec);
            } else {
                write_success_respone(msg_info_.write_socket, "resume", ec);
            }
            msg_info_.resp(ec, size_pair);
            return ec;
        }

        error_code  PlayManager::callback_impl(error_code & ec)
        {
            ec.clear();
            assert(!open_task_queue_.empty());
            LOG_S(Logger::kLevelEvent, "[callback_impl] ec, value = " << msg_info_.ec.value()
                << ", message: " <<msg_info_.ec.message() );
            pause_ = false;
            if (msg_info_.ec) {
                muxer_mod_.close(close_token_, ec);
                muxer_ = NULL;
                last_error_ = msg_info_.ec;
                handle_all(open_task_queue_.front(), last_error_);
                open_task_queue_.pop();
                if (!open_task_queue_.empty()) {
                    open_task_queue_.front().open_state = OpenState::opening;
                    seek_time_ = 0;
                    muxer_mod_.async_open(
                        open_task_queue_.front().type+std::string("://")+
                        open_task_queue_.front().playlink,
                        open_task_queue_.front().format == "m3u8" ? "ts" : open_task_queue_.front().format,
                        close_token_,
                        boost::bind(&PlayManager::open_callback, this, _1, _2));
                }
            } else {
#ifdef   _WRITE_MUX_FILE_
                std::string filename = std::string("muxout.")
                    + open_task_queue_.front().format;
                out_mux_file.open(filename.c_str(), std::ios::binary);
#endif
                last_error_.clear();
                open_task_queue_.front().open_state = OpenState::opened;
                ts_seg_next_index_ = 0;
                handle_mediainfo(open_task_queue_.front().mediainfo);
                if (!open_task_queue_.front().plays.empty()) {
                        work_state_ = PlayManager::play;
                        handle_seek(last_error_);
                }
            }
            return ec;
        }

        error_code PlayManager::info_impl(error_code & ec)
        {
            boost::uint64_t position_time;
            boost::uint32_t buffer_time;
            if (!is_play()) {
                ec = error::httpd_not_open;
            } else {
                if (is_play() && !pause_) {
                    work_state_ = PlayManager::play;
                }
                muxer_->get_buffer_time(buffer_time, ec);
                if (!ec) {
                    position_time = muxer_->current_time();
                }
            }
            LOG_S(Logger::kLevelEvent, "[info_impl], value = " << ec.value()
                << ", msg = " << ec.message());
            if (ec) {
                write_error_respone(msg_info_.write_socket,
                    ec.value(), ec.message(), "info", ec);
            } else {
                write_playing_info_to_client(msg_info_.write_socket, buffer_time, position_time, ec);
            }
            msg_info_.resp(ec, size_pair);
            return ec;
        }

        error_code PlayManager::error_impl(
            error_code & ec)
        {
            if (is_play() && !pause_) {
                work_state_ = PlayManager::play;
            }
            write_error_info_to_client(
                msg_info_.write_socket,
                last_error_,
                ec);
            LOG_S(Logger::kLevelEvent, "[error_impl], value = " << ec.value()
                << ", msg = " << ec.message());
            msg_info_.resp(ec, size_pair);
            return ec;
        }

        void PlayManager::handle_all(
            OpenInfo & open_info,
            error_code const & ec)
        {
            error_code lec;
            assert(open_info.plays.size() <= 1);
            if (!open_info.plays.empty()) {
                write_error_respone(
                    open_info.plays.front().write_socket,
                    ec.value(),
                    ec.message(),
                    "open",
                    lec);
                open_info.plays.front().resp(lec, size_pair);
            }
            while (!open_info.mediainfo.empty()) {
                write_error_respone(
                    open_info.mediainfo.front().write_socket,
                    ec.value(),
                    ec.message(),
                    "mediainfo",
                    lec);
                open_info.mediainfo.front().resp(lec, size_pair);
                open_info.mediainfo.pop_front();
            }
        }

        void PlayManager::handle_one(
            MsgInfo & msg_info,
            error_code const & ec)
        {
            error_code lec;
            std::string module;
            if (msg_info.option == PLAY) {
                module = "open";
            } else if (msg_info.option == MEDIAINFO) {
                module = "mediainfo";
            }
            if (!msg_info.resp.empty()) {
                write_error_respone(
                    msg_info.write_socket,
                    ec.value(),
                    ec.message(),
                    "open",
                    lec);
                msg_info.resp(lec, size_pair);
            }
        }

        error_code PlayManager::handle_seek(error_code & ec)
        {
            if (!is_live_url(open_task_queue_.front().type) 
                && open_task_queue_.front().format != "m3u8") {
                muxer_->seek(open_task_queue_.front().plays.front().seek_time * 1000, ec);
                if (!ec || ec == boost::asio::error::would_block) {
                    seek_time_ = open_task_queue_.front().plays.front().seek_time;
                }
            }
            muxer_->reset();
            return ec;
        }

        error_code PlayManager::write_mediainfo_to_client(
            util::protocol::HttpSocket * handle,
            ppbox::mux::MediaFileInfo const & media_info,
            error_code & ec)
        {
            TiXmlDocument doc;
            const char* strXMLContent =
                "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<template module=\"mediainfo\" version=\"1.0\">"
                "<success>0</success>"
                "</template>";

            doc.Parse( strXMLContent );
            if ( !doc.Error() )
            {
                TiXmlNode* node = 0;
                TiXmlElement* element = 0;
                node = doc.FirstChild( "template" );
                element = node->ToElement();

                TiXmlElement duration("duration");
                duration.SetAttribute("value", format(media_info.duration).c_str());

                TiXmlElement video("video");
                video.SetAttribute("codec", "h264");
                TiXmlElement videoproperty("property");
                videoproperty.SetAttribute("frame-rate", format(media_info.frame_rate).c_str());
                videoproperty.SetAttribute("width", format(media_info.width).c_str());
                videoproperty.SetAttribute("height", format(media_info.height).c_str());
                video.InsertEndChild(videoproperty);

                TiXmlElement audio("audio");
                audio.SetAttribute("codec", "aac");
                TiXmlElement audioproperty("property");
                audioproperty.SetAttribute("channels", format(media_info.channel_count).c_str());
                audioproperty.SetAttribute("sample-rate", format(media_info.sample_rate).c_str());
                audioproperty.SetAttribute("sample-size", format(media_info.sample_size).c_str());
                audio.InsertEndChild(audioproperty);

                element->InsertEndChild(duration);
                element->InsertEndChild(video);
                element->InsertEndChild(audio);
            }

            TiXmlPrinter printer;
            printer.SetStreamPrinting();
            doc.Accept( &printer );
            std::string end_xml_content = printer.CStr();

            boost::asio::write(
                *handle, 
                boost::asio::buffer(end_xml_content.c_str(), end_xml_content.size()), 
                boost::asio::transfer_all(), 
                ec);

            return ec;
        }

        error_code PlayManager::write_playing_info_to_client(
            util::protocol::HttpSocket * handle,
            boost::uint32_t buffer_time,
            boost::uint64_t time,
            error_code & ec)
        {
            TiXmlDocument doc;
            const char* strXMLContent =
                "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<template module=\"info\" version=\"1.0\">"
                "<success>0</success>"
                "</template>";

            doc.Parse( strXMLContent );
            if ( !doc.Error() )
            {
                TiXmlNode* node = 0;
                TiXmlElement* element = 0;
                node = doc.FirstChild( "template" );
                element = node->ToElement();

                TiXmlElement playtime("playtime");
                playtime.SetAttribute("value", format(time).c_str());

                TiXmlElement buffertime("buffertime");
                buffertime.SetAttribute("value", format(buffer_time).c_str());

                element->InsertEndChild(playtime);
                element->InsertEndChild(buffertime);
            }

            TiXmlPrinter printer;
            printer.SetStreamPrinting();
            doc.Accept( &printer );
            std::string end_xml_content = printer.CStr();

            boost::asio::write(
                *handle, 
                boost::asio::buffer(end_xml_content.c_str(), end_xml_content.size()), 
                boost::asio::transfer_all(), 
                ec);

            return ec;
        }

        error_code PlayManager::write_error_info_to_client(
            util::protocol::HttpSocket * handle,
            error_code const & last_error,
            error_code & ec)
        {
            std::string respone_str = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<template module=\"";
            // set module
            respone_str += "error";
            respone_str += "\" version=\"1.0\">";
            respone_str += "<success>0</success>";
            respone_str += "<error value=\"";
            // set error code value
            respone_str += format(last_error.value());
            respone_str += "\" msg=\"";
            // set error msg
            if (last_error) {
                respone_str += last_error.message();
            } else {
                respone_str += "";
            }
            
            respone_str += "\"></error>";
            respone_str += "</template>";

            boost::asio::write(
                *handle, 
                boost::asio::buffer(respone_str.c_str(), respone_str.size()), 
                boost::asio::transfer_all(), 
                ec);
            return ec;
        }

        error_code PlayManager::write_error_respone(
            util::protocol::HttpSocket * handle,
            boost::uint32_t   code,
            std::string const & error_msg,
            std::string const & module,
            error_code & ec)
        {
            std::string respone_str = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<template module=\"";
            // set module
            respone_str += module;
            respone_str += "\" version=\"1.0\">";
            respone_str += "<success>1</success>";
            respone_str += "<reason value=\"";
            // set error code value
            respone_str += format(code);
            respone_str += "\" msg=\"";
            // set error msg
            respone_str += error_msg;
            respone_str += "\"></reason>";
            respone_str += "</template>";

            boost::asio::write(
                *handle, 
                boost::asio::buffer(respone_str.c_str(), respone_str.size()), 
                boost::asio::transfer_all(), 
                ec);
            return ec;
        }

        error_code PlayManager::write_success_respone(
            util::protocol::HttpSocket * handle,
            std::string const & module,
            error_code & ec)
        {
            std::string respone_str = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                "<template module=\"";
            // set module
            respone_str += module;
            respone_str += "\" version=\"1.0\">";
            respone_str += "<success>0</success>";
            respone_str += "</template>";

            boost::asio::write(
                *handle, 
                boost::asio::buffer(respone_str.c_str(), respone_str.size()), 
                boost::asio::transfer_all(), 
                ec);

            return ec;
        }

//        void PlayManager::dump_stat(Adapter_PlayStatistic &msg)
//        {
//            boost::uint64_t play_time = 0;
//            Adapter_GetPlayTime(&play_time);
//            play_time = play_time / 1000;
//#ifdef WIN32
//#else
//            std::cout << "\r";
//            std::cout << "\033[K";
//#endif
//#ifdef WIN32
//#else
//            if (msg.play_status == ppbox_closed) {
//                std::cout << "\033[1;35m";
//            } else if (msg.play_status == ppbox_playing) {
//                std::cout << "\033[1;32m";
//            } else if (msg.play_status == ppbox_buffering) {
//                std::cout << "\033[1;31m";
//            } else if (msg.play_status == ppbox_paused) {
//                std::cout << "\033[0;37m";
//            }
//#endif
//            std::cout << play_time 
//                << "\t" << msg.buffer_time / 1000 << "(" << msg.buffering_present << "%)";
//#ifdef WIN32
//            std::cout << std::endl;
//#else
//            std::cout << "\033[m";
//            std::cout << "\r";
//            std::cout << std::flush;
//#endif
//        }

        bool PlayManager::is_live_url(std::string const & type)
        {
            bool res = false;
            if (strncasecmp(type, "ppfile-asf") || strncasecmp(type, "pplive")) {
                res = true;
            }
            return res;
        }

    } // namespace httpd

} // namespace ppbox

