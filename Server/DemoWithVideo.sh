PORT_MEMBER_FILE="/home/lg/artifacts/port_info/port_member.txt"
VIDEO_PORT=5000

if [ -e "${PORT_MEMBER_FILE}" ]; then
    PORT_OF_MEMBER=$(cat ${PORT_MEMBER_FILE} | grep -w ${USER} | awk -F: '{print $2}')
    if [ ! -z "${PORT_OF_MEMBER}" ]; then
        VIDEO_PORT=${PORT_OF_MEMBER}
    fi
fi

if [ "$1" != "" ]; then
    VIDEO_PORT="$1"
fi

echo "PORT NUMBER: "${VIDEO_PORT}
./LgFaceRecDemoTCP ${VIDEO_PORT} friends_960x540_98_12fps.smjpeg
