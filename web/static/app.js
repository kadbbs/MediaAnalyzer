const statusEl = document.querySelector("#status");
const jsonOutput = document.querySelector("#jsonOutput");
const formatEl = document.querySelector("#format");
const familyEl = document.querySelector("#family");
const confidenceEl = document.querySelector("#confidence");
const uploadForm = document.querySelector("#uploadForm");
const urlForm = document.querySelector("#urlForm");
const fileInput = document.querySelector("#fileInput");
const urlInput = document.querySelector("#urlInput");

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
  formatEl.textContent = detection.format || "-";
  familyEl.textContent = detection.family || "-";
  confidenceEl.textContent = typeof detection.confidence === "number"
    ? detection.confidence.toFixed(2)
    : "-";
  jsonOutput.textContent = JSON.stringify(payload, null, 2);
}

function setStatus(text, isError) {
  statusEl.textContent = text;
  statusEl.classList.toggle("error", isError);
}

