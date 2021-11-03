#! /bin/bash

set -e

OUTDIR=`pwd`

# List known formats with: ffmpeg -pix_fmts

ffmpeg_raw_test_files()
{
echo
echo
pix_fmt=$1
out_file=$2
set -x
ffmpeg -y -vcodec png -i PM5544_MK10.png  -vcodec rawvideo -f rawvideo -pix_fmt $pix_fmt $out_file
set +x
}

gst_raw_test_files()
{
echo
echo
pix_fmt=$1
out_file=$2
set -x
gst-launch-1.0 -v filesrc location=PM5544_MK10.png ! decodebin ! videorate ! videoconvert ! video/x-raw,format=$pix_fmt,framerate=1/1 ! filesink location=$out_file
set +x
}


ffmpeg_raw_test_files "rgb565"  "$OUTDIR/PM5544_MK10_RGB565.raw"
ffmpeg_raw_test_files "bgr565"  "$OUTDIR/PM5544_MK10_BGR565.raw"
ffmpeg_raw_test_files "yuyv422" "$OUTDIR/PM5544_MK10_YUYV422.raw"
# no supported: ffmpeg_raw_test_files "yvyu422" "$OUTDIR/PM5544_MK10_YVYU422.raw"
ffmpeg_raw_test_files "uyvy422" "$OUTDIR/PM5544_MK10_UYVY422.raw"
# not supported: ffmpeg_raw_test_files "vyuy422" "$OUTDIR/PM5544_MK10_VYUY422.raw"

gst_raw_test_files    "RGBA"    "$OUTDIR/PM5544_MK10_RGBA8888.raw"
gst_raw_test_files    "ARGB"    "$OUTDIR/PM5544_MK10_ARGB8888.raw"

gst_raw_test_files    "BGRA"    "$OUTDIR/PM5544_MK10_BGRA8888.raw"
gst_raw_test_files    "ABGR"    "$OUTDIR/PM5544_MK10_ABGR8888.raw"

gst_raw_test_files    "NV12"    "$OUTDIR/PM5544_MK10_NV12.raw"
gst_raw_test_files    "NV21"    "$OUTDIR/PM5544_MK10_NV21.raw"

gst_raw_test_files    "NV16"    "$OUTDIR/PM5544_MK10_NV16.raw"
gst_raw_test_files    "NV61"    "$OUTDIR/PM5544_MK10_NV61.raw"
gst_raw_test_files    "YV12"    "$OUTDIR/PM5544_MK10_YV12.raw"
gst_raw_test_files    "I420"    "$OUTDIR/PM5544_MK10_I420.raw"
echo "done"
