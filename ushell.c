#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "parse.h"
#include <signal.h>
#include <sys/resource.h>
#include <sys/mman.h>
extern char **environ;
static int  *endProcess;

#define OS_TEST 0

#ifdef OS_TEST
#define OS_Printf(x) printf(x)
#define Log_print(x) fprintf(x)
//#define VA_ARG(...) , ##__VA_ARGS__

#define LOG( fmt , ... )    \
        do{ \
            FILE* F = fopen( "ushell_trace.txt" , "a" ) ; \
            if( NULL == F )    \
                break ; \
            fprintf(F, fmt" FILE_NAME: %s LINE_NO: %d\n\n", ##__VA_ARGS__, \
                                __FILE__,__LINE__); \
            fclose(F);   \
        }while( 0 )
#else
#define OS_Printf(x) 
#define LOG(x) 
#endif

#define BOOL unsigned int
#define TRUE 1
#define FALSE 0
#define PERMISSIONS 0664

int originailSTDIN;
int originalSTDOUT;
int originalSTDERR;

void ExecuteCommand(Cmd Command_List,int flag);
void processPipe(Pipe pipe_Input,int flag);
void run_command_other(Cmd Command_List);
void exe_cd(Cmd Command_List);
void run_command_where(Cmd Command_List);
void exe_setenv(Cmd Command_List);
void run_command_setenv(Cmd Command_List);
void exe_unsetenv(Cmd Command_List);
void run_command_unsetenv(Cmd Command_List);
void exe_nice(Cmd Command_List);
void run_command_nice(Cmd Command_List);


typedef int system_Pipe;

system_Pipe s_Pipe[2];
//int file_Descriptor = 0;
int file_Descriptor1;
char *gpc_EnvironmentVariables = NULL;
char **ppcEnv;

typedef enum E_LIST_OF_COMMANDS
{
    e_start,
    e_cd,
    e_echo,
    e_logout,
    e_pwd,
    e_other,
    e_where,
    e_setenv,
    e_unsetenv,
    e_nice,
    e_end      
}e_List_of_Commands;

e_List_of_Commands get_CommandType(char *Command );

char *szBuiltInCommands[e_end]={ NULL,\
        "cd",\
        "echo",\
        "logout",\
        "pwd",\
        NULL,\
        "where",\
        "setenv",\
        "unsetenv",\
        "nice",\
        NULL};

void exe_pwd();
void exe_echo(Cmd Command_List);
int exe_where(Cmd Command_List);

void (*execute_command[e_end])(Cmd Command_List)={
    NULL,
    exe_cd,//run_command_cd,
    exe_echo,//run_command_echo,
    NULL,//run_command_logout,
    exe_pwd,
    NULL,//run_command_other,
    exe_where,
    exe_setenv,
    exe_unsetenv,
    exe_nice,
    NULL};

void signal_int_handler()
{
    printf("\r\n");
    printf("%%");
    fflush(STDIN_FILENO);
}

void run_child(Cmd Command_List,e_List_of_Commands e_command)
{
    int a;
    system_Pipe s_newPipe[2];
    pipe(s_newPipe);
    
    pid_t processID = fork();
    
    /*
     * processID = 0 --> Child
     * processID > 0 --> Parent
     */
    
    
    if(0 == processID) 
    {
        LOG("Making for Parent");
        LOG("FD = %d",file_Descriptor1);
        dup2(file_Descriptor1,STDIN_FILENO);
        char *pcCwd;
       
	 if(Command_List->in == Tin && NULL == Command_List->next)
        {
            LOG("Command_List->in == Tin");
            int fd = open(Command_List->infile, O_RDONLY, PERMISSIONS);
            if(fd < 0)
            {
                perror(Command_List->infile);
                exit(0);
            }
            else
            {
                dup2(fd,STDIN_FILENO);
                close(fd);
            }
        } 
        if(Command_List->next && (Command_List->next->in == Tpipe ||\
                            Command_List->next->in == TpipeErr))
        {
            LOG("Making for child");
            dup2(s_newPipe[1],STDOUT_FILENO);
            if( Command_List->next->in == TpipeErr)
                dup2(s_newPipe[1],STDERR_FILENO);
            close(s_newPipe[1]);
        }
        
        if( Command_List->next == NULL &&(Command_List->out == Tout || Command_List->out == ToutErr))//&& NULL == Command_List->next )
        {
            LOG("Command_List->out = Tout");
            int fd = open(Command_List->outfile,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            if(fd < 0)
            {    
                LOG("Could not create directory");
                perror(Command_List->outfile);
                _exit(0);
            }
            dup2(fd,STDOUT_FILENO);
            if(Command_List->out == ToutErr)
            {
                LOG("ERROR CASE Touterr");
                dup2(fd,STDERR_FILENO);
            }
            close(fd);
            
        }
        
       
        
        else if(Command_List->next == NULL && (Command_List->out == Tapp || Command_List->out == TappErr))
        {
            int fd = open(Command_List->outfile,O_WRONLY|O_APPEND|O_CREAT, PERMISSIONS);
            if(fd < 0)
            {    
                LOG("Could not create directory");
                perror(Command_List->outfile);
                _exit(0);
            }
            dup2(fd,STDOUT_FILENO);
            if(Command_List->out == TappErr)
                dup2(fd,STDERR_FILENO);
            close(fd);
        }
        
                   
        //else
        {
            LOG("Executing Single Command");
            LOG("Command: %s",Command_List->args[0]);
            LOG("Outfile: %s",Command_List->outfile);
            //execv("/bin/ls",Command_List->args);
            //execvp(Command_List->args[0],Command_List->args);
            //perror("execvp:");
            //LOG("Execvp Failed");
        }
        //close(s_Pipe[0]);
        LOG("BEOFRE");
        //exe_pwd();
        
        execute_command[e_command](Command_List);
        
        //printf("%s\n",pcCwd);
        //printf("asdsad");
        LOG("Printing Done");
        _exit(EXIT_SUCCESS);
        LOG("EXit Failed");
    }
    else if(processID > 0)
    {
        
        LOG("Parent Waiting");
        //wait();
        waitpid(processID,&a,0);
        LOG("Parent Waiting Over");
#if 0
        if(Command_List->next)
        {
            if(Command_List->next->in == Tpipe)
            {
                close(s_Pipe[1]);
            }
        }
#endif
        
        close(s_newPipe[1]);
        file_Descriptor1 = s_newPipe[0];
        LOG("FD = %d",file_Descriptor1);
    }
    else
    {
        LOG("Fork Failed");
        perror("Fork: ");
        exit(1);
    }
}
void exe_cd(Cmd Command_List)
{
    int ret;
    if(Command_List->args[1])
    {
        ret = chdir(Command_List->args[1]);
    }
    else
    {
        char *pcHomeDir;
        pcHomeDir = getenv("HOME");
        ret = chdir(pcHomeDir);
    }
    if(ret == -1)
        perror("cd");
}

void run_command_cd(Cmd Command_List)
{
    if(NULL!=Command_List->next || Command_List->out != Tnil)
    {
        run_child(Command_List,e_cd);
    }
    else
    {
        exe_cd(Command_List);
    }
}

void exe_echo(Cmd Command_List)
{
    int i;
    for( i= 1 ; i<Command_List->nargs;i++)
    {
        printf("%s",Command_List->args[i]);
        if((i+1) != Command_List->nargs)
            printf(" ");
    }
    printf("\n");
}
void run_command_echo(Cmd Command_List)
{
    int i;
    if(NULL!=Command_List->next || Command_List->out != Tnil)
    {
        run_child(Command_List,e_echo);
        LOG("Done Running Child");
    }
    else
    {
        exe_echo(Command_List);
    }
#if 0
    for( i= 1 ; i<Command_List->nargs;i++)
    {
        printf("%s",Command_List->args[i]);
        if(NULL != Command_List->next)
            printf(" ");
    }
    printf("\n");
#endif
    //printf("Hello\n");
    LOG("Done Printing");
   // exit(0);
}
int exe_where(Cmd Command_List)
{
    char pcEnvPath[1024]={0};
    char *pcTemp_Start,*pcTemp_End,*pcTemp;
    char *pcDupEnvPath;
    char szTempDir[1024]={0};
    struct stat st;
    BOOL bFlag = FALSE;
    BOOL bFound = FALSE;
    int len,i;
    memset(&szTempDir,0,1024);
    
    //memset(pcEnvPath,0,1024);
    //pcDupEnvPath = getenv("PATH");
    //pcEnvPath = malloc(strlen(getenv("PATH"))+1);
    //memset(pcEnvPath,0,strlen(pcDupEnvPath)+1);
    //memcpy(pcEnvPath,pcDupEnvPath,strlen(pcDupEnvPath));
    if(Command_List->nargs == 1)
    {
        printf("where: Too few arguments.\n");
        return 0;
    }
    
    strcpy(pcEnvPath,getenv("PATH"));
    //pcEnvPath = strdup(pcDupEnvPath);
    for(i = 1 ; i<Command_List->nargs ; i++)
    {
        bFlag = FALSE;
        e_List_of_Commands e_CommandType;
        e_CommandType = get_CommandType(Command_List->args[i]);
        if(e_CommandType >e_start && e_CommandType < e_end)
        {
            if(e_CommandType!= e_other)
            {
                printf("%s: shell built-in command.\n",\
                        szBuiltInCommands[e_CommandType]);
                bFound = TRUE;
            }
            //else
            {
                pcTemp_Start = pcEnvPath;
                pcTemp_End = strstr(pcEnvPath,":");
                pcTemp = pcTemp_Start;

                while(pcTemp_Start && pcTemp_End)
                {
                    len = pcTemp_End - pcTemp_Start;
                    //memcpy(&szTempDir,pcTemp_Start,pcTemp_Start+len);
                    strncpy(szTempDir,pcTemp_Start,len);
                    strcat(szTempDir,"/");
                    strncat(szTempDir,Command_List->args[i],\
                            strlen(Command_List->args[i]));
                    LOG("Path: %s",szTempDir);
                    if(stat(szTempDir,&st)==0)
                    {
                        printf("%s\n",szTempDir);
                        bFlag = TRUE;
                        bFound = TRUE;
                    }
                    memset(szTempDir,0,sizeof(szTempDir));
                    pcTemp_Start = pcTemp_End+1;

                    if(pcTemp_Start)
                    {
                        pcTemp_End = strstr(pcTemp_Start,":");
                    }
                }
                if(bFlag == FALSE)
                {
                    e_List_of_Commands e_CommandType;
                    e_CommandType = get_CommandType(Command_List->args[i]);
                    printf("%s: Command not found\n",Command_List->args[i]);
                }
            }
        }
    }
    return bFound;
}
int exe_where_new(Cmd Command_List)
{
    char pcEnvPath[1024]={0};
    char *pcTemp_Start,*pcTemp_End,*pcTemp;
    char *pcDupEnvPath;
    char szTempDir[1024]={0};
    struct stat st;
    BOOL bFlag = FALSE;
    BOOL bFound = FALSE;
    int len,i;
    memset(&szTempDir,0,1024);
    
    //memset(pcEnvPath,0,1024);
    //pcDupEnvPath = getenv("PATH");
    //pcEnvPath = malloc(strlen(getenv("PATH"))+1);
    //memset(pcEnvPath,0,strlen(pcDupEnvPath)+1);
    //memcpy(pcEnvPath,pcDupEnvPath,strlen(pcDupEnvPath));
    
    
    strcpy(pcEnvPath,getenv("PATH"));
    //pcEnvPath = strdup(pcDupEnvPath);
    for(i = 0 ; i<Command_List->nargs ; i++)
    {
        bFlag = FALSE;
        e_List_of_Commands e_CommandType;
        e_CommandType = get_CommandType(Command_List->args[i]);
        if(e_CommandType >e_start && e_CommandType < e_end)
        {
            if(e_CommandType!= e_other)
            {
                //printf("%s: shell built-in command.\n",\
                        szBuiltInCommands[e_CommandType]);
                bFound = TRUE;
            }
            else
            {
                pcTemp_Start = pcEnvPath;
                pcTemp_End = strstr(pcEnvPath,":");
                pcTemp = pcTemp_Start;

                while(pcTemp_Start && pcTemp_End)
                {
                    len = pcTemp_End - pcTemp_Start;
                    //memcpy(&szTempDir,pcTemp_Start,pcTemp_Start+len);
                    strncpy(szTempDir,pcTemp_Start,len);
                    strcat(szTempDir,"/");
                    strncat(szTempDir,Command_List->args[i],\
                            strlen(Command_List->args[i]));
                    LOG("Path: %s",szTempDir);
                    if(stat(szTempDir,&st)==0)
                    {
                  //      printf("%s\n",szTempDir);
                        bFlag = TRUE;
                        bFound = TRUE;
                    }
                    memset(szTempDir,0,sizeof(szTempDir));
                    pcTemp_Start = pcTemp_End+1;

                    if(pcTemp_Start)
                    {
                        pcTemp_End = strstr(pcTemp_Start,":");
                    }
                }
                if(bFlag == FALSE)
                {
                    e_List_of_Commands e_CommandType;
                    e_CommandType = get_CommandType(Command_List->args[i]);
                    //printf("%s: Command not found\n",Command_List->args[i]);
                }
            }
        }
    }
    return bFound;
}

void run_command_where(Cmd Command_List)
{
    if(NULL!= Command_List->next || Command_List->out != Tnil)
    {
        run_child(Command_List,e_where);
    }
    else
    {
        exe_where(Command_List);
    }    
}
void exe_setenv(Cmd Command_List)
{
    if(Command_List->nargs == 1)
    {
        char **env;
    
        char **ppcTmp = environ;
        while(*ppcTmp!=NULL)
        {
            printf("%s\n",*ppcTmp);
            *ppcTmp++;
        }
    }
    else if(Command_List->nargs <=3)
    {
        BOOL bFlag = FALSE;
        //char **ppcTmp = ppcEnv;
        char *pcnewEnvVar = NULL;
        char *pcLocation = NULL;
        char **ppcTmp = environ;
        
        int len = strlen(Command_List->args[1]);

        while(*ppcTmp!=NULL)
        {
            if((strcmp(Command_List->args[1],*ppcTmp) ==0))
            {
                bFlag = TRUE;
                pcLocation = *ppcTmp;
                break;
            }
            if(!bFlag)
                *ppcTmp++;
        }
        
        if(Command_List->nargs == 2)
        {
            int ret ;
            pcnewEnvVar = malloc(len+2);
            memset(pcnewEnvVar,0,len+2);
            strcpy(pcnewEnvVar,Command_List->args[1]);
            strcat(pcnewEnvVar,"=");
            if(bFlag == FALSE)
            {
                ret = putenv(pcnewEnvVar);
            }
            else
            {
                pcLocation = pcnewEnvVar;
            }
        }
        else
        {
            int ret ;
            pcnewEnvVar = malloc(len+strlen(Command_List->args[2])+2);
            memset(pcnewEnvVar,0,len+2);
            strcpy(pcnewEnvVar,Command_List->args[1]);
            strcat(pcnewEnvVar,"=");
            strcat(pcnewEnvVar,Command_List->args[2]);
            if(bFlag == FALSE)
            {
                ret = putenv(pcnewEnvVar);
            }
            else
            {
                pcLocation = pcnewEnvVar;
            }
        }
    }
    else
    {
        printf("setenv: Too many arguments.\n");
    }
}
void run_command_setenv(Cmd Command_List)
{
    if(NULL!= Command_List->next || Command_List->out != Tnil)
    {
        run_child(Command_List,e_setenv);
    }
    else
    {
        exe_setenv(Command_List);
    }
}

void exe_unsetenv(Cmd Command_List)
{
    if(Command_List->nargs == 1)
        printf("unsetenv: Too few arguments.\n");
    else
    {
#if 0
        int len = strlen(Command_List->args[1]);
        BOOL bFlag = FALSE;
        //char **ppcTmp = ppcEnv;
        char *pcnewEnvVar = NULL;
        char *pcLocation = NULL;
        char **ppcTmp = environ;
        
        int len = strlen(Command_List->args[1]);

        while(*ppcTmp!=NULL)
        {
            if((strcmp(Command_List->args[1],*ppcTmp) ==0))
            {
                bFlag = TRUE;
                pcLocation = *ppcTmp;
                break;
            }
            if(!bFlag)
                *ppcTmp++;
        }
#endif
        unsetenv(Command_List->args[1]);
    }
    
}

void run_command_unsetenv(Cmd Command_List)
{
    if(NULL!= Command_List->next || Command_List->out != Tnil)
    {
        run_child(Command_List,e_unsetenv);
    }
    else
    {
        exe_unsetenv(Command_List);
    }
}

void run_command_logout(Cmd CommandList)
{
    exit(0);
}
void exe_pwd()
{
    char cwd[1024]={0};
    getcwd(cwd,sizeof(cwd));
    printf("%s\n",cwd);
}

void run_command_pwd(Cmd CommandList)
{
    if(CommandList->nargs > 1)
        printf("pwd: ignoring non-option arguments\n");
    
    if(NULL!= CommandList->next || CommandList->out != Tnil)
    {
        run_child(CommandList,e_pwd);
    }
    else
    { 
        exe_pwd();
    }
}

void run_command_other(Cmd Command_List/*,char *pcCommandPath*/)
{
    pipe(s_Pipe);    
	int a;
    //int fd ;
    //int file_Descriptor;
    pid_t processID = fork();
    
    /*
     * processID = 0 --> Child
     * processID > 0 --> Parent
     */
    if(0 == processID) 
    {
        char str[200]={0};
        LOG("Making for Parent");
        LOG("FD = %d",file_Descriptor1);
#if 0
        if(FALSE == exe_where_new(Command_List))
        {
          printf("%s. Command not found.\n",Command_List->args[0]);
          dup2(originailSTDIN,STDIN_FILENO);
          dup2(originalSTDOUT,STDOUT_FILENO);
          dup2(originalSTDERR,STDERR_FILENO);
          close(file_Descriptor1);
          clearerr(stdout);
          clearerr(stderr);
          *endProcess = 1;
          _exit(0);
        }
#endif   
        dup2(file_Descriptor1,STDIN_FILENO);
        
        if(Command_List->in == Tin)// && NULL == Command_List->next)
        {
            LOG("Command_List->in == Tin");
            int fd = open(Command_List->infile, O_RDONLY, PERMISSIONS);
            if(fd < 0)
            {
                perror(Command_List->infile);
                //printf("Error");
                _exit(0);
            }
            else
            {
                dup2(fd,STDIN_FILENO);
                close(fd);
            }
        }
        
        if(Command_List->next && (Command_List->next->in == Tpipe ||\
                            Command_List->next->in == TpipeErr))
        {
            LOG("Making for child");
            dup2(s_Pipe[1],STDOUT_FILENO);
            if( Command_List->next->in == TpipeErr)
                dup2(s_Pipe[1],STDERR_FILENO);
            
        }
        
        if( Command_List->next == NULL &&(Command_List->out == Tout || Command_List->out == ToutErr))//&& NULL == Command_List->next )
        {
            LOG("Command_List->out = Tout");
            int fd = open(Command_List->outfile,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR,PERMISSIONS);
            if(fd < 0)
            {    
                LOG("Could not create directory");
                perror(Command_List->outfile);
                _exit(0);
            }
            dup2(fd,STDOUT_FILENO);
            if(Command_List->out == ToutErr)
            {
                LOG("ERROR CASE Touterr");
                dup2(fd,STDERR_FILENO);
            }
            close(fd);
            
        }
        
        
        
        else if(Command_List->next == NULL && (Command_List->out == Tapp || Command_List->out == TappErr))
        {
            int fd = open(Command_List->outfile,O_WRONLY|O_APPEND|O_CREAT, PERMISSIONS);
            if(fd < 0)
            {    
                LOG("Could not create directory");
                perror(Command_List->outfile);
                _exit(0);
            }
            dup2(fd,STDOUT_FILENO);
            if(Command_List->out == TappErr)
                dup2(fd,STDERR_FILENO);
            close(fd);
        }
        
                   
            //execvp(Command_List->args[0],Command_List->args);
            
        
#if 0        
        else if(Command_List->in == Tpipe)
        {
            LOG("Child Reading From SysPipe");
            close(s_Pipe[1]);
            dup2(s_Pipe[0],STDIN_FILENO);
            execv("/usr/bin/wc",Command_List->args);
            LOG("Execv Failed");
            perror("Execv: ");
        }
        
        else if(Command_List->next && Command_List->next->in == Tpipe)
        {
            LOG("Writing to SysPipe From Parent");
            close(s_Pipe[0]);
            dup2(s_Pipe[1],STDOUT_FILENO);
            close(s_Pipe[1]);
            execv("/bin/ls",Command_List->args);
            LOG("Execv Failed");
            perror("Execv: ");
        }
#endif
        //else
        {
            LOG("Executing Single Command");
            LOG("Command: %s",Command_List->args[0]);
            LOG("Outfile: %s",Command_List->outfile);
            //execv("/bin/ls",Command_List->args);
            execvp(Command_List->args[0],Command_List->args);
        fflush(stdout);    
#if 1
        if(FALSE == exe_where_new(Command_List))
        {
          printf("Command not found\n",Command_List->args[0]);
          dup2(originailSTDIN,STDIN_FILENO);
          dup2(originalSTDOUT,STDOUT_FILENO);
          dup2(originalSTDERR,STDERR_FILENO);
          close(file_Descriptor1);
          clearerr(stdout);
          clearerr(stderr);
          *endProcess = 1;
          _exit(0);
        }
#endif  	
	perror(Command_List->args[0]);
            //_exit(0);
            fflush(stdout);
            LOG("EXCEVP FAILED1");
	    exit(EXIT_FAILURE);
            LOG("Execvp Failed2");
        }
    }
    else if(processID > 0)
    {
        LOG("Parent Waiting");
        //wait();
        waitpid(processID,&a,0);
        LOG("Parent Waiting Over");
#if 0
        if(Command_List->next)
        {
            if(Command_List->next->in == Tpipe)
            {
                close(s_Pipe[1]);
            }
        }
#endif
        
        close(s_Pipe[1]);
        file_Descriptor1 = s_Pipe[0];
        LOG("FD = %d",file_Descriptor1);
    }
    else
    {
        LOG("Fork Failed");
        perror("Fork: ");
        exit(1);
    }
}

void (*run_Commands[e_end])(Cmd Command_List)={
    NULL,
    run_command_cd,
    run_command_echo,
    run_command_logout,
    run_command_pwd,
    run_command_other,
    run_command_where,
    run_command_setenv,
    run_command_unsetenv,
    run_command_nice,
    NULL};

e_List_of_Commands get_CommandType(char *Command )//,char **ppcPath)
{
    BOOL bflag = FALSE;
    
    if(strcmp(Command,"cd") == 0)
        return e_cd;
    else if(strcmp(Command,"logout")==0)
        return e_logout;
    else if(strcmp(Command,"echo") == 0)
        return e_echo;
    else if(strcmp(Command,"pwd")==0)
        return e_pwd;
    else if(strcmp(Command,"where")==0)
        return e_where;
    else if(strcmp(Command,"setenv")==0)
        return e_setenv;
    else if(strcmp(Command,"unsetenv")==0)
        return e_unsetenv;
    else if(strcmp(Command,"nice")==0)
        return e_nice;
    /*
     * Write Check to see if command exists in /bin folder
     * If command does not exist return e_end
     */
#if 0 
    /*Check whether Command exists in Environment Variable Paths*/
    bflag = get_CommandPath(Command);//,&pcPath);
    //*ppcPath = pcPath;
    
    if(FALSE == bflag)
    {
        return e_end;
    }
#endif
    return e_other;
}

void ExecuteCommand(Cmd Command_List,int flag)
{
    e_List_of_Commands e_command;
    char *pc_CommandPath = NULL;
    
    e_command = get_CommandType(Command_List->args[0]);//, &pc_CommandPath);
    
    if(strcmp(Command_List->args[0],"end")==0&& flag == 1)
        goto end;
    if(strcmp(Command_List->args[0],"end")==0&& flag == 0)
        exit(0);
    if(e_command == e_end)
    {
        /*Handle Error Case*/
    }
    else //if(e_command >= e_start && e_command < e_end)
    {
        run_Commands[e_command](Command_List);
    }
end:;
}
void processPipe(Pipe pipe_Input,int flag)
{
    Cmd Command_List;
    
    if(NULL == pipe_Input)
        return;
   
    do
    {
        for(Command_List = pipe_Input->head ; NULL != Command_List ; \
                                        Command_List = Command_List->next)
        {
//            fflush(stdout);
	    if ( !strcmp(Command_List->args[0], "end") )
                exit(0);
            ExecuteCommand(Command_List,flag);
            if(*endProcess == 1)
                break;
        }
        *endProcess = 0;
        pipe_Input = pipe_Input->next;
	printf("sfdf");
    }while(pipe_Input);
 }

void exe_nice(Cmd Command_List)
{
    pid_t process_id;
    int which = PRIO_PROCESS;
    
    process_id = getpid();
    if(Command_List->nargs == 1)
        setpriority(which,0,4);
    else if(isdigit(Command_List->args[1][0])) 
    {
        int priority = atoi(Command_List->args[1]);
        if(priority < -19)
            priority = -19;
        else if(priority>20)
            priority =20;
        
        if(Command_List->args[2]== NULL)
            setpriority(which,0,priority);
        else if(Command_List->args[2])
        {
            pid_t pid ;
            int i;
            Cmd temp = malloc(sizeof(struct cmd_t));
            temp->args = malloc((Command_List->nargs -2) * sizeof(char*));
            for(i=2;i<Command_List->nargs;i++)
                temp->args[i-2] = strdup(Command_List->args[i]);
            temp->args[i-2]=NULL;
            temp->in = Command_List->in;
            temp->exec = Command_List->exec;
            if(Command_List->infile)
                temp->infile = strdup(Command_List->infile);
            else
                temp->infile = NULL;
            temp->maxargs = Command_List->maxargs;
            temp->nargs = Command_List->nargs -2;
            temp->out = Command_List->out;
            if(Command_List->outfile)
                temp->outfile = strdup(Command_List->outfile);
            else
                temp->outfile = NULL;
            temp->next = Command_List->next;
             
             if(get_CommandType(Command_List->args[2])!=e_other)
             {
                 /*Built in Command*/
                 setpriority(which,0,priority);
                 execute_command[get_CommandType(Command_List->args[2])](temp);                 
                 
             }
             else
             {
                 /* Child Process*/
                 pid_t pChildProcess;
                 
                 pChildProcess = fork();
                 if(pChildProcess == 0)
                 {
                     setpriority(which,0,priority);
                     run_command_other(temp);
                 }
                 else if(pChildProcess>0)
                 {
                     wait();
                 }
            }
            if(temp->infile)
                free(temp->infile);
            if(temp->outfile)
                free(temp->outfile);
            if(temp->args)
            {
                //for(i=2;i<Command_List->nargs;i++)
                  //  free(temp->args[i-2]);
                //free(temp->args);
            }
            free(temp);
        }
        
    }
    
}
void run_command_nice(Cmd Command_List)
{
    if(NULL!=Command_List->next || Command_List->out != Tnil)
    {
        run_child(Command_List,e_nice);
    }
    else
    {
        exe_nice(Command_List);
    }
}

int main(int argc, char **argv)//,char **envp)
{
    Pipe pipe_Input;
    int num = 1;
    char *pc_env;
    int ushrc_fd ;
    int backupSTDIN;
    char hostname[1024]={0};
    char *pcHomeDir;
    char homepath[1024]={0};
    
    gethostname(hostname,sizeof(hostname));
    pcHomeDir = getenv("HOME");
    strcpy(homepath,pcHomeDir);
    strcat(homepath,"/.ushrc");
    //ppcEnv = envp;
    endProcess = mmap(NULL, sizeof *endProcess, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *endProcess = 0;
    if(OS_TEST)
    {
        remove("ushell_trace.txt");
    }
    
    originailSTDIN = dup(STDIN_FILENO); 
    originalSTDOUT = dup(STDOUT_FILENO);
    originalSTDERR = dup(STDERR_FILENO);
    
    signal(SIGINT,signal_int_handler);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    
    pc_env = getenv("PATH");
    gpc_EnvironmentVariables = strdup(pc_env);
#if 1
    ushrc_fd = open(homepath,O_RDONLY);
    if(ushrc_fd == -1)
    {
	perror("ushrc: ");
    }
    else
    {
        backupSTDIN = dup(STDIN_FILENO);
        dup2(ushrc_fd,STDIN_FILENO);
        close(ushrc_fd);
        
        pipe_Input = parse();
        processPipe(pipe_Input,1);
        freePipe(pipe_Input);
        
        dup2(backupSTDIN,STDIN_FILENO);
	dup2(originalSTDOUT,STDOUT_FILENO);
	close(originalSTDOUT);
        close(backupSTDIN);
    }
#endif
    //pipe(s_Pipe);
       // perror("PIPE Error: ");
#if 1
    while(TRUE)
    {
        printf("%%");
        fflush(stdout);
        pipe_Input = parse();
        processPipe(pipe_Input,0);
        freePipe(pipe_Input);
        file_Descriptor1 =0;
	dup2(originailSTDIN,STDIN_FILENO);
    }
#endif
}
