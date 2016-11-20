#include <stdio.h>  
#include <assert.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/inotify.h>  
#include <limits.h>  
#include <fcntl.h>  
  
#define BUF_LEN 1024  

static char *path;
static char  target[BUF_LEN];

int
handle_system_ret_val (int status)
{
    if(status < 0) {
        return -1;
    }
    if(WIFEXITED(status)) {
        //printf("normal termination, exit status = %d\n", WEXITSTATUS(status)); //取得cmdstring执行结果
        return 0;
    }
    else if(WIFSIGNALED(status)) {
        printf("abnormal termination,signal number =%d\n", WTERMSIG(status)); //如果cmdstring被信号中断，取得信号值
        return -1;
    }
    else if(WIFSTOPPED(status)) {
        printf("process stopped, signal number =%d\n", WSTOPSIG(status)); //如果cmdstring被信号暂停执行，取得信号值
        return -1;
    }
    return -1;
}

void
exec_cmd(char* cmd)
{
    int status = system(cmd);
    if (handle_system_ret_val(status) == 0) {
        printf("%s recompiled successful!\n", target);
    }
}

char*
concat_cmd(char* source, char* suffix_offset)
{
    char  cmd[10240]; 

    memset(target, '\0', 1024);
    strncpy(target, source, suffix_offset - source);
    strcat(target, ".css");
    sprintf(cmd, "scss %s %s", source, target);
    strdup(cmd);
}

void 
displayInotifyEvent(struct inotify_event *i)  
{  
    char  *suffix_offset;

    if(i->mask & IN_MODIFY) {
        if (i->len > 0) {
            char  source[1024];
            sprintf(source, "%s%s", path, i->name);
            if ((suffix_offset = strrchr(source, '.')) != NULL) {
                if (!strcmp(suffix_offset, ".scss")) {
                    printf("%s willed modify, will recompile!\n", source);
                    char* cmd = concat_cmd(source, suffix_offset);
                    exec_cmd(cmd);
                    free(cmd);
                }
            }
        } else {
            if ((suffix_offset = strrchr(path, '.')) != NULL) {
                if (!strcmp(suffix_offset, ".scss")) {
                    printf("%s willed modify, will recompile!\n", path);
                    char* cmd = concat_cmd(path, suffix_offset);
                    exec_cmd(cmd);
                    free(cmd);
                }
            } 
        }
    }
}  
  
int 
main(int argc,char **argv)  
{  
    char                  *p;  
    struct inotify_event  *event;  
    ssize_t                numRead;  
    char                   buf[BUF_LEN];  
    int                    inot_fd, watch_fd;  
  
    if(argc < 2 ) {  
        printf("请给定两个参数\n");  
        exit(-1);
    }  
  
    inot_fd = inotify_init();  
    if(inot_fd == -1) {  
        printf("初始化失败");  
    }  
  
    watch_fd = inotify_add_watch(inot_fd, argv[1], IN_MODIFY | IN_MOVED_TO);  
    if(watch_fd == -1) {  
        printf("没有这个文件或者目录\n");  
    }  
    path = argv[1];
    while(1) {  
        numRead = read(inot_fd, buf, BUF_LEN);  
        if(numRead == -1) {  
            printf("read error\n");  
        }  
  
        for(p = buf; p < buf + numRead;)  {  
            event = (struct inotify_event *)p;  
            displayInotifyEvent(event);  
            p += sizeof(struct inotify_event) + event->len;  
        }  
    }  

    if (inotify_rm_watch(inot_fd, watch_fd) < 0) {  
        printf("inotify_rm_watch error %d\n", errno);  
    }  
  
    close(inot_fd);  
    return 0;  
  
}  
