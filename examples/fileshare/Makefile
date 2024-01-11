sourcedir = .

lint:
	gofmt -s -d -e $(sourcedir)

lint\:fix:
	gofmt -s -l -w $(sourcedir)

run:
	go run main.go

test:
	go test -race ./...
