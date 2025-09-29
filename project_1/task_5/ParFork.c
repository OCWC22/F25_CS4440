#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void flush(FILE*out,int ch,int n){
    if(!n) return;
    if(n>=16) fprintf(out,ch=='0'?"-%d-":"+%d+",n);
    else while(n--) fputc(ch,out);
}
static void comp_buf(const char*s,size_t L,FILE*out){
    int prev=-1,n=0;
    for(size_t i=0;i<L;i++){
        int c=s[i];
        if(c=='0'||c=='1'){
            if(c==prev) n++; else { flush(out,prev,n); prev=c; n=1; }
        }else{ flush(out,prev,n); prev=-1; n=0; fputc(c,out); }
    }
    flush(out,prev,n);
}

int main(int argc,char**argv){
    if(argc!=4){ fprintf(stderr,"usage: %s src dst nproc\n",argv[0]); return 1; }

    FILE*in=fopen(argv[1],"r"); if(!in){ perror("src"); return 1; }
    fseek(in,0,SEEK_END); long N=ftell(in); fseek(in,0,SEEK_SET);
    char *buf=malloc(N); if(!buf){ perror("malloc"); return 1; }
    fread(buf,1,N,in); fclose(in);

    int n=atoi(argv[3]); if(n<1) n=1;
    long chunk=(N+n-1)/n;
    int (*pp)[2]=malloc(sizeof(int[2])*n);
    pid_t *pids=malloc(sizeof(pid_t)*n);

    for(int i=0;i<n;i++){
        pipe(pp[i]);
        if((pids[i]=fork())==0){
            close(pp[i][0]);
            FILE*w=fdopen(pp[i][1],"w");
            long s=(long)i*chunk, e=s+chunk; if(e>N) e=N;
            if(s<N) comp_buf(buf+s,e-s,w);
            fclose(w); _exit(0);
        }
        close(pp[i][1]);
    }

    FILE*out=fopen(argv[2],"w");
    for(int i=0;i<n;i++){
        FILE*r=fdopen(pp[i][0],"r"); int c;
        while((c=fgetc(r))!=EOF) fputc(c,out);
        fclose(r); waitpid(pids[i],NULL,0);
    }
    fclose(out);
    free(buf); free(pp); free(pids);
    return 0;
}
