
/*
 * Copyright (c) 2017 stephen.farrell@cs.tcd.ie
 * Author: Leong Kai Ler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

// usual includes
#include <stdio.h>
#include <stdlib.h>

// needed for getting access to /dev/random
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/random.h>

// the most we wanna print
#define LIMIT 65536

#define LINEliMIT = 8;

void usage(char *progname)
{
    fprintf(stderr,"Print some random numbers from /dev/random.\n");
    fprintf(stderr,"Options:\n");
    fprintf(stderr,"\t%s <number> where number is the number of bytes to print [Default: 10, min: 0, max: %d]\n",progname,LIMIT);
    exit(-1);
}

//returns a random byte
unsigned char rndbyte()
{
    unsigned long int s;
    syscall(SYS_getrandom, &s, sizeof(unsigned long int), 0);
    unsigned char byte=(s>>16)%256;
    return(byte);
}

int main(int argc,char *argv[])
{
    FILE *file;
    int number;
    int lineLimit=8;

//check number of inputs 
    if (argc!=3) {
        printf("Input error!!!");
        return(-1);
    }

//check number of bytes to print
    else {
        int newnumber=atoi(argv[1]);

        if (newnumber<=0) {
            fprintf(stderr,"%d too small\n",newnumber);
            usage(argv[0]);
        }
        if (newnumber>LIMIT) {
            fprintf(stderr,"%d too big\n",newnumber);
            usage(argv[0]);
        }
        number=newnumber;
        char* filename = argv[2];
        file=fopen(filename,"w");
    }

//print bytes
    for (int i = 0; i != number; i++) {
        unsigned char byte1 = rndbyte();
        fprintf(file,"%d,%02x",i,byte1);
        for ( int j = 0; j!=byte1;j++) {

            if (j % lineLimit == 0) {
                fprintf(file,"\n");
            }
            unsigned char byte2 = rndbyte();
            if (j==byte1-1)
	    {
	       	fprintf(file,"%02x", byte2); //I could put and a new line every time I end here but I guess directly having 2 new lines after I finish this inner nested loop would be better.
	    }
            else{
                fprintf(file,"%02x,", byte2);
            }	  
        }
	fprintf(file,"\n\n");
    }   
    return(0);
}
