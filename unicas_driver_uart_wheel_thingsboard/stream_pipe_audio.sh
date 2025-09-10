#!/bin/bash

# Start me in a screen via cron!
# Add to crontab -e
# @reboot sleep 20 && /usr/bin/screen -dmS stream_control /path to your script/unicas_driver_uart_wheel_thingsboard/stream_pipe_audio.sh

PIPE_PATH="/tmp/stream_control_pipe"

# Create the named pipe if it doesn't exist
if [[ ! -p "$PIPE_PATH" ]]; then
    mkfifo "$PIPE_PATH"
fi

# Store the PID of the ffmpeg process
FFMPEG_PID=""

start_stream() {
    if [ -z "$FFMPEG_PID" ] || ! kill -0 "$FFMPEG_PID" 2>/dev/null; then
        parec --format=s16le --rate=16000 --channels=1 | ffmpeg -f s16le -ar 16000 -ac 1 -i -   -c:a libopus -b:a 32k -f rtp rtp://192.168.1.5:5004 &
        FFMPEG_PID=$!
        echo "Streaming started. PID: $FFMPEG_PID"
    else
        echo "Stream is already running."
    fi
}

pause_stream() {
    if [ -n "$FFMPEG_PID" ]; then
        kill -SIGSTOP "$FFMPEG_PID"
        echo "Streaming paused."
    fi
}

resume_stream() {
    if [ -n "$FFMPEG_PID" ] && kill -0 "$FFMPEG_PID" 2>/dev/null; then
        kill -SIGCONT "$FFMPEG_PID"
        echo "Streaming resumed."
    else
        echo "Stream not running. Starting new stream..."
        start_stream
    fi
}


quit_stream() {
    if [ -n "$FFMPEG_PID" ]; then
        kill "$FFMPEG_PID"
        FFMPEG_PID=""
        echo "Streaming stopped."
    fi
}

handle_command() {
    local input="$1"
    case $input in
        s | start) start_stream ;;
        p | pause) pause_stream ;;
        r | resume) resume_stream ;;
        q | quit) quit_stream ;;
        *) echo "Unknown command: $input" ;;
    esac
}

# Trap to clean up the named pipe on exit
cleanup() {
    echo "Cleaning up..."
    rm -f "$PIPE_PATH"
    exit 0
}
trap cleanup EXIT

# Background: Listen to the named pipe
while true; do
    if read -r cmd < "$PIPE_PATH"; then
        handle_command "$cmd"
    fi
done &

#💻 Foreground: Read user input
while true; do
    echo "Press 's' to start, 'p' to pause, 'r' to resume, or 'q' to quit:"
    read -n 1 key
    echo ""
    handle_command "$key"
done
