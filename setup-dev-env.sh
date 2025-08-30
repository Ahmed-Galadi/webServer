#!/bin/bash
# setup-dev-env.sh

set -e

echo "🐳 Setting up WebServ development environment..."

# Build and start the container
echo "Building Docker container..."
docker-compose build

echo "Starting container..."
docker-compose up -d

# Wait for container to be ready
echo "Waiting for container to start..."
sleep 5

# Generate SSH key pair if it doesn't exist
if [ ! -f ~/.ssh/webserv_dev ]; then
    echo "🔑 Generating SSH key pair..."
    ssh-keygen -t rsa -b 4096 -f ~/.ssh/webserv_dev -N ""
fi

# Copy SSH public key to container
echo "📋 Setting up SSH access..."
docker exec webserv-dev mkdir -p /home/dev/.ssh
docker cp ~/.ssh/webserv_dev.pub webserv-dev:/tmp/key.pub
docker exec webserv-dev bash -c "cat /tmp/key.pub >> /home/dev/.ssh/authorized_keys"
docker exec webserv-dev bash -c "chown -R dev:dev /home/dev/.ssh"
docker exec webserv-dev bash -c "chmod 600 /home/dev/.ssh/authorized_keys"
docker exec webserv-dev rm /tmp/key.pub

# Add SSH config to your ~/.ssh/config
echo "📝 Adding SSH configuration..."
if ! grep -q "Host webserv-dev" ~/.ssh/config 2>/dev/null; then
    cat >> ~/.ssh/config << EOF

# WebServ Development Container
Host webserv-dev
    HostName localhost
    Port 2222
    User dev
    IdentityFile ~/.ssh/webserv_dev
    StrictHostKeyChecking no
    UserKnownHostsFile /dev/null
EOF
fi

echo "✅ Setup complete!"
echo ""
echo "🔧 Usage:"
echo "  SSH into container:    ssh webserv-dev"
echo "  VSCode Remote SSH:     Connect to 'webserv-dev' host"
echo "  Test server:           curl http://localhost:8080"
echo "  Stop container:        docker-compose down"
echo ""
echo "📁 Your project is mounted at: /workspace/webserv"