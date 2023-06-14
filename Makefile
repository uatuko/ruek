BUILDDIR = .build
BINDIR   = $(BUILDDIR)/bin
BINARY   = $(BINDIR)/gatekeeper

PROTODIR  = proto
SOURCEDIR = src

SOURCES  := $(shell find $(PROTODIR) -type f -name '*.proto')
SOURCES  += $(shell find $(SOURCEDIR) -type f -name '*.h' -o -name '*.cpp')

LCOV_OUTPUT       = $(BUILDDIR)/coverage.out
PROFDATA_OUTPUT   = $(BUILDDIR)/coverage.profdata
TESTING_BINARIES := $(shell find $(BINDIR) -type f -executable -name "*_tests")

llvm-cov      = $(shell which llvm-cov)
llvm-profdata = $(shell which llvm-profdata)

ifeq (, $(llvm-cov))
	llvm-cov = $(shell xcrun --find llvm-cov)
endif

ifeq (, $(llvm-profdata))
	llvm-profdata = $(shell xcrun --find llvm-profdata)
endif


.PHONY: clean
.SILENT: coverage lint

all: $(BINARY)

$(BINARY): $(BUILDDIR)
	cmake --build $(BUILDDIR)

$(BUILDDIR):
	cmake -B $(BUILDDIR) -G Ninja -DGATEKEEPER_ENABLE_COVERAGE=ON -DGATEKEEPER_ENABLE_TESTING=ON

clean:
	cmake --build $(BUILDDIR) --target clean

coverage: $(BINARY)
	find $(BUILDDIR)/$(SOURCEDIR) -type f -name "*.profraw" -exec rm {} +
	LLVM_PROFILE_FILE="%p.profraw" ctest --test-dir $(BUILDDIR)

	echo '' # Empty line to separate outputs
	-rm $(PROFDATA_OUTPUT)
	find $(BUILDDIR)/$(SOURCEDIR) -type f -name "*.profraw" -exec $(llvm-profdata) merge --sparse=true --output=$(PROFDATA_OUTPUT) {} +
	$(llvm-cov) report -ignore-filename-regex='$(BUILDDIR)\/' -instr-profile=$(PROFDATA_OUTPUT) $(foreach bin, $(TESTING_BINARIES), -object=$(bin))

coverage\:lcov: coverage
	$(llvm-cov) export -format=lcov -ignore-filename-regex='$(BUILDDIR)\/' -ignore-filename-regex='.*_test\.cpp' -instr-profile=$(PROFDATA_OUTPUT) $(foreach bin, $(TESTING_BINARIES), -object=$(bin)) > $(LCOV_OUTPUT)

	@echo '' # Empty line to separate outputs
	lcov --list $(LCOV_OUTPUT)

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

run: $(BINARY)
	$(BINARY)

test: $(BINARY)
	ctest --test-dir $(BUILDDIR) --output-on-failure

