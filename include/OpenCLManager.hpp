#ifndef OPENCL_MANAGER_HPP
#define OPENCL_MANAGER_HPP

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

class OpenCLManager {
public:
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_device_id device;

    OpenCLManager();
    ~OpenCLManager();

    bool init(const std::string& kernelPath);
    std::string getErrorInfo(cl_int error);

private:
    cl_platform_id platform;
};

#endif
