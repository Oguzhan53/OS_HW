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

#define FILL 0
#define BUBBLE 1
#define QUICK 2
#define LINEAR 3
#define BINARY 4
#define MERGE 5

#define WSC_THRESHOLD 70  // Threshold for ws clock algorithm
#define ACCESS_PER 500 // Disk access period (like clock)

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

int exist_data[EXIST_NUM]; // Data which exist in the disk. This datas used for search at the end


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
pthread_t sort_id[SORT_NUM];        // Sort thread ids
pthread_t merge_id[1];              // Merge thread id
pthread_t search_id[SEARCH_NUM];    // Search thread ids
pthread_mutex_t lock;               // Mutex for get-set synchronization


void *sort_thread(void *arg);
void *merge_thread(void *arg);
void *binary_search_th(void *arg);
void *linear_search_th(void *arg);


void free_src(void *source);
void init_mem();
void free_resource();
void error_exit();
void set(unsigned int index, int value, char * tName);
int get(unsigned int index, char * tName);
void replace_page(int in, int out,int tNum);
void write_disk(int address,int value);
int read_disk(int address);
void print_memory();
void write_all_to_disk();
void reset_lru_period();
double get_current_time();
void update_phy_index(int page_in,int page_out);
void print_info();
int get_gap(int gap);
void merge_mem();

void LRU(int page_in,int tNum);
void SC(int page_in,int tNum);
void WSClock(int page_in,int tNum);

void bubble_sort(int start, int end);
void quick_sort( int low, int high);
int partition ( int low, int high);

int linear_search(int value);
int binary_search(int l, int r, int value);
int main(int argc,char *argv[])
{


    memset(page_rep_alg,'\0',STR_SIZE);
    memset(table_type,'\0',STR_SIZE);
    memset(disk_file,'\0',STR_SIZE);

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        exit(EXIT_FAILURE);
    }


    if(argc!=8)
    {
        printf("Invalid command line argument\nExiting...\n");
        exit(EXIT_FAILURE);
    }


    if(strcmp("LRU",argv[4])!=0 && strcmp("SC",argv[4])!=0 && strcmp("WSClock",argv[4])!=0)
    {
        printf("Invalid replacement algorithm \nExiting...\n");
        exit(EXIT_FAILURE);

    }
    frame_size = atoi(argv[1]);
    num_phy = atoi(argv[2]);
    num_virt = atoi(argv[3]);
    strcpy(page_rep_alg,argv[4]);
    strcpy(table_type,argv[5]);
    page_table_print = atoi(argv[6]);
    strcpy(disk_file,argv[7]);

    if(num_phy>num_virt)
    {
        printf("Physical memory can not grater than virtual memory.\n");
        exit(EXIT_FAILURE);
    }




    strcpy(info[FILL].name,"Fill");
    strcpy(info[BUBBLE].name,"Bubble Sort");
    strcpy(info[QUICK].name,"Quick Sort");
    strcpy(info[LINEAR].name,"Linear Search");
    strcpy(info[BINARY].name,"Binary Search");
    strcpy(info[MERGE].name,"Merge Memory");
    int s;
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

    init_mem();
    int args[SORT_NUM];

    for (i = 0; i < SORT_NUM; i++)
        sort_id[i]=-1;
    for (i = 0; i < SEARCH_NUM; i++)
        search_id[i]=-1;
    merge_id[0]=-1;


    for (i = 0; i < SORT_NUM; i++)
    {
        args[i]=i;
        s = pthread_create(&sort_id[i], NULL, sort_thread, (void*)&args[i]);

        if (s != 0)
        {
            fprintf(stderr, "Error to create thread: %s\n", strerror(s));
            error_exit();
        }
    }


    for ( i = 0; i < SORT_NUM; i++)
    {

        s = pthread_join(sort_id[i], NULL);

        if (s != 0)
        {
            fprintf(stderr, "Error to join a thread: %s\n", strerror(s));
            error_exit();
        }
    }


    s = pthread_create(&merge_id[0], NULL, merge_thread, NULL);

    if (s != 0)
    {
        fprintf(stderr, "Error to create thread: %s\n", strerror(s));
        error_exit();
    }

    s = pthread_join(merge_id[0], NULL);

    if (s != 0)
    {
        fprintf(stderr, "Error to join a thread: %s\n", strerror(s));
        error_exit();
    }





    int dt1 = exist_data[0];
    int dt2 = exist_data[1];
    int dt3 = exist_data[2];
    int dt4 = -1;
    int dt5 = -2;
    s = pthread_create(&search_id[0], NULL, linear_search_th, (void*)&dt1);

    if (s != 0)
    {
        fprintf(stderr, "Error to create thread: %s\n", strerror(s));
        error_exit();
    }

    s = pthread_create(&search_id[1], NULL, binary_search_th, (void*)&dt2);

    if (s != 0)
    {
        fprintf(stderr, "Error to create thread: %s\n", strerror(s));
        error_exit();
    }

    s = pthread_create(&search_id[2], NULL, linear_search_th, (void*)&dt3);

    if (s != 0)
    {
        fprintf(stderr, "Error to create thread: %s\n", strerror(s));
        error_exit();
    }

    s = pthread_create(&search_id[3], NULL, binary_search_th, (void*)&dt4);

    if (s != 0)
    {
        fprintf(stderr, "Error to create thread: %s\n", strerror(s));
        error_exit();
    }
    s = pthread_create(&search_id[4], NULL, binary_search_th, (void*)&dt5);

    if (s != 0)
    {
        fprintf(stderr, "Error to create thread: %s\n", strerror(s));
        error_exit();
    }


    s = pthread_join(search_id[0], NULL);

    if (s != 0)
    {
        fprintf(stderr, "Error to join a thread: %s\n", strerror(s));
        error_exit();
    }

    s = pthread_join(search_id[1], NULL);

    if (s != 0)
    {
        fprintf(stderr, "Error to join a thread: %s\n", strerror(s));
        error_exit();
    }
    s = pthread_join(search_id[2], NULL);

    if (s != 0)
    {
        fprintf(stderr, "Error to join a thread: %s\n", strerror(s));
        error_exit();
    }

    s = pthread_join(search_id[3], NULL);

    if (s != 0)
    {
        fprintf(stderr, "Error to join a thread: %s\n", strerror(s));
        error_exit();
    }
    s = pthread_join(search_id[4], NULL);

    if (s != 0)
    {
        fprintf(stderr, "Error to join a thread: %s\n", strerror(s));
        error_exit();
    }

    print_info();

    write_all_to_disk();

    if(pthread_mutex_destroy(&lock)!=0)
    {
        printf("Error to destroy mutex");
        error_exit();

    }

    printf("\n");
    free_resource();
    return 0;
}



/**
*   This function prints the physical memory.
*/
void print_memory()
{
    printf("\n Physical memory pages \n");
    int i;

    for(i=0; i<num_phy_page; i++)
    {

        int j;
        printf("\nPAGE %d \n",phy_page[i].reference);
        int vir_in =vir_page[phy_page[i].reference].reference;

        for(j=vir_in; j<vir_in+frame_size; j++)
        {
            printf(" * %d\n",phy_mem[virt_mem[j]]);
        }
    }
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
    //  printf("start : %d - end : %d \n",start,end);

    for (i = start; i < end-1; i++)
    {
       // printf("%d \n",i);
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
*   This function returns the gap value for merge memory algorithm.
*/
int get_gap(int gap)
{
    if (gap <= 1)
        return 0;
    return (gap / 2) + (gap % 2);
}

/**
*   This function merges the virtual memory as a sorted with using gap algorithm.
*/
void merge_mem()
{
    int far = num_virt/2;
    int n=far,m=far;
    int t1,t2;
    int i, j, gap = n + m;
    char *alg="merge";
    for (gap = get_gap(gap);gap > 0; gap = get_gap(gap))
    {

        for (i = 0; i + gap < n; i++){
            t1=get(i,alg);
            t2=get(i+gap,alg);
            if (t1 > t2){
                set(i,t2,alg);
                set(i+gap,t1,alg);
            }

        }

        for (j = gap > n ? gap - n : 0;i < n && j < m;i++, j++)
        {
            t1=get(i,alg);
            t2=get(j+far,alg);
            if (t1 > t2){
                set(i,t2,alg);
                set(j+far,t1,alg);
            }
        }


        if (j < m) {
            for (j = 0; j + gap < m; j++){
                t1=get(j+far,alg);
                t2=get(j+far+gap,alg);
                if (t1 > t2){
                    set(j+far,t2,alg);
                    set(j+far+gap,t1,alg);
                }

            }

        }
    }


}


/**
*   This is linear search function
*/
int linear_search(int value)
{

    char *alg="linear";
    int i;

    for(i=0; i<num_virt; i++)
    {
        if(value==get(i,alg))
        {
            return i;
        }
    }

    return -1;

}

/**
*   This is binary search function
*/
int binary_search( int l, int r, int value)
{
    char *alg="binary";

    if (r >= l)
    {
        int mid = l + (r - l) / 2;

        if (get(mid,alg) == value)
            return mid;

        if (get(mid,alg) > value)
            return binary_search(l, mid - 1, value);

        return binary_search( mid + 1, r, value);
    }

    return -1;
}

/**
*   This function initialize the virtual and physical memory and disk
*/
void init_mem()
{
    frame_size = pow(2,frame_size);
    num_phy_page = pow(2,num_phy);
    num_virt_page = pow(2,num_virt);
    num_phy = frame_size*num_phy_page;
    num_virt = frame_size*num_virt_page;

    //phy_to_vir = (int*)malloc(sizeof(int)*num_phy_page);
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
}



/**
*   This function prints the algorithm informations.
*/
void print_info()
{

    int i=0;
    printf("******************** STATISTICS ********************\n");
    for(i=0;i<THREAD_NUM;i++)
    {
        printf("%s algorithm ;\n",info[i].name);
        printf("Number of reads : %d\n",info[i].read);
        printf("Number of writes : %d\n",info[i].write);
        printf("Number of page misses : %d \n",info[i].page_miss);
        printf("Number of page replacements : %d \n",info[i].page_rep);
        printf("Number of disk page writes : %d \n",info[i].disk_write);
        printf("Number of disk page reads : %d \n",info[i].disk_read);
        printf("\n");

    }

}







/**
*   This is thread function for sorting virtual memory
*/
void *sort_thread(void *arg)
{
    int id = *((int *)arg);
    //printf("part : %d \n",part);

    if(id)
        quick_sort(num_virt/2,num_virt-1);
    else
        bubble_sort(0,num_virt/2);

    return NULL;
}

/**
*   This is thread function for merge sorted parts of virtual memory
*/
void *merge_thread(void *arg)
{
    merge_mem();
    return NULL;
}

/**
*   This is thread function for search a data in virtual memory
*/
void *binary_search_th(void *arg)
{
    int value = *((int *)arg);
    int res = binary_search(0,num_virt,value);
    printf("Binary seach result : %d in %d  \n",value,res);
    return NULL;
}

void *linear_search_th(void *arg)
{
    int value = *((int *)arg);
    int res = linear_search(value);
    printf("linear seach result : %d  in : %d \n",value,res);
    return NULL;
}



/**
*   This function set data which exist in the given index with given value
*/
void set(unsigned int index, int value, char * tName)
{


    pthread_mutex_lock(&lock);
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

    if(strcmp("fill",tName)==0)
    {

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
        print_memory();
        access_counter=0;
    }

    pthread_mutex_unlock(&lock);

}


/**
*   This function take data which exist in the given index
*/
int get(unsigned int index, char * tName)
{

    pthread_mutex_lock(&lock);
    access_counter++;
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
        print_memory();
        access_counter=0;
    }

    pthread_mutex_unlock(&lock);

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

/**
*   This is WS Clock page replacement algorithm
*/
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
*   This function updates the physical memory page indexes when page replacement occurs
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

/**
*   This function frees all recources and terminates the all thread when any error occurs
*/
void error_exit()
{


    int i;
    for ( i = 0; i < SORT_NUM; i++){
        if(sort_id[i]!=-1)
            pthread_cancel(sort_id[i]);
    }

    for ( i = 0; i < SEARCH_NUM; i++){
        if(search_id[i]!=-1)
            pthread_cancel(search_id[i]);
    }

    if(merge_id[0]!=-1)
        pthread_cancel(merge_id[0]);

    for ( i = 0; i < SORT_NUM; i++){
        if(sort_id[i]!=-1)
            pthread_join(sort_id[i], NULL);
    }

    if(merge_id[0]!=-1)
        pthread_join(merge_id[0], NULL);

    for ( i = 0; i < SEARCH_NUM; i++){
        if(search_id[i]!=-1)
            pthread_join(search_id[i], NULL);
    }

    pthread_mutex_destroy(&lock);
    free_resource();
    printf("Exiting...\n");
    exit(EXIT_FAILURE);
}

/**
*   This function frees all recourses
*/
void free_resource()
{
    free_src(phy_mem);
    free_src(virt_mem);
    free_src(phy_page);
    free_src(vir_page);
    //free_src(phy_to_vir);

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
