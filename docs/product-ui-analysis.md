# MediaAnalyzer 产品和界面分析

## 目标定位

MediaAnalyzer 应该同时服务两类用户：

- 新手：快速理解“容器、轨道、sample、codec header、metadata、时间线”这些概念，并能看到字段来自哪些字节。
- 老手：快速定位结构异常、时间戳问题、codec extradata 问题、封装兼容性问题，并能下钻到 offset、size、hex、sample table。

因此界面不能只是 JSON 报告页，也不能只做教学页面。更合适的形态是专业分析工作台。

## 当前已经具备

- 文件上传和 URL 分析入口。
- ISO-BMFF/MP4 box tree。
- MP4 track 和 sample entry 摘要。
- H.264 `avcC`、HEVC `hvcC`、AAC `esds/AudioSpecificConfig` 初步解析。
- box 和 codec header 对应字节的 hex/ascii 展示。
- 新手解释、术语速查、专家字段和原始 JSON。

## 仍然缺少的专业能力

1. Sample table 深度展开
   - `stts`、`ctts`、`stsc`、`stsz`、`stco/co64`、`stss`。
   - 输出每个 sample 的 DTS、PTS、duration、size、offset、keyframe。
   - 支持虚拟表格和按 index 跳转。

2. 时间线分析
   - 每条轨道的时间轴。
   - PTS/DTS 差异。
   - 关键帧分布。
   - track duration 与 movie duration 对齐检查。

3. 诊断规则
   - 缺失 `moov`、`trak`、`mdia`、`stbl`。
   - sample table 计数不一致。
   - `stco/stsz/stts` 等表无法闭合。
   - codec extradata 缺失或和 sample entry 不匹配。
   - HEVC/H.264 profile/level 与分辨率、bit depth 不一致。

4. 字段级 byte mapping
   - 目前是 box/header 级别 hex。
   - 后续应做到字段级：例如 `timescale` 对应哪 4 个字节、`profile_idc` 对应 SPS 哪个字节。

5. 多格式扩展
   - Matroska/WebM EBML tree。
   - MPEG-TS PAT/PMT/PES/PCR/PTS。
   - Ogg/Opus pages。
   - ADTS、Annex B elementary stream。

## 专业软件式界面原则

1. 固定三栏
   - 左侧 Navigator：来源、视图、轨道、诊断。
   - 中间 Workspace：当前主表格、树、时间线、bytes。
   - 右侧 Inspector：选中对象的字段、解释、hex。

2. 数据优先
   - 表格、树、Inspector 是第一视觉层级。
   - 解释文字应辅助理解，不喧宾夺主。

3. 所有对象可下钻
   - track 可下钻到 sample entry 和 codec header。
   - box 可下钻到 children、字段、字节。
   - sample 可下钻到 offset 和 payload bytes。

4. 新手和老手共存
   - 新手看 Explanation / Glossary。
   - 老手看 offset / size / timescale / hex / diagnostics。

5. 结果可复现
   - JSON 保留。
   - 未来应支持报告保存、分享和 diff。

## 推荐下一阶段实现顺序

1. MP4 sample table parser。
2. Sample Table 视图和虚拟表格。
3. 字段级 byte mapping。
4. Diagnostics 面板。
5. 时间线视图。
6. Matroska/WebM EBML tree。
