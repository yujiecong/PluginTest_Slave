"""
BP_Cube 可见性 Bug 诊断脚本 v3
=============================
深入排查材质系统：通过 cube._ue (UUEMotionMobject) API + Sphere 对比测试
"""

import unreal
import sys
import os
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from uemotion import Scene, ORIGIN

def log_header(tag):
    print("\n" + "=" * 60)
    print(f"  [{tag}]")
    print("=" * 60)

def log_info(msg):
    print(f"  {msg}")

def log_pass(msg):
    print(f"  PASS: {msg}")

def log_fail(msg):
    print(f"  FAIL: {msg}")

def log_warn(msg):
    print(f"  WARN: {msg}")


def safe_call(obj_or_fn, attr_name=None, default=None):
    try:
        if attr_name is not None:
            return getattr(obj_or_fn, attr_name)()
        if callable(obj_or_fn):
            return obj_or_fn()
        return obj_or_fn
    except Exception:
        return default


def find_actor_by_partial_name(world, partial):
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    for a in actors:
        name = a.get_name()
        if partial in name:
            return a
    return None


def inspect_ue_mobject(ue_mob, label=""):
    """通过 UUEMotionMobject 的 Python API 深度检查"""
    log_info(f"[{label}] === UEMotionMobject API 检查 ===")

    name = safe_call(ue_mob, "get_name", default="?")
    opacity = safe_call(ue_mob, "get_opacity", default=-1)
    visible = safe_call(ue_mob, "get_visibility", default=None)
    color = safe_call(ue_mob, "get_color", default=None)
    loc = safe_call(ue_mob, "get_location", default=None)
    scale = safe_call(ue_mob, "get_scale", default=None)
    rot = safe_call(ue_mob, "get_rotation", default=None)

    log_info(f"[{label}]   Name: {name}")
    log_info(f"[{label}]   Opacity: {opacity}")
    log_info(f"[{label}]   Visibility: {visible}")
    if color:
        log_info(f"[{label}]   Color: R={color.r:.3f} G={color.g:.3f} B={color.b:.3f} A={color.a:.3f}")
    if loc:
        log_info(f"[{label}]   Location: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
    if scale:
        log_info(f"[{label}]   Scale: ({scale.x:.2f}, {scale.y:.2f}, {scale.z:.2f})")
    if rot:
        log_info(f"[{label}]   Rotation: P={rot.pitch:.1f} Y={rot.yaw:.1f} R={rot.roll:.1f}")

    origin, extent = unreal.Vector(), unreal.Vector()
    try:
        ue_mob.get_bounds(origin, extent)
        log_info(f"[{label}]   Bounds Origin: ({origin.x:.1f}, {origin.y:.1f}, {origin.z:.1f})")
        log_info(f"[{label}]   Bounds Extent: ({extent.x:.1f}, {extent.y:.1f}, {extent.z:.1f})")
    except Exception as e:
        log_warn(f"[{label}]   Bounds: unavailable ({e})")

    source_path = safe_call(ue_mob, "get_source_asset_path", default="")
    is_asset = safe_call(ue_mob, "is_asset_based", default=False)
    log_info(f"[{label}]   AssetBased: {is_asset}, SourcePath: {source_path}")

    for method_name in ["get_mesh_component", "GetMeshComponent", "get_internal_actor", "GetInternalActor"]:
        if hasattr(ue_mob, method_name):
            try:
                result = getattr(ue_mob, method_name)()
                log_info(f"[{label}]   {method_name}(): {type(result).__name__} = {result}")
                if result:
                    log_info(f"[{label}]     str: {str(result)}")
            except Exception as e:
                log_info(f"[{label}]   {method_name}(): ERROR - {e}")


def inspect_actor_material(actor, label=""):
    if not actor:
        log_warn(f"[{label}] Actor is None")
        return None

    name = safe_call(actor, "get_name", default="?")
    loc = safe_call(actor, "get_actor_location", default=None)
    cls = safe_call(lambda: actor.get_class().get_name(), default="?")

    visible = None
    for vis_fn_name in ["get_visibility", "is_actor_visible", "get_actor_hidden_in_game"]:
        if hasattr(actor, vis_fn_name):
            try:
                v = getattr(actor, vis_fn_name)()
                if vis_fn_name == "get_actor_hidden_in_game":
                    v = not v
                visible = v
                break
            except Exception:
                pass

    log_info(f"[{label}] Actor: {name}")
    log_info(f"[{label}]   Class: {cls}")
    log_info(f"[{label}]   Visible: {visible}")
    if loc:
        log_info(f"[{label}]   Location: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
    else:
        log_warn(f"[{label}]   Location: <unavailable>")

    mesh_comp = safe_call(lambda: actor.find_component_by_class(unreal.StaticMeshComponent), default=None)
    if mesh_comp:
        mv = safe_call(mesh_comp, "get_is_visible", default=None)
        log_info(f"[{label}]   MeshComp visible: {mv}")
        mat = safe_call(lambda: mesh_comp.get_material(0), default=None)
        if mat:
            mat_path = mat.get_path_name()
            mat_cls = mat.get_class().get_name()
            log_info(f"[{label}]   Material[0]: {mat_cls} @ {mat_path}")

            op_val = None
            for api_name, getter in [
                ("scalar", lambda m: m.get_scalar_parameter_value("Opacity")),
                ("editor_only", lambda m: m.get_scalar_parameter_value_editor_only("Opacity")),
            ]:
                try:
                    val = getter(mat)
                    op_val = val
                    log_info(f"[{label}]   Opacity ({api_name}): {val:.4f}")
                    break
                except Exception:
                    pass

            if op_val is None:
                log_warn(f"[{label}]   Cannot read Opacity via any API!")

            try:
                bc = mat.get_vector_parameter_value("BaseColor")
                log_info(f"[{label}]   BaseColor: R={bc.r:.3f} G={bc.g:.3f} B={bc.b:.3f}")
            except Exception:
                try:
                    bc = mat.get_vector_parameter_value_editor_only("BaseColor")
                    log_info(f"[{label}]   BaseColor(editor): R={bc.r:.3f} G={bc.g:.3f} B={bc.b:.3f}")
                except Exception as e:
                    log_warn(f"[{label}]   Cannot read BaseColor: {e}")

            try:
                parent = mat.get_base_material()
                if parent:
                    log_info(f"[{label}]   Parent material: {parent.get_path_name()}")
                    blend = parent.get_blend_mode()
                    log_info(f"[{label}]   Parent blend mode: {blend}")
            except Exception as e:
                log_warn(f"[{label}]   Cannot get base material: {e}")

            return {"opacity": op_val, "mat": mat, "mesh_comp": mesh_comp}
        else:
            log_warn(f"[{label}]   Material[0] is None!")
    else:
        log_warn(f"[{label}]   No StaticMeshComponent found!")

    return None


def test_t1_scene_init():
    log_header("T1 - Scene + Camera Init")
    s = Scene("diag_vis_v3", 1920, 1080, mode="2d")
    log_info(f"Scene created: {s.name}")
    cam_ue = s._ue.get_camera()
    log_info(f"C++ Camera object: {'VALID' if cam_ue else 'NULL'}")
    if cam_ue:
        pos = s.camera.position
        log_info(f"Camera position: ({pos.x:.0f}, {pos.y:.0f}, {pos.z:.0f})")
        log_pass("Camera OK")
    else:
        log_fail("Camera NULL!")
    return s


def test_t2_create_cube(s):
    log_header("T2 - Create Cube (Asset-Based Path)")
    cube = s.cube(color="#3498db", location=ORIGIN)
    if cube is None:
        log_fail("cube() returned None!")
        return None, None

    ue_mob = cube._ue
    inspect_ue_mobject(ue_mob, "T2-Cube-Mobj")

    world = s.scene_world
    actor = find_actor_by_partial_name(world, "BP_Cube") or find_actor_by_partial_name(world, "Mobject_")
    if actor:
        inspect_actor_material(actor, "T2-Cube-Actor")
    else:
        log_warn("Cannot find Cube actor by name!")
        all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
        names = [a.get_name() for a in all_actors]
        log_info(f"All actors ({len(names)}): {names[:20]}")

    return cube, actor


def test_t2b_create_sphere(s):
    log_header("T2b - Create Sphere (Non-Asset Path - 对比基准)")
    sphere = s.sphere(radius=25, color="#e74c3c", location=(-200, 0, 0))
    if sphere is None:
        log_fail("sphere() returned None!")
        return None

    ue_mob = sphere._ue
    inspect_ue_mobject(ue_mob, "T2b-Sphere-Mobj")

    world = s.scene_world
    actor = find_actor_by_partial_name(world, "Mobject_")
    if actor and "Sphere" not in safe_call(actor, "get_name", default=""):
        actor = None
    if not actor:
        all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
        for a in all_actors:
            n = a.get_name()
            if "Mobject_" in n:
                actor = a
                break
    if actor:
        inspect_actor_material(actor, "T2b-Sphere-Actor")
    else:
        log_warn("Cannot find Sphere actor by name!")

    return sphere


def test_t3_check_mpc():
    log_header("T3 - MPC Global Parameter Check")
    mpc_path = "/Game/UEMotion/Materials/MPC_UEMotionFade"
    mpc = unreal.load_asset(mpc_path)
    if not mpc:
        log_fail(f"MPC not found at {mpc_path}")
        return None
    log_info(f"MPC loaded: {safe_call(mpc, 'get_name', default=str(mpc))}")
    params = safe_call(lambda: mpc.get_scalar_parameters(), default=None)
    if params:
        for p in params:
            log_info(f"  Param '{safe_call(p, 'parameter_name', default='?')}' = {safe_call(p, 'default_value', default=0):.4f} (id={safe_call(p, 'id', default=0)})")
    else:
        log_warn("No scalar parameters on MPC (API not available or empty)")

    base_mat_path = "/Game/UEMotion/Materials/M_UEMotionBaseTranslucent"
    base_mat = unreal.load_asset(base_mat_path)
    if base_mat:
        log_info(f"BaseMaterial loaded: {base_mat.get_name()} @ {base_mat.get_path_name()}")
        try:
            blend = base_mat.get_blend_mode()
            log_info(f"  BlendMode: {blend}")
        except Exception as e:
            log_warn(f"  BlendMode unreadable: {e}")
        try:
            two_sided = base_mat.get_two_sided()
            log_info(f"  TwoSided: {two_sided}")
        except Exception as e:
            log_warn(f"  TwoSided unreadable: {e}")
    else:
        log_fail(f"BaseMaterial not found at {base_mat_path}")

    return mpc


def test_t4_manual_opacity_tweak(cube, s):
    log_header("T4 - Manual Opacity Tweak Test")
    if not cube:
        log_fail("No cube!")
        return
    ue_mob = cube._ue
    log_info(f"Before tweak - opacity={ue_mob.get_opacity():.4f}, visible={ue_mob.get_visibility()}")

    log_info(">>> Step 1: set_opacity(0.5) ...")
    ue_mob.set_opacity(0.5)
    time.sleep(0.5)
    log_info(f"After 0.5 - opacity={ue_mob.get_opacity():.4f}, visible={ue_mob.get_visibility()}")

    log_info(">>> Step 2: set_opacity(1.0) ...")
    ue_mob.set_opacity(1.0)
    time.sleep(0.5)
    log_info(f"After 1.0 - opacity={ue_mob.get_opacity():.4f}, visible={ue_mob.get_visibility()}")

    log_info(">>> Step 3: set_opacity(0.0) ...")
    ue_mob.set_opacity(0.0)
    time.sleep(0.3)
    log_info(f"After 0.0 - opacity={ue_mob.get_opacity():.4f}, visible={ue_mob.get_visibility()}")

    log_info(">>> Step 4: set_opacity(1.0) again ...")
    ue_mob.set_opacity(1.0)
    time.sleep(0.3)
    log_info(f"Final - opacity={ue_mob.get_opacity():.4f}, visible={ue_mob.get_visibility()}")


def test_t5_color_and_visibility_toggle(cube, s):
    log_header("T5 - Color Change + Visibility Toggle")
    if not cube:
        log_fail("No cube!")
        return
    ue_mob = cube._ue

    log_info(">>> Test color change...")
    original_color = ue_mob.get_color()
    log_info(f"Original color: R={original_color.r:.3f} G={original_color.g:.3f} B={original_color.b:.3f}")

    ue_mob.set_color(unreal.LinearColor(1.0, 0.0, 0.0, 1.0))
    time.sleep(0.3)
    new_color = ue_mob.get_color()
    log_info(f"After set red: R={new_color.r:.3f} G={new_color.g:.3f} B={new_color.b:.3f}")

    ue_mob.set_color(original_color)
    time.sleep(0.3)
    restored = ue_mob.get_color()
    log_info(f"After restore: R={restored.r:.3f} G={restored.g:.3f} B={restored.b:.3f}")

    log_info(">>> Test visibility toggle...")
    log_info(f"Before hide: visible={ue_mob.get_visibility()}")
    ue_mob.set_visibility(False)
    time.sleep(0.3)
    log_info(f"After hide: visible={ue_mob.get_visibility()}")
    ue_mob.set_visibility(True)
    time.sleep(0.3)
    log_info(f"After show: visible={ue_mob.get_visibility()}")


def test_t6_sequencer_then_check(s, cube):
    log_header("T6 - After Sequencer Binding")
    if not cube:
        log_fail("No cube!")
        return
    from uemotion import RIGHT
    log_info("Adding move animation (triggers Sequencer binding)...")
    cube.move_to(RIGHT * 2, duration=1.0)
    s.play()
    time.sleep(0.5)

    ue_mob = cube._ue
    inspect_ue_mobject(ue_mob, "T6-AfterSeq-Mobj")

    world = s.scene_world
    actor = find_actor_by_partial_name(world, "BP_Cube") or find_actor_by_partial_name(world, "Mobject_")
    if actor:
        inspect_actor_material(actor, "T6-AfterSeq-Actor")


def run_all_tests():
    print("=" * 60)
    print("  BP_Cube Visibility Bug - Diagnostic v3")
    print(f"  Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 60)

    results = {}
    s = None

    try:
        s = test_t1_scene_init()
        results["T1"] = s is not None

        cube, actor = test_t2_create_cube(s)
        results["T2"] = cube is not None

        sphere = test_t2b_create_sphere(s)
        results["T2b"] = sphere is not None

        mpc = test_t3_check_mpc()
        results["T3"] = mpc is not None

        test_t4_manual_opacity_tweak(cube, s)
        results["T4"] = True

        test_t5_color_and_visibility_toggle(cube, s)
        results["T5"] = True

        test_t6_sequencer_then_check(s, cube)
        results["T6"] = True

        log_info("[T7] Render Frame - SKIPPED (no rendering)")
        results["T7"] = True

    except Exception as e:
        log_fail(f"EXCEPTION: {e}")
        import traceback
        traceback.print_exc()

    finally:
        if s:
            log_header("Cleanup")
            s.destroy()
            log_info("Scene destroyed.")

    print("\n" + "=" * 60)
    print("  SUMMARY")
    print("=" * 60)
    desc = {
        "T1": "Scene+Camera",
        "T2": "Create Cube (Asset)",
        "T2b": "Create Sphere (Direct)",
        "T3": "MPC+Material Check",
        "T4": "Opacity Tweak",
        "T5": "Color+Visibility Toggle",
        "T6": "Sequencer Bind",
        "T7": "Render Frame",
    }
    for tid, ok in results.items():
        st = "PASS" if ok else "FAIL"
        print(f"  [{tid}] {desc.get(tid,'?'):25s} {st}")
    print("=" * 60)


if __name__ == "__main__":
    run_all_tests()
