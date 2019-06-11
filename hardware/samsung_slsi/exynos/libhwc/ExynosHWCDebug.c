#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int option = 0;
    size_t i;
    int ignoreRest = 0;
    char buf[128];

    if (argc != 2) {
        printf("Usage: debug h|d|e|v|u|a|x\n");
        printf("h = hwc\n");
        printf("d = display\n");
        printf("e = hdmi (external)\n");
        printf("v = virtual\n");
        printf("u = hwcutils\n");
        printf("a = enable all\n");
        printf("x = disable all\n");
        return 0;
    }

    for (i = 0; i < strlen(argv[1]); i++) {
        switch (argv[1][i]) {
            case 'h':
                option |= (1 << 0);
                break;
            case 'd':
                option |= (1 << 1);
                break;
            case 'e':
                option |= (1 << 2);
                break;
            case 'v':
                option |= (1 << 3);
                break;
            case 'u':
                option |= (1 << 4);
                break;
            case 'a':
                option = 31;
                ignoreRest = 1;
                break;
            case 'x':
                option = 0;
                ignoreRest = 1;
                break;
        }
        if (ignoreRest)
            break;
    }


    sprintf(buf, "service call Exynos.HWCService 105 i32 %d", option);
    system(buf);

    return 0;
}
