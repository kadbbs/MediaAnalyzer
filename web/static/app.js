const statusEl = document.querySelector("#status");
const jsonOutput = document.querySelector("#jsonOutput");
const formatEl = document.querySelector("#format");
const containerFormatEl = document.querySelector("#containerFormat");
const durationEl = document.querySelector("#duration");
const trackCountEl = document.querySelector("#trackCount");
const confidenceEl = document.querySelector("#confidence");
const inputDetails = document.querySelector("#inputDetails");
const evidenceList = document.querySelector("#evidenceList");
const beginnerInsights = document.querySelector("#beginnerInsights");
const expertFacts = document.querySelector("#expertFacts");
const glossaryList = document.querySelector("#glossaryList");
const trackSummaryRows = document.querySelector("#trackSummaryRows");
const trackList = document.querySelector("#trackList");
const structureTree = document.querySelector("#structureTree");
const boxGuide = document.querySelector("#boxGuide");
const navTrackList = document.querySelector("#navTrackList");
const byteSourceList = document.querySelector("#byteSourceList");
const parsedOutline = document.querySelector("#parsedOutline");
const byteDump = document.querySelector("#byteDump");
const byteDumpTitle = document.querySelector("#byteDumpTitle");
const diagnosticList = document.querySelector("#diagnosticList");
const selectedBoxMeta = document.querySelector("#selectedBoxMeta");
const selectedBoxExplain = document.querySelector("#selectedBoxExplain");
const selectedHexDump = document.querySelector("#selectedHexDump");
const bytesSelectionMeta = document.querySelector("#bytesSelectionMeta");
const bytesSelectionExplain = document.querySelector("#bytesSelectionExplain");
const bytesSelectedHexDump = document.querySelector("#bytesSelectedHexDump");
const uploadForm = document.querySelector("#uploadForm");
const urlForm = document.querySelector("#urlForm");
const fileInput = document.querySelector("#fileInput");
const urlInput = document.querySelector("#urlInput");
const languageSelect = document.querySelector("#languageSelect");

const dictionaries = {
  "zh-CN": {
    "app.subtitle": "容器 / 时间线 / 编码头 / 字节检查器",
    "label.language": "语言",
    "label.file": "文件",
    "action.analyze": "分析",
    "action.fetch": "拉取",
    "placeholder.url": "https://example.com/video.mp4",
    "aria.resultViews": "分析结果视图",
    "status.ready": "就绪",
    "status.chooseFile": "请选择文件",
    "status.enterUrl": "请输入 URL",
    "status.analyzing": "分析中",
    "status.done": "完成",
    "nav.source": "来源",
    "nav.views": "视图",
    "nav.tracks": "轨道",
    "nav.diagnostics": "诊断",
    "nav.evidence": "识别依据",
    "view.overview": "概览",
    "view.tracks": "轨道 / 编码",
    "view.structure": "容器树",
    "view.bytes": "字节",
    "view.json": "原始 JSON",
    "metric.format": "格式",
    "metric.container": "容器",
    "metric.duration": "时长",
    "metric.tracks": "轨道",
    "metric.confidence": "置信度",
    "panel.trackMatrix": "轨道矩阵",
    "panel.expertSnapshot": "专家快照",
    "panel.learningNotes": "学习笔记",
    "panel.glossary": "术语表",
    "panel.containerTree": "容器树",
    "panel.fullBinary": "完整二进制",
    "panel.structureIndex": "结构索引",
    "panel.inspector": "检查器",
    "panel.mappedByteRanges": "映射字节范围",
    "panel.explanation": "说明",
    "panel.selectedBytes": "选中字节",
    "panel.reference": "参考",
    "panel.codecBytes": "编码字节",
    "hint.selectNode": "选择节点以检查字节",
    "hint.mappedRanges": "映射范围",
    "field.name": "名称",
    "field.size": "大小",
    "field.bytesLoaded": "已载入字节",
    "field.truncated": "已截断",
    "field.type": "类型",
    "field.codec": "编码",
    "field.duration": "时长",
    "field.shape": "形态",
    "field.samples": "Sample",
    "field.format": "格式",
    "field.family": "家族",
    "field.majorBrand": "Major Brand",
    "field.compatibleBrands": "兼容 Brand",
    "field.movieTimescale": "Movie Timescale",
    "field.movieDurationValue": "Movie Duration Value",
    "field.topBoxes": "顶层 Box",
    "field.videoTracks": "视频轨",
    "field.audioTracks": "音频轨",
    "field.warnings": "警告",
    "field.codecFourCC": "Codec FourCC",
    "field.profile": "Profile",
    "field.level": "Level",
    "field.nalLength": "NAL 长度",
    "field.bitDepthLuma": "亮度位深",
    "field.bitDepthChroma": "色度位深",
    "field.chromaFormat": "色度格式",
    "field.audioObject": "音频对象",
    "field.ascRate": "ASC 采样率",
    "field.channelConfig": "声道配置",
    "field.timeScale": "时间基",
    "field.sampleEntries": "Sample Entries",
    "field.width": "宽度",
    "field.height": "高度",
    "field.channels": "声道",
    "field.sampleRate": "采样率",
    "field.status": "状态",
    "field.selection": "选择",
    "field.offset": "偏移",
    "field.length": "长度",
    "field.header": "Header",
    "field.previewOffset": "预览偏移",
    "field.previewLength": "预览长度",
    "field.range": "范围",
    "field.info": "信息",
    "value.yes": "是",
    "value.no": "否",
    "value.noBoxSelected": "未选择 box",
    "empty.noEvidence": "暂无识别依据",
    "empty.noTracks": "暂无轨道信息",
    "empty.noStructure": "暂无容器结构",
    "empty.noBytes": "暂无字节",
    "empty.noBoxInfo": "暂无 box 信息",
    "empty.noMappedByteRanges": "暂无映射字节范围",
    "empty.noMappedStructures": "暂无映射结构",
    "empty.noCodecByteRanges": "暂无 codec 字节范围",
    "label.input": "输入",
    "label.inputBytes": "输入字节",
    "label.inputBinary": "输入二进制",
    "label.box": "box",
    "label.track": "轨道",
    "label.sampleSuffix": "{count} samples",
    "label.size": "大小",
    "label.header": "header",
    "label.offset": "offset",
    "label.codecHeader": "Codec Header",
    "label.audioSpecificConfig": "AudioSpecificConfig",
    "outline.containerTree": "容器 / Box 树",
    "outline.tracksCodecHeaders": "轨道 / 编码头",
    "diagnostic.lowConfidence": "检测置信度较低：{confidence}",
    "diagnostic.isoNoStructure": "检测到 ISO-BMFF，但容器解析器没有返回结构",
    "diagnostic.noTracksFound": "容器已解析，但没有发现轨道",
    "diagnostic.noCodecDescription": "轨道 #{id} 还没有解析出 codec 描述",
    "diagnostic.noVideoSize": "视频轨 #{id} 还没有解析出宽高",
    "diagnostic.ok": "基础诊断未发现问题",
    "selection.noneExplanation": "选择左侧容器节点后，这里会显示解析说明和对应字节。",
    "selection.bytesExplanation": "这里显示右侧结构索引选中的局部字节；左侧始终保留整体输入二进制，方便按 offset 对照。",
    "insight.format": "<strong>{format}</strong> 是识别出的封装/格式入口；封装负责组织轨道、时间线、索引和 metadata。",
    "insight.isoBmff": "这是 ISO-BMFF 家族文件。MP4、MOV、CMAF/fMP4 都属于这个体系，核心结构由一系列 box 组成。",
    "insight.majorBrand": "<strong>{brand}</strong> 是 major brand，用来提示播放器按哪类 MP4 兼容规则理解文件。",
    "insight.tracks": "当前解析到 <strong>{count}</strong> 条轨道。每条轨道都有自己的时间基、编码格式和 sample 表。",
    "insight.track": "{type} 轨道 #{id} 使用 <strong>{codec}</strong>，{shapeText}轨道时长 {duration}。",
    "insight.trackShape": "媒体形态是 {shape}，",
    "insight.h264": "H.264 初始化信息来自 <strong>avcC</strong>。其中 SPS/PPS 会告诉解码器 profile、level、分辨率和参数集数量。",
    "insight.none": "当前格式还没有深入解析结果；可以先查看识别依据和原始 JSON。",
    "codecExp.trackType": "轨道类型",
    "codecExp.video": "视频轨道包含压缩图像 sample。容器提供时间线和索引，codec header 提供解码初始化参数。",
    "codecExp.audio": "音频轨道包含压缩或未压缩音频 sample，重点关注采样率、声道数和 codec extradata。",
    "codecExp.other": "非音视频轨道可能承载字幕、章节、metadata 或其他同步数据。",
    "codecExp.avcC": "MP4 中 H.264 常把 SPS/PPS 放在 avcC box 中。sample 内的 NALU 通常用 length 前缀分隔，而不是 Annex B start code。",
    "codecExp.h264Sets": "当前解析到 {sps} 个 SPS、{pps} 个 PPS。SPS 决定 profile、level、分辨率等关键解码参数。",
    "codecExp.profileLevel": "{profile} / {level} 描述编码工具集和复杂度上限，兼容性排查时很重要。",
    "codecExp.hvcC": "MP4 中 HEVC/H.265 常通过 hvcC 携带 VPS/SPS/PPS、profile、tier、level、bit depth 和 NAL length size。",
    "codecExp.hevcSets": "当前解析到 {vps} 个 VPS、{sps} 个 SPS、{pps} 个 PPS。排查 HEVC 兼容性时这些参数集很关键。",
    "codecExp.hevcProfile": "{profile} / {level} 描述编码工具、tier 和复杂度等级。",
    "codecExp.esds": "MP4 中 AAC 常在 esds 里携带 AudioSpecificConfig，而不是把初始化参数放到每个音频 sample 里。",
    "codecExp.asc": "当前解析到 objectType={objectType}，sampleRate={sampleRate}，channelConfig={channelConfig}。",
    "codecExp.generic": "{codec} 是这条轨道的编码格式。后续 parser 会继续补充该 codec 的 header 字段。",
    "codecExp.timescale": "timescale={timescale}，duration={duration}，换算后约 {seconds}。",
    "codecExp.sample": "当前 sample count={count}。sample 表越完整，越容易定位 seek、卡顿、时间戳和关键帧问题。",
    "box.default": "暂未内置说明。专家可先根据 offset/size 和父子层级判断它在容器中的作用。",
  },
  "en-US": {
    "app.subtitle": "Container / Timeline / Codec Header / Bytes Inspector",
    "label.language": "Language",
    "label.file": "File",
    "action.analyze": "Analyze",
    "action.fetch": "Fetch",
    "placeholder.url": "https://example.com/video.mp4",
    "aria.resultViews": "Analysis result views",
    "status.ready": "Ready",
    "status.chooseFile": "Choose a file",
    "status.enterUrl": "Enter a URL",
    "status.analyzing": "Analyzing",
    "status.done": "Done",
    "nav.source": "Source",
    "nav.views": "Views",
    "nav.tracks": "Tracks",
    "nav.diagnostics": "Diagnostics",
    "nav.evidence": "Evidence",
    "view.overview": "Overview",
    "view.tracks": "Tracks / Codec",
    "view.structure": "Container Tree",
    "view.bytes": "Bytes",
    "view.json": "Raw JSON",
    "metric.format": "Format",
    "metric.container": "Container",
    "metric.duration": "Duration",
    "metric.tracks": "Tracks",
    "metric.confidence": "Confidence",
    "panel.trackMatrix": "Track Matrix",
    "panel.expertSnapshot": "Expert Snapshot",
    "panel.learningNotes": "Learning Notes",
    "panel.glossary": "Glossary",
    "panel.containerTree": "Container Tree",
    "panel.fullBinary": "Full Binary",
    "panel.structureIndex": "Structure Index",
    "panel.inspector": "Inspector",
    "panel.mappedByteRanges": "Mapped Byte Ranges",
    "panel.explanation": "Explanation",
    "panel.selectedBytes": "Selected Bytes",
    "panel.reference": "Reference",
    "panel.codecBytes": "Codec Bytes",
    "hint.selectNode": "select a node to inspect bytes",
    "hint.mappedRanges": "mapped ranges",
    "field.name": "Name",
    "field.size": "Size",
    "field.bytesLoaded": "Bytes Loaded",
    "field.truncated": "Truncated",
    "field.type": "Type",
    "field.codec": "Codec",
    "field.duration": "Duration",
    "field.shape": "Shape",
    "field.samples": "Samples",
    "field.format": "Format",
    "field.family": "Family",
    "field.majorBrand": "Major Brand",
    "field.compatibleBrands": "Compatible Brands",
    "field.movieTimescale": "Movie Timescale",
    "field.movieDurationValue": "Movie Duration Value",
    "field.topBoxes": "Top Boxes",
    "field.videoTracks": "Video Tracks",
    "field.audioTracks": "Audio Tracks",
    "field.warnings": "Warnings",
    "field.codecFourCC": "Codec FourCC",
    "field.profile": "Profile",
    "field.level": "Level",
    "field.nalLength": "NAL Length",
    "field.bitDepthLuma": "Bit Depth Luma",
    "field.bitDepthChroma": "Bit Depth Chroma",
    "field.chromaFormat": "Chroma Format",
    "field.audioObject": "Audio Object",
    "field.ascRate": "ASC Rate",
    "field.channelConfig": "Channel Config",
    "field.timeScale": "Time Scale",
    "field.sampleEntries": "Sample Entries",
    "field.width": "Width",
    "field.height": "Height",
    "field.channels": "Channels",
    "field.sampleRate": "Sample Rate",
    "field.status": "Status",
    "field.selection": "Selection",
    "field.offset": "Offset",
    "field.length": "Length",
    "field.header": "Header",
    "field.previewOffset": "Preview Offset",
    "field.previewLength": "Preview Length",
    "field.range": "Range",
    "field.info": "Info",
    "value.yes": "yes",
    "value.no": "no",
    "value.noBoxSelected": "No box selected",
    "empty.noEvidence": "No evidence yet",
    "empty.noTracks": "No track information",
    "empty.noStructure": "No container structure",
    "empty.noBytes": "No bytes",
    "empty.noBoxInfo": "No box reference",
    "empty.noMappedByteRanges": "No mapped byte ranges",
    "empty.noMappedStructures": "No mapped structures",
    "empty.noCodecByteRanges": "No codec byte ranges",
    "label.input": "Input",
    "label.inputBytes": "input bytes",
    "label.inputBinary": "Input binary",
    "label.box": "box",
    "label.track": "track",
    "label.sampleSuffix": "{count} samples",
    "label.size": "size",
    "label.header": "header",
    "label.offset": "offset",
    "label.codecHeader": "Codec Header",
    "label.audioSpecificConfig": "AudioSpecificConfig",
    "outline.containerTree": "Container / Box Tree",
    "outline.tracksCodecHeaders": "Tracks / Codec Headers",
    "diagnostic.lowConfidence": "low detection confidence: {confidence}",
    "diagnostic.isoNoStructure": "ISO-BMFF detected but container parser did not return a structure",
    "diagnostic.noTracksFound": "container parsed but no tracks were found",
    "diagnostic.noCodecDescription": "track #{id} has no parsed codec description",
    "diagnostic.noVideoSize": "video track #{id} has no parsed width/height yet",
    "diagnostic.ok": "no basic diagnostics triggered",
    "selection.noneExplanation": "Select a container node to inspect its parsed meaning and mapped bytes.",
    "selection.bytesExplanation": "This panel shows the local bytes selected from the structure index. The full input binary remains on the left for offset-level comparison.",
    "insight.format": "<strong>{format}</strong> is the detected container or format entry point. The container organizes tracks, timelines, indexes, and metadata.",
    "insight.isoBmff": "This is an ISO-BMFF family file. MP4, MOV, CMAF, and fMP4 belong to this family, and the core structure is a sequence of boxes.",
    "insight.majorBrand": "<strong>{brand}</strong> is the major brand, which hints which MP4 compatibility rules a player should apply.",
    "insight.tracks": "The parser found <strong>{count}</strong> track(s). Each track has its own timescale, codec format, and sample tables.",
    "insight.track": "{type} track #{id} uses <strong>{codec}</strong>; {shapeText}track duration is {duration}.",
    "insight.trackShape": "media shape is {shape}; ",
    "insight.h264": "H.264 initialization data comes from <strong>avcC</strong>. SPS/PPS tell the decoder the profile, level, resolution, and parameter-set counts.",
    "insight.none": "This format does not have deep parsed results yet. Start with the evidence list and raw JSON.",
    "codecExp.trackType": "Track Type",
    "codecExp.video": "A video track contains compressed image samples. The container provides the timeline and indexes, while codec headers provide decoder initialization parameters.",
    "codecExp.audio": "An audio track contains compressed or uncompressed audio samples. Sample rate, channel count, and codec extradata are the key fields.",
    "codecExp.other": "A non-audio/video track may carry subtitles, chapters, metadata, or other synchronized data.",
    "codecExp.avcC": "In MP4, H.264 usually stores SPS/PPS inside the avcC box. NAL units in samples are usually length-prefixed instead of Annex B start-code delimited.",
    "codecExp.h264Sets": "Parsed {sps} SPS and {pps} PPS. SPS determines profile, level, resolution, and other key decoder parameters.",
    "codecExp.profileLevel": "{profile} / {level} describe the coding toolset and complexity limit, which matter for compatibility checks.",
    "codecExp.hvcC": "In MP4, HEVC/H.265 usually carries VPS/SPS/PPS, profile, tier, level, bit depth, and NAL length size through hvcC.",
    "codecExp.hevcSets": "Parsed {vps} VPS, {sps} SPS, and {pps} PPS. These parameter sets are critical for HEVC compatibility analysis.",
    "codecExp.hevcProfile": "{profile} / {level} describe the coding tools, tier, and complexity level.",
    "codecExp.esds": "In MP4, AAC usually carries AudioSpecificConfig in esds rather than repeating initialization parameters in every audio sample.",
    "codecExp.asc": "Parsed objectType={objectType}, sampleRate={sampleRate}, channelConfig={channelConfig}.",
    "codecExp.generic": "{codec} is the codec format for this track. Future parsers can add more header fields for it.",
    "codecExp.timescale": "timescale={timescale}, duration={duration}, converted duration is about {seconds}.",
    "codecExp.sample": "Current sample count={count}. More complete sample tables make it easier to locate seek points, stalls, timestamps, and keyframes.",
    "box.default": "No built-in explanation yet. Experts can infer its role from offset/size and the parent-child box hierarchy.",
  },
};

const boxDescriptions = {
  "zh-CN": {
    ftyp: "文件类型和品牌声明。播放器通常用它判断这是 MP4、MOV、CMAF 还是其他 ISO-BMFF 变体。",
    moov: "电影级元数据。包含时长、轨道、采样表、编码描述等信息；没有 moov，播放器通常无法完整建立时间线。",
    mvhd: "全局 movie header。保存整个文件的 timescale、duration 和下一个 track id。",
    trak: "一条轨道。视频、音频、字幕、metadata 通常各自对应一个 trak。",
    tkhd: "轨道 header。保存 track id、启用状态，以及视频轨道常见的显示宽高。",
    mdia: "媒体信息容器。描述这条轨道的时间基、类型和媒体数据组织方式。",
    mdhd: "媒体 header。保存轨道自己的 timescale 和 duration，是计算 PTS/DTS 的基础。",
    hdlr: "handler。说明轨道类型，例如 vide 表示视频，soun 表示音频。",
    minf: "媒体信息。根据轨道类型包含视频、音频或字幕的具体组织信息。",
    stbl: "sample table。MP4 分析最核心的区域，描述 sample 的时间、大小、偏移、关键帧和解码顺序。",
    stsd: "sample description。描述编码格式和 extradata，例如 avc1/hvc1/mp4a 以及 avcC/hvcC/esds。",
    stts: "decoding time to sample。把 sample 映射到 DTS 时间线。",
    ctts: "composition time to sample。描述 B 帧等场景下 PTS 相对 DTS 的偏移。",
    stsc: "sample to chunk。说明 sample 如何打包进 chunk。",
    stsz: "sample size。保存每个 sample 的大小，或声明固定 sample size。",
    stco: "chunk offset。32 位 chunk 文件偏移。",
    co64: "chunk offset。64 位 chunk 文件偏移，常见于大文件。",
    stss: "sync sample。关键帧表，seek 和切片定位会依赖它。",
    avc1: "H.264 视频 sample entry。包含显示尺寸，以及 avcC 这类 codec private data。",
    avc3: "H.264 视频 sample entry。通常表示参数集可能随 sample 出现，而不只在 avcC 中。",
    avcC: "AVCDecoderConfigurationRecord。保存 H.264 SPS/PPS、profile、level 和 NAL length size。",
    hvc1: "HEVC 视频 sample entry。包含 hvcC，参数集通常在 sample entry 中声明。",
    hev1: "HEVC 视频 sample entry。参数集也可能随 sample 出现。",
    hvcC: "HEVCDecoderConfigurationRecord。保存 HEVC VPS/SPS/PPS、profile、tier、level 等信息。",
    mp4a: "MPEG-4 Audio sample entry。AAC 常通过 esds 或 AudioSpecificConfig 描述。",
    esds: "Elementary Stream Descriptor。AAC 等编码常在这里携带 AudioSpecificConfig。",
    mdat: "媒体数据本体。真正的压缩音视频 sample bytes 通常在这里。",
    moof: "fragmented MP4 的 fragment metadata。直播/CMAF/fMP4 常见。",
    traf: "fragment 内单条轨道的 fragment metadata。",
    trun: "fragment sample run。描述 fragment 中 sample 的大小、时长、flag 等。",
    tfdt: "fragment base decode time。fMP4 时间线对齐的关键字段。",
    sidx: "segment index。常用于 DASH/CMAF 分片索引。",
  },
  "en-US": {
    ftyp: "File type and brand declaration. Players use it to identify MP4, MOV, CMAF, or another ISO-BMFF variant.",
    moov: "Movie-level metadata. It contains duration, tracks, sample tables, and codec descriptions. Without moov, a player usually cannot build a complete timeline.",
    mvhd: "Global movie header. Stores the movie timescale, duration, and next track id.",
    trak: "A track container. Video, audio, subtitles, and metadata are usually represented by separate trak boxes.",
    tkhd: "Track header. Stores track id, enabled state, and common display width/height fields for video tracks.",
    mdia: "Media container. Describes the track timescale, media type, and media organization.",
    mdhd: "Media header. Stores the track timescale and duration; it is the base for DTS/PTS conversion.",
    hdlr: "Handler. Identifies the track type, such as vide for video or soun for audio.",
    minf: "Media information. Contains video, audio, subtitle, or metadata-specific organization.",
    stbl: "Sample table. The core MP4 analysis area: sample timing, sizes, offsets, keyframes, and decode order.",
    stsd: "Sample description. Describes codec format and extradata such as avc1/hvc1/mp4a plus avcC/hvcC/esds.",
    stts: "Decoding time to sample. Maps samples to the DTS timeline.",
    ctts: "Composition time to sample. Describes PTS offsets from DTS, common with B-frames.",
    stsc: "Sample to chunk. Describes how samples are grouped into chunks.",
    stsz: "Sample size table. Stores per-sample sizes or declares a fixed sample size.",
    stco: "32-bit chunk offset table.",
    co64: "64-bit chunk offset table, common in large files.",
    stss: "Sync sample table. Keyframe indexes used by seeking and segment localization.",
    avc1: "H.264 video sample entry. Contains display dimensions and codec private data such as avcC.",
    avc3: "H.264 video sample entry. Usually indicates parameter sets may also appear in samples, not only in avcC.",
    avcC: "AVCDecoderConfigurationRecord. Stores H.264 SPS/PPS, profile, level, and NAL length size.",
    hvc1: "HEVC video sample entry. Contains hvcC; parameter sets are usually declared in the sample entry.",
    hev1: "HEVC video sample entry. Parameter sets may also appear in samples.",
    hvcC: "HEVCDecoderConfigurationRecord. Stores HEVC VPS/SPS/PPS, profile, tier, level, and related fields.",
    mp4a: "MPEG-4 Audio sample entry. AAC is commonly described through esds or AudioSpecificConfig.",
    esds: "Elementary Stream Descriptor. Often carries AudioSpecificConfig for AAC and similar codecs.",
    mdat: "Media data payload. The actual compressed audio/video sample bytes usually live here.",
    moof: "Fragmented MP4 fragment metadata. Common in live streaming, CMAF, and fMP4.",
    traf: "Track fragment metadata for one track inside a fragment.",
    trun: "Track fragment sample run. Describes sample sizes, durations, flags, and related fragment fields.",
    tfdt: "Fragment base decode time. A key field for fMP4 timeline alignment.",
    sidx: "Segment index. Commonly used by DASH/CMAF segment indexes.",
  },
};

const glossaries = {
  "zh-CN": [
    ["容器", "负责组织媒体数据和索引，例如 MP4/WebM/TS。它不等于编码格式。"],
    ["轨道", "一条独立时间线。一个文件通常有视频轨、音频轨，也可能有字幕或 metadata 轨。"],
    ["Sample", "容器层最小媒体访问单元。视频里通常接近一帧，音频里通常是一段音频帧。"],
    ["Timescale", "时间单位刻度。duration / timescale = 秒。MP4 里 movie 和 track 可以有不同 timescale。"],
    ["DTS/PTS", "DTS 是解码时间，PTS 是显示时间。含 B 帧的视频里二者可能不同。"],
    ["SPS/PPS", "H.264 参数集。SPS 描述分辨率/profile/level 等序列参数，PPS 描述图像参数。"],
    ["VPS/SPS/PPS", "HEVC/H.265 使用 VPS、SPS、PPS 三类参数集，VPS 描述视频参数集合，SPS/PPS 描述序列和图像参数。"],
    ["AudioSpecificConfig", "AAC 的初始化配置，通常来自 MP4 的 esds，包含音频对象类型、采样率和声道配置。"],
    ["Extradata", "容器里保存的 codec 初始化数据，例如 avcC、hvcC、AudioSpecificConfig。"],
    ["关键帧", "可以独立解码的帧。播放器 seek、切片和首屏速度都很依赖关键帧位置。"],
  ],
  "en-US": [
    ["Container", "Organizes media data and indexes, such as MP4, WebM, or TS. It is not the same as the codec format."],
    ["Track", "An independent timeline. A file usually has video and audio tracks, and may also include subtitles or metadata tracks."],
    ["Sample", "The smallest media access unit at the container layer. In video it is often close to a frame; in audio it is usually an audio frame group."],
    ["Timescale", "The time unit scale. duration / timescale = seconds. MP4 movie and track timescales may differ."],
    ["DTS/PTS", "DTS is decode time and PTS is presentation time. They can differ when video uses B-frames."],
    ["SPS/PPS", "H.264 parameter sets. SPS describes sequence parameters such as resolution/profile/level, while PPS describes picture parameters."],
    ["VPS/SPS/PPS", "HEVC/H.265 uses VPS, SPS, and PPS. VPS describes a video parameter set, while SPS/PPS describe sequence and picture parameters."],
    ["AudioSpecificConfig", "AAC initialization config, often from MP4 esds, containing audio object type, sample rate, and channel configuration."],
    ["Extradata", "Codec initialization data stored in the container, such as avcC, hvcC, or AudioSpecificConfig."],
    ["Keyframe", "A frame that can be decoded independently. Seeking, segmenting, and startup latency depend heavily on keyframe positions."],
  ],
};

let locale = localStorage.getItem("mediaAnalyzer.locale") || "zh-CN";
if (!dictionaries[locale]) {
  locale = "zh-CN";
}
let currentPayload = null;
let currentStatus = { key: "status.ready", params: {}, raw: null, isError: false };
let activeByteRange = null;
let mainHexView = null;
let mainHexRenderFrame = 0;
const HEX_BYTES_PER_LINE = 16;
const HEX_ROW_HEIGHT = 21;
const HEX_OVERSCAN_LINES = 24;

function t(key, params = {}) {
  const source = dictionaries[locale] || dictionaries["zh-CN"];
  const fallback = dictionaries["en-US"];
  const template = source[key] || fallback[key] || key;
  return template.replace(/\{(\w+)\}/g, (_, name) => valueOrDash(params[name]));
}

function yesNo(value) {
  return value ? t("value.yes") : t("value.no");
}

function applyStaticTranslations() {
  document.documentElement.lang = locale;
  if (languageSelect) {
    languageSelect.value = locale;
  }
  document.querySelectorAll("[data-i18n]").forEach((element) => {
    element.textContent = t(element.dataset.i18n);
  });
  document.querySelectorAll("[data-i18n-placeholder]").forEach((element) => {
    element.setAttribute("placeholder", t(element.dataset.i18nPlaceholder));
  });
  document.querySelectorAll("[data-i18n-aria-label]").forEach((element) => {
    element.setAttribute("aria-label", t(element.dataset.i18nAriaLabel));
  });
  renderStatus();
}

languageSelect?.addEventListener("change", () => {
  locale = languageSelect.value;
  localStorage.setItem("mediaAnalyzer.locale", locale);
  applyStaticTranslations();
  if (currentPayload) {
    render(currentPayload);
  }
});

document.querySelectorAll(".tab").forEach((tab) => {
  tab.addEventListener("click", () => {
    activateView(tab.dataset.view);
  });
});

function activateView(view) {
  document.body.dataset.view = view;
  document.querySelectorAll(".tab").forEach((item) => {
    item.classList.toggle("active", item.dataset.view === view);
  });
  document.querySelectorAll(".view").forEach((panel) => {
    panel.classList.toggle("active", panel.id === `${view}View`);
  });
}

applyStaticTranslations();
activateView("bytes");

uploadForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const file = fileInput.files[0];
  if (!file) {
    setStatus("status.chooseFile", true);
    return;
  }

  const formData = new FormData();
  formData.append("file", file);
  await analyze({
    body: formData,
  });
});

urlForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const url = urlInput.value.trim();
  if (!url) {
    setStatus("status.enterUrl", true);
    return;
  }

  await analyze({
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ url }),
  });
});

async function analyze(options) {
  setStatus("status.analyzing", false);
  try {
    const response = await fetch("/api/analyze", {
      method: "POST",
      ...options,
    });
    const text = await response.text();
    if (!response.ok) {
      throw new Error(text || response.statusText);
    }
    const payload = JSON.parse(text);
    render(payload);
    setStatus("status.done", false);
  } catch (error) {
    setStatus(error.message, true, {}, true);
  }
}

function render(payload) {
  currentPayload = payload;
  const detection = payload.detection || {};
  const container = payload.container || {};
  const tracks = Array.isArray(container.tracks) ? container.tracks : [];

  formatEl.textContent = detection.format || "-";
  containerFormatEl.textContent = container.format || detection.family || "-";
  durationEl.textContent = formatDuration(container.duration);
  trackCountEl.textContent = String(tracks.length);
  confidenceEl.textContent = typeof detection.confidence === "number"
    ? detection.confidence.toFixed(2)
    : "-";

  renderInput(payload.input || {});
  renderEvidence(detection.evidence || []);
  renderLearning(payload, tracks);
  renderDiagnostics(payload, tracks);
  renderTrackSummary(tracks);
  renderNavTracks(tracks);
  renderTracks(tracks);
  renderStructure(container.structure || []);
  renderBoxGuide(container.structure || []);
  renderByteSources(payload);
  renderParsedOutline(payload);
  jsonOutput.textContent = JSON.stringify(payload, null, 2);
}

function renderInput(input) {
  const byteInfo = input.bytes || {};
  const fields = [
    [t("field.name"), input.name || "-"],
    [t("field.size"), typeof input.size === "number" ? formatBytes(input.size) : "-"],
    [t("field.bytesLoaded"), typeof byteInfo.length === "number" ? formatBytes(byteInfo.length) : "-"],
    [t("field.truncated"), yesNo(byteInfo.truncated)],
  ];
  inputDetails.replaceChildren(...fields.map(([key, value]) => definitionRow(key, value)));
}

function renderEvidence(items) {
  if (!items.length) {
    evidenceList.replaceChildren(emptyItem(t("empty.noEvidence")));
    return;
  }
  evidenceList.replaceChildren(...items.map((item) => {
    const li = document.createElement("li");
    li.textContent = item;
    return li;
  }));
}

function renderDiagnostics(payload, tracks) {
  const detection = payload.detection || {};
  const container = payload.container || {};
  const diagnostics = [];
  (container.warnings || []).forEach((warning) => {
    diagnostics.push(["warn", warning]);
  });
  if (typeof detection.confidence === "number" && detection.confidence < 0.75) {
    diagnostics.push(["warn", t("diagnostic.lowConfidence", { confidence: detection.confidence.toFixed(2) })]);
  }
  if (detection.format === "iso-bmff" && !container.format) {
    diagnostics.push(["error", t("diagnostic.isoNoStructure")]);
  }
  if (container.format && !tracks.length) {
    diagnostics.push(["warn", t("diagnostic.noTracksFound")]);
  }
  tracks.forEach((track) => {
    if (!track.codec || !track.codec.description) {
      diagnostics.push(["warn", t("diagnostic.noCodecDescription", { id: valueOrDash(track.id) })]);
    }
    if (track.type === "video" && !(track.width || (track.codec && track.codec.width))) {
      diagnostics.push(["info", t("diagnostic.noVideoSize", { id: valueOrDash(track.id) })]);
    }
  });

  if (!diagnostics.length) {
    diagnostics.push(["ok", t("diagnostic.ok")]);
  }

  diagnosticList.replaceChildren(...diagnostics.map(([level, text]) => {
    const item = document.createElement("div");
    item.className = `diagnostic ${level}`;
    const badge = document.createElement("span");
    const message = document.createElement("strong");
    badge.textContent = level;
    message.textContent = text;
    item.append(badge, message);
    return item;
  }));
}

function renderLearning(payload, tracks) {
  const container = payload.container || {};
  const detection = payload.detection || {};
  const insights = buildBeginnerInsights(payload, tracks);
  beginnerInsights.replaceChildren(...insights.map((item) => {
    const li = document.createElement("li");
    li.innerHTML = item;
    return li;
  }));

  const topBoxes = (container.structure || []).map((node) => node.type).join(", ") || "-";
  const videoTracks = tracks.filter((track) => track.type === "video").length;
  const audioTracks = tracks.filter((track) => track.type === "audio").length;
  const facts = [
    [t("field.format"), detection.format || "-"],
    [t("field.family"), detection.family || "-"],
    [t("field.majorBrand"), container.major_brand || "-"],
    [t("field.compatibleBrands"), (container.compatible_brands || []).join(", ") || "-"],
    [t("field.movieTimescale"), container.duration && container.duration.timescale],
    [t("field.movieDurationValue"), container.duration && container.duration.value],
    [t("field.topBoxes"), topBoxes],
    [t("field.videoTracks"), videoTracks],
    [t("field.audioTracks"), audioTracks],
    [t("field.warnings"), (container.warnings || []).length],
  ];
  expertFacts.replaceChildren(...facts.map(([key, value]) => definitionRow(key, valueOrDash(value))));

  glossaryList.replaceChildren(...(glossaries[locale] || glossaries["zh-CN"]).map(([term, text]) => {
    const item = document.createElement("div");
    item.className = "glossary-item";
    const strong = document.createElement("strong");
    const p = document.createElement("p");
    strong.textContent = term;
    p.textContent = text;
    item.append(strong, p);
    return item;
  }));
}

function buildBeginnerInsights(payload, tracks) {
  const container = payload.container || {};
  const detection = payload.detection || {};
  const items = [];
  if (detection.format) {
    items.push(t("insight.format", { format: escapeHtml(detection.format) }));
  }
  if (container.format === "ISO-BMFF") {
    items.push(t("insight.isoBmff"));
  }
  if (container.major_brand) {
    items.push(t("insight.majorBrand", { brand: escapeHtml(container.major_brand) }));
  }
  if (tracks.length) {
    items.push(t("insight.tracks", { count: tracks.length }));
  }
  tracks.slice(0, 3).forEach((track) => {
    const codec = codecLabel(track.codec);
    const shape = trackShape(track);
    items.push(t("insight.track", {
      type: escapeHtml(track.type || "unknown"),
      id: valueOrDash(track.id),
      codec: escapeHtml(codec),
      shapeText: shape !== "-" ? t("insight.trackShape", { shape: escapeHtml(shape) }) : "",
      duration: escapeHtml(formatDuration(track.duration)),
    }));
  });
  const h264 = tracks.find((track) => track.codec && /H\.264|AVC/.test(track.codec.description || ""));
  if (h264 && h264.codec) {
    items.push(t("insight.h264"));
  }
  if (!items.length) {
    items.push(t("insight.none"));
  }
  return items;
}

function renderTrackSummary(tracks) {
  if (!tracks.length) {
    const row = document.createElement("tr");
    const cell = document.createElement("td");
    cell.colSpan = 6;
    cell.className = "empty";
    cell.textContent = t("empty.noTracks");
    row.append(cell);
    trackSummaryRows.replaceChildren(row);
    return;
  }

  trackSummaryRows.replaceChildren(...tracks.map((track) => {
    const row = document.createElement("tr");
    [
      valueOrDash(track.id),
      valueOrDash(track.type),
      codecLabel(track.codec),
      formatDuration(track.duration),
      trackShape(track),
      valueOrDash(track.sample_count),
    ].forEach((value) => {
      const cell = document.createElement("td");
      cell.textContent = value;
      row.append(cell);
    });
    return row;
  }));
}

function renderNavTracks(tracks) {
  if (!tracks.length) {
    navTrackList.replaceChildren(emptyBlock(t("empty.noTracks")));
    return;
  }
  navTrackList.replaceChildren(...tracks.map((track, index) => {
    const button = document.createElement("button");
    button.className = "nav-item";
    button.type = "button";
    button.textContent = `#${valueOrDash(track.id)} ${valueOrDash(track.type)} · ${codecLabel(track.codec)}`;
    button.addEventListener("click", () => {
      activateView("tracks");
      const target = document.querySelector(`[data-track-index="${index}"]`);
      target?.scrollIntoView({ behavior: "smooth", block: "start" });
    });
    return button;
  }));
}

function renderTracks(tracks) {
  if (!tracks.length) {
    trackList.replaceChildren(emptyBlock(t("empty.noTracks")));
    return;
  }

  const maxDuration = Math.max(
    ...tracks.map((track) => seconds(track.duration)).filter((value) => value > 0),
    1,
  );
  trackList.replaceChildren(...tracks.map((track, index) => trackPanel(track, maxDuration, index)));
}

function trackPanel(track, maxDuration, index) {
  const panel = document.createElement("article");
  panel.className = "track-panel";
  panel.dataset.trackIndex = String(index);

  const head = document.createElement("div");
  head.className = "track-head";

  const title = document.createElement("div");
  title.className = "track-title";
  const strong = document.createElement("strong");
  strong.textContent = `${valueOrDash(track.type)} ${t("label.track")} #${valueOrDash(track.id)}`;
  const codec = document.createElement("span");
  codec.textContent = codecLabel(track.codec);
  title.append(strong, codec);

  const tags = document.createElement("div");
  tags.className = "tag-row";
  [
    formatDuration(track.duration),
    trackShape(track),
    t("label.sampleSuffix", { count: valueOrDash(track.sample_count) }),
  ].forEach((value) => {
    const tag = document.createElement("span");
    tag.className = "tag";
    tag.textContent = value;
    tags.append(tag);
  });

  head.append(title, tags);

  const timeline = document.createElement("div");
  timeline.className = "timeline";
  const fill = document.createElement("div");
  fill.className = "timeline-fill";
  fill.style.width = `${Math.max(2, Math.min(100, (seconds(track.duration) / maxDuration) * 100))}%`;
  timeline.append(fill);

  const fields = document.createElement("div");
  fields.className = "field-grid";
  const codecInfo = track.codec || {};
  [
    [t("field.codecFourCC"), codecInfo.fourcc],
    [t("field.codec"), codecInfo.description],
    [t("field.profile"), codecInfo.profile],
    [t("field.level"), codecInfo.level],
    [t("field.nalLength"), codecInfo.length_size],
    ["VPS", codecInfo.vps_count],
    ["SPS", codecInfo.sps_count],
    ["PPS", codecInfo.pps_count],
    [t("field.bitDepthLuma"), codecInfo.bit_depth_luma],
    [t("field.bitDepthChroma"), codecInfo.bit_depth_chroma],
    [t("field.chromaFormat"), codecInfo.chroma_format],
    [t("field.audioObject"), codecInfo.audio_object_type],
    [t("field.ascRate"), codecInfo.asc_sample_rate],
    [t("field.channelConfig"), codecInfo.channel_config],
    [t("field.timeScale"), track.duration && track.duration.timescale],
    [t("field.samples"), track.sample_count],
    [t("field.sampleEntries"), track.sample_description_count],
    [t("field.width"), track.width || codecInfo.width],
    [t("field.height"), track.height || codecInfo.height],
    [t("field.channels"), track.channel_count],
    [t("field.sampleRate"), track.sample_rate],
  ].forEach(([key, value]) => {
    fields.append(field(key, valueOrDash(value)));
  });

  const explanations = document.createElement("div");
  explanations.className = "explain-grid";
  codecExplanations(track).forEach(([title, text]) => {
    explanations.append(explain(title, text));
  });

  const codecBytes = codecBytePanel(codecInfo);

  panel.append(head, timeline, fields, explanations);
  if (codecBytes) {
    panel.append(codecBytes);
  }
  return panel;
}

function codecBytePanel(codecInfo) {
  const sections = [
    [t("label.codecHeader"), codecInfo.raw_header_hex],
    ["VPS", codecInfo.vps_hex],
    ["SPS", codecInfo.sps_hex],
    ["PPS", codecInfo.pps_hex],
    [t("label.audioSpecificConfig"), codecInfo.asc_hex],
  ].filter(([, hex]) => hex);
  if (!sections.length) {
    return null;
  }

  const panel = document.createElement("div");
  panel.className = "byte-panel";
  const title = document.createElement("h2");
  title.textContent = t("panel.codecBytes");
  panel.append(title);
  sections.forEach(([label, hex]) => {
    const block = document.createElement("div");
    block.className = "byte-section";
    const heading = document.createElement("div");
    heading.className = "byte-heading";
    heading.textContent = label;
    block.append(heading, hexDumpFromHex(hex, 0));
    panel.append(block);
  });
  return panel;
}

function codecExplanations(track) {
  const codec = track.codec || {};
  const rows = [];
  if (track.type === "video") {
    rows.push([t("codecExp.trackType"), t("codecExp.video")]);
  } else if (track.type === "audio") {
    rows.push([t("codecExp.trackType"), t("codecExp.audio")]);
  } else {
    rows.push([t("codecExp.trackType"), t("codecExp.other")]);
  }

  if (codec.description === "H.264/AVC") {
    rows.push(["avcC", t("codecExp.avcC")]);
    rows.push(["SPS/PPS", t("codecExp.h264Sets", { sps: valueOrDash(codec.sps_count), pps: valueOrDash(codec.pps_count) })]);
    rows.push(["Profile/Level", t("codecExp.profileLevel", { profile: valueOrDash(codec.profile), level: valueOrDash(codec.level) })]);
  } else if (codec.description === "H.265/HEVC") {
    rows.push(["hvcC", t("codecExp.hvcC")]);
    rows.push(["VPS/SPS/PPS", t("codecExp.hevcSets", {
      vps: valueOrDash(codec.vps_count),
      sps: valueOrDash(codec.sps_count),
      pps: valueOrDash(codec.pps_count),
    })]);
    rows.push(["Profile/Tier/Level", t("codecExp.hevcProfile", { profile: valueOrDash(codec.profile), level: valueOrDash(codec.level) })]);
  } else if (codec.description === "AAC") {
    rows.push(["esds", t("codecExp.esds")]);
    rows.push(["AudioSpecificConfig", t("codecExp.asc", {
      objectType: valueOrDash(codec.audio_object_type),
      sampleRate: valueOrDash(codec.asc_sample_rate),
      channelConfig: valueOrDash(codec.channel_config),
    })]);
  } else if (codec.description) {
    rows.push([t("field.codec"), t("codecExp.generic", { codec: codec.description })]);
  }

  rows.push([t("field.timeScale"), t("codecExp.timescale", {
    timescale: valueOrDash(track.duration && track.duration.timescale),
    duration: valueOrDash(track.duration && track.duration.value),
    seconds: formatDuration(track.duration),
  })]);
  rows.push(["Sample", t("codecExp.sample", { count: valueOrDash(track.sample_count) })]);
  return rows;
}

function renderStructure(nodes) {
  if (!nodes.length) {
    structureTree.replaceChildren(emptyBlock(t("empty.noStructure")));
    renderSelectedBox(null);
    return;
  }
  structureTree.replaceChildren(...nodes.map((node, index) => treeNode(node, index === 0)));
  renderSelectedBox(nodes[0]);
}

function treeNode(node, open) {
  const details = document.createElement("details");
  details.open = open;

  const summary = document.createElement("summary");
  const type = document.createElement("span");
  type.className = "box-type";
  type.textContent = node.type || "box";
  const offset = document.createElement("span");
  offset.className = "box-meta";
  offset.textContent = `${t("label.offset")} ${valueOrDash(node.offset)}`;
  const size = document.createElement("span");
  size.className = "box-meta";
  size.textContent = `${t("label.size")} ${valueOrDash(node.size)}`;
  const note = document.createElement("span");
  note.className = "box-note";
  note.textContent = `${t("label.header")} ${valueOrDash(node.header_size)} · ${boxDescription(node.type)}`;
  summary.append(type, offset, size, note);
  summary.addEventListener("click", () => {
    document.querySelectorAll(".tree summary.selected").forEach((item) => {
      item.classList.remove("selected");
    });
    summary.classList.add("selected");
    renderSelectedBox(node);
  });
  details.append(summary);

  const children = Array.isArray(node.children) ? node.children : [];
  if (children.length) {
    const childWrap = document.createElement("div");
    childWrap.className = "tree-children";
    children.forEach((child) => childWrap.append(treeNode(child, false)));
    details.append(childWrap);
  }
  return details;
}

function renderSelectedBox(node) {
  if (!node) {
    const rows = [[t("field.status"), t("value.noBoxSelected")]];
    const explanation = t("selection.noneExplanation");
    selectedBoxMeta.replaceChildren(...rows.map(([key, value]) => definitionRow(key, value)));
    selectedBoxExplain.textContent = explanation;
    selectedHexDump.replaceChildren(emptyBlock(t("empty.noBytes")));
    renderInlineSelection(rows, `<span>${escapeHtml(explanation)}</span>`, null);
    selectMappedRange(null);
    return;
  }

  const byteInfo = node.bytes || {};
  const rows = [
    [t("field.type"), node.type],
    [t("field.offset"), node.offset],
    [t("field.size"), node.size],
    [t("field.header"), node.header_size],
    [t("field.previewOffset"), byteInfo.offset],
    [t("field.previewLength"), byteInfo.length],
    [t("field.truncated"), yesNo(byteInfo.truncated)],
  ];
  const explanation = `<strong>${escapeHtml(node.type || "box")}</strong><span>${escapeHtml(boxDescription(node.type))}</span>`;
  selectedBoxMeta.replaceChildren(...rows.map(([key, value]) => definitionRow(key, value)));
  selectedBoxExplain.innerHTML = explanation;
  selectedHexDump.replaceChildren(hexDumpFromHex(byteInfo.hex || "", byteInfo.offset || 0, byteInfo.ascii || ""));
  renderInlineSelection(rows, explanation, byteInfo);
  selectMappedRange(boxRange(node));
}

function renderSelectedBytes(title, bytes, rows = [], explanation = "", mapRange = null) {
  const byteInfo = bytes || {};
  const rowData = [
    [t("field.selection"), title || "bytes"],
    [t("field.offset"), valueOrDash(byteInfo.offset)],
    [t("field.length"), valueOrDash(byteInfo.length || hexByteLength(byteInfo.hex || ""))],
    [t("field.truncated"), yesNo(byteInfo.truncated)],
    ...rows,
  ];
  const explanationHtml = `<strong>${escapeHtml(title || "bytes")}</strong><span>${escapeHtml(explanation || t("selection.bytesExplanation"))}</span>`;
  selectedBoxMeta.replaceChildren(...rowData.map(([key, value]) => definitionRow(key, value)));
  selectedBoxExplain.innerHTML = explanationHtml;
  selectedHexDump.replaceChildren(hexDumpFromHex(byteInfo.hex || "", byteInfo.offset || 0, byteInfo.ascii || ""));
  renderInlineSelection(rowData, explanationHtml, byteInfo);
  selectMappedRange(mapRange);
}

function renderInlineSelection(rows, explanationHtml, bytes) {
  if (!bytesSelectionMeta || !bytesSelectionExplain || !bytesSelectedHexDump) {
    return;
  }
  bytesSelectionMeta.replaceChildren(...rows.map(([key, value]) => definitionRow(key, value)));
  bytesSelectionExplain.innerHTML = explanationHtml || "";
  if (!bytes || !bytes.hex) {
    bytesSelectedHexDump.replaceChildren(emptyBlock(t("empty.noBytes")));
    return;
  }
  bytesSelectedHexDump.replaceChildren(hexDumpFromHex(bytes.hex || "", bytes.offset || 0, bytes.ascii || ""));
}

function renderByteSources(payload) {
  const container = payload.container || {};
  const tracks = Array.isArray(container.tracks) ? container.tracks : [];
  const sources = [];
  if (payload.input && payload.input.bytes && payload.input.bytes.hex) {
    sources.push({
      label: `${t("label.inputBytes")} @ 0 · ${formatBytes(payload.input.bytes.length || 0)}`,
      title: `${t("label.inputBinary")} ${formatBytes(payload.input.bytes.length || 0)}${payload.input.bytes.truncated ? ` (${t("field.truncated")})` : ""}`,
      bytes: payload.input.bytes,
      mapRange: byteRange(payload.input.bytes),
    });
  }
  flattenBoxes(container.structure || []).forEach((node) => {
    if (node.bytes && node.bytes.hex) {
      sources.push({
        label: `${t("label.box")} ${node.type} @ ${node.offset}`,
        title: `${node.type} ${t("label.box")}`,
        bytes: node.bytes,
        mapRange: boxRange(node),
      });
    }
  });
  tracks.forEach((track) => {
    const codec = track.codec || {};
    [
      [t("label.codecHeader"), codec.raw_header_hex],
      ["VPS", codec.vps_hex],
      ["SPS", codec.sps_hex],
      ["PPS", codec.pps_hex],
      ["ASC", codec.asc_hex],
    ].forEach(([label, hex]) => {
      if (hex) {
        sources.push({
          label: `${t("label.track")} #${valueOrDash(track.id)} ${label}`,
          title: `${codecLabel(codec)} · ${label}`,
          bytes: { offset: 0, hex },
        });
      }
    });
  });

  if (!sources.length) {
    byteSourceList.replaceChildren(emptyBlock(t("empty.noMappedByteRanges")));
    byteDumpTitle.textContent = t("label.inputBytes");
    byteDump.replaceChildren(emptyBlock(t("empty.noBytes")));
    return;
  }

  byteSourceList.replaceChildren(...sources.map((source, index) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "byte-source";
    button.textContent = source.label;
    button.addEventListener("click", () => {
      document.querySelectorAll(".byte-source.active").forEach((item) => item.classList.remove("active"));
      button.classList.add("active");
      renderSelectedBytes(source.title, source.bytes, [[t("field.range"), source.label]], "", source.mapRange || null);
    });
    if (index === 0) {
      button.classList.add("active");
    }
    return button;
  }));

  const inputSource = sources.find((source) => source.bytes === (payload.input && payload.input.bytes)) || sources[0];
  renderMainByteDump(inputSource.title, inputSource.bytes);
}

function renderParsedOutline(payload) {
  const container = payload.container || {};
  const tracks = Array.isArray(container.tracks) ? container.tracks : [];
  const items = [];

  if (payload.input && payload.input.bytes) {
    items.push(outlineLeaf(
      t("label.input"),
      `${t("label.size")} ${formatBytes(payload.input.size || 0)}`,
      payload.input.bytes,
      byteRange(payload.input.bytes),
    ));
  }
  if (container.structure && container.structure.length) {
    const details = document.createElement("details");
    details.open = true;
    const summary = document.createElement("summary");
    summary.textContent = t("outline.containerTree");
    details.append(summary);
    const body = document.createElement("div");
    body.className = "outline-children";
    container.structure.forEach((node) => body.append(outlineBox(node)));
    details.append(body);
    items.push(details);
  }
  if (tracks.length) {
    const details = document.createElement("details");
    details.open = true;
    const summary = document.createElement("summary");
    summary.textContent = t("outline.tracksCodecHeaders");
    details.append(summary);
    const body = document.createElement("div");
    body.className = "outline-children";
    tracks.forEach((track) => body.append(outlineTrack(track)));
    details.append(body);
    items.push(details);
  }

  parsedOutline.replaceChildren(...(items.length ? items : [emptyBlock(t("empty.noMappedStructures"))]));
}

function outlineBox(node) {
  const details = document.createElement("details");
  details.open = false;
  const summary = document.createElement("summary");
  summary.textContent = `${node.type || "box"}  @${valueOrDash(node.offset)}  ${t("label.size")} ${valueOrDash(node.size)}`;
  summary.addEventListener("click", () => {
    markOutlineSelection(summary);
    renderSelectedBox(node);
  });
  details.append(summary);
  const children = node.children || [];
  if (children.length) {
    const body = document.createElement("div");
    body.className = "outline-children";
    children.forEach((child) => body.append(outlineBox(child)));
    details.append(body);
  }
  return details;
}

function outlineTrack(track) {
  const details = document.createElement("details");
  details.open = false;
  const summary = document.createElement("summary");
  summary.textContent = `${t("label.track")} #${valueOrDash(track.id)} ${valueOrDash(track.type)} · ${codecLabel(track.codec)}`;
  details.append(summary);
  const body = document.createElement("div");
  body.className = "outline-children";
  const codec = track.codec || {};
  [
    [t("label.codecHeader"), codec.raw_header_hex],
    ["VPS", codec.vps_hex],
    ["SPS", codec.sps_hex],
    ["PPS", codec.pps_hex],
    [t("label.audioSpecificConfig"), codec.asc_hex],
  ].filter(([, hex]) => hex).forEach(([label, hex]) => {
    body.append(outlineLeaf(label, codecLabel(codec), { offset: 0, hex }));
  });
  if (!body.children.length) {
    body.append(emptyBlock(t("empty.noCodecByteRanges")));
  }
  details.append(body);
  return details;
}

function outlineLeaf(label, meta, bytes, mapRange = null) {
  const button = document.createElement("button");
  button.className = "outline-leaf";
  button.type = "button";
  button.innerHTML = `<strong>${escapeHtml(label)}</strong><span>${escapeHtml(meta || "")}</span>`;
  button.addEventListener("click", () => {
    markOutlineSelection(button);
    renderSelectedBytes(label, bytes || {}, [[t("field.info"), meta || "-"]], "", mapRange);
  });
  return button;
}

function renderMainByteDump(title, bytes) {
  byteDumpTitle.textContent = title || t("label.inputBytes");
  renderVirtualHexDump(bytes || {});
}

function markOutlineSelection(target) {
  document.querySelectorAll(".outline-leaf.active, .parsed-outline summary.outline-selected").forEach((item) => {
    item.classList.remove("active", "outline-selected");
  });
  if (target.matches("summary")) {
    target.classList.add("outline-selected");
  } else {
    target.classList.add("active");
  }
}

function boxRange(node) {
  if (!node) {
    return null;
  }
  return normalizedRange(node.offset, node.size);
}

function byteRange(bytes) {
  if (!bytes) {
    return null;
  }
  return normalizedRange(bytes.offset, bytes.length || hexByteLength(bytes.hex || ""));
}

function normalizedRange(offset, length) {
  const start = Number(offset);
  const size = Number(length);
  if (!Number.isFinite(start) || !Number.isFinite(size) || size <= 0) {
    return null;
  }
  return {
    offset: Math.max(0, Math.floor(start)),
    length: Math.max(1, Math.floor(size)),
  };
}

function selectMappedRange(range) {
  activeByteRange = range;
  highlightMappedRange(true);
}

function highlightMappedRange(shouldScroll) {
  if (!mainHexView) {
    return;
  }
  if (shouldScroll && activeByteRange) {
    scrollMainHexToRange(activeByteRange);
  }
  mainHexView.lastStart = -1;
  mainHexView.lastEnd = -1;
  scheduleVirtualHexRender();
}

function renderVirtualHexDump(bytes) {
  const parsedBytes = hexToBytes(bytes.hex || "");
  if (!parsedBytes.length) {
    mainHexView = null;
    byteDump.replaceChildren(emptyBlock(t("empty.noBytes")));
    return;
  }

  const viewport = document.createElement("div");
  viewport.className = "hex-virtual";
  viewport.style.height = `${Math.ceil(parsedBytes.length / HEX_BYTES_PER_LINE) * HEX_ROW_HEIGHT}px`;

  const lines = document.createElement("div");
  lines.className = "hex-virtual-lines";
  viewport.append(lines);
  byteDump.replaceChildren(viewport);
  byteDump.scrollTop = 0;

  mainHexView = {
    bytes: parsedBytes,
    baseOffset: Number(bytes.offset || 0),
    asciiHint: bytes.ascii || "",
    totalRows: Math.ceil(parsedBytes.length / HEX_BYTES_PER_LINE),
    lines,
    lastStart: -1,
    lastEnd: -1,
  };

  byteDump.onscroll = () => scheduleVirtualHexRender();
  renderVirtualHexWindow();
}

function scheduleVirtualHexRender() {
  if (!mainHexView) {
    return;
  }
  cancelAnimationFrame(mainHexRenderFrame);
  mainHexRenderFrame = requestAnimationFrame(renderVirtualHexWindow);
}

function renderVirtualHexWindow() {
  if (!mainHexView) {
    return;
  }

  const visibleStart = Math.max(0, Math.floor(byteDump.scrollTop / HEX_ROW_HEIGHT) - HEX_OVERSCAN_LINES);
  const visibleCount = Math.ceil(byteDump.clientHeight / HEX_ROW_HEIGHT) + HEX_OVERSCAN_LINES * 2;
  const visibleEnd = Math.min(mainHexView.totalRows, visibleStart + visibleCount);
  if (visibleStart === mainHexView.lastStart && visibleEnd === mainHexView.lastEnd) {
    return;
  }

  mainHexView.lastStart = visibleStart;
  mainHexView.lastEnd = visibleEnd;
  mainHexView.lines.style.transform = `translateY(${visibleStart * HEX_ROW_HEIGHT}px)`;

  const fragment = document.createDocumentFragment();
  for (let row = visibleStart; row < visibleEnd; row += 1) {
    const byteOffset = row * HEX_BYTES_PER_LINE;
    const slice = mainHexView.bytes.slice(byteOffset, byteOffset + HEX_BYTES_PER_LINE);
    fragment.append(hexLineElement(slice, mainHexView.baseOffset + byteOffset, mainHexView.asciiHint, byteOffset));
  }
  mainHexView.lines.replaceChildren(fragment);
}

function scrollMainHexToRange(range) {
  const rangeOffset = Number(range.offset);
  if (!Number.isFinite(rangeOffset)) {
    return;
  }

  const relativeOffset = rangeOffset - mainHexView.baseOffset;
  if (relativeOffset < 0 || relativeOffset >= mainHexView.bytes.length) {
    return;
  }

  const row = Math.floor(relativeOffset / HEX_BYTES_PER_LINE);
  byteDump.scrollTop = Math.max(0, (row * HEX_ROW_HEIGHT) - (byteDump.clientHeight / 2));
}

function hexLineElement(slice, lineStart, asciiHint, asciiOffset) {
  const line = document.createElement("div");
  line.className = "hex-line";
  line.dataset.lineStart = String(lineStart);
  line.dataset.lineEnd = String(lineStart + slice.length);

  const rangeStart = activeByteRange ? activeByteRange.offset : null;
  const rangeEnd = activeByteRange ? activeByteRange.offset + activeByteRange.length : null;
  const lineIsMapped = activeByteRange && lineStart < rangeEnd && (lineStart + slice.length) > rangeStart;
  if (lineIsMapped) {
    line.classList.add("mapped");
  }

  const address = document.createElement("span");
  address.className = "hex-address";
  address.textContent = toHexOffset(lineStart);

  const hexBytes = document.createElement("span");
  hexBytes.className = "hex-bytes";
  slice.forEach((byte, index) => {
    if (index > 0) {
      hexBytes.append(document.createTextNode(" "));
    }
    const byteCell = document.createElement("span");
    byteCell.className = "hex-byte";
    byteCell.dataset.offset = String(lineStart + index);
    byteCell.textContent = byte.toString(16).padStart(2, "0");
    if (lineIsMapped && lineStart + index >= rangeStart && lineStart + index < rangeEnd) {
      byteCell.classList.add("mapped");
    }
    hexBytes.append(byteCell);
  });

  const ascii = document.createElement("span");
  ascii.className = "hex-ascii";
  const asciiText = asciiHint
    ? asciiHint.slice(asciiOffset, asciiOffset + slice.length).padEnd(slice.length, " ")
    : Array.from(slice, (byte) => byte >= 0x20 && byte <= 0x7e ? String.fromCharCode(byte) : ".").join("");
  slice.forEach((byte, index) => {
    const asciiCell = document.createElement("span");
    asciiCell.className = "ascii-byte";
    asciiCell.dataset.offset = String(lineStart + index);
    asciiCell.textContent = asciiText[index] || ".";
    if (lineIsMapped && lineStart + index >= rangeStart && lineStart + index < rangeEnd) {
      asciiCell.classList.add("mapped");
    }
    ascii.append(asciiCell);
  });

  line.append(address, hexBytes, ascii);
  return line;
}

function hexByteLength(hex) {
  if (!hex) {
    return 0;
  }
  return Math.floor(hex.replace(/\s+/g, "").length / 2);
}

function flattenBoxes(nodes) {
  const out = [];
  const visit = (node) => {
    if (!node) {
      return;
    }
    out.push(node);
    (node.children || []).forEach(visit);
  };
  nodes.forEach(visit);
  return out;
}

function renderBoxGuide(nodes) {
  const types = uniqueBoxTypes(nodes).slice(0, 18);
  if (!types.length) {
    boxGuide.replaceChildren(emptyBlock(t("empty.noBoxInfo")));
    return;
  }
  boxGuide.replaceChildren(...types.map((type) => {
    const item = document.createElement("div");
    item.className = "box-guide-item";
    const code = document.createElement("code");
    const p = document.createElement("p");
    code.textContent = type;
    p.textContent = boxDescription(type);
    item.append(code, p);
    return item;
  }));
}

function uniqueBoxTypes(nodes) {
  const seen = new Set();
  const out = [];
  const visit = (node) => {
    if (!node || !node.type) {
      return;
    }
    if (!seen.has(node.type)) {
      seen.add(node.type);
      out.push(node.type);
    }
    (node.children || []).forEach(visit);
  };
  nodes.forEach(visit);
  return out;
}

function boxDescription(type) {
  const localized = boxDescriptions[locale] || boxDescriptions["zh-CN"];
  return localized[type] || t("box.default");
}

function hexDumpFromHex(hex, baseOffset, asciiHint = "") {
  const bytes = hexToBytes(hex);
  const wrap = document.createElement("div");
  wrap.className = "hex-lines";
  if (!bytes.length) {
    wrap.append(emptyBlock(t("empty.noBytes")));
    return wrap;
  }

  for (let offset = 0; offset < bytes.length; offset += 16) {
    const slice = bytes.slice(offset, offset + 16);
    const lineStart = Number(baseOffset || 0) + offset;
    const line = document.createElement("div");
    line.className = "hex-line";
    line.dataset.lineStart = String(lineStart);
    line.dataset.lineEnd = String(lineStart + slice.length);

    const address = document.createElement("span");
    address.className = "hex-address";
    address.textContent = toHexOffset(lineStart);

    const hexBytes = document.createElement("span");
    hexBytes.className = "hex-bytes";
    slice.forEach((byte, index) => {
      if (index > 0) {
        hexBytes.append(document.createTextNode(" "));
      }
      const byteCell = document.createElement("span");
      byteCell.className = "hex-byte";
      byteCell.dataset.offset = String(lineStart + index);
      byteCell.textContent = byte.toString(16).padStart(2, "0");
      hexBytes.append(byteCell);
    });

    const ascii = document.createElement("span");
    ascii.className = "hex-ascii";
    const asciiText = asciiHint
      ? asciiHint.slice(offset, offset + 16).padEnd(slice.length, " ")
      : slice.map((byte) => byte >= 0x20 && byte <= 0x7e ? String.fromCharCode(byte) : ".").join("");
    slice.forEach((byte, index) => {
      const asciiCell = document.createElement("span");
      asciiCell.className = "ascii-byte";
      asciiCell.dataset.offset = String(lineStart + index);
      asciiCell.textContent = asciiText[index] || ".";
      ascii.append(asciiCell);
    });

    line.append(address, hexBytes, ascii);
    wrap.append(line);
  }
  return wrap;
}

function hexToBytes(hex) {
  const text = String(hex || "");
  let nibbleCount = 0;
  for (let index = 0; index < text.length; index += 1) {
    if (hexNibble(text.charCodeAt(index)) >= 0) {
      nibbleCount += 1;
    }
  }

  const out = new Uint8Array(Math.floor(nibbleCount / 2));
  let highNibble = -1;
  let outIndex = 0;
  for (let index = 0; index < text.length && outIndex < out.length; index += 1) {
    const nibble = hexNibble(text.charCodeAt(index));
    if (nibble < 0) {
      continue;
    }
    if (highNibble < 0) {
      highNibble = nibble;
      continue;
    }
    out[outIndex] = (highNibble << 4) | nibble;
    outIndex += 1;
    highNibble = -1;
  }
  return out;
}

function hexNibble(code) {
  if (code >= 48 && code <= 57) {
    return code - 48;
  }
  if (code >= 65 && code <= 70) {
    return code - 55;
  }
  if (code >= 97 && code <= 102) {
    return code - 87;
  }
  return -1;
}

function toHexOffset(value) {
  return `0x${Math.max(0, value).toString(16).padStart(8, "0")}`;
}

function definitionRow(key, value) {
  const row = document.createElement("div");
  const dt = document.createElement("dt");
  const dd = document.createElement("dd");
  dt.textContent = key;
  dd.textContent = valueOrDash(value);
  row.append(dt, dd);
  return row;
}

function field(key, value) {
  const item = document.createElement("div");
  item.className = "field";
  const label = document.createElement("span");
  const strong = document.createElement("strong");
  label.textContent = key;
  strong.textContent = valueOrDash(value);
  item.append(label, strong);
  return item;
}

function explain(title, text) {
  const item = document.createElement("div");
  item.className = "explain";
  const strong = document.createElement("strong");
  const span = document.createElement("span");
  strong.textContent = title;
  span.textContent = text;
  item.append(strong, span);
  return item;
}

function emptyItem(text) {
  const li = document.createElement("li");
  li.className = "empty";
  li.textContent = text;
  return li;
}

function emptyBlock(text) {
  const div = document.createElement("div");
  div.className = "empty";
  div.textContent = text;
  return div;
}

function codecLabel(codec) {
  if (!codec) {
    return "-";
  }
  return [codec.description, codec.fourcc ? `(${codec.fourcc})` : ""]
    .filter(Boolean)
    .join(" ");
}

function trackShape(track) {
  if (track.width || track.height) {
    return `${valueOrDash(track.width)} x ${valueOrDash(track.height)}`;
  }
  if (track.sample_rate) {
    return `${track.sample_rate} Hz`;
  }
  return "-";
}

function formatDuration(time) {
  const value = seconds(time);
  if (!value) {
    return "-";
  }
  if (value >= 3600) {
    const hours = Math.floor(value / 3600);
    const minutes = Math.floor((value % 3600) / 60);
    const secs = Math.floor(value % 60);
    return `${hours}:${String(minutes).padStart(2, "0")}:${String(secs).padStart(2, "0")}`;
  }
  if (value >= 60) {
    const minutes = Math.floor(value / 60);
    const secs = Math.floor(value % 60);
    return `${minutes}:${String(secs).padStart(2, "0")}`;
  }
  return `${value.toFixed(3)}s`;
}

function seconds(time) {
  if (!time) {
    return 0;
  }
  if (typeof time.seconds === "number") {
    return time.seconds;
  }
  if (time.timescale) {
    return Number(time.value || 0) / Number(time.timescale);
  }
  return 0;
}

function formatBytes(value) {
  if (!value) {
    return "0 B";
  }
  const units = ["B", "KB", "MB", "GB", "TB"];
  let size = Number(value);
  let unit = 0;
  while (size >= 1024 && unit < units.length - 1) {
    size /= 1024;
    unit += 1;
  }
  return `${size.toFixed(unit === 0 ? 0 : 2)} ${units[unit]}`;
}

function valueOrDash(value) {
  if (value === undefined || value === null || value === "") {
    return "-";
  }
  return String(value);
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function renderStatus() {
  const text = currentStatus.raw || t(currentStatus.key, currentStatus.params);
  statusEl.textContent = text;
  statusEl.classList.toggle("error", currentStatus.isError);
}

function setStatus(text, isError, params = {}, raw = false) {
  currentStatus = raw
    ? { key: "status.ready", params: {}, raw: text, isError }
    : { key: text, params, raw: null, isError };
  renderStatus();
  statusEl.classList.toggle("error", isError);
}
