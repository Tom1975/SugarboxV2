# Download base image ubuntu 23.10
FROM ubuntu:23.10

# LABEL about the custom image
LABEL maintainer="thomas.guillemin@gmail.com"
LABEL version="0.1"
LABEL description="This is custom Docker Image for \
building SugarBox V2 for Ubuntu

# Disable Prompt During Packages Installation
ARG DEBIAN_FRONTEND=noninteractive


# Update Ubuntu Software repository
RUN apt-get update && apt-get -qq install wget 
RUN sudo apt-get update
RUN sudo apt install qt6-base-dev --fix-missing
RUN sudo apt-get install libqt6websockets6-dev --fix-missing
RUN sudo apt-get install udev build-essential libpthread-stubs0-dev libgl1-mesa-dev libx11-dev libxrandr-dev libfreetype6-dev libglew-dev libjpeg-dev libxi-dev --fix-missing
RUN sudo apt-get install libxinerama-dev libxcursor-dev --fix-
RUN sudo apt-get install libudev-dev libglu1-mesa-dev libsecret-1-dev libnotify-dev --fix-missing
RUN sudo apt-get install checkinstall libtiff5-dev ubuntu-restricted-extras freeglut3 freeglut3-dev libcairo2-dev --fix-missing
RUN sudo apt-get install libgconf2-dev --fix-missing
RUN git submodule update --init --recursive 


COPY entrypoint_docker.sh /

#ensure entrypoint is executable (force flags update, because file flags may change if a commit is made under Windows)
RUN chmod +x /entrypoint_docker.sh

# Code file to execute when the docker container starts up ('entrypoint_docker.sh')
ENTRYPOINT ["/entrypoint_docker.sh"]


