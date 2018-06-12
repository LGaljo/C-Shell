#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


// TEXT COLOURING
#define RESET       "\033[0m"            /* Regular BW */
#define BOLDGREEN   "\033[1m\033[32m"    /* Bold Green */
#define ITALICGREEN "\033[3m\033[32m"    /* Italic Green */


// TYPE STRUCT
typedef struct argus {
    char *argv[50];
    int args;
    bool bg;
    int fd_i;
    int fd_o;
} argus;


// STATIC VARIABLES
static char shell_name[64] = "mysh";
static int last_exit_status = 0;


// FUNCTION PROTOTYPES
void route(argus arg);
void execute(argus arg);


// SIGNAL HANDLERS
void sigchld_handler(int signum) {
    int pid, status;

    while (true) {
        pid = waitpid (WAIT_ANY, &status, WNOHANG);
        if (pid < 0) {
            return;
        } else if (pid == 0) {
            return;
        }
    }
}


// HELPER FUNCTIONS
void print_all_args(argus arg) {
    for (int i = 0; i < arg.args; i++) {
        printf(".%s. ", arg.argv[i]);
    }
    printf("\n");
}

argus parse_single_arg(char *string) {
    argus new;
    new.args = 0;
    char *token;

    // Create token for strtok
    token = strtok(string, " \n\t\a\r");

    // Send tokens to struct
    while (token != NULL) {
        new.argv[new.args] = malloc((strlen(token) + 1) * sizeof(char));
        strcpy(new.argv[new.args], token);
        new.args++;

        token = strtok(NULL, " \n\t\a\r");
    }

    new.argv[new.args] = NULL;

    return new;
}

argus routes(argus arg) {
    char *input_path = NULL;
    char *output_path = NULL;

    for (int i = 0; i < arg.args; i++) {
        if (arg.argv[i] != NULL && strchr(arg.argv[i], '<') != NULL) {
            input_path = strchr(arg.argv[i], '<');
            input_path++;
            arg.argv[i] = NULL;
        }

        if (arg.argv[i] != NULL && strchr(arg.argv[i], '>') != NULL) {
            output_path = strchr(arg.argv[i], '>');
            output_path++;
            arg.argv[i] = NULL;
        }
    }

    if (arg.argv[arg.args - 1] != NULL && strcmp(arg.argv[arg.args - 1], "&") == 0) {
        arg.argv[arg.args - 1] = NULL;
        arg.args--;
        arg.bg = true;
    }

    if (input_path != NULL && ((arg.fd_i = open(input_path, O_RDONLY)) < 0)) {
        fflush(stdout);
        printf("%si: %s: %s\n", arg.argv[0], input_path, strerror(errno));
        last_exit_status = errno;
        arg.fd_i = STDIN_FILENO;
        return arg;
    }

    if (output_path != NULL && ((arg.fd_o = open(output_path, O_CREAT | O_WRONLY | O_TRUNC, ACCESSPERMS)) < 0)) {
        fflush(stdout);
        printf("%so: %s: %s\n", arg.argv[0], output_path, strerror(errno));
        last_exit_status = errno;
        arg.fd_o = STDOUT_FILENO;
        return arg;
    }

    return arg;
}

argus parse_line(char *line) {
    argus new_com;
    new_com.args = 0;
    new_com.fd_i = STDIN_FILENO;
    new_com.fd_o = STDOUT_FILENO;
    new_com.bg = false;

    char *tmp = malloc(45 * sizeof(char));
    int tmp_n = 0;

    while (line[0] == ' ') {
        line++;
    }

    if (line[0] == '#' || line[0] == '\n') {
        return new_com;
    }

    // Znak po znaku
    for (int i = 0; i < strlen(line); i++) {
        // Če naletim na presledek, moram narediti nov argument
        if (line[i] == ' ' || line[i] == '\n') {
            tmp[tmp_n] = '\0';
            tmp_n = 0;
            new_com.argv[new_com.args] = tmp;
            new_com.args++;
            tmp = malloc(45 * sizeof(char));

        } else if (line[i] == '\"') {
            // Zaključi prejšnji argument
            tmp[tmp_n] = '\0';
            tmp_n = 0;
            new_com.argv[new_com.args] = tmp;
            tmp = malloc(45 * sizeof(char));
            int j = i + 1;
            for (; j < strlen(line); j++) {
                if (line[j] == '\"') {
                    break;
                }
                tmp[tmp_n] = line[j];
                tmp_n++;
            }
            tmp[tmp_n] = '\0';
            tmp_n = 0;
            new_com.argv[new_com.args] = tmp;
            tmp = malloc(45 * sizeof(char));
            i = j + 1;
            new_com.args++;
        } else {
            // Prepisuj znake
            tmp[tmp_n] = line[i];
            tmp_n++;

        }
    }

    return new_com;
}

int get_inode(int fd) {
    struct stat buf;

    if (fstat(fd, &buf) < 0) {
        printf("linklist: %s\n", strerror(errno));
        return errno;
    }

    return (int)buf.st_ino;
}

int cat(argus arg, int A, int B) {
    // Buffer for read
    char *buf = calloc(8196, sizeof(char));
    ssize_t nread;

    while (nread = read(A, buf, sizeof(buf)), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(B, out_ptr, (size_t)nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                fflush(stdout);
                printf("%s: %s\n", arg.argv[0], strerror(errno));
                last_exit_status = errno;
                return 1;            }
        } while (nread > 0);
    }

    return 0;
}


// INNER FUNCTIONS
void name(argus arg) {
    if (arg.args == 1) {
        printf("%s\n", shell_name);
    } else {
        strcpy(shell_name, arg.argv[1]);
    }
}

void help() {
    printf("Preprosti vgrajeni ukazi\n");
    printf(BOLDGREEN" name "RESET ITALICGREEN"ime "RESET" - nastavi ime lupine, če imena ne podamo, izpiše ime lupine (privzeto ime je mysh),\n");
    printf(BOLDGREEN" help "RESET" - izpiše spisek podprtih ukazov, format izpisa je po vaši želji "RESET" - se ne preverja avtomatsko,\n");
    printf(BOLDGREEN" status "RESET" - izpiše izhodni status zadnjega (v ospredju) izvedenega ukaza,\n");
    printf(BOLDGREEN" exit "RESET ITALICGREEN"status "RESET" - konča lupino s podanim izhodnim statusom,\n");
    printf(BOLDGREEN" print "RESET ITALICGREEN"args... "RESET" - izpiše podane argumente na standardni izhod (brez končnega skoka v novo vrstico),\n");
    printf(BOLDGREEN" echo "RESET ITALICGREEN"args... "RESET" - kot print, le da izpiše še skok v novo vrstico,\n");
    printf(BOLDGREEN" pid "RESET" - izpiše pid procesa (kot $BASHPID),\n");
    printf(BOLDGREEN" ppid "RESET" - izpiše pid starša.\n\n");
    printf("Vgrajeni ukazi za delo z imeniki\n");
    printf(BOLDGREEN" dirchange "RESET ITALICGREEN"imenik "RESET" - zamenjava trenutnega delovnega imenika, če imenika ne podamo, skoči na /,\n");
    printf(BOLDGREEN" dirwhere "RESET" - izpis trenutnega delovnega imenika,\n");
    printf(BOLDGREEN" dirmake "RESET ITALICGREEN"imenik "RESET" - ustvarjanje podanega imenika,\n");
    printf(BOLDGREEN" dirremove "RESET ITALICGREEN"imenik "RESET" - brisanje podanega imenika,\n");
    printf(BOLDGREEN" dirlist "RESET ITALICGREEN"imenik "RESET" - preprost izpis vsebine imenika (le imena datotek, ločena z dvema presledkoma), če imena ne podamo, se privzame trenutni delovni imenik.\n\n");
    printf("Ostali vgrajeni ukazi za delo z datotekami\n");
    printf(BOLDGREEN "linkhard "RESET ITALICGREEN"cilj ime "RESET" - ustvarjanje trde povezave na cilj,\n");
    printf(BOLDGREEN "linksoft "RESET ITALICGREEN"cilj ime "RESET" - ustvarjanje simbolične povezave na cilj,\n");
    printf(BOLDGREEN "linkread "RESET ITALICGREEN"ime "RESET" - izpis cilja podane simbolične povezave,\n");
    printf(BOLDGREEN "linklist "RESET ITALICGREEN"ime "RESET" - izpiše vse trde povezave na datoteko z imenom ime,\n");
    printf(BOLDGREEN "unlink "RESET ITALICGREEN"ime "RESET" - brisanje datoteke,\n");
    printf(BOLDGREEN "rename "RESET ITALICGREEN"izvor ponor "RESET" - preimenovanje datoteke,\n");
    printf(BOLDGREEN "cpcat "RESET ITALICGREEN"izvor ponor "RESET" - ukaz cp in cat združena \n");
}

void mexit(argus arg) {
    char *ret;
    if (arg.args == 1) {
        exit(last_exit_status);
    } else {
        exit((int)strtol(arg.argv[1], &ret, 10));
    }
}

void print(argus arg) {
    for (int i = 1; i < arg.args; i++) {
        printf("%s", arg.argv[i]);
        if (i != arg.args - 1) {
            printf(" ");
        }
    }
}

void mecho(argus arg) {
    for (int i = 1; i < arg.args; i++) {
        printf("%s", arg.argv[i]);
        if (i != arg.args - 1) {
            printf(" ");
        }
    }
    printf("\n");
}

void dirchange(argus arg) {
    if (arg.args == 1) {
        chdir("/");
    } else {
        DIR *dir = opendir(arg.argv[1]);

        if (dir) {
            closedir(dir);
            chdir(arg.argv[1]);
        } else {
            fflush(stdout);
            printf("%s: %s\n", arg.argv[1], strerror(errno));
            last_exit_status = errno;
            return;
        }
    }
}

void dirwhere(argus arg) {
    char *path = malloc(1024*sizeof(char));
    printf("%s\n", getcwd(path, 1023));
}

void dirmake(argus arg) {
    if (mkdir(arg.argv[1], ACCESSPERMS) == -1) {
        last_exit_status = errno;
        printf("%s: %s\n", arg.argv[0], strerror(errno));
    }
}

void dirremove(argus arg) {
    if (rmdir(arg.argv[1]) == -1) {
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        last_exit_status = errno;
    }
}

void dirlist(argus arg) {
    int status = 0;
    struct dirent *dp;
    DIR *dir;
    char *path;
    char **list = malloc(260 * sizeof(char *));
    int len = 0;

    if (arg.args == 1) {
        path = ".";
    } else {
        path = arg.argv[1];
    }

    if (dir = opendir(path), dir == NULL) {
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        last_exit_status = errno;
        return;
    }

    // Berem vsebino mape
    while (dp = readdir(dir), dp != NULL) {
        list[len] = malloc(260 * sizeof(char));
        strcpy(list[len], dp->d_name);
        len++;
    }

    // Izpišem
    for (int i = 0; i < len; i++) {
        printf("%s", list[i]);
        if (i != len - 1) {
            printf("  ");
        } else {
            printf("\n");
        }
    }

    for (int i = 0; i < len; i++) {
        free(list[i]);
    }

    // Zaprem mapo
    if (status =  closedir(dir), status != 0) {
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        last_exit_status = errno;
    }
}

void linkhard(argus arg) {
    if (link(arg.argv[1], arg.argv[2]) != 0) {
        fflush(stdout);
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        last_exit_status = errno;
    }
}

void linksoft(argus arg) {
    if (symlink(arg.argv[1], arg.argv[2]) != 0) {
        fflush(stdout);
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        last_exit_status = errno;
    }
}

void linkread(argus arg) {
    char target_path[512];
    ssize_t size = readlink(arg.argv[1], target_path, sizeof(target_path));

    if (arg.args == 2) {
        if (size == -1) {
            fflush(stdout);
            printf("%s: %s\n", arg.argv[0], strerror(errno));
            if (errno == EINVAL) {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
            } else {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
            }
            last_exit_status = errno;
        } else {
            target_path[size] = '\0';
            printf("%s\n", target_path);
        }
    }
}

void linklist(argus arg) {
    int file;
    if ((file = open(arg.argv[1], O_RDONLY)) < 0) {
        last_exit_status = errno;
        printf("%s: %s\n", arg.argv[0], strerror(errno));
    }
    int inode = get_inode(file);
    struct dirent *dp;
    DIR *dir;
    char *path = malloc(1024 * sizeof(char));
    path = getcwd(path, 1023);
    bool not_first_time = false;

    if (dir = opendir(path), dir == NULL) {
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        last_exit_status = errno;
        return;
    }

    // Berem vsebino mape
    while (dp = readdir(dir), dp != NULL) {
        if (inode == dp->d_ino) {
            if (not_first_time) {
                printf("  %s", dp->d_name);
            } else {
                printf("%s", dp->d_name);
                not_first_time = true;
            }
        }
    }
    printf("\n");

    if (closedir(dir) != 0) {
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        last_exit_status = errno;
    }
}

void munlink(argus arg) {
    if (unlink(arg.argv[1]) != 0) {
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        last_exit_status = errno;
    }
}

void mrename(argus arg) {
    if (rename(arg.argv[1], arg.argv[2]) != 0) {
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        last_exit_status = errno;
    }
}

void cpcat(argus arg) {
    int file_i, file_o;

    if (arg.args == 1) {
        if (cat(arg, 0, 1) != 0) {
            last_exit_status = errno;
        }

    } else if (arg.args == 2) {
        file_i = open(arg.argv[1], O_RDONLY);
        if (file_i < 0) {
            printf("%s: %s\n", arg.argv[0], strerror(errno));
            last_exit_status = errno;
            return;        }

        if (cat(arg, file_i, 1) != 0) {
            last_exit_status = errno;
            return;        }

        if (close(file_i) == -1) {
            printf("%s: %s\n", arg.argv[0], strerror(errno));
            last_exit_status = errno;
            return;
        }

    } else if (arg.args == 3) {
        file_i = open(arg.argv[1], O_RDONLY);
        file_o = open(arg.argv[2], O_RDWR | O_TRUNC | O_CREAT, ACCESSPERMS);

        if ( file_i < 0 && file_o < 0 ) {
            // STDIN --> STDIO
            // Throw an error, because the will be no input file
            printf("%s: %s\n", arg.argv[0], strerror(errno));
            last_exit_status = errno;
            return;
        } else if ( file_i > 0 && file_o < 0 ) {
            // FILE --> STDOUT
            if (flock(file_i, LOCK_SH) == -1) {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
                last_exit_status = errno;
                return;            }

            if (cat(arg, file_i, 1) != 0) {
                last_exit_status = errno;
                return;            }

            if (close(file_i) == -1) {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
                last_exit_status = errno;
                return;            }

        } else if ( file_i < 0 && file_o > 0 ) {
            // STDIN --> FILE
            if (arg.argv[1][0] != '-') {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
                last_exit_status = errno;
                return;            }

            if (flock(file_o, LOCK_EX) == -1) {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
                last_exit_status = errno;
                return;            }

            if (cat(arg, 0, file_o) != 0) {
                last_exit_status = errno;
                return;            }

            if (close(file_o) == -1) {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
                last_exit_status = errno;
                return;            }
        } else if ( file_i > 0 && file_o > 0 ) {
            // FILE --> FILE
            if (flock(file_i, LOCK_SH) == -1 || flock(file_o, LOCK_EX) == -1) {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
                last_exit_status = errno;
                return;            }

            if (cat(arg, file_i, file_o) != 0) {
                last_exit_status = errno;
                return;            }

            if (flock(file_i, LOCK_UN) == -1 || flock(file_o, LOCK_UN) == -1) {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
                last_exit_status = errno;
                return;            }

            if (close(file_i) == -1 || close(file_o) == -1) {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
                last_exit_status = errno;
                return;            }
        }
    }
}

void pipes(argus arg) {
    argus arg1 = parse_single_arg(arg.argv[1]);
    argus arg2 = parse_single_arg(arg.argv[2]);
    print_all_args(arg1);
    print_all_args(arg2);
    int fd[2];
    int status1, status2;
    pid_t cp1, cp2;

    // Ustvari cev
    if (pipe(fd) < 0) {
        printf("%s: %s\n", "pipe", strerror(errno));
        last_exit_status = errno;
    }

    // Prvi fork
    if ((cp1 = fork()) < 0) {
        printf("%s: %s\n", "pipe", strerror(errno));
        last_exit_status = errno;
    } else if (cp1 == 0) {
        dup2(fd[1], 1);
        if (close(fd[0]) < 0) {
            printf("%s: %s\n", "pipe", strerror(errno));
            last_exit_status = errno;
        }
        if (close(fd[1]) < 0) {
            printf("%s: %s\n", "pipe", strerror(errno));
            last_exit_status = errno;
        }
        arg1.bg = true;
        execvp(arg1.argv[0], arg1.argv);
        exit(0);
    }

    // Drugi fork
    if ((cp2 = fork()) < 0) {
        printf("%s: %s\n", "pipe", strerror(errno));
        last_exit_status = errno;
    } else if (cp2 == 0) {
        dup2(fd[0], 0);
        if (close(fd[0]) < 0) {
            printf("%s: %s\n", "pipe", strerror(errno));
            last_exit_status = errno;
        }
        if (close(fd[1]) < 0) {
            printf("%s: %s\n", "pipe", strerror(errno));
            last_exit_status = errno;
        }
        arg2.bg = true;
        execvp(arg2.argv[0], arg2.argv);
        exit(0);
    }

    // Zapri deskriptorja v staršu
    if (close(fd[0]) < 0) {
        printf("%s: %s\n", "pipe", strerror(errno));
        last_exit_status = errno;
    }
    if (close(fd[1]) < 0) {
        printf("%s: %s\n", "pipe", strerror(errno));
        last_exit_status = errno;
    }

    // Počakaj otroke
    waitpid(cp1, &status1, 0);
    waitpid(cp2, &status2, 0);
}

void external(argus arg) {
    if (arg.bg) {
        fflush(stdout);
        if (!execvp(arg.argv[0], arg.argv)) {
            printf("%s: %s\n", arg.argv[0], strerror(errno));
            last_exit_status = errno;
            exit(0);
        }
    } else {
        // Zunanji ukaz
        int status = 0;
        pid_t pid = fork();
        if (pid == 0) {
            arg.argv[arg.args] = NULL;
            fflush(stdout);
            if (!execvp(arg.argv[0], arg.argv)) {
                printf("%s: %s\n", arg.argv[0], strerror(errno));
                last_exit_status = errno;
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            printf("%s: %s\n", arg.argv[0], strerror(errno));
            last_exit_status = errno;
            return;
        } else {
            if (!arg.bg) {
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    last_exit_status = WEXITSTATUS(status);
                }
            }
        }
    }
}


// MAIN FUNCTIONS
void execute(argus arg) {
    if (strcmp(arg.argv[0], "name") == 0)             name(arg);
    else if (strcmp(arg.argv[0], "help") == 0)        help();
    else if (strcmp(arg.argv[0], "status") == 0)      printf("%d\n", last_exit_status);
    else if (strcmp(arg.argv[0], "exit") == 0)        mexit(arg);
    else if (strcmp(arg.argv[0], "print") == 0)       print(arg);
    else if (strcmp(arg.argv[0], "echo") == 0)        mecho(arg);
    else if (strcmp(arg.argv[0], "pid") == 0)         printf("%d\n", getpid());
    else if (strcmp(arg.argv[0], "ppid") == 0)        printf("%d\n", getppid());
    else if (strcmp(arg.argv[0], "dirchange") == 0)   dirchange(arg);
    else if (strcmp(arg.argv[0], "dirwhere") == 0)    dirwhere(arg);
    else if (strcmp(arg.argv[0], "dirmake") == 0)     dirmake(arg);
    else if (strcmp(arg.argv[0], "dirremove") == 0)   dirremove(arg);
    else if (strcmp(arg.argv[0], "dirlist") == 0)     dirlist(arg);
    else if (strcmp(arg.argv[0], "linkhard") == 0)    linkhard(arg);
    else if (strcmp(arg.argv[0], "linksoft") == 0)    linksoft(arg);
    else if (strcmp(arg.argv[0], "linkread") == 0)    linkread(arg);
    else if (strcmp(arg.argv[0], "linklist") == 0)    linklist(arg);
    else if (strcmp(arg.argv[0], "unlink") == 0)      munlink(arg);
    else if (strcmp(arg.argv[0], "rename") == 0)      mrename(arg);
    else if (strcmp(arg.argv[0], "cpcat") == 0)       cpcat(arg);
    else if (strcmp(arg.argv[0], "pipes") == 0)       pipes(arg);
    else external(arg);
}

void route(argus arg) {
    int bq_in;
    int bq_out;

    arg = routes(arg);

    bq_in = dup(fileno(stdin));
    if (arg.fd_i != STDIN_FILENO) {
        arg.args--;
        fflush(stdin);
        dup2(arg.fd_i, STDIN_FILENO);
    }

    bq_out = dup(fileno(stdout));
    if (arg.fd_o != STDOUT_FILENO) {
        arg.args--;
        fflush(stdout);
        dup2(arg.fd_o, STDOUT_FILENO);
    }

    // Background process has to be in a separate proces
    // External process is already executed in separate process,
    // so we dont need doing it twice
    if (arg.bg) {
        pid_t pid = fork();
        if (pid == 0) {
            fflush(stdout);
            execute(arg);
            exit(0);
        } else if (pid < 0) {
            printf("%s: %s\n", arg.argv[0], strerror(errno));
            last_exit_status = errno;
            return;
        }
    } else {
        execute(arg);
    }

    fflush(stdout);

    // Restore
    dup2(bq_in, STDIN_FILENO);
    dup2(bq_out, STDOUT_FILENO);

    if (arg.fd_i != 0 && close(arg.fd_i) < 0) {
        fflush(stdout);
        last_exit_status = errno;
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        return;
    }

    if (arg.fd_o != 1 && close(arg.fd_o) < 0) {
        fflush(stdout);
        last_exit_status = errno;
        printf("%s: %s\n", arg.argv[0], strerror(errno));
        return;
    }

    fflush(stdin);
    fflush(stdout);
}

void loop() {
    argus arg;
    char *out;

    // Shell loop
    do {
        // Check if stdin is terminal
        if (isatty(0) != 0) {
            printf("%s> ", shell_name);
            fflush(stdout);
        }

        char *line = malloc(512 * sizeof(char));
        out = fgets(line, 512, stdin);

        // Parse line arguments
        arg = parse_line(line);
        // Execute line arguments
        if (arg.args != 0) {
            route(arg);
        }

        // Free memory of arguments
        for (int i = 0; i < arg.args; i++) {
            free(arg.argv[i]);
        }

        // Re-loop
    } while (out);

}

int main(int args, char *argv[]) {
    // Create SIGCHLD handler
    signal(SIGCHLD, sigchld_handler);

    loop();

    return 0;
}
