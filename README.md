# video-qsv-vp9-encoder

docker run --rm -ti --init --ipc=host -v /mnt/Volume1/data:/tmp -v /mnt/Volume1/data/lossy:/data -w /data --net=host --device /dev/dri/renderD128 qsv-vp9-recorder:latest --cid=111 --name=ptg-right.i420 --width=2048 --height=1536
