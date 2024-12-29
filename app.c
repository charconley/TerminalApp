#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

/*
Implementation of a unix-based terminal app in c, based on Stephen Brennen's article, https://brennan.io/2015/01/16/write-a-shell-in-c/
As of now, no options for any command have been implemented.

GOALS
-Create a simple shell for file creation
-Create a vi clone to run in the simple shell

Charlie Conley, 12/27/24

CHANGELOG,

12/27/24
Added funcitonality for mkdir command
Added functionality for "touch" commmand
Added qouting for creating files and folders with spaces - see func FindBetweenQoute and

12/29/24
Added functionality for "grep", "rmdir", "pwd" commands
*/

#define LSH_RL_BUFSIZE 1024
char *lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar();

        if (c == EOF || c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
        {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize)
        {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

// grab qoute to replace args[1]
char *findBetweenQoute(char *line)
{
    char *target = NULL;
    char *start, *end;
    if (strstr(line, "\"") != NULL)
    {
        start = strstr(line, "\"");
        start += strlen("\"");
        if (strstr(start, "\"") != NULL)
        {
            end = strstr(start, "\"");
            target = (char *)malloc(end - start + 1);
            memcpy(target, start, end - start);
            target[end - start] = '\0';
        }
    }

    if (target)
    {
        return target;
    }
    else
    {
        return NULL;
    }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *grabQuote = findBetweenQoute(line);
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    if (grabQuote != NULL)
    {
        if (tokens[0][0] == 'g')
        {
            tokens[2] = grabQuote;
        }
        else
        {
            tokens[1] = grabQuote;
        }
    }
    return tokens;
}

int lsh_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
        {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("lsh");
    }
    else
    {
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int lsh_cd(char **args);
int lsh_mkdir(char **args);
int lsh_touch(char **args);
int lsh_grep(char **args);
int lsh_rmdir(char **args);
int lsh_pwd();
int lsh_help(char **args);
int lsh_exit(char **args);

char *builtin_str[] = {
    "cd",
    "mkdir",
    "touch",
    "grep",
    "rmdir",
    "pwd",
    "help",
    "exit"};

int (*builtin_func[])(char **) = {
    &lsh_cd,
    &lsh_mkdir,
    &lsh_touch,
    &lsh_grep,
    &lsh_rmdir,
    &lsh_pwd,
    &lsh_help,
    &lsh_exit};

int lsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) == -1)
        {
            perror("lsh");
        }
    }
    return 1;
}

int lsh_mkdir(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "lsh: expected argument to \"mkdir\"\n");
    }
    else
    {
        if (mkdir(args[1], 0755) == -1)
        {
            perror("lsh");
        }
    }
    return 1;
}

int lsh_touch(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "lsh: expected argument to \"touch\"\n");
    }
    else
    {
        int fd = open(args[1], O_WRONLY | O_CREAT | O_EXCL, 0644);

        if (fd == -1)
        {
            perror("lsh");
        }
        close(fd);
    }
    return 1;
}

int lsh_grep(char **args)
{
    if (args[1] == NULL || args[2] == NULL)
    {
        fprintf(stderr, "lsh: expected argument to \"grep\"\n");
    }
    else
    {
        FILE *fp;
        char line[1024];
        fp = fopen(args[2], "r");
        if (fp == NULL)
        {
            perror("Error opening file");
            return EXIT_FAILURE;
        }

        while (fgets(line, sizeof(line), fp) != NULL)
        {
            if (strstr(line, args[1]) != NULL)
            {
                printf("%s", line);
            }
        }

        fclose(fp);
    }
    return 1;
}

int lsh_rmdir(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "lsh: expected argument to \"rmdir\"\n");
    }
    else
    {
        if (rmdir(args[1]) == -1)
        {
            perror("lsh");
        }
    }
    return 1;
}

int lsh_pwd()
{
    char pwd[1024];
    getcwd(pwd, sizeof(pwd));
    printf("%s\n", pwd);
    return 1;
}

int lsh_help(char **args)
{
    int i;
    printf("Charlie Conley's LSH\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < lsh_num_builtins(); i++)
    {
        printf(" %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int lsh_exit(char **args) { return 0; }

int lsh_execute(char **args)
{
    int i;
    if (args[0] == NULL)
    {
        return 1;
    }

    for (i = 0; i < lsh_num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(args);
        }
    }
    return lsh_launch(args);
}

void lsh_loop(void)
{
    char *line;
    char **args;
    int status;

    do
    {
        printf("> ");
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv)
{
    // command loop
    lsh_loop();

    return EXIT_SUCCESS;
}
