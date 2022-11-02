#include "cmd_util.hpp"
#include "pkt_util.hpp"

#include <vector>
#include <string>

#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <cstdint>

namespace cmu{
    void check_and_mkdir(const char pathname[]) {
        struct stat stat_buf;
        if (stat(pathname, &stat_buf)
            || !S_ISDIR(stat_buf.st_mode)
        ) {
            mkdir(pathname, 0777); // follow umask
        }
    }

    int check_file_exist(const char pathname[]) {
        struct stat stat_buf;
        if (!stat(pathname, &stat_buf)
            && S_ISREG(stat_buf.st_mode)
        ) {
            return 1;
        }
        return 0;
    }

    int check_permission_cli(int sock_fd) {
        char status[MAX_STATUS_LEN]="";
        if (pku::recv_str(sock_fd, status)) {
            goto check_permission_cli_err;
        }
        if (strncmp(status, STOP_MSG, MAX_STATUS_LEN) == 0) {
            printf("Permission denied.\n");
            return 0;
        } else {
            return 1;
        }
    check_permission_cli_err:
        return -1;
    }

    int check_permission_srv(clientInfo* cinfo_p, const std::string &cmd) {
        bool banned = true;
        if (ADMIN_CMDS.count(cmd)) { // Admin commands
            if (cinfo_p->username != ADMIN_UNAME) {
                if (pku::send_str(cinfo_p->sock_fd, STOP_MSG)) {
                    goto check_permission_srv_err;
                }
                return 0;
            }
        }
        pthread_mutex_lock(cinfo_p->lock_p);
        banned = cinfo_p->srv_blocklist_p->count(cinfo_p->username);
        pthread_mutex_unlock(cinfo_p->lock_p);
        if (banned) { // Not in blocklist
            if (pku::send_str(cinfo_p->sock_fd, STOP_MSG)) {
                goto check_permission_srv_err;
            }
            return 0;
        } else {
            if (pku::send_str(cinfo_p->sock_fd, CONT_MSG)) {
                goto check_permission_srv_err;
            }
            return 1;
        }
    check_permission_srv_err:
        return -1;
    }

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
                        if (pku::send_str(sock_fd, CONT_MSG)) {
                            goto ban_cli_err;
                        }
                        if (pku::send_str(sock_fd, arg_username.c_str())) {
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
        if (pku::send_str(sock_fd, STOP_MSG)) {
            goto ban_cli_err;
        }
        // Receive command result
        if (pku::recv_int64(sock_fd, ret_nums)) {
            goto ban_cli_err;
        }
        for (int64_t i=0; i<ret_nums; ++i) {
            if (pku::recv_str(sock_fd, ret)) {
                goto ban_cli_err;
            }
            printf("%s", ret);
        }

        return 0;
    ban_cli_err:
        return 1;
    }

    int ban_srv(clientInfo* cinfo_p) {
        char tgtuser_buf[MAX_UNAME_LEN]="";
        char status[MAX_STATUS_LEN]="";
        char ret_buf[MAX_BUF_SIZE]="";
        // std::string ret("");
        std::vector<std::string> rets(0);
        // Receive banning usernames
        while (1) {
            if (pku::recv_str(cinfo_p->sock_fd, status)) {
                goto ban_srv_err;
            }
            if (strncmp(status, STOP_MSG, MAX_STATUS_LEN) == 0) {
                break;
            }
            if (pku::recv_str(cinfo_p->sock_fd, tgtuser_buf)) {
                goto ban_srv_err;
            }
            std::string tgtuser(tgtuser_buf);

            if (tgtuser == ADMIN_UNAME) {
                snprintf(ret_buf, MAX_BUF_SIZE, "You cannot ban yourself!\n");
            } else {
                if (!cinfo_p->srv_blocklist_p->count(tgtuser)) {
                    pthread_mutex_lock(cinfo_p->lock_p);
                    cinfo_p->srv_blocklist_p->insert(tgtuser);
                    pthread_mutex_unlock(cinfo_p->lock_p);
                    snprintf(ret_buf, MAX_BUF_SIZE, "Ban %s successfully!\n", tgtuser.c_str());
                } else {
                    snprintf(ret_buf, MAX_BUF_SIZE, "User %s is already on the blocklist!\n", tgtuser.c_str());
                }
            }
            rets.push_back(std::string(ret_buf));
        }
        // Send command result
        if (pku::send_int64(cinfo_p->sock_fd, rets.size())) {
            goto ban_srv_err;
        }
        for (auto it=rets.begin(); it!= rets.end(); ++it) {
            if (pku::send_str(cinfo_p->sock_fd, it->c_str())) {
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
                        if (pku::send_str(sock_fd, CONT_MSG)) {
                            goto unban_cli_err;
                        }
                        if (pku::send_str(sock_fd, arg_username.c_str())) {
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
        if (pku::send_str(sock_fd, STOP_MSG)) {
            goto unban_cli_err;
        }
        // Receive command result
        if (pku::recv_int64(sock_fd, ret_nums)) {
            goto unban_cli_err;
        }
        for (int64_t i=0; i<ret_nums; ++i) {
            if (pku::recv_str(sock_fd, ret)) {
                goto unban_cli_err;
            }
            printf("%s", ret);
        }

        return 0;
    unban_cli_err:
        return 1;
    }

    int unban_srv(clientInfo* cinfo_p) {
        char tgtuser_buf[MAX_UNAME_LEN]="";
        char status[MAX_STATUS_LEN]="";
        char buf[MAX_BUF_SIZE]="";
        // std::string ret("");
        std::vector<std::string> rets(0);
        // Receive banning usernames
        while (1) {
            if (pku::recv_str(cinfo_p->sock_fd, status)) {
                goto unban_srv_err;
            }
            if (strncmp(status, STOP_MSG, MAX_STATUS_LEN) == 0) {
                break;
            }
            if (pku::recv_str(cinfo_p->sock_fd, tgtuser_buf)) {
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
        if (pku::send_int64(cinfo_p->sock_fd, rets.size())) {
            goto unban_srv_err;
        }
        for (auto it=rets.begin(); it!= rets.end(); ++it) {
            if (pku::send_str(cinfo_p->sock_fd, it->c_str())) {
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
        if (pku::recv_int64(sock_fd, list_size)) {
            goto blocklist_cli_err;
        }
        for (int64_t i=0; i<list_size; ++i) {
            if (pku::recv_str(sock_fd, username)) {
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
        if (pku::send_int64(cinfo_p->sock_fd, blist_p->size())) {
            goto blocklist_srv_err;
        }
        for (auto it=blist_p->begin(); it!=blist_p->end(); ++it) {
            if (pku::send_str(cinfo_p->sock_fd, it->c_str())) {
                goto blocklist_srv_err;
            }
        }

        return 0;
    blocklist_srv_err:
        return 1;
    }

    int ls_cli(int sock_fd) {
        char filename[MAX_FILENAME_LEN]="";
        char status[MAX_STATUS_LEN]="";
        while (1) {
            if (pku::recv_str(sock_fd, status)) {
                goto ls_cli_err;
            }
            if (strncmp(status, STOP_MSG, MAX_STATUS_LEN) == 0) {
                break;
            }
            if (pku::recv_str(sock_fd, filename)) {
                goto ls_cli_err;
            }
            printf("%s\n", filename);

        }
        return 0;
    ls_cli_err:
        return 1;
    }

    // Return 0 when success, non-0 when error
    int ls_srv(clientInfo* cinfo_p) {
        char path_buf[MAX_BUF_SIZE] = "";
        snprintf(path_buf, MAX_BUF_SIZE, "./server_dir/%s", cinfo_p->username.c_str());
        DIR* user_dir = opendir(path_buf);
        dirent* user_file;
        // fprintf(stderr, "[ls] list %s\n", path_buf);
        while ((user_file = readdir(user_dir)) != NULL) {
            if (strncmp(user_file->d_name, ".", MAX_FILENAME_LEN)
                && strncmp(user_file->d_name, "..", MAX_FILENAME_LEN)) {
                if (pku::send_str(cinfo_p->sock_fd, CONT_MSG)) {
                    goto ls_srv_err;
                }
                if (pku::send_str(cinfo_p->sock_fd, user_file->d_name)) {
                    goto ls_srv_err;
                }
                // fprintf(stderr, "[ls] - %s\n", user_file->d_name);
            }
        }
        if (pku::send_str(cinfo_p->sock_fd, STOP_MSG)) {
            goto ls_srv_err;
        }
        closedir(user_dir);
        return 0;
    ls_srv_err:
        closedir(user_dir);
        return 1;
    }

    // Return 0 when success, non-0 when error
    int put_cli(int sock_fd) {
        char filename[MAX_FILENAME_LEN]="";
        char path_buf[MAX_BUF_SIZE]="";
        // Read and send filename to be uploaded;
        scanf("%s", filename);
        if (pku::send_str(sock_fd, filename)) {
            goto put_cli_err;
        }
        // Check existence
        snprintf(path_buf, MAX_BUF_SIZE, "%s/%s", CLIDIR, filename);
        if (!check_file_exist(path_buf)) {
            if (pku::send_str(sock_fd, STOP_MSG)) {
                goto put_cli_err;
            }
            printf("%s doesn't exist.\n", filename);
            return 0;
        } else {
            if (pku::send_str(sock_fd, CONT_MSG)) {
                goto put_cli_err;
            }
        }
        // Send file content
        printf("putting %s...\n", filename);
        if (pku::send_file(sock_fd, path_buf)) {
            goto put_cli_err;
        }
        return 0;
    put_cli_err:
        return 1;
    }

    // Return 0 when success, non-0 when error
    int put_srv(clientInfo* cinfo_p) {
        char filename[MAX_FILENAME_LEN]="";
        char status[MAX_STATUS_LEN]="";
        char path_buf[MAX_BUF_SIZE]="";
        // Receive filename
        if (pku::recv_str(cinfo_p->sock_fd, filename)) {
            goto put_srv_err;
        }
        // Receive existence status
        if (pku::recv_str(cinfo_p->sock_fd, status)) {
            goto put_srv_err;
        }
        if (strncmp(status, STOP_MSG, MAX_STATUS_LEN) == 0) {
            // fprintf(stderr, "[info] File not found on client side\n");
            return 0;
        }
        // Receive file content
        snprintf(path_buf, MAX_BUF_SIZE, "%s/%s/%s", SRVDIR, cinfo_p->username.c_str(), filename);
        if (pku::recv_file(cinfo_p->sock_fd, path_buf)) {
            goto put_srv_err;
        }
        return 0;
    put_srv_err:
        return 1;
    }

    // Return 0 when success, non-0 when error
    int get_cli(int sock_fd) {
        char filename[MAX_FILENAME_LEN]="";
        char path_buf[MAX_BUF_SIZE]="";
        char status[MAX_STATUS_LEN]="";
        // Read and send filename to be downloaded;
        scanf("%s", filename);
        if (pku::send_str(sock_fd, filename)) {
            goto get_cli_err;
        }
        // Receive existence status
        if (pku::recv_str(sock_fd, status)) {
            goto get_cli_err;
        }
        if (strncmp(status, STOP_MSG, MAX_STATUS_LEN) == 0) {
            printf("%s doesn't exist.\n", filename);
            return 0;
        }
        // Receive file content
        printf("getting %s...\n", filename);
        snprintf(path_buf, MAX_BUF_SIZE, "%s/%s", CLIDIR, filename);
        if (pku::recv_file(sock_fd, path_buf)) {
            goto get_cli_err;
        }
        return 0;
    get_cli_err:
        return 1;
    }

    // Return 0 when success, non-0 when error
    int get_srv(clientInfo* cinfo_p) {
        char filename[MAX_FILENAME_LEN]="";
        char path_buf[MAX_BUF_SIZE]="";
        // Receive filename
        if (pku::recv_str(cinfo_p->sock_fd, filename)) {
            goto get_srv_err;
        }
        // Check existence
        snprintf(path_buf, MAX_BUF_SIZE, "%s/%s/%s", SRVDIR, cinfo_p->username.c_str(), filename);
        if (!check_file_exist(path_buf)) {
            if (pku::send_str(cinfo_p->sock_fd, STOP_MSG)) {
                goto get_srv_err;
            }
            // fprintf(stderr, "[info] File not found on server side\n");
            return 0;
        } else {
            if (pku::send_str(cinfo_p->sock_fd, CONT_MSG)) {
                goto get_srv_err;
            }
        }
        // Send file content
        if (pku::send_file(cinfo_p->sock_fd, path_buf)) {
            goto get_srv_err;
        }
        return 0;
    get_srv_err:
        return 1;
    }

    // Return 0 when success, non-0 when error
    int play_cli(int sock_fd) {
        char video_path[MAX_FILENAME_LEN]="";
        char status[MAX_STATUS_LEN]="";
        // Read and send video filename
        scanf("%s", video_path);
        if (pku::send_str(sock_fd, video_path)) {
            goto play_cli_err;
        }
        // Check existence
        if (pku::recv_str(sock_fd, status)) {
            goto play_cli_err;
        }
        if (strncmp(status, STOP_MSG, MAX_STATUS_LEN) == 0) {
            printf("%s doesn't exist.\n", video_path);
            return 0;
        }
        // Check file extension
        if (pku::recv_str(sock_fd, status)) {
            goto play_cli_err;
        }
        if (strncmp(status, STOP_MSG, MAX_STATUS_LEN) == 0) {
            printf("%s is not an mpg file.\n", video_path);
            return 0;
        }
        // Receive video stream
        printf("playing the video...\n");
        if (pku::recv_video(sock_fd) == 1) {
            goto play_cli_err;
        }
        return 0;
    play_cli_err:
        return 1;
    }

    // Return 0 when success, non-0 when error
    int play_srv(clientInfo* cinfo_p) {
        char video_path[MAX_FILENAME_LEN]="";
        char path_buf[MAX_BUF_SIZE]="";
        int extn_idx=-1;
        // Receive video filename
        if (pku::recv_str(cinfo_p->sock_fd, video_path)) {
            goto play_srv_err;
        }
        snprintf(path_buf, MAX_BUF_SIZE, "%s/%s/%s", SRVDIR, cinfo_p->username.c_str(), video_path);
        // Check existence
        if (!check_file_exist(path_buf)) { // File doesn't exist
            if (pku::send_str(cinfo_p->sock_fd, STOP_MSG)) {
                goto play_srv_err;
            }
            return 0;
        } else {
            if (pku::send_str(cinfo_p->sock_fd, CONT_MSG)) {
                goto play_srv_err;
            }
        }
        // Check file extension
        extn_idx = strlen(video_path)-4;
        if (extn_idx <= 0
            || strncmp(video_path+extn_idx, MPG_EXTN, MAX_FILENAME_LEN) != 0
        ) { // Not *.mpg
            if (pku::send_str(cinfo_p->sock_fd, STOP_MSG)) {
                goto play_srv_err;
            }
            return 0;
        } else {
            if (pku::send_str(cinfo_p->sock_fd, CONT_MSG)) {
                goto play_srv_err;
            }
        }
        // Send video stream
        if (pku::send_video(cinfo_p->sock_fd, path_buf) == 1) {
            goto play_srv_err;
        }

        return 0;
    play_srv_err:
        return 1;
    }
}
