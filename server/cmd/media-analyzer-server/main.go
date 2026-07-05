package main

import (
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"errors"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"time"
)

const maxProbeBytes int64 = 16 * 1024 * 1024
const maxByteRangeLength int64 = 1024 * 1024
const sessionTTL = 30 * time.Minute

type cachedMedia struct {
	data    []byte
	name    string
	created time.Time
}

var (
	sessionMu    sync.Mutex
	sessionCache = map[string]cachedMedia{}
)

func main() {
	mux := http.NewServeMux()
	mux.HandleFunc("/api/analyze", analyzeHandler)
	mux.HandleFunc("/api/bytes", bytesHandler)
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

	resp, err := runCoreAnalyzer(data, header.Filename)
	if err != nil {
		return nil, err
	}
	return attachSession(resp, storeSession(data, header.Filename))
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

	resp, err := runCoreAnalyzer(data, filepath.Base(parsed.Path))
	if err != nil {
		return nil, err
	}
	return attachSession(resp, storeSession(data, filepath.Base(parsed.Path)))
}

func bytesHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}

	id := r.URL.Query().Get("id")
	offset, err := parseNonNegativeInt(r.URL.Query().Get("offset"), "offset")
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	length, err := parseNonNegativeInt(r.URL.Query().Get("length"), "length")
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	if length <= 0 {
		http.Error(w, "length must be greater than zero", http.StatusBadRequest)
		return
	}
	if length > maxByteRangeLength {
		length = maxByteRangeLength
	}

	sessionMu.Lock()
	item, ok := sessionCache[id]
	sessionMu.Unlock()
	if !ok {
		http.Error(w, "analysis session not found or expired", http.StatusNotFound)
		return
	}

	if offset > int64(len(item.data)) {
		offset = int64(len(item.data))
	}
	end := offset + length
	truncated := false
	if end > int64(len(item.data)) {
		end = int64(len(item.data))
		truncated = true
	}
	chunk := item.data[offset:end]

	w.Header().Set("Content-Type", "application/json")
	_ = json.NewEncoder(w).Encode(map[string]any{
		"name":      item.name,
		"offset":    offset,
		"length":    len(chunk),
		"truncated": truncated || length == maxByteRangeLength,
		"hex":       bytesToHex(chunk),
		"ascii":     bytesToASCII(chunk),
	})
}

func parseNonNegativeInt(value string, name string) (int64, error) {
	if value == "" {
		return 0, errors.New(name + " is required")
	}
	parsed, err := strconv.ParseInt(value, 10, 64)
	if err != nil || parsed < 0 {
		return 0, errors.New(name + " must be a non-negative integer")
	}
	return parsed, nil
}

func storeSession(data []byte, name string) string {
	idBytes := make([]byte, 16)
	if _, err := rand.Read(idBytes); err != nil {
		idBytes = []byte(strconv.FormatInt(time.Now().UnixNano(), 16))
	}
	id := hex.EncodeToString(idBytes)
	copyData := append([]byte(nil), data...)

	sessionMu.Lock()
	defer sessionMu.Unlock()
	now := time.Now()
	for key, item := range sessionCache {
		if now.Sub(item.created) > sessionTTL {
			delete(sessionCache, key)
		}
	}
	sessionCache[id] = cachedMedia{data: copyData, name: name, created: now}
	return id
}

func attachSession(resp []byte, id string) ([]byte, error) {
	var payload map[string]any
	if err := json.Unmarshal(resp, &payload); err != nil {
		return nil, err
	}
	payload["session"] = map[string]any{
		"id":       id,
		"bytesUrl": "/api/bytes?id=" + id,
	}
	return json.MarshalIndent(payload, "", "  ")
}

func bytesToHex(data []byte) string {
	if len(data) == 0 {
		return ""
	}
	const digits = "0123456789abcdef"
	var builder strings.Builder
	builder.Grow(len(data)*3 - 1)
	for index, value := range data {
		if index != 0 {
			builder.WriteByte(' ')
		}
		builder.WriteByte(digits[value>>4])
		builder.WriteByte(digits[value&0x0f])
	}
	return builder.String()
}

func bytesToASCII(data []byte) string {
	out := make([]byte, len(data))
	for index, value := range data {
		if value >= 0x20 && value <= 0x7e {
			out[index] = value
		} else {
			out[index] = '.'
		}
	}
	return string(out)
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
