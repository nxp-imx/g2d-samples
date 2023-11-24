# G2D samples

**Introduction**

demo for g2d usage.

**Building for Linux**

1. Install Yocto SDK under /opt

   For example, installed 5.10-hardknott to  /opt/fsl-imx-internal-xwayland/5.10-hardknott

2. Setup build environment
  ```
source /opt/fsl-imx-internal-xwayland/5.10-hardknott/environment-setup-aarch64-poky-linux
  ```
   Configure build for G2D implementation
  ```
export BUILD_IMPLEMENTATION=dpu
export BUILD_IMPLEMENTATION=dpu95
export BUILD_IMPLEMENTATION=gpu-drm
export BUILD_IMPLEMENTATION=gpu-fbdev
export BUILD_IMPLEMENTATION=pxp
  ```
3. Build
  ```
DESTDIR=/opt/rootfs make clean
DESTDIR=/opt/rootfs make install
  ```

4. Run, test availability depends on BUILD_IMPLEMENTATION
  ```
$./g2d_basic_test
$./g2d_multiblit_test
$./g2d_wayland_cf_test
$./g2d_wayland_dmabuf_test
$./g2d_wayland_shm_test
  ```

The jpg file can be found on the Internet, just make sure the resolution is correct.
Prepare the 1024x768-rgb565.rgb, 800x600-bgr565.rgb, 480x360-bgr565.rgb, 352x288-yuyv.yuv, 352x288-nv16.yuv, 176x144-yuv420p.yuv with below cmd.

  ```
$ffmpeg -i 1024x768.jpg -pix_fmt rgb565le 1024x768-rgb565.rgb
$ffmpeg -i 800x600.jpg -pix_fmt bgr565le 800x600-bgr565.rgb
$ffmpeg -i 480x360.jpg -pix_fmt bgr565le 480x360-bgr565.rgb
$ffmpeg -i 352x288.jpg -pix_fmt yuyv422 352x288-yuyv.yuv
$ffmpeg -i 176x144.jpg -pix_fmt yuv420p 176x144-yuv420p.yuv
$gst-launch-1.0 videotestsrc num-buffers=1 !  \
    video/x-raw,format=NV16,width=352,height=288 ! \
    filesink location=352x288-nv16.yuv
$./g2d_overlay_test
  ```

-s   source's width and height

-d   dest's width and height

-w   line stride of source and dest, bytes unit.

  ```
$./g2d_yuv_test -s 1024x768 -d 1024x768 -w 1024x1024  -i PM5544_MK10_YUYV422.raw -f yuyv-yu12
  ```

**Building for QNX**

1. Setup build environment
  ```
source /qnx700/qnxsdp-env.sh
  ```

2. Build
  ```
make -f Makefile.qnx
  ```

3. Run
  ```
$./g2d_basic_test
$./g2d_multiblit_test
$./g2d_overlay_test
  ```

See instruction for linux how to build source files with image data.

**Additional Notes:**
  - There is no source distribution available for Android BSPs.  The driver requires specific integration into the Android OS and is not available as a separate source package.
