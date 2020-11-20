#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<signal.h>
#include<fcntl.h>

int pathArrSize;
char **paths;

char** parse(char *string, char* delimeter) {
	if (string == NULL) {
		return NULL;
	}
	char **temp = (char **)malloc(sizeof(char*)*1024);

	int index;
	for (int i = 0; i < 1024; i++) {
		index = sizeof(char*)*i;
           	*(temp+index) = NULL;
      	}

       	char *curr = strsep(&string, delimeter);
        int i = 0;
		int numTok = 0;
        //Loading tokens into temporary array
        while(curr!=NULL) {
			index = sizeof(char*)*i;
			if (strlen(curr)>0) {
           		*(temp+index) = curr;
				i++;
				numTok++;
			}
			curr = strsep(&string, delimeter);
       	}
	//resize the temp and fill with tokens
   	char **retArr = (char **)malloc(sizeof(char*) * (numTok+1));

    for(i = 0; i < numTok+1; i++) {
		index = sizeof(char*)*i;
		if (i == numTok) {
			*(retArr+index) = malloc(sizeof(char*));
			*(retArr+index) = NULL;
			break;	
        }
		char *tok = *(temp+index);
		*(retArr+index) = malloc(sizeof(char)*strlen(tok));
    	*(retArr+index) = tok;
     }
	free(temp);
	return retArr;
}

char* trimNL(char *string) {
	if (string == NULL || strlen(string) == 0) {
		return string;
	}
	char* end = (string + (strlen(string) - 1));
	if (*end == '\n') {
		*end = '\0';	
	}
	return string;
}

void resizePaths() {
	int newSize = pathArrSize*2;
	char **newPaths = (char **)malloc(sizeof(char*)*newSize);
	for (int i = 0; i < newSize; i++) {
   		*(newPaths + (sizeof(char*)*i)) = NULL;
	}

	char* curr;
	for (int i = 0; i < pathArrSize; i++) {
    	curr = *(paths+(sizeof(char*)*i));
		*(newPaths + (sizeof(char*)*i)) = curr;
    }
	free(paths);
	paths = newPaths;
	pathArrSize = newSize;
}

char* appendCmd(char *string, char *cmd) {
	int newSize = strlen(string) + strlen(cmd);
	char *new = (char*)malloc(sizeof(char)* (newSize));
   	strcpy(new, string);
	strncat(new, cmd, strlen(cmd));
	return new;	
}

char* appendSlashCmd(char *string, char *cmd) {
	int newSize = strlen(string) + strlen(cmd) + 2;
	char *new = (char*)malloc(sizeof(char)*newSize);
	strcpy(new, string);
	strncat(new, "/", 1);
	strncat(new, cmd, strlen(cmd));
	return new;
}

void addPath(char *path) {
	char *curr;
	int i = 1;
	char *save = *paths;
	char *temp = *paths;
  	while(1) {
  		if (i == pathArrSize-1) {
      			resizePaths();
 		}

   		curr = *(paths + sizeof(char*)*i);
     		if (curr == NULL) {
       			*(paths+sizeof(char*)*i) = malloc(sizeof(char)*(strlen(path)+1));
        		*(paths + sizeof(char*)*i) = temp;
        		break;
   		}
		save = *(paths + sizeof(char*)*i);
		*(paths+sizeof(char*)*i) = malloc(sizeof(char)*(strlen(path)+1));
                *(paths + sizeof(char*)*i) = temp;
		temp = save;
		i++;
 	}
	*(paths) = malloc(sizeof(char)*(strlen(path)+1));
	*(paths) = path;
}

void removePath(char *path) {
	int i;
	for (i = 0; i < pathArrSize; i++) {
		if (*(paths + sizeof(char*)*i) == NULL) {
			return;
		}
		if (!strcmp(*(paths + (sizeof(char*)*i)), path)) {
			break;
		}
	}

	if (i < pathArrSize) {
		for (int j = i; j < pathArrSize-1; j++) {
			*(paths + (sizeof(char*)*j)) = *(paths + (sizeof(char*)*(j+1)));
		}
		*(paths + sizeof(char*)*pathArrSize) = NULL;
	}
}

void clearPath() {
	for (int i = 0; i < pathArrSize; i++) {
                *(paths + (sizeof(char*)*i)) = NULL;
        }
}

int main(int argc, char *argv[]) {
	//setting up path list
	pathArrSize = 512;
	paths = (char **)malloc(sizeof(char*)*pathArrSize);
	for (int i = 0; i < pathArrSize; i++) {
        	*(paths + (sizeof(char*)*i)) = NULL;
	}
	*(paths) = malloc(sizeof("/bin"));
	*(paths) = "/bin";

	char *buff = NULL;
	size_t buffSize = 0;
	char *commandLine = NULL;
	FILE *readFrom;
	int batch;

	if (argc > 2) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

	if (argc == 2) {
		readFrom=fopen(argv[1], "r");
		if (readFrom==NULL) {
			char error_message[30] = "An error has occurred\n";
        	write(STDERR_FILENO, error_message, strlen(error_message));
        	exit(1);
		}
		batch = 1;
	} else {
		readFrom = stdin;
		batch = 0;
	}

	do {
		buff = NULL;
		buffSize = 0;
		commandLine = NULL;
		if (!batch) {
			//prompts user
			printf("smash> ");
			fflush(stdout);
		}

       	//gets the command line 
        if(getline(&buff, &buffSize, readFrom)<0) {
			break;
		}
	    commandLine = (char *)malloc(sizeof(char)*buffSize);	
		strcpy(commandLine, buff);
		commandLine = trimNL(commandLine);
		
		//array to store the multiple commands
		char **multiples = parse(commandLine, ";");
	
		int index = 0;
		int i  = 0;
		char *currMult = *(multiples+index);

		//iterate through multiple commands
		while(currMult!=NULL) {
			//array to store the parrallel commands
			char **parallels = parse(currMult, "&");

			int indexTwo = 0;
			int j = 0;
			char *currPar = *(parallels+indexTwo);

			//iterating through parallel commands
			while(currPar!=NULL) {
				//parse by " " to get individual tokens
				char **currCommands = parse(currPar, " ");

				//command is first elements in currCommands
				char *command = *currCommands;
				int numArgs = 0; //number to keep track of args
				int in = 0;
				int rdIndex  = 0;
				int redirection = 0;
				char* out = NULL;

				char *cur = *(currCommands+sizeof(char*)*in);
				while(cur!=NULL) {
					if (!strcmp(cur, ">")) {
						redirection = 1;
						rdIndex = in;
					}
					if (in == (rdIndex+1)) {
						out = cur;			
					}
					//will check for redirect eventually
					cur = *(currCommands+sizeof(char*)*(++in));
					numArgs++;
				}
					
				if (command == NULL || !strcmp(command, "")) {
					indexTwo = sizeof(char*)*(++j);
                    currPar = *(parallels+indexTwo);
					continue;
				}
			
				if (!redirection) {
					//Built in commands
					if (!strcmp(command,"exit")) {
						char *arg1 = *(currCommands+sizeof(char*));
						if (arg1 == NULL) {
							exit(0);	
						} else {
							char error_message[30] = "An error has occurred\n";
                           	write(STDERR_FILENO, error_message, strlen(error_message));
						}
					}

					else if (!strcmp(command, "cd")) {
						char *arg1 = *(currCommands+sizeof(char*));
						char *arg2 = *(currCommands+(2*sizeof(char*)));
						if (arg1!=NULL && arg2==NULL) {
							int result = chdir(arg1);
							if (result < 0) {
								char error_message[30] = "An error has occurred\n";
                       			write(STDERR_FILENO, error_message, strlen(error_message));
							}
						} else {
							char error_message[30] = "An error has occurred\n";
                      		write(STDERR_FILENO, error_message, strlen(error_message));
						}
					}

					else if(!strcmp(command, "path")) {
						char *arg1 = *(currCommands+sizeof(char*));
						if (arg1 == NULL) {
							char error_message[30] = "An error has occurred\n";
                      		write(STDERR_FILENO, error_message, strlen(error_message));
						} else {
							if (!strcmp(arg1, "add")) {
								char *arg2 = *(currCommands+(2*sizeof(char*)));
								if (arg2 == NULL) {
									char error_message[30] = "An error has occurred\n";
                           			write(STDERR_FILENO, error_message, strlen(error_message));
								} else {
									addPath(arg2);
								}
						
							} else if (!strcmp(arg1, "remove")) {
								char *arg2 = *(currCommands+(2*sizeof(char*)));
								
                            	if (arg2 == NULL) {
                           	  		char error_message[30] = "An error has occurred\n";
                           			write(STDERR_FILENO, error_message, strlen(error_message));
              					} else {
                           			removePath(arg2);
                        		}
							} else if (!strcmp(arg1, "clear")) {
								clearPath();
							} else {
								char error_message[30] = "An error has occurred\n";
                           		write(STDERR_FILENO, error_message, strlen(error_message));
							}
						} 
					}
					//Non built ins
					else {
						char *currPath;	
						char *filePath;
						int foundPath = 0;
						for (int k = 0; k < pathArrSize; k++) {
							currPath = *(paths + (sizeof(char*)*k));
							if (currPath == NULL) {
								break;
							}
							filePath = appendSlashCmd(currPath, command);
							if (!access(filePath, X_OK)) {
								foundPath = 1;
								break;
							}
						
							filePath = NULL;
							filePath = appendCmd(currPath, command);
							if (!access(filePath, X_OK)) {
								foundPath = 1;
								break;
							}
						}

						if (!foundPath) {
							char error_message[30] = "An error has occurred\n";
     	              		write(STDERR_FILENO, error_message, strlen(error_message));
                  		} else {
							int argSize = numArgs+1;
							char* args[argSize];
							for (int k = 0; k < argSize; k++) {
                           		args[k] = NULL;
                           	}
						
							char* cmd;
							for (int k = 0; k < numArgs; k++) {
								cmd = *(currCommands + (sizeof(char*)*k));						
								args[k] = cmd;
							}
							
							//---have args---
							
							int r = fork();
							if (r == 0) {
								execv(filePath, args);
								exit(0);	
							} else {
								wait(NULL);
							}
						}
  		      		} 

						
				} else { //if redirection is turned on
						if (rdIndex == 0 || out == NULL) {
							char error_message[30] = "An error has occurred\n";
                               write(STDERR_FILENO, error_message, strlen(error_message));
						}
						char *currPath;
                        char *filePath;
						int foundPath = 0;
                       	for (int k = 0; k < pathArrSize; k++) {
                       		currPath = *(paths + (sizeof(char*)*k));
                         	if (currPath == NULL) {
                      			break;
                          	}
                            filePath = appendSlashCmd(currPath, command);
                       		if (!access(filePath, X_OK)) {
                        		foundPath = 1;
                            	break;
                       		}

                      		filePath = NULL;
                       		filePath = appendCmd(currPath, command);
                          	if (!access(filePath, X_OK)) {
                     			foundPath = 1;
                    	       	break;
                            }
                    	}

                    	if (!foundPath) {
                       		char error_message[30] = "An error has occurred\n";
                           	write(STDERR_FILENO, error_message, strlen(error_message));
                     	} else {
							char *rd[rdIndex + 1];
							for (int k = 0; k < rdIndex+1; k++) {
								rd[k] = NULL;
							}
							rd[0] = command;

							char* cmd;
                         	for (int k = 0; k < rdIndex; k++) {
                             	cmd = *(currCommands + (sizeof(char*)*k));
                            	rd[k] = cmd;
                         	}

							int r = fork();
							if (r == 0) {
								int a = open(out, O_WRONLY | O_CREAT | O_TRUNC, S_IWOTH | S_IRUSR | S_IWUSR);
								if (a < 0) {
									char error_message[30] = "An error has occurred\n";
                               		write(STDERR_FILENO, error_message, strlen(error_message));
								}

								if (dup2(a,1) <0) {
									close(a);
									char error_message[30] = "An error has occurred\n";
                               		write(STDERR_FILENO, error_message, strlen(error_message));
								} else {
									if (dup2(a, STDERR_FILENO) < 0) {
										char error_message[30] = "An error has occurred\n";
                               			write(STDERR_FILENO, error_message, strlen(error_message));
									} else {
										execv(filePath, rd);
										close(a);
										exit(0);
									}
								}
							} else {
								wait(NULL);
							}
						} 
						redirection = 0;
					} 
					free(commandLine);
					free(currCommands);
					indexTwo = sizeof(char*)*(++j);
					currPar = *(parallels+indexTwo);
				}
				free(parallels);	
				index = sizeof(char*)*(++i);
				currMult = *(multiples+index);
			}
			free(multiples);
			free(buff);
			buff = NULL;
		} while(1);
}

