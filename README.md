# MediaAnalyzer

MediaAnalyzer 是一个 Web 媒体结构分析工具。目标是通过上传文件或输入 URL，自动识别容器格式，并逐步解析容器结构、轨道、时间线、采样表、metadata 和 codec bitstream headers。

当前仓库采用多技术栈分层：

- C/C++：高性能解析核心，负责二进制读取、格式识别、容器和 codec header 解析。
- Go：Web/API 服务、远程 URL 获取、Range 代理、大文件协调。
- Python：测试样本生成、golden tests、批量校验和工具脚本。
- Web 静态页面：第一阶段用原生 HTML/CSS/JS，后续可升级为 React/Vue。

## 当前状态

已建立 Milestone 0 的最小骨架：

- C++ CLI：读取文件前若干字节，输出格式识别 JSON。
- Go server：提供 `/api/analyze` 文件上传和 URL 分析入口，服务静态前端。
- Python golden test：生成小样本并验证 C++ detector。
- Web UI：上传文件或输入 URL，展示 JSON 分析结果。

## 构建 C++ 核心

```bash
cmake -S . -B build
cmake --build build
```

运行：

```bash
./build/media-analyzer-core path/to/file.mp4
```

## 运行 Python 测试

```bash
python3 tools/golden_test.py
```

## 运行 Go Web 服务

需要先安装 Go。

```bash
cd server
go run ./cmd/media-analyzer-server
```

默认监听：

```text
http://localhost:8080
```

## 当前可用的 Python 开发服务器

如果本机还没有安装 Go，可以先用 Python 开发服务器。它会服务同一套前端，并调用 C++ CLI 完成格式识别。

```bash
python3 tools/dev_server.py 8080
```

访问：

```text
http://127.0.0.1:8080
```

## 项目规划

总规划见 [PROJECT_PLAN.md](PROJECT_PLAN.md)。
