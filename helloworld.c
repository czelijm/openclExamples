//1 get platform ID

//2 get device ID

//3 Create contex

//4 Create and build a program object

//5 Create kernel objects

//6 Create buffer objects 

//8 Allocate data and transfer data between host memory and compute device memory

//7 Create image objects

//9 Allocate image data and transfer image between host memory and compute device memory

//10 Create a command queue

//11 Submit a kernel execution command to a compute device via a command queue

//12 Read data back from compute device memory to host memory

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <fcntl.h>
// #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DATA_SIZE 1024

////////////////////////////////////////////////////////////////////////////////
// Simple compute kernel which computes the square of an input array 
//
const char *KernelSource = "\n" \
"__kernel void square(                                                     \n" \
"   __global float* input,                                              \n" \
"   __global float* output,                                             \n" \
"   const unsigned int count)                                           \n" \
"{                                                                      \n" \
"   int i = get_global_id(0);                                           \n" \
"   if(i < count)                                                       \n" \
"       output[i] = input[i] * input[i];                                \n" \
"}                                                                      \n" \
"\n";
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    int dataSize = 1024;
    int err;                            // error code returned from api calls

    float data[DATA_SIZE];              // original data set given to device
    float results[DATA_SIZE];           // results returned from device
    unsigned int correct;               // number of correct results returned

    size_t global;                      // global domain size for our calculation
    size_t local;                       // local domain size for our calculation

    cl_uint numPlatforms;               // numeber of platforms
    cl_platform_id* platforms;          // platforms
    cl_device_id device_id;            // compute device id
    // cl_context context;                 // compute context
    cl_program program;                 // compute program
    cl_kernel kernel;                   // compute kernel
    // cl_command_queue commands;          // compute command queue

    cl_mem input;                       // device memory used for the input array
    cl_mem output;                      // device memory used for the output array



// Fill our data set with random float values
    //
    int i = 0;
    unsigned int count = DATA_SIZE;
    for(i = 0; i < count; i++){
        data[i] = rand() / (float)RAND_MAX;
       // printf("%f",data[i]);
    }


//1 get platform ID
//1.1 gen number of platforms
    cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);
    if (status != CL_SUCCESS) {
        printf("Error: Getting platforms!");
        return -1;
    }

    if(numPlatforms > 0)  {
        platforms = (cl_platform_id* )malloc(numPlatforms* sizeof(cl_platform_id));
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
        printf("Number of plat %d \n",numPlatforms);

    }
//--------------------------------------------------------------------------------------------------------
//2 get device ID; get all avaible devices for all avaible platorms 
    int gpu = 1;
    cl_uint numDevices[numPlatforms];
    cl_device_id *devices[numPlatforms];

    for (int i = 0; i < (int)numPlatforms; i++)
    {  
        numDevices[i] = 0;
        status = clGetDeviceIDs(platforms[i], gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 0, NULL, &numDevices[i]); 
        printf("Number of the devices %d \n",numDevices[i]);
        if (status != CL_SUCCESS)
        {
            printf("Error: Failed to create a device group!\n");
            return EXIT_FAILURE;
        }

        devices[i] = (cl_device_id* )malloc(numDevices[i]* sizeof(cl_device_id));

        if (numDevices[i] > 0) 
        {
            devices[i] = (cl_device_id*)malloc(numDevices[i] * sizeof(cl_device_id));
            status = clGetDeviceIDs(platforms[i], gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, numDevices[i], devices[i], NULL);
        }
        if (status != CL_SUCCESS)
        {
            printf("Error: Failed to create a device group!\n");
            return EXIT_FAILURE;
        }
        // for (size_t j = 0; j < numDevices[i]; j++)
        // {
        //     printf("Number of the device %d \n",(int)devices[j]);
        // }
    }
//--------------------------------------------------------------------------------------------------------    
//3 Create contex; for all platforms create contex
    cl_context contexts[numPlatforms];

    for (int i = 0; i <numPlatforms; i++)
    {  
       contexts[i] = clCreateContext(NULL, numDevices[i], devices[i], NULL, NULL, &status);
        if (!contexts[i] || status!=CL_SUCCESS)
        {
            printf("Error: Failed to create a compute context!\n");
            return EXIT_FAILURE;
        }
    }
//--------------------------------------------------------------------------------------------------------
//4 Create and build a program object
//4.1 Create the compute program from the source buffer
//
    //program for platform0
    int platformNbr = 0;
    int deviceNbr = 0;
    program = clCreateProgramWithSource(contexts[platformNbr], 1, (const char **) & KernelSource, NULL, &status);
    if (!program  || status!=CL_SUCCESS)
    {
        printf("Error: Failed to create compute program!\n");
        return EXIT_FAILURE;
    }
//4.2 Build the program executable
//The third argument (device_list) is a pointer to an array of device IDs. If this argument is NULL, it means the program is built for all devices in the associated context. 
    status = clBuildProgram(program, numDevices[platformNbr], devices[platformNbr], NULL, NULL, NULL);
    if (status != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];

        printf("Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, devices[platformNbr][deviceNbr], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        exit(1);
    }
//5 Create kernel objects
kernel = clCreateKernel(program, "square", &status);
    if (!kernel || status != CL_SUCCESS)
    {
        printf("Error: Failed to create compute kernel!\n");
        exit(1);
    }

//10 Create a command queue
    cl_command_queue *commandsArray[numPlatforms]; 

    for (int i = 0; i <numPlatforms; i++)
    {  
        commandsArray[i] = (cl_command_queue* )malloc(numDevices[i]* sizeof(cl_command_queue));
        for (int j = 0; j <numDevices[i]; j++)
        {
            commandsArray[i][j] = clCreateCommandQueue(contexts[i], devices[i][j], 0, &status);        
            if (!commandsArray[i][j] || status!=CL_SUCCESS )
            {
                printf("Error: Failed to create a command commands!\n");
                return EXIT_FAILURE;
            }
        }
    }


//6 Create buffer objects 

    // Create the input and output arrays in device memory for our calculation
    input = clCreateBuffer(contexts[platformNbr],  CL_MEM_READ_ONLY,  sizeof(float) * count, NULL, &status);
        if (!input || status != CL_SUCCESS)
    {
        printf("Error: Failed to allocate device memory!\n");
        exit(1);
    } 
    output = clCreateBuffer(contexts[platformNbr], CL_MEM_WRITE_ONLY, sizeof(float) * count, NULL, &status);
    if (status != CL_SUCCESS || !output)
    {
        printf("Error: Failed to allocate device memory!\n");
        exit(1);
    }   

//8 Allocate data and transfer data between host memory and compute device memory
// Write our data set into the input array in device memory 
    //Write for all devices for particular platform
    // for (int j = 0; j <numDevices[platformNbr]; j++)
    // {
        status = clEnqueueWriteBuffer(commandsArray[platformNbr][deviceNbr], input, CL_TRUE, 0, sizeof(float) * count, data, 0, NULL, NULL);
        if (status != CL_SUCCESS)
        {
            printf("Error: Failed to write to source array!\n");
            exit(1);
        }
    // }
//5.1 set kernel arguments
    // Set the arguments to our compute kernel
    //
    status = 0;
    status  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
    status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);
    status |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &count);
    if (status != CL_SUCCESS)
    {
        printf("Error: Failed to set kernel arguments! %d\n", status);
        exit(1);
    }

// Get the maximum work group size for executing the kernel on the device - device 0 in platform 0
    status = clGetKernelWorkGroupInfo(kernel, devices[platformNbr][deviceNbr], CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
    if (status != CL_SUCCESS)
    {
        printf("Error: Failed to retrieve kernel work group info! %d\n", status);
        exit(1);
    }
    // local = 8;
    printf("Work group size for executing the kernel  on the device %ld \n",local);

//11 Submit a kernel execution command to a compute device via a command queue
// Execute the kernel over the entire range of our 1d input data set
    // using the maximum number of work group items for this device
    //
    
    global = count;
    for (int j = 0; j <numDevices[platformNbr]; j++)
    {  
        status = clEnqueueNDRangeKernel(commandsArray[platformNbr][j], kernel, 1, NULL, &global, &local, 0, NULL, NULL);
        if (status)
        {
            printf("Error: Failed to execute kernel!\n");
            return EXIT_FAILURE;
        }
    }

//12 Wait for the command commands to get serviced before reading back results
//
    for (int j = 0; j <numDevices[platformNbr]; j++)
    {  
        clFinish(commandsArray[platformNbr][j]);
    }
//13 Read data back from compute device memory to host memory
    // for (int j = 0; j <numDevices[platformNbr]; j++)
    // {
        status = clEnqueueReadBuffer(commandsArray[platformNbr][deviceNbr], output, CL_TRUE, 0, sizeof(float) * count, results, 0, NULL, NULL );  
        if (status!= CL_SUCCESS)
        {
            printf("Error: Failed to read output array! %d\n", status);
            exit(1);
        }
    // }
//14 Validate our results
    //
    correct = 0;
    for(i = 0; i < count; i++)
    {
         printf("Result '%f' \n", results[i]);
        if(results[i] == data[i] * data[i])
            correct++;
    }
    
    // Print a brief summary detailing the results
    //
    printf("Computed '%d/%d' correct values!\n", correct, count);

// Shutdown and cleanup
    //
    clReleaseMemObject(input);
    clReleaseMemObject(output);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    for (int i = 0; i <numPlatforms; i++)
    {  
        for (int j = 0; j <numDevices[i]; j++)
        {
            clReleaseCommandQueue(commandsArray[platformNbr][j]);
        }
        
    }
    for (int i = 0; i <numPlatforms; i++)
    {  
    // clReleaseCommandQueue(commands);
        clReleaseContext(contexts[i]);
    }


//7 Create image objects

//9 Allocate image data and transfer image between host memory and compute device memory

    return 0;
}