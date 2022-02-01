#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#define KB 1024
#define MB 1048576
#define TABLE_SIZE 4096 /* Fat table size (block number) */
#define TABLE_ROW_LEN 4 /* Fat table row length */


double block_size = 0;                          /* Block size */
int system_size = 0;                            /* File system size */
int table_size = TABLE_ROW_LEN * TABLE_SIZE;    /* Fat table size*/
int table_bl_size = 0;                          /* Block number of fat table */
int root_dir_block = 0;                         /* Root directory block */
int table_in = 0 ;                              /* Fat table initial index */
int free_table_in =0 ;                          /* Free fat table initial index */
FILE *disk;                                     /* Disk file */
int table_block=0;

void create_fat_table(int bl_num);
void create_super_block();
void create_blocks();


int main(int argc,char *argv[])
{



    if(argv[1]==NULL || argv[2]==NULL)
    {
        printf("Invalid argument\n");
        exit(EXIT_FAILURE);
    }


    block_size = atof(argv[1]);
    if(block_size<0.5 || block_size>4)
    {
        printf("Block size must be 0.5 or 1 or 2 or 4 KB\n");
        exit(EXIT_FAILURE);
    }
    disk = fopen(argv[2], "w+");
    if(disk==NULL)
    {
        printf("System file can not open\n");
        exit(EXIT_FAILURE);
    }

    block_size *=KB;
    table_bl_size =table_size/ block_size;
    table_bl_size *=2;
    block_size++;

    system_size = block_size*TABLE_SIZE;


    root_dir_block = table_bl_size+1;

    table_block=1;

    create_super_block();
    create_fat_table(table_block);
    create_blocks();

    fclose(disk);

    return 0;

}

/**
*   This function creates system blocks and fill this block with '0'
*/
void create_blocks()
{
    int start_in = root_dir_block*block_size;
    int i,j;
    fseek(disk,start_in,SEEK_SET);

    for(i=root_dir_block; i<TABLE_SIZE; i++)
    {
        for(j=0; j<block_size-1; j++)
        {
            fprintf(disk,"0");
        }

        fprintf(disk,"\n");
    }

}

/**
*   This function creates super block
*/
void create_super_block()
{
    int i;

    for(i=0; i<block_size-1; i++)
        fprintf(disk," ");

    fprintf(disk,"\n");
    fseek(disk,0,SEEK_SET);
    int free_table_add = 1+table_bl_size/2;
    fprintf(disk,"Block number : %.d\n",TABLE_SIZE);
    fprintf(disk,"Block size : %.0f\n",block_size-1);
    fprintf(disk,"Beginning address of fat table blocks : %d\n",1);
    fprintf(disk,"Beginning address of free fat table blocks : %d\n",free_table_add);
    fprintf(disk,"Length of fat table blocks : %d\n",table_bl_size/2);
    fprintf(disk,"Root directory block address : %d. block\n",root_dir_block);
    fprintf(disk,"Beginning address of directory blocks : %d. block\n",root_dir_block+1);

    fprintf(disk,"Dir : 0000 , File : 0000 , Free : %.4d \n",TABLE_SIZE-table_bl_size-1);

}

/**
*   This function creates fat table
*/
void create_fat_table(int bl_num)
{
    int i;
    int c=0;
    fseek(disk, bl_num*block_size, SEEK_SET);
    table_in = bl_num*block_size;

    for(i=0; i<table_bl_size/2; i++)
    {
        int j;

        for(j=0; j<block_size-1; j+=TABLE_ROW_LEN)
        {
            fprintf(disk,"%.4d",0);
        }

        fprintf(disk,"\n");
    }

    c=0;
    free_table_in = (bl_num+table_bl_size/2)*block_size;

    for(i=0; i<table_bl_size/2; i++)
    {
        int j;

        for(j=0; j<block_size-1; j+=TABLE_ROW_LEN)
        {
            fprintf(disk,"%.4d",c);
            c++;
        }

        fprintf(disk,"\n");
    }

    fseek(disk,table_in,SEEK_SET);
    fprintf(disk,"%.3d",-2);

    for(i=0; i<table_bl_size; i++)
    {
        fprintf(disk,"%.3d",-2);
    }

    fprintf(disk,"%.3d",-1);
    fseek(disk,free_table_in,SEEK_SET);
    fprintf(disk,"%.3d",-2);

    for(i=0; i<table_bl_size+1; i++)
    {
        fprintf(disk,"%.3d",-2);
    }
}



