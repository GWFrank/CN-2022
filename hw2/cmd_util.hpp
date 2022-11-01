#ifndef CMD_UTIL_HPP
#define CMD_UTIL_HPP

#include <string>
#include <set>
#include <map>

#include <climits>

#define MAX_UNAME_LEN 16
#define MAX_STATUS_LEN 32
#define MAX_FILENAME_LEN NAME_MAX

namespace cmu {
    struct clientInfo {
        pthread_t tid;
        int sock_fd;
        std::set<std::string>* srv_blocklist_p;
        pthread_mutex_t* lock_p;
        std::string username;
        clientInfo()
            : tid(0),
            sock_fd(0),
            srv_blocklist_p(NULL),
            lock_p(NULL),
            username("") {}
        clientInfo(int _sock_fd, std::set<std::string>* _blocklist_p,
                pthread_mutex_t* _lock_p)
            : tid(0),
            sock_fd(_sock_fd),
            srv_blocklist_p(_blocklist_p),
            lock_p(_lock_p),
            username("") {}
    };

    const char SHELL_SYMBOL[] = "$ ";
    const char CONT_MSG[] = "cont";
    const char STOP_MSG[] = "stop";
    const char ADMIN[] = "admin";
    const char SRVDIR[] = "./server_dir";
    const char CLIDIR[] = "./client_dir";
    const char MPG_EXTN[] = ".mpg";

    // Normal commands
    const std::string LS_CMD("ls");
    const std::string PUT_CMD("put");
    const std::string GET_CMD("get");
    const std::string PLAY_CMD("play");

    // Admin commands
    const std::string BAN_CMD("ban");
    const std::string UNBAN_CMD("unban");
    const std::string BLOCKLLIST_CMD("blocklist");

    void check_and_mkdir(const char pathname[]);

    // Return 0 when file exist and is regular file, non-0 else
    int check_file_exist(const char pathname[]);

    // Return 0 when success, non-0 when error
    int ls_cli(int sock_fd);

    // Return 0 when success, non-0 when error
    int ls_srv(clientInfo* cinfo_p);

    // Return 0 when success, non-0 when error
    int put_cli(int sock_fd);

    // Return 0 when success, non-0 when error
    int put_srv(clientInfo* cinfo_p);

    // Return 0 when success, non-0 when error
    int get_cli(int sock_fd);

    // Return 0 when success, non-0 when error
    int get_srv(clientInfo* cinfo_p);

    // Return 0 when success, non-0 when error
    int play_cli(int sock_fd);

    // Return 0 when success, non-0 when error
    int play_srv(clientInfo* cinfo_p);

    // Return 0 when success, non-0 when error
    int ban_cli(int sock_fd);

    // Return 0 when success, non-0 when error
    int ban_srv(clientInfo* cinfo_p);

    // Return 0 when success, non-0 when error
    int unban_cli(int sock_fd);

    // Return 0 when success, non-0 when error
    int unban_srv(clientInfo* cinfo_p);

    // Return 0 when success, non-0 when error
    int blocklist_cli(int sock_fd);

    // Return 0 when success, non-0 when error
    int blocklist_srv(clientInfo* cinfo_p);

    typedef int (*cli_cmd_t)(int);
    typedef int (*srv_cmd_t)(clientInfo*);

    const std::set<std::string> ALL_CMDS{
        "ls",
        "put",
        "get",
        "play",
        "ban",
        "unban",
        "blocklist",
    };

    const std::set<std::string> ADMIN_CMDS{
        "ban",
        "unban",
        "blocklist",
    };

    const std::map<std::string, cli_cmd_t> cli_cmd_func{
        {"ls", ls_cli},
        {"put", put_cli},
        {"get", get_cli},
        {"play", play_cli},
        {"ban", ban_cli},
        {"unban", unban_cli},
        {"blocklist", blocklist_cli},
    };

    const std::map<std::string, srv_cmd_t> srv_cmd_func{
        {"ls", ls_srv},
        {"put", put_srv},
        {"get", get_srv},
        {"play", play_srv},
        {"ban", ban_srv},
        {"unban", unban_srv},
        {"blocklist", blocklist_srv},
    };
}


#endif // CMD_UTIL_HPP