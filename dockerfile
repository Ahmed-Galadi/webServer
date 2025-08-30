# Use Ubuntu 22.04 as the base image for Linux compatibility
FROM ubuntu:22.04

# Set working directory inside the container
WORKDIR /app

# Install essential build tools and libraries
RUN apt-get update && \
    apt-get install -y \
    g++ \
    make \
    valgrind \
    vim \
    && apt-get clean

# Expose the port your server listens on (e.g., 8080)
EXPOSE 8080 4040 1010

# Command to keep the container running for interactive use
CMD ["bash"]