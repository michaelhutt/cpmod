#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>


void printFullPerms(unsigned int perm) {
    unsigned int tPerm;
    tPerm = (perm&S_IRWXU)>>6;
    printf("%c%c%c",
	   (tPerm&4)!=0?'r':'-',
	   (tPerm&2)!=0?'w':'-',
	   (tPerm&1)!=0?((perm&S_ISUID)?'s':'x'):((perm&S_ISUID)?'S':'-')
	   );
    tPerm = (perm&S_IRWXG)>>3;
    printf("%c%c%c",
	   (tPerm&4)!=0?'r':'-',
	   (tPerm&2)!=0?'w':'-',
	   (tPerm&1)!=0?((perm&S_ISGID)?'s':'x'):((perm&S_ISGID)?'S':'-')
	   );    
    tPerm = (perm&S_IRWXO);
    printf("%c%c%c",
	   (tPerm&4)!=0?'r':'-',
	   (tPerm&2)!=0?'w':'-',
	   (tPerm&1)!=0?((perm&S_ISVTX)?'t':'x'):((perm&S_ISVTX)?'T':'-')
	   );    
}

void get_mask_and_shift(char spec, mode_t *mask, size_t *shift) {
    switch(spec) {
    case 'u':
	*mask = S_IRWXU;
	*shift = 6;
	break;
    case 'g':
	*mask = S_IRWXG;
	*shift = 3;
	break;
    case 'o':
	*mask = S_IRWXO;
	*shift = 0;
	break;
    }
}

void modcopy(char *filename, char *spec, int recurse) {
    struct stat filestat;
    if( stat(filename, &filestat) == 0) {
	printf("%s [ ", filename);
	printFullPerms(filestat.st_mode);
	printf(" ] -> ");
	mode_t from_mask=0;
	mode_t to_mask=0;
	size_t from_shift=0;
	size_t to_shift=0;

	get_mask_and_shift(spec[0], &from_mask, &from_shift);
	get_mask_and_shift(spec[1], &to_mask, &to_shift);
	
	/* copying to a new mode prevents having to handle 'uu','gg', or 'oo' cases */
	mode_t new_mode = filestat.st_mode;
	
	/* clear existing permissions and copy in the new ones */
	new_mode = new_mode & ~to_mask;
	new_mode = new_mode | (((filestat.st_mode & from_mask) >> from_shift) << to_shift);

	if( new_mode == filestat.st_mode || chmod(filename, new_mode) == 0 ) {
	    printf("[ ");
	    printFullPerms(new_mode);
	    printf(" ]\n");
	}
	else {
	    fprintf(stderr, "Could not change file mode on '%s', %s\n", filename, strerror(errno));
	}

	if(recurse && S_ISDIR(filestat.st_mode) != 0) {
	    DIR *dir = opendir(filename);
	    if(dir) {
		struct dirent *next_entry;
		char pathname[8096];
		strcpy(pathname, filename);

		while((next_entry = readdir(dir)) != NULL) {
		    if(strcmp(".", next_entry->d_name) != 0 && strcmp("..", next_entry->d_name) != 0) {
			char full_path[8096];
			sprintf(full_path, "%s/%s", pathname, next_entry->d_name);
			modcopy(full_path, spec, recurse);
		    }
		}
	    }
	}
    }
    else {
	fprintf(stderr, "[Error] %s (%s)\n", strerror(errno), filename);
    }
}

void printUsage(FILE *stream, const char *cmd) {
    fprintf(stream, "Usage: %s [-r] POSITION-SPEC FILE\n", cmd);
}

int main(int argc, char **argv) {
    /* TODO: convert to arg handling to getopts. */
    char *filename;
    int recurse = 0;
    int i = 1;

    if (argc < 2) {
	printUsage(stderr, argv[0]);
	return EXIT_FAILURE;
    }

    /* pick out the position spec and the recurse option */
    char *spec = argv[i++];
    if(strcmp("-r", spec) == 0){
	spec = argv[i++];
	recurse = 1;
    }
    spec[0] = tolower(spec[0]);
    spec[1] = tolower(spec[1]);
    /* Allow '-r' option to be the first or second argument. */
    if (!recurse && strcmp("-r", argv[i]) == 0) {
	recurse = 1;
	++i;
    }

    /* format/length checks */
    if(argc - i < 1) {
	printUsage(stderr, argv[0]);
	return EXIT_FAILURE;
    }
    if(strlen(spec) != 2) {
	fprintf(stderr, "Position specifier must be two characters.\n");
	printUsage(stderr, argv[0]);
	return EXIT_FAILURE;
    }
    else {
	/* make sure position specifier has the correct format */
	if((spec[0] != 'u' && spec[0] != 'g' && spec[0] != 'o') ||
	   (spec[1] != 'u' && spec[1] != 'g' && spec[1] != 'o')) {
	    fprintf(stderr, "Position specifier must be two characters long, consisting of: 'u', 'g', or 'o' (user, group, or other)\n");
	    printUsage(stderr, argv[0]);
	    return EXIT_FAILURE;
	}
    }

    for(; i<argc; ++i) {
	filename = argv[i];
	modcopy(filename, spec, recurse);
    }
    return EXIT_SUCCESS;
}
