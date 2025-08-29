#!/bin/bash

set -eo pipefail

# Function to display messages with different severity levels
# Usage: icinga2_log <severity> <message>
icinga2_log() {
    local severity="$1"
    local message="$2"

    local color=""
    local reset=""

    # Check if we are running in a terminal that supports colors,
    # otherwise fallback to plain text output.
    if [ -t 2 ]; then
        reset="\033[0m"
        # Set color codes based on severity
        case "$severity" in
            "information")
                color="\033[32m" # Green
                ;;
            "warning")
                color="\033[1;33m" # Yellow bold
                ;;
            "critical")
                color="\033[1;31m" # Red bold
                ;;
        esac
    fi

    # Print the message with the appropriate color and reset code to stderr
    echo -e "[$(date +'%Y-%m-%d %H:%M:%S %z')] ${color}${severity}${reset}/DockerEntrypoint: ${message}" >&2
}

# The entrypoint script expects at least one command to run.
if [ $# -eq 0 ]; then
    icinga2_log "critical" "Icinga 2 Docker entrypoint script requires at least one command to run."
    exit 1
fi

icinga2_log "information" "Icinga 2 Docker entrypoint script started."

ca="/var/lib/icinga2/certs/ca.crt"
if [ ! -e "$ca" ]; then
    nodeSetup=("node" "setup")
    runNodeSetup=false

    # The following loop looks for environment variables that start with ICINGA_ and applies some transformations
    # to the keys before processing them in one way or another. Their values are never modified or printed in
    # unintended ways. The key transformations have the following rules and are applied in the order they are listed:
    #
    # - Since it only processes environment variables that start with ICINGA_, it'll first strip that prefix.
    #   It then passes the key through awk to convert it to lowercase e.g. ICINGA_CN becomes cn.
    # - For each key, that hits one of the cases below, it will be processed a bit differently. In the first match,
    #   the environment variable is expected to be a boolean (1 or 0) and it only becomes part of the node setup
    #   command if and only if its value is 1. In that case, underscores in the key are replaced with dashes and
    #   passed as-is to the node setup command (e.g., ICINGA_ACCEPT_COMMANDS=1 becomes --accept-commands).
    # - The second case matches exclusively on the "global_zones" key. The value of this environment variable
    #   is expected to be one or comma-separated list of zones. If the value is not empty, it will be split by
    #   commas and each zone will be passed as a separate argument to the node setup command
    #   (e.g., ICINGA_GLOBAL_ZONES=icinga,example becomes --global_zones icinga --global_zones example).
    # - For the third match, the key is expected to be a key-value pair that should be passed to the node setup
    #   command. In this case, key and value pairs are passed as-is to the node setup command (e.g.,
    #   ICINGA_CN=icinga.example.com becomes --cn icinga.example.com).
    # - For the fourth match, the trusted certificate is expected to be a PEM-encoded certificate that should be
    #   written to a temporary file and passed to the node setup command.
    # - Lastly, the CA certificate is likewise expected to be a PEM-encoded certificate that should be written to
    #   the expected location at /var/lib/icinga2/certs/ca.crt.
    #
    # When encountering an environment variable prefixed with ICINGA_ that we don't know how to handle, we log it
    # as an informational message and continue processing the next environment variable but it doesn't cause the
    # script to fail.
    for k in "${!ICINGA_@}"; do
        # Strip the ICINGA_ prefix and convert the key to lowercase.
        key=$(echo "${k#ICINGA_}" | awk '{print tolower($0)}')
        # Get the value of the environment variable.
        value="${!k}"

        case "$key" in
            "accept_commands" | "accept_config" | "disable_confd" | "master")
                runNodeSetup=true
                if [ "$value" = "1" ]; then
                    nodeSetup+=("--${key//_/-}")
                fi
                ;;
            "global_zones")
                runNodeSetup=true
                # Split the value by commas and pass each part as a separate argument.
                IFS=',' read -ra zones <<< "$value"
                for zone in "${zones[@]}"; do
                    nodeSetup+=("--global_zones" "$zone")
                done
                ;;
            "cn" | "endpoint" | "listen" | "parent_host" | "parent_zone" | "zone" | "ticket")
                runNodeSetup=true
                nodeSetup+=("--$key" "$value")
                ;;
            "trustedcert")
                icinga2_log "information" "Writing trusted certificate to temporary file."
                runNodeSetup=true
                trustedCertFile=$(mktemp /tmp/trusted.cert.XXXXXXXX)
                printf "%s" "$value" > "$trustedCertFile"
                nodeSetup+=("--$key" "$trustedCertFile")
                chmod 0644 "$trustedCertFile"
                ;;
            "cacert")
                icinga2_log "information" "Writing CA certificate to $ca."
                runNodeSetup=true
                printf "%s" "$value" > "$ca"
                chmod 0644 "$ca"
                ;;
            *)
                # We don't know how to handle this environment variable, so log it and move on.
                icinga2_log "warning" "Ignoring unknown environment variable $k"
                ;;
        esac
    done

    if [ "$runNodeSetup" = true ]; then
        icinga2_log "information" "Running Icinga 2 node setup command..."

        icinga2 "${nodeSetup[@]}"
        # If the node setup command wasn't successful, we shouldn't reach this point due to set -e.
        icinga2_log "information" "Node setup completed successfully."
    else
        icinga2_log "information" "No node setup required based on environment variables."
    fi
fi

msmtprc_path="/var/lib/icinga2/.msmtprc"
# This script should initialize the container's msmtp configuration but never overwrite an existing configuration file.
# If the file already exists, it should not be modified, even if the MSMTPRC environment variable is set.
if [ ! -e "$msmtprc_path" ] && [ -n "${MSMTPRC}" ]; then
    icinga2_log "information" "Configuring msmtp with the provided MSMTPRC environment variable."
    printf "%s" "$MSMTPRC" > "$msmtprc_path"
    chmod 0644 "$msmtprc_path"
fi

icinga2_log "information" "Starting Icinga 2 daemon."

exec "$@"
