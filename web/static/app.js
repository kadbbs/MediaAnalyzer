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

const boxDescriptions = {
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
};

const glossary = [
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
];

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

activateView("bytes");

uploadForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const file = fileInput.files[0];
  if (!file) {
    setStatus("请选择文件", true);
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
    setStatus("请输入 URL", true);
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
  setStatus("Analyzing", false);
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
    setStatus("Done", false);
  } catch (error) {
    setStatus(error.message, true);
  }
}

function render(payload) {
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
    ["Name", input.name || "-"],
    ["Size", typeof input.size === "number" ? formatBytes(input.size) : "-"],
    ["Bytes Loaded", typeof byteInfo.length === "number" ? formatBytes(byteInfo.length) : "-"],
    ["Truncated", byteInfo.truncated ? "yes" : "no"],
  ];
  inputDetails.replaceChildren(...fields.map(([key, value]) => definitionRow(key, value)));
}

function renderEvidence(items) {
  if (!items.length) {
    evidenceList.replaceChildren(emptyItem("暂无识别依据"));
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
    diagnostics.push(["warn", `low detection confidence: ${detection.confidence.toFixed(2)}`]);
  }
  if (detection.format === "iso-bmff" && !container.format) {
    diagnostics.push(["error", "ISO-BMFF detected but container parser did not return a structure"]);
  }
  if (container.format && !tracks.length) {
    diagnostics.push(["warn", "container parsed but no tracks were found"]);
  }
  tracks.forEach((track) => {
    if (!track.codec || !track.codec.description) {
      diagnostics.push(["warn", `track #${valueOrDash(track.id)} has no parsed codec description`]);
    }
    if (track.type === "video" && !(track.width || (track.codec && track.codec.width))) {
      diagnostics.push(["info", `video track #${valueOrDash(track.id)} has no parsed width/height yet`]);
    }
  });

  if (!diagnostics.length) {
    diagnostics.push(["ok", "no basic diagnostics triggered"]);
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
    ["Format", detection.format || "-"],
    ["Family", detection.family || "-"],
    ["Major Brand", container.major_brand || "-"],
    ["Compatible Brands", (container.compatible_brands || []).join(", ") || "-"],
    ["Movie Timescale", container.duration && container.duration.timescale],
    ["Movie Duration Value", container.duration && container.duration.value],
    ["Top Boxes", topBoxes],
    ["Video Tracks", videoTracks],
    ["Audio Tracks", audioTracks],
    ["Warnings", (container.warnings || []).length],
  ];
  expertFacts.replaceChildren(...facts.map(([key, value]) => definitionRow(key, valueOrDash(value))));

  glossaryList.replaceChildren(...glossary.map(([term, text]) => {
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
    items.push(`<strong>${escapeHtml(detection.format)}</strong> 是识别出的封装/格式入口；封装负责组织轨道、时间线、索引和 metadata。`);
  }
  if (container.format === "ISO-BMFF") {
    items.push("这是 ISO-BMFF 家族文件。MP4、MOV、CMAF/fMP4 都属于这个体系，核心结构由一系列 box 组成。");
  }
  if (container.major_brand) {
    items.push(`<strong>${escapeHtml(container.major_brand)}</strong> 是 major brand，用来提示播放器按哪类 MP4 兼容规则理解文件。`);
  }
  if (tracks.length) {
    items.push(`当前解析到 <strong>${tracks.length}</strong> 条轨道。每条轨道都有自己的时间基、编码格式和 sample 表。`);
  }
  tracks.slice(0, 3).forEach((track) => {
    const codec = codecLabel(track.codec);
    const shape = trackShape(track);
    items.push(`${escapeHtml(track.type || "unknown")} 轨道 #${valueOrDash(track.id)} 使用 <strong>${escapeHtml(codec)}</strong>，${shape !== "-" ? `媒体形态是 ${escapeHtml(shape)}，` : ""}轨道时长 ${escapeHtml(formatDuration(track.duration))}。`);
  });
  const h264 = tracks.find((track) => track.codec && /H\.264|AVC/.test(track.codec.description || ""));
  if (h264 && h264.codec) {
    items.push(`H.264 初始化信息来自 <strong>avcC</strong>。其中 SPS/PPS 会告诉解码器 profile、level、分辨率和参数集数量。`);
  }
  if (!items.length) {
    items.push("当前格式还没有深入解析结果；可以先查看识别依据和原始 JSON。");
  }
  return items;
}

function renderTrackSummary(tracks) {
  if (!tracks.length) {
    const row = document.createElement("tr");
    const cell = document.createElement("td");
    cell.colSpan = 6;
    cell.className = "empty";
    cell.textContent = "暂无轨道信息";
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
    navTrackList.replaceChildren(emptyBlock("No tracks"));
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
    trackList.replaceChildren(emptyBlock("暂无轨道信息"));
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
  strong.textContent = `${valueOrDash(track.type)} track #${valueOrDash(track.id)}`;
  const codec = document.createElement("span");
  codec.textContent = codecLabel(track.codec);
  title.append(strong, codec);

  const tags = document.createElement("div");
  tags.className = "tag-row";
  [
    formatDuration(track.duration),
    trackShape(track),
    `${valueOrDash(track.sample_count)} samples`,
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
    ["Codec FourCC", codecInfo.fourcc],
    ["Codec", codecInfo.description],
    ["Profile", codecInfo.profile],
    ["Level", codecInfo.level],
    ["NAL Length", codecInfo.length_size],
    ["VPS", codecInfo.vps_count],
    ["SPS", codecInfo.sps_count],
    ["PPS", codecInfo.pps_count],
    ["Bit Depth Luma", codecInfo.bit_depth_luma],
    ["Bit Depth Chroma", codecInfo.bit_depth_chroma],
    ["Chroma Format", codecInfo.chroma_format],
    ["Audio Object", codecInfo.audio_object_type],
    ["ASC Rate", codecInfo.asc_sample_rate],
    ["Channel Config", codecInfo.channel_config],
    ["Time Scale", track.duration && track.duration.timescale],
    ["Samples", track.sample_count],
    ["Sample Entries", track.sample_description_count],
    ["Width", track.width || codecInfo.width],
    ["Height", track.height || codecInfo.height],
    ["Channels", track.channel_count],
    ["Sample Rate", track.sample_rate],
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
    ["Codec Header", codecInfo.raw_header_hex],
    ["VPS", codecInfo.vps_hex],
    ["SPS", codecInfo.sps_hex],
    ["PPS", codecInfo.pps_hex],
    ["AudioSpecificConfig", codecInfo.asc_hex],
  ].filter(([, hex]) => hex);
  if (!sections.length) {
    return null;
  }

  const panel = document.createElement("div");
  panel.className = "byte-panel";
  const title = document.createElement("h2");
  title.textContent = "Codec Bytes";
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
    rows.push(["轨道类型", "视频轨道包含压缩图像 sample。容器提供时间线和索引，codec header 提供解码初始化参数。"]);
  } else if (track.type === "audio") {
    rows.push(["轨道类型", "音频轨道包含压缩或未压缩音频 sample，重点关注采样率、声道数和 codec extradata。"]);
  } else {
    rows.push(["轨道类型", "非音视频轨道可能承载字幕、章节、metadata 或其他同步数据。"]);
  }

  if (codec.description === "H.264/AVC") {
    rows.push(["avcC", "MP4 中 H.264 常把 SPS/PPS 放在 avcC box 中。sample 内的 NALU 通常用 length 前缀分隔，而不是 Annex B start code。"]);
    rows.push(["SPS/PPS", `当前解析到 ${valueOrDash(codec.sps_count)} 个 SPS、${valueOrDash(codec.pps_count)} 个 PPS。SPS 决定 profile、level、分辨率等关键解码参数。`]);
    rows.push(["Profile/Level", `${valueOrDash(codec.profile)} / ${valueOrDash(codec.level)} 描述编码工具集和复杂度上限，兼容性排查时很重要。`]);
  } else if (codec.description === "H.265/HEVC") {
    rows.push(["hvcC", "MP4 中 HEVC/H.265 常通过 hvcC 携带 VPS/SPS/PPS、profile、tier、level、bit depth 和 NAL length size。"]);
    rows.push(["VPS/SPS/PPS", `当前解析到 ${valueOrDash(codec.vps_count)} 个 VPS、${valueOrDash(codec.sps_count)} 个 SPS、${valueOrDash(codec.pps_count)} 个 PPS。排查 HEVC 兼容性时这些参数集很关键。`]);
    rows.push(["Profile/Tier/Level", `${valueOrDash(codec.profile)} / ${valueOrDash(codec.level)} 描述编码工具、tier 和复杂度等级。`]);
  } else if (codec.description === "AAC") {
    rows.push(["esds", "MP4 中 AAC 常在 esds 里携带 AudioSpecificConfig，而不是把初始化参数放到每个音频 sample 里。"]);
    rows.push(["AudioSpecificConfig", `当前解析到 objectType=${valueOrDash(codec.audio_object_type)}，sampleRate=${valueOrDash(codec.asc_sample_rate)}，channelConfig=${valueOrDash(codec.channel_config)}。`]);
  } else if (codec.description) {
    rows.push(["Codec", `${codec.description} 是这条轨道的编码格式。后续 parser 会继续补充该 codec 的 header 字段。`]);
  }

  rows.push(["时间基", `timescale=${valueOrDash(track.duration && track.duration.timescale)}，duration=${valueOrDash(track.duration && track.duration.value)}，换算后约 ${formatDuration(track.duration)}。`]);
  rows.push(["Sample", `当前 sample count=${valueOrDash(track.sample_count)}。sample 表越完整，越容易定位 seek、卡顿、时间戳和关键帧问题。`]);
  return rows;
}

function renderStructure(nodes) {
  if (!nodes.length) {
    structureTree.replaceChildren(emptyBlock("暂无容器结构"));
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
  offset.textContent = `offset ${valueOrDash(node.offset)}`;
  const size = document.createElement("span");
  size.className = "box-meta";
  size.textContent = `size ${valueOrDash(node.size)}`;
  const note = document.createElement("span");
  note.className = "box-note";
  note.textContent = `header ${valueOrDash(node.header_size)} · ${boxDescription(node.type)}`;
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
    const rows = [["Status", "No box selected"]];
    const explanation = "选择左侧容器节点后，这里会显示解析说明和对应字节。";
    selectedBoxMeta.replaceChildren(...rows.map(([key, value]) => definitionRow(key, value)));
    selectedBoxExplain.textContent = explanation;
    selectedHexDump.replaceChildren(emptyBlock("暂无字节"));
    renderInlineSelection(rows, `<span>${escapeHtml(explanation)}</span>`, null);
    return;
  }

  const byteInfo = node.bytes || {};
  const rows = [
    ["Type", node.type],
    ["Offset", node.offset],
    ["Size", node.size],
    ["Header", node.header_size],
    ["Preview Offset", byteInfo.offset],
    ["Preview Length", byteInfo.length],
    ["Truncated", byteInfo.truncated ? "yes" : "no"],
  ];
  const explanation = `<strong>${escapeHtml(node.type || "box")}</strong><span>${escapeHtml(boxDescription(node.type))}</span>`;
  selectedBoxMeta.replaceChildren(...rows.map(([key, value]) => definitionRow(key, value)));
  selectedBoxExplain.innerHTML = explanation;
  selectedHexDump.replaceChildren(hexDumpFromHex(byteInfo.hex || "", byteInfo.offset || 0, byteInfo.ascii || ""));
  renderInlineSelection(rows, explanation, byteInfo);
}

function renderSelectedBytes(title, bytes, rows = [], explanation = "") {
  const byteInfo = bytes || {};
  const rowData = [
    ["Selection", title || "bytes"],
    ["Offset", valueOrDash(byteInfo.offset)],
    ["Length", valueOrDash(byteInfo.length || hexByteLength(byteInfo.hex || ""))],
    ["Truncated", byteInfo.truncated ? "yes" : "no"],
    ...rows,
  ];
  const explanationHtml = `<strong>${escapeHtml(title || "bytes")}</strong><span>${escapeHtml(explanation || "这里显示右侧结构索引选中的局部字节；左侧始终保留整体输入二进制，方便按 offset 对照。")}</span>`;
  selectedBoxMeta.replaceChildren(...rowData.map(([key, value]) => definitionRow(key, value)));
  selectedBoxExplain.innerHTML = explanationHtml;
  selectedHexDump.replaceChildren(hexDumpFromHex(byteInfo.hex || "", byteInfo.offset || 0, byteInfo.ascii || ""));
  renderInlineSelection(rowData, explanationHtml, byteInfo);
}

function renderInlineSelection(rows, explanationHtml, bytes) {
  if (!bytesSelectionMeta || !bytesSelectionExplain || !bytesSelectedHexDump) {
    return;
  }
  bytesSelectionMeta.replaceChildren(...rows.map(([key, value]) => definitionRow(key, value)));
  bytesSelectionExplain.innerHTML = explanationHtml || "";
  if (!bytes || !bytes.hex) {
    bytesSelectedHexDump.replaceChildren(emptyBlock("暂无字节"));
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
      label: `input bytes @ 0 · ${formatBytes(payload.input.bytes.length || 0)}`,
      title: `Input binary ${formatBytes(payload.input.bytes.length || 0)}${payload.input.bytes.truncated ? " (truncated)" : ""}`,
      bytes: payload.input.bytes,
    });
  }
  flattenBoxes(container.structure || []).forEach((node) => {
    if (node.bytes && node.bytes.hex) {
      sources.push({
        label: `box ${node.type} @ ${node.offset}`,
        title: `${node.type} box`,
        bytes: node.bytes,
      });
    }
  });
  tracks.forEach((track) => {
    const codec = track.codec || {};
    [
      ["Codec Header", codec.raw_header_hex],
      ["VPS", codec.vps_hex],
      ["SPS", codec.sps_hex],
      ["PPS", codec.pps_hex],
      ["ASC", codec.asc_hex],
    ].forEach(([label, hex]) => {
      if (hex) {
        sources.push({
          label: `track #${valueOrDash(track.id)} ${label}`,
          title: `${codecLabel(codec)} · ${label}`,
          bytes: { offset: 0, hex },
        });
      }
    });
  });

  if (!sources.length) {
    byteSourceList.replaceChildren(emptyBlock("No byte ranges"));
    byteDumpTitle.textContent = "input bytes";
    byteDump.replaceChildren(emptyBlock("No bytes"));
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
      renderSelectedBytes(source.title, source.bytes, [["Range", source.label]]);
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
    items.push(outlineLeaf("Input", `size ${formatBytes(payload.input.size || 0)}`, payload.input.bytes));
  }
  if (container.structure && container.structure.length) {
    const details = document.createElement("details");
    details.open = true;
    const summary = document.createElement("summary");
    summary.textContent = "Container boxes";
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
    summary.textContent = "Tracks and codec headers";
    details.append(summary);
    const body = document.createElement("div");
    body.className = "outline-children";
    tracks.forEach((track) => body.append(outlineTrack(track)));
    details.append(body);
    items.push(details);
  }

  parsedOutline.replaceChildren(...(items.length ? items : [emptyBlock("No parsed ranges")]));
}

function outlineBox(node) {
  const details = document.createElement("details");
  details.open = false;
  const summary = document.createElement("summary");
  summary.textContent = `${node.type || "box"}  @${valueOrDash(node.offset)}  size ${valueOrDash(node.size)}`;
  summary.addEventListener("click", () => {
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
  summary.textContent = `track #${valueOrDash(track.id)} ${valueOrDash(track.type)} · ${codecLabel(track.codec)}`;
  details.append(summary);
  const body = document.createElement("div");
  body.className = "outline-children";
  const codec = track.codec || {};
  [
    ["Codec Header", codec.raw_header_hex],
    ["VPS", codec.vps_hex],
    ["SPS", codec.sps_hex],
    ["PPS", codec.pps_hex],
    ["AudioSpecificConfig", codec.asc_hex],
  ].filter(([, hex]) => hex).forEach(([label, hex]) => {
    body.append(outlineLeaf(label, codecLabel(codec), { offset: 0, hex }));
  });
  if (!body.children.length) {
    body.append(emptyBlock("No codec byte ranges"));
  }
  details.append(body);
  return details;
}

function outlineLeaf(label, meta, bytes) {
  const button = document.createElement("button");
  button.className = "outline-leaf";
  button.type = "button";
  button.innerHTML = `<strong>${escapeHtml(label)}</strong><span>${escapeHtml(meta || "")}</span>`;
  button.addEventListener("click", () => {
    document.querySelectorAll(".outline-leaf.active").forEach((item) => item.classList.remove("active"));
    button.classList.add("active");
    renderSelectedBytes(label, bytes || {}, [["Info", meta || "-"]]);
  });
  return button;
}

function renderMainByteDump(title, bytes) {
  byteDumpTitle.textContent = title || "input bytes";
  byteDump.replaceChildren(hexDumpFromHex(bytes.hex || "", bytes.offset || 0, bytes.ascii || ""));
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
    boxGuide.replaceChildren(emptyBlock("暂无 box 信息"));
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
  return boxDescriptions[type] || "暂未内置说明。专家可先根据 offset/size 和父子层级判断它在容器中的作用。";
}

function hexDumpFromHex(hex, baseOffset, asciiHint = "") {
  const bytes = hexToBytes(hex);
  const wrap = document.createElement("div");
  wrap.className = "hex-lines";
  if (!bytes.length) {
    wrap.append(emptyBlock("暂无字节"));
    return wrap;
  }

  for (let offset = 0; offset < bytes.length; offset += 16) {
    const slice = bytes.slice(offset, offset + 16);
    const line = document.createElement("div");
    line.className = "hex-line";

    const address = document.createElement("span");
    address.className = "hex-address";
    address.textContent = toHexOffset(Number(baseOffset || 0) + offset);

    const hexBytes = document.createElement("span");
    hexBytes.className = "hex-bytes";
    hexBytes.textContent = slice.map((byte) => byte.toString(16).padStart(2, "0")).join(" ");

    const ascii = document.createElement("span");
    ascii.className = "hex-ascii";
    ascii.textContent = asciiHint
      ? asciiHint.slice(offset, offset + 16).padEnd(16, " ")
      : slice.map((byte) => byte >= 0x20 && byte <= 0x7e ? String.fromCharCode(byte) : ".").join("");

    line.append(address, hexBytes, ascii);
    wrap.append(line);
  }
  return wrap;
}

function hexToBytes(hex) {
  return String(hex || "")
    .trim()
    .split(/\s+/)
    .filter(Boolean)
    .map((part) => Number.parseInt(part, 16))
    .filter((value) => Number.isFinite(value));
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

function setStatus(text, isError) {
  statusEl.textContent = text;
  statusEl.classList.toggle("error", isError);
}
