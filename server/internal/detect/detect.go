package detect

import (
	"bytes"
	"encoding/binary"
	"strings"
)

type Result struct {
	Format     string   `json:"format"`
	Family     string   `json:"family"`
	Confidence float64  `json:"confidence"`
	Evidence   []string `json:"evidence"`
}

func Analyze(data []byte, nameHint string) Result {
	if len(data) >= 12 && bytes.Equal(data[4:8], []byte("ftyp")) {
		brand := string(data[8:12])
		return result("iso-bmff", "container", 0.99, "found ftyp box at offset 4", "major_brand="+brand)
	}
	if hasPrefix(data, []byte{0x1a, 0x45, 0xdf, 0xa3}) {
		return result("matroska/webm", "container", 0.96, "found EBML header magic 1A45DFA3")
	}
	if looksLikeMPEGTS(data) {
		return result("mpeg-ts", "container", 0.94, "found repeated 0x47 sync bytes at 188-byte intervals")
	}
	if hasPrefix(data, []byte("RIFF")) && hasAt(data, 8, []byte("WAVE")) {
		return result("wav", "container", 0.98, "found RIFF header", "found WAVE form type")
	}
	if hasPrefix(data, []byte("RF64")) && hasAt(data, 8, []byte("WAVE")) {
		return result("rf64", "container", 0.98, "found RF64 header", "found WAVE form type")
	}
	if hasPrefix(data, []byte("RIFF")) && hasAt(data, 8, []byte("AVI ")) {
		return result("avi", "container", 0.96, "found RIFF header", "found AVI form type")
	}
	if hasPrefix(data, []byte("OggS")) {
		return result("ogg", "container", 0.98, "found Ogg page capture pattern")
	}
	if hasPrefix(data, []byte("fLaC")) {
		return result("flac", "container", 0.98, "found native FLAC marker")
	}
	if hasPrefix(data, []byte("FLV")) {
		return result("flv", "container", 0.98, "found FLV signature")
	}
	if hasPrefix(data, []byte("ID3")) {
		return result("mp3", "elementary_stream", 0.86, "found ID3 tag at start")
	}
	if looksLikeMP3(data, 0) {
		return result("mp3", "elementary_stream", 0.82, "found MPEG audio frame sync at start")
	}
	if looksLikeADTS(data, 0) {
		return result("adts-aac", "elementary_stream", 0.85, "found ADTS syncword at start")
	}
	if hasPrefix(data, []byte{0, 0, 0, 1}) || hasPrefix(data, []byte{0, 0, 1}) {
		return result("annex-b-bitstream", "elementary_stream", 0.72, "found Annex B start code at start")
	}
	if looksText(data) {
		text := string(data)
		if strings.Contains(text, "#EXTM3U") {
			return result("hls-m3u8", "manifest", 0.98, "found #EXTM3U manifest marker")
		}
		if strings.Contains(text, "<MPD") || strings.Contains(text, "urn:mpeg:dash:schema:mpd") {
			return result("dash-mpd", "manifest", 0.96, "found DASH MPD marker")
		}
	}

	lowerName := strings.ToLower(nameHint)
	if strings.HasSuffix(lowerName, ".m3u8") {
		return result("hls-m3u8", "manifest", 0.55, "file extension hint .m3u8")
	}
	if strings.HasSuffix(lowerName, ".mpd") {
		return result("dash-mpd", "manifest", 0.55, "file extension hint .mpd")
	}
	if len(data) >= 8 {
		boxSize := binary.BigEndian.Uint32(data[0:4])
		if boxSize >= 8 && int(boxSize) <= len(data) {
			return result("unknown-box-container", "container", 0.35, "first 4 bytes look like a bounded big-endian box size")
		}
	}

	return result("unknown", "unknown", 0.0, "no known signature matched")
}

func result(format, family string, confidence float64, evidence ...string) Result {
	return Result{
		Format:     format,
		Family:     family,
		Confidence: confidence,
		Evidence:   evidence,
	}
}

func hasPrefix(data, prefix []byte) bool {
	return len(data) >= len(prefix) && bytes.Equal(data[:len(prefix)], prefix)
}

func hasAt(data []byte, offset int, value []byte) bool {
	return len(data) >= offset+len(value) && bytes.Equal(data[offset:offset+len(value)], value)
}

func looksLikeMPEGTS(data []byte) bool {
	const packet = 188
	if len(data) < packet*2+1 {
		return false
	}
	for start := 0; start < packet && start < len(data); start++ {
		if start+packet*2 < len(data) && data[start] == 0x47 && data[start+packet] == 0x47 && data[start+packet*2] == 0x47 {
			return true
		}
	}
	return false
}

func looksLikeMP3(data []byte, offset int) bool {
	if len(data) < offset+2 {
		return false
	}
	return data[offset] == 0xff && data[offset+1]&0xe0 == 0xe0 && data[offset+1]&0x18 != 0x08 && data[offset+1]&0x06 != 0x00
}

func looksLikeADTS(data []byte, offset int) bool {
	if len(data) < offset+2 {
		return false
	}
	return data[offset] == 0xff && data[offset+1]&0xf6 == 0xf0
}

func looksText(data []byte) bool {
	limit := len(data)
	if limit > 512 {
		limit = 512
	}
	if limit == 0 {
		return false
	}
	printable := 0
	for _, b := range data[:limit] {
		if b == '\n' || b == '\r' || b == '\t' || (b >= 0x20 && b <= 0x7e) {
			printable++
		}
	}
	return printable > limit*9/10
}
