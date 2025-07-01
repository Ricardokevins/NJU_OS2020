#!/bin/bash

# --- Configuration ---
FILE_TO_WATCH="/home/ubuntu/uniapi/api.yaml"
COMMAND_TO_RUN="sudo docker restart uniapi_uniapi_1"
DELAY_SECONDS=1800 # 30 minutes = 1800 seconds
LOCK_FILE="/tmp/file_monitor.lock"
# --- End of Configuration ---

echo "Monitoring $FILE_TO_WATCH for changes..."

# Use a loop to keep monitoring after an event is handled.
# inotifywait will block until an event occurs.
# -q makes it quiet.
# -e modify,create,delete,move makes it watch for these specific events.
while inotifywait -q -e modify,create,delete,move "$FILE_TO_WATCH"; do
    # Check if a restart is already scheduled by checking the lock file.
    if [ -f "$LOCK_FILE" ]; then
        echo "Change detected, but a restart is already scheduled. Ignoring."
        continue
    fi

    echo "Change detected on $(date). Scheduling command execution in $DELAY_SECONDS seconds."

    # Create the lock file to indicate a pending action.
    touch "$LOCK_FILE"

    # Start the delayed action in a background subshell.
    # The '&' is crucial. It runs this block in the background,
    # allowing the main 'while' loop to continue immediately if needed,
    # though inotifywait will block it again anyway.
    (
        sleep "$DELAY_SECONDS"
        echo "Executing command: $COMMAND_TO_RUN"
        
        # Execute the actual command.
        eval "$COMMAND_TO_RUN"
        
        # Remove the lock file after the command is done,
        # allowing new changes to schedule a new restart.
        rm -f "$LOCK_FILE"
    ) &
done
