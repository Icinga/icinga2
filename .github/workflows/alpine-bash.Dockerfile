# This Dockerfile is used in the linux job for Alpine Linux.
#
# As the linux.bash script is, in fact, a bash script and Alpine does not ship
# a bash by default, the "alpine:bash" container will be built using this
# Dockerfile in the GitHub Action.

FROM alpine:3
RUN ["apk", "--no-cache", "add", "bash"]
