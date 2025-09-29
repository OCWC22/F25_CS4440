#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

static void write_repeated(int out, char ch, long n){
    while(n-->0) write(out,&ch,1);
}

static void decompress_fd(int in, int out){
    unsigned char c;
    while(read(in,&c,1)==1){
        if(c=='+' || c=='-'){
            char sign = c; long num = 0; unsigned char d;
            while(read(in,&d,1)==1 && d>='0' && d<='9') num = num*10 + (d-'0');
            if(d==sign && num>0){ write_repeated(out, sign=='+'?'1':'0', num); }
            else{
                write(out,&sign,1);
                if(num){ char buf[32]; int m=snprintf(buf,sizeof(buf), "%ld", num); write(out,buf,m); }
                if(d) write(out,&d,1);
            }
        }else{
            write(out,&c,1);
        }
    }
}

int main(int argc, char *argv[]){
    if(argc!=3){ dprintf(2,"usage: %s compressed.txt out.txt\n",argv[0]); return 1; }
    int fdin = open(argv[1], O_RDONLY);
    if(fdin<0){ perror("open in"); return 1; }
    int fdout = open(argv[2], O_RDWR|O_CREAT|O_TRUNC, 0644);
    if(fdout<0){ perror("open out"); close(fdin); return 1; }
    decompress_fd(fdin, fdout);
    close(fdin); close(fdout);
    return 0;
}
