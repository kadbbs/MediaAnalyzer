# MediaAnalyzer Web 项目总规划

## 1. 项目定位

MediaAnalyzer 是一个面向开发者、音视频工程师、测试人员和内容平台排障场景的 Web 媒体分析工具。

用户可以通过上传本地文件或输入 URL，自动识别媒体容器格式，解析容器结构、轨道信息、时间线、采样表、metadata，并进一步解析常见 codec bitstream headers，例如 H.264 SPS/PPS、HEVC VPS/SPS/PPS、AAC AudioSpecificConfig、ADTS、OpusHead、AV1 sequence header 等。

项目目标不是做播放器，而是做一个可视化、可验证、可导出的媒体结构分析器。

## 2. 核心目标

1. 支持本地文件上传和远程 URL 分析。
2. 自动识别主流媒体容器格式。
3. 展示容器层级结构、box/chunk/segment/packet/block 等内部结构。
4. 解析轨道、编码、码率、时长、时间基、关键帧、采样表、索引、章节和 metadata。
5. 解析 codec bitstream headers，输出人类可读字段。
6. 支持大文件和远程文件的按需读取，避免一次性完整加载。
7. 提供结构化 JSON 导出，方便测试、自动化和问题复现。
8. 为后续扩展更多容器、codec 和诊断规则预留插件式架构。

## 3. 非目标

初期不优先做以下能力：

1. 不做完整转码、封装转换或媒体修复。
2. 不做完整播放器功能，只提供必要的预览和帧/包级定位。
3. 不在第一阶段实现所有 codec 的完整 bitstream 解码。
4. 不依赖浏览器一次性读取超大文件。
5. 不把 FFmpeg 输出文本当作唯一数据源；FFmpeg 可作为辅助校验或 fallback。

## 4. 用户入口

### 4.1 本地上传

支持拖拽或文件选择上传：

- 小文件：浏览器直接读取并解析。
- 大文件：使用分片读取，按需解析 header、索引、采样表。
- 可选后端模式：文件上传到后端，由后端统一读取和分析。

### 4.2 URL 输入

支持 HTTP/HTTPS URL：

- 优先使用 Range Request 按需读取。
- 自动识别 Content-Type、Content-Length、Accept-Ranges。
- 支持远程 MP4/MOV/WebM/TS/HLS/DASH 等。
- 对不支持 Range 的资源降级为流式读取或提示能力受限。

### 4.3 后续可选入口

- 粘贴十六进制片段。
- 上传 codec header bytes。
- 输入 manifest URL，例如 `.m3u8`、`.mpd`。
- 导入前一次分析导出的 JSON。

## 5. 总体架构

```text
User Input
  |-- Local File
  |-- Remote URL
  |-- Manifest URL
        |
        v
Byte Source Abstraction
  |-- FileByteSource
  |-- HttpRangeByteSource
  |-- StreamByteSource
        |
        v
Format Detector
        |
        v
Container Parser
  |-- ISO BMFF / MP4 / MOV / CMAF / HEIF
  |-- Matroska / WebM
  |-- MPEG-TS / M2TS
  |-- MPEG-PS / VOB
  |-- FLV
  |-- AVI
  |-- ASF / WMV / WMA
  |-- Ogg
  |-- WAV / RF64 / BWF
  |-- AIFF
  |-- MP3 elementary stream
  |-- ADTS / LATM
  |-- HLS / DASH manifests
        |
        v
Track / Timeline / Sample Model
        |
        v
Codec Header Parsers
  |-- H.264 AVC SPS/PPS/SEI
  |-- H.265 HEVC VPS/SPS/PPS/SEI
  |-- AAC ASC / ADTS
  |-- OpusHead / OpusTags
  |-- Vorbis headers
  |-- AV1 sequence header / OBU
  |-- VP8 / VP9 headers
  |-- MPEG Audio frame headers
  |-- AC-3 / E-AC-3 syncframe
  |-- FLAC STREAMINFO
        |
        v
Analysis Result
  |-- UI Views
  |-- JSON Export
  |-- Diagnostics
```

## 6. 核心模块

### 6.1 Byte Source

统一抽象不同输入来源：

```ts
interface ByteSource {
  size?: number;
  read(offset: number, length: number): Promise<Uint8Array>;
  readUntil?(offset: number, delimiter: Uint8Array, maxLength: number): Promise<Uint8Array>;
  supportsRandomAccess: boolean;
}
```

需要支持：

- 本地文件随机读取。
- HTTP Range 随机读取。
- 顺序流式读取。
- 缓存已读取 byte ranges。
- 读取超时、重试和最大读取限制。

### 6.2 Format Detector

格式识别依据：

- magic bytes。
- 文件扩展名。
- MIME type。
- box/chunk signature。
- manifest 文本特征。
- elementary stream sync pattern。

识别结果需要包含 confidence：

```ts
interface FormatDetection {
  format: string;
  family: "container" | "manifest" | "elementary_stream" | "unknown";
  confidence: number;
  evidence: string[];
}
```

### 6.3 Container Parser

每个容器解析器应输出统一结果：

```ts
interface ContainerParseResult {
  format: string;
  brands?: string[];
  duration?: RationalTime;
  bitrate?: number;
  tracks: TrackInfo[];
  chapters?: ChapterInfo[];
  metadata: MetadataEntry[];
  structure: StructureNode[];
  warnings: Diagnostic[];
  errors: Diagnostic[];
}
```

### 6.4 Track Model

```ts
interface TrackInfo {
  id: string;
  type: "video" | "audio" | "subtitle" | "metadata" | "data" | "unknown";
  codec: CodecInfo;
  duration?: RationalTime;
  timeScale?: number;
  language?: string;
  width?: number;
  height?: number;
  channelCount?: number;
  sampleRate?: number;
  sampleCount?: number;
  samples?: SampleSummary;
  extradata?: CodecExtradata;
}
```

### 6.5 Codec Header Parser

每个 codec parser 输出：

```ts
interface CodecHeaderParseResult {
  codec: string;
  profile?: string;
  level?: string;
  bitDepth?: number;
  chromaSubsampling?: string;
  width?: number;
  height?: number;
  sampleRate?: number;
  channelConfig?: string;
  rawFields: Record<string, unknown>;
  warnings: Diagnostic[];
}
```

## 7. 格式覆盖规划

### 7.1 第一优先级容器

这些格式应作为 MVP 和早期核心能力：

| 格式 | 常见扩展名 | 重点解析内容 |
| --- | --- | --- |
| MP4 / MOV / M4V / M4A | `.mp4`, `.mov`, `.m4v`, `.m4a` | ftyp, moov, mvhd, trak, mdia, minf, stbl, sample tables, edit list, metadata |
| CMAF / Fragmented MP4 | `.mp4`, `.m4s`, `.cmf*` | moof, traf, tfhd, tfdt, trun, sidx, fragments |
| Matroska / WebM | `.mkv`, `.webm` | EBML, Segment, Info, Tracks, Cues, Clusters, Tags |
| MPEG-TS | `.ts`, `.m2ts` | PAT, PMT, PES, PCR, PTS/DTS, continuity counter |
| HLS Manifest | `.m3u8` | variants, renditions, segments, discontinuity, encryption tags |
| DASH Manifest | `.mpd` | periods, adaptation sets, representations, segment timeline |
| FLV | `.flv` | header, tags, AVC/AAC sequence headers |
| WAV / RF64 / BWF | `.wav`, `.rf64` | RIFF chunks, fmt, data, bext, iXML |
| MP3 | `.mp3` | ID3v2, frame headers, Xing/LAME, VBR info |
| Ogg | `.ogg`, `.oga`, `.ogv`, `.opus` | pages, granule position, Opus/Vorbis headers |

### 7.2 第二优先级容器

| 格式 | 常见扩展名 | 重点解析内容 |
| --- | --- | --- |
| AVI | `.avi` | RIFF chunks, hdrl, strl, idx1, OpenDML |
| ASF / WMV / WMA | `.asf`, `.wmv`, `.wma` | objects, streams, indexes, metadata |
| MPEG-PS / VOB | `.mpg`, `.mpeg`, `.vob` | pack header, system header, PES |
| AIFF / AIFC | `.aiff`, `.aif`, `.aifc` | FORM chunks, COMM, SSND |
| FLAC native | `.flac` | metadata blocks, STREAMINFO, SEEKTABLE, VORBIS_COMMENT |
| ADTS AAC | `.aac` | ADTS frames, profile, sample rate, channel config |

### 7.3 第三优先级和扩展格式

- MXF。
- GXF。
- 3GPP / 3GPP2。
- AMR。
- IVF for VP8/VP9/AV1。
- Annex B elementary streams：`.h264`, `.264`, `.h265`, `.hevc`。
- Raw bitstream snippets。

## 8. Codec Header 覆盖规划

### 8.1 视频

| Codec | Header 来源 | 重点字段 |
| --- | --- | --- |
| H.264 / AVC | avcC, Annex B SPS/PPS | profile_idc, level_idc, constraint flags, width, height, crop, VUI, timing info |
| H.265 / HEVC | hvcC, Annex B VPS/SPS/PPS | profile/tier/level, chroma format, bit depth, width, height, VPS/SPS/PPS arrays |
| AV1 | av1C, OBU sequence header | profile, level, tier, width, height, bit depth, color config |
| VP8 | WebM/IVF frame header | keyframe, width, height |
| VP9 | codec private / frame header | profile, bit depth, chroma, width, height |
| MPEG-2 Video | sequence header | size, aspect ratio, frame rate, bitrate |
| ProRes | sample description / frame header | profile, dimensions, chroma, bit depth |

### 8.2 音频

| Codec | Header 来源 | 重点字段 |
| --- | --- | --- |
| AAC | AudioSpecificConfig, ADTS | object type, sample rate, channel config, SBR/PS |
| Opus | OpusHead | version, channels, pre-skip, input sample rate, mapping family |
| Vorbis | identification/comment/setup headers | channels, sample rate, bitrate, comments |
| MP3 | frame header | MPEG version, layer, bitrate, sample rate, channel mode |
| AC-3 / E-AC-3 | syncframe | sample rate, frame size, channel mode, LFE |
| FLAC | STREAMINFO | sample rate, channels, bits per sample, total samples, MD5 |
| PCM | container format fields | sample format, sample rate, channels, endianness |

### 8.3 字幕和数据

- WebVTT。
- TTML / IMSC。
- CEA-608 / CEA-708。
- SRT。
- mov_text。
- Timed ID3。
- SCTE-35。

## 9. UI 信息架构

第一屏应是实际分析工具，而不是宣传页。

主要区域：

1. 输入区：文件上传、URL 输入、最近记录。
2. 摘要区：格式、时长、大小、码率、轨道数量、主要 codec。
3. 结构树：容器 box/chunk/element/page/packet 层级。
4. 轨道列表：视频、音频、字幕、metadata。
5. 时间线视图：track duration、sample/keyframe 分布、fragments/segments。
6. 采样表视图：sample index、offset、size、DTS、PTS、duration、sync。
7. Codec Header 视图：字段树、原始 bytes、解释说明。
8. Metadata 视图：title、artist、creation_time、language、rotation、color、HDR 等。
9. Diagnostics 视图：警告、错误、兼容性问题、可疑结构。
10. JSON 导出：完整结果、选中节点、诊断报告。

## 10. 诊断能力

后续可逐步加入规则引擎：

- 容器结构缺失或非法。
- box size、chunk size、EBML size 异常。
- sample table 计数不一致。
- duration、timescale、PTS/DTS 不连续。
- MPEG-TS continuity counter 错误。
- PCR/PTS 漂移。
- fragment base decode time 异常。
- codec extradata 与 sample bitstream 不一致。
- H.264/HEVC profile/level 与分辨率或码率不匹配。
- AAC ADTS 与 ASC 参数不一致。
- metadata 编码异常。
- 不支持 Range 的远程资源提示。

## 11. 技术选型建议

### 11.1 前端

建议：

- TypeScript。
- React 或 Vue，优先选择团队熟悉的框架。
- Vite 作为开发构建工具。
- Web Worker 承载解析任务，避免阻塞 UI。
- 虚拟列表展示大规模 sample table。
- IndexedDB 或内存 LRU 缓存 byte ranges。

### 11.2 解析核心

建议用 TypeScript 实现自有解析核心：

- 方便在浏览器和 Node.js 复用。
- 更容易输出结构化字段。
- 更适合做交互式容器树和按需读取。

可选辅助：

- FFmpeg/ffprobe 作为后端校验器或 fallback。
- MediaInfo 作为对照工具。
- mp4box.js 可参考或用于早期 MP4 验证，但核心模型应保持自有抽象。

### 11.3 后端

初期可不强依赖后端，但建议预留：

- 远程 URL 代理，解决 CORS 和 Range 限制。
- 大文件上传分析。
- ffprobe/mediainfo 对照结果。
- 分析报告持久化。
- 用户历史记录。

后端候选：

- Node.js + Fastify/NestJS。
- Go，适合高性能 range proxy 和大文件处理。
- Rust，适合长期做高性能 parser，但开发成本更高。

## 12. 目录结构建议

```text
MediaAnalyzer/
  docs/
    format-support.md
    parser-architecture.md
    diagnostics-rules.md
    test-samples.md
  packages/
    core/
      src/
        byte-source/
        detector/
        containers/
        codecs/
        model/
        diagnostics/
    web/
      src/
        app/
        components/
        workers/
        views/
    server/
      src/
        proxy/
        analyzers/
        reports/
  samples/
    README.md
  tests/
    fixtures/
    golden/
```

## 13. 数据输出格式

所有解析器最终输出统一 JSON：

```json
{
  "input": {
    "type": "file",
    "name": "sample.mp4",
    "size": 12345678
  },
  "detection": {
    "format": "mp4",
    "confidence": 0.99,
    "evidence": ["ftyp box at offset 4", "major_brand=isom"]
  },
  "container": {
    "format": "ISO-BMFF",
    "duration": {
      "value": 120000,
      "timescale": 1000,
      "seconds": 120
    },
    "tracks": [],
    "metadata": [],
    "structure": []
  },
  "diagnostics": []
}
```

## 14. 测试策略

### 14.1 单元测试

- bit reader。
- endian reader。
- varint / EBML vint。
- NAL parser。
- ADTS parser。
- ISO BMFF box parser。
- sample table 展开逻辑。

### 14.2 Golden Tests

为每类格式准备小样本，解析后生成稳定 JSON：

- sample.mp4。
- fragmented.mp4。
- sample.webm。
- sample.ts。
- sample.flv。
- sample.wav。
- sample.mp3。
- sample.opus。

每次修改 parser 后比对 JSON 差异。

### 14.3 Fuzz 和异常输入

- 随机 box size。
- 截断文件。
- 错误 magic。
- 超大 length。
- 循环引用或异常 offset。
- 不支持 Range 的 URL。

### 14.4 对照验证

使用以下工具对照：

- ffprobe。
- mediainfo。
- mp4dump。
- mkvinfo。
- tsanalyze。

## 15. 里程碑

### Milestone 0：项目骨架

目标：

- 建立 monorepo 或单体项目结构。
- 建立 TypeScript、lint、test、build。
- 建立基础 UI。
- 建立 ByteSource、FormatDetector、Result Model。
- 写入第一批测试样本说明。

交付：

- 可以上传文件。
- 可以读取前 N KB。
- 可以识别 MP4/WebM/TS/WAV/MP3 的大类。

### Milestone 1：MP4/MOV MVP

目标：

- 解析 ISO BMFF box 树。
- 解析 ftyp/moov/mvhd/trak/mdia/minf/stbl。
- 输出 track、duration、timescale、sample table 摘要。
- 解析 avcC、hvcC、esds、Opus/AAC 基础 extradata。

交付：

- MP4 文件结构树。
- 视频/音频轨道摘要。
- H.264 SPS/PPS 和 AAC ASC 可读字段。
- JSON 导出。

### Milestone 2：WebM/Matroska 和 Ogg/Opus

目标：

- 解析 EBML。
- 解析 Segment/Info/Tracks/Cues/Cluster。
- 解析 OpusHead、Vorbis headers。
- 解析 Ogg pages 和 granule position。

交付：

- WebM/MKV 轨道和时间线。
- Ogg/Opus 结构和 OpusHead 字段。

### Milestone 3：MPEG-TS/HLS

目标：

- 解析 TS packet。
- 解析 PAT/PMT/PES。
- 解析 PTS/DTS/PCR。
- 识别 H.264/H.265/AAC/AC-3 elementary streams。
- 解析 HLS m3u8。

交付：

- TS 包级统计。
- continuity counter 诊断。
- HLS variant 和 segment 列表。

### Milestone 4：更多音频和传统容器

目标：

- WAV/RF64/BWF。
- MP3/ID3/Xing/LAME。
- FLAC。
- AVI。
- FLV。

交付：

- 常见音频文件完整摘要。
- FLV AVC/AAC sequence header 解析。
- AVI 基础结构解析。

### Milestone 5：DASH、Fragment、诊断规则

目标：

- DASH MPD。
- fragmented MP4 深度解析。
- sidx/moof/traf/trun/tfdt。
- 诊断规则引擎。

交付：

- DASH period/adaptation/representation 视图。
- CMAF/fMP4 fragment 时间线。
- 第一批稳定诊断规则。

### Milestone 6：性能、大文件和远程分析

目标：

- HTTP Range 缓存。
- Web Worker。
- 大 sample table 虚拟展示。
- 后端 URL proxy。
- 报告持久化。

交付：

- 大文件不会卡死 UI。
- 远程 URL 支持 CORS fallback。
- 分析报告可保存和分享。

## 16. 风险和难点

1. 格式覆盖面极大，需要按优先级推进，避免一开始追求“全部支持”。
2. 有些容器或 codec 规范复杂，字段解析容易和实际文件生态不一致。
3. 远程 URL 会遇到 CORS、Range、不稳定网络、鉴权等问题。
4. 大文件 sample table 可能非常大，UI 必须虚拟化。
5. Codec bitstream parser 要处理 emulation prevention bytes、NAL length size、Annex B、OBU、ADTS 等不同封装形式。
6. 同一个 codec 在不同容器里的 extradata 表达不同，需要统一抽象。
7. metadata 字符编码和厂商私有字段需要谨慎处理。
8. 测试样本版权和分发需要注意，只使用可公开分发的小样本或自生成样本。

## 17. 开发原则

1. 先支持最常见路径，再补异常和边缘格式。
2. 所有 parser 输出结构化数据，不依赖展示层文本。
3. 每个 parser 都必须能报告 warnings/errors，而不是直接崩溃。
4. 解析过程应尽量可中断、可恢复、可限制读取范围。
5. UI 展示原始结构和解释字段，避免隐藏底层信息。
6. 新增格式时必须带测试样本说明和 golden output。
7. 任何“不确定”的解析结果都要标记 confidence 或 warning。

## 18. 推荐下一步

1. 初始化项目骨架。
2. 建立 `packages/core` 的 ByteSource、binary reader、format detector。
3. 实现 ISO BMFF box tree parser。
4. 做一个最小 Web UI：上传 MP4 后展示 box 树。
5. 增加 avcC、hvcC、esds 和 AAC ASC parser。
6. 建立第一批 golden tests。

## 19. 长期愿景

MediaAnalyzer 最终可以成为一个浏览器内可运行、后端可增强、适合调试和自动化验证的媒体结构工作台：

- 开发者能快速定位一个文件为什么不能播放。
- 测试人员能比较两个媒体文件结构差异。
- 平台工程师能排查 HLS/DASH 分片时间线问题。
- 编码器开发者能检查 codec headers 和 profile/level 字段。
- 用户能导出可复现、可分享、可自动化处理的分析报告。
