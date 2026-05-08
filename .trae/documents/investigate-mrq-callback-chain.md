# 调查：MRQ 渲染完成后 Sequencer Object 失效问题

## 完整回调链路追踪

### 第 1 环：MRQ 渲染引擎完成

**文件**: [UEMotionRenderer.cpp L114-L128](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Rendering/UEMotionRenderer.cpp#L114-L128)

```cpp
void UUEMotionRenderer::OnRenderFinished(UMoviePipelineExecutorBase* InExecutor, bool bSuccess)
{
    bIsRendering = false;
    ActiveExecutor = nullptr;
    OnRenderFinishedDelegate.Broadcast(bSuccess);  // 广播出去
}
```

做了什么：
- 标记渲染结束 `bIsRendering = false`
- 清空执行器指针 `ActiveExecutor = nullptr`
- **广播委托** `OnRenderFinishedDelegate.Broadcast(bSuccess)`

---

### 第 2 环：Scene 接收并转发

**文件**: [UEMotionScene.cpp L890-L892](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp#L890-L892)

```cpp
void UUEMotionScene::OnRendererFinished(bool bSuccess)
{
    OnRenderFinished.Broadcast(this, bSuccess);  // 再次广播，带上 Scene 自身
}
```

注册位置在 [RenderFrames() L756](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp#L756)：
```cpp
Renderer->OnRenderFinishedDelegate.AddDynamic(this, &UUEMotionScene::OnRendererFinished);
```

做了什么：
- **纯转发**，把 `(bool success)` 变成 `(this, bool success)` 再广播给 Python 侧

---

### 第 3 环：Python Scene 包装层接收

**文件**: [scene.py L37-L43](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Scripts/uemotion/scene.py#L37-L43)

```python
def _on_render_finished(self, scene, success):
    for cb in self._render_callbacks:
        try:
            cb(success)
        except Exception as e:
            unreal.log_warning(f"UEMotion: render callback error: {e}")
    self._render_callbacks.clear()
```

做了什么：
- 遍历所有注册的回调函数（`self._render_callbacks`）
- 逐个调用 `cb(success)`
- **清空回调列表**

---

### 第 4 环：用户代码（run_full_pipeline_test.py）

**文件**: [run_full_pipeline_test.py L51-L78](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Scripts/run_full_pipeline_test.py#L51-L78)

```python
def on_render_done(success):
    # ... 检查输出图片 ...
    s.set_auto_cleanup(False)   # ← 关闭自动清理资产
    s.destroy()                 # ← 销毁场景！！！
```

做了什么：
- `s.set_auto_cleanup(False)` — 设置 `bAutoCleanup = false`（避免删除磁盘上的 .uasset 文件）
- **`s.destroy()`** — 这是关键！调用 C++ 的 `Destroy()`

---

### 第 5 环：C++ Destroy() 执行销毁

**文件**: [UEMotionScene.cpp L810-L835](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Core/UEMotionScene.cpp#L810-L835)

```cpp
void UUEMotionScene::Destroy()
{
    ClearScene();                          // ① 销毁所有 Mobject
    if (SceneActor) {
        SceneActor->Destroy();             // ② 销毁 SceneActor（相机所在 Actor）
        SceneActor = nullptr;
    }
    if (Renderer) {
        Renderer->OnRenderFinishedDelegate.RemoveAll(this);  // ③ 断开委托
        Renderer = nullptr;
    }
    Camera = nullptr;                      // ④ 清空 Camera
    LevelSequence = nullptr;               // ⑤ 清空 LevelSequence 引用
    SceneWorld = nullptr;                  // ⑥ 清空 World 引用
    bInitialized = false;                  // ⑦ 标记未初始化
    if (bAutoCleanup) {
        CleanupAssets();                   // ⑧ 删除磁盘资产（已被 set_auto_cleanup(False) 关闭）
    }
}
```

---

## 问题根因分析

### 核心原因：MRQ 使用 PIE 模式 + Destroy() 双重打击

**MRQ 渲染机制**（[UEMotionRenderer.cpp L91](file:///c:/Users/42458/Documents/Unreal%20Projects/PluginTest/Plugins/UEMotionPlugin/Source/UEMotionPlugin/Private/Rendering/UEMotionRenderer.cpp#L91)）：

```cpp
UMoviePipelinePIEExecutor* Executor = NewObject<UMoviePipelinePIEExecutor>(QueueSubsystem);
```

使用的是 **`UMoviePipelinePIEExecutor`**（Play-In-Editor），它的生命周期：

```
开始渲染 → 启动 PIE 会话（创建临时 PIE World）→ 在 PIE World 中播放 Sequence → 渲染完成 → 关闭 PIE（销毁 PIE World）
```

**PIE 关闭时发生的事情**：
1. PIE World 被销毁
2. 编辑器 World 可能被刷新/重新加载
3. Sequencer 中绑定的 Possessable 对象引用可能指向已销毁的 PIE World 中的 Actor
4. LevelSequence 内部状态可能因跨 World 播放而变得不一致

**然后 `s.destroy()` 雪上加霜**：
1. `SceneActor->Destroy()` — 显式销毁编辑器中的相机 Actor
2. `ClearScene()` — 销毁所有 Mobject（内部 Actor 也被 Destroy）
3. 所有指针置空 — `LevelSequence = nullptr`, `SceneWorld = nullptr`
4. 之后任何对 `s.level_sequence`、`s.scene_world` 的访问都返回 None/无效对象

### 时间线总结

```
MRQ 开始
  ├─ SaveAssets()          ← 保存当前状态到磁盘 ✅
  ├─ 启动 PIE              ← 创建临时 World
  │   └─ 在 PIE 中播放 Sequence
  │       └─ 绑定的 Actor 是 PIE World 中的副本
  MRQ 结束
  ├─ PIE 关闭              ← PIE World 被销毁 ⚠️ 原始绑定可能受影响
  ├─ OnRenderFinished 广播
  ├─ Python 回调触发
  │   └─ s.destroy()       ← 显式销毁所有对象 ❌❌❌
  └─ 此时 s.level_sequence = None, 所有 Actor 已 Destroy
```

## 可能的修复方向

需要确认用户的实际需求后再决定方案：

| 方案 | 思路 |
|------|------|
| A. 不自动 destroy | 回调中移除 `s.destroy()` 调用，让用户手动控制生命周期 |
| B. destroy 前保存 | 在 `destroy()` 之前先调用一次 `SaveAssets()` 确保数据落盘 |
| C. destroy 后重建 | 提供一个 `reload()` 方法重新加载已保存的 LevelSequence |
| D. 分离渲染与销毁 | 将渲染回调和清理逻辑解耦，不要在渲染回调中直接 destroy |

**请确认你期望的行为**：渲染完成后，Sequencer 中的数据是否还需要继续使用？还是说渲染完就可以完全销毁了？
