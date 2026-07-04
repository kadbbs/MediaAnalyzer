package main

import (
	"encoding/json"
	"errors"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"
)

const maxProbeBytes int64 = 16 * 1024 * 1024

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
	var resp []byte
	var err error

	if strings.HasPrefix(contentType, "multipart/form-data") {
		resp, err = analyzeUpload(r)
	} else {
		resp, err = analyzeJSON(r)
	}

	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	_, _ = w.Write(resp)
}

func analyzeUpload(r *http.Request) ([]byte, error) {
	if err := r.ParseMultipartForm(maxProbeBytes); err != nil {
		return nil, err
	}
	file, header, err := r.FormFile("file")
	if err != nil {
		return nil, err
	}
	defer file.Close()

	data, err := io.ReadAll(io.LimitReader(file, maxProbeBytes))
	if err != nil {
		return nil, err
	}

	return runCoreAnalyzer(data, header.Filename)
}

func analyzeJSON(r *http.Request) ([]byte, error) {
	var req struct {
		URL string `json:"url"`
	}
	if err := json.NewDecoder(io.LimitReader(r.Body, 1<<20)).Decode(&req); err != nil {
		return nil, err
	}
	parsed, err := url.Parse(req.URL)
	if err != nil || (parsed.Scheme != "http" && parsed.Scheme != "https") {
		return nil, errors.New("url must be http or https")
	}

	client := &http.Client{Timeout: 20 * time.Second}
	httpReq, err := http.NewRequest(http.MethodGet, req.URL, nil)
	if err != nil {
		return nil, err
	}
	httpReq.Header.Set("Range", "bytes=0-1048575")
	httpResp, err := client.Do(httpReq)
	if err != nil {
		return nil, err
	}
	defer httpResp.Body.Close()
	if httpResp.StatusCode >= 400 {
		return nil, errors.New("remote server returned " + httpResp.Status)
	}

	data, err := io.ReadAll(io.LimitReader(httpResp.Body, maxProbeBytes))
	if err != nil {
		return nil, err
	}

	return runCoreAnalyzer(data, filepath.Base(parsed.Path))
}

func runCoreAnalyzer(data []byte, nameHint string) ([]byte, error) {
	tmp, err := os.CreateTemp("", "media-analyzer-*"+filepath.Ext(nameHint))
	if err != nil {
		return nil, err
	}
	tmpPath := tmp.Name()
	defer os.Remove(tmpPath)

	if _, err := tmp.Write(data); err != nil {
		_ = tmp.Close()
		return nil, err
	}
	if err := tmp.Close(); err != nil {
		return nil, err
	}

	cmd := exec.Command(coreAnalyzerPath(), tmpPath)
	output, err := cmd.Output()
	if err != nil {
		if exitErr, ok := err.(*exec.ExitError); ok {
			message := strings.TrimSpace(string(exitErr.Stderr))
			if message != "" {
				return nil, errors.New(message)
			}
		}
		return nil, err
	}
	if !json.Valid(output) {
		return nil, errors.New("core analyzer returned invalid JSON")
	}
	return output, nil
}

func coreAnalyzerPath() string {
	if value := os.Getenv("MEDIA_ANALYZER_CORE"); value != "" {
		return value
	}
	candidates := []string{
		filepath.Join("..", "build", "media-analyzer-core"),
		filepath.Join("build", "media-analyzer-core"),
	}
	for _, candidate := range candidates {
		if stat, err := os.Stat(candidate); err == nil && !stat.IsDir() {
			return candidate
		}
	}
	return filepath.Join("..", "build", "media-analyzer-core")
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
