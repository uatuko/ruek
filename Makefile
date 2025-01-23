builddir  = .build
bindir    = $(builddir)/bin
binary    = $(bindir)/ruek
benchbin  = $(binary)_bench
buildfile = $(builddir)/build.ninja

benchdir    = bench
protodir    = proto
protogendir = $(protodir)/.gen
srcdir      = src

githubrepo = github.com/uatuko/ruek

protos  := $(shell find $(protodir)/ruek/api -type f -name '*.proto')
sources := $(shell find $(protodir) -type f -name '*.proto')
sources += $(shell find $(srcdir) -type f -name '*.h' -o -name '*.cpp')
sources += $(shell find $(benchdir) -type f -name '*.h' -o -name '*.cpp')

lcov_output       = $(builddir)/coverage.out
profdata_output   = $(builddir)/coverage.profdata
testing_binaries := $(shell find $(bindir) -type f -perm -a=x -name "*_tests")

llvm-cov      = $(shell which llvm-cov)
llvm-profdata = $(shell which llvm-profdata)

ifeq (, $(llvm-cov))
	llvm-cov = $(shell xcrun --find llvm-cov)
endif

ifeq (, $(llvm-profdata))
	llvm-profdata = $(shell xcrun --find llvm-profdata)
endif


.PHONY: $(binary) clean lint lint\:ci lint\:fix
.SILENT: coverage lint lint\:fix

%:
	@:

all: $(binary)

$(binary): $(buildfile)
	cmake --build $(builddir)

$(buildfile):
	cmake -B $(builddir) -G Ninja -DBUILD_TESTING=OFF

bench: $(binary)
	$(benchbin) $(filter-out $@,$(MAKECMDGOALS)) $(MAKEFLAGS)

clean:
	cmake --build $(builddir) --target clean

coverage: $(binary)
	find $(builddir)/$(sourcedir) -type f -name "*.profraw" -exec rm {} +
	LLVM_PROFILE_FILE="%p.profraw" ctest --test-dir $(builddir) --output-on-failure

	echo '' # Empty line to separate outputs
	-rm $(profdata_output)
	find $(builddir)/$(sourcedir) -type f -name "*.profraw" -exec $(llvm-profdata) merge --sparse=true --output=$(profdata_output) {} +
	$(llvm-cov) report -ignore-filename-regex='$(builddir)\/' -instr-profile=$(profdata_output) $(foreach bin, $(testing_binaries), -object=$(bin))

coverage\:lcov: coverage
	$(llvm-cov) export -format=lcov -ignore-filename-regex='$(builddir)\/' -ignore-filename-regex='.*_test\.cpp' -instr-profile=$(profdata_output) $(foreach bin, $(testing_binaries), -object=$(bin)) > $(lcov_output)

	@echo '' # Empty line to separate outputs
	lcov --list $(lcov_output)

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

protoc-gen-go:
ifeq (, $(shell which protoc-gen-go))
	go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
endif

protoc-gen-go-grpc:
ifeq (, $(shell which protoc-gen-go-grpc))
	go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
endif

protoc: $(protos) protoc-gen-go protoc-gen-go-grpc
	protoc \
		--go_out=$(protogendir)/go --go_opt=module=$(githubrepo)/$(protogendir)/go \
		--go-grpc_out=$(protogendir)/go --go-grpc_opt=module=$(githubrepo)/$(protogendir)/go \
		$(protos)

run: $(binary)
	$(binary) -4 127.0.0.1 $(filter-out $@,$(MAKECMDGOALS))

test: $(binary)
	ctest --test-dir $(builddir) --output-on-failure $(filter-out $@,$(MAKECMDGOALS))
