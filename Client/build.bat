windres icon.rc -O coff -o icon.res
gcc -o VitaPad main.c icon.res -lws2_32