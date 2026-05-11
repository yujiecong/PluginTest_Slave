# UE Sequencer C++ 属性动画经验总结

## 日期
2026-05-11

## 项目
UEMotion Plugin — Opacity 属性 Sequencer 动画化

## 问题
代码动态创建 `UMovieSceneFloatTrack` 来动画化 `AUEMotionMobjectActor::Opacity` 属性。
Sequencer UI 中轨道数值正确，但 **拖动时间轴时 Level 中 Actor 不响应**。

---

## 根因与修复（3 条关键规则）

### 规则 1：Setter 函数必须加 `CallInEditor`

如果一个属性有副作用（如更新材质），不能只靠 Sequencer 直接写内存值，
必须让 Sequencer 调用 setter 函数。而引擎在编辑器模式下调用 setter 需要 `CallInEditor` 标识。

```cpp
// ❌ 错误
UFUNCTION(BlueprintCallable, Category = "UEMotion")
void SetOpacity(float InOpacity);

// ✅ 正确
UFUNCTION(BlueprintCallable, CallInEditor, Category = "UEMotion")
void SetOpacity(float InOpacity);
```

**来源**: [Epic 官方 Knowledge Base — Sequencer Setter Functions](https://community.epicgames.com/knowledge-base/sequencer-setter-functions)

---

### 规则 2：UPROPERTY 需要 `BlueprintSetter` 元数据

`BlueprintSetter` 让 Blueprint VM 知道属性的自定义 setter。
此外，函数名必须是 `Set[PropertyName]` 格式，Sequencer 按此命名约定发现 setter。

```cpp
// ❌ 错误
UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "UEMotion")
float Opacity;

// ✅ 正确
UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "UEMotion",
    meta = (BlueprintSetter = "SetOpacity"))
float Opacity;
```

| 触发方式 | BlueprintSetter | Set+PropertyName 命名 |
|---------|:-:|:-:|
| Blueprint 中设置 | ✅ | ❌ |
| Sequencer 评估 | ❌ | ✅ |
| 两者同时使用 | ✅ | ✅ |

---

### 规则 3：动态修改的 LevelSequence 必须设 `Volatile` 标志

如果 LevelSequence 在创建后又动态添加 Track/Key，必须设置此标志，
否则引擎缓存空的评估模板，新 Track 不会被评估。

```cpp
// LevelSequence 创建后
NewSequence->SetSequenceFlags(
    EMovieSceneSequenceFlags::Volatile |
    EMovieSceneSequenceFlags::BlockingEvaluation
);
```

- **Volatile**：标志该序列可在运行时动态变化，引擎必须在每次评估前检查并重新编译模板
- **BlockingEvaluation**：确保每次更新都完整评估并应用状态

**来源**: [UE 官方论坛 — Editing sequencer level sequence at runtime](https://forums.unrealengine.com/t/editing-sequencer-level-sequence-at-runtime/225188)

---

## 架构总结

```
Sequencer 评估 UMovieSceneFloatTrack("Opacity")
    ↓
引擎检测到 SetOpacity 函数存在（命名约定）
    ↓
CallInEditor 标识允许编辑器模式调用  ← 规则 1
    ↓
BlueprintSetter 链接属性到 setter   ← 规则 2
    ↓
Volatile 标志确保模板不过期         ← 规则 3
    ↓
调用 AUEMotionMobjectActor::SetOpacity(value)
    ↓
DynamicMaterial->SetScalarParameterValue("Opacity", value)
    ↓
✅ 材质更新 → 视觉实时变化
```

---

## 参考文献

| # | 来源 | 标题 | URL |
|---|------|------|-----|
| 1 | Epic 官方 KB | Sequencer Setter Functions | https://community.epicgames.com/knowledge-base/sequencer-setter-functions |
| 2 | Epic 官方文档 | Property Specifiers | https://dev.epicgames.com/documentation/en-us/unreal-engine/property-specifiers |
| 3 | UE 官方 API | EMovieSceneSequenceFlags | https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/MovieScene/EMovieSceneSequenceFlags |
| 4 | UE 官方论坛 | Runtime sequence editing (Volatile) | https://forums.unrealengine.com/t/editing-sequencer-level-sequence-at-runtime/225188 |
| 5 | Qiita | BlueprintSetter vs LevelSequence | https://qiita.com/com04/items/a0ac7af84071dc8f78c2 |