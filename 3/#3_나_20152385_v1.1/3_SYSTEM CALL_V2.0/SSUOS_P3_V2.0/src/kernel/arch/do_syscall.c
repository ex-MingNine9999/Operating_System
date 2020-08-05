#include <proc/sched.h>
#include <proc/proc.h>
#include <device/device.h>
#include <interrupt.h>
#include <device/kbd.h>
#include <filesys/file.h>

pid_t do_fork(proc_func func, void* aux1, void* aux2)
{
	pid_t pid;
	struct proc_option opt;

	opt.priority = cur_process-> priority;
	pid = proc_create(func, &opt, aux1, aux2);

	return pid;
}

void do_exit(int status)
{
	cur_process->exit_status = status; 	//종료 상태 저장
	proc_free();						//프로세스 자원 해제
	do_sched_on_return();				//인터럽트 종료시 스케줄링
}

pid_t do_wait(int *status)
{
	while(cur_process->child_pid != -1)
		schedule();
	//SSUMUST : schedule 제거.

	int pid = cur_process->child_pid;
	cur_process->child_pid = -1;

	extern struct process procs[];
	procs[pid].state = PROC_UNUSED;

	if(!status)
		*status = procs[pid].exit_status;

	return pid;
}

void do_shutdown(void)
{
	dev_shutdown();
	return;
}

int do_ssuread(void)
{
	return kbd_read_char();
}

int do_open(const char *pathname, int flags)
{
	struct inode *inode;
	struct ssufile **file_cursor = cur_process->file;
	int fd;

	for(fd = 0; fd < NR_FILEDES; fd++)
		if(file_cursor[fd] == NULL) break;

	if (fd == NR_FILEDES)
		return -1;

	if ( (inode = inode_open(pathname, flags)) == NULL)
		return -1;

	if (inode->sn_type == SSU_TYPE_DIR)
		return -1;

	fd = file_open(inode,flags,0);

	return fd;
}

int do_read(int fd, char *buf, int len)
{
	return generic_read(fd, (void *)buf, len);
}
int do_write(int fd, const char *buf, int len)
{
	return generic_write(fd, (void *)buf, len);
}

int do_fcntl(int fd, int cmd, long arg)
{
	int flag = -1;
	struct ssufile **file_cursor = cur_process->file;
                                    // 현재 프로세서에 열려있는 파일 struct포인터배열를 받아옴
	if (cmd & F_DUPFD){             // F_DUPFD 명령
		int dupfd;
		for(dupfd = arg; dupfd < NR_FILEDES; dupfd++)
		{
			if(file_cursor[dupfd] == NULL)  // 비어있는 fd를 찾아 dupfd에 저장
			{
				break;
			}	
		}

		if (dupfd >= NR_FILEDES) {  // 제한된 숫자보다 크면 -1리턴
			return -1;
		}

		file_cursor[dupfd] = file_cursor[fd];   // fd번째 file_cursor가 가르키는 파일을
                                                // dupfd번째 file_cursor도 가르키도록 대입
		return dupfd;                           // dupfd 리턴
	}
	else if (cmd & F_GETFL){                    // F_GETFL 명령
		return (int)file_cursor[fd]->flags;	    // fd번째 file_cursor의 flags리턴
	}
	else if(cmd & F_SETFL){                     // F_SETFL 명령
		if ((file_cursor[fd]->flags ^ (uint8_t)arg) != O_APPEND) {  // 현재 플래그와 arg를 xor
			return -1;               // arg에 O_APPEND와 현재플래그를 제외한 다른arg가 들어오면 -1리턴
		}

		file_cursor[fd]->flags |= (uint8_t)arg;     // 파일 플래그에 arg추가
		if (arg & O_APPEND) {                       // O_APPEND 플래그가 있으면
			file_cursor[fd]->pos = file_cursor[fd]->inode->sn_size; //pos(오프셋)을 파일사이즈로 변경
		}
		return (int)file_cursor[fd]->flags;
	}
	else{
		return -1;
	}
}
