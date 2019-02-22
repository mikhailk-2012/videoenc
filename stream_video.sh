#! /bin/bash

FILE=/tmp/out1.h264

while true
do
if [ -e $FILE ]; then
    gst-launch-1.0 -v filesrc location=$FILE do-timestamp=true ! h264parse ! 'video/x-h264, alignment=(string)au' ! flvmux streamable=true ! tcpserversink port=5001 host=0.0.0.0
#    ffmpeg -y -use_wallclock_as_timestamps 0 -fflags +genpts -f h264 -i $FILE -vcodec copy -bsf:v h264_mp4toannexb -an -f flv tcp://0.0.0.0:5001?listen

fi
    sleep 5
    echo "Restarting ..."
done
exit 0
