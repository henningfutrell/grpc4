if ! [ -x "$(command -v cmake)" ]; then
	echo 'Cmake not installed' >&2
	echo "csce438" | sudo -S apt-get install -y cmake
fi
