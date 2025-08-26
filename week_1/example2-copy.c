#include <stdio.h>
#include <stdlib.h>


int main() {
    FILE *source, *destination;
    int ch;

    source = fopen("source.txt", "r");
    
    destination = fopen("destination.txt", "w");

    ch = fgetc(source);
    while (ch != EOF) {
        fputc(ch, destination);
        ch = fgetc(source);
    }

    fclose(source);
    fclose(destination);

    return 0;
}


#include <stdio.h>
#include <stdlib.h>

int main(){
    FILE *source, *destination;
    int ch;
    
    source = fopen("source.txt", "r");
    
    destination = fopen("destination.txt", "w");
    
    ch = fgetc(source);
    while (ch != EOF){
        fputc(ch, destination);
        ch = fgetc(source);
    }
    
    fclose(source);
    fclose(destination);
}
