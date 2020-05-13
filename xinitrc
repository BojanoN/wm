set -e
xterm &
compton -f -D 5 --active-opacity 1.0 &

exec ./wm


