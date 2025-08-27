# Use official Ubuntu as base
FROM ubuntu:22.04 AS build

# Install tools needed to build your project
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set Working Directory inside container
WORKDIR /app

# Copy source code into container
COPY . .

# Build project (Release mode, disable tests)
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF && \
    cmake --build build --target DistributedCachePP

# ------ Runtime Stage --------
FROM ubuntu:22.04
WORKDIR /app

# Copy binary from build stage
COPY --from=build /app/build/DistributedCachePP /app/DistributedCachePP

# Expose API port
EXPOSE 5000

# Run the server
CMD ["./DistributedCachePP", "--role", "leader", "--port", "5000"]