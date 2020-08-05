#include <filesys/inode.h>
#include <proc/proc.h>
#include <device/console.h>
#include <mem/palloc.h>


int file_open(struct inode *inode, int flags, int mode)
{
	int fd;
	struct ssufile **file_cursor = cur_process->file;           // 현재 프로세스에 열려있는 파일struct를 받아옴

	for(fd = 0; fd < NR_FILEDES; fd++)
	{
		if(file_cursor[fd] == NULL)                             // 0~NR_FILEDES사이에 빈 fd를 찾아
		{
			if( (file_cursor[fd] = (struct ssufile *)palloc_get_page()) == NULL)    // 메모리를 할당
				return -1;
			break;
		}	
	}
	
	inode->sn_refcount++;                                       // 파일이 열린 횟 수 증가

	file_cursor[fd]->inode = inode;                             // 파일 struct에 inode할당
	file_cursor[fd]->pos = 0;                                   // pos(오프셋)를 0으로 지정
    
	if(flags & O_APPEND){                                       // O_APPEND플래그가 존재하면
		file_cursor[fd]->pos = inode->sn_size;		            // pos를 파일 사이즈로 지정 (파일의 맨 끝)
	}
	else if(flags & O_TRUNC){                                   // O_TRUNC플래그가 존재하면
		char buf = 0;
		for (int i = 0; i < inode->sn_size; i++) {              // 파일의 사이즈만큼
			file_write(inode, i, (void*)&buf, 1);               // 오프셋마다 0(NULL)로 새로 씀
		}
		
		inode->sn_size = 0;                                     // 파일의 크기를 0으로 지정
	}

	file_cursor[fd]->flags = flags;                             // 파일의 플래그를 저장
	file_cursor[fd]->unused = 0;                                // 사용중임을 표시

	return fd;                                                  // fd를 리턴
}

int file_write(struct inode *inode, size_t offset, void *buf, size_t len)
{
	return inode_write(inode, offset, buf, len);
}

int file_read(struct inode *inode, size_t offset, void *buf, size_t len)
{
	return inode_read(inode, offset, buf, len);
}
