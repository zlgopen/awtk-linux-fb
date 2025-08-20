WAYLAND_SCANNER=wayland-scanner

rm -f *.h *.c

${WAYLAND_SCANNER} client-header xdg-shell.xml xdg-shell-protocol.h
${WAYLAND_SCANNER} private-code xdg-shell.xml xdg-shell-protocol.c
${WAYLAND_SCANNER} client-header fullscreen-shell-unstable-v1.xml fullscreen-shell-protocol.h
${WAYLAND_SCANNER} private-code fullscreen-shell-unstable-v1.xml fullscreen-shell-protocol.c
