# 修复 Sequencer Range 不正确的问题

## 问题分析

**根本原因**: `UEMotionScene.cpp` 中设置 Playback Range 时，直接将 **显示帧号 (Display Frame)** 传给了 `MovieScene->SetPlaybackRange()`，但该方法期望的是 **Tick 帧号 (Tick Frame)**。

在 UE5 中，Tick Resolution 默认为 24000fps，而 Display Rate 通常为 30fps。因此：
- 一个 1 秒的动画，显示帧号 EndFrame = 31
- 但对应的 Tick 帧号应该是 31 × (24000/30) = 24800
- 当前代码直接传入 31，导致 Playback Range 远小于实际关键帧范围

**对比**: 代码库中 `UpdateCameraCutRange()` 和 `UpdateCameraTransformRange()` 已经正确使用了 `UEMotionCompat::DisplayFrameToTick()` 进行转换，但 `SetPlaybackRange()` 调用遗漏了这个转换。

## 修改方案

### 1. 修复 `CreateLevelSequenceAsset()` (第 101 行)

```cpp
// 修改前:
MovieScene->SetPlaybackRange(TRange<FFrameNumber>(0, DefaultFrames), true);

// 修改后:
FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, DefaultFrames);
MovieScene->SetPlaybackRange(TRange<FFrameNumber>(StartTick, EndTick), true);
```

同时修正 ViewRange 和 WorkingRange，使用帧号/fps 计算时间而非硬编码：

```cpp
// 修改前:
MovieScene->SetWorkingRange(0.0, 10.0);
MovieScene->SetViewRange(0.0, 10.0);

// 修改后:
float EndTime = (float)DefaultFrames / PlaybackFPS;
MovieScene->SetWorkingRange(0.0, EndTime);
MovieScene->SetViewRange(0.0, EndTime);
```

### 2. 修复 `Play()` 方法 (第 693-702 行)

```cpp
// 修改前:
int32 TotalFrames = FMath::RoundToInt(CurrentTime * PlaybackFPS) + 1;
MovieScene->SetPlaybackRange(TRange<FFrameNumber>(0, TotalFrames), true);
MovieScene->SetWorkingRange(0.0, CurrentTime + 1.0);
MovieScene->SetViewRange(0.0, CurrentTime + 1.0);

// 修改后:
int32 TotalFrames = FMath::RoundToInt(CurrentTime * PlaybackFPS) + 1;
FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, TotalFrames);
MovieScene->SetPlaybackRange(TRange<FFrameNumber>(StartTick, EndTick), true);
float EndTime = (float)TotalFrames / PlaybackFPS;
MovieScene->SetWorkingRange(0.0, EndTime);
MovieScene->SetViewRange(0.0, EndTime);
```

### 3. 修复 `Wait()` 方法 (第 709-718 行)

与 `Play()` 方法相同的修复：

```cpp
// 修改前:
int32 TotalFrames = FMath::RoundToInt(CurrentTime * PlaybackFPS) + 1;
MovieScene->SetPlaybackRange(TRange<FFrameNumber>(0, TotalFrames), true);
MovieScene->SetWorkingRange(0.0, CurrentTime + 1.0);
MovieScene->SetViewRange(0.0, CurrentTime + 1.0);

// 修改后:
int32 TotalFrames = FMath::RoundToInt(CurrentTime * PlaybackFPS) + 1;
FFrameNumber StartTick = UEMotionCompat::DisplayFrameToTick(MovieScene, 0);
FFrameNumber EndTick = UEMotionCompat::DisplayFrameToTick(MovieScene, TotalFrames);
MovieScene->SetPlaybackRange(TRange<FFrameNumber>(StartTick, EndTick), true);
float EndTime = (float)TotalFrames / PlaybackFPS;
MovieScene->SetWorkingRange(0.0, EndTime);
MovieScene->SetViewRange(0.0, EndTime);
```

## 涉及文件

| 文件 | 修改内容 |
|------|---------|
| `Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp` | 修复 3 处 `SetPlaybackRange` 调用，使用 `DisplayFrameToTick` 转换；修正 ViewRange/WorkingRange 时间计算 |

## 验证方式

修改后重新编译插件，运行 `test_level_creation.py`，检查 Sequencer 中的 Playback Range 和 View Range 是否与实际动画关键帧范围一致。
