package v1

import (
	"bytes"
	"encoding/json"
	"net/http"
	"net/http/httptest"
)

func RouteHttp(
	handler http.Handler,
	method string,
	path string,
	request interface{},
	headers map[string]string,
) (*http.Response, error) {
	var buf bytes.Buffer
	if err := json.NewEncoder(&buf).Encode(request); err != nil {
		return nil, err
	}

	req := httptest.NewRequest(method, path, &buf)
	for k, v := range headers {
		req.Header.Set(k, v)
	}

	w := httptest.NewRecorder()
	handler.ServeHTTP(w, req)
	return w.Result(), nil
}
