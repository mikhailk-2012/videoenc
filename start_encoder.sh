#! /bin/bash

FFMPEG=ffmpeg
SRC_VIDEO=/dev/video0
ROOT_DIR="`pwd`"

#ENC_TP="in-enc"
ENC_TP="v4l2-enc"

echo "Encoder Starting, ROOT=[$ROOT_DIR]"
#while true
#do
   
    case $ENC_TP in

	# FFMPEG as an Input Source to RAW video...
	'in-enc' )
            rm -rf /tmp/out*.h264 /tmp/out*.nv12
            mkfifo /tmp/out1.h264
            mkfifo /tmp/out2.nv12
            echo "Starting H264 Encoder..."
	    `$FFMPEG -f v4l2 -input_format yuyv422 -r 15 -s 320x240 -i $SRC_VIDEO -pix_fmt yuv420p -an -r 15 -f rawvideo /tmp/out2.nv12 &`
sleep 3
$ROOT_DIR/videoenc -i /tmp/out2.nv12 -k 2 -r 15 -b 1024 -s 320x240 -o /tmp/out1.h264
#	    gst-launch-1.0 -v videotestsrc ! filesink location="/tmp/out2.nv12" sync=true append=true
#	    $ROOT_DIR/videoenc -i /tmp/out2.nv12 -k 2 -r 15 -b 1024 -s 320x240 -o /tmp/out1.h264
    	    ;;
	
	# using V4l2 as input source...
	'v4l2-enc' )
            rm -rf /tmp/out*.h264
	    sleep 1
            mkfifo /tmp/out1.h264
            echo "Starting H264 Encoder..."
#	    $ROOT_DIR/videoenc -i $SRC_VIDEO -k 4 -r 15 -b 4000 -q 10,50 -s 960x544 -o /tmp/out1.h264
	    $ROOT_DIR/videoenc -i /dev/video2 -k 8 -r 25 -b 5000 -q 10,40 -s 720x576 -o /tmp/out1.h264

    	    ;;
    esac
#    sleep 5
#    echo "Restarting..."
#done
rm -rf /tmp/out*.h264
exit 0
