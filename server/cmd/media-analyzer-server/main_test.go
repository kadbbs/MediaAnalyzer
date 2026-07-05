package main

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

func TestBytesToHexAndASCII(t *testing.T) {
	data := []byte{0x00, 0x41, 0x7e, 0xff}
	if got := bytesToHex(data); got != "00 41 7e ff" {
		t.Fatalf("bytesToHex() = %q", got)
	}
	if got := bytesToASCII(data); got != ".A~." {
		t.Fatalf("bytesToASCII() = %q", got)
	}
}

func TestBytesHandlerReturnsRequestedRange(t *testing.T) {
	id := storeSession([]byte{0x00, 0x41, 0x42, 0xff, 0x43}, "sample.bin")
	req := httptest.NewRequest(http.MethodGet, "/api/bytes?id="+id+"&offset=1&length=3", nil)
	rec := httptest.NewRecorder()

	bytesHandler(rec, req)
	if rec.Code != http.StatusOK {
		t.Fatalf("bytesHandler status = %d body=%s", rec.Code, rec.Body.String())
	}

	var payload struct {
		Name      string `json:"name"`
		Offset    int64  `json:"offset"`
		Length    int    `json:"length"`
		Truncated bool   `json:"truncated"`
		Hex       string `json:"hex"`
		ASCII     string `json:"ascii"`
	}
	if err := json.Unmarshal(rec.Body.Bytes(), &payload); err != nil {
		t.Fatal(err)
	}
	if payload.Name != "sample.bin" || payload.Offset != 1 || payload.Length != 3 {
		t.Fatalf("unexpected payload metadata: %+v", payload)
	}
	if payload.Hex != "41 42 ff" || payload.ASCII != "AB." {
		t.Fatalf("unexpected byte payload: %+v", payload)
	}
	if payload.Truncated {
		t.Fatalf("range should not be marked truncated: %+v", payload)
	}
}

func TestBytesHandlerRejectsMissingSession(t *testing.T) {
	req := httptest.NewRequest(http.MethodGet, "/api/bytes?id=missing&offset=0&length=1", nil)
	rec := httptest.NewRecorder()

	bytesHandler(rec, req)
	if rec.Code != http.StatusNotFound {
		t.Fatalf("bytesHandler status = %d body=%s", rec.Code, strings.TrimSpace(rec.Body.String()))
	}
}
