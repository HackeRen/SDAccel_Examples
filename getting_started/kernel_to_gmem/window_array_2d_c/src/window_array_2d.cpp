/*******************************************************************************
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
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/*******************************************************************************
Description: 
C Kernel Example using AXI4-master interface to access window of data from 2D array
*******************************************************************************/

//Includes 
#include <stdio.h>
#include <string.h>
#include "assert.h"
#include "host.h"

// Read data function : Read tile/window of Data from Global Memory
void read_data(DTYPE *inx, my_data_fifo &inFifo) {
    DTYPE tile[TILE_HEIGHT][WORD_GROUP_SIZE];
    rd_loop_i: for(int i = 0; i < TILE_PER_COLUMN; ++i) {
        rd_loop_j: for (int j = 0; j < WORD_GROUP_PER_ROW; ++j) {
#pragma HLS DATAFLOW
            rd_buf_loop_m: for (int m = 0; m < TILE_HEIGHT; ++m) {
                rd_buf_loop_n: for (int n = 0; n < WORD_GROUP_SIZE; ++n) {
#pragma HLS PIPELINE
                    // should burst WORD_GROUP_SIZE in WORD beat
                    tile[m][n] = inx[TILE_HEIGHT*WORD_GROUP_PER_ROW*WORD_GROUP_SIZE*i+WORD_GROUP_PER_ROW*WORD_GROUP_SIZE*m+WORD_GROUP_SIZE*j+n];
                }
            }
            rd_loop_m: for (int m = 0; m < TILE_HEIGHT; ++m) {
                rd_loop_n: for (int n = 0; n < WORD_GROUP_SIZE; ++n) {
#pragma HLS PIPELINE
                    inFifo << tile[m][n];
                }
            }
        }
    }
}

// Write data function : Write tile/window of Results to Global Memory
void write_data(DTYPE *outx, my_data_fifo &outFifo) {
    DTYPE tile[TILE_HEIGHT][WORD_GROUP_SIZE];
    wr_loop_i: for(int i = 0; i < TILE_PER_COLUMN; ++i) {
        wr_loop_j: for (int j = 0; j < WORD_GROUP_PER_ROW; ++j) {
#pragma HLS DATAFLOW
            wr_buf_loop_m: for (int m = 0; m < TILE_HEIGHT; ++m) {
                wr_buf_loop_n: for (int n = 0; n < WORD_GROUP_SIZE; ++n) {
#pragma HLS PIPELINE
                    // should burst WORD_GROUP_SIZE in WORD beat
                    outFifo >> tile[m][n];
                }
            }
            wr_loop_m: for (int m = 0; m < TILE_HEIGHT; ++m) {
                wr_loop_n: for (int n = 0; n < WORD_GROUP_SIZE; ++n) {
#pragma HLS PIPELINE
                    outx[TILE_HEIGHT*WORD_GROUP_PER_ROW*WORD_GROUP_SIZE*i+WORD_GROUP_PER_ROW*WORD_GROUP_SIZE*m+WORD_GROUP_SIZE*j+n] = tile[m][n];
                }
            }
        }
    }
}

// Compute function, currently as simple as possible because this example is focused on efficient memory access pattern.
void compute(my_data_fifo &inFifo, my_data_fifo &outFifo, DTYPE alpha) {
    compute_loop_i: for(int i = 0; i < NUM_ROWS; ++i) {
        compute_loop_jj: for (int jj = 0; jj < WORD_GROUP_PER_ROW; ++jj) {
            compute_loop_m: for (int m = 0; m < WORD_GROUP_SIZE; ++m) {
#pragma HLS PIPELINE
                DTYPE inTmp;
                inFifo >> inTmp;
                DTYPE outTmp = inTmp * alpha;
                outFifo << outTmp;
            }
        }
    }
}

extern "C" {
    void window_array_2d(DTYPE *inx, DTYPE *outx, DTYPE *alpha) {
// AXI master interface
#pragma HLS INTERFACE m_axi port=inx offset=slave bundle=gmem 
#pragma HLS INTERFACE m_axi port=outx offset=slave bundle=gmem 
// AXI slave interface
#pragma HLS INTERFACE s_axilite port=inx bundle=control
#pragma HLS INTERFACE s_axilite port=outx bundle=control
#pragma HLS INTERFACE s_axilite port=alpha bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

        my_data_fifo inFifo;
#pragma HLS stream variable=inFifo depth=4096
// User can change the FIFO depth if array size is not 64*64, but currently we have limitation here due to some issues in tool.
// The FIFO depth can NOT be smaller than the full array size, namely BLOCK_SIZE defined in header file "window_array_2d.h"
        my_data_fifo outFifo;

// Enables task level pipelining, allowing functions and loops to execute concurrently. Used to minimize interval. More details please refer to UG902.
#pragma HLS DATAFLOW
        // Read data from 2D array using tile/window pattern
        read_data(inx, inFifo);
        // Do computation with the acquired data
        compute(inFifo, outFifo, *alpha);
        // Write data to 2D array using tile/window pattern
        write_data(outx, outFifo);
        return;
    }
}