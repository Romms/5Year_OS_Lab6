#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <csignal>

typedef int (*func_ptr_t)( int );
const int MSG_TYPE = 1;
func_ptr_t funcPtr[2];


int f(int x) {
    sleep(200);
    return x*x + 3;
}

int g(int x) {
    sleep(3);
    return x + x;
}



typedef struct messageBuffer {
    long messageType;
    int number;
} MsgBfr;


int main() {

    int x;
    printf("Enter x: ");
    scanf("%d", &x);

    MsgBfr buffer;
    key_t msgKey = ftok(".", 'm');
    int msgQueueId = msgget(msgKey, IPC_CREAT | 0660);
    int msgLength = sizeof(MsgBfr) - sizeof(long);

    printf("Main pid: %d\n", getpid());

    pid_t pids[2] = {};
    funcPtr[0] = f;
    funcPtr[1] = g;

    for(int i=0; i<2; i++) {
        pid_t frk = fork();
        if (0 == frk) {
            buffer.messageType = MSG_TYPE;
            printf("Child stared, pid: %d\n", getpid());

            buffer.number = funcPtr[i](x);
            printf("Child process %d return result: %d\n", i, buffer.number);

            msgsnd(msgQueueId, &buffer, msgLength, 0);

            kill(getpid(), SIGTERM);
        } else {
            pids[i] = frk;
        }
    }

    int processQueue = 2, result = 0;
    while (processQueue) {
        sleep(4);
        ssize_t bytesRecieved = msgrcv(msgQueueId, &buffer, msgLength, MSG_TYPE, IPC_NOWAIT);
        if (bytesRecieved > 0) {
            printf("Main receive %d\n", buffer.number);
            result = result || buffer.number;
            if(result == true) {
                for (int i=0; i<2; i++) {
                    kill(pids[i], SIGKILL);
                    printf("%d - Killed!\n", (int) pids[i]);
                }
                break;
            }
            processQueue--;
        }
        else {

            for (int i=0; i<2; i++) {
                kill(pids[i], SIGSTOP);
                printf("%d - Stopped!\n", (int) pids[i]);
            }

            char command;
            printf("Response not received.\nContinue? (y/n) ");
            scanf("%c", &command);

            if (command == 'n') {
                for (int i=0; i<2; i++) {
                    kill(pids[i], SIGKILL);
                    printf("%d - Killed!\n", (int) pids[i]);
                }
                exit(1);
            } else {
                for (int i=0; i<2; i++) {
                    kill(pids[i], SIGCONT);
                    printf("%d - Continued!\n", (int) pids[i]);
                }
            }

        }
    }

    printf("f(%d) || g(%d) = %s", x,x, result ? "true" : "false");

    msgctl(msgQueueId, IPC_RMID, 0);

    return 0;
}
