/**********
Copyright (c) 2016, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

#include <iostream>
#include <cstring>
#include <stdio.h>

//OpenCL utility layer include
#include "xcl.h"
#include "host.h"

//Utilily to print array
void print_array(DTYPE *mat, const char *name, int size, int dim) {
    int i;
    printf("%s\n", name);
    for (i=0;i<size;i++) {
      printf("%d ",mat[i]);
      if (((i+1) % dim) == 0)
        printf("\n");
    }
}

int main(int argc, char** argv)
{
    //Allocate Memory in Host Memory
    size_t vector_size_bytes = sizeof(DTYPE) * BLOCK_SIZE;
    DTYPE* a = (DTYPE*)malloc(vector_size_bytes);// original data set given to device
    DTYPE* c = (DTYPE*)malloc(vector_size_bytes);// results returned from device
    DTYPE* sw_c = (DTYPE*)malloc(vector_size_bytes);// results returned from software

    // Create the test data and Software Result 
    DTYPE alpha = 3;
    for(int i = 0; i < BLOCK_SIZE; i++) {
      a[i] = i;
      c[i] = 0;
      sw_c[i] = alpha*a[i];
    }

//OPENCL HOST CODE AREA START
    //Create Program and Kernel
    xcl_world world = xcl_world_single();
    cl_program program = xcl_import_binary(world, "window_array_2d");
    cl_kernel krnl_window_array_2d = xcl_get_kernel(program, "window_array_2d");

    //Allocate Buffer in Global Memory
    cl_mem buffer_a = xcl_malloc(world, CL_MEM_READ_ONLY, vector_size_bytes);
    cl_mem buffer_c = xcl_malloc(world, CL_MEM_WRITE_ONLY, vector_size_bytes);

    //Copy input data to device global memory
    xcl_memcpy_to_device(world,buffer_a,a,vector_size_bytes);

    //Set the Kernel Arguments
    xcl_set_kernel_arg(krnl_window_array_2d,0,sizeof(cl_mem),&buffer_a);
    xcl_set_kernel_arg(krnl_window_array_2d,1,sizeof(cl_mem),&buffer_c);
    xcl_set_kernel_arg(krnl_window_array_2d,2,sizeof(DTYPE),&alpha);

    //Launch the Kernel
    xcl_run_kernel3d(world,krnl_window_array_2d,1,1,1);


    //Copy Result from Device Global Memory to Host Local Memory
    xcl_memcpy_from_device(world, c, buffer_c ,vector_size_bytes);
    clFinish(world.command_queue);

    //Release Device Memories and Kernels
    clReleaseMemObject(buffer_a);
    clReleaseMemObject(buffer_c);
    clReleaseKernel(krnl_window_array_2d);
    xcl_release_world(world);
//OPENCL HOST CODE AREA END


    //uncomment following 3 lines if you want to check all values
    //print_array(a, "A", BLOCK_SIZE, 16);
    //print_array(c, "Test C", BLOCK_SIZE, 16);
    //print_array(sw_c, "Gold C", BLOCK_SIZE, 16);

    // Validate
    unsigned int correct = 0;              // number of correct results returned
    for (int i = 0;i < BLOCK_SIZE; i++) {
      if(c[i] == sw_c[i]) {
        correct++; 
      } else { 
        printf("\n wrong sw %d hw %d index %d \n", sw_c[i], c[i], i);
      }
    }
    
    // Print a brief summary detailing the results
    printf("Computed '%d/%d' correct values!\n", correct, BLOCK_SIZE);

    free(a);
    free(c);
    free(sw_c);
    
    if(correct == BLOCK_SIZE){
        std::cout << "TEST PASSED." << std::endl; 
        return EXIT_SUCCESS;
    }
    else{
        std::cout << "TEST FAILED." << std::endl; 
        return EXIT_FAILURE;
    }
}
