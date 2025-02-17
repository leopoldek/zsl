#!/bin/sh

clear

while getopts "ird" opt; do case "$opt" in
	i) INIT=true ;;
	r) RUN=true ;;
	d) DEBUG=true ;;
	*) exit 1 ;;
esac; done
shift $((OPTIND-1))

BUILD_FOLDER=${1:-debug}

if command -v ninja >/dev/null 2>&1; then
	GENERATOR="ninja"
	GENERATOR_NAME="Ninja"
else
	GENERATOR="make"
	GENERATOR_NAME="Unix Makefiles"
fi

if [[ ${INIT} = true ]]; then
	generate(){
		printf "\033[0;36mGenerating ${1} build...\033[0m\n"
		cmake "-G${GENERATOR_NAME}" "-Bbin/${1}" -H. "${@:2}" || exit 1
	}
	generate debug -DCMAKE_BUILD_TYPE=Debug -DZSL_BUILD_TESTS=ON
	generate release -DCMAKE_BUILD_TYPE=Release -DZSL_BUILD_TESTS=ON
fi

cd bin/$BUILD_FOLDER || { echo "Failed to cd to $BUILD_FOLDER build folder. Make sure you run \"build -i\" and you're in the project directory."; exit 1; }
${GENERATOR} || exit 1

if [[ ${DEBUG} = true ]]; then
	gdb tests/tests
elif [[ ${RUN} = true ]]; then
	./tests/tests
fi
