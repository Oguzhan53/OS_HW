#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>

#define STR_SIZE 64
#define SORT_NUM 2  // Sorting algorithm number
#define SEARCH_NUM 5 // Search number
#define EXIST_NUM 3 // Exist number in the disk

#define THREAD_NUM 6   //Total algorithm number

#define REP_NUM 3   // Replace algorithm number

#define FILL 0
#define BUBBLE 1
#define QUICK 2
#define LINEAR 3
#define BINARY 4
#define MERGE 5

#define WSC_THRESHOLD 3  // Threshold for ws clock algorithm
#define ACCESS_PER 200 // Disk access period (like clock)

#define LINE_LEN 11 // Disk file line length
typedef struct
{
    int modified;
    int reference;
    int present;
    int used;
    int second_ch;
    double access_time;
} page_entry;

typedef struct
{
    char name[STR_SIZE];
    int read;
    int write;
    int page_miss;
    int page_rep;
    int disk_write;
    int disk_read;
} information;

int exist_data[EXIST_NUM]; // Data exist im the disk


page_entry page;

int frame_size;
int num_phy;
int num_phy_page;
int num_virt;
int num_virt_page;
char page_rep_alg[STR_SIZE];
char table_type[STR_SIZE];
int page_table_print;
char disk_file[STR_SIZE];




FILE *disk;
int *phy_mem;
int *virt_mem;
page_entry *phy_page;
page_entry *vir_page;


int period = 0;         // Period counter for LRU and WSClock algorithm.When it become 100 then all used bit has been reset
int phy_index = 0;      // Pointer for point in the physical memory pages
int access_counter = 0; // Memory access counter for print information

information info[THREAD_NUM];       // Thread information structs



void *sort_thread(void *arg);
void *merge_thread(void *arg);



void free_src(void *source);
void init_mem(int fl,int t_frame,int t_vir,int t_phy);
void free_resource();
void error_exit();
void set(unsigned int index, int value, char * tName);
int get(unsigned int index, char * tName);
void replace_page(int in, int out,int tNum);
void write_disk(int address,int value);
int read_disk(int address);

void write_all_to_disk();
void reset_lru_period();
double get_current_time();
void update_phy_index(int page_in,int page_out);

void find_best_frame();
void find_best_replace_alg();

void LRU(int page_in,int tNum);
void SC(int page_in,int tNum);
void WSClock(int page_in,int tNum);

void bubble_sort(int start, int end);
void quick_sort( int low, int high);
int partition ( int low, int high);

int main(int argc,char *argv[])
{


    memset(page_rep_alg,'\0',STR_SIZE);
    memset(table_type,'\0',STR_SIZE);
    memset(disk_file,'\0',STR_SIZE);

    strcpy(info[FILL].name,"Fill");
    strcpy(info[BUBBLE].name,"Bubble Sort");
    strcpy(info[QUICK].name,"Quick Sort");
    strcpy(info[LINEAR].name,"Linear Search");
    strcpy(info[BINARY].name,"Binary Search");
    strcpy(info[MERGE].name,"Merge Memory");
    int i;
    for(i=0;i<THREAD_NUM;i++)
    {
        info[i].read=0;
        info[i].write=0;
        info[i].page_miss=0;
        info[i].page_rep=0;
        info[i].disk_write=0;
        info[i].disk_read=0;
    }
    if(argv[1] == NULL)
    {
         printf("Please enter \"./program 1 \" or \"./program 2 \"\n \"1\" for find best frame size, \"2\" for find best page replacement algorithm\n");
         exit(EXIT_FAILURE);
    }
    if(atoi(argv[1])==1)
        find_best_frame();
    else if(atoi(argv[1])==2)
        find_best_replace_alg();
    else
    {
        printf("Please enter \"./program 1 \" or \"./program 2 \"\n \"1\" for find best frame size, \"2\" for find best page replacement algorithm\n");
        exit(EXIT_FAILURE);
    }


    write_all_to_disk();


    printf("\n");
    free_resource();
    return 0;
}










/**
*   This is partition function for quick sort
*/
int partition ( int low, int high)
{
    char *alg="quick";
    int pivot = get(high,alg);
    int i = (low - 1);
    int j,t1,t2;

    for (j = low; j <= high - 1; j++)
    {

        t1 = get(j,alg);

        if (t1 < pivot)
        {
            i++;
            t2=get(i,alg);
            set(i,t1,alg);
            set(j,t2,alg);
        }
    }

    t1=get(high,alg);
    t2=get(i+1,alg);
    set(i+1,t1,alg);
    set(high,t2,alg);
    return (i + 1);
}


/**
*   This is quick sort function
*/
void quick_sort( int low, int high)
{
    if (low < high)
    {

        int pi = partition( low, high);
        quick_sort( low, pi - 1);
        quick_sort( pi + 1, high);
    }
}


/**
*   This is bubble sort function
*/
void bubble_sort(int start, int end)
{
    int i, j;

    for (i = start; i < end-1; i++)
    {
        for (j = start; j < end-i-1; j++)
        {
            int t1 = get(j,"bubble"),t2 = get(j+1,"bubble");

            if(t1>t2)
            {
                set(j,t2,"bubble");
                set(j+1,t1,"bubble");
            }
        }

    }

}








/**
*   This function initialize the virtual and physical memory and disk
*/
void init_mem(int fl,int t_frame,int t_vir,int t_phy)
{


    if(fl){
        free_src(phy_mem);
        free_src(virt_mem);
        free_src(phy_page);
        free_src(vir_page);


    }

   // printf("vir : %d - phy : %d \n",t_vir,t_phy);
    frame_size = pow(2,t_frame);
    num_phy_page = pow(2,t_phy);
    num_virt_page = pow(2,t_vir);
    num_phy = frame_size*num_phy_page;
    num_virt = frame_size*num_virt_page;
    phy_mem = (int*)malloc(sizeof(int)*num_phy);
    virt_mem = (int*)malloc(sizeof(int)*num_virt);
    phy_page = (page_entry*)malloc(sizeof(page_entry)*num_phy_page);
    vir_page = (page_entry*)malloc(sizeof(page_entry)*num_virt_page);


    disk = fopen(disk_file,"w+");

    if(disk==NULL)
    {
        printf("Error opening disk file.\n");
        error_exit();
    }

    srand(1000);
    int i;
    int val;

    for(i=0; i<num_virt; i++)
    {
        val = rand();

        if(i<EXIST_NUM)
            exist_data[i]=val;

        set(i,val,"fill"); //setting memory with random number.
    }

    int j=0;

    for(i=0; i<num_virt_page; i++)
    {


        vir_page[i].modified = 0;
        vir_page[i].reference = i*frame_size;
        vir_page[i].used = 0;
        vir_page[i].second_ch =0;
        vir_page[i].access_time=get_current_time();

        if(i*frame_size<num_phy)
        {
            vir_page[i].present=1;
            phy_page[j].reference = i;
            j++;
        }

        else
            vir_page[i].present=0;
    }

    if(fclose(disk)!=0)
    {
        printf("Error closing file.\n");
        error_exit();
    }
    phy_index=0;
}


/**
*   This function finds the best replacement algorithm for each sorting algorithm.
*/
void find_best_replace_alg()
{
    int fr=2,phy=4,vir=6;
    int t_frame_size = fr;
    int t_num_phy = phy;
    int t_num_virt = vir;
    strcpy(table_type,"inverted");
    page_table_print = 10000;
    strcpy(disk_file,"diskFileName.dat");
    int num_page_rep = 0;
    char best_alg[STR_SIZE];
    memset(best_alg,'\0',STR_SIZE);
    int i;
    init_mem(0,t_frame_size,t_num_virt,t_num_phy);;
    printf("******************** FIND BEST REPLACEMENT ALGORITHM FOR SORT ALGORTIHMS ********************\n\n");

    printf("Frame size : %d Phsical memory size : %d\tVirtual memory size : %d\n\n",frame_size,num_phy,num_virt);
    printf("Bubble Sort Algorithm Statistic \n");
    for(i=0;i<REP_NUM;i++)
    {
        info[BUBBLE].page_rep=0;
        if(i==0)
        {
            memset(page_rep_alg,'\0',STR_SIZE);
            strcpy(page_rep_alg,"LRU");
            init_mem(1,t_frame_size,t_num_virt,t_num_phy);
            bubble_sort(0,num_virt);
            printf("Page fault number for LRU algorithm : %d \n",info[BUBBLE].page_rep);
            num_page_rep = info[BUBBLE].page_rep;
            memset(best_alg,'\0',STR_SIZE);
            strcpy(best_alg,page_rep_alg);
        }
        else if(i==1)
        {
            memset(page_rep_alg,'\0',STR_SIZE);
            strcpy(page_rep_alg,"SC");
            init_mem(1,t_frame_size,t_num_virt,t_num_phy);
            bubble_sort(0,num_virt);
            printf("Page fault number for SC algorithm : %d \n",info[BUBBLE].page_rep);
            if(num_page_rep > info[BUBBLE].page_rep){
                num_page_rep = info[BUBBLE].page_rep;
                memset(best_alg,'\0',STR_SIZE);
                strcpy(best_alg,page_rep_alg);
            }

        }
        else
        {
            memset(page_rep_alg,'\0',STR_SIZE);
            strcpy(page_rep_alg,"WSClock");
            init_mem(1,t_frame_size,t_num_virt,t_num_phy);
            bubble_sort(0,num_virt);
            printf("Page fault number for WSClock algorithm : %d \n",info[BUBBLE].page_rep);
            if(num_page_rep > info[BUBBLE].page_rep){
                num_page_rep = info[BUBBLE].page_rep;
                memset(best_alg,'\0',STR_SIZE);
                strcpy(best_alg,page_rep_alg);
            }

        }
    }
    printf("Best replace algorithm : %s , page fault number : %d\n\n",best_alg,num_page_rep);


    printf("Quick Sort Algorithm Statistic \n");
    for(i=0;i<REP_NUM;i++)
    {
        info[BUBBLE].page_rep=0;
        if(i==0)
        {
            memset(page_rep_alg,'\0',STR_SIZE);
            strcpy(page_rep_alg,"LRU");
            init_mem(1,t_frame_size,t_num_virt,t_num_phy);
            quick_sort(0,num_virt-1);
            printf("Page fault number for LRU algorithm : %d \n",info[QUICK].page_rep);
            num_page_rep = info[QUICK].page_rep;
            memset(best_alg,'\0',STR_SIZE);
            strcpy(best_alg,page_rep_alg);
        }
        else if(i==1)
        {
            memset(page_rep_alg,'\0',STR_SIZE);
            strcpy(page_rep_alg,"SC");
            init_mem(1,t_frame_size,t_num_virt,t_num_phy);
            quick_sort(0,num_virt-1);
            printf("Page fault number for SC algorithm : %d \n",info[QUICK].page_rep);
            if(num_page_rep > info[QUICK].page_rep){
                num_page_rep = info[QUICK].page_rep;
                memset(best_alg,'\0',STR_SIZE);
                strcpy(best_alg,page_rep_alg);
            }

        }
        else
        {
            memset(page_rep_alg,'\0',STR_SIZE);
            strcpy(page_rep_alg,"WSClock");
            init_mem(1,t_frame_size,t_num_virt,t_num_phy);
            quick_sort(0,num_virt-1);
            printf("Page fault number for WSClock algorithm : %d \n",info[QUICK].page_rep);
            if(num_page_rep > info[QUICK].page_rep){
                num_page_rep = info[QUICK].page_rep;
                memset(best_alg,'\0',STR_SIZE);
                strcpy(best_alg,page_rep_alg);
            }

        }
    }
    printf("Best replace algorithm : %s , page fault number : %d\n",best_alg,num_page_rep);

}


/**
*   This function finds the best frame size for each sorting algorithm.
*/
void find_best_frame()
{

    printf("******************** FIND BEST FRAME SIZE ********************\n\n");

    int fr=1,phy=7,vir=8;
    int t_frame_size = fr;
    int t_num_phy = phy;
    int t_num_virt = vir;
    strcpy(page_rep_alg,"LRU");
    strcpy(table_type,"inverted");
    page_table_print = 10000;
    strcpy(disk_file,"diskFileName.dat");
    init_mem(0,t_frame_size,t_num_virt,t_num_phy);

    int num_page_rep = 0, best_frame=0;

    int i;

    printf("FINDIG BEST PAGE FRAME SIZE FOR BUBBLE SORT\n");
    printf("Phsical memory size : %d\tVirtual memory size : %d\n",num_phy,num_virt);
    for(i=0;num_phy>0;i++)
    {
        info[BUBBLE].page_rep=0;
        t_frame_size++;
        t_num_phy--;
        t_num_virt--;
        bubble_sort(0,num_virt);
        if(i==0)
        {
            best_frame = frame_size;
            num_page_rep = info[BUBBLE].page_rep;
        }

        if(num_page_rep>info[BUBBLE].page_rep)
        {
            best_frame = frame_size;
            num_page_rep = info[BUBBLE].page_rep;
        }
        printf("Frame size : %d\tPage replacement number : %d\n",frame_size,info[BUBBLE].page_rep);
        init_mem(1,t_frame_size,t_num_virt,t_num_phy);
    }

    printf("Best frame size for Bubble Sort : %d replacement num : %d \n\n",best_frame,num_page_rep);

    num_page_rep = 0;
    best_frame=0;
    t_frame_size = 1;
    t_num_phy = 8;
    t_num_virt = 9;
    init_mem(1,t_frame_size,t_num_virt,t_num_phy);

    printf("FINDIG BEST PAGE FRAME SIZE FOR QUICK SORT\n");
    printf("Phsical memory size : %d\tVirtual memory size : %d\n",num_phy,num_virt);
    for(i=0;num_phy>0;i++)
    {
        info[QUICK].page_rep=0;
        t_frame_size++;
        t_num_phy--;
        t_num_virt--;
        quick_sort(0,num_virt-1);

        if(i==0)
        {
            best_frame = frame_size;
            num_page_rep = info[QUICK].page_rep;
        }

        if(num_page_rep>info[QUICK].page_rep)
        {
            best_frame = frame_size;
            num_page_rep = info[QUICK].page_rep;
        }
        printf("Frame size : %d\tPage replacement number : %d\n",frame_size,info[QUICK].page_rep);
        if(num_phy==frame_size){
            break;
        }

        init_mem(1,t_frame_size,t_num_virt,t_num_phy);
    }
    printf("Best frame size for Quick Sort : %d replacement num : %d \n\n",best_frame,num_page_rep);

}


/**
*   This function set data which exist in the given index with given value
*/
void set(unsigned int index, int value, char * tName)
{


    access_counter++;

    int tNum;
    if(strcmp("fill",tName)==0)
        tNum=FILL;
    else if(strcmp("bubble",tName)==0)
        tNum=BUBBLE;
    else if(strcmp("quick",tName)==0)
        tNum=QUICK;
    else if(strcmp("linear",tName)==0)
        tNum=LINEAR;
    else if(strcmp("binary",tName)==0)
        tNum=BINARY;
    else
        tNum=MERGE;
    //printf("set %s \n",tName);
    if(strcmp("fill",tName)==0)
    {

     //   printf("fill : %d - %d \n",info[tNum].write,tNum);
        if(index>=num_phy)  // if there is no more space in phys memmory.
        {
            virt_mem[index] = -1;
            fprintf(disk,"%.10d\n",value);
            info[tNum].write++;
        }
        else
        {

            info[tNum].write++;
            info[tNum].disk_write++;
            virt_mem[index] = index;
            phy_mem[index] = value;
            fprintf(disk,"%.10d\n",value);
        }
    }
    else
    {
        int page_in=index/frame_size;

        if(vir_page[page_in].present)  // if the page exist in the physical memory
            vir_page[page_in].second_ch=1;
        else
        {
            info[tNum].page_miss++;
            info[tNum].page_rep++;
            if(strcmp("LRU",page_rep_alg)==0)
                LRU(page_in,tNum);
            else if(strcmp("SC",page_rep_alg)==0)
                SC(page_in,tNum);
            else if(strcmp("WSClock",page_rep_alg)==0)
                WSClock(page_in,tNum);
            else{
                printf("Invalid page replacement algoritm \n");
                error_exit();
            }
        }
        info[tNum].write++;
        phy_mem[virt_mem[index]]=value;
        vir_page[page_in].modified=1;
        vir_page[page_in].used++;
        vir_page[page_in].access_time= get_current_time();

    }
    if(access_counter==page_table_print){
        //print_memory();
        access_counter=0;
    }


}


/**
*   This function take data which exist in the given index
*/
int get(unsigned int index, char * tName)
{

    access_counter++;
    // printf("get lock yess \n");
    int page_in=index/frame_size;
    int res ;
    int tNum;
    if(strcmp("fill",tName)==0)
        tNum=FILL;
    else if(strcmp("bubble",tName)==0)
        tNum=BUBBLE;
    else if(strcmp("quick",tName)==0)
        tNum=QUICK;
    else if(strcmp("linear",tName)==0)
        tNum=LINEAR;
    else if(strcmp("binary",tName)==0)
        tNum=BINARY;
    else
        tNum=MERGE;



    if(vir_page[page_in].present)  // if the page exist in the physical memory
        vir_page[page_in].second_ch=1;

    else // if the page does not exist in the physical memory
    {
        info[tNum].page_miss++;
        info[tNum].page_rep++;
        if(strcmp("LRU",page_rep_alg)==0)
            LRU(page_in,tNum);
        else if(strcmp("SC",page_rep_alg)==0)
            SC(page_in,tNum);
        else if(strcmp("WSClock",page_rep_alg)==0)
            WSClock(page_in,tNum);
        else{
            printf("Invalid page replacement algoritm \n");
            error_exit();
        }

    }
    res =phy_mem[virt_mem[index]];
    info[tNum].read++;
    vir_page[page_in].used++;
    vir_page[page_in].access_time= get_current_time();
    if(access_counter==page_table_print){
        access_counter=0;
    }


    return res;
}

/**
*   This is LRU page replacement algorithm
*/
void LRU(int page_in,int tNum)
{
    int i=0,least_used,page_out;
    int fl=1;

    period++;

    for(i=0; i<num_phy_page; i++)
    {

        if(fl)
        {
            least_used=vir_page[phy_page[i].reference].used;
            page_out = phy_page[i].reference;
            fl=0;
        }
        else
        {
            if(vir_page[phy_page[i].reference].used<least_used)
            {
                least_used=vir_page[phy_page[i].reference].used;
                page_out = phy_page[i].reference;
            }
        }
    }
    replace_page(page_in,page_out,tNum);
    update_phy_index(page_in,page_out);

    if(period==ACCESS_PER)
    {
        reset_lru_period();
        period=0;

    }


}

/**
*   This is Second Chance page replacement algorithm
*/
void SC(int page_in,int tNum)
{

    int page_out;

    if(vir_page[phy_page[phy_index].reference].second_ch)
    {
        while(vir_page[phy_page[phy_index].reference].second_ch)
        {

           // printf("phy_index : %d * ",phy_index);
            vir_page[phy_page[phy_index].reference].second_ch=0;
            phy_index++;
            phy_index=phy_index%num_phy_page;
        }

    }
    page_out=phy_page[phy_index].reference;
    replace_page(page_in,page_out,tNum);
    phy_index++;
    phy_index=phy_index%num_phy_page;
    update_phy_index(page_in,page_out);

}

void WSClock(int page_in,int tNum)
{

    int page_out;
    period++;

    while((!vir_page[phy_page[phy_index].reference].used) && (get_current_time()-vir_page[phy_page[phy_index].reference].access_time)>WSC_THRESHOLD)
    {

        vir_page[phy_page[phy_index].reference].used=0;
        phy_index++;
        phy_index=phy_index%num_phy_page;
    }

    page_out=phy_page[phy_index].reference;
    replace_page(page_in,page_out,tNum);
    phy_index++;
    phy_index=phy_index%num_phy_page;
    update_phy_index(page_in,page_out);
    if(period==ACCESS_PER)
    {
        reset_lru_period();
        period=0;

    }
}

/**
*   This function returns the current time
*/
void update_phy_index(int page_in,int page_out){
    int i;

    for(i=0; i<num_phy_page; i++)
    {
        if(phy_page[i].reference==page_out)
        {
            phy_page[i].reference=page_in;
            break;
        }

    }


}

/**
*   This function returns the current time
*/
double get_current_time()
{
    struct timespec tstart= {0,0};
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    double res =(double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec;
    res *=1000;
    return res;
}


/**
*   This function resets the used bit
*/
void reset_lru_period()
{
    int i;

    for(i =0; i<num_phy_page; i++)
        vir_page[phy_page[i].reference].used=0;

}


/**
*   This function writes all data which exist in virtual memory and modified
*   data at the end of the program
*/
void write_all_to_disk()
{

    int i ,j;

    for(i=0; i<num_phy_page; i++)
    {
        if(vir_page[phy_page[i].reference].modified && vir_page[phy_page[i].reference].present)
        {
            int vir_in = vir_page[phy_page[i].reference].reference;

            for(j=vir_in; j<vir_in+frame_size; j++)
            {
                write_disk(j,phy_mem[virt_mem[j]]);
            }
        }
    }
}


/**
*   This function replace the out page with in page
*/
void replace_page(int in, int out,int tNum)
{

    int i;

    if(vir_page[out].modified)
    {
        for(i=vir_page[out].reference; i<vir_page[out].reference+frame_size; i++)
        {
            info[tNum].disk_write++;
            write_disk(i,phy_mem[virt_mem[i]]);
        }
    }


    int j = vir_page[out].reference;

    int vir_index=vir_page[in].reference;

    for(i=vir_index; i<vir_index+frame_size; i++)
    {
        virt_mem[i]=virt_mem[j];
        virt_mem[j]=-1;
        phy_mem[virt_mem[i]]=read_disk(i);
        info[tNum].disk_read++;
        j++;
    }

    vir_page[out].present=0;
    vir_page[out].modified=0;
    vir_page[out].used=0;
    vir_page[in].present=1;


}

/**
*   This function reads the data from disk with given address
*/
int read_disk(int address)
{
    disk = fopen(disk_file,"r+");

    if(disk==NULL)
    {
        printf("Error opening disk file.\n");
        error_exit();
    }

    fseek(disk,address*LINE_LEN,SEEK_SET);
    char val[LINE_LEN+1];
    memset(val,'\0',sizeof(val));
    fgets(val,sizeof(val),disk);
    fclose(disk);
    return atoi(val);
}


/**
*  This function write new value to given address
*/
void write_disk(int address,int value)
{

    disk = fopen(disk_file,"r+");

    if(disk==NULL)
    {
        printf("Error opening disk file.\n");
        error_exit();
    }

    fseek(disk,address*LINE_LEN,SEEK_SET);
    fprintf(disk,"%.10d\n",value);
    fclose(disk);

}

void error_exit()
{



    free_resource();
    printf("Exiting...\n");
    exit(EXIT_FAILURE);
}


void free_resource()
{
    free_src(phy_mem);
    free_src(virt_mem);
    free_src(phy_page);
    free_src(vir_page);


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
