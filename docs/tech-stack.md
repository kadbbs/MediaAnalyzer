# 技术栈分工

## C/C++ 核心

职责：

- 二进制读取和基础数据结构。
- 容器格式识别。
- 容器 parser。
- codec bitstream header parser。
- 可复用 CLI 和后续可嵌入库。

原因：

- 解析大文件和复杂 bitstream 时性能稳定。
- 更适合处理 byte-level、bit-level 数据结构。
- 后续可编译成 native library 或 WebAssembly。

## Go 服务端

职责：

- Web/API 服务。
- 文件上传入口。
- URL 分析入口。
- HTTP Range proxy。
- 调用 C++ CLI 或 native library。
- 报告存储和任务调度。

原因：

- 标准库 HTTP 能力强。
- 部署简单。
- 并发处理远程 URL 和大文件任务更直接。

## Python 工具链

职责：

- 测试样本生成。
- golden JSON 比对。
- 批量回归。
- 调用 ffprobe/mediainfo 做对照验证。
- 开发阶段快速脚本。

原因：

- 适合测试和自动化胶水逻辑。
- 方便生成特殊二进制样本。

## Web 前端

职责：

- 上传文件和 URL 输入。
- 展示分析摘要、结构树、轨道、metadata、diagnostics。
- 展示 JSON 导出。

第一阶段使用静态 HTML/CSS/JS 降低工程复杂度。等核心能力稳定后，再升级到 TypeScript + React/Vue。

