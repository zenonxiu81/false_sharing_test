/**
 *false_sharing.c
 * Copyright (C) 2021 Zenon Xiu
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#define _GNU_SOURCE
#include <stdio.h>



#include<stdlib.h>
 
#include<pthread.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>


#define CACHE_LINE_SZ (64)

typedef struct {
	
  volatile long long data0;
  volatile long long data1; 

} test_data;

typedef struct {
	
  volatile long long data0  __attribute__((aligned(CACHE_LINE_SZ)));
  volatile long long data1  __attribute__((aligned(CACHE_LINE_SZ))); 

} cacheline_aligned_test_data;

unsigned int cpu_run_T0, cpu_run_T1;    //cpu number to run the test.

test_data false_sharing_test_data;
cacheline_aligned_test_data false_sharing_cacheline_aligned_test_data;

unsigned long long iteration;

pthread_t tidp[2];


void * false_sharing_test(void * a)
{
  int id, raw_id; 
  unsigned long long  i;
  int test_num=*((int *)a);
  cpu_set_t cpuset;
  int ret;
  pthread_t pid;
  volatile long long * p_data0;
  volatile long long * p_data1;


  if(test_num==0)
   {
    p_data0=&(false_sharing_test_data.data0);
    p_data1=&(false_sharing_test_data.data1);
   }
  else if(test_num==1)
   {
    p_data0=&(false_sharing_cacheline_aligned_test_data.data0);
    p_data1=&(false_sharing_cacheline_aligned_test_data.data1);   
   }
    
   
   while(tidp[0]==0); //Wait for thread 0 being created.
   CPU_ZERO(&cpuset);
   CPU_SET(cpu_run_T0,&cpuset);  //set which CPU to run the thread 0
   pthread_setname_np(tidp[0]," T0");
   ret=pthread_setaffinity_np(tidp[0],sizeof(cpu_set_t),&cpuset);
   if(ret<0)
    printf("set thread0 affinity failed.. %d\n",ret);
   
   while(tidp[1]==0); //Wait for thread 1 being created.
   CPU_ZERO(&cpuset);
   CPU_SET(cpu_run_T1,&cpuset);  //set which CPU to run the thread 1
   pthread_setname_np(tidp[1]," T1");
   ret=pthread_setaffinity_np(tidp[1],sizeof(cpu_set_t),&cpuset);
  if(ret<0)
   printf("set thread1 affinity failed.. %d\n", ret);
  
  pid=pthread_self();

  if(pid==tidp[0]){
  printf("T0 started!\n");
  for(i=0;i<iteration; i++)
   {
   *p_data0 +=1;
   }
  printf("T0 done!\n"); 
  }
  else if(pid==tidp[1]){
   printf("T1 started!\n");
    for(i=0;i<iteration; i++)
     {
      *p_data1 +=1;
     } 
   printf("T1 done!\n"); 
  } 	
}

int main(int agrc,char* argv[])
{

 int i;
 int test_num;
 tidp[0]=0;
 tidp[1]=0;
 
 if(agrc<6)
  {
   printf("Wrong paramters\n");
   printf("-n to specific number of iteration, -t to specify which test: 0 for unaligned data, 1 for cacheline aligned test,  -c to specific two cpus to run,\n for example \" -n 10000 -t 0 -c 1 2\" \n");
   return -1;
  }
 for(i=1; i<agrc; )
  {
   if(strcmp(argv[i],"-n")==0)
     {
       iteration=atoi(argv[i+1]);
       i+=2;
       continue;
     }

   if(strcmp(argv[i],"-t")==0)
     {
       test_num=atoi(argv[i+1]);
       i+=2;
       continue;
     }
   if(strcmp(argv[i],"-c")==0)
     {
       cpu_run_T0=atoi(argv[i+1]);
       cpu_run_T1=atoi(argv[i+2]);
       i+=3;
       continue;
     }
   else 
   {
     i++; 
     continue;
   } 
 }
 
 printf("Parameter used: -n %lld -t %d -c %d %d\n",iteration,test_num,cpu_run_T0,cpu_run_T1);


 __asm__("dmb ish ");
 if ((pthread_create(&tidp[0], NULL, false_sharing_test, (void*)&test_num)) == -1)
  {
    printf("create error!\n");
    return 1;
  }
 
  
  __asm__("dmb ish ");

  if ((pthread_create(&tidp[1], NULL, false_sharing_test, (void*)&test_num)) == -1)
  {
    printf("create error!\n");
    return 1;
  }

  if (pthread_join(tidp[0], NULL))
  {
    printf("thread is not exit...\n");
    return -2;
  }

  if (pthread_join(tidp[1], NULL))
  {
    printf("thread is not exit...\n");
    return -2;
  }
 
 
  return 0;	
	
}
