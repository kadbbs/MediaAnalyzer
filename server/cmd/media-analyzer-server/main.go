package main

import (
	"encoding/json"
	"errors"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"
	"path/filepath"
	"time"

	"media-analyzer/server/internal/detect"
)

const maxProbeBytes int64 = 1024 * 1024

type analyzeResponse struct {
	Input    inputInfo     `json:"input"`
	Detector detect.Result `json:"detection"`
}

type inputInfo struct {
	Type string `json:"type"`
	Name string `json:"name,omitempty"`
	URL  string `json:"url,omitempty"`
	Size int64  `json:"size,omitempty"`
}

func main() {
	mux := http.NewServeMux()
	mux.HandleFunc("/api/analyze", analyzeHandler)
	mux.Handle("/", http.FileServer(http.Dir(staticDir())))

	addr := getenv("MEDIA_ANALYZER_ADDR", ":8080")
	server := &http.Server{
		Addr:              addr,
		Handler:           logRequests(mux),
		ReadHeaderTimeout: 10 * time.Second,
	}

	log.Printf("MediaAnalyzer server listening on http://localhost%s", addr)
	if err := server.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
		log.Fatal(err)
	}
}

func analyzeHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}

	contentType := r.Header.Get("Content-Type")
	var resp analyzeResponse
	var err error

	if len(contentType) >= len("multipart/form-data") && contentType[:len("multipart/form-data")] == "multipart/form-data" {
		resp, err = analyzeUpload(r)
	} else {
		resp, err = analyzeJSON(r)
	}

	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	encoder := json.NewEncoder(w)
	encoder.SetIndent("", "  ")
	_ = encoder.Encode(resp)
}

func analyzeUpload(r *http.Request) (analyzeResponse, error) {
	if err := r.ParseMultipartForm(maxProbeBytes); err != nil {
		return analyzeResponse{}, err
	}
	file, header, err := r.FormFile("file")
	if err != nil {
		return analyzeResponse{}, err
	}
	defer file.Close()

	data, err := io.ReadAll(io.LimitReader(file, maxProbeBytes))
	if err != nil {
		return analyzeResponse{}, err
	}

	return analyzeResponse{
		Input: inputInfo{
			Type: "file",
			Name: header.Filename,
			Size: header.Size,
		},
		Detector: detect.Analyze(data, header.Filename),
	}, nil
}

func analyzeJSON(r *http.Request) (analyzeResponse, error) {
	var req struct {
		URL string `json:"url"`
	}
	if err := json.NewDecoder(io.LimitReader(r.Body, 1<<20)).Decode(&req); err != nil {
		return analyzeResponse{}, err
	}
	parsed, err := url.Parse(req.URL)
	if err != nil || (parsed.Scheme != "http" && parsed.Scheme != "https") {
		return analyzeResponse{}, errors.New("url must be http or https")
	}

	client := &http.Client{Timeout: 20 * time.Second}
	httpReq, err := http.NewRequest(http.MethodGet, req.URL, nil)
	if err != nil {
		return analyzeResponse{}, err
	}
	httpReq.Header.Set("Range", "bytes=0-1048575")
	httpResp, err := client.Do(httpReq)
	if err != nil {
		return analyzeResponse{}, err
	}
	defer httpResp.Body.Close()
	if httpResp.StatusCode >= 400 {
		return analyzeResponse{}, errors.New("remote server returned " + httpResp.Status)
	}

	data, err := io.ReadAll(io.LimitReader(httpResp.Body, maxProbeBytes))
	if err != nil {
		return analyzeResponse{}, err
	}

	return analyzeResponse{
		Input: inputInfo{
			Type: "url",
			URL:  req.URL,
			Name: filepath.Base(parsed.Path),
			Size: httpResp.ContentLength,
		},
		Detector: detect.Analyze(data, parsed.Path),
	}, nil
}

func staticDir() string {
	if value := os.Getenv("MEDIA_ANALYZER_STATIC_DIR"); value != "" {
		return value
	}
	return filepath.Join("..", "web", "static")
}

func getenv(key, fallback string) string {
	if value := os.Getenv(key); value != "" {
		return value
	}
	return fallback
}

func logRequests(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		next.ServeHTTP(w, r)
		log.Printf("%s %s %s", r.Method, r.URL.Path, time.Since(start))
	})
}
