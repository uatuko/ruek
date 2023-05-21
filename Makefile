SOURCEDIR = src
SOURCES  := $(shell find $(SOURCEDIR) -type f -name '*.h' -o -name '*.cpp')


.SILENT: lint

lint:
ifeq (, $(shell which clang-format))
	echo '\033[1;41m WARN \033[0m clang-format not found, not linting files';
else
	clang-format -style=file --dry-run $(SOURCES)
endif

lint\:ci:
	clang-format -style=file --dry-run -Werror $(SOURCES)

lint\:fix:
	clang-format -style=file -i $(SOURCES)

