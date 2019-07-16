# video-qsv-vp9-encoder

docker run --rm -ti --init --net=host --ipc=host -v /tmp:/tmp --device /dev/dri/renderD128 -v $PWD:/data qsv --cid=111 --name=video0.i420 --width=640 --height=480 --gop=1 --verbose
