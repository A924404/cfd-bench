# For NVIDIA GPUs

* Using CUDA:
    ```bash
    export CUDA_VISIBLE_DEVICES=<gpu id>
    python3 autobench.py cuda --output v100_cuda.csv --repeat 10 --nvidia-sm 70 --extra-compile-flags="CC=nvcc"
    ```

* Using SYCL:
    ```bash
    export ONEAPI_DEVICE_SELECTOR=cuda:<cuda id>
    python3 autobench.py sycl --output v100_sycl.csv --repeat 10 --sycl-type cuda --nvidia-sm 70 --extra-compile-flags="CC=icpx"
    ```

# For AMD GPUs

* Using HIP:
    ```bash
    python3 autobench.py hip --output 6700XT_hip.csv --repeat 10 --amd-arch gfx1031 --extra-compile-flags="CC=hipcc"
    ```    

* Using SYCL:
    ```bash
    export ONEAPI_DEVICE_SELECTOR=hip:<hip id>
    python3 autobench.py sycl --output 6700XT_sycl.csv --repeat 10 --sycl-type hip --amd-arch gfx1031 --extra-compile-flags="CC=icpx"
    ```

# For Intel GPUs

* Using OpenCL:
    ```bash
    export ONEAPI_DEVICE_SELECTOR=opencl:<opencl id>
    python3 autobench.py sycl --output intel_max_ocl.csv --repeat 10 --sycl-type opencl --extra-compile-flags="CC=icpx"
    ```

* Using Level0:
    ```bash
    export ONEAPI_DEVICE_SELECTOR=level_zero:<level0 id>
    python3 autobench.py sycl --output intel_max_l0.csv --repeat 10 --sycl-type opencl --extra-compile-flags="CC=icpx"
    ```