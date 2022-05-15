/****************************************************************************
*                                                                           *
*             OpenMP MicroBenchmark Suite - Version 3.1                     *
*                                                                           *
*                            produced by                                    *
*                                                                           *
*             Mark Bull, Fiona Reid and Nix Mc Donnell                      *
*                                                                           *
*                                at                                         *
*                                                                           *
*                Edinburgh Parallel Computing Centre                        *
*                                                                           *
*         email: markb@epcc.ed.ac.uk or fiona@epcc.ed.ac.uk                 *
*                                                                           *
*                                                                           *
*      This version copyright (c) The University of Edinburgh, 2015.        *
*                                                                           *
*                                                                           *
*  Licensed under the Apache License, Version 2.0 (the "License");          *
*  you may not use this file except in compliance with the License.         *
*  You may obtain a copy of the License at                                  *
*                                                                           *
*      http://www.apache.org/licenses/LICENSE-2.0                           *
*                                                                           *
*  Unless required by applicable law or agreed to in writing, software      *
*  distributed under the License is distributed on an "AS IS" BASIS,        *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
*  See the License for the specific language governing permissions and      *
*  limitations under the License.                                           *
*                                                                           *
****************************************************************************/


#include "common.h"
#include "schedbench.h"

//extern void pthread_create(void);
//extern void pthread_join(void);
//extern pthread_t;
typedef struct
{
  void * p;                   /* Pointer to actual object */
  unsigned int x;             /* Extra information - reuse count etc */
  //mjc add attr into pthread_t
  //struct pthread_attr_t_* attr;
} pthread_t;

int cksz, itersperthr = 128;
char testName[32];

static void testpthread();
static void testpthreadintegrated();
static void dummypthread();
int schedbench_main(int argc, char **argv) {

    ompbench_init(argc, argv);

    /* GENERATE REFERENCE TIME */
    reference("reference time", &refer);

    /*REFERENCE PTHREAD*/
    // benchmark("pthreadrecreate", &testpthread);
    benchmark("dummypthread", &dummypthread);

    benchmark("pthreadonce",&testpthreadintegrated);
    /* TEST STATIC */
    benchmark("STATIC", &teststatic);

    /* TEST STATIC,n */
    cksz = 1;
    while (cksz <= itersperthr) {
	sprintf(testName, "STATIC %d", cksz);
	benchmark(testName, &teststaticn);
	cksz *= 2;
    }

    /* TEST DYNAMIC,n */
    cksz = 1;
    while (cksz <= itersperthr) {
	sprintf(testName, "DYNAMIC %d", cksz);
	benchmark(testName, &testdynamicn);
	cksz *= 2;
    }

    /* TEST GUIDED,n */
    cksz = 1;
    while (cksz <= itersperthr / nthreads) {
	sprintf(testName, "GUIDED %d", cksz);
	benchmark(testName, &testguidedn);
	cksz *= 2;
    }

    finalise();

    return EXIT_SUCCESS;

}

void integratedloop(){
  for(int j=0;j<innerreps;j++){
   for(int i=0;i<itersperthr;i++){
     delay(delaylength);
   }
  }
}


void runloop(){
   for(int i=0;i<itersperthr;i++){
     delay(delaylength);
   }
}
void dummy(){
  return;
}
static void dummypthread(){
     int i,j;   
     pthread_t threads[nthreads];
     for(i=0; i<nthreads; i++){
       pthread_create(&threads[i],NULL, &dummy,NULL);
     }
     for(i=0; i<nthreads; i++){
       pthread_join(threads[i],NULL);
     }
}


static void testpthreadintegrated(){
     int i,j;   
     pthread_t threads[nthreads];
     for(i=0; i<nthreads; i++){
       pthread_create(&threads[i],NULL, &integratedloop,NULL);
     }
     for(i=0; i<nthreads; i++){
       pthread_join(threads[i],NULL);
     }
}

static void testpthread(){
     int i,j;   
     pthread_t threads[nthreads];
     for(j=0; j < innerreps; j++){
         for(i=0; i<nthreads; i++){
           pthread_create(&threads[i],NULL, &runloop,NULL);
         }
         for(i=0; i<nthreads; i++){
            pthread_join(threads[i],NULL);
          }
     }
}

static void refer() {
    int i, j;
    for (j = 0; j < innerreps; j++) {
	for (i = 0; i < itersperthr; i++) {
	    delay(delaylength);
	}
    }
}

void teststatic() {

    int i, j;
#pragma omp parallel private(j)
    {
	for (j = 0; j < innerreps; j++) {
#pragma omp for schedule(static)
	    for (i = 0; i < itersperthr * nthreads; i++) {
		delay(delaylength);
	    }
	}
    }
}

void teststaticn() {
    int i, j;
#pragma omp parallel private(j)
    {
	for (j = 0; j < innerreps; j++) {
#pragma omp for schedule(static,cksz)
	    for (i = 0; i < itersperthr * nthreads; i++) {
		delay(delaylength);
	    }
	}
    }
}

void testdynamicn() {
    int i, j;
#pragma omp parallel private(j)
    {
	for (j = 0; j < innerreps; j++) {
#pragma omp for schedule(dynamic,cksz)
	    for (i = 0; i < itersperthr * nthreads; i++) {
		delay(delaylength);
	    }
	}
    }
}

void testguidedn() {
    int i, j;
#pragma omp parallel private(j)
    {
	for (j = 0; j < innerreps; j++) {
#pragma omp for schedule(guided,cksz)
	    for (i = 0; i < itersperthr * nthreads; i++) {
		delay(delaylength);
	    }
	}
    }
}
