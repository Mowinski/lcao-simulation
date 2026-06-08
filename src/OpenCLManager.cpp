#include "OpenCLManager.hpp"

OpenCLManager::OpenCLManager() : context(nullptr), queue(nullptr), program(nullptr), kernel(nullptr) {}

OpenCLManager::~OpenCLManager() {
    if (kernel) clReleaseKernel(kernel);
    if (program) clReleaseProgram(program);
    if (queue) clReleaseCommandQueue(queue);
    if (context) clReleaseContext(context);
}

bool OpenCLManager::init(const std::string& kernelPath) {
    cl_int err;

    // Get Platform
    err = clGetPlatformIDs(1, &platform, nullptr);
    if (err != CL_SUCCESS) { std::cerr << "OpenCL: Failed to find platform" << std::endl; return false; }

    // Get GPU Device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    if (err != CL_SUCCESS) {
        // Fallback to CPU if no GPU found
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, nullptr);
        if (err != CL_SUCCESS) { std::cerr << "OpenCL: Failed to find device" << std::endl; return false; }
    }

    // Create Context
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    if (err != CL_SUCCESS) { std::cerr << "OpenCL: Failed to create context" << std::endl; return false; }

    // Create Command Queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) { std::cerr << "OpenCL: Failed to create command queue" << std::endl; return false; }

    // Load and Compile Kernel
    std::ifstream file(kernelPath);
    if (!file.is_open()) { std::cerr << "OpenCL: Failed to open kernel file: " << kernelPath << std::endl; return false; }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string src = ss.str();
    const char* src_ptr = src.c_str();
    size_t src_size = src.length();

    program = clCreateProgramWithSource(context, 1, &src_ptr, &src_size, &err);
    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        std::vector<char> log(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
        std::cerr << "OpenCL: Build Error:\n" << log.data() << std::endl;
        return false;
    }

    kernel = clCreateKernel(program, "generate_points", &err);
    if (err != CL_SUCCESS) { std::cerr << "OpenCL: Failed to create kernel" << std::endl; return false; }

    std::cout << "OpenCL: Initialized successfully on device" << std::endl;
    return true;
}

std::string OpenCLManager::getErrorInfo(cl_int error) {
    switch(error) {
        case CL_SUCCESS: return "Success";
        case CL_DEVICE_NOT_FOUND: return "Device Not Found";
        case CL_DEVICE_NOT_AVAILABLE: return "Device Not Available";
        case CL_COMPILER_NOT_AVAILABLE: return "Compiler Not Available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "Memory Object Allocation Failure";
        case CL_OUT_OF_RESOURCES: return "Out of Resources";
        case CL_OUT_OF_HOST_MEMORY: return "Out of Host Memory";
        case CL_INVALID_VALUE: return "Invalid Value";
        case CL_INVALID_PLATFORM: return "Invalid Platform";
        case CL_INVALID_DEVICE: return "Invalid Device";
        default: return "Unknown Error (" + std::to_string(error) + ")";
    }
}
