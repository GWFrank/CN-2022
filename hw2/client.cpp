#include "pkt_util.hpp"
#include "cmd_util.hpp"

#include <string>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void read_until_newline() {
    char in_char=-1;
    while (in_char != '\n') {
        scanf("%c", &in_char);
    }
}

void cli_interact(int sock_fd, char* username) {
    // char in_buf[MAX_BUF_SIZE] = "";
    char out_buf[MAX_BUF_SIZE] = "";
    char scan_buf[MAX_BUF_SIZE] = "";
    // Login
    snprintf(out_buf, MAX_BUF_SIZE, "%s", username);
    if (pku::send_str(sock_fd, out_buf)) {
        goto cli_interact_exit;
    }
    printf("User %s successfully logged in.\n", username);
    // Check ./client_dir/ exist
    cmu::check_and_mkdir(cmu::CLIDIR);
    // Shell loop
    // fprintf(stderr, "[info] Entering shell loop\n");
    while (1) {
        printf("%s", cmu::SHELL_SYMBOL);
        // Read command from stdin
        scanf("%s", scan_buf);
        std::string cmd(scan_buf);
        // Send command
        snprintf(out_buf, MAX_BUF_SIZE, "%s", cmd.c_str());
        if (pku::send_str(sock_fd, out_buf)) {
            goto cli_interact_exit;
        }
        
        // Check permission
        int ret = cmu::check_permission_cli(sock_fd);
        if (ret == 1) { // Allowed
            // Check command validity and run
            if (cmu::ALL_CMDS.count(cmd)) {
                if (cmu::cli_cmd_func.find(cmd)->second(sock_fd)) {
                    goto cli_interact_exit;
                }
            } else {
                printf("Command not found.\n");
                read_until_newline();
            }
        } else if (ret == 0) { // Banned
            read_until_newline();
            continue;
        } else if (ret == -1) { // Errored
            goto cli_interact_exit;
        } else { // Weird cases
            ERR_EXIT("Unexpected return from permission checking");
        }
    }
cli_interact_exit:
    return;
}

int main(int argc, char *argv[]) {
    int PORT;
    char SRV_ADDR[16];
    // char* USERNAME;
    char USERNAME[11];
    // Handle arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: ./client <username> <server address>:<port>\n");
        exit(1);
    }
    // Parse username
    // USERNAME = (char*)malloc((strlen(argv[1])+1)*sizeof(char));
    snprintf(USERNAME, 11, "%s", argv[1]);
    // Parse <address>:<port>
    // SRV_ADDR = (char*)malloc((strlen(argv[2])+1)*sizeof(char));
    int sep_idx = 0;
    while (argv[2][sep_idx] != ':' && sep_idx<16) {
        SRV_ADDR[sep_idx] = argv[2][sep_idx]; 
        sep_idx++;
    }
    SRV_ADDR[sep_idx] = 0;
    if ((PORT=atoi(&argv[2][sep_idx+1])) == 0) {
        fprintf(stderr, "Invalid port number given\n");
        exit(1);
    }
    

    int sock_fd;
    struct sockaddr_in addr;

    // Get socket file descriptor
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ERR_EXIT("socket failed\n");
    }

    // Set server address
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(SRV_ADDR);
    addr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ERR_EXIT("connect failed\n");
    }

    cli_interact(sock_fd, USERNAME);

    close(sock_fd);
    return 0;
}
