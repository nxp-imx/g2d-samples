# G2D samples

**Introduction**

demo for g2d usage.

**Build**

1. Install Yocto SDK under /opt

   For example, installed 5.10-hardknott to  /opt/fsl-imx-internal-xwayland/5.10-hardknott

2. Setup build environment
  ```
source /opt/fsl-imx-internal-xwayland/5.10-hardknott/environment-setup-aarch64-poky-linux
  ```
3. make
  ```
DESTDIR=/opt/rootfs make clean
DESTDIR=/opt/rootfs make install
  ```

4. run
  ```
$./g2d_basic_test
$./g2d_multiblit_test
$./g2d_wayland_cf_test
$./g2d_wayland_dmabuf_test
$./g2d_wayland_shm_test
  ```

the jpg file can be found in the internet, the only thing you shall be carefull is the resolution is right.
prepare the 1024x768-rgb565.rgb, 800x600-bgr565.rgb, 480x360-bgr565.rgb, 352x288-yuyv.yuv, 352x288-nv16.yuv, 176x144-yuv420p.yuv with below cmd.

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
$./g2d_yuv_test -s 1920x1080 -d 1920x1080 -w 3840x1920 -i PM5544_MK10_YUYV422.raw -f yuyv-yu12
  ```

**Additional Notes:**
  - There is no source distribution available for Android BSPs.  The driver requires specific integration into the Android OS and is not available as a separate source package.
