#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>


#define KB 1024
#define MB 1048576
#define TABLE_SIZE 4096 /* Fat table size (block number) */
#define TABLE_ROW_LEN 4 /* Fat table row length */
#define LINE_LEN 128    /* Super block line len*/
#define SUPER_LINE 7


/* Directory entry byte numbers */
#define FILE_NAME 13
#define EXTENSION 1
#define DATE 10
#define TIME 8
#define ADDRESS 4
#define SIZE 4
#define E_SIZE 41
#define ADD_IN 36
#define SIZE_IN 32

FILE *disk;             /* Disk file */
int block_size;         /* Block size */
int table_bl_len = 0;   /* Block number for fat table */
int root_dir_add = 0;   /* Root directory block */
int dir_add = 0 ;       /* Directories initial block */
int free_table_add =0 ; /* Free fat table initial block */
int table_add = 0;      /* Fat table initial block */


void error_exit();
void read_info();
int get_num(char buff[]);
int mkdir(char path[]);
void get_date_time(char ttime[]);
void free_src(void *source);
char *create_dir_entry(char name[],int size,int bl_add,int ext);
int find_free_block();
int check_block(char *name,int add);
int get_table_val(int in,int t_num);
void set_table_val(int in,int val,int t_num);
void record_file_entry(int add,char data[]);
char *read_file(char *file_name,int *size);
int f_write(char *path,char *file_name);
char **tokenize_path(char *path,int *path_rate);
void write_disk(int add,char *data,int size);
int check_block_for_file(char *name,int add,int *size);
int f_read(char *path,char *file_name);
void update_dir_num(int ch,int val);
void dumpe2fs();
void f_dir(char *path);
int f_del(char *path);
void remove_name(char *name,int add);
void add_free_block(int add);

int main(int argc,char *argv[])
{

    if(argv[1]==NULL)
    {
        printf("Invalid parameter\n");
        exit(EXIT_FAILURE);
    }
    disk= fopen(argv[1],"r+");
    if(disk==NULL)
    {
        printf("Invalid disk file\n");
        exit(EXIT_FAILURE);
    }

    read_info();
    block_size++;


    if(argv[2]==NULL)
    {
        printf("Invalid argument \n");
        fclose(disk);
        exit(EXIT_FAILURE);
    }
    int fl=0;

    if(strcmp(argv[2],"mkdir")==0)
    {
        if(argv[3]!=NULL)
            mkdir(argv[3]);
        else
            fl=1;
    }
    else if(strcmp(argv[2],"read")==0)
    {
        if(argv[3]!=NULL && argv[4]!=NULL)
            f_read(argv[3],argv[4]);
        else
            fl=1;
    }

    else if(strcmp(argv[2],"write")==0)
    {
        if(argv[3]!=NULL && argv[4]!=NULL)
            f_write(argv[3],argv[4]);
        else
            fl=1;
    }

    else if(strcmp(argv[2],"dumpe2fs")==0)
        dumpe2fs();
    else if(strcmp(argv[2],"dir")==0)
    {
        if(argv[3]!=NULL)
            f_dir(argv[3]);
        else
            fl=1;
    }

    else if(strcmp(argv[2],"del")==0)
    {
        if(argv[3]!=NULL)
            f_del(argv[3]);
        else
            fl=1;
    }
    else
        printf("Inavlid operation\n");

    if(fl)
        printf("Invalid argument \n");

    fclose(disk);
    return 0;
}



/**
*   This function deletes the file
*/
int f_del(char *path)
{
    int path_rate=0;
    char **div_path=tokenize_path(path,&path_rate);
    int now = root_dir_add,next=0,i;

    for(i=0; i<path_rate; i++)
    {

        next = check_block(div_path[i],now);

        if(next!=-1 && i==path_rate-1)
        {


            int f_size=0;

            if(check_block_for_file(div_path[i],now,&f_size)==-2)
            {
                printf("This is not a file\n");
                return -1;
            }

            int j=0;
            remove_name(div_path[i],now);
            now = get_table_val(next,1);
            set_table_val(next,0,1);
            add_free_block(next);
            fseek(disk,next*block_size,SEEK_SET);
            next=now;
            for(j=0;j<f_size-1;j++)
            {

                if(j!=0 && (j%(block_size-1)==0))
                {

                    next=get_table_val(now,1);
                    set_table_val(now,0,1);
                    add_free_block(now);
                    fseek(disk,now*block_size,SEEK_SET);
                    now=next;

                }

                fprintf(disk,"0");
            }
            return -1;


        }
        if(next==-1){
            printf("Wrong path\n");
            break;
        }

        now=next;

    }

    for(i=0; i<path_rate; i++)
        free_src(div_path[i]);

    free_src(div_path);
    return 0;
}

/**
*   This function deletes file name from parent directory
*/
void remove_name(char *name,int add)
{
    int in = add*block_size;
    int i;
    char buff[block_size];
    memset(buff,'\0',block_size);
    fseek(disk,in,SEEK_SET);
    fgets(buff,block_size,disk);
    // printf("buff :  -%s-  -%s- \n",name,buff);
    int fl=0;
    int next =0,now =add;

    while(1)
    {

        next = get_table_val(now,1);

        if(fl)
        {
            fseek(disk,now*block_size,SEEK_SET);
            memset(buff,'\0',block_size);
            fgets(buff,block_size,disk);

        }

        fl=1;

        for(i=0; i<block_size-1-E_SIZE; i+=E_SIZE-1)
        {
            if(buff[i]!='0')
            {
                int j;
                int fl2=1;
                int j1=0;

                for(j=i; j<i+strlen(name); j++)
                {
                    if(buff[j]!=name[j1])
                    {
                        fl2=0;
                        break;
                    }

                    j1++;
                }

                if(fl2)
                {
                    for(j=i; j<i+E_SIZE-1; j++)
                    {
                        buff[j]='0';

                    }
                   fseek(disk,now*block_size,SEEK_SET);
                    fprintf(disk,"%s",buff);
                    update_dir_num(0,-1);
                    return;
                }
            }
        }


        if(next==-1)
            return ;

        now=next;



    }




}



/**
*   This function adds free block to the free fat table
*/
void add_free_block(int add)
{
     int i=0,i2;


    for(i=0; i<table_bl_len; i++)
    {
        int in = (free_table_add+i) * block_size;
        fseek(disk,in,SEEK_SET);

        for(i2=0; i2<block_size-1-TABLE_ROW_LEN; i2+=TABLE_ROW_LEN)
        {

            int j=0,bl_val;
            char buff[TABLE_ROW_LEN];
            memset(buff,'\0',TABLE_ROW_LEN);

            while(j<4)
            {
                fread(buff+j, 1, 1, disk);
                j++;
            }

            bl_val=atoi(buff);

            // printf("val : %d,  -%s-\n",bl_val,buff);
            if(bl_val<0)
            {
                fseek(disk,in+i2,SEEK_SET);
                fprintf(disk,"%.4d",add);
                return ;
            }
        }

    }

}


/**
*   This function lists the contents of the root directory
*/
void f_dir(char *path)
{
    int path_rate=0;
    char **div_path=tokenize_path(path,&path_rate);
    int now = root_dir_add,next=0,i;

    for(i=0; i<path_rate; i++)
    {
        next = check_block(div_path[i],now);
        if(next!=-1 && i==path_rate-1)
        {

            int in = next*block_size;
            char buff[block_size];
            memset(buff,'\0',block_size);
            fseek(disk,in,SEEK_SET);
            fgets(buff,block_size,disk);
            int fl=0;
            int tnext =0,tnow =now;


            while(1)
            {

                tnext = get_table_val(tnow,1);

                if(fl)
                {
                    fseek(disk,now*block_size,SEEK_SET);
                    memset(buff,'\0',block_size);
                    fgets(buff,block_size,disk);

                }

                fl=1;
                int i1;
                for(i1=0; i1<block_size-1-E_SIZE; i1+=E_SIZE-1)
                {
                    if(buff[i1]!='0')
                    {
                        int j;


                        for(j=i1; j<i1+FILE_NAME; j++)
                        {
                            printf("%c",buff[j]);

                        }
                        printf("\n");
                    }
                }


        if(tnext==-1)
           break;

        tnow=tnext;



    }



        }
        if(next==-1){
            printf("Wrong path\n");
            break;
        }
        now=next;

    }
    for(i=0; i<path_rate; i++)
        free_src(div_path[i]);

    free_src(div_path);

}


/**
*   This function writes the file system information
*/
void dumpe2fs()
{

    fseek(disk,0,SEEK_SET);
    char buff[block_size];
    memset(buff,'\0',block_size);
    fread(&buff,1,block_size,disk);
    int i=0,j=0;
    while(1)
    {

        printf("%c",buff[i]);
        if(buff[i]=='\n')
            j++;
        if(j==8)
            break;
        i++;

    }
}


/**
*   This function updates fat and free table
*/
void update_dir_num(int ch,int val)
{
    fseek(disk,0,SEEK_SET);
    char buff[block_size];
    memset(buff,'\0',block_size);
    fread(&buff,1,block_size,disk);
    int i;
    int j=0;
    for(i=0;i<block_size;i++)
    {
        if(j==7)
            break;
        if(buff[i]=='\n')
            j++;
    }
    fseek(disk,i,SEEK_SET);
    char number[SIZE];
    memset(number,'\0',SIZE);

    if(ch){
        for(;i<block_size;i++)
        {
            if(buff[i]==':'){
                i+=2;
                fseek(disk,i,SEEK_SET);
                fread(&number,1,SIZE,disk);
                int t = atoi(number);
                t+=val;
                fseek(disk,i,SEEK_SET);
                fprintf(disk,"%.4d",t);
                fseek(disk,i+28,SEEK_SET);
                fread(&number,1,SIZE,disk);
                t = atoi(number);
                t-=val;
                fseek(disk,i+28,SEEK_SET);
                fprintf(disk,"%.4d",t);
                break;
            }
        }
    }
    else{
        int fl=0;
        for(;i<block_size;i++)
        {
            if(buff[i]==':'){
                fl++;
            }
            if(fl==2){
                i+=2;
                fseek(disk,i,SEEK_SET);
                fread(&number,1,SIZE,disk);
                int t = atoi(number);
                t+=val;
                fseek(disk,i,SEEK_SET);
                fprintf(disk,"%.4d",t);
                fseek(disk,i+14,SEEK_SET);
                fread(&number,1,SIZE,disk);
                t = atoi(number);
                t-=val;
                fseek(disk,i+14,SEEK_SET);
                fprintf(disk,"%.4d",t);
                break;
            }
        }
    }


}

/**
*   This function read file content from disk and writes to given file
*/
int f_read(char *path,char *file_name)
{

    int path_rate=0;
    char **div_path=tokenize_path(path,&path_rate);
    int now = root_dir_add,next=0,i;

    for(i=0; i<path_rate; i++)
    {

        next = check_block(div_path[i],now);

        if(next!=-1 && i==path_rate-1)
        {


            int f_size=0;

            if(check_block_for_file(div_path[i],now,&f_size)==-2)
            {
                printf("This is not a file\n");
                return -1;
            }




            char f_cont[f_size];
            memset(f_cont,'\0',f_size);
            int j=0,n_add=next;
            //int fl=0;
            fseek(disk,next*block_size,SEEK_SET);
            for(j=0;j<f_size-1;j++)
            {

                if(j!=0 && (j%(block_size-1)==0))
                {
                    n_add=get_table_val(n_add,1);
                    fseek(disk,n_add*block_size,SEEK_SET);

                }

                fread(f_cont+j, 1, 1, disk);

            }




            FILE *t_file;
            t_file= fopen(file_name,"w+");
            fprintf(t_file,"%s",f_cont);

            fclose(t_file);




        }
        if(next==-1){
            printf("Wrong path\n");
            break;
        }

        now=next;

    }

    for(i=0; i<path_rate; i++)
        free_src(div_path[i]);

    free_src(div_path);
    return 0;

}


/**
*   This function reads given file content and create a file in file system then writes content.
*/
int f_write(char *path,char *file_name)
{

    int path_rate=0;
    char **div_path=tokenize_path(path,&path_rate);
    int n_add=-1;
    int i;
    int now = root_dir_add,next=0;

    for(i=0; i<path_rate; i++)
    {
        next = check_block(div_path[i],now);

        if(i==path_rate-1)
        {
            int f_size=0;

            if(check_block_for_file(div_path[i],now,&f_size)>0)
            {
                printf("File alrady exist\n");

                for(i=0; i<path_rate; i++)
                    free_src(div_path[i]);

                free_src(div_path);
                return -1;

            }
            f_size=0;
            char *f_cont=read_file(file_name,&f_size);
            if(f_cont!=NULL){
                n_add = find_free_block();
                if(n_add<=0)
                {
                    for(i=0; i<path_rate; i++)
                        free_src(div_path[i]);

                    free_src(div_path);
                    return -1;
                }

                char *dir_ent =create_dir_entry(div_path[i],f_size,n_add,0);
                record_file_entry(now,dir_ent);
                write_disk(n_add,f_cont,f_size);
                free_src(dir_ent);
                update_dir_num(0,1);
            }

            free_src(f_cont);

            break;

        }

        if(next==-1)
        {
            printf("Wrong path\n");
            break;
        }

        now=next;



    }

    for(i=0; i<path_rate; i++)
        free_src(div_path[i]);

    free_src(div_path);

    return 0;
}

/**
*   This function writes the file content to given address in disk
*/
void write_disk(int add,char *data,int size)
{
    fseek(disk,add*block_size,SEEK_SET);
    int i;
    if(size<block_size-1)
    {

        fprintf(disk,"%s",data);
        set_table_val(add,-1,1);
    }
    else
    {

        fseek(disk,add*block_size,SEEK_SET);
        int n_add=0;
        int now=add;

        for(i=0; i<size-1; i++)
        {
            if(i!=0 && (i%(block_size-1)==0))
            {
                n_add=find_free_block();
                set_table_val(now,n_add,1);
                set_table_val(n_add,-1,1);
                now=n_add;
                fseek(disk,n_add*block_size,SEEK_SET);
            }

            fprintf(disk,"%c",data[i]);
        }
    }

}


/**
*   This function reads the file for record the system
*/
char *read_file(char *file_name,int *size)
{

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    int fd = open(file_name, O_RDONLY, mode);

    if (fd == -1){
        printf("Wrong input file\n");
        return NULL;
    }


    size_t bytes_read, offset = 0;
    char c[1];
    memset(c, '\0', 1);
    *size= lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char *buff =(char*)malloc(*size);
    memset(buff,'\0',*size);
    int i=0;

    do
    {
        bytes_read = read(fd, c, 1);
        offset += bytes_read;
        buff[i]=c[0];
        i++;
    }
    while (offset < *size-1);

    close(fd);

    return buff;

}


/**
*   This function parses the path
*/
char **tokenize_path(char *path,int *path_rate)
{
    int i;
    *path_rate=0;

    for(i=0; i<strlen(path); i++)
    {
        if(path[i]=='/')
            *path_rate=*path_rate+1;

    }


    char **div_path =(char **)malloc(sizeof(char*)*(*path_rate));

    for(i=0; i<*path_rate; i++)
    {
        div_path[i]=(char *)malloc(FILE_NAME);
        memset(div_path[i],'\0',FILE_NAME);
    }

    char temp[strlen(path)];
    memset(temp,'\0',strlen(path));
    strcpy(temp,path);
    char* token = strtok(temp, "/");
    i=0;

    while (token != NULL)
    {
        strcpy(div_path[i],token);
        token = strtok(NULL, "/");
        i++;
    }

    return div_path;


}


/**
*   This function creates new directory
*/
int mkdir(char path[])
{

    if(path==NULL)
        error_exit();

    fseek(disk,root_dir_add*block_size,SEEK_SET);


    int i;
    int path_rate=0;
    char **div_path = tokenize_path(path,&path_rate);
    int n_add=-1;
    int fl=0;
    if(path_rate==1) // File will add under the roor directory
    {

        n_add = find_free_block();

        if(n_add<=0)
        {
            for(i=0; i<path_rate; i++)
                free_src(div_path[i]);

            free_src(div_path);
            printf("There is not free block space in disk\n");
            return -1;
        }

        else{

            char *dir_ent =create_dir_entry(div_path[0],1,n_add,1);
            set_table_val(n_add,-1,1);
            record_file_entry(root_dir_add,dir_ent);
            fl=1;
            free_src(dir_ent);
            update_dir_num(1,1);
        }

    }
    else
    {


        int now = root_dir_add,next=0;

        for(i=0; i<path_rate; i++)
        {

            next = check_block(div_path[i],now);

            if(next==-1 && i==path_rate-1)
            {

                n_add = find_free_block();

                if(n_add<=0)
                {
                    for(i=0; i<path_rate; i++)
                        free_src(div_path[i]);

                    free_src(div_path);
                    return -1;
                }
                fl=1;
                set_table_val(n_add,-1,1);
                char *dir_ent =create_dir_entry(div_path[i],1,n_add,1);
                record_file_entry(now,dir_ent);
                free_src(dir_ent);
                update_dir_num(1,1);

            }
            else if(next==-1)
                {
                    printf("Wrong directory \n");
                    fl=1;
                    break;
                }

            now=next;

        }
    }

    if(!fl)
        printf("Directory already exist\n");
    for(i=0; i<path_rate; i++)
        free_src(div_path[i]);

    free_src(div_path);

    return 0;
}


/**
*   This function checks if the file is in the block
*/
int check_block_for_file(char *name,int add,int *size)
{
    int in = add*block_size;
    int i;
    char buff[block_size];
    memset(buff,'\0',block_size);
    fseek(disk,in,SEEK_SET);
    fgets(buff,block_size,disk);
    int fl=0;
    int next =0,now =add;
    int res=-1;
    while(1)
    {

        next = get_table_val(now,1);

        if(fl)
        {
            fseek(disk,now*block_size,SEEK_SET);
            memset(buff,'\0',block_size);
            fgets(buff,block_size,disk);

        }

        fl=1;

        for(i=0; i<block_size-1-E_SIZE; i+=E_SIZE-1)
        {
            if(buff[i]!='0')
            {
                int j;
                int fl2=1;
                int j1=0;

                for(j=i; j<i+strlen(name); j++)
                {
                    if(buff[j]!=name[j1])
                    {
                        fl2=0;
                        break;
                    }

                    j1++;
                }

                if(fl2)
                {

                    if(buff[i+FILE_NAME]=='1') // This is a directory
                    {
                        res=-2;
                        continue;
                    }

                    int j3=0;
                    char t_size[SIZE];

                    j=0;

                    for(j1=i+SIZE_IN; j1<i+SIZE_IN+SIZE; j1++)
                    {
                        t_size[j3]=buff[j1];
                        j3++;
                    }

                    *size=atoi(t_size);
                    return *size;

                }
            }
        }


        if(next==-1)
            return res;

        now=next;



    }


    return res;

}


/**
*   This function checks if the file is in the block then returns the required file address
*/
int check_block(char *name,int add)
{
    int in = add*block_size;
    int i;
    char buff[block_size];
    memset(buff,'\0',block_size);
    fseek(disk,in,SEEK_SET);
    fgets(buff,block_size,disk);
    int fl=0;
    int next =0,now =add;

    while(1)
    {

        next = get_table_val(now,1);

        if(fl)
        {
            fseek(disk,now*block_size,SEEK_SET);
            memset(buff,'\0',block_size);
            fgets(buff,block_size,disk);

        }

        fl=1;

        for(i=0; i<block_size-1-E_SIZE; i+=E_SIZE-1)
        {
            if(buff[i]!='0')
            {
                int j;
                int fl2=1;
                int j1=0;

                for(j=i; j<i+strlen(name); j++)
                {

                    if(buff[j]!=name[j1])
                    {
                        fl2=0;
                        break;
                    }

                    j1++;
                }

                if(fl2)
                {
                    char t_add[ADDRESS];
                    memset(t_add,'\0',ADDRESS);
                    int j3=0;

                    for(j1=i+ADD_IN; j1<i+ADD_IN+ADDRESS; j1++)
                    {
                        t_add[j3]=buff[j1];
                        j3++;
                    }

                    return atoi(t_add);

                }
            }
        }


        if(next==-1)
            return -1;

        now=next;



    }


    return -1;
}


/**
*   This function returns the given index value from table
*/
int get_table_val(int in,int t_num)
{
    int nl_num=(in*TABLE_ROW_LEN)/(block_size-1);

    int t_in;

    if(t_num) // fat table address
        t_in = (table_add*block_size)+(in*TABLE_ROW_LEN+nl_num);
    else    // free table address
        t_in = (free_table_add*block_size)+(in*TABLE_ROW_LEN+nl_num);

    fseek(disk,t_in,SEEK_SET);
    int j=0,bl_val=-1;
    char buff[TABLE_ROW_LEN];
    memset(buff,'\0',TABLE_ROW_LEN);

    while(j<TABLE_ROW_LEN)
    {
        fread(buff+j, 1, 1, disk);
        j++;
    }

    bl_val=atoi(buff);
    return bl_val;

}

/**
*   This function sets the required index value with given value
*/
void set_table_val(int in,int val,int t_num)
{

    int nl_num=(in*TABLE_ROW_LEN)/(block_size-1);
    int t_in;

    if(t_num) // fat table address
        t_in = (table_add*block_size)+(in*TABLE_ROW_LEN+nl_num);
    else    // free table address
        t_in = (free_table_add*block_size)+(in*TABLE_ROW_LEN+nl_num);

    fseek(disk,t_in,SEEK_SET);
    char buff[TABLE_ROW_LEN];

    if(val<0)
        sprintf(buff,"%.3d",val);
    else
        sprintf(buff,"%.4d",val);

    fprintf(disk,"%s",buff);
}


/**
*   This function records file entry bytes to the given block address
*/
void record_file_entry(int add,char data[])
{

    char buff[block_size];
    memset(buff,'\0',block_size);
    fseek(disk,add*block_size,SEEK_SET);
    fgets(buff,block_size,disk);
    int i;
    int fl=0,fl2=0;
    int next =0,now =add;

    while(1)
    {
        next = get_table_val(now,1);

        if(fl2)
        {
            fseek(disk,now*block_size,SEEK_SET);
            fgets(buff,block_size,disk);

        }

        fl2=1;

        for(i=0; i<block_size-1-E_SIZE; i+=E_SIZE-1)
        {
            if(buff[i]=='0')
            {
                fl=1;
                break;
            }
        }

        int j;

        if(fl)
        {
            for(j=0; j<E_SIZE-1; j++)
            {
                buff[i]=data[j];
                i++;
            }

            fseek(disk,now*block_size,SEEK_SET);
            fprintf(disk,"%s",buff);
            return;

        }
        else
            if(next == -1)
            {
                int n_add=find_free_block();
                set_table_val(now,n_add,1);
                set_table_val(n_add,-1,1);
                fseek(disk,n_add*block_size,SEEK_SET);
                fprintf(disk,"%s",data);
                return;
            }
            else
            {

                now=next;
            }


    }


}


/**
*   This function finds the free block
*/
int find_free_block()
{
    int i=0,i2;


    for(i=0; i<table_bl_len; i++)
    {
        int in = (free_table_add+i) * block_size;
        fseek(disk,in,SEEK_SET);

        for(i2=0; i2<block_size-1-TABLE_ROW_LEN; i2+=TABLE_ROW_LEN)
        {

            int j=0,bl_val;
            char buff[TABLE_ROW_LEN];
            memset(buff,'\0',TABLE_ROW_LEN);

            while(j<4)
            {
                fread(buff+j, 1, 1, disk);
                j++;
            }

            bl_val=atoi(buff);

            if(bl_val>0)
            {
                fseek(disk,in+i2,SEEK_SET);
                fprintf(disk,"%.3d",-2);
                return bl_val;
            }
        }

    }

    return -1;
}


/**
*   This function creates directory entry bytes
*/
char *create_dir_entry(char name[],int size,int bl_add,int ext)
{
    int i;

    char *entry = (char*)malloc(E_SIZE);
    memset(entry,'\0',E_SIZE);
    char tname[FILE_NAME];
    memset(tname,' ',FILE_NAME);

    for(i=0; i<strlen(name); i++)
        entry[i]=name[i];

    for(; i<FILE_NAME; i++)
        entry[i]=' ';

    if(ext)
        entry[i]='1';
    else
        entry[i]='0';

    char date[DATE+TIME];
    memset(date,'\0',DATE+TIME);
    get_date_time(date);
    strcat(entry,date);
    i=i+DATE+TIME+1;
    char csize [SIZE];
    memset(csize,'\0',SIZE);
    sprintf(csize,"%.4d",size);
    strcat(entry,csize);
    char add[ADDRESS];
    memset(add,'\0',ADDRESS);
    sprintf(add,"%.4d",bl_add);
    strcat(entry,add);


    return entry;
}

/**
*   This function frees a pointer
*/
void free_src(void *source)
{
    if (source == NULL)
        return;

    free(source);
}


/**
*   This function returns current date and time
*/
void get_date_time(char ttime[])
{

    int hours, minutes, seconds, day, month, year;

    time_t now;
    time(&now);

    struct tm *local = localtime(&now);

    hours = local->tm_hour;
    minutes = local->tm_min;
    seconds = local->tm_sec;

    day = local->tm_mday;
    month = local->tm_mon + 1;
    year = local->tm_year + 1900;

    char buff[18];
    memset(buff,'\0',18);

    sprintf(buff,"%.2d/%.2d/%.4d%.2d:%.2d:%.2d",day,month,year,hours,minutes,seconds);

    strcpy(ttime,buff);


}


void error_exit()
{
    printf("Exiting...\n");
    exit(EXIT_FAILURE);
}


/**
*   This function reads the super block informations
*/
void read_info()
{

    char line_buff[LINE_LEN];
    memset(line_buff,'\0',LINE_LEN);

    if(disk==NULL)
    {
        fprintf(stderr, "Error open file : %s\n", strerror(errno));
        error_exit();
    }

    int i;

    for(i=0; i<SUPER_LINE; i++)
    {
        memset(line_buff,'\0',LINE_LEN);
        fgets(line_buff,sizeof(line_buff),disk);

        switch(i)
        {
        case 1:
            block_size = get_num(line_buff);
            break;

        case 2:
            table_add = get_num(line_buff);
            break;

        case 3:
            free_table_add = get_num(line_buff);
            break;

        case 4:
            table_bl_len = get_num(line_buff);
            break;

        case 5:
            root_dir_add = get_num(line_buff);
            break;

        case 6:
            dir_add = get_num(line_buff);
            break;

        default:
            break;
        }

    }





}

/**
*   This function parses super block lines and converts it to integer
*/
int get_num(char buff[])
{
    int i;

    for(i=0; i<LINE_LEN; i++)
    {
        if(buff[i]==':')
            break;
    }

    i+=2;
    char temp[LINE_LEN];
    memset(temp,'\0',LINE_LEN);
    int j=0;

    for(; i<LINE_LEN; i++)
    {
        if(buff[i]=='.' || buff[i]=='\n')
            break;

        temp[j]=buff[i];
        j++;
    }

    return atoi(temp);

}
