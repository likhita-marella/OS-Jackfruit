#define _GNU_SOURCE
#include <sched.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define STACK_SIZE (1024 * 1024)
#define DATA_FILE "containers.txt"

/* ================= STRUCT ================= */
typedef struct {
    char id[32];
    char rootfs[256];
} child_config_t;

/* ================= CHILD ================= */
int child_fn(void *arg)
{
    child_config_t *config = (child_config_t *)arg;

    sethostname(config->id, strlen(config->id));

    if (chroot(config->rootfs) != 0) {
        perror("chroot");
        return 1;
    }

    chdir("/");

    mount("proc", "/proc", "proc", 0, NULL);

    /* ===== LOGGING (TEE: terminal + file) ===== */

    mkdir("/logs", 0755);

    char log_path[256];
    snprintf(log_path, sizeof(log_path), "/logs/%s.log", config->id);

    int fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    int pipefd[2];
    pipe(pipefd);

    pid_t log_pid = fork();

    if (log_pid == 0) {
        // logger process
        close(pipefd[1]);

        char buffer[1024];
        int n;

        while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            write(fd, buffer, n);              // write to file
            write(STDOUT_FILENO, buffer, n);   // also print
        }

        close(fd);
        exit(0);
    } else {
        // container process
        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
    }

    execl("/bin/sh", "/bin/sh", NULL);

    perror("exec failed");
    return 1;
}

/* ================= ADD ================= */
void add_container(char *id, pid_t pid)
{
    FILE *f = fopen(DATA_FILE, "a");
    if (!f) return;

    fprintf(f, "%s %d\n", id, pid);
    fclose(f);
}

/* ================= PS ================= */
void list_containers()
{
    FILE *f = fopen(DATA_FILE, "r");
    if (!f) {
        printf("No containers\n");
        return;
    }

    char id[32];
    int pid;

    printf("ID\tPID\n");

    while (fscanf(f, "%s %d", id, &pid) != EOF) {
        printf("%s\t%d\n", id, pid);
    }

    fclose(f);
}

/* ================= STOP ================= */
void stop_container(char *id)
{
    FILE *f = fopen(DATA_FILE, "r");
    FILE *tmp = fopen("temp.txt", "w");

    char cid[32];
    int pid;

    while (fscanf(f, "%s %d", cid, &pid) != EOF) {
        if (strcmp(cid, id) == 0) {
            kill(pid, SIGKILL);
            printf("Stopped %s\n", id);
        } else {
            fprintf(tmp, "%s %d\n", cid, pid);
        }
    }

    fclose(f);
    fclose(tmp);

    remove(DATA_FILE);
    rename("temp.txt", DATA_FILE);
}

/* ================= MAIN ================= */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage:\n");
        printf("./engine run <id> <rootfs>\n");
        printf("./engine start <id> <rootfs>\n");
        printf("./engine ps\n");
        printf("./engine stop <id>\n");
        return 1;
    }

    /* ===== RUN / START ===== */
    if (strcmp(argv[1], "run") == 0 || strcmp(argv[1], "start") == 0) {

        if (argc < 4) {
            printf("Usage: run/start <id> <rootfs>\n");
            return 1;
        }

        child_config_t config;
        memset(&config, 0, sizeof(config));

        strcpy(config.id, argv[2]);
        strcpy(config.rootfs, argv[3]);

        char *stack = malloc(STACK_SIZE);

        pid_t pid = clone(child_fn,
                          stack + STACK_SIZE,
                          CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD,
                          &config);

        if (pid < 0) {
            perror("clone");
            return 1;
        }

        if (strcmp(argv[1], "start") == 0) {
            add_container(argv[2], pid);
            printf("Started %s (PID %d)\n", argv[2], pid);
            return 0;
        }

        // interactive
        waitpid(pid, NULL, 0);
        return 0;
    }

    /* ===== PS ===== */
    else if (strcmp(argv[1], "ps") == 0) {
        list_containers();
    }

    /* ===== STOP ===== */
    else if (strcmp(argv[1], "stop") == 0) {
        stop_container(argv[2]);
    }

    else {
        printf("Unknown command\n");
    }

    return 0;
}
