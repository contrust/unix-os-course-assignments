#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>

int main(){
    FILE * output;
    long int sum = 0;
    char buffer[30];
    int number;
    output = fopen("/tmp/numbers.txt", "w");
	if (output == NULL)
		return 1;
    
    srand(time(NULL));
    for (int i = 0; i < 999; ++i){
        number = rand() % 1000000 - 500000;
        sum += number;
        int n = snprintf(buffer, sizeof(buffer), "%d\n", number);
        fwrite(buffer, 1, n, output);
    }
    int n = snprintf(buffer, sizeof(buffer), "%ld\n", -sum);
    fwrite(buffer, 1, n, output);
}