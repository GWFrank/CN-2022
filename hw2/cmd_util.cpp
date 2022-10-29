#include "cmd_util.hpp"
#include "pkt_util.hpp"

#include <vector>
#include <string>

#include <pthread.h>
#include <cstdio>
#include <cstdint>

int ban_cli(int sock_fd) {
    bool reading = true;
    char in_char=-1, ret[MAX_BUF_SIZE];
    std::string arg_username("");
    int64_t ret_nums=-1;
    // Read and send banning usernames
    while (reading) {
        scanf("%c", &in_char);
        switch (in_char) {
            case '\n':
                reading = false;
            case ' ':
                if (arg_username != "") {
                    if (send_str(sock_fd, CONT_MSG)) {
                        goto ban_cli_err;
                    }
                    if (send_str(sock_fd, arg_username.c_str())) {
                        goto ban_cli_err;
                    }
                }
                arg_username.clear();
                break;
            default:
                arg_username += in_char;
                break;
        }
    }
    if (send_str(sock_fd, STOP_MSG)) {
        goto ban_cli_err;
    }
    // Receive command result
    if (recv_int64(sock_fd, ret_nums)) {
        goto ban_cli_err;
    }
    for (int64_t i=0; i<ret_nums; ++i) {
        if (recv_str(sock_fd, ret)) {
            goto ban_cli_err;
        }
        printf("%s", ret);
    }

    return 0;
ban_cli_err:
    return 1;
}

int ban_srv(clientInfo* cinfo_p) {
    char tgtuser_buf[MAX_UNAME_LEN]="", status[MAX_UNAME_LEN], buf[MAX_BUF_SIZE];
    // std::string ret("");
    std::vector<std::string> rets(0);
    // Receive banning usernames
    while (1) {
        if (recv_str(cinfo_p->sock_fd, status)) {
            goto ban_srv_err;
        }
        if (strncmp(status, STOP_MSG, MAX_UNAME_LEN) == 0) {
            break;
        }
        if (recv_str(cinfo_p->sock_fd, tgtuser_buf)) {
            goto ban_srv_err;
        }
        std::string tgtuser(tgtuser_buf);

        if (tgtuser == ADMIN) {
            snprintf(buf, MAX_BUF_SIZE, "You cannot ban yourself!\n");
        } else {
            if (!cinfo_p->srv_blocklist_p->count(tgtuser)) {
                pthread_mutex_lock(cinfo_p->lock_p);
                cinfo_p->srv_blocklist_p->insert(tgtuser);
                pthread_mutex_unlock(cinfo_p->lock_p);
                snprintf(buf, MAX_BUF_SIZE, "Ban %s successfully!\n", tgtuser.c_str());
            } else {
                snprintf(buf, MAX_BUF_SIZE, "User %s is already on the blocklist!\n", tgtuser.c_str());
            }
        }
        rets.push_back(std::string(buf));
    }
    // Send command result
    if (send_int64(cinfo_p->sock_fd, rets.size())) {
        goto ban_srv_err;
    }
    for (auto it=rets.begin(); it!= rets.end(); ++it) {
        if (send_str(cinfo_p->sock_fd, it->c_str())) {
            goto ban_srv_err;
        }
    }

    return 0;
ban_srv_err:
    return 1;
}

int unban_cli(int sock_fd) {
    bool reading = true;
    char in_char=-1, ret[MAX_BUF_SIZE];
    std::string arg_username("");
    int64_t ret_nums=-1;
    // Read and send banning usernames
    while (reading) {
        scanf("%c", &in_char);
        switch (in_char) {
            case '\n':
                reading = false;
            case ' ':
                if (arg_username != "") {
                    if (send_str(sock_fd, CONT_MSG)) {
                        goto unban_cli_err;
                    }
                    if (send_str(sock_fd, arg_username.c_str())) {
                        goto unban_cli_err;
                    }
                }
                arg_username.clear();
                break;
            default:
                arg_username += in_char;
                break;
        }
    }
    if (send_str(sock_fd, STOP_MSG)) {
        goto unban_cli_err;
    }
    // Receive command result
    if (recv_int64(sock_fd, ret_nums)) {
        goto unban_cli_err;
    }
    for (int64_t i=0; i<ret_nums; ++i) {
        if (recv_str(sock_fd, ret)) {
            goto unban_cli_err;
        }
        printf("%s", ret);
    }

    return 0;
unban_cli_err:
    return 1;
}

int unban_srv(clientInfo* cinfo_p) {
    char tgtuser_buf[MAX_UNAME_LEN]="", status[MAX_UNAME_LEN], buf[MAX_BUF_SIZE];
    // std::string ret("");
    std::vector<std::string> rets(0);
    // Receive banning usernames
    while (1) {
        if (recv_str(cinfo_p->sock_fd, status)) {
            goto unban_srv_err;
        }
        if (strncmp(status, STOP_MSG, MAX_UNAME_LEN) == 0) {
            break;
        }
        if (recv_str(cinfo_p->sock_fd, tgtuser_buf)) {
            goto unban_srv_err;
        }
        std::string tgtuser(tgtuser_buf);

        
        if (cinfo_p->srv_blocklist_p->count(tgtuser)) {
            pthread_mutex_lock(cinfo_p->lock_p);
            cinfo_p->srv_blocklist_p->erase(tgtuser);
            pthread_mutex_unlock(cinfo_p->lock_p);
            snprintf(buf, MAX_BUF_SIZE, "Successfully removed %s from the blocklist!\n", tgtuser.c_str());
        } else {
            snprintf(buf, MAX_BUF_SIZE, "User %s is not on the blocklist!\n", tgtuser.c_str());
        }
        
        rets.push_back(std::string(buf));
    }
    // Send command result
    if (send_int64(cinfo_p->sock_fd, rets.size())) {
        goto unban_srv_err;
    }
    for (auto it=rets.begin(); it!= rets.end(); ++it) {
        if (send_str(cinfo_p->sock_fd, it->c_str())) {
            goto unban_srv_err;
        }
    }

    return 0;
unban_srv_err:
    return 1;
}

int blocklist_cli(int sock_fd) {
    int64_t list_size=-1;
    char username[MAX_UNAME_LEN];
    if (recv_int64(sock_fd, list_size)) {
        goto blocklist_cli_err;
    }
    for (int64_t i=0; i<list_size; ++i) {
        if (recv_str(sock_fd, username)) {
            goto blocklist_cli_err;
        }
        printf("%s\n", username);
    }

    return 0;
blocklist_cli_err:
    return 1;
}

int blocklist_srv(clientInfo* cinfo_p) {
    auto blist_p = cinfo_p->srv_blocklist_p;
    if (send_int64(cinfo_p->sock_fd, blist_p->size())) {
        goto blocklist_srv_err;
    }
    for (auto it=blist_p->begin(); it!=blist_p->end(); ++it) {
        if (send_str(cinfo_p->sock_fd, it->c_str())) {
            goto blocklist_srv_err;
        }
    }

    return 0;
blocklist_srv_err:
    return 1;
}

int ls_cli(int sock_fd) {
    return 0;
}

// Return 0 when success, non-0 when error
int ls_srv(clientInfo* cinfo_p) {
    return 0;
}

// Return 0 when success, non-0 when error
int put_cli(int sock_fd) {
    return 0;
}

// Return 0 when success, non-0 when error
int put_srv(clientInfo* cinfo_p) {
    return 0;
}

// Return 0 when success, non-0 when error
int get_cli(int sock_fd) {
    return 0;
}

// Return 0 when success, non-0 when error
int get_srv(clientInfo* cinfo_p) {
    return 0;
}

// Return 0 when success, non-0 when error
int play_cli(int sock_fd) {
    return 0;
}

// Return 0 when success, non-0 when error
int play_srv(clientInfo* cinfo_p) {
    return 0;
}