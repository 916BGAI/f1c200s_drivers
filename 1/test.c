#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd;
    int val;
    char *filename = NULL;
    unsigned char data_buf[1];

    // 检查参数
    if (argc != 3) {
        printf("usage: ./test [device] [on/off]\n");
        close(fd);
        return -1;
    }

    // 打开设备文件
    filename = argv[1];
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        printf("open %s error!\n", filename);
        return -1;
    }

    if (!strcmp(argv[2], "on"))
	val = 0;
    else if (!strcmp(argv[2], "off"))
	val = 1;
    else {
	close(fd);
	return -1;
    }
    write(fd, &val, 4);
    close(fd);

    return 0;
}