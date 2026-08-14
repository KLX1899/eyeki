#!/bin/bash
set -e

echo "==> Installing EyeKi..."

# 1. Compile
echo "==> Compiling..."
gcc -O2 -Wall -o EyeKi EyeKi.c \
  $(pkg-config --libs --cflags libnotify libsystemd gtk+-3.0)

# 2. Copy binary
echo "==> Installing binary..."
mkdir -p ~/.local/bin
cp EyeKi ~/.local/bin/EyeKi
chmod +x ~/.local/bin/EyeKi

# 3. Create systemd service
echo "==> Creating systemd service..."
mkdir -p ~/.config/systemd/user
cat > ~/.config/systemd/user/EyeKi.service << 'EOF'
[Unit]
Description=EyeKi – Eye Drop Reminder
After=graphical-session.target

[Service]
Type=simple
ExecStart=%h/.local/bin/EyeKi --daemon
Restart=on-failure
RestartSec=5
Environment=DISPLAY=:0
Environment=DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/%U/bus

[Install]
WantedBy=default.target
EOF

# 4. Enable service
echo "==> Enabling service..."
systemctl --user daemon-reload
systemctl --user enable --now EyeKi.service

echo ""
echo "✓ EyeKi installed successfully!"
echo "✓ It will run automatically in the background after every login."
