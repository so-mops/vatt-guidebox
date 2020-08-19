#include <stdio.h>
/* Standard headers */  
#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <math.h>       
#include <unistd.h>     
#include <sys/time.h>   
                         

#define FN "LOWER_FNAMES"

int main(int argc, char * argv[])
{
        FILE *fid;
	char filters[5][30];
	if( access( FN, F_OK ) != 1 )
		fid = fopen(FN, "r");
	else                                                                                              
	{                                                                                                 
		fid = NULL;   
		printf("SHIT fid is null\n")		;
	}

	if(fid)                                                                                           
	{
		int wc = fscanf(fid, "%[^,], %[^,], %[^,], %[^,], %s", filters[0], filters[1], filters[2], filters[3], filters[4] );
		printf("%s %s %s %s %s\n", filters[0], filters[1], filters[2], filters[3], filters[4] );
	}

}
