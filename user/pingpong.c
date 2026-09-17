#include "kernel/types.h"
#include "user/user.h"

int main(){
    int ping[2];
    int pong[2];
    pipe(ping);
    pipe(pong);

    char buf[512];

    int father_pid = getpid();
    int pid = fork();

    if (pid == 0){
        close(ping[1]);
        close(pong[0]);

        if(read(ping[0], buf, sizeof(buf))>0){
            printf("%d: received ping from pid %d\n",getpid(),father_pid);
        }
        close(ping[0]);

        write(pong[1], buf, sizeof(buf));
        close(pong[1]);

        exit(1);
    } else {
        close(pong[1]);
        close(ping[0]);

        write(ping[1], buf, sizeof(buf));
        close(ping[1]);

        if(read(pong[0], buf, sizeof(buf))>0){
            printf("%d: received pong from pid %d\n",getpid(),pid);
        }
        close(pong[0]);

        exit(1);
    }

    exit(1);
}