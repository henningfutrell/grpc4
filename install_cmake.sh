if ! [ -x "$(command -v cmake)" ]; then
	echo 'Not installed' >&2
	echo "csce438" | sudo -S apt-get install cmake
fi
