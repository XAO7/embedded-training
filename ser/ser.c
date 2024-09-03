/*===============================================
*   文件名称：ser.c
*   创 建 者：     
*   创建日期：2024年05月25日
*   描    述：
================================================*/
#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>

struct buffer {
    void *start;
    size_t length;
};

int camera_init();
int camera_dqbuf(char **, unsigned int *, unsigned int *, struct v4l2_buffer *);
int camera_qbuf(struct v4l2_buffer *);

char *path = "/dev/video0";

int camera_fd;

struct v4l2_buffer vbuf;

#define BUF_COUNT 10
struct buffer bufs[BUF_COUNT];

char *buf;
unsigned int size;
unsigned int index_;


int main(int argc, char *argv[])
{
	/*创建socket*/
	int fd =socket(PF_INET,  SOCK_STREAM,  0);
	
	/*sockfd为需要端口复用的套接字*/
	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
	
	/*绑定ip和端口*/
	struct sockaddr_in myaddr;
	memset(&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = PF_INET;	//使用IP协议
	myaddr.sin_port = htons(54321);  //端口号
	myaddr.sin_addr.s_addr = inet_addr("10.10.10.128");  //IP地址
	bind(fd, (struct sockaddr*)(&myaddr), sizeof(myaddr));

	/*监听*/
	listen(fd,10);
	/*等待连接*/
	while (1)
	{
		struct sockaddr_in caddr;
		int caddr_len = 0;
		printf("accept begin...\n");
		int accept_fd = accept(fd, (struct sockaddr *) &caddr,&caddr_len);
		printf("accept ok...\n");

		int camera_on = 1;
		int a = 0;

		while (1)
		{
			if (camera_on)
			{
				if (a == 0)
				{
					if (camera_init() == -1)
						return -1;
					a = 1;
				}

				
				memset(&vbuf, 0, sizeof(struct v4l2_buffer));

				camera_dqbuf(&buf, &size, &index_, &vbuf);
				
				printf("index: %d ", index_);

				write(accept_fd, &size, 4);
				printf("write file size: %d\n", size);

				camera_qbuf(&vbuf);

				int w_len = 0;
				while(w_len < size)
				{
					int l = write(accept_fd, buf + w_len, size - w_len);
					w_len += l;
				}

				usleep(1000 * 300);
			}
			else
			{

			}

		}

		close(accept_fd);
	}


    return 0;
} 

int camera_init()
{
	printf("init start\n");

	camera_fd = open(path, O_RDWR);


	if (camera_fd == -1)
	{
		// If the file could not be opened, print an error message
		printf("Error opening file %s: %s\n", path, strerror(errno));
		return 1;
	}

	struct v4l2_format format;
	/*设置捕获的视频格式 MJPEG*/
	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	format.fmt.pix.width = 640;
	format.fmt.pix.height = 480;
	format.fmt.pix.field = V4L2_FIELD_ANY;
	if(ioctl(camera_fd, VIDIOC_S_FMT, &format) == -1)
	{
		perror("error: set format");
		close(camera_fd);
	}
	else {
	printf("camera->init: picture format is mjpeg\n");
	}



	struct v4l2_requestbuffers reqbufs;
	/*向驱动申请缓存*/
	memset(&reqbufs, 0, sizeof(struct v4l2_requestbuffers));
	reqbufs.count = BUF_COUNT; //缓存区个数
	reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbufs.memory = V4L2_MEMORY_MMAP;
	//设置操作申请缓存的方式:映射 MMAP
	if (ioctl(camera_fd, VIDIOC_REQBUFS, &reqbufs) == -1)
	{
		perror("error: request buffer");
		close(camera_fd);
		return -1;
	}



	for (int i = 0; i < reqbufs.count; i++)
	{
		/*获取真正缓存的信息*/
		memset(&vbuf, 0, sizeof(struct v4l2_buffer));
		vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		vbuf.memory = V4L2_MEMORY_MMAP;
		vbuf.index = i;
		if (ioctl(camera_fd, VIDIOC_QUERYBUF, &vbuf) == -1)
		{
			perror("error: query buffer");
			close(camera_fd);
			return -1;
		}

		/*映射缓存到用户空间,通过mmap讲内核的缓存地址映射到用户空间,并切和文件
		描述符fd相关联*/

		bufs[i].length = vbuf.length;
		bufs[i].start = mmap(NULL, vbuf.length, PROT_READ | PROT_WRITE,
		MAP_SHARED, camera_fd, vbuf.m.offset);
		if (bufs[i].start == MAP_FAILED)
		{
			perror("error: mmap fail");
			close(camera_fd);
			return -1;
		}
		/*每次映射都会入队,放入缓冲队列*/
		vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		vbuf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(camera_fd, VIDIOC_QBUF, &vbuf) == -1)
		{
			perror("error: queue");
			close(camera_fd);
			return -1;
		}
	}

	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	/*ioctl控制摄像头开始采集*/
	if (ioctl(camera_fd, VIDIOC_STREAMON, &type) == -1)
	{
		perror("error: stream on");
		close(camera_fd);
		return -1;
	}

	printf("init finished\n");
	return 0;
}

int camera_qbuf(struct v4l2_buffer *vbuf_ptr)
{
	if (ioctl(camera_fd, VIDIOC_QBUF, vbuf_ptr) == -1)
	{
		perror("error: queue");
		close(camera_fd);
		return -1;
	}
}

int camera_dqbuf(char **buf, unsigned int *size, unsigned int *index, struct v4l2_buffer *vbuf_ptr)
{
	int ret;
	fd_set fds;
	struct timeval timeout;

	while (1) {
		FD_ZERO(&fds);
		FD_SET(camera_fd, &fds);
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		ret = select(camera_fd + 1, &fds, NULL, NULL, &timeout);	//使用select机制,保证fd有采集到图像的时候才能出对
		if (ret == -1) {
			perror("camera->dbytesusedqbuf");
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else if (ret == 0) {
			fprintf(stderr, "camera->dqbuf: dequeue buffer timeout\n");
			return -1;
		} else {
			vbuf_ptr->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			vbuf_ptr->memory = V4L2_MEMORY_MMAP;
			ret = ioctl(camera_fd, VIDIOC_DQBUF, vbuf_ptr);	//出队
			if (ret == -1) {
				perror("camera->dqbuf");
				return -1;
			}
			*buf = bufs[vbuf_ptr->index].start;
			*size = vbuf_ptr->bytesused;
			*index = vbuf_ptr->index;

			return 0;
		}
	}
}