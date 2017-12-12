#! /bin/bash

#ENC_VIDEO="-f h264 -re -i /tmp/out1.h264 -flags +global_header"
ENC_VIDEO="-f h264 -i /tmp/out1.h264 -flags +global_header"

#ENC_URL="rtmp://a.rtmp.youtube.com/live2/$YOUTUBE_KEY"

#ffmpeg -y $ENC_VIDEO -vcodec copy -an -f flv "$ENC_URL"
while true
do
    ffmpeg -y $ENC_VIDEO -vcodec copy -an -f flv tcp://0.0.0.0:5001?listen
    sleep 5
    echo "Restarting ..."
done
exit 0
