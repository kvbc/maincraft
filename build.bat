@echo off
:: GLFW
cmake -S lib/glfw-3.3.7 -B build/glfw -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF
make -C build/glfw
:: CGLM
cmake -S lib/cglm-0.8.5 -B build/cglm -G "MinGW Makefiles" -DCGLM_SHARED=OFF -DCGLM_STATIC=ON -DCGLM_USE_C99=OFF -DCGLM_USE_TEST=OFF
make -C build/cglm
:: MC
cmake -S . -B build/mc -G "MinGW Makefiles"
make -C build/mc
pause