// HttpProxy.cpp

#include "ppbox/httpd/Common.h"
#include "ppbox/httpd/Version.h"
#include "ppbox/httpd/HttpManager.h"

#include <framework/process/Process.h>
#include <framework/process/SignalHandler.h>

#include <boost/bind.hpp>

#include <framework/logger/StreamRecord.h>
using namespace framework::logger;

//FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("ppbox_httpd",0)

int httpd_main(int argc, char * argv[])
{
    util::daemon::Daemon my_daemon("ppbox_httpd.conf");
    char const * default_argv[] = {
        "++Logger.stream_count=1", 
        "++Logger.ResolverService=1", 
        "++LogStream0.file=$LOG/ppbox_httpd.log", 
        "++LogStream0.append=true", 
        "++LogStream0.roll=true", 
        "++LogStream0.level=5", 
        "++LogStream0.size=102400", 
    };
    my_daemon.parse_cmdline(sizeof(default_argv) / sizeof(default_argv[0]), default_argv);
    my_daemon.parse_cmdline(argc, (char const **)argv);

    framework::process::SignalHandler sig_handler(
        framework::process::Signal::sig_int, 
        boost::bind(&util::daemon::Daemon::post_stop, &my_daemon), true);

    //framework::logger::global_logger().load_config(my_daemon.config());

    ppbox::common::log_versions();

    ppbox::common::CommonModule & common = 
        util::daemon::use_module<ppbox::common::CommonModule>(my_daemon, "ppbox_httpd");
    common.set_version(ppbox::httpd::version());


    util::daemon::use_module<ppbox::httpd::HttpManager>(my_daemon);

    my_daemon.start(framework::this_process::notify_wait);


    return 0;
}

#ifndef _LIB
int main(int argc, char * argv[])
{
    return httpd_main( argc, argv);
}
#endif

