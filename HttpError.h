// Error.h

#ifndef _PPBOX_HTTP_ERROR_H_
#define _PPBOX_HTTP_ERROR_H_

namespace ppbox
{
    namespace httpd
    {

        namespace error {

            enum errors
            {
                // ·­ÒëppboxµÄ´íÎóÂë
                httpd_not_start = 1, 
                httpd_already_start, 
                httpd_not_open, 
                httpd_already_open, 
                httpd_operation_canceled, 
                httpd_would_block, 
                httpd_stream_end, 
                httpd_logic_error, 
                httpd_network_error, 
                httpd_demux_error, 
                httpd_certify_error, 
                // HttpdµÄ´íÎóÂë
                httpd_option_not_support = 100,
                httpd_not_authed,
                httpd_auth_failed,
                httpd_client_closed,
                httpd_seek_error,
                httpd_request_error,
                httpd_other_error = 1024,
            };

            namespace detail {

                class http_category
                    : public boost::system::error_category
                {
                public:
                    http_category()
                    {
                        register_category(*this);
                    }

                    const char* name() const
                    {
                        return "httpd";
                    }

                    std::string message(int value) const
                    {
                        switch (value) {
                            // ·­ÒëppboxµÄ´íÎóÂë
                            case httpd_not_start:
                                return "pptv not start";
                            case httpd_already_start:
                                return "pptv already start";
                            case httpd_not_open:
                                return "pptv not open";
                            case httpd_already_open:
                                return "pptv already open";
                            case httpd_operation_canceled:
                                return "pptv operation canceled";
                            case httpd_would_block:
                                return "pptv would block";
                            case httpd_stream_end:
                                return "pptv stream end";
                            case httpd_logic_error:
                                return "pptv logic error";
                            case httpd_network_error:
                                return "pptv network error";
                            case httpd_demux_error:
                                return "pptv demux error";
                            case httpd_certify_error:
                                return "pptv certify error";
                            // HttpdµÄ´íÎóÂë
                            case httpd_option_not_support:
                                return "httpd option not support";
                            case httpd_not_authed:
                                return "httpd not authed";
                            case httpd_auth_failed:
                                return "httpd auth failed";
                            case httpd_client_closed:
                                return "httpd client closed";
                            case httpd_seek_error:
                                return "httpd seek error";
                            case httpd_request_error:
                                return "httpd request error";
                            default:
                                return "other error";
                        }
                    }
                };

            } // namespace detail

            inline const boost::system::error_category & get_category()
            {
                static detail::http_category instance;
                return instance;
            }

            inline boost::system::error_code make_error_code(
                errors e)
            {
                return boost::system::error_code(
                    static_cast<int>(e), get_category());
            }

        } // namespace error

    } // namespace httpd
} // namespace ppbox

namespace boost
{
    namespace system
    {

        template<>
        struct is_error_code_enum<ppbox::httpd::error::errors>
        {
            BOOST_STATIC_CONSTANT(bool, value = true);
        };

#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
        using ppbox::httpd::error::make_error_code;
#endif

    }
}

#endif // _PPBOX_HTTP_ERROR_H_
