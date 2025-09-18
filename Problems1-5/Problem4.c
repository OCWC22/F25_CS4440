#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
    if(argc!=3){ fprintf(stderr,"usage: %s src dst\n",argv[0]); return 1; }

    int p[2]; if(pipe(p)){ perror("pipe"); return 1; }
    int pid=fork(); if(pid<0){ perror("fork"); return 1; }

    if(pid==0){                                    // child: pipe -> dst (lowercased)
        close(p[1]);
        FILE *in=fdopen(p[0],"r"), *out=fopen(argv[2],"w");
        if(!in||!out){ perror("fopen"); return 1; }
        int c; while((c=fgetc(in))!=EOF){ if(c>='A'&&c<='Z') c+=32; fputc(c,out); }
        fclose(out); fclose(in); return 0;
    }else{                                         // parent: src -> pipe
        close(p[0]);
        FILE *src=fopen(argv[1],"r"); if(!src){ perror("fopen"); return 1; }
        FILE *w=fdopen(p[1],"w");
        int c; while((c=fgetc(src))!=EOF) fputc(c,w);
        fclose(src); fclose(w); wait(NULL); return 0;
    }
}

