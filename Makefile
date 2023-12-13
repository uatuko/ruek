builddir  = .build
bindir    = $(builddir)/bin
binary    = $(bindir)/sentium
buildfile = $(builddir)/build.ninja

protodir = proto
srcdir   = src

sources := $(shell find $(protodir) -type f -name '*.proto')
sources += $(shell find $(srcdir) -type f -name '*.h' -o -name '*.cpp')

.PHONY: $(binary) all clean lint lint\:ci lint\:fix
.SILENT: lint lint\:fix

all: $(binary)

$(binary): $(buildfile)
	cmake --build $(builddir)

$(buildfile):
	cmake -B $(builddir) -G Ninja

clean:
	cmake --build $(builddir) --target clean

lint:
ifeq (, $(shell which clang-format))
	echo '\033[1;41m WARN \033[0m clang-format not found, not linting files';
else
	clang-format --style=file --dry-run $(sources)
endif

lint\:ci:
	clang-format --style=file --dry-run --Werror $(sources)

lint\:fix:
	clang-format --style=file -i $(sources)

run: $(binary)
	$(binary)

test: $(binary)
	ctest --test-dir $(builddir) --output-on-failure
